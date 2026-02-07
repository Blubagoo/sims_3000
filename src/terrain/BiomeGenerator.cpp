/**
 * @file BiomeGenerator.cpp
 * @brief Implementation of alien biome distribution generation.
 *
 * Uses noise-based clustering with elevation and proximity rules to place
 * alien biomes in ecologically plausible patterns.
 *
 * Generation is single-threaded for deterministic output.
 * Compile with strict FP semantics for cross-platform determinism:
 * - MSVC: /fp:strict
 * - GCC/Clang: -ffp-contract=off
 */

#include "sims3000/terrain/BiomeGenerator.h"
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

/**
 * @brief Check if a terrain type is a special biome (already placed).
 */
static bool isSpecialBiome(TerrainType type) {
    return type == TerrainType::BiolumeGrove ||
           type == TerrainType::PrismaFields ||
           type == TerrainType::SporeFlats ||
           type == TerrainType::BlightMires ||
           type == TerrainType::EmberCrust;
}

// =============================================================================
// BiomeGenerator Public Methods
// =============================================================================

bool BiomeGenerator::isEligibleForBiome(TerrainType type) {
    // Only Substrate and Ridge tiles can have biomes placed on them
    // (Ridge is converted to other types like EmberCrust or PrismaFields)
    return type == TerrainType::Substrate || type == TerrainType::Ridge;
}

float BiomeGenerator::sampleBiomeNoise(
    const SimplexNoise& noise,
    float x,
    float y,
    const BiomeConfig& config,
    std::int32_t seedOffset)
{
    // Build noise config for biome sampling
    NoiseConfig noiseConfig;
    noiseConfig.octaves = config.octaves;
    noiseConfig.persistence = config.persistence;
    noiseConfig.amplitude = 1.0f;
    noiseConfig.scale = config.baseFeatureScale;
    noiseConfig.lacunarity = 2.0f;
    noiseConfig.seed_offset = seedOffset;

    // Get normalized fBm value (0.0 to 1.0)
    return noise.fbm2DNormalized(x, y, noiseConfig);
}

bool BiomeGenerator::isEmberElevation(std::uint8_t elevation, const BiomeConfig& config) {
    return elevation >= config.volcanicMinElevation;
}

bool BiomeGenerator::isPrismaElevation(std::uint8_t elevation, const BiomeConfig& config) {
    return elevation >= config.ridgeMinElevation;
}

bool BiomeGenerator::isLowlandElevation(std::uint8_t elevation, const BiomeConfig& config) {
    return elevation <= config.lowlandMaxElevation;
}

bool BiomeGenerator::isNearWater(
    const WaterDistanceField& waterDist,
    std::uint16_t x,
    std::uint16_t y,
    const BiomeConfig& config)
{
    std::uint8_t distance = waterDist.get_water_distance(x, y);
    return distance > 0 && distance <= config.groveWaterProximityMax;
}

bool BiomeGenerator::isAdjacentToType(
    const TerrainGrid& grid,
    std::uint16_t x,
    std::uint16_t y,
    TerrainType type)
{
    // 4-connected neighbor offsets
    constexpr std::int16_t dx[4] = { 0, 1, 0, -1 };
    constexpr std::int16_t dy[4] = { -1, 0, 1, 0 };

    for (int i = 0; i < 4; ++i) {
        std::int32_t nx = static_cast<std::int32_t>(x) + dx[i];
        std::int32_t ny = static_cast<std::int32_t>(y) + dy[i];

        if (grid.in_bounds(nx, ny)) {
            if (grid.at(static_cast<std::uint16_t>(nx),
                       static_cast<std::uint16_t>(ny)).getTerrainType() == type) {
                return true;
            }
        }
    }
    return false;
}

bool BiomeGenerator::isTransitionalZone(
    const TerrainGrid& grid,
    std::uint16_t x,
    std::uint16_t y)
{
    // Transitional zones are adjacent to BiolumeGrove but on Substrate
    const TerrainComponent& tile = grid.at(x, y);
    if (tile.getTerrainType() != TerrainType::Substrate) {
        return false;
    }

    return isAdjacentToType(grid, x, y, TerrainType::BiolumeGrove);
}

// =============================================================================
// BiomeGenerator Private Placement Methods
// =============================================================================

std::uint32_t BiomeGenerator::placeEmberCrust(
    TerrainGrid& grid,
    const SimplexNoise& noise,
    const BiomeConfig& config)
{
    std::uint32_t count = 0;

    // Use a separate noise instance with ember seed offset
    SimplexNoise emberNoise(noise.getSeed() + static_cast<std::uint64_t>(config.emberSeedOffset));

    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            TerrainComponent& tile = grid.at(x, y);

            // Only place on eligible tiles (Ridge at high elevation)
            if (!isEligibleForBiome(tile.getTerrainType())) {
                continue;
            }

            // Check elevation requirement
            if (!isEmberElevation(tile.getElevation(), config)) {
                continue;
            }

            // Sample noise for this biome
            float noiseValue = sampleBiomeNoise(
                emberNoise,
                static_cast<float>(x),
                static_cast<float>(y),
                config,
                0  // Already using offset in seed
            );

            // Place if noise exceeds threshold
            if (noiseValue >= config.emberNoiseThreshold) {
                tile.setTerrainType(TerrainType::EmberCrust);
                ++count;
            }
        }
    }

    return count;
}

