/**
 * @file IContaminationSource.h
 * @brief Contamination source data types for Epic 5 (Ticket 5-025)
 *
 * Defines:
 * - ContaminationType: Enum for contamination origin categories
 * - ContaminationSourceData: Per-source contamination info for downstream systems
 *
 * Used by EnergySystem::get_contamination_sources() to expose contamination
 * data to the ContaminationSystem for pollution spread calculation.
 *
 * @see /docs/canon/interfaces.yaml (energy.IContaminationSource)
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace energy {

/**
 * @enum ContaminationType
 * @brief Categories of contamination origin.
 *
 * Identifies the source system generating contamination. Currently
 * only Energy is supported (from energy nexuses), but future systems
 * (industry, transport) may add additional categories.
 */
enum class ContaminationType : uint8_t {
    None   = 0,   ///< No contamination
    Energy = 1    ///< From energy nexuses (carbon, petrochemical, gaseous)
};

/**
 * @struct ContaminationSourceData
 * @brief Per-source contamination information for downstream systems.
 *
 * Contains all information needed by the ContaminationSystem to calculate
 * pollution spread from a single contamination source. Each online energy
 * nexus with nonzero contamination_output produces one of these.
 */
struct ContaminationSourceData {
    uint32_t entity_id;              ///< Entity ID of the contamination source
    uint8_t owner_id;                ///< Owning player ID (0-3)
    uint32_t contamination_output;   ///< Current contamination output (units/tick)
    ContaminationType type;          ///< Contamination category
    uint32_t x;                      ///< Grid X position
    uint32_t y;                      ///< Grid Y position
    uint8_t radius;                  ///< Contamination spread radius (tiles)
};

} // namespace energy
} // namespace sims3000
