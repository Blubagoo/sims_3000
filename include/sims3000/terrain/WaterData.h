/**
 * @file WaterData.h
 * @brief Water body identification and flow direction storage for TerrainSystem.
 *
 * Extends terrain data storage to track discrete water bodies and per-tile
 * river flow direction. Each contiguous water region gets a unique body ID
 * (computed during generation via flood-fill). River tiles store flow direction
 * as an 8-direction enum.
 *
 * This data enables:
 * - Single-mesh-per-body water rendering (all tiles with same body ID share mesh)
 * - Directional UV scrolling for FlowChannel tiles based on flow direction
 *
 * Memory budget: 3 bytes per tile
 * - WaterBodyID: 2 bytes (uint16)
 * - FlowDirection: 1 byte (uint8)
 * - 512x512 map: 262,144 tiles * 3 bytes = 786,432 bytes = 768KB
 *
 * @see TerrainGrid.h for the main terrain data grid
 * @see /docs/canon/patterns.yaml (dense_grid_exception)
 */

#ifndef SIMS3000_TERRAIN_WATERDATA_H
#define SIMS3000_TERRAIN_WATERDATA_H

#include <sims3000/terrain/TerrainGrid.h>
#include <cstdint>
#include <vector>
#include <cassert>

namespace sims3000 {
namespace terrain {

/**
 * @typedef WaterBodyID
 * @brief Unique identifier for a contiguous water body.
 *
 * Value 0 means "no water body" (tile is not part of any water body).
 * Values 1-65535 are valid water body IDs.
 *
 * Water body IDs are assigned during terrain generation via flood-fill
 * algorithm (see ticket 3-009). Each contiguous region of water tiles
 * (DeepVoid, FlowChannel, StillBasin) receives a unique ID.
 */
using WaterBodyID = std::uint16_t;

/// Sentinel value indicating tile is not part of any water body
constexpr WaterBodyID NO_WATER_BODY = 0;

/// Maximum valid water body ID
constexpr WaterBodyID MAX_WATER_BODY_ID = 65535;

/**
 * @enum FlowDirection
 * @brief 8-direction flow direction for river (FlowChannel) tiles.
 *
 * Direction indicates which way water flows FROM this tile.
 * Computed from elevation gradient descent during river placement.
 *
 * Values match standard 8-directional compass:
 * - N = North (up, -Y)
 * - NE = Northeast
 * - E = East (right, +X)
 * - SE = Southeast
 * - S = South (down, +Y)
 * - SW = Southwest
 * - W = West (left, -X)
 * - NW = Northwest
 * - None = No flow direction (for non-river tiles or still water)
 */
enum class FlowDirection : std::uint8_t {
    None = 0,  ///< No flow (still water or non-water tile)
    N = 1,     ///< North (up, -Y)
    NE = 2,    ///< Northeast (+X, -Y)
    E = 3,     ///< East (right, +X)
    SE = 4,    ///< Southeast (+X, +Y)
    S = 5,     ///< South (down, +Y)
    SW = 6,    ///< Southwest (-X, +Y)
    W = 7,     ///< West (left, -X)
    NW = 8     ///< Northwest (-X, -Y)
};

/// Total number of flow direction values (including None)
constexpr std::uint8_t FLOW_DIRECTION_COUNT = 9;

/**
 * @brief Check if a flow direction value is valid.
 * @param value The raw uint8 value to check.
 * @return true if value is a valid FlowDirection (0-8).
 */
constexpr bool isValidFlowDirection(std::uint8_t value) {
    return value < FLOW_DIRECTION_COUNT;
}

/**
 * @brief Get the X offset for a flow direction.
 * @param dir The flow direction.
 * @return -1, 0, or +1 for the X component of the direction.
 */
constexpr std::int8_t getFlowDirectionDX(FlowDirection dir) {
    switch (dir) {
        case FlowDirection::NE:
        case FlowDirection::E:
        case FlowDirection::SE:
            return 1;
        case FlowDirection::NW:
        case FlowDirection::W:
        case FlowDirection::SW:
            return -1;
        default:
            return 0;
    }
}

/**
 * @brief Get the Y offset for a flow direction.
 * @param dir The flow direction.
 * @return -1, 0, or +1 for the Y component of the direction.
 */
constexpr std::int8_t getFlowDirectionDY(FlowDirection dir) {
    switch (dir) {
        case FlowDirection::N:
        case FlowDirection::NE:
        case FlowDirection::NW:
            return -1;
        case FlowDirection::S:
        case FlowDirection::SE:
        case FlowDirection::SW:
            return 1;
        default:
            return 0;
    }
}

/**
 * @brief Get the opposite flow direction.
 * @param dir The flow direction.
 * @return The opposite direction (N<->S, E<->W, etc.), or None if dir is None.
 */
constexpr FlowDirection getOppositeDirection(FlowDirection dir) {
    switch (dir) {
        case FlowDirection::N:  return FlowDirection::S;
        case FlowDirection::NE: return FlowDirection::SW;
        case FlowDirection::E:  return FlowDirection::W;
        case FlowDirection::SE: return FlowDirection::NW;
        case FlowDirection::S:  return FlowDirection::N;
        case FlowDirection::SW: return FlowDirection::NE;
        case FlowDirection::W:  return FlowDirection::E;
        case FlowDirection::NW: return FlowDirection::SE;
        default:                return FlowDirection::None;
    }
}

/**
 * @struct WaterBodyGrid
 * @brief Dense 2D array storing water body IDs for all tiles.
 *
 * Row-major layout matching TerrainGrid: index = y * width + x
 * Memory: 2 bytes per tile (WaterBodyID = uint16)
 *
 * Memory budget:
 * - 128x128: 16,384 tiles * 2 bytes = 32,768 bytes (32KB)
 * - 256x256: 65,536 tiles * 2 bytes = 131,072 bytes (128KB)
 * - 512x512: 262,144 tiles * 2 bytes = 524,288 bytes (512KB)
 */
struct WaterBodyGrid {
    std::uint16_t width;   ///< Grid width in tiles (128, 256, or 512)
    std::uint16_t height;  ///< Grid height in tiles (128, 256, or 512)
    std::vector<WaterBodyID> body_ids;  ///< Dense storage (row-major)

