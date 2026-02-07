/**
 * @file ElevationGenerator.cpp
 * @brief Implementation of elevation heightmap generation.
 *
 * Uses multi-octave simplex noise (fBm) to generate natural-looking
 * terrain with coherent ridgelines and valleys.
 *
 * Generation is single-threaded for deterministic output.
 * Compile with strict FP semantics for cross-platform determinism:
 * - MSVC: /fp:strict
 * - GCC/Clang: -ffp-contract=off
 */

#include "sims3000/terrain/ElevationGenerator.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace sims3000 {
namespace terrain {

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * @brief Check if a terrain type is water (should not be overwritten).
 */
static bool isWaterType(TerrainType type) {
    return type == TerrainType::DeepVoid ||
           type == TerrainType::FlowChannel ||
           type == TerrainType::StillBasin;
}

// =============================================================================
// ElevationGenerator Implementation
// =============================================================================

float ElevationGenerator::sampleRawElevation(
    const SimplexNoise& noise,
    float x,
    float y,
    const ElevationConfig& config)
{
    // Build noise config from elevation config
    NoiseConfig noiseConfig;
    noiseConfig.octaves = config.octaves;
    noiseConfig.persistence = config.roughness;
    noiseConfig.amplitude = config.amplitude;
    noiseConfig.scale = config.featureScale;
    noiseConfig.lacunarity = config.lacunarity;
    noiseConfig.seed_offset = config.seedOffset;

    // Get base fBm noise value (normalized to 0-1)
    float baseValue = noise.fbm2DNormalized(x, y, noiseConfig);

    // Apply ridge enhancement if enabled
    if (config.enhanceRidges && config.ridgeStrength > 0.0f) {
        baseValue = applyRidgeEnhancement(noise, x, y, baseValue, config);
    }

    return baseValue;
}

float ElevationGenerator::applyRidgeEnhancement(
    const SimplexNoise& noise,
    float x,
    float y,
    float baseElevation,
    const ElevationConfig& config)
{
    // Ridged noise: use absolute value of noise centered at 0
    // This creates sharp peaks (ridges) where the noise crosses zero

    // Sample a separate noise layer for ridge pattern
    NoiseConfig ridgeConfig;
    ridgeConfig.octaves = std::max(static_cast<std::uint8_t>(3),
                                    static_cast<std::uint8_t>(config.octaves - 1));
    ridgeConfig.persistence = config.roughness * 0.8f;
    ridgeConfig.amplitude = 1.0f;
    ridgeConfig.scale = config.featureScale * 0.7f;  // Larger features for ridges
    ridgeConfig.lacunarity = config.lacunarity;
    ridgeConfig.seed_offset = config.seedOffset + 500;  // Different offset for ridge layer

    // Get ridge noise value (returns -1 to 1 range before normalization)
    float ridgeNoise = noise.fbm2D(x, y, ridgeConfig);

    // Convert to ridged noise: peaks at 0, valleys at +/-1
    // 1 - abs(noise) creates ridges
    float ridgeValue = 1.0f - std::abs(ridgeNoise);

    // Square the ridge value to sharpen the peaks
    ridgeValue = ridgeValue * ridgeValue;

    // Blend base elevation with ridge pattern
    float result = baseElevation * (1.0f - config.ridgeStrength) +
                   ridgeValue * config.ridgeStrength;

    // Add some of the ridge pattern to high elevations for sharper peaks
    if (baseElevation > 0.6f) {
        float highBlend = (baseElevation - 0.6f) / 0.4f;
        result += ridgeValue * highBlend * config.ridgeStrength * 0.5f;
    }

    return std::clamp(result, 0.0f, 1.0f);
}

std::uint8_t ElevationGenerator::rawToElevation(
    float rawValue,
    const ElevationConfig& config)
{
    // Clamp raw value to valid range
    rawValue = std::clamp(rawValue, 0.0f, 1.0f);

    // Map to elevation range
    float range = static_cast<float>(config.maxElevation - config.minElevation);
    float scaled = rawValue * range + static_cast<float>(config.minElevation);

    // Clamp to absolute limits (0-31)
    scaled = std::clamp(scaled, 0.0f, 31.0f);

    return static_cast<std::uint8_t>(scaled);
}

bool ElevationGenerator::isRidge(
    std::uint8_t elevation,
    const ElevationConfig& config)
{
    return elevation >= config.ridgeThreshold;
}

ElevationResult ElevationGenerator::generate(
    TerrainGrid& grid,
    std::uint64_t seed,
    const ElevationConfig& config)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    ElevationResult result{};
    result.minElevation = 255;  // Will be updated
    result.maxElevation = 0;
    result.meanElevation = 0.0f;
    result.ridgeTileCount = 0;
    result.totalTiles = grid.tile_count();

    // Initialize noise generator with seed
    SimplexNoise noise(seed);

    // Accumulator for mean calculation
    std::uint64_t elevationSum = 0;

    // Generate elevation for each tile in row-major order
    // Row-major: iterate y (rows) first, then x (columns) within each row
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            TerrainComponent& tile = grid.at(x, y);

            // Skip water tiles (preserve previously set water)
            if (isWaterType(tile.getTerrainType())) {
                continue;
            }

            // Sample raw elevation value
            float rawElevation = sampleRawElevation(
                noise,
                static_cast<float>(x),
                static_cast<float>(y),
                config
            );

            // Convert to discrete elevation level
            std::uint8_t elevation = rawToElevation(rawElevation, config);

            // Set elevation on tile
            tile.setElevation(elevation);

            // Update statistics
            if (elevation < result.minElevation) {
                result.minElevation = elevation;
            }
            if (elevation > result.maxElevation) {
                result.maxElevation = elevation;
            }
            elevationSum += elevation;

            // Assign terrain type based on elevation
            if (isRidge(elevation, config)) {
                tile.setTerrainType(TerrainType::Ridge);
                ++result.ridgeTileCount;
            } else {
                tile.setTerrainType(TerrainType::Substrate);
            }
        }
    }

    // Calculate mean elevation
    if (result.totalTiles > 0) {
        result.meanElevation = static_cast<float>(elevationSum) /
                               static_cast<float>(result.totalTiles);
    }

    // Calculate generation time
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime);
    result.generationTimeMs = static_cast<float>(duration.count()) / 1000.0f;

    return result;
}

} // namespace terrain
} // namespace sims3000
