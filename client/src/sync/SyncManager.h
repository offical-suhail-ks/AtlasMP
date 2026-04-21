#pragma once
// client/src/sync/SyncManager.h
// Reads local game state and sends to server; receives remote state and applies it.

#include "../../../shared/include/Packets.h"
#include "LocalPlayer.h"
#include "RemotePlayer.h"
#include <unordered_map>

namespace Atlas {

class NativeInvoker;
class NetworkClient;

// ─── SyncManager ─────────────────────────────────────────────────────────────
class SyncManager {
public:
    SyncManager(NativeInvoker* natives, NetworkClient* network);
    ~SyncManager();

    void Initialize();
    void Shutdown();

    /// Called every game tick — reads local state + updates remote players
    void Tick(float deltaTime);

    // ── Remote player management ───────────────────────────────────────────
    void OnPlayerJoin(uint16_t playerId, const std::string& name);
    void OnPlayerLeave(uint16_t playerId);
    void OnPlayerStateReceived(const Packets::PlayerStatePacket& pkt);
    void OnVehicleStateReceived(const Packets::VehicleStatePacket& pkt);

    const RemotePlayerState* GetRemotePlayer(uint16_t id) const;

    // Sync rate control
    void SetSyncRate(float hz) { m_syncIntervalMs = 1000.0f / hz; }

private:
    // Reading local game state
    LocalPlayerSnapshot ReadLocalState();
    void SendLocalState(const LocalPlayerSnapshot& snap);

    // Applying remote state to NPC peds
    void SpawnRemotePed(RemotePlayerState& state);
    void ApplyRemoteState(RemotePlayerState& state, float deltaTime);
    void InterpolateRemote(RemotePlayerState& state, float deltaTime);
    void DeleteRemotePed(RemotePlayerState& state);

    NativeInvoker* m_natives;
    NetworkClient* m_network;

    std::unordered_map<uint16_t, RemotePlayerState> m_remotePlayers;

    float m_syncIntervalMs     = 1000.0f / 60.0f; // 60hz default
    float m_timeSinceLastSendMs = 0.0f;

    LocalPlayerSnapshot m_lastSnapshot;
    bool m_initialized = false;
};

} // namespace Atlas