    /**
     * @brief Default constructor creates an empty grid.
     */
    WaterBodyGrid()
        : width(0)
        , height(0)
        , body_ids()
    {}

    /**
     * @brief Construct a grid with the specified dimensions.
     *
     * All tiles are initialized to NO_WATER_BODY (0).
     *
     * @param map_size The size tier to use (Small/Medium/Large).
     */
    explicit WaterBodyGrid(MapSize map_size)
        : width(static_cast<std::uint16_t>(map_size))
        , height(static_cast<std::uint16_t>(map_size))
        , body_ids(static_cast<std::size_t>(width) * height, NO_WATER_BODY)
    {}

    /**
     * @brief Construct a grid with explicit width and height.
     *
     * @param w Grid width (must be 128, 256, or 512).
     * @param h Grid height (must be 128, 256, or 512).
     */
    WaterBodyGrid(std::uint16_t w, std::uint16_t h)
        : width(w)
        , height(h)
        , body_ids()
    {
        assert(isValidMapSize(w) && "Width must be 128, 256, or 512");
        assert(isValidMapSize(h) && "Height must be 128, 256, or 512");
        assert(w == h && "Maps must be square");
        body_ids.resize(static_cast<std::size_t>(width) * height, NO_WATER_BODY);
    }

    /**
     * @brief Initialize or reinitialize the grid to a specific size.
     *
     * All tiles are reset to NO_WATER_BODY.
     *
     * @param map_size The size tier to use.
     */
    void initialize(MapSize map_size) {
        width = static_cast<std::uint16_t>(map_size);
        height = static_cast<std::uint16_t>(map_size);
        body_ids.clear();
        body_ids.resize(static_cast<std::size_t>(width) * height, NO_WATER_BODY);
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
     * @brief Get water body ID at (x, y).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Water body ID at the specified location.
     */
    template<typename T, typename U>
    WaterBodyID get(T x, U y) const {
        assert(in_bounds(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)) && "Coordinates out of bounds");
        return body_ids[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)];
    }