std::uint32_t BiomeGenerator::placePrismaFields(
    TerrainGrid& grid,
    const SimplexNoise& noise,
    const BiomeConfig& config)
{
    std::uint32_t count = 0;

    // Use a separate noise instance with prisma seed offset
    SimplexNoise prismaNoise(noise.getSeed() + static_cast<std::uint64_t>(config.prismaSeedOffset));

    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            TerrainComponent& tile = grid.at(x, y);

            // Only place on eligible tiles
            if (!isEligibleForBiome(tile.getTerrainType())) {
                continue;
            }

            // Skip if already assigned a biome
            if (isSpecialBiome(tile.getTerrainType())) {
                continue;
            }

            // Check elevation requirement (ridgelines and plateaus)
            if (!isPrismaElevation(tile.getElevation(), config)) {
                continue;
            }

            // Sample noise for this biome (higher threshold = rarer)
            float noiseValue = sampleBiomeNoise(
                prismaNoise,
                static_cast<float>(x),
                static_cast<float>(y),
                config,
                0
            );

            // Place if noise exceeds threshold (higher threshold for rarity)
            if (noiseValue >= config.prismaNoiseThreshold) {
                tile.setTerrainType(TerrainType::PrismaFields);
                ++count;
            }
        }
    }

    return count;
}

std::uint32_t BiomeGenerator::placeBiolumeGrove(
    TerrainGrid& grid,
    const WaterDistanceField& waterDist,
    const SimplexNoise& noise,
    const BiomeConfig& config)
{
    std::uint32_t count = 0;

    // Use a separate noise instance with grove seed offset
    SimplexNoise groveNoise(noise.getSeed() + static_cast<std::uint64_t>(config.groveSeedOffset));

    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            TerrainComponent& tile = grid.at(x, y);

            // Only place on Substrate (not Ridge, not water, not special biomes)
            if (tile.getTerrainType() != TerrainType::Substrate) {
                continue;
            }

            // BiolumeGrove placement has two conditions:
            // 1. Lowland areas
            // 2. Near flow channel banks (water proximity)

            bool isLowland = isLowlandElevation(tile.getElevation(), config);
            bool nearWater = isNearWater(waterDist, x, y, config);

            // Must satisfy at least one condition
            if (!isLowland && !nearWater) {
                continue;
            }

            // Sample noise for this biome
            float noiseValue = sampleBiomeNoise(
                groveNoise,
                static_cast<float>(x),
                static_cast<float>(y),
                config,
                0
            );

            // Boost probability near water (groves cluster along banks)
            float threshold = config.groveNoiseThreshold;
            if (nearWater) {
                // Lower threshold near water = higher chance of placement
                threshold -= 0.1f;
            }

            // Place if noise exceeds threshold
            if (noiseValue >= threshold) {
                tile.setTerrainType(TerrainType::BiolumeGrove);
                ++count;
            }
        }
    }

    return count;
}

std::uint32_t BiomeGenerator::placeBlightMires(
    TerrainGrid& grid,
    const WaterDistanceField& waterDist,
    const SimplexNoise& noise,
    const BiomeConfig& config)
{
    std::uint32_t count = 0;

    // Use a separate noise instance with mire seed offset
    SimplexNoise mireNoise(noise.getSeed() + static_cast<std::uint64_t>(config.mireSeedOffset));

    // Use a second noise layer for ensuring gaps between mire patches
    SimplexNoise gapNoise(noise.getSeed() + static_cast<std::uint64_t>(config.mireSeedOffset) + 100);

    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            TerrainComponent& tile = grid.at(x, y);

            // Only place on Substrate (not Ridge, not water, not special biomes)
            if (tile.getTerrainType() != TerrainType::Substrate) {
                continue;
            }

            // BlightMires only in lowlands
            if (!isLowlandElevation(tile.getElevation(), config)) {
                continue;
            }

            // Mires should not be immediately adjacent to water
            std::uint8_t waterDist_val = waterDist.get_water_distance(x, y);
            if (waterDist_val < config.mireWaterProximityMin) {
                continue;
            }

            // Sample noise for this biome
            float noiseValue = sampleBiomeNoise(
                mireNoise,
                static_cast<float>(x),
                static_cast<float>(y),
                config,
                0
            );

            // Additional gap noise to ensure expansion paths
            // Use larger scale for more coherent gaps
            NoiseConfig gapConfig;
            gapConfig.octaves = 2;
            gapConfig.persistence = 0.5f;
            gapConfig.amplitude = 1.0f;
            gapConfig.scale = config.baseFeatureScale * 0.3f;  // Larger features
            gapConfig.lacunarity = 2.0f;
            gapConfig.seed_offset = 100;

            float gapValue = gapNoise.fbm2DNormalized(
                static_cast<float>(x),
                static_cast<float>(y),
                gapConfig
            );

            // Create gaps in mire placement (ensures expansion paths)
            // High gap values block mire placement
            if (gapValue > 0.7f) {
                continue;
            }

            // Place if noise exceeds threshold
            if (noiseValue >= config.mireNoiseThreshold) {
                tile.setTerrainType(TerrainType::BlightMires);
                ++count;
            }
        }
    }

    return count;
}

