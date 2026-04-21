#pragma once
// server/src/sync/SyncManager.h
// Server-authoritative entity sync system.
// Receives state from clients, validates, stores, and rebroadcasts.

#include "../../../shared/include/Packets.h"
#include "../../../shared/include/Types.h"
#include "../../../shared/include/AtlasMath.h"
#include <unordered_map>
#include <memory>
#include <vector>

namespace Atlas {

class NetworkServer;
class PlayerManager;
class AntiCheat;

// ─── Interest zone config ─────────────────────────────────────────────────────
struct SyncConfig
{
    float interestDistance = 500.0f;   // Only sync entities within this range (metres)
    int   tickRate         = 60;       // How many times per second to broadcast state
    bool  deltaCompression = true;     // Only send changed fields
};

// ─── SyncManager ─────────────────────────────────────────────────────────────
class SyncManager
{
public:
    SyncManager(NetworkServer* network, PlayerManager* players, const SyncConfig& config);
    ~SyncManager() = default;

    // Called every server tick
    void Tick(float deltaTime);

    // ── Receive from client ────────────────────────────────────────────────

    /// Client sent their player state — validate and store
    void OnPlayerStateReceived(uint16_t senderId,
                               const Packets::PlayerStatePacket& pkt);

    /// Client sent vehicle state they own
    void OnVehicleStateReceived(uint16_t senderId,
                                const Packets::VehicleStatePacket& pkt);

    // ── Server creates/destroys entities ──────────────────────────────────

    EntityId CreateVehicle(uint32_t modelHash, const Vector3& pos,
                           const Quaternion& rot, uint16_t ownerId);
    void     DestroyEntity(EntityId id);

    void     SetVehicleOwner(EntityId vehicleId, uint16_t newOwnerId);

private:
    // ── Broadcast logic ───────────────────────────────────────────────────

    void BroadcastPlayerStates();
    void BroadcastVehicleStates();

    /// Determine which players should receive updates about a given entity
    std::vector<uint16_t> GetRelevantPlayers(const Vector3& entityPos,
                                              uint16_t excludeId = INVALID_PLAYER);

    // ── Validation ────────────────────────────────────────────────────────

    bool ValidatePlayerState(uint16_t playerId,
                             const Packets::PlayerStatePacket& pkt);

    NetworkServer* m_network;
    PlayerManager* m_players;
    SyncConfig     m_config;

    // Server-side vehicle state store
    struct VehicleState
    {
        EntityId   id;
        uint32_t   modelHash;
        uint16_t   ownerId;
        Vector3    position;
        Vector3    velocity;
        Quaternion rotation;
        float      engineHealth = 1000.0f;
        float      bodyHealth   = 1000.0f;
        bool       engineOn     = false;
    };

    std::unordered_map<EntityId, VehicleState> m_vehicles;
    EntityId m_nextEntityId = 1;

    float m_timeSinceBroadcast = 0.0f;
    float m_broadcastInterval;   // 1.0 / tickRate
};

} // namespace Atlas
