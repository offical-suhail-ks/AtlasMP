// server/src/scripting/ScriptHost.cpp
// Dynamically loads runtime DLLs (AtlasMP-Lua.dll, AtlasMP-JS.dll etc.)
// and routes game events into them.

#include "ScriptHost.h"
#include "../core/Logger.h"
#include "../core/EventBus.h"
#include "../../include/PlayerManager.h"
#include "../network/NetworkServer.h"
#include "../../../sdk/include/IScriptRuntime.h"
#include "../../../shared/include/Packets.h"

#include <filesystem>
#include <functional>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
using DylibHandle = HMODULE;
static DylibHandle DylibOpen(const std::string& path) {
    return LoadLibraryA(path.c_str());
}
static void* DylibSym(DylibHandle h, const char* sym) {
    return (void*)GetProcAddress(h, sym);
}
static void DylibClose(DylibHandle h) { FreeLibrary(h); }
#else
#  include <dlfcn.h>
using DylibHandle = void*;
static DylibHandle DylibOpen(const std::string& path) {
    return dlopen(path.c_str(), RTLD_LAZY);
}
static void* DylibSym(DylibHandle h, const char* sym) { return dlsym(h, sym); }
static void DylibClose(DylibHandle h) { dlclose(h); }
#endif

namespace fs = std::filesystem;

namespace Atlas {

using CreateRuntimeFn = IScriptRuntime*(*)();

// ── RuntimeModule: one loaded runtime DLL ────────────────────────────────────
struct ScriptHost::RuntimeModule {
    DylibHandle       handle   = nullptr;
    IScriptRuntime*   runtime  = nullptr;
    std::string       id;

