#pragma once
// runtimes/lua/src/LuaRuntime.h
#include "../../../sdk/include/IScriptRuntime.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

struct lua_State;

namespace Atlas {

struct LuaEnvironment {
    lua_State*  L            = nullptr;
    std::string resourceName;
    std::string basePath;
    bool        running      = false;
    ~LuaEnvironment();
};

class LuaResource : public IResource {
public:
    LuaResource(const std::string& name, const std::string& path,
                LuaEnvironment* env)
        : m_name(name), m_path(path), m_env(env) {}

    const std::string& GetName() const override { return m_name; }
    bool Execute(const std::string& filePath) override;
    ScriptValue CallExport(const std::string& funcName,
                            const ScriptArgs& args) override;

    LuaEnvironment* m_env;
private:
    std::string m_name, m_path;
};

class LuaRuntime : public IScriptRuntime {
public:
    bool        Initialize(uint32_t sdkVersion) override;
    void        Shutdown()                       override;
    void        Tick()                           override;

    const char* GetRuntimeId()      const override { return "lua"; }
    const char* GetRuntimeVersion() const override { return "LuaJIT-2.1.0"; }

    IResource*  LoadResource(const std::string& name,
                              const std::string& basePath) override;
    void        UnloadResource(IResource* resource) override;

    void        TriggerEvent(const std::string& name,
                              const ScriptArgs& args = {}) override;
    void        TriggerResourceEvent(IResource* resource,
                                      const std::string& name,
                                      const ScriptArgs& args = {}) override;
    void        RegisterNative(const std::string& name,
                                ScriptCallback cb) override;

private:
    bool m_initialized = false;
    std::vector<std::unique_ptr<LuaEnvironment>> m_environments;
    std::vector<std::unique_ptr<LuaResource>>    m_resources;
    std::unordered_map<std::string, ScriptCallback> m_natives;
};

} // namespace Atlas
