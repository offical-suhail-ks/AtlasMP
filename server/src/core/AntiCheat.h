#pragma once
// server/src/core/AntiCheat.h
#include "../../../shared/include/Types.h"
#include "../../../shared/include/AtlasMath.h"
#include "../../../shared/include/Packets.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <functional>

namespace Atlas {

class PlayerManager;
class NetworkServer;

enum class ViolationSeverity : uint8_t {
    Low    = 1,
    Medium = 2,
    High   = 3,
    Fatal  = 4,
};

struct Violation {
    uint16_t    playerId;
    std::string reason;
    ViolationSeverity severity;
    std::string details;
    std::chrono::system_clock::time_point time;
};

struct AntiCheatConfig {
    bool  enabled             = true;
    bool  kickOnViolation     = true;
    int   violationsToKick    = 3;
    float maxSpeedMps         = 150.0f;
    float maxHealthGainPerS   = 10.0f;
    int   maxPacketsPerSecond = 200;
};

class AntiCheat {
public:
    AntiCheat(PlayerManager* players, NetworkServer* network,
              const AntiCheatConfig& config);

    bool ValidatePosition(uint16_t playerId,
                          const Vector3& newPos,
                          const Vector3& oldPos,
                          float deltaTime);
    bool ValidateHealth(uint16_t playerId, int newHealth, int oldHealth,
                        float deltaTime);
    bool ValidateWeapon(uint16_t playerId, uint32_t weaponHash);
    bool ValidatePacketRate(uint16_t playerId);
    bool ValidateSequence(uint16_t playerId, uint32_t sequence);

    void RecordViolation(uint16_t playerId, const std::string& reason,
                         ViolationSeverity severity,
                         const std::string& details = "");

    using ViolationCallback = std::function<void(const Violation&)>;
    void OnViolation(ViolationCallback cb) { m_onViolation = cb; }
    const std::vector<Violation>& GetViolations() const { return m_violations; }

private:
    struct PlayerACState {
        Vector3  lastPosition;
        int      lastHealth        = 200;
        uint32_t lastSequence      = 0;
        int      packetsThisSecond = 0;
        int      highViolations    = 0;
        using Clock = std::chrono::steady_clock;
        Clock::time_point lastPositionTime;
        Clock::time_point lastPacketTime;
    };

    PlayerManager*   m_players;
    NetworkServer*   m_network;
    AntiCheatConfig  m_config;

    std::vector<Violation>                      m_violations;
    ViolationCallback                           m_onViolation;
    std::unordered_map<uint16_t, PlayerACState> m_states;
    std::unordered_set<uint32_t>                m_validWeapons;
};

} // namespace Atlas