    ~RuntimeModule() {
        if (runtime) { runtime->Shutdown(); delete runtime; runtime = nullptr; }
        if (handle)  { DylibClose(handle); handle = nullptr; }
    }
};

ScriptHost::ScriptHost(const Config& cfg, EventBus* events,
                       PlayerManager* players, NetworkServer* network)
    : m_config(cfg), m_events(events), m_players(players), m_network(network)
{}

ScriptHost::~ScriptHost() = default;

bool ScriptHost::Initialize() {
    Logger::Info("[ScriptHost] Initializing...");

    // Discover and load runtime DLLs from ./runtimes/ next to the server binary
    fs::path runtimesDir = fs::current_path() / "runtimes";
    if (!fs::exists(runtimesDir)) runtimesDir = fs::current_path(); // fallback

    for (const auto& name : {"AtlasMP-Lua", "AtlasMP-JS"}) {
#ifdef _WIN32
        fs::path dllPath = runtimesDir / (std::string(name) + ".dll");
#else
        fs::path dllPath = runtimesDir / (std::string("lib") + name + ".so");
#endif
        if (!fs::exists(dllPath)) {
            Logger::Debug("[ScriptHost] Runtime not found: {}", dllPath.string());
            continue;
        }
        LoadRuntime(dllPath.string());
    }

    if (m_runtimes.empty()) {
        Logger::Warn("[ScriptHost] No runtimes loaded — scripting disabled");
    } else {
        Logger::Info("[ScriptHost] {} runtime(s) loaded", m_runtimes.size());
    }

    WireNetworkEvents();
    WireServerEvents();
    return true;
}

bool ScriptHost::LoadRuntime(const std::string& dllPath) {
    DylibHandle h = DylibOpen(dllPath);
    if (!h) {
        Logger::Error("[ScriptHost] Failed to load runtime: {}", dllPath);
        return false;
    }

    auto createFn = (CreateRuntimeFn)DylibSym(h, "Atlas_CreateRuntime");
    if (!createFn) {
        Logger::Error("[ScriptHost] Atlas_CreateRuntime not found in {}", dllPath);
        DylibClose(h);
        return false;
    }

    IScriptRuntime* rt = createFn();
    if (!rt) {
        DylibClose(h);
        return false;
    }

    if (!rt->Initialize(ATLAS_SDK_VERSION)) {
        Logger::Error("[ScriptHost] Runtime Initialize() failed: {}", dllPath);
        delete rt;
        DylibClose(h);
        return false;
    }

    Logger::Info("[ScriptHost] Loaded runtime: {} ({})",
        rt->GetRuntimeId(), rt->GetRuntimeVersion());

    auto mod = std::make_unique<RuntimeModule>();
    mod->handle  = h;
    mod->runtime = rt;
    mod->id      = rt->GetRuntimeId();
    m_runtimes.push_back(std::move(mod));
    return true;
}

void ScriptHost::WireNetworkEvents() {
    if (!m_network) return;

    // Chat packet → playerChat event in scripts
    m_network->OnPacket(Packets::PacketType::PLAYER_CHAT,
        [this](PlayerId id, Packets::PacketType, const uint8_t* data, size_t len) {
            if (len < sizeof(Packets::PlayerChatPacket)) return;
            const auto& pkt = *reinterpret_cast<const Packets::PlayerChatPacket*>(data);
            std::string msg(pkt.message, strnlen(pkt.message, sizeof(pkt.message)));

            Logger::Info("[Chat] Player {}: {}", id, msg);

            // Echo to all clients
            m_network->Broadcast(Packets::PacketType::PLAYER_CHAT, data, len);

            // Fire event in scripts
            TriggerEvent("playerChat", {
                ScriptValue::Int(id),
                ScriptValue::String(msg)
            });
        });

    // Player join → playerJoin event
    m_network->OnPacket(Packets::PacketType::PLAYER_JOIN,
        [this](PlayerId id, Packets::PacketType, const uint8_t* data, size_t len) {
            if (len < sizeof(Packets::PlayerJoinPacket)) return;
            const auto& pkt = *reinterpret_cast<const Packets::PlayerJoinPacket*>(data);
            std::string name(pkt.name, strnlen(pkt.name, sizeof(pkt.name)));
            Logger::Info("[ScriptHost] playerJoin: {} ({})", name, id);
            TriggerEvent("playerJoin", { ScriptValue::Int(id) });
        });

    // Player leave → playerLeave event
    m_network->OnPacket(Packets::PacketType::PLAYER_LEAVE,
        [this](PlayerId id, Packets::PacketType, const uint8_t* /*data*/, size_t /*len*/) {
            Logger::Info("[ScriptHost] playerLeave: {}", id);
            TriggerEvent("playerLeave", { ScriptValue::Int(id) });
        });
}

void ScriptHost::WireServerEvents() {
    if (!m_events) return;
    // Subscribe to server tick event to call script tick timers
    m_events->Subscribe<ServerTickEvent>([this](const ServerTickEvent& e) {
        Tick(e.deltaTime);
    });
}

void ScriptHost::LoadResourceScripts(IResource* resource,
                                      const std::string& basePath,
                                      const std::string& lang,
                                      const std::vector<std::string>& files)
{
    for (const auto& file : files) {
        std::string fullPath = basePath + "/" + file;
        resource->Execute(fullPath);
    }
}

IResource* ScriptHost::FindRuntimeForLang(const std::string& lang) {
    // Returns the first loaded resource for the given language
    // Caller should use the runtime directly
    return nullptr; // handled per-resource in LoadResource
}

void ScriptHost::TriggerEvent(const std::string& name, const ScriptArgs& args) {
    for (auto& mod : m_runtimes) {
        if (mod->runtime)
            mod->runtime->TriggerEvent(name, args);
    }
}

void ScriptHost::Tick(float dt) {
    m_tickAccum += dt;
    // Process script timers at 30 Hz
    if (m_tickAccum >= (1.0f / 30.0f)) {
        m_tickAccum = 0.0f;
        for (auto& mod : m_runtimes)
            if (mod->runtime) mod->runtime->Tick();
    }
}

IResource* ScriptHost::LoadResource(const std::string& name,
                                     const std::string& basePath,
                                     const std::string& lang)
{
    // Find appropriate runtime
    for (auto& mod : m_runtimes) {
        if (mod->runtime) {
            const char* rid = mod->runtime->GetRuntimeId();
            bool match = (lang == "lua"  && std::string(rid) == "lua")  ||
                         (lang == "js"   && std::string(rid) == "js")   ||
                         (lang == "auto");
            if (match) {
                auto* res = mod->runtime->LoadResource(name, basePath);
                if (res) {
                    Logger::Info("[ScriptHost] Loaded resource '{}' via {} runtime",
                        name, rid);
                    return res;
                }
            }
        }
    }
    Logger::Warn("[ScriptHost] No runtime found for language '{}' (resource: {})",
        lang, name);
    return nullptr;
}

} // namespace Atlas
