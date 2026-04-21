// server/src/network/NetworkServer.cpp
#include "NetworkServer.h"
#include "../core/Config.h"
#include "../core/Logger.h"
#include "../../include/PlayerManager.h"
#include "../../../shared/include/Packets.h"

#include <enet/enet.h>
#include <cstring>

namespace Atlas {

NetworkServer::NetworkServer(const Config& config, PlayerManager* players)
    : m_config(config), m_players(players) {}

NetworkServer::~NetworkServer() { Stop(); }

bool NetworkServer::Start() {
    if (enet_initialize() != 0) {
        Logger::Error("[Network] Failed to initialize ENet!");
        return false;
    }

    ENetAddress addr;
    addr.host = ENET_HOST_ANY;
    addr.port = m_config.GetPort();

    m_host = enet_host_create(&addr,
        m_config.GetMaxPlayers(), // max connections
        2,                        // channels (0=reliable, 1=unreliable)
        0, 0);                    // no bandwidth limits

    if (!m_host) {
        Logger::Error("[Network] Failed to create ENet host on port {}", m_config.GetPort());
        enet_deinitialize();
        return false;
    }

    m_running = true;
    Logger::Info("[Network] ENet server listening on {}:{}",
        m_config.GetHost(), m_config.GetPort());
    return true;
}

void NetworkServer::Stop() {
    if (!m_running) return;
    m_running = false;

    if (m_host) {
        // Disconnect all peers gracefully
        for (size_t i = 0; i < m_host->peerCount; i++) {
            if (m_host->peers[i].state == ENET_PEER_STATE_CONNECTED)
                enet_peer_disconnect_now(&m_host->peers[i], 0);
        }
        enet_host_destroy(m_host);
        m_host = nullptr;
    }
    enet_deinitialize();
    Logger::Info("[Network] Server stopped.");
}

void NetworkServer::Poll() {
    if (!m_host) return;

    ENetEvent event;
    // Process up to 100 events per tick (non-blocking: timeout=0)
    while (enet_host_service(m_host, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            OnConnect(event.peer);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            OnDisconnect(event.peer, event.data);
            break;

        case ENET_EVENT_TYPE_RECEIVE:
            if (event.packet) {
                OnReceive(event.peer,
                          event.packet->data,
                          event.packet->dataLength);
                enet_packet_destroy(event.packet);
            }
            break;

        default: break;
        }
    }
}

void NetworkServer::OnConnect(ENetPeer* peer) {
    char ipBuf[64] = {};
    enet_address_get_host_ip(&peer->address, ipBuf, sizeof(ipBuf));
    Logger::Info("[Network] New connection from {} — waiting for handshake", ipBuf);
    // Don't assign player ID yet — wait for HANDSHAKE_REQUEST
    peer->data = nullptr;
}

void NetworkServer::OnDisconnect(ENetPeer* peer, uint32_t /*data*/) {
    auto it = m_peerToPlayer.find(peer);
    if (it != m_peerToPlayer.end()) {
        PlayerId pid = it->second;
        Logger::Info("[Network] Player {} disconnected", pid);

        // Fire any registered disconnect handler
        auto hit = m_handlers.find((uint16_t)Packets::PacketType::PLAYER_LEAVE);
        if (hit != m_handlers.end()) {
            Packets::PlayerLeavePacket leavePkt{};
            leavePkt.playerId = pid;
            leavePkt.reason   = (uint8_t)DisconnectReason::Unknown;
            hit->second(pid, Packets::PacketType::PLAYER_LEAVE,
                        reinterpret_cast<const uint8_t*>(&leavePkt),
                        sizeof(leavePkt));
        }

        m_playerToPeer.erase(pid);
        m_peerToPlayer.erase(it);
        m_players->RemovePlayer(pid);
    }
    peer->data = nullptr;
}

void NetworkServer::OnReceive(ENetPeer* peer,
                               const uint8_t* data, size_t len) {
    if (len < sizeof(Packets::PacketHeader)) return;

    const auto* hdr = reinterpret_cast<const Packets::PacketHeader*>(data);
    if (hdr->magic != ATLAS_MAGIC) return;

    const uint8_t* payload = data + sizeof(Packets::PacketHeader);
    size_t payloadLen      = len  - sizeof(Packets::PacketHeader);

    // Special case: handshake (player not yet in map)
    if (hdr->type == Packets::PacketType::HANDSHAKE_REQUEST) {
        if (payloadLen >= sizeof(Packets::HandshakeRequest)) {
            const auto& req = *reinterpret_cast<const Packets::HandshakeRequest*>(payload);
            HandleHandshake(peer, req);
        }
        return;
    }

    // Resolve sender player ID
    auto it = m_peerToPlayer.find(peer);
    if (it == m_peerToPlayer.end()) return; // Not authenticated yet
    PlayerId senderId = it->second;

    // Dispatch to registered handler
    auto hit = m_handlers.find((uint16_t)hdr->type);
    if (hit != m_handlers.end())
        hit->second(senderId, hdr->type, payload, payloadLen);
}

void NetworkServer::HandleHandshake(ENetPeer* peer,
                                     const Packets::HandshakeRequest& req)
{
    char ipBuf[64] = {};
    enet_address_get_host_ip(&peer->address, ipBuf, sizeof(ipBuf));

    // Version check
    if (req.protocolVersion != ATLAS_PROTOCOL_VERSION) {
        Packets::HandshakeResponse resp{};
        resp.accepted = false;
        resp.reason   = (uint8_t)DisconnectReason::ProtocolError;
        strncpy(resp.message, "Protocol version mismatch", sizeof(resp.message)-1);
        SendTo(INVALID_PLAYER, Packets::PacketType::HANDSHAKE_RESPONSE,
               &resp, sizeof(resp), true);
        enet_peer_disconnect_later(peer, 0);
        return;
    }

    // Password check
    if (!m_config.GetPassword().empty() &&
        m_config.GetPassword() != req.password) {
        Packets::HandshakeResponse resp{};
        resp.accepted = false;
        resp.reason   = (uint8_t)DisconnectReason::Kicked;
        strncpy(resp.message, "Wrong password", sizeof(resp.message)-1);
        // Send directly via peer since not yet in map
        std::vector<uint8_t> buf(sizeof(Packets::PacketHeader) + sizeof(resp));
        auto* hdr     = reinterpret_cast<Packets::PacketHeader*>(buf.data());
        hdr->magic    = ATLAS_MAGIC;
        hdr->type     = Packets::PacketType::HANDSHAKE_RESPONSE;
        hdr->size     = sizeof(resp);
        hdr->sequence = 0;
        hdr->senderId = 0;
        memcpy(buf.data() + sizeof(Packets::PacketHeader), &resp, sizeof(resp));
        ENetPacket* pkt = enet_packet_create(buf.data(), buf.size(),
                                              ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, 0, pkt);
        enet_peer_disconnect_later(peer, 0);
        return;
    }

    // Create player
    std::string name(req.name, strnlen(req.name, sizeof(req.name)));
    PlayerId pid = m_players->AddPlayer(name, ipBuf);

    m_peerToPlayer[peer]  = pid;
    m_playerToPeer[pid]   = peer;

    Logger::Info("[Network] Player {} ({}) authenticated as ID {}",
        name, ipBuf, pid);

    // Send accepted response
    Packets::HandshakeResponse resp{};
    resp.accepted = true;
    resp.playerId = pid;
    strncpy(resp.message, "Welcome to AtlasMP!", sizeof(resp.message)-1);

    std::vector<uint8_t> buf(sizeof(Packets::PacketHeader) + sizeof(resp));
    auto* hdr     = reinterpret_cast<Packets::PacketHeader*>(buf.data());
    hdr->magic    = ATLAS_MAGIC;
    hdr->type     = Packets::PacketType::HANDSHAKE_RESPONSE;
    hdr->size     = sizeof(resp);
    hdr->sequence = 0;
    hdr->senderId = 0;
    memcpy(buf.data() + sizeof(Packets::PacketHeader), &resp, sizeof(resp));

    ENetPacket* pkt = enet_packet_create(buf.data(), buf.size(),
                                          ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, pkt);

    // Fire playerJoin handler
    auto hit = m_handlers.find((uint16_t)Packets::PacketType::PLAYER_JOIN);
    if (hit != m_handlers.end()) {
        Packets::PlayerJoinPacket joinPkt{};
        joinPkt.playerId = pid;
        strncpy(joinPkt.name, name.c_str(), sizeof(joinPkt.name)-1);
        hit->second(pid, Packets::PacketType::PLAYER_JOIN,
                    reinterpret_cast<const uint8_t*>(&joinPkt),
                    sizeof(joinPkt));
    }
}

bool NetworkServer::SendTo(PlayerId playerId, Packets::PacketType type,
                            const void* data, size_t dataLen, bool reliable)
{
    auto it = m_playerToPeer.find(playerId);
    if (it == m_playerToPeer.end()) return false;

    std::vector<uint8_t> buf(sizeof(Packets::PacketHeader) + dataLen);
    auto* hdr     = reinterpret_cast<Packets::PacketHeader*>(buf.data());
    hdr->magic    = ATLAS_MAGIC;
    hdr->type     = type;
    hdr->size     = (uint16_t)dataLen;
    hdr->sequence = 0;
    hdr->senderId = 0;
    if (data && dataLen) memcpy(buf.data() + sizeof(Packets::PacketHeader), data, dataLen);

    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* pkt = enet_packet_create(buf.data(), buf.size(), flags);
    return enet_peer_send(it->second, reliable ? 0 : 1, pkt) == 0;
}

void NetworkServer::Broadcast(Packets::PacketType type,
                               const void* data, size_t dataLen, bool reliable)
{
    for (auto& [pid, peer] : m_playerToPeer)
        SendTo(pid, type, data, dataLen, reliable);
}

void NetworkServer::BroadcastExcept(PlayerId excludeId,
                                     Packets::PacketType type,
                                     const void* data, size_t dataLen,
                                     bool reliable)
{
    for (auto& [pid, peer] : m_playerToPeer)
        if (pid != excludeId)
            SendTo(pid, type, data, dataLen, reliable);
}

void NetworkServer::KickPlayer(PlayerId playerId, const std::string& reason) {
    auto it = m_playerToPeer.find(playerId);
    if (it == m_playerToPeer.end()) return;

    Packets::DisconnectPacket pkt{};
    pkt.reason = (uint8_t)DisconnectReason::Kicked;
    strncpy(pkt.message, reason.c_str(), sizeof(pkt.message)-1);
    SendTo(playerId, Packets::PacketType::DISCONNECT, &pkt, sizeof(pkt));
    enet_peer_disconnect_later(it->second, 0);
    Logger::Info("[Network] Kicked player {} — {}", playerId, reason);
}

void NetworkServer::OnPacket(Packets::PacketType type, PacketRecvCallback cb) {
    m_handlers[(uint16_t)type] = std::move(cb);
}

uint32_t NetworkServer::GetPlayerCount() const {
    return (uint32_t)m_playerToPeer.size();
}

} // namespace Atlas
