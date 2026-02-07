/**
 * @file TerrainGrid.h
 * @brief Dense 2D array storage for terrain tile data.
 *
 * TerrainGrid provides O(1) coordinate-to-tile access for terrain data.
 * Uses row-major storage (x varies fastest within a row) for optimal
 * cache performance during row-by-row iteration.
 *
 * Supported map sizes:
 * - 128x128: 64KB memory budget (16,384 tiles)
 * - 256x256: 256KB memory budget (65,536 tiles)
 * - 512x512: 1MB memory budget (262,144 tiles)
 *
 * This is a canonical exception to the ECS-everywhere principle.
 * Dense grids preserve ECS separation of concerns:
 * - Data: Pure data structs (TerrainComponent)
 * - Logic: Stateless system operations (TerrainSystem)
 * - Identity: Grid coordinates serve as implicit entity IDs
 *
 * Implements ISerializable for binary serialization with version field.
 * For full terrain serialization including WaterData, use TerrainGridSerializer.
 *
 * @see /docs/canon/patterns.yaml (dense_grid_exception)
 * @see /plans/decisions/epic-3-terrain-dense-grid.md
 */

#ifndef SIMS3000_TERRAIN_TERRAINGRID_H
#define SIMS3000_TERRAIN_TERRAINGRID_H

#include <sims3000/terrain/TerrainTypes.h>
#include <sims3000/core/Serialization.h>
#include <cstdint>
#include <vector>
#include <cassert>

namespace sims3000 {
namespace terrain {

/**
 * @brief Valid map size dimensions.
 *
 * Maps are always square. These are the canonical size tiers
 * as defined in patterns.yaml.
 */
enum class MapSize : std::uint16_t {
    Small = 128,   ///< 128x128 tiles, 64KB, recommended 1-2 players
    Medium = 256,  ///< 256x256 tiles, 256KB, recommended 2-3 players
    Large = 512    ///< 512x512 tiles, 1MB, recommended 2-4 players
};

/**
 * @brief Default sea level for terrain (0-31 elevation range).
 *
 * Tiles at or below this elevation are considered underwater.
 * Default value of 8 allows for varied underwater topography
 * while leaving 23 levels for above-water terrain.
 */
constexpr std::uint8_t DEFAULT_SEA_LEVEL = 8;

/**
 * @brief Check if a dimension value is a valid map size.
 * @param dimension The width or height to check.
 * @return true if dimension is 128, 256, or 512.
 */
constexpr bool isValidMapSize(std::uint16_t dimension) {
    return dimension == 128 || dimension == 256 || dimension == 512;
}

/**
 * @brief Current terrain grid serialization format version.
 *
 * Used by ISerializable implementation for backwards compatibility.
 * Increment when format changes.
 * Version history:
 * - v1: Initial format (header + tiles)
 */
constexpr std::uint16_t TERRAIN_GRID_VERSION = 1;

/**
 * @struct TerrainGrid
 * @brief Dense 2D array storing TerrainComponent data for all tiles.
 *
 * Implements ISerializable interface for binary serialization.
 *
 * Row-major layout: index = y * width + x
 * This layout is optimal for:
 * - Row-by-row terrain generation
 * - Horizontal scanline operations
 * - Cache-friendly iteration patterns
 *
 * Memory budget at 4 bytes per tile:
 * - 128x128: 16,384 tiles * 4 bytes = 65,536 bytes (64KB)
 * - 256x256: 65,536 tiles * 4 bytes = 262,144 bytes (256KB)
 * - 512x512: 262,144 tiles * 4 bytes = 1,048,576 bytes (1MB)
 */
struct TerrainGrid : public ISerializable {
    std::uint16_t width;      ///< Grid width in tiles (128, 256, or 512)
    std::uint16_t height;     ///< Grid height in tiles (128, 256, or 512)
    std::uint8_t sea_level;   ///< Sea level elevation (default: 8)
    std::vector<TerrainComponent> tiles;  ///< Dense tile storage (row-major)

    /**
     * @brief Default constructor creates an empty grid.
     *
     * Call initialize() to allocate storage for a specific map size.
     */
    TerrainGrid()
        : width(0)
        , height(0)
        , sea_level(DEFAULT_SEA_LEVEL)
        , tiles()
    {}

