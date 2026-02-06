/**
 * @file IPersistenceProvider.h
 * @brief Abstract interface for persisting server state.
 *
 * IPersistenceProvider defines methods for saving and loading critical server
 * state that must survive server restarts:
 * - EntityIdGenerator state (next ID counter)
 * - PlayerSession data (tokens, timestamps, connection state)
 *
 * Implementations:
 * - NullPersistenceProvider: No-op for testing (no actual persistence)
 * - FilePersistenceProvider: File-based persistence for local servers
 *
 * Error handling:
 * - Load operations return std::optional (empty on failure/missing)
 * - Save operations return bool (false on failure)
 * - Implementations should log errors with context
 *
 * Thread safety:
 * - Not thread-safe. Caller must synchronize concurrent access.
 *
 * Usage:
 * @code
 *   // Save entity ID generator state
 *   provider->saveEntityIdState(generator.getNextId());
 *
 *   // Load on restart
 *   if (auto nextId = provider->loadEntityIdState()) {
 *       generator.restore(*nextId);
 *   }
 *
 *   // Save player sessions
 *   provider->savePlayerSessions(sessions);
 *
 *   // Load on restart
 *   if (auto sessions = provider->loadPlayerSessions()) {
 *       // Restore sessions...
 *   }
 * @endcode
 */

#ifndef SIMS3000_PERSISTENCE_IPERSISTENCEPROVIDER_H
#define SIMS3000_PERSISTENCE_IPERSISTENCEPROVIDER_H

#include <cstdint>
#include <optional>
#include <vector>
#include <string>
#include <array>

namespace sims3000{

// Forward declaration from NetworkServer.h
struct PlayerSession;

/// Session token size in bytes (128-bit = 16 bytes).
constexpr std::size_t PERSISTENCE_SESSION_TOKEN_SIZE = 16;

/**
 * @struct PersistentPlayerSession
 * @brief Serializable player session data for persistence.
 *
 * This is a simplified version of PlayerSession containing only
 * the data needed for reconnection after server restart.
 */
struct PersistentPlayerSession {
    /// 128-bit session token for reconnection.
    std::array<std::uint8_t, PERSISTENCE_SESSION_TOKEN_SIZE> token{};

    /// Player ID assigned to this session.
    std::uint8_t playerId = 0;

    /// Player name for verification.
    std::string playerName;

    /// Timestamp when session was created (real-world ms since epoch).
    std::uint64_t createdAt = 0;

    /// Timestamp when player disconnected (0 if was connected at save time).
    std::uint64_t disconnectedAt = 0;

    /// Whether the session was connected at save time.
    bool wasConnected = false;
};

/**
 * @struct PersistentServerState
 * @brief Complete server state for persistence.
 *
 * Combines all persistable state into a single structure for
 * atomic save/load operations.
 */
struct PersistentServerState {
    /// Version number for format compatibility.
    std::uint32_t version = 1;

    /// Next entity ID to generate.
    std::uint64_t nextEntityId = 1;

    /// All player sessions (active and within grace period).
    std::vector<PersistentPlayerSession> sessions;

    /// Timestamp when state was saved (real-world ms since epoch).
    std::uint64_t savedAt = 0;
};

/**
 * @class IPersistenceProvider
 * @brief Abstract interface for server state persistence.
 *
 * Defines the contract for saving and loading server state that must
 * survive restarts. Implementations may use files, databases, or other
 * storage mechanisms.
 */
class IPersistenceProvider {
public:
    virtual ~IPersistenceProvider() = default;

    // =========================================================================
    // Entity ID Generator State
    // =========================================================================

    /**
     * @brief Save the entity ID generator state.
     *
     * Persists the next entity ID to be generated. This ensures IDs are
     * never reused after a server restart.
     *
     * @param nextId The next ID that will be generated.
     * @return true if saved successfully, false on error.
     */
    virtual bool saveEntityIdState(std::uint64_t nextId) = 0;

    /**
     * @brief Load the entity ID generator state.
     *
     * @return The next entity ID to generate, or std::nullopt if
     *         no state exists or data is corrupt.
     */
    virtual std::optional<std::uint64_t> loadEntityIdState() = 0;

    // =========================================================================
    // Player Session State
    // =========================================================================

    /**
     * @brief Save player session state.
     *
     * Persists all active sessions (connected or within grace period).
     * This enables session token validation after server restart.
     *
     * @param sessions Vector of session data to persist.
     * @return true if saved successfully, false on error.
     */
    virtual bool savePlayerSessions(const std::vector<PersistentPlayerSession>& sessions) = 0;

    /**
     * @brief Load player session state.
     *
     * @return Vector of sessions, or std::nullopt if no state exists
     *         or data is corrupt.
     */
    virtual std::optional<std::vector<PersistentPlayerSession>> loadPlayerSessions() = 0;

    // =========================================================================
    // Combined State (Atomic Operations)
    // =========================================================================

    /**
     * @brief Save complete server state atomically.
     *
     * Saves all server state in a single operation. This is preferred
     * over individual save calls for consistency.
     *
     * @param state Complete server state to persist.
     * @return true if saved successfully, false on error.
     */
    virtual bool saveServerState(const PersistentServerState& state) = 0;

    /**
     * @brief Load complete server state.
     *
     * @return Complete server state, or std::nullopt if no state exists
     *         or data is corrupt.
     */
    virtual std::optional<PersistentServerState> loadServerState() = 0;

    // =========================================================================
    // State Management
    // =========================================================================

    /**
     * @brief Clear all persisted state.
     *
     * Removes all saved state. Used when starting a new game or
     * for testing cleanup.
     *
     * @return true if cleared successfully, false on error.
     */
    virtual bool clearState() = 0;

    /**
     * @brief Check if any persisted state exists.
     *
     * @return true if loadServerState() would return data.
     */
    virtual bool hasState() const = 0;

    /**
     * @brief Get the storage location description.
     *
     * Returns a human-readable description of where state is stored
     * (e.g., file path, "memory", etc.).
     *
     * @return Storage location description.
     */
    virtual std::string getStorageLocation() const = 0;
};

} // namespace sims3000

#endif // SIMS3000_PERSISTENCE_IPERSISTENCEPROVIDER_H
