/**
 * @file FluidPlacementValidation.cpp
 * @brief Placement validation for fluid extractors and reservoirs (Tickets 6-027, 6-028)
 *
 * Implements:
 * - calculate_water_factor(): distance-to-efficiency curve
 * - validate_extractor_placement(): bounds, water proximity, terrain, structure checks
 * - validate_reservoir_placement(): bounds, terrain, structure checks (no water req)
 *
 * Follows the same validation pattern as EnergySystem::validate_nexus_placement().
 *
 * @see FluidPlacementValidation.h for API documentation
 */

#include <sims3000/fluid/FluidPlacementValidation.h>
#include <sims3000/terrain/ITerrainQueryable.h>

namespace sims3000 {
namespace fluid {

// =============================================================================
// Water Factor Calculation
// =============================================================================

float calculate_water_factor(uint8_t distance) {
    if (distance == 0) {
        return 1.0f;   // 100% - directly on water
    }
    if (distance <= 2) {
        return 0.9f;   // 90% - very close
    }
    if (distance <= 4) {
        return 0.7f;   // 70% - close
    }
    if (distance <= 6) {
        return 0.5f;   // 50% - moderate distance
    }
    if (distance <= 8) {
        return 0.3f;   // 30% - far but operational
    }
    return 0.0f;        // 0% - too far, cannot operate
}

// =============================================================================
// Extractor Placement Validation
// =============================================================================

ExtractorPlacementResult validate_extractor_placement(
    uint32_t x, uint32_t y, uint8_t owner,
    uint32_t map_width, uint32_t map_height,
    const terrain::ITerrainQueryable* terrain,
    entt::registry* registry)
{
    ExtractorPlacementResult result;

    // 1. Bounds check
    if (x >= map_width || y >= map_height) {
        result.can_place = false;
        result.reason = "Position out of bounds";
        return result;
    }

    // 2. Water proximity check
    uint8_t water_dist = 255; // default: max distance (no terrain)
    if (terrain) {
        uint32_t raw_dist = terrain->get_water_distance(
            static_cast<int32_t>(x), static_cast<int32_t>(y));
        // Clamp to uint8_t range (get_water_distance returns uint32_t capped at 255)
        water_dist = static_cast<uint8_t>(raw_dist > 255 ? 255 : raw_dist);
    }

    result.water_distance = water_dist;

    if (water_dist > EXTRACTOR_DEFAULT_MAX_PLACEMENT_DISTANCE) {
        result.can_place = false;
        result.reason = "Too far from water source";
        result.expected_efficiency = 0.0f;
        result.will_be_operational = false;
        return result;
    }

    // 3. Calculate expected efficiency from distance
    result.expected_efficiency = calculate_water_factor(water_dist);

    // 4. Will be operational check
    result.will_be_operational = (water_dist <= EXTRACTOR_DEFAULT_MAX_OPERATIONAL_DISTANCE);

    // 5. Terrain buildable check (if terrain != nullptr)
    if (terrain) {
        if (!terrain->is_buildable(static_cast<int32_t>(x), static_cast<int32_t>(y))) {
            result.can_place = false;
            result.reason = "Terrain not buildable";
            return result;
        }
    }
    // If terrain is nullptr, default to buildable (stub)

    // 6. No existing structure check (stub: always passes)
    // Future: if registry != nullptr, check for entities at position
    (void)owner;
    (void)registry;

    result.can_place = true;
    result.reason = "";
    return result;
}

// =============================================================================
// Reservoir Placement Validation
// =============================================================================

ReservoirPlacementResult validate_reservoir_placement(
    uint32_t x, uint32_t y, uint8_t owner,
    uint32_t map_width, uint32_t map_height,
    const terrain::ITerrainQueryable* terrain,
    entt::registry* registry)
{
    ReservoirPlacementResult result;

    // 1. Bounds check
    if (x >= map_width || y >= map_height) {
        result.can_place = false;
        result.reason = "Position out of bounds";
        return result;
    }

    // 2. Terrain buildable check (if terrain != nullptr)
    if (terrain) {
        if (!terrain->is_buildable(static_cast<int32_t>(x), static_cast<int32_t>(y))) {
            result.can_place = false;
            result.reason = "Terrain not buildable";
            return result;
        }
    }
    // If terrain is nullptr, default to buildable (stub)

    // 3. No existing structure check (stub: always passes)
    // Future: if registry != nullptr, check for entities at position
    (void)owner;
    (void)registry;

    // NO water proximity requirement -- reservoirs can be placed anywhere buildable

    result.can_place = true;
    result.reason = "";
    return result;
}

} // namespace fluid
} // namespace sims3000
