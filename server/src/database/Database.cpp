// server/src/database/Database.cpp
// Lightweight persistence: bans saved to bans.json, player data in memory.
// SQLite can be swapped in later by replacing the Exec() stub.
#ifndef _USE_MATH_DEFINES
#  define _USE_MATH_DEFINES
#endif
#ifdef _MSC_VER
#  include <corecrt_math.h>
#endif
#include <cmath>
#include <cstdlib>
#include "Database.h"
#include "../core/Logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace Atlas {

static std::string JsonEsc(const std::string& s) {
    std::string o; o.reserve(s.size());
    for (char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else o.push_back(c);
    }
    return o;
}

static std::string JStr(const std::string& json, const std::string& key) {
    auto p = json.find("\"" + key + "\"");
    if (p == std::string::npos) return {};
    p = json.find(':', p); if (p == std::string::npos) return {};
    p = json.find('"', p); if (p == std::string::npos) return {};
    ++p;
    std::string v;
    while (p < json.size() && json[p] != '"') {
        if (json[p] == '\\' && p+1 < json.size()) ++p;
        v.push_back(json[p++]);
    }
    return v;
}

Database::Database(const std::string& path) : m_path(path) {}
Database::~Database() { Close(); }

bool Database::Open() {
    std::filesystem::create_directories(
        std::filesystem::path(m_path).parent_path());
    m_open = true;
    LoadBansFromFile();
    Logger::Info("[Database] Opened: {}", m_path);
    return true;
}

void Database::Close() {
    if (!m_open) return;
    SaveBansToFile();
    m_open = false;
    Logger::Info("[Database] Closed.");
}

void Database::SaveBansToFile() {
    std::string bansPath = std::filesystem::path(m_path).parent_path().string() + "/bans.json";
    std::ofstream f(bansPath, std::ios::trunc);
    if (!f) return;
    f << "[\n";
    for (size_t i = 0; i < m_banCache.size(); ++i) {
        const auto& b = m_banCache[i];
        f << "  {\"id\":" << b.id
          << ",\"name\":\"" << JsonEsc(b.playerName) << "\""
          << ",\"ip\":\"" << JsonEsc(b.ip) << "\""
          << ",\"reason\":\"" << JsonEsc(b.reason) << "\""
          << ",\"admin\":\"" << JsonEsc(b.adminName) << "\""
          << "}";
        if (i + 1 < m_banCache.size()) f << ",";
        f << "\n";
    }
    f << "]\n";
}

void Database::LoadBansFromFile() {
    std::string bansPath = std::filesystem::path(m_path).parent_path().string() + "/bans.json";
    std::ifstream f(bansPath);
    if (!f) return;
    std::string json((std::istreambuf_iterator<char>(f)), {});
    // Simple line-by-line parse (each ban is one line object)
    std::istringstream ss(json);
    std::string line;
    int nextId = 1;
    while (std::getline(ss, line)) {
        if (line.find('"') == std::string::npos) continue;
        BanEntry b;
        b.playerName = JStr(line, "name");
        b.ip         = JStr(line, "ip");
        b.reason     = JStr(line, "reason");
        b.adminName  = JStr(line, "admin");
        if (b.playerName.empty() && b.ip.empty()) continue;
        b.id = nextId++;
        m_banCache.push_back(b);
        if (b.id >= m_nextBanId) {
            m_nextBanId = b.id + 1;
        }
    }
    Logger::Info("[Database] Loaded {} bans from {}", m_banCache.size(), bansPath);
}

bool Database::CreateSchema() { return true; }
bool Database::Exec(const std::string& /*sql*/) { return true; }

bool Database::AddBan(const BanEntry& ban) {
    BanEntry b = ban;
    b.id = m_nextBanId++;
    m_banCache.push_back(b);
    SaveBansToFile();
    Logger::Info("[Database] Banned '{}' ({}): {}", b.playerName, b.ip, b.reason);
    return true;
}

bool Database::RemoveBan(int id) {
    auto it = std::find_if(m_banCache.begin(), m_banCache.end(),
        [id](const BanEntry& b){ return b.id == id; });
    if (it == m_banCache.end()) return false;
    m_banCache.erase(it);
    SaveBansToFile();
    return true;
}

bool Database::IsIPBanned(const std::string& ip) const {
    for (auto& b : m_banCache) if (b.ip == ip) return true;
    return false;
}
bool Database::IsNameBanned(const std::string& name) const {
    for (auto& b : m_banCache) if (b.playerName == name) return true;
    return false;
}
std::vector<BanEntry> Database::GetBans() const { return m_banCache; }

bool Database::SetPlayerValue(const std::string& player,
                               const std::string& key,
                               const std::string& value)
{
    m_playerData[player][key] = value;
    return true;
}
std::string Database::GetPlayerValue(const std::string& player,
                                      const std::string& key,
                                      const std::string& def) const
{
    auto it = m_playerData.find(player);
    if (it == m_playerData.end()) return def;
    auto it2 = it->second.find(key);
    if (it2 == it->second.end()) return def;
    return it2->second;
}

} // namespace Atlas
