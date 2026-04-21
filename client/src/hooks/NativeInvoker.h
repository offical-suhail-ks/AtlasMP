#pragma once
// client/src/hooks/NativeInvoker.h
// Calls GTA V script native functions by their 64-bit hash.
// Study reference: scripthookvdotnet, plugin-sdk

#include <cstdint>
#include <cstring>

namespace Atlas {

// ─── Native context ───────────────────────────────────────────────────────────
// GTA V passes args and receives results through a context struct
// Max 32 args + 4 result slots (from RAGE internals)
struct NativeContext {
    static constexpr int MAX_ARGS = 32;

    void*    m_pReturn;         // Points to m_pArgs[0] by default
    uint32_t m_nArgCount;
    void*    m_pArgs;

    uint32_t m_nDataCount;
    uintptr_t m_vectorSpace[MAX_ARGS * 3]; // Space for vector args
    uintptr_t m_args[MAX_ARGS];
    uintptr_t m_returnValue[4];

    NativeContext() {
        m_pReturn    = m_returnValue;
        m_nArgCount  = 0;
        m_pArgs      = m_args;
        m_nDataCount = 0;
        memset(m_args, 0, sizeof(m_args));
        memset(m_returnValue, 0, sizeof(m_returnValue));
    }

    template<typename T>
    void Push(T val) {
        static_assert(sizeof(T) <= sizeof(uintptr_t));
        memcpy(&m_args[m_nArgCount++], &val, sizeof(T));
    }

    template<typename T>
    T GetResult() {
        T result;
        memcpy(&result, m_returnValue, sizeof(T));
        return result;
    }

    void Reset() {
        m_nArgCount = 0;
        memset(m_args, 0, sizeof(m_args));
        memset(m_returnValue, 0, sizeof(m_returnValue));
        m_pReturn = m_returnValue;
    }
};

// ─── Native function pointer type ─────────────────────────────────────────────
using NativeFunc = void(*)(NativeContext* ctx);

// ─── NativeInvoker ───────────────────────────────────────────────────────────
class NativeInvoker {
public:
    NativeInvoker();
    ~NativeInvoker() = default;

    bool Initialize();

    /// Call a native by hash. Returns false if hash not found.
    bool Invoke(uint64_t hash, NativeContext& ctx);

    /// High-level typed call helper.
    /// Usage: Call<void>(GET_PLAYER_PED, player);
    template<typename Ret, typename... Args>
    Ret Call(uint64_t hash, Args... args) {
        NativeContext ctx;
        (ctx.Push(args), ...);  // C++17 fold expression to push all args
        Invoke(hash, ctx);
        if constexpr (!std::is_void_v<Ret>) {
            return ctx.GetResult<Ret>();
        }
    }

    /// Resolve native hash to function pointer (one-time lookup + cache)
    NativeFunc GetNativeFunc(uint64_t hash);

private:
    /// Lookup the native handler table in GTA5.exe memory
    bool BuildNativeTable();

    // GTA V uses a hash table of function pointers
    // We cache resolved hashes to avoid repeated table lookups
    struct CachedNative {
        uint64_t    hash;
        NativeFunc  func;
    };

    // TODO: Replace with a proper hash map
    static constexpr int CACHE_SIZE = 4096;
    CachedNative m_cache[CACHE_SIZE] = {};
    int m_cacheCount = 0;

    // Pointer to RAGE's native registration table
    void* m_nativeTable = nullptr;
};

// ─── Common native hashes ─────────────────────────────────────────────────────
// Source: https://github.com/alloc8or/gta5-nativedb-data
// Full list: include alloc8or's natives.h when building
namespace Natives {
    // PLAYER
    constexpr uint64_t GET_PLAYER_PED             = 0x43A66C31C68491C0;
    constexpr uint64_t GET_PLAYER_PED_SCRIPT_INDEX = 0x50FAC3A3E030A6E1;
    constexpr uint64_t SET_PLAYER_MODEL            = 0x00A1CADD00108836;
    constexpr uint64_t GET_PLAYER_NAME             = 0x6D0DE6A7B5DA71F8;

    // ENTITY
    constexpr uint64_t GET_ENTITY_COORDS           = 0x3FEF770D40960D5A;
    constexpr uint64_t SET_ENTITY_COORDS           = 0x06843DA7060A026B;
    constexpr uint64_t GET_ENTITY_VELOCITY         = 0x4805D2B1D8CF2FEB;
    constexpr uint64_t SET_ENTITY_VELOCITY         = 0x1C99BB7B6E96D16F;
    constexpr uint64_t GET_ENTITY_ROTATION         = 0xAFBD61CC738D9EB9;
    constexpr uint64_t SET_ENTITY_ROTATION         = 0x8524A8B0171D5E07;
    constexpr uint64_t GET_ENTITY_HEALTH           = 0xEEF059FAD016D209;
    constexpr uint64_t SET_ENTITY_HEALTH           = 0x6B76DC1F3AE6E6A3;
    constexpr uint64_t IS_ENTITY_DEAD              = 0x5F9532F3B5CC2551;
    constexpr uint64_t DELETE_ENTITY               = 0xAD738C3085FE7E11;

    // PED
    constexpr uint64_t CREATE_PED                  = 0xD49F9B0955C367DE;
    constexpr uint64_t IS_PED_IN_ANY_VEHICLE       = 0x997ABD671D25CA0B;
    constexpr uint64_t GET_VEHICLE_PED_IS_IN       = 0x9A9112A0FE9A4713;
    constexpr uint64_t SET_PED_DEFAULT_COMPONENT_VARIATION = 0x45EEE61580806D63;

    // VEHICLE
    constexpr uint64_t CREATE_VEHICLE              = 0xAF35D0D2583051B0;
    constexpr uint64_t GET_VEHICLE_MODEL_HASH      = 0xEEF059FAD016D209;
    constexpr uint64_t SET_VEHICLE_ENGINE_ON       = 0x2497C4717C8B881E;
    constexpr uint64_t GET_VEHICLE_SPEED           = 0x43A230A8A3BD0EA6;

    // NETWORK / SYNC (GTA Online internals)
    constexpr uint64_t NETWORK_GET_NETWORK_ID_FROM_ENTITY = 0xA11700439731F8A5;
    constexpr uint64_t NETWORK_GET_ENTITY_FROM_NETWORK_ID = 0xCE4E5D9B0A4FF560;

    // UI
    constexpr uint64_t BEGIN_TEXT_COMMAND_PRINT    = 0xB04058885DCBD763;
    constexpr uint64_t END_TEXT_COMMAND_PRINT      = 0x9D77056A530643F6;
    constexpr uint64_t DRAW_RECT                   = 0x3A618A217E5154F0;
} // namespace Natives

} // namespace Atlas
