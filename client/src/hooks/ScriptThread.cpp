// client/src/hooks/ScriptThread.cpp
// Hooks GTA V's scrThread::Tick to inject our client tick every game frame.
#include "ScriptThread.h"
#include "../core/Client.h"
#include "../core/Logger.h"
#include <MinHook.h>

namespace Atlas {

// ── Hooked function signature ──────────────────────────────────────────────
// scrThread::Tick(uint32_t opsToExecute) — virtual method called each frame
using FnScrThreadTick = void*(__fastcall*)(void* self, uint32_t ops);
static FnScrThreadTick g_origScrThreadTick = nullptr;
static bool            g_injected          = false;

static void* __fastcall Hook_ScrThreadTick(void* self, uint32_t ops) {
    // Call our client tick once per game frame
    if (!g_injected) {
        g_injected = true;
        Logger::Info("[ScriptThread] First tick — client running inside RAGE");
    }

    if (Client* client = Client::Get()) {
        client->Tick();
    }

    // Call original
    return g_origScrThreadTick(self, ops);
}

bool ScriptThread::InstallHook(HookManager* hooks, uintptr_t tickAddr) {
    if (!tickAddr) {
        Logger::Warn("[ScriptThread] Cannot install hook — address is 0");
        return false;
    }

    bool ok = hooks->Hook("scrThread::Tick",
                           reinterpret_cast<void*>(tickAddr),
                           reinterpret_cast<void*>(&Hook_ScrThreadTick),
                           reinterpret_cast<void**>(&g_origScrThreadTick));

    if (ok) Logger::Info("[ScriptThread] Hook installed successfully");
    else    Logger::Error("[ScriptThread] Hook installation failed");
    return ok;
}

} // namespace Atlas
