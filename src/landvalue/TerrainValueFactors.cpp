/**
 * @file TerrainValueFactors.cpp
 * @brief Implementation of terrain-based land value bonus calculations.
 *
 * @see TerrainValueFactors.h for interface documentation.
 * @see E10-101
 */

#include <sims3000/landvalue/TerrainValueFactors.h>
#include <algorithm>

namespace sims3000 {
namespace landvalue {

// Terrain type enum values (from terrain::TerrainType)
// BiolumeGrove = 5 (forest), PrismaFields = 6 (crystal),
// SporeFlats = 7 (spore), BlightMires = 8 (toxic)
namespace {
    constexpr uint8_t TERRAIN_BIOLUME_GROVE = 5;
    constexpr uint8_t TERRAIN_PRISMA_FIELDS = 6;
    constexpr uint8_t TERRAIN_SPORE_FLATS   = 7;
    constexpr uint8_t TERRAIN_BLIGHT_MIRES  = 8;
} // anonymous namespace

int8_t calculate_terrain_bonus(uint8_t terrain_type, uint8_t water_distance) {
    // Base bonus from terrain type
    int base_bonus = 0;
    switch (terrain_type) {
        case TERRAIN_PRISMA_FIELDS:
            base_bonus = terrain_bonus::CRYSTAL_FIELDS;  // +25
            break;
        case TERRAIN_SPORE_FLATS:
            base_bonus = terrain_bonus::SPORE_PLAINS;    // +15
            break;
        case TERRAIN_BIOLUME_GROVE:
            base_bonus = terrain_bonus::FOREST;           // +10
            break;
        case TERRAIN_BLIGHT_MIRES:
            base_bonus = terrain_bonus::TOXIC_MARSHES;    // -30
            break;
        default:
            base_bonus = 0;
            break;
    }

    // Water proximity bonus
    int water_bonus = 0;
    if (water_distance <= 1) {
        water_bonus = terrain_bonus::WATER_ADJACENT;  // +30
    } else if (water_distance == 2) {
        water_bonus = terrain_bonus::WATER_1_TILE;    // +20
    } else if (water_distance == 3) {
        water_bonus = terrain_bonus::WATER_2_TILES;   // +10
    }

    // Sum and clamp to int8_t range
    int total = base_bonus + water_bonus;
    total = std::max(-128, std::min(127, total));
    return static_cast<int8_t>(total);
}

void apply_terrain_bonuses(LandValueGrid& grid,
                           const std::vector<TerrainTileInfo>& terrain_info) {
    for (const auto& tile : terrain_info) {
        int8_t bonus = calculate_terrain_bonus(tile.terrain_type, tile.water_distance);

        // Store positive bonuses in terrain_bonus cache (negative stored as 0)
        uint8_t stored_bonus = (bonus > 0)
            ? static_cast<uint8_t>(bonus)
            : static_cast<uint8_t>(0);
        grid.set_terrain_bonus(tile.x, tile.y, stored_bonus);

        // Add bonus to current value with clamping to 0-255
        int current = static_cast<int>(grid.get_value(tile.x, tile.y));
        int new_value = current + static_cast<int>(bonus);
        new_value = std::max(0, std::min(255, new_value));
        grid.set_value(tile.x, tile.y, static_cast<uint8_t>(new_value));
    }
}

} // namespace landvalue
} // namespace sims3000
