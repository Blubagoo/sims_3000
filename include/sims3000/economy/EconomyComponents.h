/**
 * @file EconomyComponents.h
 * @brief Core economy ECS components (Ticket E11-001)
 *
 * Defines TributableComponent and MaintenanceCostComponent for
 * attaching economic data to building entities.
 *
 * Uses ZoneBuildingType from building/BuildingTypes.h (already defined).
 */

#ifndef SIMS3000_ECONOMY_ECONOMYCOMPONENTS_H
#define SIMS3000_ECONOMY_ECONOMYCOMPONENTS_H

#include <cstdint>
#include <type_traits>
#include <sims3000/building/BuildingTypes.h>

namespace sims3000 {
namespace economy {

// Re-export ZoneBuildingType into economy namespace for convenience
using sims3000::building::ZoneBuildingType;

/**
 * @struct TributableComponent
 * @brief ECS component for entities that generate tribute (tax) revenue.
 *
 * Attached to zone buildings to track their tribute generation parameters.
 * The actual tribute collected depends on zone_type, base_value,
 * density_level, and tribute_modifier.
 *
 * Target size: ~12 bytes.
 */
struct TributableComponent {
    ZoneBuildingType zone_type = ZoneBuildingType::Habitation; ///< Zone classification
    uint32_t base_value = 100;        ///< Base tribute value per phase
    uint8_t density_level = 0;        ///< 0=low, 1=high
    float tribute_modifier = 1.0f;    ///< Modified by sector value, services
};

static_assert(sizeof(TributableComponent) <= 16, "TributableComponent should be approximately 12 bytes");
static_assert(std::is_trivially_copyable<TributableComponent>::value, "TributableComponent must be trivially copyable");

/**
 * @struct MaintenanceCostComponent
 * @brief ECS component for entities that incur maintenance costs.
 *
 * Attached to infrastructure and service buildings to track their
 * ongoing maintenance expenses.
 *
 * Target size: 8 bytes.
 */
struct MaintenanceCostComponent {
    int32_t base_cost = 0;            ///< Base maintenance per phase
    float cost_multiplier = 1.0f;     ///< Modifier (age, damage)
};

static_assert(sizeof(MaintenanceCostComponent) == 8, "MaintenanceCostComponent should be 8 bytes");
static_assert(std::is_trivially_copyable<MaintenanceCostComponent>::value, "MaintenanceCostComponent must be trivially copyable");

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_ECONOMYCOMPONENTS_H
