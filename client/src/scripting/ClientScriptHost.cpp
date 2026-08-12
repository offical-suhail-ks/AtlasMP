// client/src/scripting/ClientScriptHost.cpp
#include "ClientScriptHost.h"
#include "../core/Logger.h"

namespace Atlas {

ClientScriptHost::ClientScriptHost(NativeInvoker* natives,
                                    NetworkClient* network,
                                    NUIManager*    nui)
    : m_natives(natives), m_network(network), m_nui(nui) {}

bool ClientScriptHost::Initialize() {
    // TODO: initialize Lua/JS runtimes for client side
    // Similar to server ScriptHost but runs inside GTA V process
    Logger::Info("[ClientScripts] Script host initialized (stub)");
    m_initialized = true;
    return true;
}

void ClientScriptHost::Shutdown() {
    m_initialized = false;
}

void ClientScriptHost::Tick(float /*deltaTime*/) {
    // TODO: drive script coroutines / timer callbacks
}

bool ClientScriptHost::LoadScript(const std::string& path,
                                   const std::string& language)
{
    Logger::Info("[ClientScripts] LoadScript stub: %s (%s)",
                 path.c_str(), language.c_str());
    return true;
}

void ClientScriptHost::TriggerEvent(const std::string& name,
                                     const std::vector<std::string>& /*args*/)
{
    Logger::Debug("[ClientScripts] TriggerEvent: %s", name.c_str());
}

} // namespace Atlas
