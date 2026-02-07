/**
 * @file ElevationGenerator.h
 * @brief Elevation heightmap generation using multi-octave noise.
 *
 * Generates geologically coherent terrain with ridges, valleys, plateaus,
 * and lowlands. Uses multi-octave fBm (fractal Brownian motion) noise to
 * create natural-looking elevation patterns.
 *
 * Elevation bands:
 * - Lowlands: 0-3
 * - Foothills: 4-10
 * - Highlands: 11-20
 * - Ridgelines: 21-27
 * - Peaks: 28-31
 *
 * Features:
 * - Multi-octave noise (4-6 octaves) for natural terrain
 * - Configurable roughness, amplitude, feature scale, ridge threshold
 * - Deterministic generation (same seed = same heightmap)
 * - Row-major generation order (top-to-bottom, left-to-right)
 * - Assigns TerrainType::Ridge to tiles above ridge threshold
 * - Assigns TerrainType::Substrate to remaining non-water tiles
 *
 * @see ProceduralNoise.h for underlying noise functions
 * @see TerrainGrid.h for grid storage
 */

#ifndef SIMS3000_TERRAIN_ELEVATIONGENERATOR_H
#define SIMS3000_TERRAIN_ELEVATIONGENERATOR_H

#include <sims3000/terrain/ProceduralNoise.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <cstdint>

namespace sims3000 {
namespace terrain {

/**
 * @struct ElevationConfig
 * @brief Configuration for elevation heightmap generation.
 *
 * Controls the noise parameters and terrain classification thresholds.
 * All values have sensible defaults that produce natural-looking terrain.
 */
struct ElevationConfig {
    // =========================================================================
    // Noise Parameters
    // =========================================================================

    /**
     * @brief Number of noise octaves (1-8).
     *
     * More octaves add finer detail but cost more computation.
     * Recommended: 4-6 for terrain generation.
     */
    std::uint8_t octaves = 5;

    /**
     * @brief Roughness (persistence) - amplitude multiplier per octave.
     *
     * Controls how quickly amplitude decreases at higher frequencies.
     * Lower values = smoother terrain, higher values = rougher terrain.
     * Range: 0.0 - 1.0, typical: 0.4 - 0.6
     */
    float roughness = 0.5f;

    /**
     * @brief Base amplitude for noise.
     *
     * Scales the overall noise contribution. Higher values increase
     * elevation variance.
     */
    float amplitude = 1.0f;

    /**
     * @brief Feature scale - controls size of terrain features.
     *
     * Lower values = larger features (broad ridges/valleys).
     * Higher values = smaller features (many small hills).
     * Typical range: 0.002 - 0.02 for terrain.
     */
    float featureScale = 0.008f;

    /**
     * @brief Frequency multiplier per octave (lacunarity).
     *
     * Controls how quickly frequency increases at each octave.
     * Standard value is 2.0, higher values add more high-frequency detail.
     */
    float lacunarity = 2.0f;

    // =========================================================================
    // Terrain Classification Thresholds
    // =========================================================================

    /**
     * @brief Elevation threshold for ridge classification.
     *
     * Tiles with elevation >= this value are assigned TerrainType::Ridge.
     * Default: 21 (ridgelines band starts at 21).
     */
    std::uint8_t ridgeThreshold = 21;

    // =========================================================================
    // Elevation Distribution
    // =========================================================================

    /**
     * @brief Minimum elevation value (before clamping to 0-31).
     *
     * Used to shift the elevation distribution. Lower values create
     * more lowland areas.
     */
    std::uint8_t minElevation = 0;

    /**
     * @brief Maximum elevation value (before clamping to 0-31).
     *
     * Used to limit peak heights. Standard is 31 (full range).
     */
    std::uint8_t maxElevation = 31;

    /**
     * @brief Seed offset for this elevation layer.
     *
     * Allows generating different terrain from the same base seed.
     * Different layers (elevation, moisture) should use different offsets.
     */
    std::int32_t seedOffset = 0;

    // =========================================================================
    // Ridge Enhancement
    // =========================================================================

    /**
     * @brief Enable ridge enhancement for more coherent ridgelines.
     *
     * When enabled, applies additional processing to create more
     * pronounced and connected ridgelines rather than random peaks.
     */
    bool enhanceRidges = true;

    /**
     * @brief Ridge enhancement strength (0.0 - 1.0).
     *
     * Controls how much ridge enhancement affects the final output.
     * Higher values = more pronounced ridges and deeper valleys.
     */
    float ridgeStrength = 0.3f;

    // =========================================================================
    // Factory Methods
    // =========================================================================

    /**
     * @brief Default configuration for standard terrain.
     *
     * Produces balanced terrain with moderate ridges and valleys.
     * Good starting point for most maps.
     */
    static ElevationConfig defaultConfig() {
        return ElevationConfig{};
    }

    /**
     * @brief Configuration for mountainous terrain.
     *
     * Higher elevations, more dramatic ridges, deeper valleys.
     */
    static ElevationConfig mountainous() {
        ElevationConfig cfg;
        cfg.octaves = 6;
        cfg.roughness = 0.55f;
        cfg.featureScale = 0.006f;
        cfg.ridgeThreshold = 18;
        cfg.ridgeStrength = 0.4f;
        return cfg;
    }

