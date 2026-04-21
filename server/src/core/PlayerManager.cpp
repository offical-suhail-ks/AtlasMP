// server/src/core/PlayerManager.cpp
#include "../include/PlayerManager.h"

namespace Atlas {

PlayerId PlayerManager::AddPlayer(const std::string& name, const std::string& ip) {
    PlayerId id = m_nextId++;
    auto player         = std::make_unique<Player>();
    player->id          = id;
    player->name        = name;
    player->ip          = ip;
    player->connectTime = Player::Clock::now();
    player->lastPacketTime = player->connectTime;
    m_players[id]       = std::move(player);
    return id;
}

void PlayerManager::RemovePlayer(PlayerId id) {
    m_players.erase(id);
}

Player* PlayerManager::GetPlayer(PlayerId id) {
    auto it = m_players.find(id);
    return it != m_players.end() ? it->second.get() : nullptr;
}

const Player* PlayerManager::GetPlayer(PlayerId id) const {
    auto it = m_players.find(id);
    return it != m_players.end() ? it->second.get() : nullptr;
}

bool PlayerManager::HasPlayer(PlayerId id) const {
    return m_players.count(id) > 0;
}

std::vector<PlayerId> PlayerManager::GetAllPlayerIds() const {
    std::vector<PlayerId> ids;
    ids.reserve(m_players.size());
    for (auto& [id, _] : m_players) ids.push_back(id);
    return ids;
}

uint32_t PlayerManager::GetPlayerCount() const {
    return (uint32_t)m_players.size();
}

} // namespace Atlas
