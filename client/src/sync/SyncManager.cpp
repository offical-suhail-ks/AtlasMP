// client/src/sync/SyncManager.cpp
#include "SyncManager.h"
#include "../core/Logger.h"
#include "../hooks/NativeInvoker.h"
#include "../network/NetworkClient.h"
#include <algorithm>


namespace Atlas {

SyncManager::SyncManager(NativeInvoker *natives, NetworkClient *network)
    : m_natives(natives), m_network(network) {}

SyncManager::~SyncManager() = default;

void SyncManager::Initialize() {
  if (!m_network)
    return;

  // Register incoming player state handler
  m_network->OnPacket(
      Packets::PacketType::PLAYER_STATE,
      [this](Packets::PacketType, const uint8_t *data, size_t len) {
        if (len >= sizeof(Packets::PlayerStatePacket))
          OnPlayerStateReceived(
              *reinterpret_cast<const Packets::PlayerStatePacket *>(data));
      });

  m_network->OnPacket(
      Packets::PacketType::PLAYER_JOIN,
      [this](Packets::PacketType, const uint8_t *data, size_t len) {
        if (len >= sizeof(Packets::PlayerJoinPacket)) {
          const auto &pkt =
              *reinterpret_cast<const Packets::PlayerJoinPacket *>(data);
          OnPlayerJoin(pkt.playerId, pkt.name);
        }
      });

  m_network->OnPacket(
      Packets::PacketType::PLAYER_LEAVE,
      [this](Packets::PacketType, const uint8_t *data, size_t len) {
        if (len >= sizeof(Packets::PlayerLeavePacket)) {
          const auto &pkt =
              *reinterpret_cast<const Packets::PlayerLeavePacket *>(data);
          OnPlayerLeave(pkt.playerId);
        }
      });

  m_initialized = true;
  Logger::Info("[SyncManager] Initialized");
}

void SyncManager::Shutdown() {
  m_remotePlayers.clear();
  m_initialized = false;
}

void SyncManager::Tick(float deltaTime) {
  if (!m_initialized)
    return;

  // 1. Read and send local state at configured rate
  m_timeSinceLastSendMs += deltaTime * 1000.0f;
  if (m_timeSinceLastSendMs >= m_syncIntervalMs) {
    m_timeSinceLastSendMs = 0.0f;

    LocalPlayerSnapshot snap;
    if (m_natives) {
      LocalPlayer lp(m_natives);
      snap = lp.ReadSnapshot();
    }
    SendLocalState(snap);
  }

  // 2. Spawn pending remote players (retry each tick until model is loaded)
  //    then interpolate already-spawned ones.
  for (auto &[id, state] : m_remotePlayers) {
    if (!state.spawned)
      SpawnRemotePed(state); // keeps retrying until EnsureModelLoaded succeeds
    else
      InterpolateRemote(state, deltaTime);
  }
}

void SyncManager::OnPlayerJoin(uint16_t id, const std::string &name) {
  if (m_remotePlayers.count(id))
    return;
  RemotePlayerState state;
  state.id = id;
  state.name = name;
  state.spawned = false;
  m_remotePlayers[id] = state;
  Logger::Info("[SyncManager] Remote player joined: %s (%d)", name.c_str(), (int)id);
}

void SyncManager::OnPlayerLeave(uint16_t id) {
  auto it = m_remotePlayers.find(id);
  if (it == m_remotePlayers.end())
    return;
  // TODO: delete their ped via NativeInvoker
  m_remotePlayers.erase(it);
  Logger::Info("[SyncManager] Remote player left: %d", (int)id);
}

void SyncManager::OnPlayerStateReceived(const Packets::PlayerStatePacket &pkt) {
  auto it = m_remotePlayers.find(pkt.playerId);
  if (it == m_remotePlayers.end()) {
    // First time we've heard about this player — create and spawn a ped.
    RemotePlayerState fresh;
    fresh.id = pkt.playerId;
    fresh.position = pkt.position;
    fresh.prevPosition = pkt.position;
    fresh.rotation = pkt.rotation;
    fresh.prevRotation = pkt.rotation;
    fresh.heading = pkt.heading;
    fresh.health = pkt.health;
    fresh.lastUpdate = RemotePlayerState::Clock::now();
    m_remotePlayers[pkt.playerId] = fresh;
    Logger::Info("[SyncManager] New remote player %d — spawning ped", (int)pkt.playerId);
    SpawnRemotePed(m_remotePlayers[pkt.playerId]);
    return;
  }

  auto &state = it->second;
  state.prevPosition = state.position;
  state.prevRotation = state.rotation;
  state.position = pkt.position;
  state.velocity = pkt.velocity;
  state.rotation = pkt.rotation;
  state.heading = pkt.heading;
  state.health = pkt.health;
  state.armour = pkt.armour;
  state.flags = pkt.flags;
  state.alpha = 0.0f;
  state.lastUpdate = RemotePlayerState::Clock::now();
}

void SyncManager::SendLocalState(const LocalPlayerSnapshot &snap) {
  if (!m_network || !m_network->IsConnected())
    return;

  Packets::PlayerStatePacket pkt{};
  pkt.playerId = m_network->GetLocalId();
  pkt.position = snap.position;
  pkt.velocity = snap.velocity;
  pkt.rotation = snap.rotation;
  pkt.heading = snap.heading;
  pkt.health = (int16_t)snap.health;
  pkt.armour = (int16_t)snap.armour;
  pkt.flags = snap.flags;

  m_network->Send(Packets::PacketType::PLAYER_STATE, &pkt, sizeof(pkt),
                  false); // unreliable for position
}

// NOTE: InterpolateRemote(), SpawnRemotePed() and DeleteRemotePed() are now
// defined in RemotePed_1604.cpp (they call GTA V natives). Kept out of this
// file to avoid duplicate-symbol (LNK2005) errors.

const RemotePlayerState *SyncManager::GetRemotePlayer(uint16_t id) const {
  auto it = m_remotePlayers.find(id);
  return it != m_remotePlayers.end() ? &it->second : nullptr;
}

} // namespace Atlas