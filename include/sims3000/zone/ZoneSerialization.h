/**
 * @file ZoneSerialization.h
 * @brief Zone data serialization/deserialization (Epic 4, ticket 4-041)
 *
 * Standalone serialization functions for zone components and grid.
 * Format: version byte (1) + fixed-size fields, little-endian.
 *
 * @see /docs/epics/epic-4/tickets.md (ticket 4-041)
 */

#ifndef SIMS3000_ZONE_ZONESERIALIZATION_H
#define SIMS3000_ZONE_ZONESERIALIZATION_H

#include <sims3000/zone/ZoneTypes.h>
#include <sims3000/zone/ZoneGrid.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace zone {

/// Current serialization version for zone data
constexpr std::uint8_t ZONE_SERIALIZATION_VERSION = 1;

/**
 * @brief Serialize a ZoneComponent to a byte buffer.
 * Format: version(1) + zone_type(1) + density(1) + state(1) + desirability(1) = 5 bytes
 */
void serialize_zone_component(const ZoneComponent& comp, std::vector<std::uint8_t>& buffer);

/**
 * @brief Deserialize a ZoneComponent from raw bytes.
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @return Deserialized ZoneComponent.
 */
ZoneComponent deserialize_zone_component(const std::uint8_t* data, std::size_t size);

/**
 * @brief Serialize a ZoneGrid to a byte buffer.
 * Format: version(1) + width(2) + height(2) + cell_count(4) + cells(4*N)
 */
void serialize_zone_grid(const ZoneGrid& grid, std::vector<std::uint8_t>& buffer);

/**
 * @brief Deserialize a ZoneGrid from raw bytes.
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @return Deserialized ZoneGrid.
 */
ZoneGrid deserialize_zone_grid(const std::uint8_t* data, std::size_t size);

/**
 * @brief Serialize ZoneCounts to a byte buffer.
 * Format: version(1) + 9 x uint32(4) = 37 bytes
 */
void serialize_zone_counts(const ZoneCounts& counts, std::vector<std::uint8_t>& buffer);

/**
 * @brief Deserialize ZoneCounts from raw bytes.
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @return Deserialized ZoneCounts.
 */
ZoneCounts deserialize_zone_counts(const std::uint8_t* data, std::size_t size);

/**
 * @brief Serialize ZoneDemandData to a byte buffer.
 * Format: version(1) + habitation_demand(1) + exchange_demand(1) + fabrication_demand(1) = 4 bytes
 */
void serialize_zone_demand_data(const ZoneDemandData& demand, std::vector<std::uint8_t>& buffer);

/**
 * @brief Deserialize ZoneDemandData from raw bytes.
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @return Deserialized ZoneDemandData.
 */
ZoneDemandData deserialize_zone_demand_data(const std::uint8_t* data, std::size_t size);

} // namespace zone
} // namespace sims3000

#endif // SIMS3000_ZONE_ZONESERIALIZATION_H
