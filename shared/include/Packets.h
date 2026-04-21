#pragma once
// shared/include/Packets.h
// All AtlasMP network packet type definitions
// Both client and server include this file

#include "Types.h"
#include "AtlasMath.h"
#include <cstdint>
#include <string>

namespace Atlas::Packets {

// ─── Packet type IDs ─────────────────────────────────────────────────────────
enum class PacketType : uint16_t {
    // Handshake
    HANDSHAKE_REQUEST   = 0x0001,
    HANDSHAKE_RESPONSE  = 0x0002,
    DISCONNECT          = 0x0003,
    PING                = 0x0004,
    PONG                = 0x0005,

    // Server info
    SERVER_INFO         = 0x0010,
    PLAYER_LIST         = 0x0011,

    // Player management
    PLAYER_JOIN         = 0x0020,
    PLAYER_LEAVE        = 0x0021,
    PLAYER_CHAT         = 0x0022,
    PLAYER_COMMAND      = 0x0023,
    PLAYER_SPAWN        = 0x0024,
    PLAYER_DEATH        = 0x0025,
    PLAYER_KICK         = 0x0026,

    // Sync packets (high frequency)
    PLAYER_STATE        = 0x0100,  // Position, rotation, velocity, health
    VEHICLE_STATE       = 0x0101,  // Vehicle position, rotation, velocity
    PED_STATE           = 0x0102,  // Synced NPC state
    OBJECT_STATE        = 0x0103,  // Synced prop/object state

    // Entity management
    ENTITY_CREATE       = 0x0200,
    ENTITY_DESTROY      = 0x0201,
    ENTITY_OWNERSHIP    = 0x0202,  // Transfer ownership of entity

    // Scripting
    SCRIPT_EVENT        = 0x0300,  // Trigger a script event
    SCRIPT_NATIVE_CALL  = 0x0301,  // Server triggers native on client
    NUI_MESSAGE         = 0x0302,  // Message to/from NUI browser

    // Resources
    RESOURCE_START      = 0x0400,
    RESOURCE_STOP       = 0x0401,
    RESOURCE_FILE       = 0x0402,  // File streaming chunk

    // Anti-cheat
    AC_VIOLATION        = 0x0500,

    INVALID             = 0xFFFF,
};

// ─── Packet header (all packets start with this) ─────────────────────────────
#pragma pack(push, 1)

struct PacketHeader {
    uint32_t    magic;      // ATLAS_MAGIC (0x41544C53)
    PacketType  type;       // Packet type
    uint16_t    size;       // Payload size in bytes (excluding header)
    uint32_t    sequence;   // Packet sequence number
    uint16_t    senderId;   // Sender's player ID (0 = server)
};

// ─── Handshake ───────────────────────────────────────────────────────────────

struct HandshakeRequest {
    uint16_t    protocolVersion;    // ATLAS_PROTOCOL_VERSION
    uint16_t    gameVersion;        // GTA V build number
    char        name[32];           // Player display name
    char        password[64];       // Server password (empty if none)
    uint8_t     clientVersion[3];   // AtlasMP client version [major,minor,patch]
};

struct HandshakeResponse {
    bool        accepted;           // true = connected, false = rejected
    PlayerId    playerId;           // Assigned player ID
    uint8_t     reason;             // DisconnectReason if rejected
    char        message[128];       // Rejection message or welcome message
    uint32_t    serverFlags;        // Feature flags enabled on this server
};

// ─── Server Info ─────────────────────────────────────────────────────────────

struct ServerInfoPacket {
    char        name[64];
    char        description[256];
    uint16_t    playerCount;
    uint16_t    maxPlayers;
    uint32_t    serverFlags;
};

// ─── Player state (synced every tick) ────────────────────────────────────────

struct PlayerStatePacket {
    PlayerId    playerId;
    Vector3     position;
    Vector3     velocity;
    Quaternion  rotation;
    float       heading;        // Yaw in degrees
    int16_t     health;         // 0-200
    int16_t     armour;         // 0-100
    uint32_t    animHash;       // Current animation hash
    float       animPhase;      // Animation progress 0.0-1.0
    uint8_t     flags;          // On ground, in vehicle, aiming, etc.
    uint8_t     weaponHash[4];  // Current weapon (truncated hash)
};

// ─── Vehicle state ────────────────────────────────────────────────────────────

struct VehicleStatePacket {
    VehicleId   vehicleId;
    PlayerId    driverPlayerId;     // Who is driving
    Vector3     position;
    Vector3     velocity;
    Quaternion  rotation;
    float       engineHealth;
    float       bodyHealth;
    uint8_t     gear;
    float       rpm;
    float       throttle;
    float       brake;
    float       steer;
    uint8_t     lights;             // Headlights, indicators flags
    uint8_t     doors;              // Door open/close bitfield
    uint32_t    modelHash;
};

// ─── Player join/leave ────────────────────────────────────────────────────────

struct PlayerJoinPacket {
    PlayerId    playerId;
    char        name[32];
};

struct PlayerLeavePacket {
    PlayerId    playerId;
    uint8_t     reason;     // DisconnectReason
};

// ─── Chat ─────────────────────────────────────────────────────────────────────

struct PlayerChatPacket {
    PlayerId    senderId;
    char        message[256];
};

// ─── Script event (Lua/JS/C# server ↔ client) ────────────────────────────────

struct ScriptEventPacket {
    char        eventName[64];
    uint16_t    dataSize;
    // Followed by `dataSize` bytes of JSON-encoded arguments
};

// ─── Entity create/destroy ────────────────────────────────────────────────────

struct EntityCreatePacket {
    EntityId    entityId;
    uint8_t     entityType;     // EntityType enum
    uint32_t    modelHash;
    Vector3     position;
    Quaternion  rotation;
    PlayerId    ownerId;
};

struct EntityDestroyPacket {
    EntityId    entityId;
};

// ─── Disconnect ──────────────────────────────────────────────────────────────

struct DisconnectPacket {
    uint8_t     reason;
    char        message[128];
};

#pragma pack(pop)

} // namespace Atlas::Packets
