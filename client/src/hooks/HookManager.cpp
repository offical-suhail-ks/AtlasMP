// client/src/hooks/HookManager.cpp
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <Windows.h>

#include "HookManager.h"
#include "ScriptThread.h"
#include "../core/Logger.h"
#include "../core/Memory.h"
#include <MinHook.h>

namespace Atlas {

HookManager::HookManager()  = default;
HookManager::~HookManager() { Shutdown(); }

bool HookManager::Initialize() {
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        Logger::Error("[Hooks] MH_Initialize failed: %d", (int)status);
        return false;
    }
    m_minhookInitialized = true;
    Logger::Info("[Hooks] MinHook initialized");

    // Resolve GTA pattern addresses — retry a few times while game loads
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (ResolveAddresses()) break;
        Logger::Warn("[Hooks] Address scan attempt %d/10 failed — retrying in 2s", attempt+1);
        Sleep(2000);
    }

    // Install scrThread::Tick hook (entry point for our client tick)
    if (m_addrs.scrThreadTick) {
        ScriptThread::InstallHook(this, m_addrs.scrThreadTick);
    } else {
        Logger::Warn("[Hooks] scrThread::Tick not found — client tick disabled");
        Logger::Warn("[Hooks] GTA build may have a different pattern. Check RE guide.");
    }

    return true;
}

void HookManager::Shutdown() {
    UnhookAll();
    if (m_minhookInitialized) {
        MH_Uninitialize();
        m_minhookInitialized = false;
    }
}

bool HookManager::Hook(const std::string& name, void* target,
                        void* detour, void** original)
{
    if (!m_minhookInitialized || !target) return false;

    MH_STATUS s = MH_CreateHook(target, detour, original);
    if (s != MH_OK) {
        Logger::Error("[Hooks] MH_CreateHook failed for '%s': %d", name.c_str(), (int)s);
        return false;
    }
    s = MH_EnableHook(target);
    if (s != MH_OK) {
        Logger::Error("[Hooks] MH_EnableHook failed for '%s': %d", name.c_str(), (int)s);
        return false;
    }
    m_hooks.push_back({name, target, detour, original, true});
    Logger::Info("[Hooks] Hooked: %s @ 0x%llX", name.c_str(),
        (unsigned long long)(uintptr_t)target);
    return true;
}

bool HookManager::Unhook(const std::string& name) {
    for (auto& h : m_hooks) {
        if (h.name == name && h.enabled) {
            MH_DisableHook(h.target);
            MH_RemoveHook(h.target);
            h.enabled = false;
            return true;
        }
    }
    return false;
}

void HookManager::UnhookAll() {
    for (auto& h : m_hooks) {
        if (h.enabled) { MH_DisableHook(h.target); MH_RemoveHook(h.target); }
    }
    m_hooks.clear();
}

bool HookManager::Enable(const std::string& name) {
    for (auto& h : m_hooks) if (h.name == name) { MH_EnableHook(h.target); h.enabled=true; return true; }
    return false;
}
bool HookManager::Disable(const std::string& name) {
    for (auto& h : m_hooks) if (h.name == name) { MH_DisableHook(h.target); h.enabled=false; return true; }
    return false;
}

bool HookManager::ResolveAddresses() {
    // ── scrThread::Tick ───────────────────────────────────────────────────────
    // Called every game frame for every script thread.
    // We hook this to drive our client tick.
    //
    // Pattern research sources:
    //   citizenfx/fivem — rage-scripting-five/src/scrThread.cpp
    //   wl-fivem fork   — same file
    //
    // The pattern varies slightly between GTA V build versions.
    // We try multiple known patterns in order.

    struct Pattern { const char* sig; int offset; };
    static const Pattern scrThreadPatterns[] = {
        // Build 2699+  (from citizenfx/fivem scrThread.cpp)
        {"48 83 EC 20 48 83 B9 ? 01 00 00 00 48 8B D9 74 14", -6},
        // Older builds (wl-fivem)
        {"80 B9 46 01 00 00 00 8B FA 48 8B D9 74 05", -0xF},
        // Additional fallback
        {"48 83 B9 ? ? 00 00 00 8B FA 48 8B D9 74", -6},
        {nullptr, 0}
    };

    for (int i = 0; scrThreadPatterns[i].sig; ++i) {
        uintptr_t addr = Memory::PatternScan(scrThreadPatterns[i].sig);
        if (addr) {
            m_addrs.scrThreadTick = addr + scrThreadPatterns[i].offset;
            Logger::Info("[Hooks] scrThread::Tick @ 0x%llX (pattern %d)",
                (unsigned long long)m_addrs.scrThreadTick, i);
            break;
        }
    }

    if (!m_addrs.scrThreadTick) {
        Logger::Warn("[Hooks] scrThread::Tick not found in GTA5.exe");
        Logger::Warn("[Hooks] Known patterns failed — GTA build may be newer");
        Logger::Warn("[Hooks] RE guide: open GTA5.exe in x64dbg, attach, set BP on");
        Logger::Warn("[Hooks]   Search for string 'legalese' or 'main' in memory");
        Logger::Warn("[Hooks]   Break when accessed — walk up call stack to Tick");
        return false;
    }

    // ── IDXGISwapChain::Present (for NUI overlay) ─────────────────────────────
    m_addrs.presentHook = Memory::PatternScan(
        "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 41 8B F8",
        "dxgi.dll");

    if (m_addrs.presentHook)
        Logger::Info("[Hooks] Present @ 0x%llX", (unsigned long long)m_addrs.presentHook);
    else
        Logger::Warn("[Hooks] Present not found in dxgi.dll — NUI disabled");

    return true;
}

} // namespace Atlas
