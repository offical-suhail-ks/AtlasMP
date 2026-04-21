#pragma once
// client/src/core/Client.h
// Main client class — owns all client-side subsystems

#include <memory>
#include <atomic>
#include <string>

namespace Atlas {

// Forward declarations
class HookManager;
class NativeInvoker;
class NetworkClient;
class SyncManager;
class ClientScriptHost;
class NUIManager;
class LocalPlayer;

class Client {
public:
    Client();
    ~Client();

    bool Initialize();
    void Shutdown();
    void Tick();           // Called from hooked scrThread every frame

    bool IsRunning() const { return m_running.load(); }

    // Subsystem accessors
    HookManager*    GetHooks()    const { return m_hooks.get(); }
    NativeInvoker*  GetNatives()  const { return m_natives.get(); }
    NetworkClient*  GetNetwork()  const { return m_network.get(); }
    SyncManager*    GetSync()     const { return m_sync.get(); }
    NUIManager*     GetNUI()      const { return m_nui.get(); }
    LocalPlayer*    GetPlayer()   const { return m_localPlayer.get(); }

    // Singleton access (client is a singleton — one per process)
    static Client* Get() { return s_instance; }

    // Server connection
    bool Connect(const std::string& host, uint16_t port,
                 const std::string& name, const std::string& password = "");
    void Disconnect(const std::string& reason = "");
    bool IsConnected() const;

private:
    bool InitHooks();
    bool InitNatives();
    bool InitNetwork();

    std::atomic<bool> m_running{false};
    bool m_initialized = false;

    std::unique_ptr<HookManager>   m_hooks;
    std::unique_ptr<NativeInvoker> m_natives;
    std::unique_ptr<NetworkClient> m_network;
    std::unique_ptr<SyncManager>   m_sync;
    std::unique_ptr<ClientScriptHost> m_scripts;
    std::unique_ptr<NUIManager>    m_nui;
    std::unique_ptr<LocalPlayer>   m_localPlayer;

    static Client* s_instance;
};

} // namespace Atlas
