// client/src/sync/RemotePlayer.cpp
#include "RemotePlayer.h"
#include "../hooks/NativeInvoker.h"
#include "../core/Logger.h"

namespace Atlas {

RemotePlayer::RemotePlayer(NativeInvoker* natives,
                            PlayerId id, const std::string& name)
    : m_natives(natives)
{
    m_state.id   = id;
    m_state.name = name;
}

RemotePlayer::~RemotePlayer() {
    Destroy();
}

void RemotePlayer::Spawn() {
    if (m_state.spawned || !m_natives) return;
    // TODO: natives CREATE_PED with model hash, position
    // int ped = m_natives->Call<int>(Natives::CREATE_PED, 26,
    //     0xA8683715, m_state.position.x, m_state.position.y, m_state.position.z,
    //     m_state.heading, false, false);
    // m_state.ped     = ped;
    m_state.spawned = true;
    Logger::Info("[RemotePlayer] Spawned ped for player {}", (int)m_state.id);
}

void RemotePlayer::Destroy() {
    if (!m_state.spawned || !m_natives || !m_state.ped) return;
    // TODO: natives DELETE_ENTITY(ped)
    // int ped = m_state.ped;
    // m_natives->Call<void>(Natives::DELETE_ENTITY, &ped);
    m_state.ped     = 0;
    m_state.spawned = false;
}

void RemotePlayer::ApplyState(const RemotePlayerState& state) {
    m_state.prevPosition = m_state.position;
    m_state.prevRotation = m_state.rotation;
    m_state.position     = state.position;
    m_state.rotation     = state.rotation;
    m_state.velocity     = state.velocity;
    m_state.heading      = state.heading;
    m_state.health       = state.health;
    m_state.armour       = state.armour;
    m_state.flags        = state.flags;
    m_state.alpha        = 0.0f;
    m_state.lastUpdate   = RemotePlayerState::Clock::now();

    if (!m_state.spawned) Spawn();
}

void RemotePlayer::Interpolate(float deltaTime) {
    if (!m_state.spawned || !m_state.ped || !m_natives) return;

    // Advance interpolation alpha (target: ~100ms ahead)
    m_state.alpha = std::min(1.0f, m_state.alpha + deltaTime * 10.0f);

    Vector3 pos = Vector3::lerp(m_state.prevPosition,
                                 m_state.position, m_state.alpha);
    Quaternion rot = Quaternion::slerp(m_state.prevRotation,
                                        m_state.rotation, m_state.alpha);

    // TODO: apply to GTA V ped
    // m_natives->Call<void>(Natives::SET_ENTITY_COORDS,
    //     m_state.ped, pos.x, pos.y, pos.z, false, false, false, false);
    // m_natives->Call<void>(Natives::SET_ENTITY_HEADING,
    //     m_state.ped, m_state.heading);
    (void)pos; (void)rot;
}

} // namespace Atlas
