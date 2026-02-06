/**
 * @file FilePersistenceProvider.cpp
 * @brief Implementation of file-based persistence provider.
 */

#include "sims3000/persistence/FilePersistenceProvider.h"
#include <fstream>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <filesystem>

// SDL3 for logging (optional, falls back to stderr)
#ifdef _WIN32
#include <io.h>
#define access _access
#define F_OK 0
#else
#include <unistd.h>
#endif

namespace sims3000 {

namespace {

// CRC32 lookup table (IEEE polynomial, complete 256 entries)
static const std::uint32_t CRC32_TABLE[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7AC5, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAEE2DE5B, 0xD9E5AECA,
    0x40ECBF4A, 0x37EBF2DC, 0xA9C8D27F, 0xDECFC7E9, 0x47C6C653, 0x30C1F6C5,
    0xBDC9CA30, 0xCACAFBB1, 0x53C33A0B, 0x24C40B9D, 0xBAA0FE3E, 0xCD07CEA8,
    0x5400DE12, 0x2307EEE8, 0xB3B8C078, 0xC4BFC0EE, 0x5DB8D054, 0x2ABF40C2,
    0xB4D90761, 0xC3DE37F7, 0x5AD7264D, 0x2DD037DB
};

/// Get current time in milliseconds since epoch.
std::uint64_t getCurrentTimeMs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
    );
}

/// Write a uint32_t to buffer in little-endian.
void writeU32(std::vector<std::uint8_t>& buf, std::uint32_t value) {
    buf.push_back(static_cast<std::uint8_t>(value & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
}

/// Write a uint64_t to buffer in little-endian.
void writeU64(std::vector<std::uint8_t>& buf, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<std::uint8_t>((value >> (i * 8)) & 0xFF));
    }
}

/// Write a string to buffer (length-prefixed).
void writeString(std::vector<std::uint8_t>& buf, const std::string& str) {
    writeU32(buf, static_cast<std::uint32_t>(str.size()));
    for (char c : str) {
        buf.push_back(static_cast<std::uint8_t>(c));
    }
}

/// Read a uint32_t from buffer in little-endian. Returns false on overflow.
bool readU32(const std::uint8_t* data, std::size_t size, std::size_t& pos, std::uint32_t& out) {
    if (pos + 4 > size) return false;
    out = static_cast<std::uint32_t>(data[pos])
        | (static_cast<std::uint32_t>(data[pos + 1]) << 8)
        | (static_cast<std::uint32_t>(data[pos + 2]) << 16)
        | (static_cast<std::uint32_t>(data[pos + 3]) << 24);
    pos += 4;
    return true;
}

/// Read a uint64_t from buffer in little-endian. Returns false on overflow.
bool readU64(const std::uint8_t* data, std::size_t size, std::size_t& pos, std::uint64_t& out) {
    if (pos + 8 > size) return false;
    out = 0;
    for (int i = 0; i < 8; ++i) {
        out |= static_cast<std::uint64_t>(data[pos + i]) << (i * 8);
    }
    pos += 8;
    return true;
}

/// Read a string from buffer (length-prefixed). Returns false on overflow.
bool readString(const std::uint8_t* data, std::size_t size, std::size_t& pos, std::string& out) {
    std::uint32_t length;
    if (!readU32(data, size, pos, length)) return false;
    if (pos + length > size) return false;
    out.assign(reinterpret_cast<const char*>(data + pos), length);
    pos += length;
    return true;
}

/// Log an error message to stderr.
void logError(const char* operation, const std::string& path, const char* details) {
    std::fprintf(stderr, "[FilePersistenceProvider] %s failed for '%s': %s\n",
                 operation, path.c_str(), details);
}

} // anonymous namespace

// =============================================================================
// Constructor
// =============================================================================

FilePersistenceProvider::FilePersistenceProvider(const std::string& filePath)
    : m_filePath(filePath)
{
}

// =============================================================================
// Entity ID Generator State
// =============================================================================

bool FilePersistenceProvider::saveEntityIdState(std::uint64_t nextId) {
    // Load existing state (or create new), update entity ID, save
    PersistentServerState state;

    if (auto existing = loadServerState()) {
        state = std::move(*existing);
    }

    state.nextEntityId = nextId;
    state.savedAt = getCurrentTimeMs();

    return saveServerState(state);
}

std::optional<std::uint64_t> FilePersistenceProvider::loadEntityIdState() {
    auto state = loadServerState();
    if (!state) {
        return std::nullopt;
    }
    return state->nextEntityId;
}

// =============================================================================
// Player Session State
// =============================================================================

bool FilePersistenceProvider::savePlayerSessions(const std::vector<PersistentPlayerSession>& sessions) {
    // Load existing state (or create new), update sessions, save
    PersistentServerState state;

    if (auto existing = loadServerState()) {
        state = std::move(*existing);
    }

    state.sessions = sessions;
    state.savedAt = getCurrentTimeMs();

    return saveServerState(state);
}

