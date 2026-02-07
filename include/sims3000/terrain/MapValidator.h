/**
 * @file MapValidator.h
 * @brief Post-generation map validation to ensure quality requirements.
 *
 * Validates generated maps against quality criteria before accepting them:
 * - Minimum buildable area percentage (>= 50% immediately buildable)
 * - At least one river (flow channel) exists
 * - Coastline continuity (no single-tile ocean gaps if ocean exists)
 * - No single-tile terrain anomalies (isolated terrain types)
 * - Terrain type distribution within target ranges
 * - All spawn points meet minimum quality threshold
 *
 * If validation fails, the generator can retry with seed+1, up to N retries.
 * If all retries exhausted, accepts the best attempt with a warning.
 *
 * @see ElevationGenerator.h for elevation heightmap
 * @see WaterBodyGenerator.h for water body placement
 * @see BiomeGenerator.h for biome distribution
 * @see SpawnPointGenerator.h for spawn point quality
 */

#ifndef SIMS3000_TERRAIN_MAPVALIDATOR_H
#define SIMS3000_TERRAIN_MAPVALIDATOR_H

#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <sims3000/terrain/WaterDistanceField.h>
#include <sims3000/terrain/SpawnPointGenerator.h>
#include <cstdint>
#include <vector>
#include <string>

namespace sims3000 {
namespace terrain {

/**
 * @struct ValidationConfig
 * @brief Configuration for map validation criteria.
 */
struct ValidationConfig {
    // =========================================================================
    // Buildable Area Requirements
    // =========================================================================

    /**
     * @brief Minimum percentage of tiles that must be immediately buildable.
     *
     * Immediately buildable means Substrate, BiolumeGrove, PrismaFields,
     * SporeFlats, or EmberCrust (cleared or clearable terrain).
     * Water and BlightMires are NOT buildable.
     */
    float minBuildablePercent = 0.50f;

    // =========================================================================
    // River Requirements
    // =========================================================================

    /**
     * @brief Whether at least one river must exist.
     */
    bool requireRiver = true;

    // =========================================================================
    // Coastline Continuity
    // =========================================================================

    /**
     * @brief Check for single-tile ocean gaps if deep void present.
     *
     * A single-tile gap is a land tile completely surrounded by ocean.
     */
    bool checkCoastlineContinuity = true;

    // =========================================================================
    // Terrain Anomaly Detection
    // =========================================================================

    /**
     * @brief Check for isolated single-tile terrain type anomalies.
     *
     * An anomaly is a single tile of one type completely surrounded by
     * a different type.
     */
    bool checkTerrainAnomalies = true;

    // =========================================================================
    // Terrain Distribution Ranges
    // =========================================================================

    /**
     * @brief Minimum Substrate coverage (as fraction of land tiles).
     */
    float substrateMinPercent = 0.35f;

    /**
     * @brief Maximum Substrate coverage (as fraction of land tiles).
     */
    float substrateMaxPercent = 0.45f;

    /**
     * @brief Enable terrain distribution validation.
     */
    bool checkTerrainDistribution = true;

    // =========================================================================
    // Spawn Point Requirements
    // =========================================================================

    /**
     * @brief Minimum score threshold for spawn points (0.0 - 1.0).
     *
     * All spawn points must score at least this value.
     */
    float minSpawnPointScore = 0.3f;

    /**
     * @brief Number of players to validate spawn points for.
     */
    std::uint8_t playerCount = 2;

    /**
     * @brief Enable spawn point quality validation.
     */
    bool checkSpawnPoints = true;

    // =========================================================================
    // Retry Configuration
    // =========================================================================

    /**
     * @brief Maximum number of retries on validation failure.
     */
    std::uint8_t maxRetries = 10;

    // =========================================================================
    // Factory Methods
    // =========================================================================

    /**
     * @brief Default configuration for standard maps.
     */
    static ValidationConfig defaultConfig() {
        return ValidationConfig{};
    }

    /**
     * @brief Strict configuration for competitive maps.
     */
    static ValidationConfig strict() {
        ValidationConfig cfg;
        cfg.minBuildablePercent = 0.55f;
        cfg.minSpawnPointScore = 0.4f;
        cfg.substrateMinPercent = 0.38f;
        cfg.substrateMaxPercent = 0.42f;
        return cfg;
    }