std::uint32_t BiomeGenerator::placeSporeFlats(
    TerrainGrid& grid,
    const SimplexNoise& noise,
    const BiomeConfig& config)
{
    std::uint32_t count = 0;

    // Use a separate noise instance with spore seed offset
    SimplexNoise sporeNoise(noise.getSeed() + static_cast<std::uint64_t>(config.sporeSeedOffset));

    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            TerrainComponent& tile = grid.at(x, y);

            // Only place on Substrate
            if (tile.getTerrainType() != TerrainType::Substrate) {
                continue;
            }

            // SporeFlats prefer transitional zones (adjacent to groves)
            bool isTransitional = isTransitionalZone(grid, x, y);

            // Also allow in mid-elevation areas
            std::uint8_t elevation = tile.getElevation();
            bool isMidElevation = elevation > config.lowlandMaxElevation &&
                                   elevation < config.highlandMinElevation;

            // Must satisfy at least one condition
            if (!isTransitional && !isMidElevation) {
                continue;
            }

            // Sample noise for this biome
            float noiseValue = sampleBiomeNoise(
                sporeNoise,
                static_cast<float>(x),
                static_cast<float>(y),
                config,
                0
            );

            // Boost probability in transitional zones
            float threshold = config.sporeNoiseThreshold;
            if (isTransitional) {
                threshold -= 0.12f;  // Higher boost for transitional zones
            }

            // Place if noise exceeds threshold
            if (noiseValue >= threshold) {
                tile.setTerrainType(TerrainType::SporeFlats);
                ++count;
            }
        }
    }

    return count;
}

// =============================================================================
// BiomeGenerator Main Generation Method
// =============================================================================

BiomeResult BiomeGenerator::generate(
    TerrainGrid& grid,
    const WaterDistanceField& waterDist,
    std::uint64_t seed,
    const BiomeConfig& config)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    BiomeResult result{};
    result.totalTiles = static_cast<std::uint32_t>(grid.tile_count());

    // Initialize noise generator with base seed
    SimplexNoise noise(seed);

    // Count initial state (water and ridge tiles)
    result.waterCount = 0;
    result.ridgeCount = 0;
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            TerrainType type = grid.at(x, y).getTerrainType();
            if (isWaterType(type)) {
                ++result.waterCount;
            } else if (type == TerrainType::Ridge) {
                ++result.ridgeCount;
            }
        }
    }

    // Place biomes in order of priority (highest elevation first)
    // This ensures proper layering and prevents overlap conflicts

    // 1. EmberCrust - volcanic ridges at highest elevations
    result.emberCount = placeEmberCrust(grid, noise, config);

    // 2. PrismaFields - ridgelines and plateaus (rarest)
    result.prismaCount = placePrismaFields(grid, noise, config);

    // 3. BiolumeGrove - lowlands and water banks
    result.groveCount = placeBiolumeGrove(grid, waterDist, noise, config);

    // 4. BlightMires - lowlands with expansion safety
    result.mireCount = placeBlightMires(grid, waterDist, noise, config);

    // 5. SporeFlats - transitional zones (placed last to fill gaps)
    result.sporeCount = placeSporeFlats(grid, noise, config);

    // Count remaining substrate and update ridge count
    result.substrateCount = 0;
    result.ridgeCount = 0;  // Recount as some ridges were converted
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            TerrainType type = grid.at(x, y).getTerrainType();
            if (type == TerrainType::Substrate) {
                ++result.substrateCount;
            } else if (type == TerrainType::Ridge) {
                ++result.ridgeCount;
            }
        }
    }

    // Calculate land tiles (non-water)
    result.landTiles = result.totalTiles - result.waterCount;

    // Calculate coverage percentages
    float landFloat = static_cast<float>(result.landTiles);
    if (landFloat > 0.0f) {
        result.groveCoverage = static_cast<float>(result.groveCount) / landFloat * 100.0f;
        result.prismaCoverage = static_cast<float>(result.prismaCount) / landFloat * 100.0f;
        result.sporeCoverage = static_cast<float>(result.sporeCount) / landFloat * 100.0f;
        result.mireCoverage = static_cast<float>(result.mireCount) / landFloat * 100.0f;
        result.emberCoverage = static_cast<float>(result.emberCount) / landFloat * 100.0f;
        result.substrateCoverage = static_cast<float>(result.substrateCount) / landFloat * 100.0f;
    } else {
        result.groveCoverage = 0.0f;
        result.prismaCoverage = 0.0f;
        result.sporeCoverage = 0.0f;
        result.mireCoverage = 0.0f;
        result.emberCoverage = 0.0f;
        result.substrateCoverage = 0.0f;
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