    /**
     * @brief Construct a grid with the specified dimensions.
     *
     * @param map_size The size tier to use (Small/Medium/Large).
     * @param initial_sea_level Optional custom sea level (default: 8).
     */
    explicit TerrainGrid(MapSize map_size, std::uint8_t initial_sea_level = DEFAULT_SEA_LEVEL)
        : width(static_cast<std::uint16_t>(map_size))
        , height(static_cast<std::uint16_t>(map_size))
        , sea_level(initial_sea_level)
        , tiles(static_cast<std::size_t>(width) * height)
    {}

    /**
     * @brief Construct a grid with explicit width and height.
     *
     * @param w Grid width (must be 128, 256, or 512).
     * @param h Grid height (must be 128, 256, or 512).
     * @param initial_sea_level Optional custom sea level (default: 8).
     *
     * @note Width must equal height (square maps only).
     *       In debug builds, asserts if dimensions are invalid.
     */
    TerrainGrid(std::uint16_t w, std::uint16_t h, std::uint8_t initial_sea_level = DEFAULT_SEA_LEVEL)
        : width(w)
        , height(h)
        , sea_level(initial_sea_level)
        , tiles()
    {
        assert(isValidMapSize(w) && "Width must be 128, 256, or 512");
        assert(isValidMapSize(h) && "Height must be 128, 256, or 512");
        assert(w == h && "Maps must be square");
        tiles.resize(static_cast<std::size_t>(width) * height);
    }

    /**
     * @brief Initialize or reinitialize the grid to a specific size.
     *
     * Clears any existing data and allocates fresh storage.
     * All tiles are default-initialized (zero-filled).
     *
     * @param map_size The size tier to use.
     * @param new_sea_level Optional new sea level (default: 8).
     */
    void initialize(MapSize map_size, std::uint8_t new_sea_level = DEFAULT_SEA_LEVEL) {
        width = static_cast<std::uint16_t>(map_size);
        height = static_cast<std::uint16_t>(map_size);
        sea_level = new_sea_level;
        tiles.clear();
        tiles.resize(static_cast<std::size_t>(width) * height);
    }

    /**
     * @brief Check if coordinates are within grid bounds.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if (x, y) is within [0, width) x [0, height).
     */
    bool in_bounds(std::int32_t x, std::int32_t y) const {
        return x >= 0 && x < static_cast<std::int32_t>(width) &&
               y >= 0 && y < static_cast<std::int32_t>(height);
    }

    /**
     * @brief Access tile at (x, y) with bounds checking in debug builds.
     *
     * Row-major indexing: index = y * width + x
     *
     * Accepts any integer type for coordinates. Negative or out-of-bounds
     * coordinates will trigger an assertion in debug builds.
     *
     * @tparam T Integer type for x coordinate.
     * @tparam U Integer type for y coordinate.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Reference to the TerrainComponent at (x, y).
     *
     * @note In debug builds, asserts if coordinates are out of bounds.
     *       In release builds, behavior is undefined for out-of-bounds access.
     */
    template<typename T, typename U>
    TerrainComponent& at(T x, U y) {
        assert(in_bounds(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)) && "Coordinates out of bounds");
        return tiles[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)];
    }

