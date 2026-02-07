/**
 * @file MapSizeScaling.h
 * @brief Map size scaling helper for procedural generation parameters.
 *
 * Ensures procedural generation parameters scale properly across 128/256/512
 * map sizes. Feature density remains perceptually similar regardless of map
 * size - a 512x512 map has proportionally more features (more rivers, more
 * biome clusters), not just a zoomed-in 256x256.
 *
 * Scaling principles:
 * - Noise frequency scales inversely with map size (consistent world-space scale)
 * - Feature counts scale proportionally with area (4x tiles = 4x features)
 * - Cluster sizes scale with map dimensions (larger maps = larger clusters)
 * - Border/margin widths scale linearly with map dimension
 *
 * Reference map size is 256x256 (Medium) - the baseline for all parameters.
 *
 * @see ElevationGenerator.h
 * @see WaterBodyGenerator.h
 * @see BiomeGenerator.h
 * @see TerrainGrid.h for MapSize enum
 */

#ifndef SIMS3000_TERRAIN_MAPSIZESCALING_H
#define SIMS3000_TERRAIN_MAPSIZESCALING_H

#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/ElevationGenerator.h>
#include <sims3000/terrain/WaterBodyGenerator.h>
#include <sims3000/terrain/BiomeGenerator.h>
#include <cstdint>
#include <cmath>

namespace sims3000 {
namespace terrain {

/**
 * @brief Reference map size for scaling calculations.
 *
 * All default configurations are tuned for 256x256 maps.
 * Scaling factors are calculated relative to this size.
 */
constexpr std::uint16_t REFERENCE_MAP_SIZE = 256;

/**
 * @class MapSizeScaling
 * @brief Helper class for scaling generation parameters across map sizes.
 *
 * Provides factory methods that return properly scaled configurations
 * for ElevationGenerator, WaterBodyGenerator, and BiomeGenerator.
 *
 * Usage:
 * @code
 * MapSize mapSize = MapSize::Large;  // 512x512
 * ElevationConfig elevConfig = MapSizeScaling::scaleElevationConfig(
 *     ElevationConfig::defaultConfig(), mapSize);
 * WaterBodyConfig waterConfig = MapSizeScaling::scaleWaterBodyConfig(
 *     WaterBodyConfig::defaultConfig(), mapSize);
 * BiomeConfig biomeConfig = MapSizeScaling::scaleBiomeConfig(
 *     BiomeConfig::defaultConfig(), mapSize);
 * @endcode
 *
 * Scaling Formulas:
 * ================
 *
 * 1. Noise Frequency Scaling (featureScale, baseFeatureScale):
 *    - scaledFrequency = baseFrequency * (REFERENCE_SIZE / actualSize)
 *    - Example: 256x256 uses 0.008, 512x512 uses 0.004 (half frequency = double feature size)
 *    - This ensures features have consistent world-space scale
 *
 * 2. Feature Count Scaling (riverCount, lakeCount, etc.):
 *    - scaledCount = baseCount * (areaRatio)
 *    - areaRatio = (actualSize / REFERENCE_SIZE)^2
 *    - Example: 512x512 has 4x the tiles of 256x256, so 4x the rivers
 *
 * 3. Border Width Scaling (oceanBorderWidth, margins):
 *    - scaledWidth = baseWidth * (actualSize / REFERENCE_SIZE)
 *    - Example: 256x256 uses 5 tiles, 512x512 uses 10 tiles
 *
 * 4. Cluster Size Scaling (minClusterRadius, groveWaterProximityMax):
 *    - scaledSize = baseSize * sqrt(actualSize / REFERENCE_SIZE)
 *    - Square root for perceptually similar cluster sizes
 *
 * Scaling Table (Medium 256x256 as reference):
 * =============================================
 *
 * | Parameter              | 128x128 | 256x256 | 512x512 |
 * |------------------------|---------|---------|---------|
 * | featureScale           | 0.016   | 0.008   | 0.004   |
 * | riverCount (min-max)   | 1-2     | 1-4     | 2-8     |
 * | lakeCount (max)        | 2       | 3       | 6       |
 * | oceanBorderWidth       | 3       | 5       | 10      |
 * | biomeFeatureScale      | 0.030   | 0.015   | 0.0075  |
 * | minClusterRadius       | 1       | 2       | 3       |
 * | groveWaterProximity    | 2       | 3       | 4       |
 *
 * @note All scaling is deterministic - same map size always produces
 *       same scaled parameters for reproducible terrain generation.
 */
class MapSizeScaling {
public:
    // =========================================================================
    // Scaling Factor Calculations
    // =========================================================================

