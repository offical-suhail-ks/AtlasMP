// tests/client/TestPacketCodec.cpp
// Unit tests for packet serialization / deserialization

#include <gtest/gtest.h>
#include "../../shared/include/Packets.h"
#include "../../shared/include/Types.h"
#include <cstring>

using namespace Atlas;
using namespace Atlas::Packets;

// ─── Header ───────────────────────────────────────────────────────────────────

TEST(PacketHeader, SizeIs14Bytes) {
    EXPECT_EQ(sizeof(PacketHeader), 14u);
}

TEST(PacketHeader, MagicConstant) {
    EXPECT_EQ(ATLAS_MAGIC, 0x41544C53u);
}

// ─── Handshake ────────────────────────────────────────────────────────────────

TEST(HandshakeRequest, FitsInMaxPacketSize) {
    EXPECT_LE(sizeof(HandshakeRequest), ATLAS_MAX_PACKET_SIZE);
}

TEST(HandshakeRequest, NameFieldSize) {
    HandshakeRequest req;
    EXPECT_EQ(sizeof(req.name), 32u);
}

TEST(HandshakeResponse, HasPlayerIdField) {
    HandshakeResponse resp;
    resp.accepted  = true;
    resp.playerId  = 42;
    resp.reason    = 0;
    EXPECT_EQ(resp.playerId, 42);
    EXPECT_TRUE(resp.accepted);
}

// ─── PlayerState ──────────────────────────────────────────────────────────────

TEST(PlayerStatePacket, PositionRoundTrip) {
    PlayerStatePacket pkt;
    pkt.playerId   = 7;
    pkt.position   = { 100.5f, -200.0f, 31.2f };
    pkt.velocity   = { 0.0f,   0.0f,   -9.8f };
    pkt.health     = 150;
    pkt.armour     = 50;

    // Simulate raw copy (like what happens over the network)
    uint8_t buffer[sizeof(PlayerStatePacket)];
    memcpy(buffer, &pkt, sizeof(pkt));

    PlayerStatePacket out;
    memcpy(&out, buffer, sizeof(out));

    EXPECT_EQ(out.playerId, 7);
    EXPECT_FLOAT_EQ(out.position.x, 100.5f);
    EXPECT_FLOAT_EQ(out.position.y, -200.0f);
    EXPECT_FLOAT_EQ(out.position.z, 31.2f);
    EXPECT_EQ(out.health, 150);
    EXPECT_EQ(out.armour, 50);
}

TEST(PlayerStatePacket, FlagsField) {
    PlayerStatePacket pkt;
    pkt.flags = 0;
    pkt.flags |= (1 << 0); // ON_GROUND
    pkt.flags |= (1 << 2); // AIMING

    EXPECT_TRUE(pkt.flags & (1 << 0));  // on ground
    EXPECT_FALSE(pkt.flags & (1 << 1)); // not in vehicle
    EXPECT_TRUE(pkt.flags & (1 << 2));  // aiming
}

// ─── VehicleState ─────────────────────────────────────────────────────────────

TEST(VehicleStatePacket, EngineHealthDefault) {
    VehicleStatePacket pkt;
    pkt.vehicleId    = 1;
    pkt.driverPlayerId = 3;
    pkt.engineHealth = 1000.0f;
    pkt.modelHash    = 0x18D5FA52; // adder

    EXPECT_EQ(pkt.vehicleId, 1u);
    EXPECT_EQ(pkt.driverPlayerId, 3u);
    EXPECT_FLOAT_EQ(pkt.engineHealth, 1000.0f);
}

// ─── Disconnect ──────────────────────────────────────────────────────────────

TEST(DisconnectPacket, MessageFits) {
    DisconnectPacket pkt;
    pkt.reason = (uint8_t)DisconnectReason::Kicked;
    strncpy(pkt.message, "You were kicked.", sizeof(pkt.message) - 1);

    EXPECT_EQ(pkt.reason, (uint8_t)DisconnectReason::Kicked);
    EXPECT_STREQ(pkt.message, "You were kicked.");
}

// ─── PacketType enum ──────────────────────────────────────────────────────────

TEST(PacketType, HandshakeValues) {
    EXPECT_EQ((uint16_t)PacketType::HANDSHAKE_REQUEST,  0x0001);
    EXPECT_EQ((uint16_t)PacketType::HANDSHAKE_RESPONSE, 0x0002);
    EXPECT_EQ((uint16_t)PacketType::DISCONNECT,         0x0003);
}

TEST(PacketType, PlayerStateValue) {
    EXPECT_EQ((uint16_t)PacketType::PLAYER_STATE, 0x0100);
}

TEST(PacketType, VehicleStateValue) {
    EXPECT_EQ((uint16_t)PacketType::VEHICLE_STATE, 0x0101);
}
