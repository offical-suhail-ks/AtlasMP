#pragma once
// client/src/core/Memory.h
// Memory utilities: pattern scanning, read/write into GTA V process memory
#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace Atlas::Memory {

/// Read a value from process memory
template<typename T>
T Read(uintptr_t address) {
    return *reinterpret_cast<T*>(address);
}

/// Write a value to process memory
template<typename T>
void Write(uintptr_t address, const T& value) {
    DWORD old;
    VirtualProtect(reinterpret_cast<void*>(address), sizeof(T),
                   PAGE_EXECUTE_READWRITE, &old);
    *reinterpret_cast<T*>(address) = value;
    VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), old, &old);
}

/// Get the base address of a loaded module (e.g. "GTA5.exe")
uintptr_t GetModuleBase(const char* moduleName = nullptr);

/// Get the size of a module's .text section
size_t GetModuleSize(const char* moduleName = nullptr);

/// IDA-style pattern scan: "48 8B ? ? ? ? 48 85 C0"
/// Returns 0 if not found
uintptr_t PatternScan(const std::string& pattern,
                       const char* moduleName = nullptr);

/// Follow a relative call/jmp instruction at addr to get the target
uintptr_t FollowRelativeCall(uintptr_t addr);

/// Resolve a RIP-relative address (e.g. lea rax, [rip+offset])
uintptr_t ResolveRIPRelative(uintptr_t instrAddr, int instrLen);

} // namespace Atlas::Memory