    /**
     * @brief Get the linear scaling factor for a map size.
     *
     * linearFactor = actualSize / REFERENCE_SIZE
     *
     * @param size The target map size.
     * @return Linear scaling factor (0.5 for Small, 1.0 for Medium, 2.0 for Large).
     */
    static float getLinearFactor(MapSize size) {
        return static_cast<float>(static_cast<std::uint16_t>(size)) /
               static_cast<float>(REFERENCE_MAP_SIZE);
    }

    /**
     * @brief Get the area scaling factor for a map size.
     *
     * areaFactor = (actualSize / REFERENCE_SIZE)^2
     *
     * @param size The target map size.
     * @return Area scaling factor (0.25 for Small, 1.0 for Medium, 4.0 for Large).
     */
    static float getAreaFactor(MapSize size) {
        float linear = getLinearFactor(size);
        return linear * linear;
    }

    /**
     * @brief Get the inverse linear factor for frequency scaling.
     *
     * inverseFactor = REFERENCE_SIZE / actualSize
     *
     * Used for noise frequency - larger maps need smaller frequency
     * to maintain consistent world-space feature size.
     *
     * @param size The target map size.
     * @return Inverse factor (2.0 for Small, 1.0 for Medium, 0.5 for Large).
     */
    static float getInverseLinearFactor(MapSize size) {
        return static_cast<float>(REFERENCE_MAP_SIZE) /
               static_cast<float>(static_cast<std::uint16_t>(size));
    }

    /**
     * @brief Get the square root scaling factor.
     *
     * sqrtFactor = sqrt(actualSize / REFERENCE_SIZE)
     *
     * Used for cluster sizes - provides intermediate scaling between
     * linear and area factors for perceptually consistent clusters.
     *
     * @param size The target map size.
     * @return Square root factor (~0.71 for Small, 1.0 for Medium, ~1.41 for Large).
     */
    static float getSqrtFactor(MapSize size) {
        return std::sqrt(getLinearFactor(size));
    }

    // =========================================================================
    // Configuration Scaling Methods
    // =========================================================================

    /**
     * @brief Scale an ElevationConfig for the target map size.
     *
     * Scales:
     * - featureScale: Inversely with map size (larger maps = smaller frequency)
     *
     * Other parameters (octaves, roughness, etc.) are not scaled as they
     * control the character of terrain, not its spatial extent.
     *
     * @param baseConfig The base configuration (tuned for 256x256).
     * @param size The target map size.
     * @return Scaled configuration for the target map size.
     */
    static ElevationConfig scaleElevationConfig(
        const ElevationConfig& baseConfig,
        MapSize size
    ) {
        ElevationConfig scaled = baseConfig;

        // Scale noise frequency inversely with map size
        // Larger maps need smaller frequency for consistent feature scale
        float invFactor = getInverseLinearFactor(size);
        scaled.featureScale = baseConfig.featureScale * invFactor;

        return scaled;
    }

