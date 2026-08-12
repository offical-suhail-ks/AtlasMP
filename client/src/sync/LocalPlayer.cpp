// client/src/sync/LocalPlayer.cpp
#include "LocalPlayer.h"
#include "../hooks/NativeInvoker.h"

namespace Atlas {

// Only VERIFIED-correct native hashes (proven working in-game).
namespace {
  constexpr uint64_t PLAYER_PED_ID      = 0xD80958FC74E988A6;
  constexpr uint64_t GET_ENTITY_COORDS  = 0x3FEF770D40960D5A;
  constexpr uint64_t GET_ENTITY_HEADING = 0xE83D4F9BA2A38914;
  constexpr uint64_t DOES_ENTITY_EXIST  = 0x7239B21A38F536BA;
}

LocalPlayer::LocalPlayer(NativeInvoker* natives) : m_natives(natives) {}

LocalPlayerSnapshot LocalPlayer::ReadSnapshot() {
    LocalPlayerSnapshot snap;
    if (!m_natives) return snap;

    int ped = m_natives->Call<int>(PLAYER_PED_ID);
    if (ped == 0 || !m_natives->Call<int>(DOES_ENTITY_EXIST, ped))
        return snap; // not in world yet — send zeroed snapshot

    // Position: Vector3 in the first 3 return slots (float in low 32 bits each).
    {
        NativeContext ctx;
        ctx.Push(ped);
        ctx.Push(1); // alive
        m_natives->Invoke(GET_ENTITY_COORDS, ctx);
        snap.position.x = *reinterpret_cast<float*>(&ctx.m_returnValue[0]);
        snap.position.y = *reinterpret_cast<float*>(&ctx.m_returnValue[1]);
        snap.position.z = *reinterpret_cast<float*>(&ctx.m_returnValue[2]);
    }

    snap.heading = m_natives->Call<float>(GET_ENTITY_HEADING, ped);

    // velocity/health omitted: their hashes in NativeInvoker.h are wrong for
    // this build and aren't needed to sync visible movement. Add later with
    // verified hashes if you want smoother interpolation.
    return snap;
}

} // namespace Atlas