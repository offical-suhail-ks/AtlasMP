#pragma once
// runtimes/js/src/JSBindings.h
// Registers the atlas.* JavaScript API
// Implementation depends on chosen JS engine (QuickJS or V8)
#include <string>

namespace Atlas {

class JSBindings {
public:
    /// Register atlas.* API into a JS context
    /// ctx is JSContext* (QuickJS) or v8::Context* (V8) — cast as needed
    static void Register(void* ctx, const std::string& resourceName);
};

} // namespace Atlas
