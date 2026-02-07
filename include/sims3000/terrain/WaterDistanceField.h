/**
 * @file WaterDistanceField.h
 * @brief Pre-computed water distance field for O(1) proximity queries.
 *
 * Provides a dense uint8 grid storing the Manhattan distance from each tile
 * to the nearest water tile. Distance is capped at 255 tiles (uint8 max).
 * Computed via multi-source BFS from all water tiles simultaneously.
 *
 * This data enables O(1) water proximity queries for:
 * - FluidSystem: Water pump placement validation
 * - LandValueSystem: Waterfront property value boost
 * - PortSystem: Port and dock placement requirements
 *
 * Memory budget: 1 byte per tile
 * - 128x128: 16,384 bytes (16KB)
 * - 256x256: 65,536 bytes (64KB)
 * - 512x512: 262,144 bytes (256KB)
 *
 * Performance target: BFS completes in <5ms for 512x512 grid.
 *
 * @see TerrainGrid.h for the main terrain data grid
 * @see TerrainTypes.h for water terrain types (DeepVoid, FlowChannel, StillBasin)
 * @see /docs/canon/patterns.yaml (dense_grid_exception)
 */

#ifndef SIMS3000_TERRAIN_WATERDISTANCEFIELD_H
#define SIMS3000_TERRAIN_WATERDISTANCEFIELD_H

#include <sims3000/terrain/TerrainGrid.h>
#include <cstdint>
#include <vector>
#include <cassert>

namespace sims3000 {
namespace terrain {

/// Maximum distance value (uint8 max) - tiles farther than this are capped
constexpr std::uint8_t MAX_WATER_DISTANCE = 255;

/// Distance value for water tiles themselves
constexpr std::uint8_t WATER_TILE_DISTANCE = 0;

/**
 * @struct WaterDistanceField
 * @brief Dense 2D array storing pre-computed distance to nearest water tile.
 *
 * Row-major layout matching TerrainGrid: index = y * width + x
 * Memory: 1 byte per tile (distance = uint8)
 *
 * Distance semantics:
 * - 0 = Water tile (DeepVoid, FlowChannel, StillBasin)
 * - 1-254 = Manhattan distance to nearest water tile
 * - 255 = At least 255 tiles from any water (capped)
 *
 * Memory budget:
 * - 128x128: 16,384 bytes (16KB)
 * - 256x256: 65,536 bytes (64KB)
 * - 512x512: 262,144 bytes (256KB)
 */
struct WaterDistanceField {
    std::uint16_t width;   ///< Grid width in tiles (128, 256, or 512)
    std::uint16_t height;  ///< Grid height in tiles (128, 256, or 512)
    std::vector<std::uint8_t> distances;  ///< Dense storage (row-major)

    /**
     * @brief Default constructor creates an empty field.
     */
    WaterDistanceField()
        : width(0)
        , height(0)
        , distances()
    {}

    /**
     * @brief Construct a field with the specified dimensions.
     *
     * All tiles are initialized to MAX_WATER_DISTANCE.
     * Call compute() with terrain data to populate actual distances.
     *
     * @param map_size The size tier to use (Small/Medium/Large).
     */
    explicit WaterDistanceField(MapSize map_size)
        : width(static_cast<std::uint16_t>(map_size))
        , height(static_cast<std::uint16_t>(map_size))
        , distances(static_cast<std::size_t>(width) * height, MAX_WATER_DISTANCE)
    {}

    /**
     * @brief Construct a field with explicit width and height.
     *
     * @param w Grid width (must be 128, 256, or 512).
     * @param h Grid height (must be 128, 256, or 512).
     */
    WaterDistanceField(std::uint16_t w, std::uint16_t h)
        : width(w)
        , height(h)
        , distances()
    {
        assert(isValidMapSize(w) && "Width must be 128, 256, or 512");
        assert(isValidMapSize(h) && "Height must be 128, 256, or 512");
        assert(w == h && "Maps must be square");
        distances.resize(static_cast<std::size_t>(width) * height, MAX_WATER_DISTANCE);
    }

    /**
     * @brief Initialize or reinitialize the field to a specific size.
     *
     * All tiles are reset to MAX_WATER_DISTANCE.
     *
     * @param map_size The size tier to use.
     */
    void initialize(MapSize map_size) {
        width = static_cast<std::uint16_t>(map_size);
        height = static_cast<std::uint16_t>(map_size);
        distances.clear();
        distances.resize(static_cast<std::size_t>(width) * height, MAX_WATER_DISTANCE);
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
     * @brief Get water distance at (x, y).
     *
     * This is the primary O(1) query method.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Distance to nearest water tile (0 = is water, 255 = very far).
     */
    template<typename T, typename U>
    std::uint8_t get_water_distance(T x, U y) const {
        assert(in_bounds(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)) && "Coordinates out of bounds");
        return distances[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)];
    }

    /**
     * @brief Set water distance at (x, y).
     *
     * Internal use during computation. External code should not call this.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param distance Distance value to set.
     */
    template<typename T, typename U>
    void set_distance(T x, U y, std::uint8_t distance) {
        assert(in_bounds(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)) && "Coordinates out of bounds");
        distances[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)] = distance;
    }

    /**
     * @brief Calculate the linear index for a coordinate pair.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Linear index into the distances vector.
     */
    std::size_t index_of(std::uint16_t x, std::uint16_t y) const {
        return static_cast<std::size_t>(y) * width + x;
    }

    /**
     * @brief Get total number of tiles in the field.
     *
     * @return width * height
     */
    std::size_t tile_count() const {
        return static_cast<std::size_t>(width) * height;
    }

    /**
     * @brief Get memory size in bytes used by storage.
     *
     * @return Number of bytes used by the distances vector.
     */
    std::size_t memory_bytes() const {
        return distances.size() * sizeof(std::uint8_t);
    }

    /**
     * @brief Check if the field is empty (uninitialized).
     *
     * @return true if width or height is 0.
     */
    bool empty() const {
        return width == 0 || height == 0 || distances.empty();
    }

    /**
     * @brief Reset all distances to MAX_WATER_DISTANCE.
     *
     * Call this before recomputing distances.
     */
    void clear() {
        std::fill(distances.begin(), distances.end(), MAX_WATER_DISTANCE);
    }

    /**
     * @brief Compute water distances from terrain data using multi-source BFS.
     *
     * This is the main computation method. It performs a breadth-first search
     * starting from ALL water tiles simultaneously, computing the shortest
     * Manhattan distance to any water tile for each non-water tile.
     *
     * Water tiles (DeepVoid, FlowChannel, StillBasin) get distance 0.
     * Adjacent tiles get distance 1, and so on outward.
     * Distance is capped at 255.
     *
     * Performance: O(width * height) - visits each tile at most once.
     * Target: <5ms for 512x512 grid.
     *
     * @param terrain The terrain grid to compute distances from.
     */
    void compute(const TerrainGrid& terrain);

    /**
     * @brief Check if a tile is a water tile based on terrain type.
     *
     * Water types are: DeepVoid, FlowChannel, StillBasin.
     *
     * @param type The terrain type to check.
     * @return true if the terrain type is a water type.
     */
    static bool is_water_type(TerrainType type) {
        return type == TerrainType::DeepVoid ||
               type == TerrainType::FlowChannel ||
               type == TerrainType::StillBasin;
    }
};

// Verify distance type is exactly 1 byte
static_assert(sizeof(std::uint8_t) == 1,
    "Distance storage must be 1 byte per tile");

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_WATERDISTANCEFIELD_H
