/**
 * @file FilePersistenceProvider.h
 * @brief File-based persistence provider for local servers.
 *
 * FilePersistenceProvider implements IPersistenceProvider using local
 * file storage. State is saved in a binary format with version tags
 * for forward compatibility.
 *
 * File safety:
 * - Uses atomic write pattern (write temp file, then rename)
 * - Creates backup before overwriting existing state
 * - Validates data on load (version check, checksum)
 *
 * File format:
 * - Magic bytes: "ZCPS" (ZergCity Persistence State)
 * - Version: uint32_t
 * - Timestamp: uint64_t (ms since epoch)
 * - NextEntityId: uint64_t
 * - SessionCount: uint32_t
 * - Sessions[]: PersistentPlayerSession data
 * - Checksum: uint32_t (CRC32 of all preceding bytes)
 *
 * Thread safety:
 * - Not thread-safe. Caller must synchronize concurrent access.
 *
 * Usage:
 * @code
 *   FilePersistenceProvider provider("server_state.bin");
 *
 *   // Save state
 *   PersistentServerState state;
 *   state.nextEntityId = 100;
 *   provider.saveServerState(state);
 *
 *   // Load state on restart
 *   if (auto loaded = provider.loadServerState()) {
 *       // Restore from loaded state
 *   }
 * @endcode
 */

#ifndef SIMS3000_PERSISTENCE_FILEPERSISTENCEPROVIDER_H
#define SIMS3000_PERSISTENCE_FILEPERSISTENCEPROVIDER_H

#include "sims3000/persistence/IPersistenceProvider.h"
#include <string>

namespace sims3000 {

/// Magic bytes for persistence file format: "ZCPS"
constexpr std::uint32_t PERSISTENCE_FILE_MAGIC = 0x5350435A; // "ZCPS" in little-endian

/// Current persistence file format version
constexpr std::uint32_t PERSISTENCE_FILE_VERSION = 1;

/**
 * @class FilePersistenceProvider
 * @brief File-based implementation of IPersistenceProvider.
 *
 * Stores server state in a local binary file with atomic write
 * operations and integrity checking.
 */
class FilePersistenceProvider : public IPersistenceProvider {
public:
    /**
     * @brief Construct with a file path.
     *
     * @param filePath Path to the persistence file. Parent directory
     *                 must exist. File will be created on first save.
     */
    explicit FilePersistenceProvider(const std::string& filePath);

    ~FilePersistenceProvider() override = default;

    // Non-copyable, non-movable (owns file path state)
    FilePersistenceProvider(const FilePersistenceProvider&) = delete;
    FilePersistenceProvider& operator=(const FilePersistenceProvider&) = delete;
    FilePersistenceProvider(FilePersistenceProvider&&) = delete;
    FilePersistenceProvider& operator=(FilePersistenceProvider&&) = delete;

    // =========================================================================
    // Entity ID Generator State
    // =========================================================================

    bool saveEntityIdState(std::uint64_t nextId) override;
    std::optional<std::uint64_t> loadEntityIdState() override;

    // =========================================================================
    // Player Session State
    // =========================================================================

    bool savePlayerSessions(const std::vector<PersistentPlayerSession>& sessions) override;
    std::optional<std::vector<PersistentPlayerSession>> loadPlayerSessions() override;

    // =========================================================================
    // Combined State
    // =========================================================================

    bool saveServerState(const PersistentServerState& state) override;
    std::optional<PersistentServerState> loadServerState() override;

    // =========================================================================
    // State Management
    // =========================================================================

    bool clearState() override;
    bool hasState() const override;
    std::string getStorageLocation() const override;

    // =========================================================================
    // File Path Access
    // =========================================================================

    /**
     * @brief Get the main file path.
     */
    const std::string& getFilePath() const { return m_filePath; }

    /**
     * @brief Get the backup file path (.bak extension).
     */
    std::string getBackupPath() const { return m_filePath + ".bak"; }

    /**
     * @brief Get the temporary file path (.tmp extension).
     */
    std::string getTempPath() const { return m_filePath + ".tmp"; }

private:
    /**
     * @brief Serialize state to binary format.
     * @param state State to serialize.
     * @return Binary data with header, content, and checksum.
     */
    std::vector<std::uint8_t> serializeState(const PersistentServerState& state) const;

    /**
     * @brief Deserialize state from binary format.
     * @param data Binary data to deserialize.
     * @return Deserialized state, or nullopt on error.
     */
    std::optional<PersistentServerState> deserializeState(const std::vector<std::uint8_t>& data) const;

    /**
     * @brief Calculate CRC32 checksum.
     * @param data Data to checksum.
     * @param length Number of bytes.
     * @return CRC32 checksum.
     */
    std::uint32_t calculateChecksum(const std::uint8_t* data, std::size_t length) const;

    /**
     * @brief Write data to file atomically (temp + rename).
     * @param path Target file path.
     * @param data Data to write.
     * @return true on success, false on error.
     */
    bool atomicWrite(const std::string& path, const std::vector<std::uint8_t>& data);

    /**
     * @brief Read entire file contents.
     * @param path File path to read.
     * @return File contents, or empty vector on error.
     */
    std::vector<std::uint8_t> readFile(const std::string& path) const;

    /**
     * @brief Create backup of existing file.
     * @return true on success or if no file to backup.
     */
    bool createBackup();

    std::string m_filePath;
};

} // namespace sims3000

#endif // SIMS3000_PERSISTENCE_FILEPERSISTENCEPROVIDER_H