    /**
     * @brief Set water body ID at (x, y).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param id Water body ID to set.
     */
    template<typename T, typename U>
    void set(T x, U y, WaterBodyID id) {
        assert(in_bounds(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)) && "Coordinates out of bounds");
        body_ids[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)] = id;
    }

    /**
     * @brief Calculate the linear index for a coordinate pair.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Linear index into the body_ids vector.
     */
    std::size_t index_of(std::uint16_t x, std::uint16_t y) const {
        return static_cast<std::size_t>(y) * width + x;
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
     * @brief Get memory size in bytes used by storage.
     *
     * @return Number of bytes used by the body_ids vector.
     */
    std::size_t memory_bytes() const {
        return body_ids.size() * sizeof(WaterBodyID);
    }

    /**
     * @brief Check if the grid is empty (uninitialized).
     *
     * @return true if width or height is 0.
     */
    bool empty() const {
        return width == 0 || height == 0 || body_ids.empty();
    }

    /**
     * @brief Clear all water body assignments.
     *
     * Sets all tiles to NO_WATER_BODY.
     */
    void clear() {
        std::fill(body_ids.begin(), body_ids.end(), NO_WATER_BODY);
    }
};

/**
 * @struct FlowDirectionGrid
 * @brief Dense 2D array storing flow directions for all tiles.
 *
 * Row-major layout matching TerrainGrid: index = y * width + x
 * Memory: 1 byte per tile (FlowDirection = uint8)
 *
 * Note: Flow direction is only meaningful for FlowChannel (river) tiles.
 * Non-river tiles should have FlowDirection::None.
 *
 * Memory budget:
 * - 128x128: 16,384 tiles * 1 byte = 16,384 bytes (16KB)
 * - 256x256: 65,536 tiles * 1 byte = 65,536 bytes (64KB)
 * - 512x512: 262,144 tiles * 1 byte = 262,144 bytes (256KB)
 */
struct FlowDirectionGrid {
    std::uint16_t width;   ///< Grid width in tiles (128, 256, or 512)
    std::uint16_t height;  ///< Grid height in tiles (128, 256, or 512)
    std::vector<FlowDirection> directions;  ///< Dense storage (row-major)

    /**
     * @brief Default constructor creates an empty grid.
     */
    FlowDirectionGrid()
        : width(0)
        , height(0)
        , directions()
    {}

    /**
     * @brief Construct a grid with the specified dimensions.
     *
     * All tiles are initialized to FlowDirection::None.
     *
     * @param map_size The size tier to use (Small/Medium/Large).
     */
    explicit FlowDirectionGrid(MapSize map_size)
        : width(static_cast<std::uint16_t>(map_size))
        , height(static_cast<std::uint16_t>(map_size))
        , directions(static_cast<std::size_t>(width) * height, FlowDirection::None)
    {}

    /**
     * @brief Construct a grid with explicit width and height.
     *
     * @param w Grid width (must be 128, 256, or 512).
     * @param h Grid height (must be 128, 256, or 512).
     */
    FlowDirectionGrid(std::uint16_t w, std::uint16_t h)
        : width(w)
        , height(h)
        , directions()
    {
        assert(isValidMapSize(w) && "Width must be 128, 256, or 512");
        assert(isValidMapSize(h) && "Height must be 128, 256, or 512");
        assert(w == h && "Maps must be square");
        directions.resize(static_cast<std::size_t>(width) * height, FlowDirection::None);
    }

    /**
     * @brief Initialize or reinitialize the grid to a specific size.
     *
     * All tiles are reset to FlowDirection::None.
     *
     * @param map_size The size tier to use.
     */
    void initialize(MapSize map_size) {
        width = static_cast<std::uint16_t>(map_size);
        height = static_cast<std::uint16_t>(map_size);
        directions.clear();
        directions.resize(static_cast<std::size_t>(width) * height, FlowDirection::None);
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
     * @brief Get flow direction at (x, y).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Flow direction at the specified location.
     */
    template<typename T, typename U>
    FlowDirection get(T x, U y) const {
        assert(in_bounds(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)) && "Coordinates out of bounds");
        return directions[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)];
    }

    /**
     * @brief Set flow direction at (x, y).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param dir Flow direction to set.
     */
    template<typename T, typename U>
    void set(T x, U y, FlowDirection dir) {
        assert(in_bounds(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)) && "Coordinates out of bounds");
        directions[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)] = dir;
    }

    /**
     * @brief Calculate the linear index for a coordinate pair.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Linear index into the directions vector.
     */
    std::size_t index_of(std::uint16_t x, std::uint16_t y) const {
        return static_cast<std::size_t>(y) * width + x;
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
     * @brief Get memory size in bytes used by storage.
     *
     * @return Number of bytes used by the directions vector.
     */
    std::size_t memory_bytes() const {
        return directions.size() * sizeof(FlowDirection);
    }

    /**
     * @brief Check if the grid is empty (uninitialized).
     *
     * @return true if width or height is 0.
     */
    bool empty() const {
        return width == 0 || height == 0 || directions.empty();
    }

    /**
     * @brief Clear all flow direction assignments.
     *
     * Sets all tiles to FlowDirection::None.
     */
    void clear() {
        std::fill(directions.begin(), directions.end(), FlowDirection::None);
    }
};

