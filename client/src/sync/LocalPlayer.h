#pragma once
// client/src/sync/LocalPlayer.h
#include "../../../shared/include/AtlasMath.h"
#include <cstdint>

namespace Atlas {
class NativeInvoker;

struct LocalPlayerSnapshot {
    Vector3    position;
    Vector3    velocity;
    Quaternion rotation;
    float      heading   = 0.0f;
    int        health    = 200;
    int        armour    = 0;
    uint32_t   weaponHash = 0;
    uint8_t    flags     = 0;
};

class LocalPlayer {
public:
    explicit LocalPlayer(NativeInvoker* natives);
    LocalPlayerSnapshot ReadSnapshot();

private:
    NativeInvoker* m_natives;
};

} // namespace Atlas
