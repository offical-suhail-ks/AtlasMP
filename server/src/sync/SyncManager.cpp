// server/src/sync/SyncManager.cpp
#include "SyncManager.h"
#include "../core/Logger.h"
#include "../network/NetworkServer.h"
#include "../../include/PlayerManager.h"
#include <cmath>

namespace Atlas {

SyncManager::SyncManager(NetworkServer* network, PlayerManager* players,
                          const SyncConfig& config)
    : m_network(network), m_players(players), m_config(config)
{
    m_broadcastInterval = 1.0f / (float)m_config.tickRate;

    // Register packet handlers
    m_network->OnPacket(Packets::PacketType::PLAYER_STATE,
        [this](PlayerId id, Packets::PacketType, const uint8_t* data, size_t len) {
            if (len >= sizeof(Packets::PlayerStatePacket))
                OnPlayerStateReceived(id, *reinterpret_cast<const Packets::PlayerStatePacket*>(data));
        });

    m_network->OnPacket(Packets::PacketType::VEHICLE_STATE,
        [this](PlayerId id, Packets::PacketType, const uint8_t* data, size_t len) {
            if (len >= sizeof(Packets::VehicleStatePacket))
                OnVehicleStateReceived(id, *reinterpret_cast<const Packets::VehicleStatePacket*>(data));
        });
}

void SyncManager::Tick(float deltaTime) {
    m_timeSinceBroadcast += deltaTime;
    if (m_timeSinceBroadcast >= m_broadcastInterval) {
        m_timeSinceBroadcast = 0.0f;
        BroadcastPlayerStates();
        BroadcastVehicleStates();
    }
}

void SyncManager::OnPlayerStateReceived(uint16_t senderId,
                                         const Packets::PlayerStatePacket& pkt)
{
    auto* player = m_players->GetPlayer(senderId);
    if (!player) return;

    if (!ValidatePlayerState(senderId, pkt)) return;

    // Update authoritative state
    player->position   = pkt.position;
    player->velocity   = pkt.velocity;
    player->rotation   = pkt.rotation;
    player->heading    = pkt.heading;
    player->health     = pkt.health;
    player->armour     = pkt.armour;
    player->weaponHash = 0; // TODO: restore from pkt
}

void SyncManager::OnVehicleStateReceived(uint16_t senderId,
                                          const Packets::VehicleStatePacket& pkt)
{
    auto it = m_vehicles.find(pkt.vehicleId);
    if (it == m_vehicles.end()) return;
    if (it->second.ownerId != senderId) return; // Only owner can update

    it->second.position     = pkt.position;
    it->second.velocity     = pkt.velocity;
    it->second.rotation     = pkt.rotation;
    it->second.engineHealth = pkt.engineHealth;
    it->second.bodyHealth   = pkt.bodyHealth;
}

EntityId SyncManager::CreateVehicle(uint32_t modelHash, const Vector3& pos,
                                     const Quaternion& rot, uint16_t ownerId)
{
    VehicleState v;
    v.id        = m_nextEntityId++;
    v.modelHash = modelHash;
    v.position  = pos;
    v.rotation  = rot;
    v.ownerId   = ownerId;
    m_vehicles[v.id] = v;

    // Broadcast creation to all players
    Packets::EntityCreatePacket pkt{};
    pkt.entityId   = v.id;
    pkt.entityType = (uint8_t)EntityType::Vehicle;
    pkt.modelHash  = modelHash;
    pkt.position   = pos;
    pkt.rotation   = rot;
    pkt.ownerId    = ownerId;
    m_network->Broadcast(Packets::PacketType::ENTITY_CREATE, &pkt, sizeof(pkt));
    return v.id;
}

void SyncManager::DestroyEntity(EntityId id) {
    m_vehicles.erase(id);
    Packets::EntityDestroyPacket pkt{ id };
    m_network->Broadcast(Packets::PacketType::ENTITY_DESTROY, &pkt, sizeof(pkt));
}

void SyncManager::BroadcastPlayerStates() {
    m_players->ForEach([&](Player& sender) {
        Packets::PlayerStatePacket pkt{};
        pkt.playerId  = sender.id;
        pkt.position  = sender.position;
        pkt.velocity  = sender.velocity;
        pkt.rotation  = sender.rotation;
        pkt.heading   = sender.heading;
        pkt.health    = (int16_t)sender.health;
        pkt.armour    = (int16_t)sender.armour;

        // Send to all OTHER players
        m_network->BroadcastExcept(sender.id,
            Packets::PacketType::PLAYER_STATE,
            &pkt, sizeof(pkt), false); // unreliable for state
    });

    // ── DEBUG: fake moving player (test remote rendering with one real client) ──
    // Phantom player 999 walks a slow circle near the airport spawn so a single
    // connected client can see a remote ped appear and move. Remove this block
    // once real two-client testing works.
#if 1
    {
        static float t = 0.0f;
        t += 0.05f;
        Packets::PlayerStatePacket fake{};
        fake.playerId = 999;
        // Circle of radius 5m around a fixed point near the client's spawn.
        fake.position.x = -1165.9f + 5.0f * cosf(t);
        fake.position.y = -1426.4f + 5.0f * sinf(t);
        fake.position.z = 4.6f;
        fake.heading    = t * 57.2958f; // radians->deg, so it faces its travel
        fake.health     = 200;
        // Broadcast to EVERYONE (the phantom has no own connection to exclude).
        m_network->Broadcast(Packets::PacketType::PLAYER_STATE,
                             &fake, sizeof(fake), false);
    }
#endif
}

void SyncManager::BroadcastVehicleStates() {
    for (auto& [id, v] : m_vehicles) {
        Packets::VehicleStatePacket pkt{};
        pkt.vehicleId      = v.id;
        pkt.driverPlayerId = v.ownerId;
        pkt.position       = v.position;
        pkt.velocity       = v.velocity;
        pkt.rotation       = v.rotation;
        pkt.engineHealth   = v.engineHealth;
        pkt.bodyHealth     = v.bodyHealth;
        pkt.modelHash      = v.modelHash;

        auto relevant = GetRelevantPlayers(v.position, INVALID_PLAYER);
        for (auto pid : relevant)
            m_network->SendTo(pid, Packets::PacketType::VEHICLE_STATE,
                              &pkt, sizeof(pkt), false);
    }
}

std::vector<uint16_t> SyncManager::GetRelevantPlayers(const Vector3& pos,
                                                        uint16_t excludeId)
{
    std::vector<uint16_t> out;
    m_players->ForEach([&](Player& p) {
        if (p.id == excludeId) return;
        if (p.position.distance(pos) <= m_config.interestDistance)
            out.push_back(p.id);
    });
    return out;
}

bool SyncManager::ValidatePlayerState(uint16_t /*playerId*/,
                                       const Packets::PlayerStatePacket& /*pkt*/)
{
    // TODO: hook into AntiCheat for speed/health checks
    return true;
}

} // namespace Atlas