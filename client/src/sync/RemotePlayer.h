#pragma once
// client/src/sync/RemotePlayer.h
#include "../../../shared/include/AtlasMath.h"
#include "../../../shared/include/Types.h"
#include <string>
#include <chrono>

namespace Atlas {
class NativeInvoker;

struct RemotePlayerState {
    PlayerId   id       = INVALID_PLAYER;
    std::string name;
    int        ped      = 0;      // GTA V ped handle we spawned
    bool       spawned  = false;

    Vector3    position;
    Vector3    velocity;
    Quaternion rotation;
    float      heading  = 0.0f;
    int        health   = 200;
    int        armour   = 0;
    uint8_t    flags    = 0;

    // Interpolation
    Vector3    prevPosition;
    Quaternion prevRotation;
    float      alpha    = 0.0f;

    using Clock = std::chrono::steady_clock;
    Clock::time_point lastUpdate;
};

class RemotePlayer {
public:
    RemotePlayer(NativeInvoker* natives, PlayerId id, const std::string& name);
    ~RemotePlayer();

    void ApplyState(const RemotePlayerState& state);
    void Interpolate(float deltaTime);
    void Spawn();
    void Destroy();

    const RemotePlayerState& GetState() const { return m_state; }

private:
    NativeInvoker*    m_natives;
    RemotePlayerState m_state;
};

} // namespace Atlas
