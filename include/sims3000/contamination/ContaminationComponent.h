/**
 * @file ContaminationComponent.h
 * @brief Contamination component for ECS entities (Epic 10)
 *
 * Defines the ContaminationComponent that tracks contamination output,
 * spread characteristics, and cached local contamination level for
 * buildings and other contamination sources.
 *
 * @see E10-080
 */

#ifndef SIMS3000_CONTAMINATION_CONTAMINATIONCOMPONENT_H
#define SIMS3000_CONTAMINATION_CONTAMINATIONCOMPONENT_H

#include <cstdint>
#include <sims3000/contamination/ContaminationType.h>

namespace sims3000 {
namespace contamination {

/**
 * @struct ContaminationComponent
 * @brief ECS component tracking contamination output and spread for an entity.
 *
 * Attached to entities that produce environmental contamination (factories,
 * power plants, busy roads). The local_contamination_level is a cached value
 * from the contamination overlay grid.
 *
 * Size: exactly 16 bytes (packed with explicit padding).
 */
struct ContaminationComponent {
    uint32_t base_contamination_output = 0;       ///< Base contamination output (before modifiers)
    uint32_t current_contamination_output = 0;    ///< Current contamination output (after modifiers)
    uint8_t spread_radius = 4;                    ///< Radius of contamination spread in tiles
    uint8_t spread_decay_rate = 10;               ///< Decay rate per tile of distance (percentage)
    ContaminationType contamination_type = ContaminationType::Industrial; ///< Source type
    uint8_t local_contamination_level = 0;        ///< Cached contamination level from overlay grid
    bool is_active_source = false;                ///< True if currently emitting contamination
    uint8_t padding[3] = {};                      ///< Explicit padding for alignment
};

static_assert(sizeof(ContaminationComponent) == 16, "ContaminationComponent size check");

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_CONTAMINATIONCOMPONENT_H
