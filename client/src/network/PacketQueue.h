#pragma once
// client/src/network/PacketQueue.h
// Thread-safe outgoing packet queue (game thread → network thread)
#include "../../../shared/include/Packets.h"
#include <queue>
#include <mutex>
#include <vector>
#include <cstdint>

namespace Atlas {

struct QueuedPacket {
    Packets::PacketType       type;
    std::vector<uint8_t>      data;
    bool                      reliable;
};

class PacketQueue {
public:
    void Push(Packets::PacketType type, const void* data,
              size_t len, bool reliable = true)
    {
        QueuedPacket pkt;
        pkt.type     = type;
        pkt.reliable = reliable;
        pkt.data.assign(static_cast<const uint8_t*>(data),
                        static_cast<const uint8_t*>(data) + len);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(pkt));
    }

    bool Pop(QueuedPacket& out) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return false;
        out = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool Empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

private:
    mutable std::mutex        m_mutex;
    std::queue<QueuedPacket>  m_queue;
};

} // namespace Atlas
