/**
 * @file TerrainEvents.h
 * @brief Terrain modification events and related types
 *
 * Defines:
 * - GridRect: Rectangular region of tiles specified by position and dimensions
 * - ModificationType: Enum of terrain modification categories
 * - TerrainModifiedEvent: Event carrying affected area and modification type
 *
 * These types support the terrain change notification system, enabling
 * RenderingSystem and other consumers to respond efficiently to terrain changes.
 *
 * @see ChunkDirtyTracker for dirty flag management
 */

#ifndef SIMS3000_TERRAIN_TERRAINEVENTS_H
#define SIMS3000_TERRAIN_TERRAINEVENTS_H

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace terrain {

/**
 * @struct GridRect
 * @brief Rectangular region of tiles on the terrain grid.
 *
 * Represents an axis-aligned rectangle in tile coordinates.
 * Used to specify affected regions in terrain modification events.
 *
 * The rectangle includes all tiles from (x, y) to (x + width - 1, y + height - 1).
 * A rectangle with width=1 and height=1 represents a single tile.
 */
struct GridRect {
    std::int16_t x = 0;       ///< Left edge X coordinate (inclusive)
    std::int16_t y = 0;       ///< Top edge Y coordinate (inclusive)
    std::uint16_t width = 0;  ///< Width in tiles (0 = empty rectangle)
    std::uint16_t height = 0; ///< Height in tiles (0 = empty rectangle)

    /**
     * @brief Check if this rectangle is empty (zero area).
     * @return true if width or height is zero.
     */
    bool isEmpty() const {
        return width == 0 || height == 0;
    }

    /**
     * @brief Check if a point is inside this rectangle.
     * @param px X coordinate to test.
     * @param py Y coordinate to test.
     * @return true if point is within the rectangle bounds.
     */
    bool contains(std::int16_t px, std::int16_t py) const {
        return px >= x && px < (x + static_cast<std::int16_t>(width)) &&
               py >= y && py < (y + static_cast<std::int16_t>(height));
    }

    /**
     * @brief Get the right edge X coordinate (exclusive).
     * @return X coordinate of the first column after the rectangle.
     */
    std::int16_t right() const {
        return static_cast<std::int16_t>(x + width);
    }

    /**
     * @brief Get the bottom edge Y coordinate (exclusive).
     * @return Y coordinate of the first row after the rectangle.
     */
    std::int16_t bottom() const {
        return static_cast<std::int16_t>(y + height);
    }

    /**
     * @brief Create a GridRect from corner coordinates.
     * @param x1 Left edge (inclusive).
     * @param y1 Top edge (inclusive).
     * @param x2 Right edge (exclusive).
     * @param y2 Bottom edge (exclusive).
     * @return GridRect covering the specified region.
     */
    static GridRect fromCorners(std::int16_t x1, std::int16_t y1,
                                 std::int16_t x2, std::int16_t y2) {
        GridRect rect;
        rect.x = x1;
        rect.y = y1;
        rect.width = static_cast<std::uint16_t>(x2 > x1 ? x2 - x1 : 0);
        rect.height = static_cast<std::uint16_t>(y2 > y1 ? y2 - y1 : 0);
        return rect;
    }

    /**
     * @brief Create a GridRect for a single tile.
     * @param tx Tile X coordinate.
     * @param ty Tile Y coordinate.
     * @return GridRect with width=1 and height=1 at the specified position.
     */
    static GridRect singleTile(std::int16_t tx, std::int16_t ty) {
        GridRect rect;
        rect.x = tx;
        rect.y = ty;
        rect.width = 1;
        rect.height = 1;
        return rect;
    }

    bool operator==(const GridRect& other) const {
        return x == other.x && y == other.y &&
               width == other.width && height == other.height;
    }

    bool operator!=(const GridRect& other) const {
        return !(*this == other);
    }
};

// Verify GridRect is trivially copyable for network serialization
static_assert(std::is_trivially_copyable<GridRect>::value,
    "GridRect must be trivially copyable");
static_assert(sizeof(GridRect) == 8, "GridRect should be 8 bytes");

/**
 * @enum ModificationType
 * @brief Categories of terrain modification.
 *
 * Used to indicate what kind of change occurred to the terrain,
 * allowing listeners to respond appropriately to different modification types.
 */
enum class ModificationType : std::uint8_t {
    Cleared = 0,        ///< Vegetation/obstacles cleared for building
    Leveled = 1,        ///< Terrain flattened (elevation changes)
    Terraformed = 2,    ///< Terrain type changed (e.g., land to water)
    Generated = 3,      ///< Initial terrain generation (new map)
    SeaLevelChanged = 4 ///< Global sea level adjustment
};

/// Total number of modification types
constexpr std::uint8_t MODIFICATION_TYPE_COUNT = 5;

/**
 * @brief Check if a modification type value is valid.
 * @param value The raw uint8 value to check.
 * @return true if value is a valid ModificationType.
 */
constexpr bool isValidModificationType(std::uint8_t value) {
    return value < MODIFICATION_TYPE_COUNT;
}

// Verify ModificationType size
static_assert(sizeof(ModificationType) == 1,
    "ModificationType must be 1 byte");

/**
 * @struct TerrainModifiedEvent
 * @brief Event fired when terrain is modified.
 *
 * Carries information about which tiles were affected and what type
 * of modification occurred. Event handlers (e.g., RenderingSystem)
 * can use this to update only the affected areas.
 *
 * This event is the primary mechanism for terrain change notification.
 * Systems subscribe to this event type to receive updates.
 *
 * @note When this event is processed, the ChunkDirtyTracker automatically
 *       marks all chunks overlapping the affected_area as dirty.
 */
struct TerrainModifiedEvent {
    GridRect affected_area;              ///< Tiles that were modified
    ModificationType modification_type;  ///< Category of modification
    std::uint8_t _padding[3] = {0, 0, 0}; ///< Alignment padding

    /**
     * @brief Default constructor.
     */
    TerrainModifiedEvent() = default;

    /**
     * @brief Construct an event with affected area and modification type.
     * @param area The rectangular region of affected tiles.
     * @param type The type of modification that occurred.
     */
    TerrainModifiedEvent(GridRect area, ModificationType type)
        : affected_area(area), modification_type(type) {}

    /**
     * @brief Convenience constructor for single-tile modification.
     * @param tile_x X coordinate of the modified tile.
     * @param tile_y Y coordinate of the modified tile.
     * @param type The type of modification.
     */
    TerrainModifiedEvent(std::int16_t tile_x, std::int16_t tile_y,
                          ModificationType type)
        : affected_area(GridRect::singleTile(tile_x, tile_y)),
          modification_type(type) {}
};

// Verify TerrainModifiedEvent is trivially copyable for network serialization
static_assert(std::is_trivially_copyable<TerrainModifiedEvent>::value,
    "TerrainModifiedEvent must be trivially copyable");
static_assert(sizeof(TerrainModifiedEvent) == 12,
    "TerrainModifiedEvent should be 12 bytes (8 + 1 + 3 padding)");

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINEVENTS_H
