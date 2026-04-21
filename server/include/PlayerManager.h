#pragma once
// server/include/PlayerManager.h
// Tracks all connected players and their authoritative server state

#include "../../../shared/include/Types.h"
#include "../../../shared/include/AtlasMath.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <any>
#include <memory>
#include <chrono>

namespace Atlas {

// ─── Player ──────────────────────────────────────────────────────────────────
struct Player
{
    PlayerId    id;
    std::string name;
    std::string ip;
    uint32_t    ping        = 0;
    bool        connected   = true;

    // Authoritative state (set by server, validated, broadcast to others)
    Vector3     position;
    Vector3     velocity;
    Quaternion  rotation;
    float       heading     = 0.0f;
    int         health      = 200;
    int         armour      = 0;
    uint32_t    weaponHash  = 0;
    bool        isDead      = false;
    bool        inVehicle   = false;
    VehicleId   vehicleId   = INVALID_VEHICLE;
    int         vehicleSeat = -1;

    // Script KV store (per-player arbitrary data from Lua/JS/C#)
    std::unordered_map<std::string, std::any> data;

    // Connection metadata
    using Clock = std::chrono::steady_clock;
    Clock::time_point connectTime;
    Clock::time_point lastPacketTime;
    uint32_t packetsSent     = 0;
    uint32_t packetsReceived = 0;
    uint32_t violations      = 0;  // Anti-cheat violation count

    // Helpers
    void SetData(const std::string& key, std::any val) { data[key] = std::move(val); }
    std::any GetData(const std::string& key) const {
        auto it = data.find(key);
        return it != data.end() ? it->second : std::any{};
    }
};

// ─── PlayerManager ───────────────────────────────────────────────────────────
class PlayerManager
{
public:
    PlayerManager() = default;

    // ── Lifecycle ─────────────────────────────────────────────────────────
    PlayerId AddPlayer(const std::string& name, const std::string& ip);
    void     RemovePlayer(PlayerId id);

    // ── Lookup ────────────────────────────────────────────────────────────
    Player*       GetPlayer(PlayerId id);
    const Player* GetPlayer(PlayerId id) const;
    bool          HasPlayer(PlayerId id) const;

    std::vector<PlayerId> GetAllPlayerIds() const;
    uint32_t              GetPlayerCount() const;

    // ── Iteration ─────────────────────────────────────────────────────────
    template<typename Fn>
    void ForEach(Fn&& fn) {
        for (auto& [id, player] : m_players) {
            fn(*player);
        }
    }

    template<typename Fn>
    void ForEachExcept(PlayerId exclude, Fn&& fn) {
        for (auto& [id, player] : m_players) {
            if (id != exclude) fn(*player);
        }
    }

private:
    std::unordered_map<PlayerId, std::unique_ptr<Player>> m_players;
    PlayerId m_nextId = 1;
};

} // namespace Atlas
