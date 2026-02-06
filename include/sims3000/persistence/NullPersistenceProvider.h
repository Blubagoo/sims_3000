/**
 * @file NullPersistenceProvider.h
 * @brief No-op persistence provider for testing.
 *
 * NullPersistenceProvider implements IPersistenceProvider with no actual
 * persistence. All save operations succeed but discard data. All load
 * operations return empty results.
 *
 * Use cases:
 * - Unit testing without file I/O
 * - Development/debugging without state persistence
 * - Single-session servers that don't need restart recovery
 *
 * Thread safety: Thread-safe (no state).
 */

#ifndef SIMS3000_PERSISTENCE_NULLPERSISTENCEPROVIDER_H
#define SIMS3000_PERSISTENCE_NULLPERSISTENCEPROVIDER_H

#include "sims3000/persistence/IPersistenceProvider.h"

namespace sims3000 {

/**
 * @class NullPersistenceProvider
 * @brief No-op implementation of IPersistenceProvider.
 *
 * Useful for testing and scenarios where persistence is not needed.
 * All save operations succeed, all load operations return empty.
 */
class NullPersistenceProvider : public IPersistenceProvider {
public:
    NullPersistenceProvider() = default;
    ~NullPersistenceProvider() override = default;

    // =========================================================================
    // Entity ID Generator State
    // =========================================================================

    /**
     * @brief No-op save. Always succeeds.
     */
    bool saveEntityIdState(std::uint64_t /*nextId*/) override {
        return true;
    }

    /**
     * @brief No state stored. Always returns empty.
     */
    std::optional<std::uint64_t> loadEntityIdState() override {
        return std::nullopt;
    }

    // =========================================================================
    // Player Session State
    // =========================================================================

    /**
     * @brief No-op save. Always succeeds.
     */
    bool savePlayerSessions(const std::vector<PersistentPlayerSession>& /*sessions*/) override {
        return true;
    }

    /**
     * @brief No state stored. Always returns empty.
     */
    std::optional<std::vector<PersistentPlayerSession>> loadPlayerSessions() override {
        return std::nullopt;
    }

    // =========================================================================
    // Combined State
    // =========================================================================

    /**
     * @brief No-op save. Always succeeds.
     */
    bool saveServerState(const PersistentServerState& /*state*/) override {
        return true;
    }

    /**
     * @brief No state stored. Always returns empty.
     */
    std::optional<PersistentServerState> loadServerState() override {
        return std::nullopt;
    }

    // =========================================================================
    // State Management
    // =========================================================================

    /**
     * @brief No-op clear. Always succeeds.
     */
    bool clearState() override {
        return true;
    }

    /**
     * @brief No state ever stored.
     */
    bool hasState() const override {
        return false;
    }

    /**
     * @brief Returns "null" as the storage location.
     */
    std::string getStorageLocation() const override {
        return "null";
    }
};

} // namespace sims3000

#endif // SIMS3000_PERSISTENCE_NULLPERSISTENCEPROVIDER_H
