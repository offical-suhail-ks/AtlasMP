#pragma once
// client/src/hooks/HookManager.h
// Manages all MinHook-based hooks into GTA V / RAGE engine

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <string>
#include <functional>

namespace Atlas {

// ─── Pattern scanner ──────────────────────────────────────────────────────────
class PatternScanner {
public:
    /// Scan the GTA5.exe module for a byte pattern with wildcards (??)
    static uintptr_t Scan(const char* pattern, const char* mask = nullptr);

    /// Scan a specific memory range
    static uintptr_t ScanRange(uintptr_t start, uintptr_t end,
                                const char* pattern, size_t len,
                                const char* mask);

    /// Scan using IDA-style pattern string e.g. "48 8B ? ? ? ? 48 85 C0"
    static uintptr_t ScanIDA(const std::string& idaPattern);

private:
    static uintptr_t s_moduleBase;
    static uintptr_t s_moduleSize;
};

// ─── Hook entry ───────────────────────────────────────────────────────────────
struct HookEntry {
    std::string name;
    void*       target;
    void*       detour;
    void**      original;
    bool        enabled = false;
};

// ─── HookManager ─────────────────────────────────────────────────────────────
class HookManager {
public:
    HookManager();
    ~HookManager();

    bool Initialize();
    void Shutdown();

    /// Install a hook. original receives the trampoline pointer.
    bool Hook(const std::string& name, void* target, void* detour, void** original);

    /// Remove a hook by name
    bool Unhook(const std::string& name);

    /// Enable/disable a hook without removing it
    bool Enable(const std::string& name);
    bool Disable(const std::string& name);

    /// Remove all installed hooks
    void UnhookAll();

    // ── Known hook addresses (resolved at runtime by pattern scan) ────────────
    // These are populated by ResolveAddresses()
    struct KnownAddresses {
        uintptr_t scrThreadTick       = 0; // scrThread::Tick vtable slot
        uintptr_t scrEngineRunScripts = 0; // scrEngine::RunScripts
        uintptr_t nativeInvoker       = 0; // Native hash resolver
        uintptr_t presentHook         = 0; // IDXGISwapChain::Present (for NUI)
        uintptr_t wndProc             = 0; // Game window WndProc (for input)
    };

    const KnownAddresses& GetAddresses() const { return m_addrs; }
    bool ResolveAddresses();

private:
    std::vector<HookEntry> m_hooks;
    KnownAddresses m_addrs;
    bool m_minhookInitialized = false;
};

} // namespace Atlas
