#pragma once
// client/src/hooks/HookManager.h
// Manages hooks into GTA V / RAGE engine.
// NOTE: This header intentionally does NOT include <Windows.h>. Nothing in the
// public interface needs Windows types (only uintptr_t / std::string / vector),
// and pulling <Windows.h> into every includer was triggering an include-depth
// error on some toolchains. .cpp files include <Windows.h>/<MinHook.h> as needed.

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace Atlas {

// ─── Pattern scanner ─────────────────────────────────────────────────────────
class PatternScanner {
public:
  static uintptr_t Scan(const char *pattern, const char *mask = nullptr);
  static uintptr_t ScanRange(uintptr_t start, uintptr_t end,
                             const char *pattern, size_t len, const char *mask);
  static uintptr_t ScanIDA(const std::string &idaPattern);

private:
  static uintptr_t s_moduleBase;
  static uintptr_t s_moduleSize;
};

// ─── Hook entry ──────────────────────────────────────────────────────────────
struct HookEntry {
  std::string name;
  void *target;
  void *detour;
  void **original;
  bool enabled = false;
};

// ─── HookManager ─────────────────────────────────────────────────────────────
class HookManager {
public:
  HookManager();
  ~HookManager();

  bool Initialize();
  void Shutdown();

  bool Hook(const std::string &name, void *target, void *detour,
            void **original);
  bool Unhook(const std::string &name);
  bool Enable(const std::string &name);
  bool Disable(const std::string &name);
  void UnhookAll();

  struct KnownAddresses {
    uintptr_t scrThreadTick = 0;
    uintptr_t scrEngineRunScripts = 0;
    uintptr_t nativeInvoker = 0;
    uintptr_t presentHook = 0;
    uintptr_t wndProc = 0;
  };

  const KnownAddresses &GetAddresses() const { return m_addrs; }
  bool ResolveAddresses();

private:
  std::vector<HookEntry> m_hooks;
  KnownAddresses m_addrs;
  bool m_minhookInitialized = false;
};

} // namespace Atlas