/**
 * @struct WaterData
 * @brief Combined water body and flow direction data for the terrain.
 *
 * This struct bundles the water body grid and flow direction grid together
 * for convenient access and consistent initialization.
 *
 * Combined memory budget: 3 bytes per tile
 * - 128x128: 48KB (32KB body + 16KB flow)
 * - 256x256: 192KB (128KB body + 64KB flow)
 * - 512x512: 768KB (512KB body + 256KB flow)
 */
struct WaterData {
    WaterBodyGrid water_body_ids;  ///< Water body ID for each tile
    FlowDirectionGrid flow_directions;  ///< Flow direction for each tile

    /**
     * @brief Default constructor creates empty grids.
     */
    WaterData() = default;

    /**
     * @brief Construct with the specified map size.
     *
     * @param map_size The size tier to use (Small/Medium/Large).
     */
    explicit WaterData(MapSize map_size)
        : water_body_ids(map_size)
        , flow_directions(map_size)
    {}

    /**
     * @brief Initialize or reinitialize to a specific size.
     *
     * @param map_size The size tier to use.
     */
    void initialize(MapSize map_size) {
        water_body_ids.initialize(map_size);
        flow_directions.initialize(map_size);
    }

    /**
     * @brief Get water body ID at (x, y).
     *
     * Convenience wrapper for water_body_ids.get().
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Water body ID at the specified location.
     */
    template<typename T, typename U>
    WaterBodyID get_water_body_id(T x, U y) const {
        return water_body_ids.get(x, y);
    }

    /**
     * @brief Get flow direction at (x, y).
     *
     * Convenience wrapper for flow_directions.get().
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Flow direction at the specified location.
     */
    template<typename T, typename U>
    FlowDirection get_flow_direction(T x, U y) const {
        return flow_directions.get(x, y);
    }

    /**
     * @brief Set water body ID at (x, y).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param id Water body ID to set.
     */
    template<typename T, typename U>
    void set_water_body_id(T x, U y, WaterBodyID id) {
        water_body_ids.set(x, y, id);
    }

    /**
     * @brief Set flow direction at (x, y).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param dir Flow direction to set.
     */
    template<typename T, typename U>
    void set_flow_direction(T x, U y, FlowDirection dir) {
        flow_directions.set(x, y, dir);
    }

    /**
     * @brief Check if coordinates are within bounds.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if (x, y) is within bounds.
     */
    bool in_bounds(std::int32_t x, std::int32_t y) const {
        return water_body_ids.in_bounds(x, y);
    }

    /**
     * @brief Get total memory usage in bytes.
     *
     * @return Combined memory usage of both grids.
     */
    std::size_t memory_bytes() const {
        return water_body_ids.memory_bytes() + flow_directions.memory_bytes();
    }

    /**
     * @brief Check if water data is empty (uninitialized).
     *
     * @return true if either grid is empty.
     */
    bool empty() const {
        return water_body_ids.empty() || flow_directions.empty();
    }

    /**
     * @brief Clear all water data.
     *
     * Resets all body IDs to NO_WATER_BODY and all directions to None.
     */
    void clear() {
        water_body_ids.clear();
        flow_directions.clear();
    }
};

// Verify FlowDirection is exactly 1 byte
static_assert(sizeof(FlowDirection) == 1,
    "FlowDirection must be 1 byte");

// Verify WaterBodyID is exactly 2 bytes
static_assert(sizeof(WaterBodyID) == 2,
    "WaterBodyID must be 2 bytes");

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_WATERDATA_H