    /**
     * @brief Configuration for flat terrain with gentle hills.
     *
     * Lower elevations, smoother features, fewer ridges.
     */
    static ElevationConfig plains() {
        ElevationConfig cfg;
        cfg.octaves = 4;
        cfg.roughness = 0.4f;
        cfg.featureScale = 0.012f;
        cfg.ridgeThreshold = 25;
        cfg.ridgeStrength = 0.15f;
        cfg.maxElevation = 20;
        return cfg;
    }

    /**
     * @brief Configuration for rolling hills.
     *
     * Medium elevation with consistent rolling features.
     */
    static ElevationConfig rolling() {
        ElevationConfig cfg;
        cfg.octaves = 5;
        cfg.roughness = 0.45f;
        cfg.featureScale = 0.01f;
        cfg.ridgeThreshold = 22;
        cfg.ridgeStrength = 0.2f;
        return cfg;
    }
};

// Verify ElevationConfig is trivially copyable for serialization
static_assert(std::is_trivially_copyable<ElevationConfig>::value,
    "ElevationConfig must be trivially copyable");

/**
 * @struct ElevationResult
 * @brief Statistics from elevation generation.
 *
 * Provides information about the generated heightmap for debugging
 * and verification purposes.
 */
struct ElevationResult {
    std::uint8_t minElevation;    ///< Minimum elevation value generated
    std::uint8_t maxElevation;    ///< Maximum elevation value generated
    float meanElevation;          ///< Mean elevation value
    std::uint32_t ridgeTileCount; ///< Number of tiles classified as Ridge
    std::uint32_t totalTiles;     ///< Total number of tiles processed
    float generationTimeMs;       ///< Time taken to generate (milliseconds)
};

/**
 * @class ElevationGenerator
 * @brief Generates elevation heightmaps using multi-octave noise.
 *
 * Uses the SimplexNoise fBm implementation for natural terrain generation.
 * The generator is stateless - all parameters are passed via ElevationConfig.
 *
 * Usage:
 * @code
 * TerrainGrid grid(MapSize::Medium);
 * ElevationConfig config = ElevationConfig::defaultConfig();
 * ElevationResult result = ElevationGenerator::generate(grid, 12345, config);
 * @endcode
 *
 * Thread Safety:
 * - generate() is NOT thread-safe (modifies grid)
 * - sampleElevation() is thread-safe after SimplexNoise construction
 *
 * @note Generation is single-threaded for deterministic RNG call order.
 */
class ElevationGenerator {
public:
    /**
     * @brief Generate elevation heightmap for the entire grid.
     *
     * Fills the grid with elevation values using multi-octave noise.
     * Also assigns terrain types:
     * - TerrainType::Ridge for tiles >= ridgeThreshold
     * - TerrainType::Substrate for remaining tiles
     *
     * Preserves existing water tiles (DeepVoid, FlowChannel, StillBasin)
     * if they were previously set.
     *
     * @param grid The terrain grid to fill with elevation data.
     * @param seed Random seed for deterministic generation.
     * @param config Configuration parameters for generation.
     * @return Statistics about the generated heightmap.
     *
     * @note Generation order is row-major (top-to-bottom, left-to-right).
     * @note This modifies the grid in place.
     */
    static ElevationResult generate(
        TerrainGrid& grid,
        std::uint64_t seed,
        const ElevationConfig& config = ElevationConfig::defaultConfig()
    );

    /**
     * @brief Sample raw elevation value at a specific coordinate.
     *
     * Returns a raw noise value (0.0 to 1.0) that can be converted
     * to an elevation level. Useful for testing or custom generation.
     *
     * @param noise Pre-initialized SimplexNoise generator.
     * @param x X coordinate in grid space.
     * @param y Y coordinate in grid space.
     * @param config Configuration parameters.
     * @return Raw normalized elevation value (0.0 to 1.0).
     */
    static float sampleRawElevation(
        const SimplexNoise& noise,
        float x,
        float y,
        const ElevationConfig& config
    );

    /**
     * @brief Convert raw elevation (0.0-1.0) to discrete level (0-31).
     *
     * Applies the elevation range from config (minElevation, maxElevation)
     * and clamps to valid range.
     *
     * @param rawValue Raw normalized value (0.0 to 1.0).
     * @param config Configuration with min/max elevation.
     * @return Discrete elevation level (0-31).
     */
    static std::uint8_t rawToElevation(
        float rawValue,
        const ElevationConfig& config
    );

    /**
     * @brief Check if an elevation qualifies as a ridge.
     *
     * @param elevation Elevation value (0-31).
     * @param config Configuration with ridge threshold.
     * @return true if elevation >= ridgeThreshold.
     */
    static bool isRidge(
        std::uint8_t elevation,
        const ElevationConfig& config
    );

private:
    /**
     * @brief Apply ridge enhancement to raw elevation.
     *
     * Uses ridged noise technique to create more pronounced ridgelines.
     * Takes absolute value and inverts to create sharp peaks.
     */
    static float applyRidgeEnhancement(
        const SimplexNoise& noise,
        float x,
        float y,
        float baseElevation,
        const ElevationConfig& config
    );
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_ELEVATIONGENERATOR_H
