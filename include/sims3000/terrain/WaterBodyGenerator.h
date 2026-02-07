/**
 * @file WaterBodyGenerator.h
 * @brief Water body generation for terrain: oceans, rivers, and lakes.
 *
 * Places three water terrain types using elevation heightmap:
 * - DeepVoid (ocean): Along map edges below sea level within border width
 * - FlowChannel (river): From high elevation to ocean/edge via gradient descent
 * - StillBasin (lake): In terrain depressions surrounded by higher terrain
 *
 * After placement, assigns water body IDs via flood-fill, computes flow
 * directions for rivers, and updates derived data (coastal flags, underwater
 * flags, water distance field).
 *
 * Water types target ~15-20% of map area.
 *
 * @see ElevationGenerator.h for elevation heightmap
 * @see WaterData.h for water body ID and flow direction storage
 * @see WaterDistanceField.h for distance field computation
 */

#ifndef SIMS3000_TERRAIN_WATERBODYGENERATOR_H
#define SIMS3000_TERRAIN_WATERBODYGENERATOR_H

#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/WaterData.h>
#include <sims3000/terrain/WaterDistanceField.h>
#include <sims3000/terrain/ProceduralNoise.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace terrain {

/**
 * @struct WaterBodyConfig
 * @brief Configuration for water body generation.
 *
 * Controls placement of oceans, rivers, and lakes.
 * All values have sensible defaults that produce natural-looking water features.
 */
struct WaterBodyConfig {
    // =========================================================================
    // Ocean (DeepVoid) Configuration
    // =========================================================================

    /**
     * @brief Sea level elevation threshold (0-31).
     *
     * Tiles at or below this elevation within the ocean border are DeepVoid.
     * Default: 8 (matches DEFAULT_SEA_LEVEL from TerrainGrid.h)
     */
    std::uint8_t seaLevel = 8;

    /**
     * @brief Width of ocean border in tiles.
     *
     * DeepVoid is placed along map edges within this distance.
     * Larger values create more ocean around the map perimeter.
     * Default: 5 tiles
     */
    std::uint16_t oceanBorderWidth = 5;

    // =========================================================================
    // River (FlowChannel) Configuration
    // =========================================================================

    /**
     * @brief Minimum number of rivers to generate.
     *
     * Generator will attempt to create at least this many rivers.
     * Default: 1 (guaranteed at least one river per map)
     */
    std::uint8_t minRiverCount = 1;

    /**
     * @brief Maximum number of rivers to generate.
     *
     * Actual count depends on map size and available high-elevation sources.
     * Default: 4
     */
    std::uint8_t maxRiverCount = 4;

    /**
     * @brief Minimum elevation for river source points.
     *
     * Rivers start from tiles at or above this elevation.
     * Should be high enough to create meaningful gradient descent.
     * Default: 18
     */
    std::uint8_t riverSourceMinElevation = 18;

    /**
     * @brief River width in tiles.
     *
     * Rivers can widen by this many tiles on each side.
     * 1 = single tile river, 2 = up to 3 tiles wide, etc.
     * Default: 1
     */
    std::uint8_t riverWidth = 1;

    /**
     * @brief Probability of spawning a tributary (0.0 - 1.0).
     *
     * When carving a river, chance to branch off a side channel.
     * Higher values create more complex river networks.
     * Default: 0.15
     */
    float tributaryProbability = 0.15f;

    /**
     * @brief Minimum length for tributaries in tiles.
     *
     * Tributaries shorter than this are not created.
     * Default: 10
     */
    std::uint16_t minTributaryLength = 10;

    // =========================================================================
    // Lake (StillBasin) Configuration
    // =========================================================================

    /**
     * @brief Maximum number of lakes to generate.
     *
     * Actual count depends on terrain depressions found.
     * Default: 3
     */
    std::uint8_t maxLakeCount = 3;

    /**
     * @brief Minimum depression depth for lake placement.
     *
     * A depression must be at least this many elevation levels lower
     * than its surrounding rim to be considered for lake placement.
     * Default: 2
     */
    std::uint8_t minDepressionDepth = 2;

    /**
     * @brief Maximum lake radius in tiles.
     *
     * Lakes will not grow beyond this radius from their center.
     * Default: 8
     */
    std::uint8_t maxLakeRadius = 8;

    /**
     * @brief Fill lake up to rim elevation.
     *
     * When true, lakes fill the entire depression. When false,
     * only tiles at or below sea level become lakes.
     * Default: true
     */
    bool fillToRim = true;

    // =========================================================================
    // Water Coverage Target
    // =========================================================================

    /**
     * @brief Minimum water coverage as fraction of map (0.0 - 1.0).
     *
     * Generator will add water features until at least this coverage is reached.
     * Default: 0.15 (15%)
     */
    float minWaterCoverage = 0.15f;

    /**
     * @brief Maximum water coverage as fraction of map (0.0 - 1.0).
     *
     * Generator will stop adding water features after reaching this coverage.
     * Default: 0.20 (20%)
     */
    float maxWaterCoverage = 0.20f;

