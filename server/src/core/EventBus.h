#pragma once
// server/src/core/EventBus.h
// Internal C++ event bus for server subsystem communication.
// NOT the scripting event system — this is C++-to-C++ only.

#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <any>
#include <typeindex>

namespace Atlas {

// ─── EventBus ────────────────────────────────────────────────────────────────
// Typed pub/sub. Subsystems subscribe to events using their C++ types.
// 
// Usage:
//   bus.Subscribe<PlayerJoinEvent>([](const PlayerJoinEvent& e) {
//       Logger::Info("Player {} joined", e.playerId);
//   });
//   bus.Publish(PlayerJoinEvent{ playerId = 3, name = "Suhail" });

class EventBus
{
public:
    using HandlerId = uint64_t;

    template<typename EventT>
    HandlerId Subscribe(std::function<void(const EventT&)> handler)
    {
        auto id = m_nextId++;
        auto key = std::type_index(typeid(EventT));
        m_handlers[key].push_back({ id, [handler](const std::any& e) {
            handler(std::any_cast<const EventT&>(e));
        }});
        return id;
    }

    template<typename EventT>
    void Publish(const EventT& event)
    {
        auto key = std::type_index(typeid(EventT));
        auto it  = m_handlers.find(key);
        if (it == m_handlers.end()) return;
        std::any wrapped = event;
        for (auto& entry : it->second) {
            entry.fn(wrapped);
        }
    }

    void Unsubscribe(HandlerId id)
    {
        for (auto& [key, entries] : m_handlers) {
            entries.erase(
                std::remove_if(entries.begin(), entries.end(),
                    [id](const HandlerEntry& e){ return e.id == id; }),
                entries.end()
            );
        }
    }

private:
    struct HandlerEntry {
        HandlerId id;
        std::function<void(const std::any&)> fn;
    };

    std::unordered_map<std::type_index, std::vector<HandlerEntry>> m_handlers;
    HandlerId m_nextId = 1;
};

// ─── Built-in server events ───────────────────────────────────────────────────

struct PlayerJoinEvent {
    uint16_t    playerId;
    std::string name;
    std::string ip;
};

struct PlayerLeaveEvent {
    uint16_t    playerId;
    std::string reason;
};

struct PlayerChatEvent {
    uint16_t    playerId;
    std::string message;
};

struct PlayerDeathEvent {
    uint16_t playerId;
    uint16_t killerId;     // 0 if no killer
    uint32_t weaponHash;
};

struct ResourceStartEvent {
    std::string resourceName;
};

struct ResourceStopEvent {
    std::string resourceName;
};

struct ServerTickEvent {
    float deltaTime;
    uint64_t tickCount;
};

} // namespace Atlas
