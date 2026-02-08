/**
 * @file BuildingSerialization.h
 * @brief Building component serialization/deserialization (Epic 4, ticket 4-042)
 *
 * Standalone serialization functions for building components.
 * Format: version byte (1) + fixed-size fields, little-endian.
 * Reserved padding bytes (4 bytes) after each component for future fields.
 *
 * Component formats:
 * - BuildingComponent: version(1) + template_id(4) + zone_type(1) + density(1) +
 *     state(1) + level(1) + health(1) + capacity(2) + current_occupancy(2) +
 *     footprint_w(1) + footprint_h(1) + state_changed_tick(4) + abandon_timer(2) +
 *     rotation(1) + color_accent_index(1) + reserved(4) = 28 bytes
 * - ConstructionComponent: version(1) + ticks_total(2) + ticks_elapsed(2) +
 *     phase(1) + phase_progress(1) + construction_cost(4) + is_paused(1) +
 *     reserved(4) = 16 bytes
 * - DebrisComponent: version(1) + original_template_id(4) + clear_timer(2) +
 *     footprint_w(1) + footprint_h(1) + reserved(4) = 13 bytes
 *
 * @see /docs/epics/epic-4/tickets.md (ticket 4-042)
 */

#ifndef SIMS3000_BUILDING_BUILDINGSERIALIZATION_H
#define SIMS3000_BUILDING_BUILDINGSERIALIZATION_H

#include <sims3000/building/BuildingComponents.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace building {

/// Current serialization version for building data
constexpr std::uint8_t BUILDING_SERIALIZATION_VERSION = 1;

/**
 * @brief Serialize a BuildingComponent to a byte buffer.
 * Format: version(1) + template_id(4) + zone_type(1) + density(1) + state(1) +
 *   level(1) + health(1) + capacity(2) + current_occupancy(2) + footprint_w(1) +
 *   footprint_h(1) + state_changed_tick(4) + abandon_timer(2) + rotation(1) +
 *   color_accent_index(1) + reserved(4) = 28 bytes
 */
void serialize_building_component(const BuildingComponent& comp, std::vector<std::uint8_t>& buffer);

/**
 * @brief Deserialize a BuildingComponent from raw bytes.
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @return Deserialized BuildingComponent.
 */
BuildingComponent deserialize_building_component(const std::uint8_t* data, std::size_t size);

/**
 * @brief Serialize a ConstructionComponent to a byte buffer.
 * Format: version(1) + ticks_total(2) + ticks_elapsed(2) + phase(1) +
 *   phase_progress(1) + construction_cost(4) + is_paused(1) + reserved(4) = 16 bytes
 */
void serialize_construction_component(const ConstructionComponent& comp, std::vector<std::uint8_t>& buffer);

/**
 * @brief Deserialize a ConstructionComponent from raw bytes.
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @return Deserialized ConstructionComponent.
 */
ConstructionComponent deserialize_construction_component(const std::uint8_t* data, std::size_t size);

/**
 * @brief Serialize a DebrisComponent to a byte buffer.
 * Format: version(1) + original_template_id(4) + clear_timer(2) +
 *   footprint_w(1) + footprint_h(1) + reserved(4) = 13 bytes
 */
void serialize_debris_component(const DebrisComponent& comp, std::vector<std::uint8_t>& buffer);

/**
 * @brief Deserialize a DebrisComponent from raw bytes.
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @return Deserialized DebrisComponent.
 */
DebrisComponent deserialize_debris_component(const std::uint8_t* data, std::size_t size);

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGSERIALIZATION_H