    /**
     * @brief Relaxed configuration for casual/sandbox maps.
     */
    static ValidationConfig relaxed() {
        ValidationConfig cfg;
        cfg.minBuildablePercent = 0.40f;
        cfg.minSpawnPointScore = 0.2f;
        cfg.checkTerrainDistribution = false;
        return cfg;
    }
};

// Verify ValidationConfig is trivially copyable for serialization
static_assert(std::is_trivially_copyable<ValidationConfig>::value,
    "ValidationConfig must be trivially copyable");

/**
 * @struct ValidationCheckResult
 * @brief Result of a single validation check.
 */
struct ValidationCheckResult {
    bool passed;            ///< Whether this check passed
    float actualValue;      ///< The actual measured value
    float requiredValue;    ///< The required threshold value
    std::string checkName;  ///< Name of the check for logging
};

/**
 * @struct ValidationResult
 * @brief Complete result of map validation.
 */
struct ValidationResult {
    bool isValid;                              ///< True if all required checks passed
    float aggregateScore;                      ///< Overall quality score (0.0 - 1.0)
    float validationTimeMs;                    ///< Time taken for validation (ms)

    // Individual check results
    bool buildableAreaPassed;                  ///< Buildable area >= minimum
    float buildableAreaPercent;                ///< Actual buildable percentage

    bool riverExistsPassed;                    ///< At least one river exists
    std::uint32_t riverTileCount;              ///< Number of FlowChannel tiles

    bool coastlineContinuityPassed;            ///< No single-tile ocean gaps
    std::uint32_t coastlineGapCount;           ///< Number of gaps found

    bool terrainAnomaliesPassed;               ///< No isolated single-tile anomalies
    std::uint32_t anomalyCount;                ///< Number of anomalies found

    bool terrainDistributionPassed;            ///< Substrate within target range
    float substratePercent;                    ///< Actual Substrate coverage

    bool spawnPointsPassed;                    ///< All spawn points meet threshold
    float minSpawnScore;                       ///< Lowest spawn point score

    // Detailed terrain breakdown
    std::uint32_t substrateCount;
    std::uint32_t ridgeCount;
    std::uint32_t waterCount;
    std::uint32_t biomeCount;
    std::uint32_t totalTiles;
    std::uint32_t landTiles;

    /**
     * @brief Get number of checks that passed.
     */
    std::uint8_t passedCheckCount() const {
        std::uint8_t count = 0;
        if (buildableAreaPassed) ++count;
        if (riverExistsPassed) ++count;
        if (coastlineContinuityPassed) ++count;
        if (terrainAnomaliesPassed) ++count;
        if (terrainDistributionPassed) ++count;
        if (spawnPointsPassed) ++count;
        return count;
    }

    /**
     * @brief Get total number of checks performed.
     */
    static constexpr std::uint8_t totalCheckCount() {
        return 6;
    }
};

/**
 * @class MapValidator
 * @brief Validates generated maps against quality requirements.
 *
 * Performs individual validation checks that can be run independently,
 * then aggregates results into an overall validation score.
 *
 * Usage:
 * @code
 * TerrainGrid grid(MapSize::Medium);
 * WaterDistanceField waterDist(MapSize::Medium);
 * // ... generate terrain ...
 *
 * ValidationConfig config = ValidationConfig::defaultConfig();
 * ValidationResult result = MapValidator::validate(grid, waterDist, seed, config);
 *
 * if (!result.isValid) {
 *     // Try with different seed or accept best attempt
 * }
 * @endcode
 *
 * Thread Safety:
 * - validate() is thread-safe (reads only from grid)
 * - Individual check functions are thread-safe
 */
class MapValidator {
public:
    /**
     * @brief Validate a generated map against all criteria.
     *
     * Performs all validation checks and returns aggregate result.
     * Target: <10ms for validation.
     *
     * @param grid The terrain grid to validate.
     * @param waterDist Pre-computed water distance field.
     * @param seed The seed used for generation (for spawn point validation).
     * @param config Validation configuration.
     * @return Complete validation result.
     */
    static ValidationResult validate(
        const TerrainGrid& grid,
        const WaterDistanceField& waterDist,
        std::uint64_t seed,
        const ValidationConfig& config = ValidationConfig::defaultConfig()
    );

