#pragma once
// sdk/include/IScriptRuntime.h
// Abstract interface implemented by each scripting runtime DLL.

#include <string>
#include <vector>
#include <functional>

#define ATLAS_SDK_VERSION 1

namespace Atlas {

// ── ScriptValue — generic value passed between C++ and scripts ────────────────
enum class ScriptValueType { Null, Boolean, Integer, Float, String };

struct ScriptValue {
    ScriptValueType type     = ScriptValueType::Null;
    bool            boolVal  = false;
    int64_t         intVal   = 0;
    double          floatVal = 0.0;
    std::string     strVal;

    static ScriptValue Null()              { return {}; }
    static ScriptValue Bool(bool v)        { ScriptValue s; s.type=ScriptValueType::Boolean; s.boolVal=v; return s; }
    static ScriptValue Int(int64_t v)      { ScriptValue s; s.type=ScriptValueType::Integer; s.intVal=v; return s; }
    static ScriptValue Float(double v)     { ScriptValue s; s.type=ScriptValueType::Float; s.floatVal=v; return s; }
    static ScriptValue String(std::string v){ ScriptValue s; s.type=ScriptValueType::String; s.strVal=std::move(v); return s; }
};

using ScriptArgs     = std::vector<ScriptValue>;
using ScriptCallback = std::function<ScriptValue(const ScriptArgs&)>;

// ── IResource — one script resource (e.g. one folder with server.lua) ─────────
class IResource {
public:
    virtual ~IResource() = default;
    virtual const std::string& GetName() const = 0;
    virtual bool Execute(const std::string& filePath) = 0;
    virtual ScriptValue CallExport(const std::string& funcName,
                                    const ScriptArgs& args) = 0;
};

// ── IScriptRuntime — implemented by each runtime DLL ─────────────────────────
class IScriptRuntime {
public:
    virtual ~IScriptRuntime() = default;

    virtual bool        Initialize(uint32_t sdkVersion) = 0;
    virtual void        Shutdown() = 0;
    virtual void        Tick() {}  // called regularly for timers/coroutines

    virtual const char* GetRuntimeId()      const = 0;  // "lua", "js", etc.
    virtual const char* GetRuntimeVersion() const = 0;

    virtual IResource*  LoadResource(const std::string& name,
                                      const std::string& basePath) = 0;
    virtual void        UnloadResource(IResource* resource) = 0;

    virtual void        TriggerEvent(const std::string& name,
                                      const ScriptArgs& args = {}) = 0;
    virtual void        TriggerResourceEvent(IResource* resource,
                                              const std::string& name,
                                              const ScriptArgs& args = {}) = 0;
    virtual void        RegisterNative(const std::string& name,
                                        ScriptCallback cb) = 0;
};

} // namespace Atlas