    /**
     * @brief Scale a WaterBodyConfig for the target map size.
     *
     * Scales:
     * - oceanBorderWidth: Linearly with map size
     * - riverCount (min/max): With area (more tiles = more rivers)
     * - lakeCount: With area
     * - maxLakeRadius: With sqrt (intermediate scaling)
     * - minTributaryLength: Linearly with map size
     *
     * @param baseConfig The base configuration (tuned for 256x256).
     * @param size The target map size.
     * @return Scaled configuration for the target map size.
     */
    static WaterBodyConfig scaleWaterBodyConfig(
        const WaterBodyConfig& baseConfig,
        MapSize size
    ) {
        WaterBodyConfig scaled = baseConfig;

        float linearFactor = getLinearFactor(size);
        float areaFactor = getAreaFactor(size);
        float sqrtFactor = getSqrtFactor(size);

        // Border width scales linearly
        scaled.oceanBorderWidth = static_cast<std::uint16_t>(
            std::max(3.0f, baseConfig.oceanBorderWidth * linearFactor)
        );

        // River count scales with area (more tiles = more rivers)
        scaled.minRiverCount = static_cast<std::uint8_t>(
            std::max(1.0f, baseConfig.minRiverCount * areaFactor)
        );
        scaled.maxRiverCount = static_cast<std::uint8_t>(
            std::max(2.0f, baseConfig.maxRiverCount * areaFactor)
        );

        // Lake count scales with area
        scaled.maxLakeCount = static_cast<std::uint8_t>(
            std::max(1.0f, baseConfig.maxLakeCount * areaFactor)
        );

        // Lake radius scales with sqrt for perceptually consistent size
        scaled.maxLakeRadius = static_cast<std::uint8_t>(
            std::max(4.0f, baseConfig.maxLakeRadius * sqrtFactor)
        );

        // Tributary minimum length scales linearly
        scaled.minTributaryLength = static_cast<std::uint16_t>(
            std::max(5.0f, baseConfig.minTributaryLength * linearFactor)
        );

        // River width doesn't scale - rivers should be similar visual width
        // regardless of map size

        return scaled;
    }

    /**
     * @brief Scale a BiomeConfig for the target map size.
     *
     * Scales:
     * - baseFeatureScale: Inversely with map size (consistent cluster scale)
     * - minClusterRadius: With sqrt (perceptually consistent)
     * - groveWaterProximityMax: With sqrt
     * - mireWaterProximityMin: With sqrt
     * - mireMinPatchDistance: Linearly with map size
     *
     * @param baseConfig The base configuration (tuned for 256x256).
     * @param size The target map size.
     * @return Scaled configuration for the target map size.
     */
    static BiomeConfig scaleBiomeConfig(
        const BiomeConfig& baseConfig,
        MapSize size
    ) {
        BiomeConfig scaled = baseConfig;

        float invFactor = getInverseLinearFactor(size);
        float linearFactor = getLinearFactor(size);
        float sqrtFactor = getSqrtFactor(size);

        // Scale noise frequency inversely with map size
        scaled.baseFeatureScale = baseConfig.baseFeatureScale * invFactor;

        // Scale cluster radius with sqrt for perceptually consistent clusters
        scaled.minClusterRadius = static_cast<std::uint8_t>(
            std::max(1.0f, baseConfig.minClusterRadius * sqrtFactor)
        );

        // Scale water proximity thresholds with sqrt
        scaled.groveWaterProximityMax = static_cast<std::uint8_t>(
            std::max(2.0f, baseConfig.groveWaterProximityMax * sqrtFactor)
        );
        scaled.mireWaterProximityMin = static_cast<std::uint8_t>(
            std::max(1.0f, baseConfig.mireWaterProximityMin * sqrtFactor)
        );

        // Scale mire patch distance linearly (expansion path spacing)
        scaled.mireMinPatchDistance = static_cast<std::uint8_t>(
            std::max(4.0f, baseConfig.mireMinPatchDistance * linearFactor)
        );

        // Mire patches per edge scales with sqrt
        scaled.mireMaxPatchesPerEdge = static_cast<std::uint8_t>(
            std::max(1.0f, baseConfig.mireMaxPatchesPerEdge * sqrtFactor)
        );

        return scaled;
    }

    // =========================================================================
    // Convenience Factory Methods
    // =========================================================================

    /**
     * @brief Create a default ElevationConfig scaled for the target map size.
     *
     * @param size The target map size.
     * @return Properly scaled ElevationConfig.
     */
    static ElevationConfig createElevationConfig(MapSize size) {
        return scaleElevationConfig(ElevationConfig::defaultConfig(), size);
    }

    /**
     * @brief Create a default WaterBodyConfig scaled for the target map size.
     *
     * @param size The target map size.
     * @return Properly scaled WaterBodyConfig.
     */
    static WaterBodyConfig createWaterBodyConfig(MapSize size) {
        return scaleWaterBodyConfig(WaterBodyConfig::defaultConfig(), size);
    }

    /**
     * @brief Create a default BiomeConfig scaled for the target map size.
     *
     * @param size The target map size.
     * @return Properly scaled BiomeConfig.
     */
    static BiomeConfig createBiomeConfig(MapSize size) {
        return scaleBiomeConfig(BiomeConfig::defaultConfig(), size);
    }

