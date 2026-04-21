#pragma once
// shared/include/AtlasMath.h
// stdlib.h and math.h MUST come before anything else on MSVC 19.50
#include <stdlib.h>
#include <math.h>
#include <string>

#ifndef _USE_MATH_DEFINES
#  define _USE_MATH_DEFINES
#endif

namespace Atlas {

struct Vector2 {
    float x = 0.0f, y = 0.0f;
    Vector2() = default;
    Vector2(float x, float y) : x(x), y(y) {}
    Vector2 operator+(const Vector2& o) const { return {x+o.x, y+o.y}; }
    Vector2 operator-(const Vector2& o) const { return {x-o.x, y-o.y}; }
    Vector2 operator*(float s)          const { return {x*s,   y*s};   }
    float   length()                    const { return sqrtf(x*x + y*y); }
    float   distance(const Vector2& o)  const { return (*this - o).length(); }
};

struct Vector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vector3() = default;
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3 operator+(const Vector3& o)  const { return {x+o.x, y+o.y, z+o.z}; }
    Vector3 operator-(const Vector3& o)  const { return {x-o.x, y-o.y, z-o.z}; }
    Vector3 operator*(float s)           const { return {x*s,   y*s,   z*s};   }
    bool    operator==(const Vector3& o) const { return x==o.x && y==o.y && z==o.z; }
    float   length()                     const { return sqrtf(x*x + y*y + z*z); }
    float   distance(const Vector3& o)   const { return (*this - o).length(); }
    Vector3 normalize() const {
        float len = length();
        if (len < 1e-6f) return {0,0,0};
        return {x/len, y/len, z/len};
    }
    float   dot  (const Vector3& o) const { return x*o.x + y*o.y + z*o.z; }
    Vector3 cross(const Vector3& o) const {
        return {y*o.z-z*o.y, z*o.x-x*o.z, x*o.y-y*o.x};
    }
    std::string toString() const {
        return "("+std::to_string(x)+", "+std::to_string(y)+", "+std::to_string(z)+")";
    }
    static Vector3 lerp(const Vector3& a, const Vector3& b, float t) {
        return a + (b-a)*t;
    }
};

struct Vector4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    Vector4() = default;
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

struct Quaternion {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
    Quaternion() = default;
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) {
        float dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
        if (dot < 0.0f) { Quaternion nb={-b.x,-b.y,-b.z,-b.w}; return slerp(a,nb,t); }
        if (dot > 0.9995f)
            return {a.x+t*(b.x-a.x), a.y+t*(b.y-a.y), a.z+t*(b.z-a.z), a.w+t*(b.w-a.w)};
        float th0 = acosf(dot), th = th0*t, s0f = sinf(th0);
        float s0 = cosf(th) - dot*sinf(th)/s0f;
        float s1 = sinf(th)/s0f;
        return {s0*a.x+s1*b.x, s0*a.y+s1*b.y, s0*a.z+s1*b.z, s0*a.w+s1*b.w};
    }
};

inline float headingToRad(float h) { return h * 0.01745329251994329576f; }

} // namespace Atlas
