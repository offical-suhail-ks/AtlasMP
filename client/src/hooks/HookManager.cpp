// client/src/hooks/HookManager.cpp
#include <Windows.h>

#include "HookManager.h"
#include "ScriptThread.h"
#include "../core/Logger.h"
#include "../core/Memory.h"
#include <MinHook.h>

namespace Atlas {

HookManager::HookManager() = default;
HookManager::~HookManager() { Shutdown(); }

bool HookManager::Initialize() {
  // ── ScriptHookV / ASI mode ──────────────────────────────────────────────
  // We install no hooks of our own: ScriptHookV drives the tick. Nothing to do,
  // and we don't touch MinHook (ScriptHookV manages native access). No-op.
  Logger::Info("[Hooks] ASI mode - no client hooks needed");
  return true;
}

void HookManager::Shutdown() {
  UnhookAll();
  if (m_minhookInitialized) {
    MH_Uninitialize();
    m_minhookInitialized = false;
  }
}

bool HookManager::Hook(const std::string &name, void *target, void *detour,
                       void **original) {
  if (!m_minhookInitialized || !target)
    return false;
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

bool HookManager::Unhook(const std::string &name) {
  for (auto &h : m_hooks) {
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
  for (auto &h : m_hooks) {
    if (h.enabled) {
      MH_DisableHook(h.target);
      MH_RemoveHook(h.target);
    }
  }
  m_hooks.clear();
}

bool HookManager::Enable(const std::string &name) {
  for (auto &h : m_hooks)
    if (h.name == name) { MH_EnableHook(h.target); h.enabled = true; return true; }
  return false;
}

bool HookManager::Disable(const std::string &name) {
  for (auto &h : m_hooks)
    if (h.name == name) { MH_DisableHook(h.target); h.enabled = false; return true; }
  return false;
}

bool HookManager::ResolveAddresses() {
  // Unused in ASI mode (ScriptHookV drives the tick). Kept so the interface
  // is unchanged; contains no FiveM-derived patterns in the active build path.
  return true;
}

} // namespace Atlas
