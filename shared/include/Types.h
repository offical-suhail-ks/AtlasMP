#pragma once
// shared/include/Types.h
// Common type aliases and constants used throughout AtlasMP

#include <cstdint>
#include <string>

namespace Atlas {

// ─── ID types ────────────────────────────────────────────────────────────────
using EntityId  = uint32_t;
using PlayerId  = uint16_t;
using VehicleId = uint32_t;
using ResourceId = uint32_t;
using NetHandle = uint32_t;

constexpr EntityId  INVALID_ENTITY  = 0xFFFFFFFF;
constexpr PlayerId  INVALID_PLAYER  = 0xFFFF;
constexpr VehicleId INVALID_VEHICLE = 0xFFFFFFFF;

// ─── Protocol ────────────────────────────────────────────────────────────────
constexpr uint16_t ATLAS_PROTOCOL_VERSION  = 1;
constexpr uint16_t ATLAS_DEFAULT_PORT      = 7788;
constexpr uint32_t ATLAS_MAGIC             = 0x41544C53; // 'ATLS'
constexpr uint32_t ATLAS_MAX_PLAYERS       = 1024;
constexpr uint32_t ATLAS_MAX_ENTITIES      = 4096;
constexpr uint32_t ATLAS_MAX_RESOURCES     = 256;
constexpr uint32_t ATLAS_MAX_PACKET_SIZE   = 65536;
constexpr float    ATLAS_SYNC_RATE_HZ      = 60.0f;

// ─── Entity types ─────────────────────────────────────────────────────────────
enum class EntityType : uint8_t {
    Player   = 0,
    Vehicle  = 1,
    Ped      = 2,
    Object   = 3,
    Blip     = 4,
    Pickup   = 5,
};

// ─── Connection states ────────────────────────────────────────────────────────
enum class ConnectionState : uint8_t {
    Disconnected  = 0,
    Connecting    = 1,
    Handshaking   = 2,
    Connected     = 3,
    Disconnecting = 4,
};

// ─── Kick / ban reasons ───────────────────────────────────────────────────────
enum class DisconnectReason : uint8_t {
    Unknown         = 0,
    Kicked          = 1,
    Banned          = 2,
    ServerShutdown  = 3,
    Timeout         = 4,
    ProtocolError   = 5,
    AntiCheat       = 6,
    DuplicateConnect = 7,
};

} // namespace Atlas
