// client/src/sync/LocalPlayer.cpp
#include "LocalPlayer.h"
#include "../hooks/NativeInvoker.h"

namespace Atlas {

LocalPlayer::LocalPlayer(NativeInvoker* natives) : m_natives(natives) {}

LocalPlayerSnapshot LocalPlayer::ReadSnapshot() {
    LocalPlayerSnapshot snap;
    if (!m_natives) return snap;

    // TODO: call GTA V natives to read actual game state
    // int ped = m_natives->Call<int>(Natives::GET_PLAYER_PED, 0);
    // snap.position = m_natives->Call<Vector3>(Natives::GET_ENTITY_COORDS, ped, true);
    // snap.velocity = m_natives->Call<Vector3>(Natives::GET_ENTITY_VELOCITY, ped);
    // snap.health   = m_natives->Call<int>(Natives::GET_ENTITY_HEALTH, ped);
    // snap.heading  = m_natives->Call<float>(Natives::GET_ENTITY_HEADING, ped);

    return snap;
}

} // namespace Atlas