    // =========================================================================
    // Factory Methods
    // =========================================================================

    /**
     * @brief Default configuration for standard maps.
     */
    static WaterBodyConfig defaultConfig() {
        return WaterBodyConfig{};
    }

    /**
     * @brief Configuration for island maps (more ocean).
     */
    static WaterBodyConfig island() {
        WaterBodyConfig cfg;
        cfg.oceanBorderWidth = 12;
        cfg.seaLevel = 10;
        cfg.minWaterCoverage = 0.25f;
        cfg.maxWaterCoverage = 0.35f;
        return cfg;
    }

    /**
     * @brief Configuration for river-rich maps.
     */
    static WaterBodyConfig riverHeavy() {
        WaterBodyConfig cfg;
        cfg.minRiverCount = 3;
        cfg.maxRiverCount = 6;
        cfg.tributaryProbability = 0.25f;
        cfg.riverWidth = 2;
        cfg.maxLakeCount = 1;
        return cfg;
    }

    /**
     * @brief Configuration for lake-rich maps.
     */
    static WaterBodyConfig lakeHeavy() {
        WaterBodyConfig cfg;
        cfg.maxLakeCount = 6;
        cfg.maxLakeRadius = 12;
        cfg.minRiverCount = 1;
        cfg.maxRiverCount = 2;
        return cfg;
    }

    /**
     * @brief Configuration for arid/desert maps (minimal water).
     */
    static WaterBodyConfig arid() {
        WaterBodyConfig cfg;
        cfg.oceanBorderWidth = 3;
        cfg.minRiverCount = 1;
        cfg.maxRiverCount = 1;
        cfg.maxLakeCount = 0;
        cfg.minWaterCoverage = 0.05f;
        cfg.maxWaterCoverage = 0.10f;
        return cfg;
    }
};

// Verify WaterBodyConfig is trivially copyable for serialization
static_assert(std::is_trivially_copyable<WaterBodyConfig>::value,
    "WaterBodyConfig must be trivially copyable");

/**
 * @struct WaterBodyResult
 * @brief Statistics from water body generation.
 */
struct WaterBodyResult {
    std::uint32_t oceanTileCount;       ///< Number of DeepVoid tiles
    std::uint32_t riverTileCount;       ///< Number of FlowChannel tiles
    std::uint32_t lakeTileCount;        ///< Number of StillBasin tiles
    std::uint32_t totalWaterTiles;      ///< Total water tiles
    std::uint32_t totalTiles;           ///< Total tiles in grid
    float waterCoverage;                ///< Water coverage fraction (0.0 - 1.0)
    std::uint16_t waterBodyCount;       ///< Number of distinct water bodies
    std::uint8_t riverCount;            ///< Number of rivers generated
    std::uint8_t lakeCount;             ///< Number of lakes generated
    std::uint32_t coastalTileCount;     ///< Number of land tiles marked coastal
    float generationTimeMs;             ///< Time taken to generate (milliseconds)
};

/**
 * @class WaterBodyGenerator
 * @brief Generates water bodies using elevation heightmap.
 *
 * Places DeepVoid (ocean), FlowChannel (river), and StillBasin (lake)
 * terrain types, then computes water body IDs, flow directions,
 * underwater/coastal flags, and water distance field.
 *
 * Usage:
 * @code
 * TerrainGrid grid(MapSize::Medium);
 * ElevationGenerator::generate(grid, seed, elevConfig);
 *
 * WaterData waterData(MapSize::Medium);
 * WaterDistanceField distanceField(MapSize::Medium);
 * WaterBodyConfig config = WaterBodyConfig::defaultConfig();
 *
 * WaterBodyResult result = WaterBodyGenerator::generate(
 *     grid, waterData, distanceField, seed, config);
 * @endcode
 *
 * Thread Safety:
 * - generate() is NOT thread-safe (modifies grid, waterData, distanceField)
 *
 * @note Generation is single-threaded for deterministic RNG call order.
 */
class WaterBodyGenerator {
public:
    /**
     * @brief Generate all water bodies for the terrain.
     *
     * Performs in order:
     * 1. Place DeepVoid along map edges below sea level
     * 2. Generate FlowChannel rivers via gradient descent
     * 3. Place StillBasin lakes in terrain depressions
     * 4. Assign water body IDs via flood-fill
     * 5. Compute flow directions for river tiles
     * 6. Set is_underwater flag for all water tiles
     * 7. Set is_coastal flag for land tiles adjacent to water
     * 8. Compute water distance field
     *
     * @param grid The terrain grid (elevation and terrain types).
     * @param waterData Output water body IDs and flow directions.
     * @param distanceField Output water distance field.
     * @param seed Random seed for deterministic generation.
     * @param config Configuration parameters for generation.
     * @return Statistics about the generated water bodies.
     *
     * @note Modifies grid terrain types and flags in place.
     */
    static WaterBodyResult generate(
        TerrainGrid& grid,
        WaterData& waterData,
        WaterDistanceField& distanceField,
        std::uint64_t seed,
        const WaterBodyConfig& config = WaterBodyConfig::defaultConfig()
    );

