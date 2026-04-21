// server/src/core/AntiCheat.cpp
#include "AntiCheat.h"
#include "Logger.h"
#include "../../include/PlayerManager.h"
#include "../network/NetworkServer.h"

namespace Atlas {

AntiCheat::AntiCheat(PlayerManager* players, NetworkServer* network,
                     const AntiCheatConfig& config)
    : m_players(players), m_network(network), m_config(config)
{}

bool AntiCheat::ValidatePosition(uint16_t playerId,
                                  const Vector3& newPos,
                                  const Vector3& oldPos,
                                  float deltaTime)
{
    if (!m_config.enabled || deltaTime <= 0.0f) return true;

    float dist  = oldPos.distance(newPos);
    float speed = dist / deltaTime;

    if (speed > m_config.maxSpeedMps) {
        RecordViolation(playerId,
            "Speed hack detected",
            ViolationSeverity::High,
            "Speed: " + std::to_string(speed) + " m/s (max: "
                      + std::to_string(m_config.maxSpeedMps) + ")");
        return false;
    }
    return true;
}

bool AntiCheat::ValidateHealth(uint16_t playerId, int newHealth, int oldHealth,
                                float deltaTime)
{
    if (!m_config.enabled) return true;
    if (newHealth <= oldHealth) return true; // damage is always OK

    float gain = (float)(newHealth - oldHealth);
    float rate = gain / (deltaTime > 0.0f ? deltaTime : 1.0f);

    if (rate > m_config.maxHealthGainPerS) {
        RecordViolation(playerId,
            "Health hack detected",
            ViolationSeverity::High,
            "Gain rate: " + std::to_string(rate) + " hp/s");
        return false;
    }
    return true;
}

bool AntiCheat::ValidateWeapon(uint16_t /*playerId*/, uint32_t /*weaponHash*/) {
    // TODO: check against m_validWeapons set loaded from nativedb
    return true;
}

bool AntiCheat::ValidatePacketRate(uint16_t playerId) {
    if (!m_config.enabled) return true;

    auto& state = m_states[playerId];
    auto  now   = PlayerACState::Clock::now();
    float elapsed = std::chrono::duration<float>(now - state.lastPacketTime).count();

    if (elapsed >= 1.0f) {
        state.packetsThisSecond = 0;
        state.lastPacketTime    = now;
    }

    state.packetsThisSecond++;
    if (state.packetsThisSecond > m_config.maxPacketsPerSecond) {
        RecordViolation(playerId,
            "Packet flood",
            ViolationSeverity::Medium,
            "Packets/s: " + std::to_string(state.packetsThisSecond));
        return false;
    }
    return true;
}

bool AntiCheat::ValidateSequence(uint16_t playerId, uint32_t sequence) {
    if (!m_config.enabled) return true;

    auto& state = m_states[playerId];
    if (sequence <= state.lastSequence && state.lastSequence > 0) {
        RecordViolation(playerId, "Packet replay", ViolationSeverity::Low,
            "Seq " + std::to_string(sequence)
            + " <= last " + std::to_string(state.lastSequence));
        return false;
    }
    state.lastSequence = sequence;
    return true;
}

void AntiCheat::RecordViolation(uint16_t playerId,
                                 const std::string& reason,
                                 ViolationSeverity severity,
                                 const std::string& details)
{
    Violation v;
    v.playerId = playerId;
    v.reason   = reason;
    v.severity = severity;
    v.details  = details;
    v.time     = std::chrono::system_clock::now();
    m_violations.push_back(v);

    Logger::Warn("[AntiCheat] Player {} — {} ({})",
        playerId, reason.c_str(), details.c_str());

    if (m_onViolation) m_onViolation(v);

    if (severity == ViolationSeverity::High) {
        auto& state = m_states[playerId];
        state.highViolations++;

        if (m_config.kickOnViolation &&
            state.highViolations >= m_config.violationsToKick)
        {
            Logger::Warn("[AntiCheat] Kicking player {} — too many violations", playerId);
            if (m_network)
                m_network->KickPlayer(playerId, "Anti-cheat: " + reason);
        }
    }
}

} // namespace Atlas
