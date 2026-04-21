#pragma once
// shared/src/PacketCodec.h
#include "../include/Packets.h"
#include <vector>
#include <cstdint>

namespace Atlas {

class PacketCodec {
public:
    static std::vector<uint8_t> Encode(Packets::PacketType type,
                                        const void* payload,
                                        uint16_t payloadSize,
                                        uint16_t senderId  = 0,
                                        uint32_t sequence  = 0);

    static bool Decode(const uint8_t* data, size_t len,
                       Packets::PacketHeader& headerOut,
                       const uint8_t*& payloadOut,
                       uint16_t& payloadSizeOut);
};

} // namespace Atlas