std::optional<std::vector<PersistentPlayerSession>> FilePersistenceProvider::loadPlayerSessions() {
    auto state = loadServerState();
    if (!state) {
        return std::nullopt;
    }
    return std::move(state->sessions);
}

// =============================================================================
// Combined State
// =============================================================================

bool FilePersistenceProvider::saveServerState(const PersistentServerState& state) {
    // Create backup of existing file
    if (!createBackup()) {
        logError("Backup", m_filePath, "Could not create backup");
        // Continue anyway - save is more important than backup
    }

    // Serialize state
    auto data = serializeState(state);
    if (data.empty()) {
        logError("Serialize", m_filePath, "Serialization failed");
        return false;
    }

    // Write atomically
    if (!atomicWrite(m_filePath, data)) {
        logError("Write", m_filePath, "Atomic write failed");
        return false;
    }

    return true;
}

std::optional<PersistentServerState> FilePersistenceProvider::loadServerState() {
    // Read file
    auto data = readFile(m_filePath);
    if (data.empty()) {
        // File doesn't exist or empty - not an error, just no state
        return std::nullopt;
    }

    // Deserialize
    auto state = deserializeState(data);
    if (!state) {
        logError("Deserialize", m_filePath, "Data corrupt or incompatible version");
        return std::nullopt;
    }

    return state;
}

// =============================================================================
// State Management
// =============================================================================

bool FilePersistenceProvider::clearState() {
    std::error_code ec;

    // Remove main file
    if (std::filesystem::exists(m_filePath, ec)) {
        if (!std::filesystem::remove(m_filePath, ec)) {
            logError("Clear", m_filePath, ec.message().c_str());
            return false;
        }
    }

    // Remove backup
    std::string backupPath = getBackupPath();
    if (std::filesystem::exists(backupPath, ec)) {
        std::filesystem::remove(backupPath, ec); // Best effort
    }

    // Remove temp file
    std::string tempPath = getTempPath();
    if (std::filesystem::exists(tempPath, ec)) {
        std::filesystem::remove(tempPath, ec); // Best effort
    }

    return true;
}

bool FilePersistenceProvider::hasState() const {
    std::error_code ec;
    return std::filesystem::exists(m_filePath, ec) &&
           std::filesystem::file_size(m_filePath, ec) > 0;
}

std::string FilePersistenceProvider::getStorageLocation() const {
    return m_filePath;
}

// =============================================================================
// Serialization
// =============================================================================

std::vector<std::uint8_t> FilePersistenceProvider::serializeState(const PersistentServerState& state) const {
    std::vector<std::uint8_t> buf;
    buf.reserve(256 + state.sessions.size() * 64); // Reasonable estimate

    // Header
    writeU32(buf, PERSISTENCE_FILE_MAGIC);
    writeU32(buf, state.version);
    writeU64(buf, state.savedAt);

    // Entity ID state
    writeU64(buf, state.nextEntityId);

    // Sessions
    writeU32(buf, static_cast<std::uint32_t>(state.sessions.size()));
    for (const auto& session : state.sessions) {
        // Token (16 bytes)
        for (std::size_t i = 0; i < PERSISTENCE_SESSION_TOKEN_SIZE; ++i) {
            buf.push_back(session.token[i]);
        }

        // Player ID
        buf.push_back(session.playerId);

        // Player name
        writeString(buf, session.playerName);

        // Timestamps
        writeU64(buf, session.createdAt);
        writeU64(buf, session.disconnectedAt);

        // Was connected flag
        buf.push_back(session.wasConnected ? 1 : 0);
    }

    // Calculate checksum of everything so far
    std::uint32_t checksum = calculateChecksum(buf.data(), buf.size());
    writeU32(buf, checksum);

    return buf;
}

