// client/src/core/dllmain.cpp
// AtlasMP Client — ScriptHookV ASI plugin (Option A).
//
// Deployment (all in the GTA V folder):
//   dinput8.dll         (Ultimate ASI Loader)
//   ScriptHookV.dll     (Alexander Blade)
//   AtlasMP-Client.asi  (this build)
//   atlasmp-client.toml (config)
//
// ScriptHookV loads first, initializes native access, then loads our .asi and
// calls ScriptMain() on its script thread. We drive Client::Tick() from there,
// so every native call runs on the correct thread with ScriptHookV ready.

#include "Client.h"
#include "Logger.h"
#include "../hooks/NativeInvoker.h"
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>

// ScriptHookV SDK
#include <main.h>

static Atlas::Client* g_client  = nullptr;
static HMODULE        g_hModule = nullptr;

// ── Config (unchanged parser) ────────────────────────────────────────────────
namespace {
struct Cfg{std::string host="127.0.0.1";uint16_t port=7788;std::string name="AtlasPlayer",pw;bool ac=true;};
static std::string Tr(std::string v){v.erase(v.begin(),std::find_if(v.begin(),v.end(),[](unsigned char c){return!std::isspace(c);}));v.erase(std::find_if(v.rbegin(),v.rend(),[](unsigned char c){return!std::isspace(c);}).base(),v.end());return v;}
static std::string Lo(std::string v){std::transform(v.begin(),v.end(),v.begin(),[](unsigned char c){return(char)std::tolower(c);});return v;}
static std::string Uq(std::string v){if(v.size()>=2&&((v.front()=='"'&&v.back()=='"')||(v.front()=='\''&&v.back()=='\''))){v=v.substr(1,v.size()-2);}return v;}
static bool LoadCfg(const std::string& path,Cfg& c){
    std::ifstream f(path);if(!f)return false;
    std::string sec,line;
    while(std::getline(f,line)){
        auto p=line.find('#'),q=line.find(';');size_t cut=std::string::npos;
        if(p!=std::string::npos)cut=p;if(q!=std::string::npos&&(cut==std::string::npos||q<cut))cut=q;
        if(cut!=std::string::npos)line=line.substr(0,cut);line=Tr(line);if(line.empty())continue;
        if(line.front()=='['&&line.back()==']'){sec=Lo(Tr(line.substr(1,line.size()-2)));continue;}
        if(!sec.empty()&&sec!="connection")continue;
        auto eq=line.find('=');if(eq==std::string::npos)continue;
        auto k=Lo(Tr(line.substr(0,eq)));auto v=Uq(Tr(line.substr(eq+1)));
        if(k=="host"&&!v.empty())c.host=v;
        else if(k=="port"){char* e=nullptr;auto n=strtoul(v.c_str(),&e,10);if(e!=v.c_str()&&n>=1&&n<=65535)c.port=(uint16_t)n;}
        else if(k=="name"&&!v.empty())c.name=v;
        else if(k=="password")c.pw=v;
        else if(k=="auto_connect"){auto lv=Lo(v);c.ac=(lv=="true"||lv=="1"||lv=="yes");}
    }
    return true;
}
static std::string DirOf(HMODULE m){wchar_t p[MAX_PATH]{};GetModuleFileNameW(m,p,MAX_PATH);return std::filesystem::path(p).parent_path().string();}
static std::string FindCfg(){
    namespace fs=std::filesystem;
    for(auto d:{DirOf(nullptr),DirOf(g_hModule)}){std::string p=d+"\\atlasmp-client.toml";if(fs::exists(p))return p;}
    return DirOf(g_hModule)+"\\atlasmp-client.toml";
}
}