    // =========================================================================
    // Preset Configurations Scaled for Map Size
    // =========================================================================

    /**
     * @brief Create a mountainous ElevationConfig scaled for the target map size.
     */
    static ElevationConfig createMountainousElevationConfig(MapSize size) {
        return scaleElevationConfig(ElevationConfig::mountainous(), size);
    }

    /**
     * @brief Create a plains ElevationConfig scaled for the target map size.
     */
    static ElevationConfig createPlainsElevationConfig(MapSize size) {
        return scaleElevationConfig(ElevationConfig::plains(), size);
    }

    /**
     * @brief Create a rolling ElevationConfig scaled for the target map size.
     */
    static ElevationConfig createRollingElevationConfig(MapSize size) {
        return scaleElevationConfig(ElevationConfig::rolling(), size);
    }

    /**
     * @brief Create an island WaterBodyConfig scaled for the target map size.
     */
    static WaterBodyConfig createIslandWaterBodyConfig(MapSize size) {
        return scaleWaterBodyConfig(WaterBodyConfig::island(), size);
    }

    /**
     * @brief Create a river-heavy WaterBodyConfig scaled for the target map size.
     */
    static WaterBodyConfig createRiverHeavyWaterBodyConfig(MapSize size) {
        return scaleWaterBodyConfig(WaterBodyConfig::riverHeavy(), size);
    }

    /**
     * @brief Create a lake-heavy WaterBodyConfig scaled for the target map size.
     */
    static WaterBodyConfig createLakeHeavyWaterBodyConfig(MapSize size) {
        return scaleWaterBodyConfig(WaterBodyConfig::lakeHeavy(), size);
    }

    /**
     * @brief Create an arid WaterBodyConfig scaled for the target map size.
     */
    static WaterBodyConfig createAridWaterBodyConfig(MapSize size) {
        return scaleWaterBodyConfig(WaterBodyConfig::arid(), size);
    }

    /**
     * @brief Create a lush BiomeConfig scaled for the target map size.
     */
    static BiomeConfig createLushBiomeConfig(MapSize size) {
        return scaleBiomeConfig(BiomeConfig::lush(), size);
    }

    /**
     * @brief Create a volcanic BiomeConfig scaled for the target map size.
     */
    static BiomeConfig createVolcanicBiomeConfig(MapSize size) {
        return scaleBiomeConfig(BiomeConfig::volcanic(), size);
    }

    /**
     * @brief Create a crystalline BiomeConfig scaled for the target map size.
     */
    static BiomeConfig createCrystallineBiomeConfig(MapSize size) {
        return scaleBiomeConfig(BiomeConfig::crystalline(), size);
    }

    // =========================================================================
    // Validation and Debugging
    // =========================================================================

    /**
     * @brief Get the expected feature count ratio between two map sizes.
     *
     * Useful for validating that scaled generation produces proportional
     * feature counts.
     *
     * @param from Source map size.
     * @param to Target map size.
     * @return Expected feature count multiplier (e.g., 4.0 from Medium to Large).
     */
    static float getExpectedFeatureRatio(MapSize from, MapSize to) {
        float fromArea = getAreaFactor(from);
        float toArea = getAreaFactor(to);
        return toArea / fromArea;
    }

    /**
     * @brief Validate that a feature count is within expected range after scaling.
     *
     * Checks that scaledCount is approximately equal to baseCount * areaRatio,
     * with tolerance for random variation.
     *
     * @param baseCount Feature count on reference (256x256) map.
     * @param scaledCount Feature count on target map.
     * @param size Target map size.
     * @param tolerance Acceptable deviation (default 0.5 = 50%).
     * @return true if scaledCount is within expected range.
     */
    static bool validateFeatureCount(
        std::uint32_t baseCount,
        std::uint32_t scaledCount,
        MapSize size,
        float tolerance = 0.5f
    ) {
        float areaRatio = getAreaFactor(size);
        float expected = static_cast<float>(baseCount) * areaRatio;
        float lower = expected * (1.0f - tolerance);
        float upper = expected * (1.0f + tolerance);
        return static_cast<float>(scaledCount) >= lower &&
               static_cast<float>(scaledCount) <= upper;
    }
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_MAPSIZESCALING_H
