/**
 * @file types.h
 * @brief Core type definitions for Sims 3000
 *
 * Defines fundamental types used throughout the codebase:
 * - EntityID: ECS entity identifier
 * - PlayerID: Multiplayer player identifier
 * - GridPosition: Tile-based position on the game grid
 * - Credits: In-game currency
 * - SimulationTick: Discrete simulation time unit
 */

#ifndef SIMS3000_CORE_TYPES_H
#define SIMS3000_CORE_TYPES_H

#include <cstdint>
#include <functional>

namespace sims3000 {

/// Entity identifier for ECS. Matches EnTT's default entity type.
using EntityID = std::uint32_t;

/// Player identifier for multiplayer. Supports up to 255 players (0 = no owner).
using PlayerID = std::uint8_t;

/// In-game currency. Signed to allow debt/negative values.
using Credits = std::int64_t;

/// Discrete simulation time unit. Ticks at 20 Hz (50ms per tick).
using SimulationTick = std::uint64_t;

/**
 * @enum MapSizeTier
 * @brief Map size configuration tiers.
 *
 * Determines grid dimensions and expected entity counts:
 * - Small:  128x128 (16,384 tiles) - fast startup, lighter resource use
 * - Medium: 256x256 (65,536 tiles) - balanced (default)
 * - Large:  512x512 (262,144 tiles) - maximum city size
 *
 * Use MapSizeTier::getDimensions() to get the actual width/height.
 */
enum class MapSizeTier : std::uint8_t {
    Small = 0,   ///< 128x128 grid
    Medium = 1,  ///< 256x256 grid (default)
    Large = 2    ///< 512x512 grid
};

/**
 * @brief Get map dimensions for a given tier.
 *
 * @param tier The map size tier.
 * @param width Output: grid width in tiles.
 * @param height Output: grid height in tiles.
 */
inline void getMapDimensionsForTier(MapSizeTier tier,
                                    std::uint16_t& width,
                                    std::uint16_t& height) {
    switch (tier) {
        case MapSizeTier::Small:
            width = 128;
            height = 128;
            break;
        case MapSizeTier::Medium:
            width = 256;
            height = 256;
            break;
        case MapSizeTier::Large:
            width = 512;
            height = 512;
            break;
        default:
            width = 256;
            height = 256;
            break;
    }
}

/**
 * @struct GridPosition
 * @brief Tile-based position on the game grid.
 *
 * Uses signed 16-bit integers to allow negative coordinates
 * for potential map expansion or centered coordinate systems.
 * Range: [-32768, 32767] per axis.
 */
struct GridPosition {
    std::int16_t x = 0;
    std::int16_t y = 0;

    bool operator==(const GridPosition& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const GridPosition& other) const {
        return !(*this == other);
    }

    GridPosition operator+(const GridPosition& other) const {
        return {static_cast<std::int16_t>(x + other.x),
                static_cast<std::int16_t>(y + other.y)};
    }

    GridPosition operator-(const GridPosition& other) const {
        return {static_cast<std::int16_t>(x - other.x),
                static_cast<std::int16_t>(y - other.y)};
    }
};

static_assert(sizeof(EntityID) == 4, "EntityID must be 4 bytes");
static_assert(sizeof(PlayerID) == 1, "PlayerID must be 1 byte");
static_assert(sizeof(Credits) == 8, "Credits must be 8 bytes");
static_assert(sizeof(SimulationTick) == 8, "SimulationTick must be 8 bytes");
static_assert(sizeof(GridPosition) == 4, "GridPosition must be 4 bytes");
static_assert(sizeof(MapSizeTier) == 1, "MapSizeTier must be 1 byte");

} // namespace sims3000

// Hash specialization for GridPosition (for use in unordered containers)
namespace std {
template <>
struct hash<sims3000::GridPosition> {
    std::size_t operator()(const sims3000::GridPosition& pos) const noexcept {
        // Combine x and y into a single hash
        return std::hash<std::int32_t>{}(
            (static_cast<std::int32_t>(pos.x) << 16) |
            (static_cast<std::int32_t>(pos.y) & 0xFFFF)
        );
    }
};
} // namespace std

#endif // SIMS3000_CORE_TYPES_H
