// server/src/core/Server.cpp
#include "Server.h"
#include "Logger.h"
#include "EventBus.h"
#include "AntiCheat.h"
#include "../network/NetworkServer.h"
#include "../resources/ResourceLoader.h"
#include "../sync/SyncManager.h"
#include "../scripting/ScriptHost.h"
#include "../database/Database.h"
#include "../../include/PlayerManager.h"
#include <thread>
#include <chrono>

namespace Atlas {

Server::Server(const Config& config) : m_config(config) {}

Server::~Server() { Stop(); }

bool Server::Start() {
    if (m_running) return true;

    Logger::Info("[Server] Initializing subsystems...");

    // Event bus (first — others publish to it)
    m_events   = std::make_unique<EventBus>();

    // Player manager
    m_players  = std::make_unique<PlayerManager>();

    // Network
    m_network  = std::make_unique<NetworkServer>(m_config, m_players.get());
    if (!m_network->Start()) {
        Logger::Error("[Server] Failed to start network!");
        return false;
    }

    // Sync manager
    SyncConfig syncCfg;
    syncCfg.tickRate         = m_config.GetSyncTickRate();
    syncCfg.interestDistance = 500.0f;
    m_sync = std::make_unique<SyncManager>(m_network.get(), m_players.get(), syncCfg);

    // Script host
    m_scripts = std::make_unique<ScriptHost>(m_config, m_events.get(), m_players.get(), m_network.get());
    m_scripts->Initialize();

    // Resource loader
    m_resources = std::make_unique<ResourceLoader>();
    m_resources->ScanDirectory(m_config.GetResourcesPath());

    // Autostart resources
    for (const auto& name : m_config.GetAutostart()) {
        Logger::Info("[Server] Starting resource: {}", name);
        if (!m_resources->StartResource(name))
            Logger::Warn("[Server] Failed to start resource: {}", name);
    }

    m_running = true;
    Logger::Info("[Server] Ready on {}:{}", m_config.GetHost(), m_config.GetPort());
    return true;
}

void Server::Stop() {
    if (!m_running) return;
    m_running = false;
    Logger::Info("[Server] Stopping...");

    if (m_network) m_network->Stop();
    if (m_tickThread.joinable()) m_tickThread.join();

    Logger::Info("[Server] All subsystems stopped.");
}

void Server::RunBlocking() {
    // Start tick loop on a background thread
    m_tickThread = std::thread([this]{ TickLoop(); });

    // Block main thread until stopped
    while (m_running)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (m_tickThread.joinable()) m_tickThread.join();
}

void Server::TickLoop() {
    using Clock    = std::chrono::steady_clock;
    using Duration = std::chrono::duration<double>;

    auto lastTick = Clock::now();

    while (m_running) {
        auto now   = Clock::now();
        float dt   = std::chrono::duration<float>(now - lastTick).count();
        lastTick   = now;

        Tick(dt);
        m_tickCount++;

        // Sleep remainder of tick interval
        auto elapsed = Clock::now() - now;
        if (elapsed < TICK_INTERVAL)
            std::this_thread::sleep_for(TICK_INTERVAL - elapsed);
    }
}

void Server::Tick(float deltaTime) {
    // 1. Poll network (process all queued packets)
    if (m_network) m_network->Poll();

    // 2. Sync broadcast
    if (m_sync) m_sync->Tick(deltaTime);

    // 3. Script timers / coroutines
    if (m_scripts) m_scripts->Tick(deltaTime);

    // 4. Publish server tick event
    if (m_events) m_events->Publish(ServerTickEvent{ deltaTime, m_tickCount });
}

} // namespace Atlas
