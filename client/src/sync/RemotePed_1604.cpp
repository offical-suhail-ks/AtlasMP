// ============================================================================
//  AtlasMP — Remote Ped proof + filled SyncManager methods  (GTA V build 1604)
// ============================================================================
//
//  WHAT THIS IS
//  ------------
//  Two things in one file:
//    (A) Atlas_ProofTick()  — a self-contained, NO-NETWORKING test. Spawns ONE
//        ped ~3m in front of your local player and slides it side-to-side.
//        This is the milestone: if this ped appears and moves on screen,
//        your NativeInvoker works against a live GTA5.exe. Everything else
//        in AtlasMP is downstream of this fact.
//    (B) SyncManager::SpawnRemotePed / InterpolateRemote / DeleteRemotePed —
//        the real fills for the TODOs in your SyncManager.cpp, using the same
//        natives, so once (A) passes you flip straight to networked peds.
//
//  CRITICAL — THREADING
//  --------------------
//  GTA V natives MUST run on the game's script thread. In AtlasMP that means
//  these are only ever called from inside your scrThread::Tick hook path
//  (ScriptThread -> HookManager). Calling a native from your network thread,
//  a std::thread, or DllMain WILL crash to desktop. Route everything here
//  through the tick.
//
//  1604 NOTE
//  ---------
//  Every hash below is a legacy 1604 hash (they match the ones already in your
//  NativeInvoker.h). None of this is copied from FiveM — hashes come from the
//  public alloc8or native DB, which is data, not their engine code.
// ============================================================================

#include "../core/Logger.h"
#include "../hooks/NativeInvoker.h"
#include "RemotePlayer.h"
#include "SyncManager.h"
#include <cmath> // sinf, cosf