// ── AtlasMP Boot Sequence ─────────────────────────────────────────────────────
// Runs once after story mode loads. Skips the intro, sets a freemode character,
// and teleports to the AtlasMP spawn point. Everything runs on ScriptHookV's
// script thread so native calls are safe.
static void AtlasBootSequence() {
    constexpr uint64_t PLAYER_PED_ID    = 0xD80958FC74E988A6;
    constexpr uint64_t DOES_ENTITY_EXIST= 0x7239B21A38F536BA;
    constexpr uint64_t SET_ENTITY_COORDS= 0x06843DA7060A026B;
    constexpr uint64_t SET_ENTITY_HDG   = 0x8E2530AA8ADA980E;
    constexpr uint64_t GET_HASH_KEY     = 0xD24D37CC275948CC;
    constexpr uint64_t REQUEST_MODEL    = 0x963D27A58DF860AC;
    constexpr uint64_t HAS_MODEL_LOADED = 0x98A4EB5D89A0C952;
    constexpr uint64_t NO_LONGER_NEEDED = 0xE532F5D78798DAAB;
    // SET_PLAYER_MODEL from FiveM docs — if game closes here, comment it out.
    constexpr uint64_t SET_PLAYER_MODEL = 0x00A1CADD00108836;

    if (!g_client) return;
    auto* nv = g_client->GetNatives();
    if (!nv) return;

    Atlas::Logger::Info("[Boot] Waiting for game world...");

    // Wait for player ped to exist (world is ready, story mode loaded).
    for (int i = 0; i < 30000; ++i) {
        int ped = nv->Call<int>(PLAYER_PED_ID);
        if (ped != 0 && nv->Call<int>(DOES_ENTITY_EXIST, ped)) break;
        WAIT(0);
    }
    // 3 extra seconds — lets any intro cutscene start so we cleanly override it.
    for (int i = 0; i < 180; ++i) WAIT(0);

    int ped = nv->Call<int>(PLAYER_PED_ID);
    if (ped == 0) { Atlas::Logger::Error("[Boot] No player ped — abort"); return; }

    Atlas::Logger::Info("[Boot] World ready — applying AtlasMP character");

    // Set freemode male character model (GTA Online freemode).
    uint32_t model = nv->Call<uint32_t>(GET_HASH_KEY, "mp_m_freemode_01");
    nv->Call<void>(REQUEST_MODEL, model);
    for (int i = 0; i < 500 && !nv->Call<int>(HAS_MODEL_LOADED, model); ++i) WAIT(0);
    if (nv->Call<int>(HAS_MODEL_LOADED, model)) {
        nv->Call<void>(SET_PLAYER_MODEL, 0, model); // 0 = local player index
        nv->Call<void>(NO_LONGER_NEEDED, model);
        Atlas::Logger::Info("[Boot] Character: mp_m_freemode_01");
        for (int i = 0; i < 30; ++i) WAIT(0); // let model swap settle
        ped = nv->Call<int>(PLAYER_PED_ID);     // re-get ped after swap
    }

    // Teleport to AtlasMP spawn — Pillbox Hill, central LS, open area.
    if (ped != 0) {
        nv->Call<void>(SET_ENTITY_COORDS, ped, -269.4f, -955.3f, 31.2f, 0, 0, 0, 1);
        nv->Call<void>(SET_ENTITY_HDG,    ped, 90.0f);
        Atlas::Logger::Info("[Boot] Spawned at AtlasMP location (-269, -955, 31)");
    }

    Atlas::Logger::Info("[Boot] Done — handing off to AtlasMP loop");
}

// ── ScriptHookV entry ────────────────────────────────────────────────────────
// Called by ScriptHookV on its script thread. Natives are safe to call here.
static void ScriptMain() {
    // One-time init.
    std::string log = DirOf(g_hModule) + "\\AtlasMP-client.log";
    std::string cfg = FindCfg();

    Atlas::Logger::Init(log);
    Atlas::Logger::Info("[ASI] AtlasMP Client v" ATLAS_VERSION_STRING " (ScriptHookV mode)");
    Atlas::Logger::Info("[ASI] Log: %s", log.c_str());

    g_client = new Atlas::Client();
    if (!g_client->Initialize()) {
        Atlas::Logger::Error("[ASI] Client init failed");
        delete g_client; g_client = nullptr;
        return;
    }
    Atlas::Logger::Info("[ASI] Client initialized");

    // ── Boot sequence: skip story intro, set character, teleport to spawn ─────
    AtlasBootSequence();

    Cfg c;
    if (LoadCfg(cfg, c) && c.ac) {
        Atlas::Logger::Info("[ASI] Connecting to %s:%u...", c.host.c_str(), (unsigned)c.port);
        g_client->Connect(c.host, c.port, c.name, c.pw);
    }

    Atlas::Logger::Info("[ASI] Entering script loop");
    // ScriptHookV script loop: WAIT(0) yields one game frame. Client::Tick()
    // runs here every frame, on the script thread, with native access ready.
    while (true) {
        if (g_client) g_client->Tick();
        WAIT(0);
    }
}

// ── DllMain ──────────────────────────────────────────────────────────────────
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        // Register our script with ScriptHookV. It will call ScriptMain() on
        // its own script thread once the game is ready.
        scriptRegister(hModule, ScriptMain);
        break;
    case DLL_PROCESS_DETACH:
        scriptUnregister(hModule);
        if (g_client) { g_client->Shutdown(); delete g_client; g_client = nullptr; }
        break;
    }
    return TRUE;
}