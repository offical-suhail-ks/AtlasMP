// runtimes/js/src/JSRuntime.cpp
// JavaScript runtime stub — ready for QuickJS or V8 integration
// QuickJS: https://github.com/bellard/quickjs
// V8 embed: https://v8.dev/docs/embed
#include "JSRuntime.h"
#include "../../../sdk/include/IScriptRuntime.h"

#include <algorithm>
#include <filesystem>

namespace Atlas {

// ── JSEngineState stub ────────────────────────────────────────────────────────
// Replace with: struct JSEngineState { JSRuntime* rt; JSContext* ctx; }; for QuickJS
// or:           struct JSEngineState { v8::Isolate* isolate; ... }; for V8
struct JSEngineState {
    std::string resourceName;
    bool        running = false;
};

// ── JSResource ────────────────────────────────────────────────────────────────

bool JSResource::Execute(const std::string& filePath)
{
    if (!m_state) {
        return false;
    }

    if (!std::filesystem::exists(filePath)) {
        return false;
    }

    // TODO: Execute script content with the selected JS engine.
    m_running = true;
    m_state->running = true;
    return true;
}

ScriptValue JSResource::CallExport(const std::string& /*funcName*/,
                                    const ScriptArgs& /*args*/)
{
    return ScriptValue::Null();
}

// ── JSRuntime ─────────────────────────────────────────────────────────────────

bool JSRuntime::Initialize(uint32_t /*sdkVersion*/) {
    // TODO: JS_NewRuntime() for QuickJS
    //       v8::V8::InitializePlatform() for V8
    m_initialized = true;
    return true;
}

void JSRuntime::Shutdown() {
    m_resources.clear();
    m_states.clear();
    // TODO: JS_FreeRuntime() / v8 teardown
    m_initialized = false;
}

IResource* JSRuntime::LoadResource(const std::string& name,
                                    const std::string& basePath)
{
    auto state = std::make_unique<JSEngineState>();
    state->resourceName = name;
    state->running      = true;

    // TODO: create JS context, load + execute server.js

    auto res = std::make_unique<JSResource>(name, basePath, state.get());
    IResource* ptr = res.get();
    m_states.push_back(std::move(state));
    m_resources.push_back(std::move(res));
    return ptr;
}

void JSRuntime::UnloadResource(IResource* resource) {
    m_resources.erase(
        std::remove_if(m_resources.begin(), m_resources.end(),
            [resource](const auto& r){ return r.get() == resource; }),
        m_resources.end());
}

void JSRuntime::TriggerEvent(const std::string& eventName,
                              const ScriptArgs& args)
{
    for (auto& res : m_resources)
        TriggerResourceEvent(res.get(), eventName, args);
}

void JSRuntime::TriggerResourceEvent(IResource* /*resource*/,
                                      const std::string& /*eventName*/,
                                      const ScriptArgs& /*args*/)
{
    // TODO: call JS event dispatcher in the resource's context
}

void JSRuntime::RegisterNative(const std::string& name,
                                ScriptCallback callback)
{
    m_natives[name] = std::move(callback);
}

// ── DLL exports ───────────────────────────────────────────────────────────────
extern "C" {
    __declspec(dllexport)
    IScriptRuntime* Atlas_CreateRuntime() { return new JSRuntime(); }

    __declspec(dllexport)
    uint32_t Atlas_GetSDKVersion() { return 1; }
}

} // namespace Atlas
