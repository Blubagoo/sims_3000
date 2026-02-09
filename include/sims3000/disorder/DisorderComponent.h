/**
 * @file DisorderComponent.h
 * @brief Disorder component for ECS entities (Epic 10)
 *
 * Defines the DisorderComponent that tracks disorder generation, suppression,
 * and local disorder levels for buildings and enforcers. Buildings may be
 * disorder sources (generating crime/unrest) or enforcers (suppressing it).
 *
 * @see E10-070
 */

#ifndef SIMS3000_DISORDER_DISORDERCOMPONENT_H
#define SIMS3000_DISORDER_DISORDERCOMPONENT_H

#include <cstdint>

namespace sims3000 {
namespace disorder {

/**
 * @struct DisorderComponent
 * @brief ECS component tracking disorder generation and suppression per entity.
 *
 * Attached to buildings that either generate disorder (is_disorder_source=true)
 * or suppress it (is_enforcer=true). The local_disorder_level is a cached value
 * from the disorder overlay grid for quick access.
 *
 * Size: exactly 12 bytes (packed with explicit padding).
 */
struct DisorderComponent {
    uint16_t base_disorder_generation = 0;     ///< Base disorder output (before modifiers)
    uint16_t current_disorder_generation = 0;  ///< Current disorder output (after modifiers)
    uint16_t suppression_power = 0;            ///< Disorder suppression strength (enforcers only)
    uint8_t suppression_radius = 0;            ///< Radius of suppression effect in tiles
    uint8_t local_disorder_level = 0;          ///< Cached disorder level from overlay grid
    bool is_disorder_source = false;           ///< True if this entity generates disorder
    bool is_enforcer = false;                  ///< True if this entity suppresses disorder
    uint8_t padding[2] = {};                   ///< Explicit padding for alignment
};

static_assert(sizeof(DisorderComponent) == 12, "DisorderComponent size check");

} // namespace disorder
} // namespace sims3000

#endif // SIMS3000_DISORDER_DISORDERCOMPONENT_H
