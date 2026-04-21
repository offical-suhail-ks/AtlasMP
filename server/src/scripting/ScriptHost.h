#pragma once
// server/src/scripting/ScriptHost.h
#include "../core/Config.h"
#include "../core/EventBus.h"
#include "../../../sdk/include/IScriptRuntime.h"
#include <memory>
#include <string>
#include <vector>

namespace Atlas {
class PlayerManager;
class NetworkServer;

class ScriptHost {
public:
    ScriptHost(const Config& cfg, EventBus* events,
               PlayerManager* players, NetworkServer* network);
    ~ScriptHost();

    bool Initialize();
    void Tick(float dt);

    // Load a resource and execute its scripts
    IResource* LoadResource(const std::string& name,
                             const std::string& basePath,
                             const std::string& lang = "auto");

    // Fire an event across all runtimes
    void TriggerEvent(const std::string& name, const ScriptArgs& args = {});

private:
    struct RuntimeModule;

    bool LoadRuntime(const std::string& dllPath);
    void WireNetworkEvents();
    void WireServerEvents();
    void LoadResourceScripts(IResource* resource, const std::string& basePath,
                              const std::string& lang,
                              const std::vector<std::string>& files);
    IResource* FindRuntimeForLang(const std::string& lang);

    const Config&   m_config;
    EventBus*       m_events;
    PlayerManager*  m_players;
    NetworkServer*  m_network;

    std::vector<std::unique_ptr<RuntimeModule>> m_runtimes;
    float m_tickAccum = 0.0f;
};

} // namespace Atlas
