// client/src/core/Client.cpp
#include "Client.h"
#include "Logger.h"
#include "../hooks/HookManager.h"
#include "../hooks/NativeInvoker.h"
#include "../network/NetworkClient.h"
#include "../sync/SyncManager.h"
#include "../ui/NUIManager.h"
#include "../ui/WindowBranding.h"
#include "../sync/LocalPlayer.h"
#include "../scripting/ClientScriptHost.h"

namespace Atlas {

Client* Client::s_instance = nullptr;

Client::Client() { s_instance = this; }
Client::~Client() { Shutdown(); s_instance = nullptr; }

bool Client::Initialize() {
    if (m_initialized) return true;

    Logger::Info("Client::Initialize()");

    m_hooks   = std::make_unique<HookManager>();
    m_natives = std::make_unique<NativeInvoker>();
    m_network = std::make_unique<NetworkClient>();
    m_sync    = std::make_unique<SyncManager>(m_natives.get(), m_network.get());
    m_nui     = std::make_unique<NUIManager>();
    m_localPlayer = std::make_unique<LocalPlayer>(m_natives.get());

    // Initialize window branding (change title and icon from GTA5 to AtlasMP)
    WindowBranding::Initialize();

    if (!m_hooks->Initialize()) {
        Logger::Error("HookManager failed to initialize");
        return false;
    }

    if (!m_natives->Initialize()) {
        Logger::Error("NativeInvoker failed to initialize");
        return false;
    }

    m_sync->Initialize();

    m_initialized = true;
    m_running     = true;
    Logger::Info("Client initialized successfully");
    return true;
}

void Client::Shutdown() {
    if (!m_initialized) return;
    m_running = false;
    Logger::Info("Client shutting down...");

    if (m_sync)    m_sync->Shutdown();
    if (m_network) m_network->Disconnect();
    if (m_hooks)   m_hooks->Shutdown();

    m_initialized = false;
    Logger::Info("Client shutdown complete.");
}

void Client::Tick() {
    if (!m_running) return;
    // This is called from the hooked scrThread every game frame
    if (m_sync) m_sync->Tick(1.0f / 60.0f);
    if (m_network) m_network->Poll();
}

bool Client::Connect(const std::string& host, uint16_t port,
                      const std::string& name, const std::string& password)
{
    Logger::Info("Connecting to %s:%u...", host.c_str(), static_cast<unsigned>(port));
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

void Client::Disconnect(const std::string& reason) {
    Logger::Info("Disconnecting: %s", reason.c_str());
    if (m_network) m_network->Disconnect();
}

bool Client::IsConnected() const {
    return m_network && m_network->IsConnected();
}

} // namespace Atlas
