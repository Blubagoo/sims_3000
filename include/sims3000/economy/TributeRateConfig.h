/**
 * @file TributeRateConfig.h
 * @brief Per-zone tribute rate configuration utilities (Ticket E11-006)
 *
 * Pure calculation module for managing per-zone tribute rates.
 * Provides clamping, get/set accessors keyed by ZoneBuildingType,
 * average rate calculation, and a change event struct.
 *
 * No system dependencies -- operates directly on TreasuryState.
 */

#ifndef SIMS3000_ECONOMY_TRIBUTERATECONFIG_H
#define SIMS3000_ECONOMY_TRIBUTERATECONFIG_H

#include <cstdint>
#include <sims3000/economy/TreasuryState.h>
#include <sims3000/economy/EconomyComponents.h>

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
namespace constants {

constexpr uint8_t MIN_TRIBUTE_RATE     = 0;
constexpr uint8_t MAX_TRIBUTE_RATE     = 20;
constexpr uint8_t DEFAULT_TRIBUTE_RATE = 7;

} // namespace constants

// ---------------------------------------------------------------------------
// Event
// ---------------------------------------------------------------------------

/**
 * @struct TributeRateChangedEvent
 * @brief Event data emitted when a tribute rate changes.
 *
 * Since there is no event bus yet, callers receive this via the return
 * value of set_tribute_rate.
 */
struct TributeRateChangedEvent {
    uint8_t          player_id;   ///< Owning player id (caller-supplied)
    ZoneBuildingType zone_type;   ///< Zone whose rate changed
    uint8_t          old_rate;    ///< Previous rate (0-20)
    uint8_t          new_rate;    ///< New rate after clamping (0-20)
};

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

/**
 * @brief Clamp a tribute rate to the valid [0, 20] range.
 * @param rate Raw rate value.
 * @return Clamped rate.
 */
uint8_t clamp_tribute_rate(uint8_t rate);

/**
 * @brief Read the tribute rate for a given zone type.
 * @param treasury Treasury state to query.
 * @param zone_type Which zone type to query.
 * @return Current tribute rate (already in 0-20 range).
 */
uint8_t get_tribute_rate(const TreasuryState& treasury, ZoneBuildingType zone_type);

/**
 * @brief Set the tribute rate for a given zone type (clamped to 0-20).
 *
 * Returns a TributeRateChangedEvent describing the change.  If the rate
 * did not actually change (old == new after clamping), the event is still
 * returned with old_rate == new_rate so the caller can decide whether to
 * propagate it.
 *
 * @param treasury   Treasury state to modify.
 * @param zone_type  Which zone type to modify.
 * @param rate       Desired rate (will be clamped).
 * @param player_id  Owning player id (stored in the returned event).
 * @return TributeRateChangedEvent describing the change.
 */
TributeRateChangedEvent set_tribute_rate(TreasuryState& treasury,
                                         ZoneBuildingType zone_type,
                                         uint8_t rate,
                                         uint8_t player_id = 0);

/**
 * @brief Calculate the arithmetic mean of all three tribute rates.
 * @param treasury Treasury state to query.
 * @return Average tribute rate as a float.
 */
float get_average_tribute_rate(const TreasuryState& treasury);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_TRIBUTERATECONFIG_H
