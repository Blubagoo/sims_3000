/**
 * @file SpawnPointGenerator.h
 * @brief Multiplayer spawn point selection with terrain value scoring for fairness.
 *
 * Generates spawn points for 2-4 player multiplayer maps that:
 * - Are placed on or adjacent to substrate with minimum buildable radius
 * - Avoid blight mires contamination radius
 * - Have access to fluid sources within configurable distance
 * - Are roughly equidistant from each other
 * - Have terrain value scores within 15% tolerance
 * - Use approximate rotational symmetry (180/120/90 degrees for 2/3/4 players)
 *
 * Spawn points are deterministically generated from a seed for reproducibility.
 *
 * @see ElevationGenerator.h for elevation data
 * @see BiomeGenerator.h for biome distribution
 * @see WaterDistanceField.h for fluid proximity
 * @see ContaminationSourceQuery.h for blight mire locations
 */

#ifndef SIMS3000_TERRAIN_SPAWNPOINTGENERATOR_H
#define SIMS3000_TERRAIN_SPAWNPOINTGENERATOR_H

#include <sims3000/core/types.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/WaterDistanceField.h>
#include <sims3000/terrain/ProceduralNoise.h>
#include <cstdint>
#include <vector>
#include <array>

namespace sims3000 {
namespace terrain {

/**
 * @struct SpawnConfig
 * @brief Configuration for spawn point generation.
 *
 * Controls placement rules, scoring weights, and fairness tolerances.
 */
struct SpawnConfig {
    // =========================================================================
    // Player Count
    // =========================================================================

    /**
     * @brief Number of players to generate spawn points for (2-4).
     */
    std::uint8_t playerCount = 2;

    // =========================================================================
    // Placement Rules
    // =========================================================================

    /**
     * @brief Minimum buildable radius around spawn point (in tiles).
     *
     * All tiles within this radius must be buildable (substrate or cleared).
     */
    std::uint8_t minBuildableRadius = 5;

    /**
     * @brief Minimum distance from any blight mire (contamination source).
     *
     * Spawn points must be at least this far from any BlightMires tile.
     */
    std::uint8_t blightMireMinDistance = 10;

    /**
     * @brief Maximum distance to a fluid source (water tile).
     *
     * Spawn points must have at least one water tile within this distance.
     */
    std::uint8_t fluidAccessMaxDistance = 20;

    /**
     * @brief Minimum distance from map edge.
     *
     * Keeps spawn points away from ocean borders.
     */
    std::uint8_t edgeMargin = 15;

    // =========================================================================
    // Scoring Weights (0.0 - 1.0, sum should be ~1.0)
    // =========================================================================

    /**
     * @brief Weight for fluid access score component.
     *
     * Closer water = higher score.
     */
    float weightFluidAccess = 0.25f;

    /**
     * @brief Weight for special terrain bonus.
     *
     * Nearby PrismaFields, BiolumeGrove, SporeFlats add value.
     */
    float weightSpecialTerrain = 0.15f;

    /**
     * @brief Weight for buildable area score.
     *
     * More buildable tiles in radius = higher score.
     */
    float weightBuildableArea = 0.30f;

    /**
     * @brief Weight for contamination exposure penalty.
     *
     * Closer BlightMires = lower score.
     */
    float weightContaminationExposure = 0.15f;

    /**
     * @brief Weight for elevation advantage.
     *
     * Higher average elevation in spawn area = higher score.
     */
    float weightElevationAdvantage = 0.15f;

    // =========================================================================
    // Fairness Tolerance
    // =========================================================================

    /**
     * @brief Maximum allowed score difference as fraction (0.15 = 15%).
     *
     * All spawn scores must be within this tolerance of each other.
     */
    float scoreTolerance = 0.15f;

    // =========================================================================
    // Symmetry Configuration
    // =========================================================================

    /**
     * @brief Radius from map center for spawn placement (as fraction of map size).
     *
     * 0.35 means spawns are placed at ~35% of the distance from center to edge.
     */
    float spawnRadiusFraction = 0.35f;

    /**
     * @brief Angular tolerance for symmetry search (in degrees).
     *
     * When searching for symmetric positions, candidates within this angle
     * of the ideal position are considered.
     */
    float symmetryAngleTolerance = 15.0f;

    // =========================================================================
    // Scoring Radius
    // =========================================================================

    /**
     * @brief Radius around spawn point used for terrain value scoring.
     *
     * Larger radius considers more surrounding terrain for fairness.
     */
    std::uint8_t scoringRadius = 15;

    // =========================================================================
    // Factory Methods
    // =========================================================================

    /**
     * @brief Default configuration for standard multiplayer.
     */
    static SpawnConfig defaultConfig(std::uint8_t players = 2) {
        SpawnConfig cfg;
        cfg.playerCount = players;
        return cfg;
    }

    /**
     * @brief Configuration for competitive/ranked play with tighter tolerances.
     */
    static SpawnConfig competitive(std::uint8_t players) {
        SpawnConfig cfg;
        cfg.playerCount = players;
        cfg.scoreTolerance = 0.10f;  // Tighter 10% tolerance
        cfg.scoringRadius = 20;      // Larger scoring area
        cfg.minBuildableRadius = 7;  // Larger starting area
        return cfg;
    }