std::optional<PersistentServerState> FilePersistenceProvider::deserializeState(
    const std::vector<std::uint8_t>& data) const
{
    if (data.size() < 28) { // Minimum: magic(4) + version(4) + timestamp(8) + entityId(8) + count(4)
        return std::nullopt;
    }

    const std::uint8_t* ptr = data.data();
    std::size_t size = data.size();
    std::size_t pos = 0;

    // Verify checksum first (checksum is last 4 bytes)
    if (size < 4) return std::nullopt;
    std::size_t checksumPos = size - 4;
    std::uint32_t storedChecksum;
    if (!readU32(ptr, size, checksumPos, storedChecksum)) return std::nullopt;

    std::uint32_t calculatedChecksum = calculateChecksum(ptr, size - 4);
    if (storedChecksum != calculatedChecksum) {
        return std::nullopt; // Checksum mismatch - corrupt data
    }

    // Read header
    std::uint32_t magic;
    if (!readU32(ptr, size - 4, pos, magic)) return std::nullopt;
    if (magic != PERSISTENCE_FILE_MAGIC) return std::nullopt;

    std::uint32_t version;
    if (!readU32(ptr, size - 4, pos, version)) return std::nullopt;
    if (version > PERSISTENCE_FILE_VERSION) return std::nullopt; // Newer version

    PersistentServerState state;
    state.version = version;

    if (!readU64(ptr, size - 4, pos, state.savedAt)) return std::nullopt;
    if (!readU64(ptr, size - 4, pos, state.nextEntityId)) return std::nullopt;

    // Read sessions
    std::uint32_t sessionCount;
    if (!readU32(ptr, size - 4, pos, sessionCount)) return std::nullopt;

    // Sanity check - max 256 sessions (way more than 4 players)
    if (sessionCount > 256) return std::nullopt;

    state.sessions.reserve(sessionCount);
    for (std::uint32_t i = 0; i < sessionCount; ++i) {
        PersistentPlayerSession session;

        // Token
        if (pos + PERSISTENCE_SESSION_TOKEN_SIZE > size - 4) return std::nullopt;
        std::memcpy(session.token.data(), ptr + pos, PERSISTENCE_SESSION_TOKEN_SIZE);
        pos += PERSISTENCE_SESSION_TOKEN_SIZE;

        // Player ID
        if (pos >= size - 4) return std::nullopt;
        session.playerId = ptr[pos++];

        // Player name
        if (!readString(ptr, size - 4, pos, session.playerName)) return std::nullopt;

        // Timestamps
        if (!readU64(ptr, size - 4, pos, session.createdAt)) return std::nullopt;
        if (!readU64(ptr, size - 4, pos, session.disconnectedAt)) return std::nullopt;

        // Was connected flag
        if (pos >= size - 4) return std::nullopt;
        session.wasConnected = (ptr[pos++] != 0);

        state.sessions.push_back(std::move(session));
    }

    return state;
}

std::uint32_t FilePersistenceProvider::calculateChecksum(const std::uint8_t* data, std::size_t length) const {
    std::uint32_t crc = 0xFFFFFFFF;
    for (std::size_t i = 0; i < length; ++i) {
        crc = CRC32_TABLE[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

// =============================================================================
// File Operations
// =============================================================================

bool FilePersistenceProvider::atomicWrite(const std::string& path, const std::vector<std::uint8_t>& data) {
    std::string tempPath = path + ".tmp";

    // Write to temp file
    {
        std::ofstream file(tempPath, std::ios::binary | std::ios::trunc);
        if (!file) {
            logError("Open temp", tempPath, "Could not open for writing");
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()),
                   static_cast<std::streamsize>(data.size()));

        if (!file) {
            logError("Write temp", tempPath, "Write failed");
            return false;
        }

        file.flush();
        if (!file) {
            logError("Flush temp", tempPath, "Flush failed");
            return false;
        }
    }

    // Rename temp to target (atomic on most filesystems)
    std::error_code ec;

    // Remove existing target if present (required on Windows)
    if (std::filesystem::exists(path, ec)) {
        if (!std::filesystem::remove(path, ec)) {
            logError("Remove old", path, ec.message().c_str());
            // Try to continue anyway
        }
    }

    std::filesystem::rename(tempPath, path, ec);
    if (ec) {
        logError("Rename", path, ec.message().c_str());

        // Try direct copy as fallback
        try {
            std::filesystem::copy_file(tempPath, path,
                std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                logError("Copy fallback", path, ec.message().c_str());
                return false;
            }
            std::filesystem::remove(tempPath, ec); // Best effort cleanup
        } catch (const std::exception& e) {
            logError("Copy exception", path, e.what());
            return false;
        }
    }

    return true;
}

std::vector<std::uint8_t> FilePersistenceProvider::readFile(const std::string& path) const {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return {}; // No file, not an error
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        logError("Open read", path, "Could not open for reading");
        return {};
    }

    auto fileSize = file.tellg();
    if (fileSize <= 0) {
        return {}; // Empty file
    }

    file.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> data(static_cast<std::size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(data.data()), fileSize)) {
        logError("Read", path, "Could not read file contents");
        return {};
    }

    return data;
}

bool FilePersistenceProvider::createBackup() {
    std::error_code ec;

    if (!std::filesystem::exists(m_filePath, ec)) {
        return true; // Nothing to backup
    }

    std::string backupPath = getBackupPath();

    // Remove existing backup
    if (std::filesystem::exists(backupPath, ec)) {
        std::filesystem::remove(backupPath, ec);
    }

    // Copy current to backup
    try {
        std::filesystem::copy_file(m_filePath, backupPath,
            std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            logError("Backup copy", backupPath, ec.message().c_str());
            return false;
        }
    } catch (const std::exception& e) {
        logError("Backup exception", backupPath, e.what());
        return false;
    }

    return true;
}

} // namespace sims3000
