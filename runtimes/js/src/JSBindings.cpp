// runtimes/js/src/JSBindings.cpp
#include "JSBindings.h"

namespace Atlas {

void JSBindings::Register(void* /*ctx*/, const std::string& /*resourceName*/) {
    // TODO: register atlas.on, atlas.emit, atlas.log, atlas.getPlayers, etc.
    // For QuickJS:
    //   JSContext* ctx = (JSContext*)ctxPtr;
    //   JSValue global = JS_GetGlobalObject(ctx);
    //   JSValue atlas  = JS_NewObject(ctx);
    //   JS_SetPropertyStr(ctx, atlas, "log",
    //       JS_NewCFunction(ctx, js_atlas_log, "log", 1));
    //   JS_SetPropertyStr(ctx, global, "atlas", atlas);
    //   JS_FreeValue(ctx, global);
}

} // namespace Atlas
