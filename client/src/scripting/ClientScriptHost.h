#pragma once
// client/src/scripting/ClientScriptHost.h
// Hosts client-side scripts (Lua/JS running inside GTA V process)
#include <string>
#include <vector>

namespace Atlas {
class NativeInvoker;
class NetworkClient;
class NUIManager;

class ClientScriptHost {
public:
    ClientScriptHost(NativeInvoker* natives,
                     NetworkClient* network,
                     NUIManager*    nui);
    ~ClientScriptHost() = default;

    bool Initialize();
    void Shutdown();
    void Tick(float deltaTime);

    /// Load and execute a client-side script file
    bool LoadScript(const std::string& path, const std::string& language);

    /// Trigger a named event in all client scripts
    void TriggerEvent(const std::string& name,
                      const std::vector<std::string>& args = {});

private:
    NativeInvoker* m_natives;
    NetworkClient* m_network;
    NUIManager*    m_nui;
    bool           m_initialized = false;
};

} // namespace Atlas
