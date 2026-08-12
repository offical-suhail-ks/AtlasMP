// client/src/hooks/NativeInvoker.cpp
// Native calls delegate to ScriptHookV's native caller.
#include "NativeInvoker.h"
#include "../core/Logger.h"
#include <Windows.h>
#include <main.h>
#include <nativeCaller.h>

namespace Atlas {

NativeInvoker::NativeInvoker() = default;

bool NativeInvoker::Initialize() {
    m_nativeTable = reinterpret_cast<void*>(0x1);
    Logger::Info("[Natives] Using ScriptHookV native caller");
    return true;
}

// SEH-guarded native call. ScriptHookV raises a fatal (which would close the
// game) when a native hash doesn't exist in the current build. We catch it,
// log the offending hash once per hash, and continue so the game stays alive
// and we can see ALL stale hashes in one run instead of crashing on the first.
static bool CallNativeGuarded(uint64_t hash, NativeContext& ctx, bool& missing) {
    missing = false;
    __try {
        nativeInit(hash);
        for (uint32_t i = 0; i < ctx.m_nArgCount; ++i)
            nativePush64(static_cast<uint64_t>(ctx.m_args[i]));
        uint64_t* ret = nativeCall();
        if (ret) {
            ctx.m_returnValue[0] = static_cast<uintptr_t>(ret[0]);
            ctx.m_returnValue[1] = static_cast<uintptr_t>(ret[1]);
            ctx.m_returnValue[2] = static_cast<uintptr_t>(ret[2]);
            ctx.m_returnValue[3] = static_cast<uintptr_t>(ret[3]);
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        missing = true;
        return false;
    }
}

bool NativeInvoker::Invoke(uint64_t hash, NativeContext& ctx) {
    bool missing = false;
    bool ok = CallNativeGuarded(hash, ctx, missing);
    if (missing) {
        // Log each missing hash only once to avoid spamming the log every frame.
        static uint64_t s_seen[64]; static int s_n = 0;
        bool already = false;
        for (int i = 0; i < s_n; ++i) if (s_seen[i] == hash) { already = true; break; }
        if (!already) {
            if (s_n < 64) s_seen[s_n++] = hash;
            Logger::Error("[Natives] MISSING native 0x%016llX in this build — skipped",
                          (unsigned long long)hash);
        }
        // Zero the return so callers reading a result get 0 instead of garbage.
        ctx.m_returnValue[0] = ctx.m_returnValue[1] =
        ctx.m_returnValue[2] = ctx.m_returnValue[3] = 0;
    }
    return ok;
}

NativeFunc NativeInvoker::GetNativeFunc(uint64_t) { return nullptr; }
void       NativeInvoker::DumpNativeTableRaw()    {}
bool       NativeInvoker::BuildNativeTable()      { return true; }

} // namespace Atlas
