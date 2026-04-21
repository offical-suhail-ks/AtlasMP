// shared/src/PacketCodec.cpp
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "PacketCodec.h"
#include "AtlasMath.h"
#include "../include/Packets.h"
#include <vector>

namespace Atlas {

std::vector<uint8_t> PacketCodec::Encode(Packets::PacketType type,
    const void* payload, uint16_t payloadSize,
    uint16_t senderId, uint32_t sequence)
{
    std::vector<uint8_t> buf(sizeof(Packets::PacketHeader) + payloadSize);
    auto* hdr     = reinterpret_cast<Packets::PacketHeader*>(buf.data());
    hdr->magic    = ATLAS_MAGIC;
    hdr->type     = type;
    hdr->size     = payloadSize;
    hdr->sequence = sequence;
    hdr->senderId = senderId;
    if (payload && payloadSize > 0)
        memcpy(buf.data() + sizeof(Packets::PacketHeader), payload, payloadSize);
    return buf;
}

bool PacketCodec::Decode(const uint8_t* data, size_t len,
    Packets::PacketHeader& hdrOut, const uint8_t*& payloadOut, uint16_t& sizeOut)
{
    if (!data || len < sizeof(Packets::PacketHeader)) return false;
    memcpy(&hdrOut, data, sizeof(Packets::PacketHeader));
    if (hdrOut.magic != ATLAS_MAGIC) return false;
    if (len < sizeof(Packets::PacketHeader) + hdrOut.size) return false;
    payloadOut = data + sizeof(Packets::PacketHeader);
    sizeOut    = hdrOut.size;
    return true;
}

} // namespace Atlas
