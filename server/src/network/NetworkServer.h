#pragma once
// server/src/network/NetworkServer.h
// ENet-based UDP server — manages all player connections

#include "../../../shared/include/Packets.h"
#include "../../../shared/include/Types.h"
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

// Forward declare ENet types to avoid including enet.h in this header
struct _ENetHost;
struct _ENetPeer;
using ENetHost = _ENetHost;
using ENetPeer = _ENetPeer;

namespace Atlas {

class Config;
class PlayerManager;

// ─── Receive callback ─────────────────────────────────────────────────────────
using PacketRecvCallback = std::function<void(
    PlayerId senderId,
    Packets::PacketType type,
    const uint8_t* data,
    size_t dataLen
)>;

// ─── NetworkServer ────────────────────────────────────────────────────────────
class NetworkServer {
public:
    explicit NetworkServer(const Config& config, PlayerManager* players);
    ~NetworkServer();

    bool Start();
    void Stop();

    /// Poll network events — call this every server tick
    void Poll();

    /// Send a packet to a specific player
    bool SendTo(PlayerId playerId, Packets::PacketType type,
                const void* data, size_t dataLen, bool reliable = true);

    /// Broadcast a packet to all connected players
    void Broadcast(Packets::PacketType type,
                   const void* data, size_t dataLen, bool reliable = true);

    /// Broadcast to all except one player
    void BroadcastExcept(PlayerId excludeId, Packets::PacketType type,
                         const void* data, size_t dataLen, bool reliable = true);

    /// Kick a player with a reason message
    void KickPlayer(PlayerId playerId, const std::string& reason);

    /// Register a handler for incoming packets of a specific type
    void OnPacket(Packets::PacketType type, PacketRecvCallback cb);

    bool IsRunning() const { return m_running; }
    uint32_t GetPlayerCount() const;

private:
    void OnConnect(ENetPeer* peer);
    void OnDisconnect(ENetPeer* peer, uint32_t data);
    void OnReceive(ENetPeer* peer, const uint8_t* data, size_t len);

    void HandleHandshake(ENetPeer* peer,
                         const Packets::HandshakeRequest& req);

    ENetHost* m_host = nullptr;

    const Config& m_config;
    PlayerManager* m_players;

    // peer → playerId mapping
    std::unordered_map<ENetPeer*, PlayerId> m_peerToPlayer;
    // playerId → peer mapping
    std::unordered_map<PlayerId, ENetPeer*> m_playerToPeer;

    // Registered packet handlers
    std::unordered_map<uint16_t, PacketRecvCallback> m_handlers;

    bool m_running = false;
    uint16_t m_nextPlayerId = 1;
};

} // namespace Atlas