    /**
     * @brief Configuration for casual play with relaxed rules.
     */
    static SpawnConfig casual(std::uint8_t players) {
        SpawnConfig cfg;
        cfg.playerCount = players;
        cfg.scoreTolerance = 0.20f;  // More relaxed 20% tolerance
        cfg.edgeMargin = 10;         // Can spawn closer to edge
        return cfg;
    }
};

// Verify SpawnConfig is trivially copyable for serialization
static_assert(std::is_trivially_copyable<SpawnConfig>::value,
    "SpawnConfig must be trivially copyable");

/**
 * @struct SpawnPoint
 * @brief A single spawn point with its computed terrain value score.
 */
struct SpawnPoint {
    GridPosition position;           ///< Center position of spawn area
    float score;                     ///< Terrain value score (0.0 - 1.0)
    float fluidDistance;             ///< Distance to nearest fluid source
    float buildableAreaFraction;     ///< Fraction of tiles in radius that are buildable
    float contaminationDistance;     ///< Distance to nearest contamination source
    float avgElevation;              ///< Average elevation in spawn radius
    std::uint8_t playerIndex;        ///< Player index this spawn is assigned to (0-based)
    std::uint8_t _padding[3] = {0, 0, 0}; ///< Alignment padding
};

// Verify SpawnPoint is trivially copyable
static_assert(std::is_trivially_copyable<SpawnPoint>::value,
    "SpawnPoint must be trivially copyable");

/**
 * @struct SpawnPointResult
 * @brief Result of spawn point generation including validation info.
 */
struct SpawnPointResult {
    std::vector<SpawnPoint> spawns;  ///< Generated spawn points (one per player)
    float minScore;                  ///< Minimum score among all spawns
    float maxScore;                  ///< Maximum score among all spawns
    float scoreDifference;           ///< (max - min) / max score difference
    bool isValid;                    ///< True if all placement rules are satisfied
    bool isFair;                     ///< True if score difference <= tolerance
    float generationTimeMs;          ///< Time taken to generate

    /**
     * @brief Check if result meets all fairness criteria.
     */
    bool meetsAllCriteria() const {
        return isValid && isFair;
    }

    /**
     * @brief Get spawn point for a specific player index.
     */
    const SpawnPoint* getSpawnForPlayer(std::uint8_t playerIndex) const {
        for (const auto& spawn : spawns) {
            if (spawn.playerIndex == playerIndex) {
                return &spawn;
            }
        }
        return nullptr;
    }
};

/**
 * @struct MapSpawnData
 * @brief Spawn data stored with map for multiplayer join.
 *
 * This struct is saved with the map data and loaded when players join
 * to ensure consistent spawn assignments.
 */
struct MapSpawnData {
    static constexpr std::uint8_t MAX_PLAYERS = 4;

    std::array<GridPosition, MAX_PLAYERS> spawnPositions;  ///< Spawn positions per player
    std::array<float, MAX_PLAYERS> spawnScores;            ///< Terrain value scores
    std::uint8_t playerCount = 0;                          ///< Number of valid spawn points
    std::uint8_t _padding[3] = {0, 0, 0};                  ///< Alignment padding
    std::uint64_t generationSeed = 0;                      ///< Seed used for generation

    /**
     * @brief Get spawn position for a player index.
     * @param playerIndex Player index (0-based, must be < playerCount).
     * @return Grid position for the player's spawn.
     */
    GridPosition getSpawnPosition(std::uint8_t playerIndex) const {
        if (playerIndex < playerCount) {
            return spawnPositions[playerIndex];
        }
        return GridPosition{0, 0};
    }

    /**
     * @brief Check if spawn data is valid.
     */
    bool isValid() const {
        return playerCount >= 2 && playerCount <= MAX_PLAYERS;
    }
};

// Verify MapSpawnData size for serialization
static_assert(sizeof(MapSpawnData) == 48,
    "MapSpawnData should be 48 bytes (16 + 16 + 1 + 3 + 8 + 4 padding)");
static_assert(std::is_trivially_copyable<MapSpawnData>::value,
    "MapSpawnData must be trivially copyable");

/**
 * @class SpawnPointGenerator
 * @brief Generates fair spawn points for multiplayer games.
 *
 * Uses terrain analysis and rotational symmetry to place spawn points
 * that provide approximately equal starting conditions for all players.
 *
 * Usage:
 * @code
 * TerrainGrid grid(MapSize::Medium);
 * WaterDistanceField waterDist(MapSize::Medium);
 * // ... generate terrain ...
 *
 * SpawnConfig config = SpawnConfig::defaultConfig(4);
 * SpawnPointResult result = SpawnPointGenerator::generate(
 *     grid, waterDist, seed, config);
 *
 * if (result.meetsAllCriteria()) {
 *     MapSpawnData mapData = SpawnPointGenerator::toMapSpawnData(result, seed);
 *     // Store mapData with map
 * }
 * @endcode
 *
 * Thread Safety:
 * - generate() is NOT thread-safe (reads grid data)
 */
class SpawnPointGenerator {
public:
    /**
     * @brief Generate spawn points for multiplayer game.
     *
     * Attempts to find spawn points that satisfy all placement rules
     * and have terrain value scores within the tolerance.
     *
     * @param grid Terrain grid with elevation and biome data.
     * @param waterDist Pre-computed water distance field.
     * @param seed Random seed for deterministic generation.
     * @param config Configuration for spawn generation.
     * @return Result containing spawn points and validation info.
     */
    static SpawnPointResult generate(
        const TerrainGrid& grid,
        const WaterDistanceField& waterDist,
        std::uint64_t seed,
        const SpawnConfig& config = SpawnConfig::defaultConfig()
    );

