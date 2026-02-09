/**
 * @file Ordinance.h
 * @brief Ordinance framework and sample ordinances (Ticket E11-021)
 *
 * Provides an OrdinanceType enum, OrdinanceConfig presets, and
 * OrdinanceState for tracking active ordinances. Each ordinance has
 * a per-phase cost and an effect multiplier for integration with
 * other game systems.
 *
 * OrdinanceState is kept separate from TreasuryState. The integration
 * layer (EconomySystem) manages both and applies get_total_cost()
 * to TreasuryState.ordinance_costs each phase.
 */

#ifndef SIMS3000_ECONOMY_ORDINANCE_H
#define SIMS3000_ECONOMY_ORDINANCE_H

#include <array>
#include <cstdint>

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// OrdinanceType enum
// ---------------------------------------------------------------------------

/**
 * @enum OrdinanceType
 * @brief Available ordinance types.
 */
enum class OrdinanceType : uint8_t {
    EnhancedPatrol = 0,        ///< -10% disorder
    IndustrialScrubbers = 1,   ///< -15% contamination
    FreeTransit = 2            ///< +10 transport accessibility
};

/// Total number of ordinance types
constexpr uint8_t ORDINANCE_TYPE_COUNT = 3;

// ---------------------------------------------------------------------------
// OrdinanceConfig
// ---------------------------------------------------------------------------

/**
 * @struct OrdinanceConfig
 * @brief Configuration for an ordinance type.
 */
struct OrdinanceConfig {
    OrdinanceType type;            ///< Ordinance type
    const char* name;              ///< Human-readable name
    int32_t cost_per_phase;        ///< Cost deducted each budget phase
    float effect_multiplier;       ///< Multiplier for system hooks
};

// ---------------------------------------------------------------------------
// Predefined ordinance configs
// ---------------------------------------------------------------------------

/// Enhanced Patrol: 1000/phase, -10% disorder
constexpr OrdinanceConfig ORDINANCE_ENHANCED_PATROL = {
    OrdinanceType::EnhancedPatrol, "Enhanced Patrol", 1000, 0.10f
};

/// Industrial Scrubbers: 2000/phase, -15% contamination
constexpr OrdinanceConfig ORDINANCE_INDUSTRIAL_SCRUBBERS = {
    OrdinanceType::IndustrialScrubbers, "Industrial Scrubbers", 2000, 0.15f
};

/// Free Transit: 5000/phase, +10 transport accessibility
constexpr OrdinanceConfig ORDINANCE_FREE_TRANSIT = {
    OrdinanceType::FreeTransit, "Free Transit", 5000, 10.0f
};

/**
 * @brief Get the ordinance configuration for a given type.
 * @param type The ordinance type to look up.
 * @return Reference to the corresponding OrdinanceConfig.
 */
const OrdinanceConfig& get_ordinance_config(OrdinanceType type);

// ---------------------------------------------------------------------------
// OrdinanceState
// ---------------------------------------------------------------------------

/**
 * @struct OrdinanceState
 * @brief Tracks which ordinances are currently active.
 *
 * Managed separately from TreasuryState. Integration code applies
 * get_total_cost() to TreasuryState.ordinance_costs each phase.
 */
struct OrdinanceState {
    std::array<bool, ORDINANCE_TYPE_COUNT> active{}; ///< All inactive by default

    /**
     * @brief Enable an ordinance.
     * @param type The ordinance to enable.
     */
    void enable(OrdinanceType type);

    /**
     * @brief Disable an ordinance.
     * @param type The ordinance to disable.
     */
    void disable(OrdinanceType type);

    /**
     * @brief Check if an ordinance is active.
     * @param type The ordinance to check.
     * @return true if the ordinance is enabled.
     */
    bool is_active(OrdinanceType type) const;

    /**
     * @brief Get the total cost of all active ordinances per phase.
     * @return Sum of cost_per_phase for all active ordinances.
     */
    int64_t get_total_cost() const;
};

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

/**
 * @struct OrdinanceChangedEvent
 * @brief Event emitted when an ordinance is enabled or disabled.
 */
struct OrdinanceChangedEvent {
    uint8_t player_id;         ///< Player who changed the ordinance
    OrdinanceType type;        ///< Ordinance that was changed
    bool enabled;              ///< true = enabled, false = disabled
};

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_ORDINANCE_H
