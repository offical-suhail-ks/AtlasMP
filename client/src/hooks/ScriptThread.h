#pragma once
// client/src/hooks/ScriptThread.h
#include "HookManager.h"
#include <cstdint>

namespace Atlas {

class ScriptThread {
public:
    /// Install the scrThread::Tick hook at the resolved address
    static bool InstallHook(HookManager* hooks, uintptr_t tickAddr);
};

} // namespace Atlas
