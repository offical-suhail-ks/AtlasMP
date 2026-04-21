#pragma once
// client/src/network/NetworkClient.h
#include "../../../shared/include/Packets.h"
#include "../../../shared/include/Types.h"
#include <string>
#include <functional>
#include <vector>
#include <cstdint>

struct _ENetHost; struct _ENetPeer;
using ENetHost = _ENetHost; using ENetPeer = _ENetPeer;

namespace Atlas {

using ClientPacketCallback = std::function<void(
    Packets::PacketType type, const uint8_t* data, size_t len)>;

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool Connect(const std::string& host, uint16_t port,
                 const std::string& playerName,
                 const std::string& password = "");
    void Disconnect();
    void Poll();

    bool Send(Packets::PacketType type, const void* data,
              size_t len, bool reliable = true);

    void OnPacket(Packets::PacketType type, ClientPacketCallback cb);

    bool     IsConnected() const { return m_connected; }
    PlayerId GetLocalId()  const { return m_localId; }

private:
    void OnReceive(const uint8_t* data, size_t len);
    void HandleHandshakeResponse(const Packets::HandshakeResponse& resp);

    ENetHost* m_host   = nullptr;
    ENetPeer* m_server = nullptr;

    bool     m_connected = false;
    PlayerId m_localId   = INVALID_PLAYER;

    std::vector<std::pair<Packets::PacketType, ClientPacketCallback>> m_handlers;
};

} // namespace Atlas
