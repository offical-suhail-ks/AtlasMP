#pragma once
// server/src/core/Config.h
// Parses server.toml and provides typed accessors

#include <string>
#include <vector>
#include <cstdint>

namespace Atlas {

class Config {
public:
    Config() = default;

    bool Load(const std::string& path);
    bool SaveDefault(const std::string& path);

    // ── Server section ────────────────────────────────────────────────────────
    const std::string& GetName()       const { return m_name; }
    const std::string& GetHost()       const { return m_host; }
    uint16_t           GetPort()       const { return m_port; }
    uint32_t           GetMaxPlayers() const { return m_maxPlayers; }
    const std::string& GetPassword()   const { return m_password; }
    bool               GetAnnounce()   const { return m_announce; }
    const std::string& GetDescription() const { return m_description; }

    // ── Resources section ─────────────────────────────────────────────────────
    const std::string&              GetResourcesPath()  const { return m_resourcesPath; }
    const std::vector<std::string>& GetAutostart()      const { return m_autostart; }

    // ── Sync section ──────────────────────────────────────────────────────────
    uint32_t GetSyncTickRate()   const { return m_syncTickRate; }
    uint32_t GetMaxEntities()    const { return m_maxEntities; }

    // ── Database section ──────────────────────────────────────────────────────
    const std::string& GetDbDriver() const { return m_dbDriver; }
    const std::string& GetDbPath()   const { return m_dbPath; }

    // ── Anti-cheat section ────────────────────────────────────────────────────
    bool GetACEnabled()        const { return m_acEnabled; }
    bool GetACKickOnViolation() const { return m_acKickOnViolation; }

private:
    // Server
    std::string m_name        = "AtlasMP Server";
    std::string m_description = "A custom GTA V server";
    std::string m_host        = "0.0.0.0";
    uint16_t    m_port        = 7788;
    uint32_t    m_maxPlayers  = 128;
    std::string m_password    = "";
    bool        m_announce    = true;

    // Resources
    std::string              m_resourcesPath = "resources";
    std::vector<std::string> m_autostart;

    // Sync
    uint32_t m_syncTickRate = 60;
    uint32_t m_maxEntities  = 1000;

    // Database
    std::string m_dbDriver = "sqlite";
    std::string m_dbPath   = "data/atlasmp.db";

    // Anti-cheat
    bool m_acEnabled          = true;
    bool m_acKickOnViolation  = true;
};

} // namespace Atlas
