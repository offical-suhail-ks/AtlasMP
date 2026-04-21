// tools/rpflib/src/RPFDecrypt.cpp
// RPF7 decryption stubs
// AES mode: uses standard AES-128/256
// NG mode: uses Rockstar's custom "Next-Gen" cipher (reverse-engineered)
// Reference: OpenIV research, LibGTAV

#include "../include/RPFArchive.h"

namespace Atlas::RPF {

std::vector<uint8_t> RPFArchive::DecryptData(const uint8_t* data, size_t size) {
    // TODO: implement based on m_header.encryption
    // EncryptionType::None  → copy as-is
    // EncryptionType::AES   → AES-128-ECB with known key
    // EncryptionType::NG    → Rockstar NG cipher

    if (m_header.encryption == EncryptionType::None) {
        return std::vector<uint8_t>(data, data + size);
    }

    // Placeholder: return raw (will be wrong for encrypted files)
    return std::vector<uint8_t>(data, data + size);
}

std::vector<uint8_t> RPFArchive::DecompressData(const uint8_t* data,
                                                  size_t compressedSize,
                                                  size_t originalSize)
{
    (void)originalSize;
    // TODO: zlib inflate
    // #ifdef ATLAS_HAVE_ZLIB
    //   std::vector<uint8_t> out(originalSize);
    //   uLongf destLen = originalSize;
    //   uncompress(out.data(), &destLen, data, compressedSize);
    //   return out;
    // #endif
    return std::vector<uint8_t>(data, data + compressedSize);
}

} // namespace Atlas::RPF
