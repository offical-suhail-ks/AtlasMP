#pragma once
// runtimes/js/src/JSRuntime.h
// JavaScript scripting runtime — backed by QuickJS or V8
//
// DECISION NEEDED: QuickJS vs V8
//   QuickJS: ~210KB library, easy to embed, C API, slower execution
//   V8:      Full Chrome JS engine, complex build, faster execution, better compat
//
// Recommend starting with QuickJS for simplicity, upgrading to V8 later.
// QuickJS repo: https://github.com/bellard/quickjs
// V8 embed guide: https://v8.dev/docs/embed

#include "../../../sdk/include/IScriptRuntime.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace Atlas {

// Forward declare the JS engine context (QuickJS: JSRuntime/JSContext, V8: Isolate/Context)
// We use a void* wrapper until engine is chosen
struct JSEngineState;

class JSResource : public IResource
{
public:
    JSResource(const std::string& name, const std::string& basePath, JSEngineState* state)
        : m_name(name), m_basePath(basePath), m_state(state) {}

    const std::string& GetName()     const override { return m_name; }
    bool Execute(const std::string& filePath) override;

    ScriptValue CallExport(const std::string& funcName,
                           const ScriptArgs& args) override;

    const std::string& GetBasePath() const { return m_basePath; }
    bool               IsRunning()   const { return m_running; }

    JSEngineState* m_state  = nullptr;
    bool           m_running = false;

private:
    std::string m_name;
    std::string m_basePath;
};

class JSRuntime : public IScriptRuntime
{
public:
    JSRuntime() = default;
    ~JSRuntime() override { Shutdown(); }

    bool Initialize(uint32_t sdkVersion) override;
    void Shutdown() override;

    IResource* LoadResource(const std::string& name,
                            const std::string& basePath) override;
    void UnloadResource(IResource* resource) override;

    void TriggerEvent(const std::string& eventName,
                      const ScriptArgs& args) override;
    void TriggerResourceEvent(IResource* resource,
                              const std::string& eventName,
                              const ScriptArgs& args) override;

    void RegisterNative(const std::string& name,
                        ScriptCallback callback) override;

    const char* GetRuntimeId()      const override { return "js"; }
    const char* GetRuntimeVersion() const override { return "QuickJS-2024"; }

private:
    // TODO: Initialize chosen JS engine here
    // QuickJS: JSRuntime* rt = JS_NewRuntime();
    // V8:      v8::Platform / v8::Isolate

    bool m_initialized = false;
    std::vector<std::unique_ptr<JSEngineState>>  m_states;
    std::vector<std::unique_ptr<JSResource>>     m_resources;
    std::unordered_map<std::string, ScriptCallback> m_natives;
};

} // namespace Atlas