    /**
     * @brief Convert generation result to storable map spawn data.
     *
     * @param result Generation result with spawn points.
     * @param seed Seed used for generation.
     * @return MapSpawnData suitable for storage with map.
     */
    static MapSpawnData toMapSpawnData(
        const SpawnPointResult& result,
        std::uint64_t seed
    );

    /**
     * @brief Calculate terrain value score for a position.
     *
     * Computes weighted score based on:
     * - Fluid access (closer water = higher)
     * - Special terrain (nearby crystals/groves = higher)
     * - Buildable area (more buildable = higher)
     * - Contamination (closer blight = lower)
     * - Elevation (higher average = higher)
     *
     * @param grid Terrain grid.
     * @param waterDist Water distance field.
     * @param pos Position to score.
     * @param config Scoring configuration.
     * @return Terrain value score (0.0 - 1.0).
     */
    static float calculateTerrainScore(
        const TerrainGrid& grid,
        const WaterDistanceField& waterDist,
        GridPosition pos,
        const SpawnConfig& config
    );

    /**
     * @brief Check if a position satisfies all placement rules.
     *
     * Validates:
     * - On or adjacent to substrate
     * - Minimum buildable radius
     * - Not near blight mires
     * - Has fluid access
     * - Not too close to edge
     *
     * @param grid Terrain grid.
     * @param waterDist Water distance field.
     * @param pos Position to validate.
     * @param config Placement rules.
     * @return true if position is valid for spawn.
     */
    static bool isValidSpawnPosition(
        const TerrainGrid& grid,
        const WaterDistanceField& waterDist,
        GridPosition pos,
        const SpawnConfig& config
    );

    /**
     * @brief Get rotation angle for player count symmetry.
     *
     * @param playerCount Number of players (2-4).
     * @return Rotation angle in degrees (180, 120, or 90).
     */
    static float getSymmetryAngle(std::uint8_t playerCount);

    /**
     * @brief Calculate distance between two grid positions.
     *
     * @param a First position.
     * @param b Second position.
     * @return Euclidean distance in tiles.
     */
    static float calculateDistance(GridPosition a, GridPosition b);

private:
    /**
     * @brief Find candidate spawn positions using rotational symmetry.
     *
     * Generates positions at regular angular intervals from map center.
     */
    static std::vector<GridPosition> findSymmetricCandidates(
        const TerrainGrid& grid,
        const WaterDistanceField& waterDist,
        Xoshiro256& rng,
        const SpawnConfig& config
    );

    /**
     * @brief Score and select best spawn positions from candidates.
     */
    static std::vector<SpawnPoint> selectBestSpawns(
        const TerrainGrid& grid,
        const WaterDistanceField& waterDist,
        const std::vector<GridPosition>& candidates,
        const SpawnConfig& config
    );

    /**
     * @brief Check if all tiles in radius are buildable.
     */
    static bool checkBuildableRadius(
        const TerrainGrid& grid,
        GridPosition center,
        std::uint8_t radius
    );

    /**
     * @brief Find distance to nearest contamination source.
     */
    static float findContaminationDistance(
        const TerrainGrid& grid,
        GridPosition pos,
        std::uint8_t searchRadius
    );

    /**
     * @brief Count special terrain tiles in radius.
     */
    static std::uint32_t countSpecialTerrain(
        const TerrainGrid& grid,
        GridPosition center,
        std::uint8_t radius
    );

    /**
     * @brief Calculate average elevation in radius.
     */
    static float calculateAvgElevation(
        const TerrainGrid& grid,
        GridPosition center,
        std::uint8_t radius
    );

    /**
     * @brief Count buildable tiles in radius.
     */
    static std::uint32_t countBuildableTiles(
        const TerrainGrid& grid,
        GridPosition center,
        std::uint8_t radius
    );

    /**
     * @brief Check if terrain type is buildable.
     */
    static bool isBuildable(TerrainType type);

    /**
     * @brief Check if terrain type is special (bonus value).
     */
    static bool isSpecialTerrain(TerrainType type);

    /**
     * @brief Rotate a position around the map center.
     */
    static GridPosition rotateAroundCenter(
        GridPosition pos,
        float angleDegrees,
        std::uint16_t mapWidth,
        std::uint16_t mapHeight
    );
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_SPAWNPOINTGENERATOR_H
