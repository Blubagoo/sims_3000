/**
 * @file PortZoneValidation.cpp
 * @brief Port zone validation implementation for Epic 8 (Tickets E8-008, E8-009)
 *
 * Implements aero port and aqua port zone validation:
 * - Aero port: minimum size, flat runway detection, pathway access
 * - Aqua port: minimum size, water adjacency, dock tiles, pathway access
 */

#include <sims3000/port/PortZoneValidation.h>
#include <sims3000/terrain/TerrainTypes.h>

namespace sims3000 {
namespace port {

// =============================================================================
// Internal helpers
// =============================================================================

namespace {

/**
 * @brief Check if a terrain type is water.
 *
 * Water types: DeepVoid (2), FlowChannel (3), StillBasin (4)
 */
bool is_water_type(terrain::TerrainType type) {
    return type == terrain::TerrainType::DeepVoid ||
           type == terrain::TerrainType::FlowChannel ||
           type == terrain::TerrainType::StillBasin;
}

/**
 * @brief Check if any tile on the zone perimeter is accessible via pathway.
 *
 * Iterates zone perimeter tiles and checks if any tile within
 * PORT_PATHWAY_MAX_DISTANCE of the perimeter has road access.
 *
 * @param zone Zone rectangle.
 * @param transport Transport provider for road accessibility.
 * @return true if at least one perimeter tile has pathway access.
 */
bool check_pathway_accessibility(const terrain::GridRect& zone,
                                  const building::ITransportProvider& transport) {
    // Check all perimeter tiles for pathway accessibility
    // Top and bottom edges
    for (int16_t x = zone.x; x < zone.right(); ++x) {
        // Top edge
        if (transport.is_road_accessible_at(
                static_cast<uint32_t>(x),
                static_cast<uint32_t>(zone.y),
                PORT_PATHWAY_MAX_DISTANCE)) {
            return true;
        }
        // Bottom edge
        if (zone.height > 1) {
            if (transport.is_road_accessible_at(
                    static_cast<uint32_t>(x),
                    static_cast<uint32_t>(zone.bottom() - 1),
                    PORT_PATHWAY_MAX_DISTANCE)) {
                return true;
            }
        }
    }
    // Left and right edges (excluding corners already checked)
    for (int16_t y = static_cast<int16_t>(zone.y + 1);
         y < static_cast<int16_t>(zone.bottom() - 1); ++y) {
        // Left edge
        if (transport.is_road_accessible_at(
                static_cast<uint32_t>(zone.x),
                static_cast<uint32_t>(y),
                PORT_PATHWAY_MAX_DISTANCE)) {
            return true;
        }
        // Right edge
        if (zone.width > 1) {
            if (transport.is_road_accessible_at(
                    static_cast<uint32_t>(zone.right() - 1),
                    static_cast<uint32_t>(y),
                    PORT_PATHWAY_MAX_DISTANCE)) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Try to find a valid runway within the zone.
 *
 * Scans the zone for a contiguous rectangular area that is:
 * - At least AERO_RUNWAY_MIN_LENGTH tiles long (horizontal)
 * - At least AERO_RUNWAY_MIN_WIDTH tiles wide (vertical)
 * - All tiles at the same elevation (flat terrain)
 *
 * Tries horizontal runways first, then vertical runways.
 *
 * @param zone Zone rectangle.
 * @param terrain_query Terrain query interface.
 * @return true if a valid runway area exists.
 */
bool find_valid_runway(const terrain::GridRect& zone,
                       const terrain::ITerrainQueryable& terrain_query) {
    // Try horizontal runways (length along X axis)
    if (zone.width >= AERO_RUNWAY_MIN_LENGTH && zone.height >= AERO_RUNWAY_MIN_WIDTH) {
        for (int16_t start_y = zone.y;
             start_y <= static_cast<int16_t>(zone.bottom() - AERO_RUNWAY_MIN_WIDTH);
             ++start_y) {
            for (int16_t start_x = zone.x;
                 start_x <= static_cast<int16_t>(zone.right() - AERO_RUNWAY_MIN_LENGTH);
                 ++start_x) {
                // Check if this runway candidate is flat
                uint8_t ref_elevation = terrain_query.get_elevation(start_x, start_y);
                bool flat = true;

                for (int16_t ry = start_y;
                     ry < static_cast<int16_t>(start_y + AERO_RUNWAY_MIN_WIDTH) && flat;
                     ++ry) {
                    for (int16_t rx = start_x;
                         rx < static_cast<int16_t>(start_x + AERO_RUNWAY_MIN_LENGTH) && flat;
                         ++rx) {
                        if (terrain_query.get_elevation(rx, ry) != ref_elevation) {
                            flat = false;
                        }
                    }
                }

                if (flat) {
                    return true;
                }
            }
        }
    }

    // Try vertical runways (length along Y axis)
    if (zone.height >= AERO_RUNWAY_MIN_LENGTH && zone.width >= AERO_RUNWAY_MIN_WIDTH) {
        for (int16_t start_x = zone.x;
             start_x <= static_cast<int16_t>(zone.right() - AERO_RUNWAY_MIN_WIDTH);
             ++start_x) {
            for (int16_t start_y = zone.y;
                 start_y <= static_cast<int16_t>(zone.bottom() - AERO_RUNWAY_MIN_LENGTH);
                 ++start_y) {
                uint8_t ref_elevation = terrain_query.get_elevation(start_x, start_y);
                bool flat = true;

                for (int16_t rx = start_x;
                     rx < static_cast<int16_t>(start_x + AERO_RUNWAY_MIN_WIDTH) && flat;
                     ++rx) {
                    for (int16_t ry = start_y;
                         ry < static_cast<int16_t>(start_y + AERO_RUNWAY_MIN_LENGTH) && flat;
                         ++ry) {
                        if (terrain_query.get_elevation(rx, ry) != ref_elevation) {
                            flat = false;
                        }
                    }
                }

                if (flat) {
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * @brief Count water-adjacent perimeter tiles for aqua port dock placement.
 *
 * Checks each tile on the zone perimeter for adjacency to water tiles
 * (DeepVoid, FlowChannel, StillBasin) immediately outside the zone boundary.
 *
 * @param zone Zone rectangle.
 * @param terrain_query Terrain query interface.
 * @return Number of perimeter tiles adjacent to water outside the zone.
 */
uint32_t count_water_adjacent_perimeter_tiles(
    const terrain::GridRect& zone,
    const terrain::ITerrainQueryable& terrain_query) {

    uint32_t count = 0;

    // Check top edge - look at tiles above the zone (y - 1)
    if (zone.y > 0) {
        for (int16_t x = zone.x; x < zone.right(); ++x) {
            if (is_water_type(terrain_query.get_terrain_type(x, zone.y - 1))) {
                ++count;
            }
        }
    }

    // Check bottom edge - look at tiles below the zone (bottom())
    for (int16_t x = zone.x; x < zone.right(); ++x) {
        if (is_water_type(terrain_query.get_terrain_type(x, zone.bottom()))) {
            ++count;
        }
    }

    // Check left edge - look at tiles left of the zone (x - 1)
    if (zone.x > 0) {
        for (int16_t y = zone.y; y < zone.bottom(); ++y) {
            if (is_water_type(terrain_query.get_terrain_type(zone.x - 1, y))) {
                ++count;
            }
        }
    }

    // Check right edge - look at tiles right of the zone (right())
    for (int16_t y = zone.y; y < zone.bottom(); ++y) {
        if (is_water_type(terrain_query.get_terrain_type(zone.right(), y))) {
            ++count;
        }
    }

    return count;
}

} // anonymous namespace

// =============================================================================
// Public API
// =============================================================================

bool validate_aero_port_zone(const terrain::GridRect& zone,
                              const terrain::ITerrainQueryable& terrain_query,
                              const building::ITransportProvider& transport) {
    // 1. Check minimum zone size (36 tiles)
    uint32_t area = static_cast<uint32_t>(zone.width) * static_cast<uint32_t>(zone.height);
    if (area < AERO_PORT_MIN_TILES) {
        return false;
    }

    // 2. Check for valid runway (flat terrain, minimum 6x2)
    if (!find_valid_runway(zone, terrain_query)) {
        return false;
    }

    // 3. Check pathway accessibility (within 3 tiles of zone perimeter)
    if (!check_pathway_accessibility(zone, transport)) {
        return false;
    }

    return true;
}

bool validate_aqua_port_zone(const terrain::GridRect& zone,
                              const terrain::ITerrainQueryable& terrain_query,
                              const building::ITransportProvider& transport) {
    // 1. Check minimum zone size (32 tiles)
    uint32_t area = static_cast<uint32_t>(zone.width) * static_cast<uint32_t>(zone.height);
    if (area < AQUA_PORT_MIN_TILES) {
        return false;
    }

    // 2. Check water adjacency on perimeter - minimum 4 water-adjacent tiles
    uint32_t dock_tiles = count_water_adjacent_perimeter_tiles(zone, terrain_query);
    if (dock_tiles < AQUA_PORT_MIN_DOCK_TILES) {
        return false;
    }

    // 3. Check pathway accessibility (within 3 tiles of zone perimeter)
    if (!check_pathway_accessibility(zone, transport)) {
        return false;
    }

    return true;
}

} // namespace port
} // namespace sims3000
