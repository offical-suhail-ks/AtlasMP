#pragma once
// server/src/database/Database.h
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <functional>

namespace Atlas {

struct BanEntry {
    int         id;
    std::string playerName;
    std::string ip;
    std::string reason;
    std::string adminName;
    std::string date;
    std::string expires;    // "permanent" or ISO date string
};

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    bool Open();
    void Close();
    bool IsOpen() const { return m_open; }

    // Ban management
    bool AddBan(const BanEntry& ban);
    bool RemoveBan(int id);
    bool IsIPBanned(const std::string& ip) const;
    bool IsNameBanned(const std::string& name) const;
    std::vector<BanEntry> GetBans() const;

    // Player data persistence (key-value per player name)
    bool  SetPlayerValue(const std::string& player,
                         const std::string& key,
                         const std::string& value);
    std::string GetPlayerValue(const std::string& player,
                               const std::string& key,
                               const std::string& defaultVal = "") const;

private:
    bool Exec(const std::string& sql);
    bool CreateSchema();
    void SaveBansToFile();
    void LoadBansFromFile();

    std::string m_path;
    void*       m_db   = nullptr;  // sqlite3* (opaque to avoid header dep)
    bool        m_open = false;
    int         m_nextBanId = 1;

    // In-memory caches (until SQLite fully wired up)
    std::vector<BanEntry> m_banCache;
    std::unordered_map<std::string,
        std::unordered_map<std::string, std::string>> m_playerData;
};

} // namespace Atlas
