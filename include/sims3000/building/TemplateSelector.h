/**
 * @file TemplateSelector.h
 * @brief Weighted random template selection algorithm (Epic 4, ticket 4-022)
 *
 * Implements deterministic weighted random selection of building templates
 * based on zone type, density, land value, and neighbor context.
 *
 * Selection steps:
 * 1. Get candidate pool from registry for zone_type + density
 * 2. Filter by min_land_value <= land_value
 * 3. Filter by min_level <= 1 (initial spawn)
 * 4. Weight candidates with duplicate penalty
 * 5. Weighted random selection using seeded PRNG
 *
 * Per CCR-010: NO scale variation - rotation and color accent only.
 *
 * @see /docs/epics/epic-4/tickets.md (ticket 4-022)
 */

#ifndef SIMS3000_BUILDING_TEMPLATESELECTOR_H
#define SIMS3000_BUILDING_TEMPLATESELECTOR_H

#include <sims3000/building/BuildingTemplate.h>
#include <sims3000/building/BuildingTypes.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace building {

/**
 * @struct TemplateSelectionResult
 * @brief Result of a template selection operation.
 *
 * Contains the selected template ID plus variation parameters
 * (rotation and color accent). No scale variation per CCR-010.
 */
struct TemplateSelectionResult {
    std::uint32_t template_id;          ///< Selected template ID (0 = no selection)
    std::uint8_t rotation;              ///< Rotation (0-3 for 0/90/180/270 degrees)
    std::uint8_t color_accent_index;    ///< Index into template's accent palette

    /// Default constructor - no selection
    TemplateSelectionResult()
        : template_id(0)
        , rotation(0)
        , color_accent_index(0)
    {}
};

/**
 * @brief Select a building template using weighted random selection.
 *
 * Deterministic selection based on position and simulation tick.
 * Same inputs always produce the same output.
 *
 * Algorithm:
 * 1. Get candidate pool from registry for zone_type + density
 * 2. Filter by min_land_value <= land_value
 * 3. Filter by min_level <= 1 (initial spawn)
 * 4. Weight candidates:
 *    - Base weight: 1.0
 *    - Land value bonus: +0.5 if land_value > 100
 *    - Duplicate penalty: -0.7 per neighbor match (orthogonal only)
 *    - Minimum weight: 0.1
 * 5. Weighted random selection using seeded PRNG
 *
 * PRNG seed: hash of (tile_x, tile_y, sim_tick)
 *   seed = tile_x * 73856093 ^ tile_y * 19349663 ^ sim_tick * 83492791
 *   Uses std::minstd_rand for determinism.
 *
 * Variation output:
 *   rotation = rand() % 4 (0-3)
 *   color_accent_index = rand() % template.color_accent_count
 *
 * Fallback: If no candidates pass filtering, falls back to first template
 * in pool with minimum weight.
 *
 * @param registry The building template registry.
 * @param zone_type Zone building type to select for.
 * @param density Density level to select for.
 * @param land_value Current land value at tile (0-255 scale).
 * @param tile_x Tile X coordinate (for deterministic seed).
 * @param tile_y Tile Y coordinate (for deterministic seed).
 * @param sim_tick Current simulation tick (for deterministic seed).
 * @param neighbor_template_ids Template IDs of orthogonal neighbors
 *        (up to 4 entries; 0 = no neighbor).
 * @return TemplateSelectionResult with selected template and variation.
 *         template_id = 0 if no templates available in pool.
 */
TemplateSelectionResult select_template(
    const BuildingTemplateRegistry& registry,
    ZoneBuildingType zone_type,
    DensityLevel density,
    float land_value,
    std::int32_t tile_x,
    std::int32_t tile_y,
    std::uint64_t sim_tick,
    const std::vector<std::uint32_t>& neighbor_template_ids);

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_TEMPLATESELECTOR_H
