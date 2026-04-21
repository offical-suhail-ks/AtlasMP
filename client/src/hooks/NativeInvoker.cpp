// client/src/hooks/NativeInvoker.cpp
#include "NativeInvoker.h"
#include "../core/Logger.h"
#include "../core/Memory.h"

namespace Atlas {

NativeInvoker::NativeInvoker() = default;

bool NativeInvoker::Initialize() {
    if (!BuildNativeTable()) {
        Logger::Warn("[Natives] Native table not found — natives disabled");
        return false;
    }
    Logger::Info("[Natives] Native table found @ 0x%llX",
                 static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(m_nativeTable)));
    return true;
}

bool NativeInvoker::Invoke(uint64_t hash, NativeContext& ctx) {
    NativeFunc fn = GetNativeFunc(hash);
    if (!fn) return false;
    fn(&ctx);
    return true;
}

NativeFunc NativeInvoker::GetNativeFunc(uint64_t hash) {
    // Check cache first
    for (int i = 0; i < m_cacheCount; i++)
        if (m_cache[i].hash == hash) return m_cache[i].func;

    if (!m_nativeTable) return nullptr;

    // GTA V uses a 256-bucket hash table of linked lists
    // Each bucket entry: [NativeFunc*, uint64_t hash, next*]
    // bucket = (hash >> 0) & 0xFF  — lower 8 bits

    struct NativeEntry {
        NativeFunc  func;
        uint64_t    hash;
        NativeEntry* next;
    };

    NativeEntry** table = reinterpret_cast<NativeEntry**>(m_nativeTable);
    uint32_t bucket = (uint32_t)(hash & 0xFF);
    NativeEntry* entry = table[bucket];

    while (entry) {
        if (entry->hash == hash) {
            // Cache it
            if (m_cacheCount < CACHE_SIZE) {
                m_cache[m_cacheCount++] = { hash, entry->func };
            }
            return entry->func;
        }
        entry = entry->next;
    }
    return nullptr;
}

bool NativeInvoker::BuildNativeTable() {
    // Pattern: find the native registration function in GTA5.exe
    // This address changes every GTA V update
    // Reference: scripthookvdotnet / plugin-sdk research

    uintptr_t addr = Memory::PatternScan(
        "76 32 48 8B 53 40 48 8D 0D");

    if (!addr) {
        // Try fallback pattern
        addr = Memory::PatternScan(
            "48 8D 0D ? ? ? ? 48 8B 14 D1 48 85 D2 74");
    }

    if (!addr) return false;

    // Resolve RIP-relative pointer to the table
    uintptr_t tableAddr = Memory::ResolveRIPRelative(addr + 6, 7);
    if (!tableAddr) return false;

    m_nativeTable = reinterpret_cast<void*>(tableAddr);
    return true;
}

} // namespace Atlas
