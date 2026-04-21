#pragma once
// tools/rpflib/include/RPFArchive.h
// Read/write Rockstar Package Format v7 (.rpf) archives
// Reference: yamp-project/rpflib, OpenIV research

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <functional>

namespace Atlas::RPF {

// ─── RPF7 constants ───────────────────────────────────────────────────────────
constexpr uint32_t RPF_MAGIC        = 0x52504637; // 'RPF7'
constexpr uint32_t RPF_ENTRY_SIZE   = 16;

// Encryption types
enum class EncryptionType : uint32_t {
    None      = 0x4E45504F, // 'OPEN'
    AES       = 0x0FFFFFF9,
    NG        = 0x0FEFFFFF, // NextGen (GTAV PC)
};

// ─── RPF entry ────────────────────────────────────────────────────────────────
struct RPFEntry {
    std::string name;
    bool        isDirectory;
    uint32_t    nameOffset;
    uint32_t    dataOffset;   // Offset into RPF data section
    uint32_t    dataSize;     // Compressed size
    uint32_t    originalSize; // Uncompressed size
    bool        isCompressed;
    bool        isResource;   // RSC resource flag

    // Directory-only
    uint32_t    firstEntryIndex;
    uint32_t    entryCount;

    std::vector<RPFEntry> children; // Populated for directories
};

// ─── RPF header ───────────────────────────────────────────────────────────────
#pragma pack(push, 1)
struct RPF7Header {
    uint32_t magic;           // RPF_MAGIC
    uint32_t entryCount;
    uint32_t namesLength;
    EncryptionType encryption;
};
#pragma pack(pop)

// ─── RPFArchive ──────────────────────────────────────────────────────────────
class RPFArchive {
public:
    RPFArchive();
    ~RPFArchive();

    /// Open an RPF archive for reading
    bool Open(const std::string& path);

    /// Close the archive
    void Close();

    bool IsOpen() const { return m_file != nullptr; }

    // ── Reading ───────────────────────────────────────────────────────────────

    /// Get the root directory entry
    const RPFEntry* GetRoot() const;

    /// Find an entry by its path (e.g. "x64/audio/sfx/ambience_1.rpf")
    const RPFEntry* FindEntry(const std::string& path) const;

    /// Read entry data into a buffer. Returns empty vector on failure.
    std::vector<uint8_t> ReadEntry(const RPFEntry& entry);

    /// Extract all files to a directory
    bool ExtractAll(const std::string& outputDir,
                    std::function<void(const std::string&)> progress = nullptr);

    // ── Info ─────────────────────────────────────────────────────────────────
    const std::string& GetPath()     const { return m_path; }
    EncryptionType     GetEncryption() const { return m_header.encryption; }
    uint32_t           GetEntryCount() const { return m_header.entryCount; }

    // ── Walk entries ─────────────────────────────────────────────────────────
    using WalkCallback = std::function<void(const RPFEntry&, const std::string& fullPath)>;
    void Walk(WalkCallback cb) const;

private:
    bool ReadHeader();
    bool ReadEntries();
    bool ReadNames();
    void BuildTree();
    void WalkInternal(const RPFEntry& entry, const std::string& path,
                      WalkCallback& cb) const;

    std::vector<uint8_t> DecryptData(const uint8_t* data, size_t size);
    std::vector<uint8_t> DecompressData(const uint8_t* data,
                                         size_t compressedSize,
                                         size_t originalSize);

    FILE*       m_file   = nullptr;
    std::string m_path;
    RPF7Header  m_header = {};

    std::vector<RPFEntry>   m_entries;
    std::vector<char>       m_names;

    RPFEntry m_root;
};

} // namespace Atlas::RPF