    /**
     * @brief Place DeepVoid (ocean) tiles along map edges.
     *
     * Tiles within oceanBorderWidth of any edge that have elevation
     * at or below seaLevel are converted to DeepVoid.
     *
     * @param grid The terrain grid to modify.
     * @param config Configuration with seaLevel and oceanBorderWidth.
     * @return Number of tiles converted to DeepVoid.
     */
    static std::uint32_t placeOcean(
        TerrainGrid& grid,
        const WaterBodyConfig& config
    );

    /**
     * @brief Generate rivers via gradient descent from high points.
     *
     * Finds high-elevation source points and carves rivers downhill
     * toward ocean or map edge. Optionally creates tributaries.
     *
     * @param grid The terrain grid to modify.
     * @param waterData Output flow directions for river tiles.
     * @param rng Random number generator for source selection.
     * @param config Configuration with river parameters.
     * @return Number of rivers generated.
     */
    static std::uint8_t placeRivers(
        TerrainGrid& grid,
        WaterData& waterData,
        Xoshiro256& rng,
        const WaterBodyConfig& config
    );

    /**
     * @brief Place lakes in terrain depressions.
     *
     * Finds local minima in elevation surrounded by higher terrain
     * and fills them with StillBasin tiles.
     *
     * @param grid The terrain grid to modify.
     * @param rng Random number generator for lake selection.
     * @param config Configuration with lake parameters.
     * @return Number of lakes generated.
     */
    static std::uint8_t placeLakes(
        TerrainGrid& grid,
        Xoshiro256& rng,
        const WaterBodyConfig& config
    );

    /**
     * @brief Assign water body IDs via flood-fill.
     *
     * Each contiguous region of water tiles gets a unique ID.
     * Non-water tiles have ID 0 (NO_WATER_BODY).
     *
     * @param grid The terrain grid (read-only for terrain types).
     * @param waterData Output water body IDs.
     * @return Number of distinct water bodies found.
     */
    static std::uint16_t assignWaterBodyIDs(
        const TerrainGrid& grid,
        WaterData& waterData
    );

    /**
     * @brief Set is_underwater flag for all water tiles.
     *
     * @param grid The terrain grid to update flags.
     * @return Number of tiles with is_underwater set.
     */
    static std::uint32_t setUnderwaterFlags(TerrainGrid& grid);

    /**
     * @brief Set is_coastal flag for land tiles adjacent to water.
     *
     * A land tile is coastal if any of its 8 neighbors is water.
     *
     * @param grid The terrain grid to update flags.
     * @return Number of tiles with is_coastal set.
     */
    static std::uint32_t setCoastalFlags(TerrainGrid& grid);

private:
    /**
     * @brief Check if terrain type is water.
     */
    static bool isWater(TerrainType type);

    /**
     * @brief Get direction from source to lowest neighbor.
     *
     * Returns the FlowDirection toward the neighbor with lowest elevation.
     * Returns FlowDirection::None if no lower neighbor exists.
     */
    static FlowDirection getDownhillDirection(
        const TerrainGrid& grid,
        std::uint16_t x,
        std::uint16_t y
    );

    /**
     * @brief Carve a single river from source to destination.
     *
     * Follows gradient descent until reaching water, map edge,
     * or a tile that's already water.
     *
     * @param grid The terrain grid to modify.
     * @param waterData Output flow directions.
     * @param startX River source X coordinate.
     * @param startY River source Y coordinate.
     * @param rng Random number generator for tributaries.
     * @param config Configuration with river parameters.
     * @param depth Recursion depth (for tributaries).
     * @return Number of tiles carved.
     */
    static std::uint32_t carveRiver(
        TerrainGrid& grid,
        WaterData& waterData,
        std::uint16_t startX,
        std::uint16_t startY,
        Xoshiro256& rng,
        const WaterBodyConfig& config,
        int depth = 0
    );

    /**
     * @brief Find depression candidates for lake placement.
     *
     * Returns list of (x, y) coordinates that are local elevation minima.
     */
    static std::vector<std::pair<std::uint16_t, std::uint16_t>> findDepressions(
        const TerrainGrid& grid,
        const WaterBodyConfig& config
    );

    /**
     * @brief Fill a depression with lake tiles.
     *
     * @param grid The terrain grid to modify.
     * @param centerX Depression center X.
     * @param centerY Depression center Y.
     * @param config Configuration with lake parameters.
     * @return Number of tiles converted to StillBasin.
     */
    static std::uint32_t fillLake(
        TerrainGrid& grid,
        std::uint16_t centerX,
        std::uint16_t centerY,
        const WaterBodyConfig& config
    );

    /**
     * @brief Count current water tiles in grid.
     */
    static std::uint32_t countWaterTiles(const TerrainGrid& grid);

    /**
     * @brief Calculate current water coverage as fraction.
     */
    static float calculateWaterCoverage(const TerrainGrid& grid);
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_WATERBODYGENERATOR_H
