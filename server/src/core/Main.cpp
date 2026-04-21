// server/src/core/Main.cpp
// AtlasMP Dedicated Server — Entry Point

#include "Server.h"
#include "Config.h"
#include "Logger.h"

#include <iostream>
#include <csignal>
#include <atomic>

// ─── Global server instance ───────────────────────────────────────────────────
static Atlas::Server* g_server = nullptr;
static std::atomic<bool> g_running{true};

// ─── Signal handler ───────────────────────────────────────────────────────────
void OnSignal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        Atlas::Logger::Info("[Main] Shutdown signal received ({}). Stopping...", sig);
        g_running = false;
        if (g_server) {
            g_server->Stop();
        }
    }
}

// ─── Banner ───────────────────────────────────────────────────────────────────
void PrintBanner() {
    std::cout << R"(
  ___  _   _           __  __ ____
 / _ \| |_| | __ _ ___|  \/  |  _ \
| | | | __| |/ _` / __| |\/| | |_) |
| |_| | |_| | (_| \__ \ |  | |  __/
 \___/ \__|_|\__,_|___/_|  |_|_|
)" << "\n";
    std::cout << "  AtlasMP Dedicated Server v" ATLAS_VERSION_STRING "\n";
    std::cout << "  GTA V Multiplayer Platform\n\n";
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    PrintBanner();

    // Register signal handlers
    std::signal(SIGINT,  OnSignal);
    std::signal(SIGTERM, OnSignal);

    // Parse config path from args (default: server.toml)
    std::string configPath = "server.toml";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            configPath = argv[++i];
        }
    }

    // Initialize logger
    Atlas::Logger::Init("logs/server.log");
    Atlas::Logger::Info("[Main] AtlasMP Server starting...");
    Atlas::Logger::Info("[Main] Config: {}", configPath);

    // Load configuration
    Atlas::Config config;
    if (!config.Load(configPath)) {
        Atlas::Logger::Error("[Main] Failed to load config: {}", configPath);
        Atlas::Logger::Error("[Main] Creating default config...");
        if (!config.SaveDefault(configPath)) {
            Atlas::Logger::Error("[Main] Failed to write default config: {}", configPath);
            return 1;
        }
        Atlas::Logger::Info("[Main] Default config written to: {}", configPath);
        Atlas::Logger::Info("[Main] Please edit {} and restart.", configPath);
        return 1;
    }

    // Create and start server
    Atlas::Server server(config);
    g_server = &server;

    if (!server.Start()) {
        Atlas::Logger::Error("[Main] Server failed to start!");
        return 1;
    }

    Atlas::Logger::Info("[Main] Server running on {}:{}",
        config.GetHost(), config.GetPort());
    Atlas::Logger::Info("[Main] Press Ctrl+C to stop.");

    // Main loop — server runs its own tick thread
    // We just wait for the shutdown signal here
    server.RunBlocking();

    Atlas::Logger::Info("[Main] Server stopped cleanly.");
    g_server = nullptr;
    return 0;
}
