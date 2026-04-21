// client/src/core/Memory.cpp
#include "Memory.h"
#include "Logger.h"
#include <Psapi.h>
#include <sstream>

#pragma comment(lib, "Psapi.lib")

namespace Atlas::Memory {

uintptr_t GetModuleBase(const char* moduleName) {
    return reinterpret_cast<uintptr_t>(GetModuleHandleA(moduleName));
}

size_t GetModuleSize(const char* moduleName) {
    HMODULE hMod = GetModuleHandleA(moduleName);
    if (!hMod) return 0;
    MODULEINFO info{};
    GetModuleInformation(GetCurrentProcess(), hMod, &info, sizeof(info));
    return info.SizeOfImage;
}

// Parse "48 8B ? ? ? ? 48 85 C0" into bytes and mask
static bool ParsePattern(const std::string& pattern,
                          std::vector<uint8_t>& bytes,
                          std::string& mask)
{
    bytes.clear();
    mask.clear();
    std::istringstream ss(pattern);
    std::string token;
    while (ss >> token) {
        if (token == "?" || token == "??") {
            bytes.push_back(0x00);
            mask.push_back('?');
        } else {
            bytes.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 16)));
            mask.push_back('x');
        }
    }
    return !bytes.empty();
}

uintptr_t PatternScan(const std::string& pattern, const char* moduleName) {
    uintptr_t base = GetModuleBase(moduleName);
    size_t    size = GetModuleSize(moduleName);
    if (!base || !size) return 0;

    std::vector<uint8_t> bytes;
    std::string mask;
    if (!ParsePattern(pattern, bytes, mask)) return 0;

    const uint8_t* mem = reinterpret_cast<const uint8_t*>(base);
    const size_t   len = bytes.size();

    for (size_t i = 0; i + len <= size; i++) {
        bool found = true;
        for (size_t j = 0; j < len; j++) {
            if (mask[j] == 'x' && mem[i + j] != bytes[j]) {
                found = false;
                break;
            }
        }
        if (found) return base + i;
    }
    return 0;
}

uintptr_t FollowRelativeCall(uintptr_t addr) {
    // E8 xx xx xx xx  — relative call
    // target = addr + 5 + *(int32_t*)(addr+1)
    int32_t offset = *reinterpret_cast<int32_t*>(addr + 1);
    return addr + 5 + offset;
}

uintptr_t ResolveRIPRelative(uintptr_t instrAddr, int instrLen) {
    // e.g. lea rax, [rip + offset]  (instrLen = 7, offset at instrAddr+3)
    int32_t offset = *reinterpret_cast<int32_t*>(instrAddr + instrLen - 4);
    return instrAddr + instrLen + offset;
}

} // namespace Atlas::Memory
