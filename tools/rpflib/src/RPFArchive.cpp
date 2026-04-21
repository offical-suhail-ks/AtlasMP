// tools/rpflib/src/RPFArchive.cpp
#define _CRT_SECURE_NO_WARNINGS
#include "../include/RPFArchive.h"
#include <cstring>
#include <algorithm>

namespace Atlas::RPF {

RPFArchive::RPFArchive()  = default;
RPFArchive::~RPFArchive() { Close(); }

bool RPFArchive::Open(const std::string& path) {
    m_file = fopen(path.c_str(), "rb");
    if (!m_file) return false;
    m_path = path;

    if (!ReadHeader())  { Close(); return false; }
    if (!ReadEntries()) { Close(); return false; }
    if (!ReadNames())   { Close(); return false; }
    BuildTree();
    return true;
}

void RPFArchive::Close() {
    if (m_file) { fclose(m_file); m_file = nullptr; }
    m_entries.clear();
    m_names.clear();
    m_path.clear();
}

bool RPFArchive::ReadHeader() {
    if (fread(&m_header, sizeof(m_header), 1, m_file) != 1) return false;
    if (m_header.magic != RPF_MAGIC) return false;
    return true;
}

bool RPFArchive::ReadEntries() {
    if (m_header.entryCount == 0) return true;
    m_entries.resize(m_header.entryCount);

    // Each entry is 16 bytes — read raw then parse
    // (Simplified: full implementation needs RPF encryption handling)
    struct RawEntry {
        uint32_t nameOffset;
        uint32_t dataOrChildIdx;
        uint32_t sizeOrEntryCount;
        uint32_t flags;
    };

    std::vector<RawEntry> raw(m_header.entryCount);
    if (fread(raw.data(), sizeof(RawEntry), m_header.entryCount, m_file)
        != m_header.entryCount) return false;

    for (size_t i = 0; i < raw.size(); i++) {
        auto& e      = m_entries[i];
        e.nameOffset = raw[i].nameOffset;

        // Bit 31 of nameOffset indicates directory
        e.isDirectory = (raw[i].flags & 0x80000000) != 0;

        if (e.isDirectory) {
            e.firstEntryIndex = raw[i].dataOrChildIdx;
            e.entryCount      = raw[i].sizeOrEntryCount;
        } else {
            e.dataOffset      = raw[i].dataOrChildIdx & 0x7FFFFFFF;
            e.dataSize        = raw[i].sizeOrEntryCount & 0xFFFFFF;
            e.isResource      = (raw[i].flags & 0x40000000) != 0;
        }
    }
    return true;
}

bool RPFArchive::ReadNames() {
    if (m_header.namesLength == 0) return true;
    m_names.resize(m_header.namesLength);
    if (fread(m_names.data(), 1, m_header.namesLength, m_file)
        != m_header.namesLength) return false;

    // Assign names to entries
    for (auto& e : m_entries) {
        if (e.nameOffset < m_names.size())
            e.name = &m_names[e.nameOffset];
    }
    return true;
}

void RPFArchive::BuildTree() {
    if (m_entries.empty()) return;
    m_root = m_entries[0]; // Root is always first entry (directory)
    // Full tree building would recursively assign children
    // Simplified: leave flat for now
}

const RPFEntry* RPFArchive::GetRoot() const {
    return &m_root;
}

const RPFEntry* RPFArchive::FindEntry(const std::string& path) const {
    for (const auto& e : m_entries)
        if (e.name == path) return &e;
    return nullptr;
}

std::vector<uint8_t> RPFArchive::ReadEntry(const RPFEntry& entry) {
    if (!m_file || entry.isDirectory) return {};

    std::vector<uint8_t> buf(entry.dataSize);
    fseek(m_file, entry.dataOffset, SEEK_SET);
    if (fread(buf.data(), 1, entry.dataSize, m_file) != entry.dataSize)
        return {};

    // TODO: decrypt (AES/NG) if needed
    // TODO: decompress (zlib/oodle) if isCompressed
    return buf;
}

bool RPFArchive::ExtractAll(const std::string& outputDir,
                             std::function<void(const std::string&)> progress)
{
    Walk([&](const RPFEntry& entry, const std::string& fullPath) {
        if (entry.isDirectory) return;
        if (progress) progress(fullPath);
        auto data = ReadEntry(entry);
        if (data.empty()) return;

        std::string outPath = outputDir + "/" + fullPath;
        // Create parent directories
        size_t pos = outPath.rfind('/');
        if (pos != std::string::npos) {
            // mkdir -p equivalent (platform-specific)
        }
        FILE* f = fopen(outPath.c_str(), "wb");
        if (f) { fwrite(data.data(), 1, data.size(), f); fclose(f); }
    });
    return true;
}

void RPFArchive::Walk(WalkCallback cb) const {
    WalkInternal(m_root, "", cb);
}

void RPFArchive::WalkInternal(const RPFEntry& entry,
                               const std::string& path,
                               WalkCallback& cb) const
{
    std::string fullPath = path.empty() ? entry.name : path + "/" + entry.name;
    cb(entry, fullPath);
    for (const auto& child : entry.children)
        WalkInternal(child, fullPath, cb);
}

} // namespace Atlas::RPF
