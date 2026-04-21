#pragma once
// server/src/core/Server.h
// Main server class — owns all subsystems

#include "Config.h"
#include "../../include/PlayerManager.h"

#include <memory>
#include <atomic>
#include <thread>
#include <chrono>

namespace Atlas {

// Forward declarations
class NetworkServer;
class ResourceLoader;
class SyncManager;
class ScriptHost;
class EventBus;
class AntiCheat;
class Database;

class Server {
public:
    explicit Server(const Config& config);
    ~Server();

    // Lifecycle
    bool Start();
    void Stop();
    void RunBlocking();     // Blocks until stopped

    // Accessors for subsystems
    NetworkServer*  GetNetwork()   const { return m_network.get(); }
    ResourceLoader* GetResources() const { return m_resources.get(); }
    SyncManager*    GetSync()      const { return m_sync.get(); }
    ScriptHost*     GetScripts()   const { return m_scripts.get(); }
    EventBus*       GetEvents()    const { return m_events.get(); }
    PlayerManager*  GetPlayers()   const { return m_players.get(); }
    const Config&   GetConfig()    const { return m_config; }

    bool IsRunning() const { return m_running.load(); }

    // Server tick rate (target 60 Hz)
    static constexpr int TARGET_TPS = 60;
    static constexpr auto TICK_INTERVAL = std::chrono::microseconds(1000000 / TARGET_TPS);

private:
    void TickLoop();
    void Tick(float deltaTime);

    const Config& m_config;
    std::atomic<bool> m_running{false};

    std::unique_ptr<EventBus>       m_events;
    std::unique_ptr<NetworkServer>  m_network;
    std::unique_ptr<PlayerManager>  m_players;
    std::unique_ptr<ResourceLoader> m_resources;
    std::unique_ptr<SyncManager>    m_sync;
    std::unique_ptr<ScriptHost>     m_scripts;
    std::unique_ptr<AntiCheat>      m_anticheat;
    std::unique_ptr<Database>       m_database;

    std::thread m_tickThread;
    uint64_t    m_tickCount = 0;
};

} // namespace Atlas
