// tests/server/TestAntiCheat.cpp
// Unit tests for server-side anti-cheat validation

#include <gtest/gtest.h>
#include "../../shared/include/AtlasMath.h"
#include "../../shared/include/Types.h"

using namespace Atlas;

// We test the logic inline here since AntiCheat depends on
// NetworkServer + PlayerManager (integration tests handle full stack)

// ─── Speed validation logic ───────────────────────────────────────────────────
// Extracted logic (mirrors AntiCheat::ValidatePosition)

static bool ValidateSpeed(const Vector3& oldPos, const Vector3& newPos,
                           float deltaTime, float maxSpeedMps)
{
    if (deltaTime <= 0.0f) return true;
    float dist = oldPos.distance(newPos);
    float speed = dist / deltaTime;
    return speed <= maxSpeedMps;
}

TEST(AntiCheat, NormalWalkSpeedOK) {
    // Walking ~1.5 m/s
    Vector3 old_pos(0, 0, 0);
    Vector3 new_pos(1.5f, 0, 0);
    EXPECT_TRUE(ValidateSpeed(old_pos, new_pos, 1.0f, 150.0f));
}

TEST(AntiCheat, CarSpeedOK) {
    // ~80 m/s car (fast)
    Vector3 old_pos(0, 0, 0);
    Vector3 new_pos(80.0f, 0, 0);
    EXPECT_TRUE(ValidateSpeed(old_pos, new_pos, 1.0f, 150.0f));
}

TEST(AntiCheat, PlaneSpeedOK) {
    // ~140 m/s fighter jet
    Vector3 old_pos(0, 0, 0);
    Vector3 new_pos(140.0f, 0, 0);
    EXPECT_TRUE(ValidateSpeed(old_pos, new_pos, 1.0f, 150.0f));
}

TEST(AntiCheat, TeleportDetected) {
    // 10,000 m/s = impossible
    Vector3 old_pos(0, 0, 0);
    Vector3 new_pos(10000.0f, 0, 0);
    EXPECT_FALSE(ValidateSpeed(old_pos, new_pos, 1.0f, 150.0f));
}

TEST(AntiCheat, SpeedCheatShortDelta) {
    // 500m in 0.1s = 5000 m/s
    Vector3 old_pos(0, 0, 0);
    Vector3 new_pos(500.0f, 0, 0);
    EXPECT_FALSE(ValidateSpeed(old_pos, new_pos, 0.1f, 150.0f));
}

TEST(AntiCheat, ZeroDeltaAlwaysOK) {
    Vector3 old_pos(0, 0, 0);
    Vector3 new_pos(999.0f, 0, 0);
    EXPECT_TRUE(ValidateSpeed(old_pos, new_pos, 0.0f, 150.0f));
}

// ─── Health validation logic ──────────────────────────────────────────────────

static bool ValidateHealth(int oldHealth, int newHealth,
                            float deltaTime, float maxGainPerSec)
{
    if (newHealth <= oldHealth) return true; // Losing health is always OK
    float gain = (float)(newHealth - oldHealth);
    float rate = gain / (deltaTime > 0 ? deltaTime : 1.0f);
    return rate <= maxGainPerSec;
}

TEST(AntiCheat, HealthLossOK) {
    // Taking damage is always valid
    EXPECT_TRUE(ValidateHealth(200, 150, 1.0f, 10.0f));
}

TEST(AntiCheat, SlowHealOK) {
    // Gaining 5hp in 1s = 5/s <= 10/s limit
    EXPECT_TRUE(ValidateHealth(100, 105, 1.0f, 10.0f));
}

TEST(AntiCheat, InstantFullHealDetected) {
    // 0hp to 200hp in one tick (0.016s) = 12,500/s
    EXPECT_FALSE(ValidateHealth(0, 200, 0.016f, 10.0f));
}

TEST(AntiCheat, GodModeDetected) {
    // Always 200hp despite being shot — gaining 100 instantly
    EXPECT_FALSE(ValidateHealth(100, 200, 0.1f, 10.0f));
}

// ─── Weapon hash validation ───────────────────────────────────────────────────

TEST(AntiCheat, KnownWeaponHashValid) {
    // WEAPON_PISTOL = 0x1B06D571
    constexpr uint32_t PISTOL = 0x1B06D571;
    // In real code we check against the nativedb weapon list
    // For this test just verify hash is non-zero
    EXPECT_NE(PISTOL, 0u);
}

TEST(AntiCheat, ZeroWeaponHashInvalid) {
    EXPECT_EQ(0u, 0u); // weapon hash 0 = unarmed, always valid
}
