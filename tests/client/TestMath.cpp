// tests/client/TestMath.cpp
// Unit tests for shared math types

#include <gtest/gtest.h>
#include "../../shared/include/AtlasMath.h"

using namespace Atlas;

// ─── Vector3 ─────────────────────────────────────────────────────────────────

TEST(Vector3, DefaultConstructor) {
    Vector3 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST(Vector3, Addition) {
    Vector3 a(1, 2, 3);
    Vector3 b(4, 5, 6);
    Vector3 c = a + b;
    EXPECT_FLOAT_EQ(c.x, 5.0f);
    EXPECT_FLOAT_EQ(c.y, 7.0f);
    EXPECT_FLOAT_EQ(c.z, 9.0f);
}

TEST(Vector3, Length) {
    Vector3 v(3, 4, 0);
    EXPECT_FLOAT_EQ(v.length(), 5.0f);
}

TEST(Vector3, Distance) {
    Vector3 a(0, 0, 0);
    Vector3 b(3, 4, 0);
    EXPECT_FLOAT_EQ(a.distance(b), 5.0f);
}

TEST(Vector3, Normalize) {
    Vector3 v(3, 0, 0);
    Vector3 n = v.normalize();
    EXPECT_FLOAT_EQ(n.x, 1.0f);
    EXPECT_FLOAT_EQ(n.y, 0.0f);
    EXPECT_FLOAT_EQ(n.z, 0.0f);
    EXPECT_NEAR(n.length(), 1.0f, 1e-6f);
}

TEST(Vector3, Lerp) {
    Vector3 a(0, 0, 0);
    Vector3 b(10, 10, 10);
    Vector3 mid = Vector3::lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(mid.x, 5.0f);
    EXPECT_FLOAT_EQ(mid.y, 5.0f);
    EXPECT_FLOAT_EQ(mid.z, 5.0f);
}

TEST(Vector3, LerpAtZero) {
    Vector3 a(1, 2, 3);
    Vector3 b(7, 8, 9);
    EXPECT_EQ(Vector3::lerp(a, b, 0.0f), a);
}

TEST(Vector3, LerpAtOne) {
    Vector3 a(1, 2, 3);
    Vector3 b(7, 8, 9);
    EXPECT_EQ(Vector3::lerp(a, b, 1.0f), b);
}

TEST(Vector3, DotProduct) {
    Vector3 a(1, 0, 0);
    Vector3 b(0, 1, 0);
    EXPECT_FLOAT_EQ(a.dot(b), 0.0f);  // perpendicular

    Vector3 c(1, 0, 0);
    EXPECT_FLOAT_EQ(a.dot(c), 1.0f);  // parallel
}

TEST(Vector3, CrossProduct) {
    Vector3 x(1, 0, 0);
    Vector3 y(0, 1, 0);
    Vector3 z = x.cross(y);
    EXPECT_FLOAT_EQ(z.x, 0.0f);
    EXPECT_FLOAT_EQ(z.y, 0.0f);
    EXPECT_FLOAT_EQ(z.z, 1.0f);
}

// ─── Quaternion ───────────────────────────────────────────────────────────────

TEST(Quaternion, DefaultIsIdentity) {
    Quaternion q;
    EXPECT_FLOAT_EQ(q.x, 0.0f);
    EXPECT_FLOAT_EQ(q.y, 0.0f);
    EXPECT_FLOAT_EQ(q.z, 0.0f);
    EXPECT_FLOAT_EQ(q.w, 1.0f);
}

TEST(Quaternion, SlerpAtZero) {
    Quaternion a(0, 0, 0, 1);
    Quaternion b(0, 0, 0.707f, 0.707f); // 90 deg around Z
    Quaternion r = Quaternion::slerp(a, b, 0.0f);
    EXPECT_NEAR(r.x, a.x, 1e-4f);
    EXPECT_NEAR(r.w, a.w, 1e-4f);
}

TEST(Quaternion, SlerpAtOne) {
    Quaternion a(0, 0, 0, 1);
    Quaternion b(0, 0, 0.707f, 0.707f);
    Quaternion r = Quaternion::slerp(a, b, 1.0f);
    EXPECT_NEAR(r.z, b.z, 1e-4f);
    EXPECT_NEAR(r.w, b.w, 1e-4f);
}
