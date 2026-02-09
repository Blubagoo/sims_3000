/**
 * @file FundingLevel.h
 * @brief Funding level storage and API (Ticket E11-013)
 *
 * Pure calculation module for managing per-service funding levels.
 * Provides clamping, get/set accessors keyed by service_type,
 * an effectiveness curve, and a change event struct.
 *
 * No system dependencies -- operates directly on TreasuryState.
 */

#ifndef SIMS3000_ECONOMY_FUNDINGLEVEL_H
#define SIMS3000_ECONOMY_FUNDINGLEVEL_H

#include <cstdint>
#include <sims3000/economy/TreasuryState.h>

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
namespace constants {

constexpr uint8_t MIN_FUNDING_LEVEL     = 0;
constexpr uint8_t MAX_FUNDING_LEVEL     = 150;
constexpr uint8_t DEFAULT_FUNDING_LEVEL = 100;

} // namespace constants

// ---------------------------------------------------------------------------
// Event
// ---------------------------------------------------------------------------

/**
 * @struct FundingLevelChangedEvent
 * @brief Event data emitted when a funding level changes.
 *
 * Since there is no event bus yet, callers receive this via the return
 * value of set_funding_level.
 */
struct FundingLevelChangedEvent {
    uint8_t player_id;      ///< Owning player id (caller-supplied)
    uint8_t service_type;   ///< Service type whose funding changed (0-3)
    uint8_t old_level;      ///< Previous funding level (0-150)
    uint8_t new_level;      ///< New funding level after clamping (0-150)
};

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

/**
 * @brief Clamp a funding level to the valid [0, 150] range.
 * @param level Raw funding level value.
 * @return Clamped funding level.
 */
uint8_t clamp_funding_level(uint8_t level);

/**
 * @brief Read the funding level for a given service type.
 * @param treasury Treasury state to query.
 * @param service_type Service type to query (0=Enforcer, 1=HazardResponse, 2=Medical, 3=Education).
 * @return Current funding level (0-150). Returns DEFAULT_FUNDING_LEVEL for unknown types.
 */
uint8_t get_funding_level(const TreasuryState& treasury, uint8_t service_type);

/**
 * @brief Set the funding level for a given service type (clamped to 0-150).
 * @param treasury Treasury state to modify.
 * @param service_type Service type to modify (0=Enforcer, 1=HazardResponse, 2=Medical, 3=Education).
 * @param level Desired funding level (will be clamped).
 * @param player_id Owning player id (stored in the returned event).
 * @return FundingLevelChangedEvent describing the change.
 */
FundingLevelChangedEvent set_funding_level(TreasuryState& treasury,
                                           uint8_t service_type,
                                           uint8_t level,
                                           uint8_t player_id = 0);

/**
 * @brief Calculate effectiveness multiplier from a funding level.
 *
 * Implements a diminishing returns curve:
 * - 0%   -> 0.0
 * - 25%  -> 0.40
 * - 50%  -> 0.65
 * - 75%  -> 0.85
 * - 100% -> 1.0
 * - 150% -> 1.10 (capped)
 *
 * Uses a piecewise linear interpolation between key points.
 *
 * @param funding_level Funding level (0-150, values above 150 treated as 150).
 * @return Effectiveness multiplier (0.0 to 1.1).
 */
float calculate_effectiveness(uint8_t funding_level);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_FUNDINGLEVEL_H
