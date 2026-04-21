// client/src/network/NetworkClient.cpp
#include "NetworkClient.h"
#include "../core/Logger.h"
#include <enet/enet.h>
#include <cstring>
#include <vector>

namespace Atlas {

NetworkClient::NetworkClient() {
    enet_initialize();
}

NetworkClient::~NetworkClient() {
    Disconnect();
    enet_deinitialize();
}

bool NetworkClient::Connect(const std::string& host, uint16_t port,
                             const std::string& playerName,
                             const std::string& password)
{
    m_host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!m_host) {
        Logger::Error("[NetClient] Failed to create ENet host");
        return false;
    }

    ENetAddress addr{};
    enet_address_set_host(&addr, host.c_str());
    addr.port = port;

    m_server = enet_host_connect(m_host, &addr, 2, 0);
    if (!m_server) {
        Logger::Error("[NetClient] Failed to initiate connection");
        return false;
    }

    // Wait up to 5s for connection
    ENetEvent event{};
    if (enet_host_service(m_host, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        Logger::Info("[NetClient] Connected to {}:{}", host.c_str(), port);

        // Send handshake
        Packets::HandshakeRequest req{};
        req.protocolVersion = ATLAS_PROTOCOL_VERSION;
        req.gameVersion     = 3095;
        strncpy(req.name,     playerName.c_str(), sizeof(req.name) - 1);
        strncpy(req.password, password.c_str(),   sizeof(req.password) - 1);
        req.clientVersion[0] = 0;
        req.clientVersion[1] = 1;
        req.clientVersion[2] = 0;

        Send(Packets::PacketType::HANDSHAKE_REQUEST, &req, sizeof(req));
        return true;
    }

    Logger::Error("[NetClient] Connection to {}:{} timed out", host.c_str(), port);
    enet_peer_reset(m_server);
    m_server = nullptr;
    return false;
}

void NetworkClient::Disconnect() {
    if (m_server && m_connected) {
        enet_peer_disconnect(m_server, 0);
        m_connected = false;
        m_localId   = INVALID_PLAYER;
    }
    if (m_host) {
        enet_host_destroy(m_host);
        m_host = nullptr;
    }
}

void NetworkClient::Poll() {
    if (!m_host) return;
    ENetEvent event{};
    while (enet_host_service(m_host, &event, 0) > 0) {
        if (event.type == ENET_EVENT_TYPE_RECEIVE && event.packet) {
            OnReceive(event.packet->data, event.packet->dataLength);
            enet_packet_destroy(event.packet);
        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            Logger::Info("[NetClient] Disconnected from server");
            m_connected = false;
            m_server    = nullptr;
        }
    }
}

bool NetworkClient::Send(Packets::PacketType type, const void* data,
                          size_t len, bool reliable)
{
    if (!m_server) return false;

    std::vector<uint8_t> buf(sizeof(Packets::PacketHeader) + len);
    auto* hdr     = reinterpret_cast<Packets::PacketHeader*>(buf.data());
    hdr->magic    = ATLAS_MAGIC;
    hdr->type     = type;
    hdr->size     = (uint16_t)len;
    hdr->sequence = 0;
    hdr->senderId = m_localId;
    if (data && len)
        memcpy(buf.data() + sizeof(Packets::PacketHeader), data, len);

    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* pkt = enet_packet_create(buf.data(), buf.size(), flags);
    return enet_peer_send(m_server, reliable ? 0 : 1, pkt) == 0;
}

void NetworkClient::OnReceive(const uint8_t* data, size_t len) {
    if (len < sizeof(Packets::PacketHeader)) return;
    const auto* hdr = reinterpret_cast<const Packets::PacketHeader*>(data);
    if (hdr->magic != ATLAS_MAGIC) return;

    const uint8_t* payload = data + sizeof(Packets::PacketHeader);
    size_t payloadLen      = len  - sizeof(Packets::PacketHeader);

    if (hdr->type == Packets::PacketType::HANDSHAKE_RESPONSE &&
        payloadLen >= sizeof(Packets::HandshakeResponse))
    {
        HandleHandshakeResponse(
            *reinterpret_cast<const Packets::HandshakeResponse*>(payload));
        return;
    }

    for (auto& [type, cb] : m_handlers)
        if (type == hdr->type)
            cb(hdr->type, payload, payloadLen);
}

void NetworkClient::HandleHandshakeResponse(
    const Packets::HandshakeResponse& resp)
{
    if (resp.accepted) {
        m_localId   = resp.playerId;
        m_connected = true;
        Logger::Info("[NetClient] Authenticated — player ID {}", (int)m_localId);
    } else {
        Logger::Error("[NetClient] Connection rejected: {}", resp.message);
        enet_peer_disconnect(m_server, 0);
    }
}

void NetworkClient::OnPacket(Packets::PacketType type,
                               ClientPacketCallback cb)
{
    m_handlers.emplace_back(type, std::move(cb));
}

} // namespace Atlas
