// tests/integration/TestHandshake.cpp
// Integration test: server handshake packet flow (no GTA V required)

#include <gtest/gtest.h>
#include "../../shared/include/Packets.h"
#include "../../shared/include/Types.h"
#include <cstring>

using namespace Atlas;
using namespace Atlas::Packets;

// ─── Simulated handshake ──────────────────────────────────────────────────────
// This tests the packet layer in isolation (no real networking)

class HandshakeTest : public ::testing::Test {
protected:
    // Simulate building a HANDSHAKE_REQUEST packet
    std::vector<uint8_t> BuildHandshakeRequest(const std::string& name,
                                                const std::string& password = "")
    {
        HandshakeRequest payload;
        payload.protocolVersion = ATLAS_PROTOCOL_VERSION;
        payload.gameVersion     = 3095;   // Example GTA V build
        strncpy(payload.name, name.c_str(), sizeof(payload.name) - 1);
        strncpy(payload.password, password.c_str(), sizeof(payload.password) - 1);
        payload.clientVersion[0] = 0;
        payload.clientVersion[1] = 1;
        payload.clientVersion[2] = 0;

        std::vector<uint8_t> buf(sizeof(PacketHeader) + sizeof(HandshakeRequest));

        PacketHeader* hdr = reinterpret_cast<PacketHeader*>(buf.data());
        hdr->magic    = ATLAS_MAGIC;
        hdr->type     = PacketType::HANDSHAKE_REQUEST;
        hdr->size     = sizeof(HandshakeRequest);
        hdr->sequence = 1;
        hdr->senderId = 0; // Server-bound: 0 = not yet assigned

        memcpy(buf.data() + sizeof(PacketHeader), &payload, sizeof(payload));
        return buf;
    }

    // Simulate server accepting the connection
    std::vector<uint8_t> BuildHandshakeResponse(bool accepted, uint16_t playerId,
                                                  const std::string& msg = "")
    {
        HandshakeResponse payload;
        payload.accepted  = accepted;
        payload.playerId  = playerId;
        payload.reason    = 0;
        strncpy(payload.message, msg.c_str(), sizeof(payload.message) - 1);

        std::vector<uint8_t> buf(sizeof(PacketHeader) + sizeof(HandshakeResponse));

        PacketHeader* hdr = reinterpret_cast<PacketHeader*>(buf.data());
        hdr->magic    = ATLAS_MAGIC;
        hdr->type     = PacketType::HANDSHAKE_RESPONSE;
        hdr->size     = sizeof(HandshakeResponse);
        hdr->sequence = 1;
        hdr->senderId = 0; // From server

        memcpy(buf.data() + sizeof(PacketHeader), &payload, sizeof(payload));
        return buf;
    }
};

// ─── Tests ────────────────────────────────────────────────────────────────────

TEST_F(HandshakeTest, RequestBuildsCorrectly) {
    auto buf = BuildHandshakeRequest("Suhail");

    ASSERT_GE(buf.size(), sizeof(PacketHeader) + sizeof(HandshakeRequest));

    const PacketHeader* hdr = reinterpret_cast<const PacketHeader*>(buf.data());
    EXPECT_EQ(hdr->magic, ATLAS_MAGIC);
    EXPECT_EQ(hdr->type, PacketType::HANDSHAKE_REQUEST);
    EXPECT_EQ(hdr->size, sizeof(HandshakeRequest));
    EXPECT_EQ(hdr->sequence, 1u);
}

TEST_F(HandshakeTest, RequestPayloadNameCorrect) {
    auto buf = BuildHandshakeRequest("Suhail");
    const HandshakeRequest* req = reinterpret_cast<const HandshakeRequest*>(
        buf.data() + sizeof(PacketHeader));
    EXPECT_STREQ(req->name, "Suhail");
    EXPECT_EQ(req->protocolVersion, ATLAS_PROTOCOL_VERSION);
}

TEST_F(HandshakeTest, ResponseAccepted) {
    auto buf = BuildHandshakeResponse(true, 5, "Welcome to AtlasMP!");
    const PacketHeader* hdr = reinterpret_cast<const PacketHeader*>(buf.data());
    EXPECT_EQ(hdr->type, PacketType::HANDSHAKE_RESPONSE);

    const HandshakeResponse* resp = reinterpret_cast<const HandshakeResponse*>(
        buf.data() + sizeof(PacketHeader));
    EXPECT_TRUE(resp->accepted);
    EXPECT_EQ(resp->playerId, 5);
    EXPECT_STREQ(resp->message, "Welcome to AtlasMP!");
}

TEST_F(HandshakeTest, ResponseRejected) {
    auto buf = BuildHandshakeResponse(false, INVALID_PLAYER, "Wrong password");
    const HandshakeResponse* resp = reinterpret_cast<const HandshakeResponse*>(
        buf.data() + sizeof(PacketHeader));
    EXPECT_FALSE(resp->accepted);
    EXPECT_STREQ(resp->message, "Wrong password");
}

TEST_F(HandshakeTest, NameTruncatedAt31Chars) {
    std::string longName(50, 'A'); // 50 A's
    auto buf = BuildHandshakeRequest(longName);
    const HandshakeRequest* req = reinterpret_cast<const HandshakeRequest*>(
        buf.data() + sizeof(PacketHeader));
    // Name field is 32 bytes, max 31 chars + null
    EXPECT_EQ(strlen(req->name), 31u);
}

TEST_F(HandshakeTest, EmptyPasswordAllowed) {
    auto buf = BuildHandshakeRequest("Player", "");
    const HandshakeRequest* req = reinterpret_cast<const HandshakeRequest*>(
        buf.data() + sizeof(PacketHeader));
    EXPECT_EQ(strlen(req->password), 0u);
}