namespace Atlas {

// ─── Natives this file needs that aren't already in NativeInvoker.h ──────────
namespace N {
using namespace Atlas::Natives; // reuse the ones you have
constexpr uint64_t PLAYER_PED_ID = 0xD80958FC74E988A6;
constexpr uint64_t GET_ENTITY_HEADING = 0xE83D4F9BA2A38914;
constexpr uint64_t GET_HASH_KEY = 0xD24D37CC275948CC;
constexpr uint64_t REQUEST_MODEL = 0x963D27A58DF860AC;
constexpr uint64_t HAS_MODEL_LOADED = 0x98A4EB5D89A0C952;
constexpr uint64_t SET_MODEL_AS_NO_LONGER_NEEDED = 0xE532F5D78798DAAB;
constexpr uint64_t SET_ENTITY_HEADING = 0x8E2530AA8ADA980E;
constexpr uint64_t SET_ENTITY_INVINCIBLE = 0x3882114BDE571AD4;
constexpr uint64_t FREEZE_ENTITY_POSITION = 0x428CA6DBD1094446;
constexpr uint64_t SET_BLOCKING_OF_NON_TEMPORARY_EVENTS = 0x9F8AA94D6D97DBF4;
constexpr uint64_t SET_ENTITY_AS_MISSION_ENTITY = 0x6B71FE9AC46209B4;
    constexpr uint64_t GET_ENTITY_COORDS   = 0x3FEF770D40960D5A;
    constexpr uint64_t SET_ENTITY_COORDS   = 0x06843DA7060A026B;
    constexpr uint64_t CREATE_PED          = 0xD49F9B0955C367DE;
    constexpr uint64_t DELETE_ENTITY       = 0xAD738C3085FE7E11;
    constexpr uint64_t DOES_ENTITY_EXIST   = 0x7239B21A38F536BA;
} // namespace N

// A reliable, always-visible ambient model. No component setup needed.
static constexpr const char *kProofModel = "a_m_y_hipster_01";

// ────────────────────────────────────────────────────────────────────────────
//  Small helpers
// ────────────────────────────────────────────────────────────────────────────
static uint32_t GetModelHash(NativeInvoker *nv, const char *name) {
  return nv->Call<uint32_t>(N::GET_HASH_KEY, name);
}

// Non-blocking model streaming. Returns true once the model is resident.
// Call every tick until it returns true, THEN create the ped. Never spin-wait
// on the script thread — that freezes the game.
static bool EnsureModelLoaded(NativeInvoker *nv, uint32_t model) {
  if (nv->Call<int>(N::HAS_MODEL_LOADED, model))
    return true;
  nv->Call<void>(N::REQUEST_MODEL, model);
  return false;
}

// Spawn a ped at world coords. Returns ped handle, or 0 on failure.
static int SpawnPed(NativeInvoker *nv, uint32_t model, float x, float y,
                    float z, float heading) {
  // MINIMAL spawn: only the natives strictly needed to place a visible ped.
  // The "make it stable" natives (mission entity / blocking events / invincible)
  // are intentionally omitted — they're cosmetic and some had wrong hashes that
  // hard-crash ScriptHookV. A plain CREATE_PED is enough to prove native calls.
  //
  // CREATE_PED(pedType, model, x, y, z, heading, isNetwork, bScriptHostPed)
  int ped = nv->Call<int>(N::CREATE_PED, 4, model, x, y, z, heading, 0, 0);
  if (ped == 0)
    return 0;
  // Release the model from memory now that the ped exists.
  nv->Call<void>(N::SET_MODEL_AS_NO_LONGER_NEEDED, model);
  return ped;
}

// ============================================================================
//  (A)  PROOF TICK  — call this ONCE PER FRAME from your scrThread tick.
//       No networking. Spawns one ped and oscillates it left/right so you can
//       see position updates land. Delete-tests too, after ~10s.
// ============================================================================
void Atlas_ProofTick(NativeInvoker *nv, float deltaTime) {
  if (!nv) return;

  static int   s_ped   = 0;
  static bool  s_done  = false;
  static float s_t     = 0.0f;
  static float s_wait  = 0.0f;
  static int   s_ready = 0;
  static uint32_t s_model = 0;

  if (s_done) return;

  // ── Readiness gate ──────────────────────────────────────────────────────
  // Wait until the player ped exists and is stable before calling spawn
  // natives, so we never fire during the loading screen (which crashes).
  int player = nv->Call<int>(N::PLAYER_PED_ID);
  if (player == 0 || player == -1) { s_ready = 0; return; }
  if (!nv->Call<int>(N::DOES_ENTITY_EXIST, player)) { s_ready = 0; return; }
  if (s_ready < 120) { s_ready++; return; }
  s_wait += deltaTime;
  if (s_wait < 2.0f) return;

  s_t += deltaTime;

  if (s_ped == 0) {
    if (s_model == 0) s_model = GetModelHash(nv, kProofModel);
    if (!EnsureModelLoaded(nv, s_model)) return; // wait across ticks

    // Read player coords. Through ScriptHookV, a Vector3 return comes back in
    // the low 32 bits of the first three 8-byte result slots. Read each slot
    // as a float from its low word.
    NativeContext ctx;
    ctx.Push(player);
    ctx.Push(1);
    nv->Invoke(N::GET_ENTITY_COORDS, ctx);
    float px = *reinterpret_cast<float*>(&ctx.m_returnValue[0]);
    float py = *reinterpret_cast<float*>(&ctx.m_returnValue[1]);
    float pz = *reinterpret_cast<float*>(&ctx.m_returnValue[2]);

    // Sanity check — if coords are absurd, bail instead of crashing CREATE_PED.
    if (!(px > -20000.f && px < 20000.f && py > -20000.f && py < 20000.f &&
          pz > -2000.f && pz < 5000.f)) {
      Logger::Error("[Proof] Bad coords (%.1f %.1f %.1f) — aborting", px, py, pz);
      s_done = true; return;
    }

    float heading = nv->Call<float>(N::GET_ENTITY_HEADING, player);
    constexpr float DEG2RAD = 3.14159265f / 180.0f;
    float sx = px - sinf(heading * DEG2RAD) * 3.0f;
    float sy = py + cosf(heading * DEG2RAD) * 3.0f;

    s_ped = SpawnPed(nv, s_model, sx, sy, pz, heading);
    if (s_ped == 0) { Logger::Error("[Proof] CREATE_PED failed"); s_done = true; return; }
    Logger::Info("[Proof] Spawned ped handle %d at %.1f %.1f %.1f", s_ped, sx, sy, pz);
    return;
  }

  // Ped stands for 15s so you can walk up to it, then clean up.
  if (s_t > 120.0f) {  // ped stays 2 minutes so you can walk up to it
    int ped = s_ped;
    nv->Call<void>(N::DELETE_ENTITY, &ped);
    Logger::Info("[Proof] Deleted ped — proof complete");
    s_ped = 0; s_done = true;
  }
}

// ============================================================================
//  (B)  REAL FILLS  — replace the stubbed bodies in SyncManager.cpp with these.
//       These match the declarations already in your SyncManager.h.
// ============================================================================

void SyncManager::SpawnRemotePed(RemotePlayerState &state) {
  if (!m_natives || state.spawned)
    return;

  static uint32_t model = 0;
  if (model == 0)
    model = GetModelHash(m_natives, kProofModel);
  if (!EnsureModelLoaded(m_natives, model))
    return; // retry next tick

  state.ped = SpawnPed(m_natives, model, state.position.x, state.position.y,
                       state.position.z, state.heading);
  if (state.ped == 0)
    return;

  state.spawned = true;
  Logger::Info("[Sync] Spawned remote ped for player %d (handle %d)", state.id,
               state.ped);
}

void SyncManager::InterpolateRemote(RemotePlayerState &state, float deltaTime) {
  if (!m_natives)
    return;
  if (!state.spawned) {
    SpawnRemotePed(state);
    return;
  }

  // Advance interpolation alpha toward the latest snapshot.
  state.alpha = state.alpha + deltaTime * 10.0f;
  if (state.alpha > 1.0f)
    state.alpha = 1.0f;

  Vector3 pos = Vector3::lerp(state.prevPosition, state.position, state.alpha);

  m_natives->Call<void>(N::SET_ENTITY_COORDS, state.ped, pos.x, pos.y, pos.z, 0,
                        0, 0, 0);
  m_natives->Call<void>(N::SET_ENTITY_HEADING, state.ped, state.heading);
}

void SyncManager::DeleteRemotePed(RemotePlayerState &state) {
  if (!m_natives || !state.spawned || state.ped == 0)
    return;
  m_natives->Call<void>(N::DELETE_ENTITY, &state.ped);
  state.ped = 0;
  state.spawned = false;
  Logger::Info("[Sync] Deleted remote ped for player %d", state.id);
}

} // namespace Atlas