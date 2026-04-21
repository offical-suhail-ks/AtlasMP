// server/src/core/Config.cpp
#include "Config.h"
#include "Logger.h"
#include <toml++/toml.hpp>
#include <fstream>
#include <filesystem>

namespace Atlas {

bool Config::Load(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        Logger::Warn("[Config] File not found: {}", path);
        return false;
    }

    try {
        auto tbl = toml::parse_file(path);

        // [server]
        if (auto s = tbl["server"].as_table()) {
            if (auto v = s->get("name"))        m_name        = v->value_or(m_name);
            if (auto v = s->get("description")) m_description = v->value_or(m_description);
            if (auto v = s->get("host"))        m_host        = v->value_or(m_host);
            if (auto v = s->get("port"))        m_port        = (uint16_t)v->value_or((int64_t)m_port);
            if (auto v = s->get("max_players")) m_maxPlayers  = (uint32_t)v->value_or((int64_t)m_maxPlayers);
            if (auto v = s->get("password"))    m_password    = v->value_or(m_password);
            if (auto v = s->get("announce"))    m_announce    = v->value_or(m_announce);
        }

        // [resources]
        if (auto r = tbl["resources"].as_table()) {
            if (auto v = r->get("path")) m_resourcesPath = v->value_or(m_resourcesPath);
            if (auto arr = r->get("autostart"); arr && arr->is_array()) {
                m_autostart.clear();
                for (auto& el : *arr->as_array())
                    if (auto sv = el.value<std::string>())
                        m_autostart.push_back(*sv);
            }
        }

        // [sync]
        if (auto s = tbl["sync"].as_table()) {
            if (auto v = s->get("tick_rate"))    m_syncTickRate = (uint32_t)v->value_or((int64_t)m_syncTickRate);
            if (auto v = s->get("max_entities")) m_maxEntities  = (uint32_t)v->value_or((int64_t)m_maxEntities);
        }

        // [database]
        if (auto d = tbl["database"].as_table()) {
            if (auto v = d->get("driver")) m_dbDriver = v->value_or(m_dbDriver);
            if (auto v = d->get("path"))   m_dbPath   = v->value_or(m_dbPath);
        }

        // [anticheat]
        if (auto a = tbl["anticheat"].as_table()) {
            if (auto v = a->get("enabled"))           m_acEnabled         = v->value_or(m_acEnabled);
            if (auto v = a->get("kick_on_violation")) m_acKickOnViolation = v->value_or(m_acKickOnViolation);
        }

        Logger::Info("[Config] Loaded: {}", path);
        Logger::Info("[Config] Server name: {}", m_name);
        Logger::Info("[Config] Listening on: {}:{}", m_host, m_port);
        Logger::Info("[Config] Max players: {}", m_maxPlayers);
        return true;

    } catch (const toml::parse_error& e) {
        Logger::Error("[Config] Parse error in {}: {}", path, e.what());
        return false;
    }
}

bool Config::SaveDefault(const std::string& path) {
    try {
        const auto outputPath = std::filesystem::path(path);
        const auto parentPath = outputPath.parent_path();
        if (!parentPath.empty()) {
            std::filesystem::create_directories(parentPath);
        }

        std::ofstream f(path);
        if (!f.is_open()) {
            Logger::Error("[Config] Failed to create default config file: {}", path);
            return false;
        }

        f << R"([server]
name        = "AtlasMP Server"
description = "A custom GTA V server"
host        = "0.0.0.0"
port        = 7788
max_players = 128
password    = ""
announce    = true

[resources]
path      = "resources"
autostart = ["chat"]

[sync]
tick_rate    = 60
max_entities = 1000

[anticheat]
enabled           = true
kick_on_violation = true

[database]
driver = "sqlite"
path   = "data/atlasmp.db"

[logging]
level = "info"
file  = "logs/server.log"
)";
        return true;
    } catch (const std::exception& e) {
        Logger::Error("[Config] Failed writing default config {}: {}", path, e.what());
        return false;
    }
}

} // namespace Atlas
