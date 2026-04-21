// tests/server/TestNetworking.cpp
#include <gtest/gtest.h>
#include "../../shared/include/Packets.h"
#include "../../shared/include/Types.h"
using namespace Atlas;
using namespace Atlas::Packets;

TEST(Networking, MagicConstant) {
    EXPECT_EQ(ATLAS_MAGIC, 0x41544C53u);
}

TEST(Networking, DefaultPort) {
    EXPECT_EQ(ATLAS_DEFAULT_PORT, 7788);
}

TEST(Networking, MaxPlayers) {
    EXPECT_EQ(ATLAS_MAX_PLAYERS, 1024u);
}

TEST(Networking, PacketHeaderLayout) {
    // Verify the header is exactly 14 bytes (packed)
    EXPECT_EQ(sizeof(PacketHeader), 14u);
}

TEST(Networking, HandshakeRequestLayout) {
    HandshakeRequest req{};
    req.protocolVersion = ATLAS_PROTOCOL_VERSION;
    EXPECT_EQ(req.protocolVersion, ATLAS_PROTOCOL_VERSION);
    EXPECT_LT(sizeof(HandshakeRequest), (size_t)ATLAS_MAX_PACKET_SIZE);
}

TEST(Networking, InvalidPlayerConstant) {
    EXPECT_EQ(INVALID_PLAYER, 0xFFFFu);
}

TEST(Networking, DisconnectReasonValues) {
    EXPECT_EQ((uint8_t)DisconnectReason::Kicked,  1);
    EXPECT_EQ((uint8_t)DisconnectReason::Banned,  2);
    EXPECT_EQ((uint8_t)DisconnectReason::Timeout, 4);
}
