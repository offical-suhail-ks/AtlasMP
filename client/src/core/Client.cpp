// client/src/core/Client.cpp
#include "Client.h"
#include "../hooks/HookManager.h"
#include "../hooks/NativeInvoker.h"
#include "../network/NetworkClient.h"
#include "../scripting/ClientScriptHost.h"
#include "../sync/LocalPlayer.h"
#include "../sync/SyncManager.h"
#include "../ui/NUIManager.h"
#include "../ui/WindowBranding.h"
#include "Logger.h"


namespace Atlas {

// ── Ped proof (defined in sync/RemotePed_1604.cpp) ───────────────────────────
// Set to 1 to run the no-networking single-ped test; 0 for normal operation.
#define ATLAS_RUN_PED_PROOF 0  // proof done - real networked sync now active
void Atlas_ProofTick(NativeInvoker *nv, float deltaTime);

Client *Client::s_instance = nullptr;

Client::Client() { s_instance = this; }
Client::~Client() {
  Shutdown();
  s_instance = nullptr;
}

bool Client::Initialize() {
  if (m_initialized)
    return true;

  Logger::Info("Client::Initialize()");

  m_hooks = std::make_unique<HookManager>();
  m_natives = std::make_unique<NativeInvoker>();
  m_network = std::make_unique<NetworkClient>();
  m_sync = std::make_unique<SyncManager>(m_natives.get(), m_network.get());
  m_nui = std::make_unique<NUIManager>();
  m_localPlayer = std::make_unique<LocalPlayer>(m_natives.get());
  Logger::Info("[Init] subsystems constructed");

  // WindowBranding (title-bar rename) is cosmetic and can be unsafe to call
  // from ScriptHookV's script thread during startup — skip it. Re-enable later
  // once the core works if you want the "AtlasMP" title.
  // WindowBranding::Initialize();
  Logger::Info("[Init] window branding skipped (ASI mode)");

  Logger::Info("[Init] initializing hooks...");
  if (!m_hooks->Initialize()) {
    Logger::Warn("HookManager init returned false — continuing (ScriptHookV drives tick)");
  }
  Logger::Info("[Init] hooks done");

  Logger::Info("[Init] initializing natives...");
  if (!m_natives->Initialize()) {
    Logger::Error("NativeInvoker failed to initialize");
    return false;
  }
  Logger::Info("[Init] natives done");

  m_sync->Initialize();

  m_initialized = true;
  m_running = true;
  Logger::Info("Client initialized successfully");
  return true;
}

void Client::Shutdown() {
  if (!m_initialized)
    return;
  m_running = false;
  Logger::Info("Client shutting down...");

  if (m_sync)
    m_sync->Shutdown();
  if (m_network)
    m_network->Disconnect();
  if (m_hooks)
    m_hooks->Shutdown();

  m_initialized = false;
  Logger::Info("Client shutdown complete.");
}

void Client::Tick() {
  if (!m_running)
    return;
  // This is called from the hooked scrThread every game frame — safe to call
  // GTA V natives here (correct thread).

#if ATLAS_RUN_PED_PROOF
  // No-networking proof: spawns one ped in front of the player and moves it.
  // Once verified in-game, set ATLAS_RUN_PED_PROOF to 0.
  if (m_natives)
    Atlas_ProofTick(m_natives.get(), 1.0f / 60.0f);
#endif

  if (m_sync)
    m_sync->Tick(1.0f / 60.0f);
  if (m_network)
    m_network->Poll();
}

bool Client::Connect(const std::string &host, uint16_t port,
                     const std::string &name, const std::string &password) {
  Logger::Info("Connecting to %s:%u...", host.c_str(),
               static_cast<unsigned>(port));
  if (!m_network) {
    Logger::Error("Connect failed: network subsystem not initialized");
    return false;
  }

  const bool ok = m_network->Connect(host, port, name, password);
  if (!ok) {
    Logger::Error("Connect failed");
  }
  return ok;
}

void Client::Disconnect(const std::string &reason) {
  Logger::Info("Disconnecting: %s", reason.c_str());
  if (m_network)
    m_network->Disconnect();
}

bool Client::IsConnected() const {
  return m_network && m_network->IsConnected();
}

} // namespace Atlas