    /**
     * @brief Check minimum buildable area percentage.
     *
     * Counts tiles that are immediately buildable:
     * - Substrate (always buildable)
     * - BiolumeGrove (clearable)
     * - PrismaFields (clearable)
     * - SporeFlats (clearable)
     * - EmberCrust (buildable)
     * - Ridge (buildable with cost)
     *
     * NOT buildable:
     * - DeepVoid, FlowChannel, StillBasin (water)
     * - BlightMires (toxic)
     *
     * @param grid The terrain grid.
     * @param minPercent Minimum required percentage (0.0 - 1.0).
     * @param outPercent Output: actual buildable percentage.
     * @return true if buildable area >= minPercent.
     */
    static bool checkBuildableArea(
        const TerrainGrid& grid,
        float minPercent,
        float& outPercent
    );

    /**
     * @brief Check that at least one river exists.
     *
     * @param grid The terrain grid.
     * @param outRiverTileCount Output: number of FlowChannel tiles.
     * @return true if at least one FlowChannel tile exists.
     */
    static bool checkRiverExists(
        const TerrainGrid& grid,
        std::uint32_t& outRiverTileCount
    );

    /**
     * @brief Check coastline continuity (no single-tile ocean gaps).
     *
     * A gap is a non-ocean tile surrounded on all 8 sides by ocean.
     * Only checked if deep void is present on the map.
     *
     * @param grid The terrain grid.
     * @param outGapCount Output: number of gaps found.
     * @return true if no gaps found (or no ocean present).
     */
    static bool checkCoastlineContinuity(
        const TerrainGrid& grid,
        std::uint32_t& outGapCount
    );

    /**
     * @brief Check for single-tile terrain type anomalies.
     *
     * An anomaly is a tile of one type surrounded on all 8 sides by
     * a different single type. Small clusters (1 tile) are anomalies.
     *
     * @param grid The terrain grid.
     * @param outAnomalyCount Output: number of anomalies found.
     * @return true if no anomalies found.
     */
    static bool checkTerrainAnomalies(
        const TerrainGrid& grid,
        std::uint32_t& outAnomalyCount
    );

    /**
     * @brief Check terrain type distribution is within target ranges.
     *
     * Currently checks Substrate coverage (35-45% of land tiles).
     *
     * @param grid The terrain grid.
     * @param minSubstrate Minimum Substrate percentage.
     * @param maxSubstrate Maximum Substrate percentage.
     * @param outSubstratePercent Output: actual Substrate percentage.
     * @return true if Substrate within range.
     */
    static bool checkTerrainDistribution(
        const TerrainGrid& grid,
        float minSubstrate,
        float maxSubstrate,
        float& outSubstratePercent
    );

    /**
     * @brief Check spawn point quality meets minimum threshold.
     *
     * @param grid The terrain grid.
     * @param waterDist Water distance field.
     * @param seed Seed for spawn point generation.
     * @param playerCount Number of players.
     * @param minScore Minimum required score per spawn.
     * @param outMinScore Output: actual minimum score.
     * @return true if all spawn points >= minScore.
     */
    static bool checkSpawnPointQuality(
        const TerrainGrid& grid,
        const WaterDistanceField& waterDist,
        std::uint64_t seed,
        std::uint8_t playerCount,
        float minScore,
        float& outMinScore
    );

    /**
     * @brief Calculate aggregate validation score (0.0 - 1.0).
     *
     * Weighted combination of individual check results.
     * Higher score = better map quality.
     *
     * @param result The validation result to score.
     * @return Aggregate score from 0.0 (worst) to 1.0 (best).
     */
    static float calculateAggregateScore(const ValidationResult& result);

    /**
     * @brief Check if a terrain type is buildable.
     *
     * @param type The terrain type to check.
     * @return true if terrain is buildable or clearable.
     */
    static bool isBuildable(TerrainType type);

    /**
     * @brief Check if a terrain type is water.
     *
     * @param type The terrain type to check.
     * @return true if terrain is DeepVoid, FlowChannel, or StillBasin.
     */
    static bool isWater(TerrainType type);

private:
    /**
     * @brief Count terrain tiles by type.
     */
    static void countTerrainTypes(
        const TerrainGrid& grid,
        std::uint32_t& outSubstrate,
        std::uint32_t& outRidge,
        std::uint32_t& outWater,
        std::uint32_t& outBiome
    );

    /**
     * @brief Check if a tile is surrounded by a single different type.
     *
     * Returns true if all 8 neighbors are the same type AND different
     * from the center tile.
     */
    static bool isSingleTileAnomaly(
        const TerrainGrid& grid,
        std::uint16_t x,
        std::uint16_t y
    );
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_MAPVALIDATOR_H
