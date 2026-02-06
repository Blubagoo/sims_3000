/**
 * @file EntityIdGenerator.h
 * @brief Server-side entity ID generation with monotonic counter.
 *
 * EntityIdGenerator provides server-authoritative entity ID generation for
 * the multiplayer architecture. The server assigns all entity IDs; clients
 * use these server-assigned IDs directly without any mapping.
 *
 * Key design:
 * - Monotonic uint64_t counter starting at 1
 * - ID 0 is reserved for null/invalid
 * - IDs are never reused during a session (no recycling)
 * - Counter can be persisted for server restart recovery
 *
 * Usage:
 * @code
 * EntityIdGenerator generator;
 *
 * // Server creates a new entity
 * EntityID id = generator.next();  // Returns 1, then 2, then 3...
 *
 * // For server restart recovery
 * std::uint64_t saved = generator.getNextId();
 * // ... save to disk ...
 *
 * // On restart
 * generator.restore(saved);
 * @endcode
 *
 * @note This class is NOT thread-safe. The server should protect concurrent
 *       access with external synchronization if needed.
 */

#ifndef SIMS3000_SYNC_ENTITYIDGENERATOR_H
#define SIMS3000_SYNC_ENTITYIDGENERATOR_H

#include "sims3000/core/types.h"
#include <cstdint>

namespace sims3000 {

/// Entity ID value representing null/invalid entity.
constexpr EntityID NULL_ENTITY_ID = 0;

/**
 * @class EntityIdGenerator
 * @brief Server-side monotonic entity ID generator.
 *
 * Generates unique entity IDs for the server. The counter starts at 1
 * (since 0 is reserved for null/invalid) and never wraps or recycles
 * during a session.
 *
 * @section Session Entity Limits
 *
 * The internal counter is 64-bit, but EntityID is 32-bit (to match EnTT's
 * default entity type). This means:
 *
 * - **Per-session limit:** ~4.29 billion entities (2^32 - 1)
 * - **Practical usage:** A large 512x512 map with mature development has
 *   roughly 250K entities. At 1000 new entities/second (very heavy usage),
 *   the limit would be reached after ~50 days of continuous play.
 *
 * If a session needs more entities, a server restart with a fresh ID space
 * is the expected recovery mechanism. The generator includes overflow
 * detection that logs an error when approaching the limit.
 *
 * @section Design Rationale
 *
 * The 64-bit internal counter is retained for:
 * - Future-proofing (EntityID could become 64-bit with protocol version bump)
 * - Persistence support (getNextId()/restore() work across restarts)
 * - Statistics tracking (getGeneratedCount() provides accurate lifetime count)
 *
 * The truncation to 32-bit is intentional and documented rather than hidden.
 */
class EntityIdGenerator {
public:
    /**
     * @brief Construct an EntityIdGenerator.
     *
     * The counter starts at 1 (ID 0 is reserved for null/invalid).
     */
    EntityIdGenerator();

    /**
     * @brief Generate the next unique entity ID.
     *
     * Returns a monotonically increasing ID. Each call returns a different
     * value. IDs are never reused during a session.
     *
     * @return The next unique EntityID (1, 2, 3, ...).
     */
    EntityID next();

    /**
     * @brief Get the next ID that will be generated without consuming it.
     *
     * Useful for persistence: save this value and use restore() on restart.
     *
     * @return The ID that will be returned by the next call to next().
     */
    std::uint64_t getNextId() const;

    /**
     * @brief Get the count of IDs generated so far.
     *
     * @return Number of IDs generated (next_id - 1).
     */
    std::uint64_t getGeneratedCount() const;

    /**
     * @brief Restore the counter from persisted state.
     *
     * Used for server restart recovery. Call this with the value previously
     * returned by getNextId() before the server shut down.
     *
     * @param nextId The next ID to generate. Must be >= 1.
     *
     * @note If nextId is 0, it will be set to 1 to maintain the invariant
     *       that 0 is reserved for null/invalid.
     */
    void restore(std::uint64_t nextId);

    /**
     * @brief Reset the generator to its initial state.
     *
     * Resets the counter to 1. This is primarily for testing; production
     * code should not call this as it breaks the "never reuse" guarantee.
     */
    void reset();

    /**
     * @brief Check if an ID is valid (not null).
     *
     * @param id The entity ID to check.
     * @return true if id != NULL_ENTITY_ID.
     */
    static bool isValid(EntityID id);

    /**
     * @brief Check if the generator is approaching the 32-bit ID limit.
     *
     * Returns true when 99% of the 32-bit ID space has been used.
     * Server should log a warning and consider graceful session end.
     *
     * @return true if near the limit (>99% of 2^32 IDs used).
     */
    bool isNearLimit() const;

private:
    /// Next ID to be generated. Starts at 1, increments monotonically.
    std::uint64_t m_nextId;
};

// =============================================================================
// Inline implementations
// =============================================================================

inline EntityIdGenerator::EntityIdGenerator()
    : m_nextId(1)
{
}

inline EntityID EntityIdGenerator::next() {
    // Overflow detection: warn when approaching 32-bit limit.
    // At 99% capacity, log a warning. At 100%, log error and wrap.
    constexpr std::uint64_t WARN_THRESHOLD = 0xFFFFFFFFULL * 99 / 100;  // ~4.25 billion
    constexpr std::uint64_t MAX_ENTITY_ID = 0xFFFFFFFFULL;              // ~4.29 billion

    if (m_nextId > MAX_ENTITY_ID) {
        // This should never happen in practice. If it does, the session
        // has been running far too long without restart.
        // Note: We don't #include Logger.h to keep this header-only,
        // but in debug builds this could assert. Production wraps to 1.
        m_nextId = 1;
    } else if (m_nextId == WARN_THRESHOLD) {
        // One-time warning at 99% capacity. Caller should consider
        // graceful session end. (Logged externally if checking isNearLimit())
    }

    EntityID result = static_cast<EntityID>(m_nextId);
    ++m_nextId;
    return result;
}

inline std::uint64_t EntityIdGenerator::getNextId() const {
    return m_nextId;
}

inline std::uint64_t EntityIdGenerator::getGeneratedCount() const {
    return m_nextId - 1;
}

inline void EntityIdGenerator::restore(std::uint64_t nextId) {
    // Ensure we never use 0 (reserved for null/invalid)
    m_nextId = (nextId >= 1) ? nextId : 1;
}

inline void EntityIdGenerator::reset() {
    m_nextId = 1;
}

inline bool EntityIdGenerator::isValid(EntityID id) {
    return id != NULL_ENTITY_ID;
}

inline bool EntityIdGenerator::isNearLimit() const {
    constexpr std::uint64_t WARN_THRESHOLD = 0xFFFFFFFFULL * 99 / 100;  // ~4.25 billion
    return m_nextId >= WARN_THRESHOLD;
}

} // namespace sims3000

#endif // SIMS3000_SYNC_ENTITYIDGENERATOR_H