    /**
     * @brief Access tile at (x, y) with bounds checking in debug builds (const version).
     *
     * @tparam T Integer type for x coordinate.
     * @tparam U Integer type for y coordinate.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Const reference to the TerrainComponent at (x, y).
     */
    template<typename T, typename U>
    const TerrainComponent& at(T x, U y) const {
        assert(in_bounds(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)) && "Coordinates out of bounds");
        return tiles[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)];
    }

    /**
     * @brief Calculate the linear index for a coordinate pair.
     *
     * Row-major: index = y * width + x
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Linear index into the tiles vector.
     *
     * @note Does NOT perform bounds checking.
     */
    std::size_t index_of(std::uint16_t x, std::uint16_t y) const {
        return static_cast<std::size_t>(y) * width + x;
    }

    /**
     * @brief Convert a linear index back to coordinates.
     *
     * @param index Linear index into the tiles vector.
     * @param out_x Output: X coordinate (column).
     * @param out_y Output: Y coordinate (row).
     *
     * @note Does NOT perform bounds checking.
     */
    void coords_of(std::size_t index, std::uint16_t& out_x, std::uint16_t& out_y) const {
        out_y = static_cast<std::uint16_t>(index / width);
        out_x = static_cast<std::uint16_t>(index % width);
    }

    /**
     * @brief Get total number of tiles in the grid.
     *
     * @return width * height
     */
    std::size_t tile_count() const {
        return static_cast<std::size_t>(width) * height;
    }

    /**
     * @brief Get memory size in bytes used by tile storage.
     *
     * @return Number of bytes used by the tiles vector.
     */
    std::size_t memory_bytes() const {
        return tiles.size() * sizeof(TerrainComponent);
    }

    /**
     * @brief Check if the grid is empty (uninitialized).
     *
     * @return true if width or height is 0.
     */
    bool empty() const {
        return width == 0 || height == 0 || tiles.empty();
    }

    /**
     * @brief Fill all tiles with a specific terrain component value.
     *
     * Useful for initializing terrain to a base type.
     *
     * @param value The TerrainComponent to copy to all tiles.
     */
    void fill(const TerrainComponent& value) {
        std::fill(tiles.begin(), tiles.end(), value);
    }

    /**
     * @brief Fill all tiles with a specific terrain type.
     *
     * Sets terrain type while zeroing other fields.
     *
     * @param type The TerrainType to set for all tiles.
     */
    void fill_type(TerrainType type) {
        TerrainComponent tc = {};
        tc.setTerrainType(type);
        fill(tc);
    }

    // =========================================================================
    // ISerializable implementation
    // =========================================================================

    /**
     * @brief Serialize the terrain grid to a binary buffer.
     *
     * Binary format (little-endian):
     * - Header (6 bytes):
     *   - version: uint16 (format version for backwards compatibility)
     *   - width: uint16 (128, 256, or 512)
     *   - height: uint16 (128, 256, or 512)
     * - Metadata (1 byte):
     *   - sea_level: uint8
     * - Tile data (width * height * 4 bytes):
     *   - TerrainComponent: 4 bytes per tile (terrain_type, elevation, moisture, flags)
     *
     * @param buffer The WriteBuffer to serialize into.
     */
    void serialize(WriteBuffer& buffer) const override {
        // Write header with version
        buffer.writeU16(TERRAIN_GRID_VERSION);
        buffer.writeU16(width);
        buffer.writeU16(height);
        buffer.writeU8(sea_level);

        // Write tiles in row-major order
        for (const auto& tile : tiles) {
            buffer.writeU8(tile.terrain_type);
            buffer.writeU8(tile.elevation);
            buffer.writeU8(tile.moisture);
            buffer.writeU8(tile.flags);
        }
    }

    /**
     * @brief Deserialize the terrain grid from a binary buffer.
     *
     * Reads the header, validates version and dimensions, then loads tile data.
     * The grid is resized to match the dimensions in the header.
     *
     * @param buffer The ReadBuffer to deserialize from.
     *
     * @note If deserialization fails (invalid version, dimensions, or insufficient data),
     *       the grid state is undefined. Callers should check buffer.hasMore() and
     *       validate the result.
     */
    void deserialize(ReadBuffer& buffer) override {
        // Read header
        std::uint16_t version = buffer.readU16();
        std::uint16_t w = buffer.readU16();
        std::uint16_t h = buffer.readU16();
        std::uint8_t sl = buffer.readU8();

        // Validate version (only accept version 1 for now)
        if (version != TERRAIN_GRID_VERSION) {
            // Invalid version - leave grid in empty state
            width = 0;
            height = 0;
            tiles.clear();
            return;
        }

        // Validate dimensions
        if (!isValidMapSize(w) || !isValidMapSize(h) || w != h) {
            // Invalid dimensions - leave grid in empty state
            width = 0;
            height = 0;
            tiles.clear();
            return;
        }

        // Initialize grid with dimensions from header
        width = w;
        height = h;
        sea_level = sl;
        tiles.resize(static_cast<std::size_t>(width) * height);

        // Read tiles
        const std::size_t tileCount = tile_count();
        for (std::size_t i = 0; i < tileCount; ++i) {
            tiles[i].terrain_type = buffer.readU8();
            tiles[i].elevation = buffer.readU8();
            tiles[i].moisture = buffer.readU8();
            tiles[i].flags = buffer.readU8();
        }
    }

    /**
     * @brief Get the format version written by serialize().
     *
     * @return The current serialization format version (TERRAIN_GRID_VERSION).
     */
    static constexpr std::uint16_t getFormatVersion() {
        return TERRAIN_GRID_VERSION;
    }
};

// Note: TerrainGrid is NOT standard layout due to std::vector member.
// Serialization must be done explicitly (not via memcpy).

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINGRID_H
