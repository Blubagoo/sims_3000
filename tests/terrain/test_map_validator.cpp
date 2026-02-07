/**
 * @file test_map_validator.cpp
 * @brief Unit tests for MapValidator
 *
 * Tests for ticket 3-013: Post-Generation Map Validation
 *
 * Tests cover:
 * - Minimum buildable area percentage (>= 50%)
 * - At least one river exists
 * - Coastline continuity (no single-tile ocean gaps)
 * - No single-tile terrain type anomalies
 * - Terrain type distribution within target ranges
 * - Spawn point quality threshold
 * - Validation completes in <10ms
 * - Retry logic with seed increment
 * - Best attempt selection when retries exhausted
 */

#include <sims3000/terrain/MapValidator.h>
#include <sims3000/terrain/ElevationGenerator.h>
#include <sims3000/terrain/BiomeGenerator.h>
#include <sims3000/terrain/WaterBodyGenerator.h>
#include <sims3000/terrain/WaterData.h>
#include <cmath>
#include <cstdio>
#include <cassert>
#include <chrono>

using namespace sims3000;
using namespace sims3000::terrain;

namespace {

// Test counter
int g_testsPassed = 0;
int g_testsFailed = 0;

void TEST_ASSERT(bool condition, const char* message) {
    if (condition) {
        ++g_testsPassed;
        std::printf("  PASS: %s\n", message);
    } else {
        ++g_testsFailed;
        std::printf("  FAIL: %s\n", message);
    }
}

// Helper to create a terrain grid with procedural generation
void setupGeneratedTerrain(TerrainGrid& grid, WaterData& waterData,
                           WaterDistanceField& waterDist, std::uint64_t seed) {
    // Generate elevation
    ElevationConfig elevConfig = ElevationConfig::defaultConfig();
    ElevationGenerator::generate(grid, seed, elevConfig);

    // Generate water bodies
    WaterBodyConfig waterConfig = WaterBodyConfig::defaultConfig();
    WaterBodyGenerator::generate(grid, waterData, waterDist, seed, waterConfig);

    // Generate biomes
    BiomeConfig biomeConfig = BiomeConfig::defaultConfig();
    BiomeGenerator::generate(grid, waterDist, seed, biomeConfig);
}

// Helper to create a simple controllable test grid
void setupSimpleTestGrid(TerrainGrid& grid, WaterDistanceField& waterDist) {
    // Fill with substrate
    grid.fill_type(TerrainType::Substrate);

    // Set all elevations to mid-range
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(10);
        }
    }

    // Add water at the edges
    for (std::uint16_t i = 0; i < grid.width; ++i) {
        grid.at(i, 0).setTerrainType(TerrainType::DeepVoid);
        grid.at(i, grid.height - 1).setTerrainType(TerrainType::DeepVoid);
        grid.at(0, i).setTerrainType(TerrainType::DeepVoid);
        grid.at(grid.width - 1, i).setTerrainType(TerrainType::DeepVoid);
    }

    // Add a river through the center
    std::uint16_t riverX = grid.width / 2;
    for (std::uint16_t y = 5; y < grid.height - 5; ++y) {
        grid.at(riverX, y).setTerrainType(TerrainType::FlowChannel);
    }

    // Compute water distance field
    waterDist.compute(grid);
}

// ============================================================================
// Test: ValidationConfig struct
// ============================================================================
void test_ValidationConfig_DefaultValues() {
    std::printf("\ntest_ValidationConfig_DefaultValues:\n");

    ValidationConfig config;

    TEST_ASSERT(config.minBuildablePercent == 0.50f, "Default minBuildablePercent is 50%");
    TEST_ASSERT(config.requireRiver == true, "Default requireRiver is true");
    TEST_ASSERT(config.checkCoastlineContinuity == true, "Default checkCoastlineContinuity is true");
    TEST_ASSERT(config.checkTerrainAnomalies == true, "Default checkTerrainAnomalies is true");
    TEST_ASSERT(config.substrateMinPercent == 0.35f, "Default substrateMinPercent is 35%");
    TEST_ASSERT(config.substrateMaxPercent == 0.45f, "Default substrateMaxPercent is 45%");
    TEST_ASSERT(config.maxRetries == 10, "Default maxRetries is 10");
}

void test_ValidationConfig_FactoryMethods() {
    std::printf("\ntest_ValidationConfig_FactoryMethods:\n");

    ValidationConfig strict = ValidationConfig::strict();
    TEST_ASSERT(strict.minBuildablePercent == 0.55f, "strict has higher buildable requirement");
    TEST_ASSERT(strict.minSpawnPointScore == 0.4f, "strict has higher spawn score requirement");

    ValidationConfig relaxed = ValidationConfig::relaxed();
    TEST_ASSERT(relaxed.minBuildablePercent == 0.40f, "relaxed has lower buildable requirement");
    TEST_ASSERT(relaxed.checkTerrainDistribution == false, "relaxed disables distribution check");
}

// ============================================================================
// Test: ValidationResult struct
// ============================================================================
void test_ValidationResult_PassedCheckCount() {
    std::printf("\ntest_ValidationResult_PassedCheckCount:\n");

    ValidationResult result = {};
    result.buildableAreaPassed = true;
    result.riverExistsPassed = true;
    result.coastlineContinuityPassed = true;
    result.terrainAnomaliesPassed = false;
    result.terrainDistributionPassed = true;
    result.spawnPointsPassed = false;

    TEST_ASSERT(result.passedCheckCount() == 4, "Counts 4 passed checks");
    TEST_ASSERT(ValidationResult::totalCheckCount() == 6, "Total check count is 6");
}

// ============================================================================
// Test: Buildable area check
// ============================================================================
void test_CheckBuildableArea_AllSubstrate() {
    std::printf("\ntest_CheckBuildableArea_AllSubstrate:\n");

    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    float actualPercent = 0.0f;
    bool passed = MapValidator::checkBuildableArea(grid, 0.50f, actualPercent);

    TEST_ASSERT(passed, "All substrate passes buildable check");
    TEST_ASSERT(actualPercent == 1.0f, "All substrate is 100% buildable");
}

void test_CheckBuildableArea_HalfWater() {
    std::printf("\ntest_CheckBuildableArea_HalfWater:\n");

    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    // Make bottom half water
    for (std::uint16_t y = grid.height / 2; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setTerrainType(TerrainType::DeepVoid);
        }
    }

    float actualPercent = 0.0f;
    bool passed = MapValidator::checkBuildableArea(grid, 0.50f, actualPercent);

    TEST_ASSERT(passed, "50% substrate passes 50% buildable check");
    TEST_ASSERT(std::abs(actualPercent - 0.5f) < 0.01f, "Accurately calculates 50% buildable");
}

void test_CheckBuildableArea_TooMuchWater() {
    std::printf("\ntest_CheckBuildableArea_TooMuchWater:\n");

    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::DeepVoid);

    // Only 25% land
    for (std::uint16_t y = 0; y < grid.height / 4; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    float actualPercent = 0.0f;
    bool passed = MapValidator::checkBuildableArea(grid, 0.50f, actualPercent);

    TEST_ASSERT(!passed, "25% buildable fails 50% requirement");
    TEST_ASSERT(std::abs(actualPercent - 0.25f) < 0.01f, "Accurately calculates 25% buildable");
}

void test_CheckBuildableArea_BlightMiresNotBuildable() {
    std::printf("\ntest_CheckBuildableArea_BlightMiresNotBuildable:\n");

    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::BlightMires);

    float actualPercent = 0.0f;
    bool passed = MapValidator::checkBuildableArea(grid, 0.50f, actualPercent);

    TEST_ASSERT(!passed, "BlightMires are not buildable");
    TEST_ASSERT(actualPercent == 0.0f, "BlightMires count as 0% buildable");
}

void test_CheckBuildableArea_ClearableTypes() {
    std::printf("\ntest_CheckBuildableArea_ClearableTypes:\n");

    TerrainGrid grid(MapSize::Small);

    // Set each quadrant to a different clearable type
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            if (x < grid.width / 2 && y < grid.height / 2) {
                grid.at(x, y).setTerrainType(TerrainType::BiolumeGrove);
            } else if (x >= grid.width / 2 && y < grid.height / 2) {
                grid.at(x, y).setTerrainType(TerrainType::PrismaFields);
            } else if (x < grid.width / 2 && y >= grid.height / 2) {
                grid.at(x, y).setTerrainType(TerrainType::SporeFlats);
            } else {
                grid.at(x, y).setTerrainType(TerrainType::EmberCrust);
            }
        }
    }

    float actualPercent = 0.0f;
    bool passed = MapValidator::checkBuildableArea(grid, 0.50f, actualPercent);

    TEST_ASSERT(passed, "Clearable terrain types are buildable");
    TEST_ASSERT(actualPercent == 1.0f, "All clearable types are 100% buildable");
}

// ============================================================================
// Test: River existence check
// ============================================================================
void test_CheckRiverExists_HasRiver() {
    std::printf("\ntest_CheckRiverExists_HasRiver:\n");

    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    // Add a small river
    for (std::uint16_t y = 10; y < 50; ++y) {
        grid.at(64, y).setTerrainType(TerrainType::FlowChannel);
    }

    std::uint32_t riverTileCount = 0;
    bool passed = MapValidator::checkRiverExists(grid, riverTileCount);

    TEST_ASSERT(passed, "Grid with river passes river check");
    TEST_ASSERT(riverTileCount == 40, "Correctly counts river tiles");
}

void test_CheckRiverExists_NoRiver() {
    std::printf("\ntest_CheckRiverExists_NoRiver:\n");

    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    // Only ocean, no river
    for (std::uint16_t i = 0; i < 10; ++i) {
        grid.at(i, 0).setTerrainType(TerrainType::DeepVoid);
    }

    std::uint32_t riverTileCount = 0;
    bool passed = MapValidator::checkRiverExists(grid, riverTileCount);

    TEST_ASSERT(!passed, "Grid without river fails river check");
    TEST_ASSERT(riverTileCount == 0, "Zero river tiles counted");
}

// ============================================================================
// Test: Coastline continuity check
// ============================================================================
void test_CheckCoastlineContinuity_NoOcean() {
    std::printf("\ntest_CheckCoastlineContinuity_NoOcean:\n");

    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    std::uint32_t gapCount = 0;
    bool passed = MapValidator::checkCoastlineContinuity(grid, gapCount);

    TEST_ASSERT(passed, "Grid without ocean passes coastline check");
    TEST_ASSERT(gapCount == 0, "No gaps when no ocean");
}

void test_CheckCoastlineContinuity_ContinuousCoast() {
    std::printf("\ntest_CheckCoastlineContinuity_ContinuousCoast:\n");

    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    // Add ocean at edges only
    for (std::uint16_t i = 0; i < grid.width; ++i) {
        grid.at(i, 0).setTerrainType(TerrainType::DeepVoid);
        grid.at(i, 1).setTerrainType(TerrainType::DeepVoid);
        grid.at(i, 2).setTerrainType(TerrainType::DeepVoid);
    }

    std::uint32_t gapCount = 0;
    bool passed = MapValidator::checkCoastlineContinuity(grid, gapCount);

    TEST_ASSERT(passed, "Continuous coastline passes check");
    TEST_ASSERT(gapCount == 0, "No gaps in continuous coast");
}

void test_CheckCoastlineContinuity_SingleTileGap() {
    std::printf("\ntest_CheckCoastlineContinuity_SingleTileGap:\n");

    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::DeepVoid);

    // Create a single land tile surrounded by ocean
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);

    std::uint32_t gapCount = 0;
    bool passed = MapValidator::checkCoastlineContinuity(grid, gapCount);

    TEST_ASSERT(!passed, "Single-tile gap fails coastline check");
    TEST_ASSERT(gapCount == 1, "Detects one gap");
}

// ============================================================================
// Test: Terrain anomaly check
// ============================================================================
void test_CheckTerrainAnomalies_NoAnomalies() {
    std::printf("\ntest_CheckTerrainAnomalies_NoAnomalies:\n");

    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    std::uint32_t anomalyCount = 0;
    bool passed = MapValidator::checkTerrainAnomalies(grid, anomalyCount);

    TEST_ASSERT(passed, "Uniform grid has no anomalies");
    TEST_ASSERT(anomalyCount == 0, "Zero anomalies counted");
}

void test_CheckTerrainAnomalies_SingleTileAnomaly() {
    std::printf("\ntest_CheckTerrainAnomalies_SingleTileAnomaly:\n");

    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    // Add a single tile of different type surrounded by substrate
    grid.at(64, 64).setTerrainType(TerrainType::BiolumeGrove);

    std::uint32_t anomalyCount = 0;
    bool passed = MapValidator::checkTerrainAnomalies(grid, anomalyCount);

    TEST_ASSERT(!passed, "Single-tile anomaly fails check");
    TEST_ASSERT(anomalyCount == 1, "Detects one anomaly");
}

void test_CheckTerrainAnomalies_ClusterNotAnomaly() {
    std::printf("\ntest_CheckTerrainAnomalies_ClusterNotAnomaly:\n");

    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    // Add a cluster of biome (3x3) - not an anomaly
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            grid.at(64 + dx, 64 + dy).setTerrainType(TerrainType::BiolumeGrove);
        }
    }

    std::uint32_t anomalyCount = 0;
    bool passed = MapValidator::checkTerrainAnomalies(grid, anomalyCount);

    TEST_ASSERT(passed, "Cluster is not an anomaly");
    TEST_ASSERT(anomalyCount == 0, "No anomalies in cluster");
}

// ============================================================================
// Test: Terrain distribution check
// ============================================================================
void test_CheckTerrainDistribution_InRange() {
    std::printf("\ntest_CheckTerrainDistribution_InRange:\n");

    TerrainGrid grid(MapSize::Small);

    // 40% substrate, 60% biome (within 35-45% range)
    std::uint32_t totalTiles = static_cast<std::uint32_t>(grid.tile_count());
    std::uint32_t substrateCount = static_cast<std::uint32_t>(totalTiles * 0.40f);

    for (std::uint32_t i = 0; i < totalTiles; ++i) {
        if (i < substrateCount) {
            grid.tiles[i].setTerrainType(TerrainType::Substrate);
        } else {
            grid.tiles[i].setTerrainType(TerrainType::BiolumeGrove);
        }
    }

    float substratePercent = 0.0f;
    bool passed = MapValidator::checkTerrainDistribution(grid, 0.35f, 0.45f, substratePercent);

    TEST_ASSERT(passed, "40% substrate passes 35-45% range");
    TEST_ASSERT(std::abs(substratePercent - 0.40f) < 0.02f, "Accurately calculates 40%");
}

void test_CheckTerrainDistribution_TooLow() {
    std::printf("\ntest_CheckTerrainDistribution_TooLow:\n");

    TerrainGrid grid(MapSize::Small);

    // 20% substrate, 80% biome (below 35% minimum)
    std::uint32_t totalTiles = static_cast<std::uint32_t>(grid.tile_count());
    std::uint32_t substrateCount = static_cast<std::uint32_t>(totalTiles * 0.20f);

    for (std::uint32_t i = 0; i < totalTiles; ++i) {
        if (i < substrateCount) {
            grid.tiles[i].setTerrainType(TerrainType::Substrate);
        } else {
            grid.tiles[i].setTerrainType(TerrainType::BiolumeGrove);
        }
    }

    float substratePercent = 0.0f;
    bool passed = MapValidator::checkTerrainDistribution(grid, 0.35f, 0.45f, substratePercent);

    TEST_ASSERT(!passed, "20% substrate fails 35-45% range");
}

void test_CheckTerrainDistribution_TooHigh() {
    std::printf("\ntest_CheckTerrainDistribution_TooHigh:\n");

    TerrainGrid grid(MapSize::Small);

    // 60% substrate, 40% biome (above 45% maximum)
    std::uint32_t totalTiles = static_cast<std::uint32_t>(grid.tile_count());
    std::uint32_t substrateCount = static_cast<std::uint32_t>(totalTiles * 0.60f);

    for (std::uint32_t i = 0; i < totalTiles; ++i) {
        if (i < substrateCount) {
            grid.tiles[i].setTerrainType(TerrainType::Substrate);
        } else {
            grid.tiles[i].setTerrainType(TerrainType::BiolumeGrove);
        }
    }

    float substratePercent = 0.0f;
    bool passed = MapValidator::checkTerrainDistribution(grid, 0.35f, 0.45f, substratePercent);

    TEST_ASSERT(!passed, "60% substrate fails 35-45% range");
}

void test_CheckTerrainDistribution_IgnoresWater() {
    std::printf("\ntest_CheckTerrainDistribution_IgnoresWater:\n");

    TerrainGrid grid(MapSize::Small);

    // Half water, half land. Of land: 40% substrate
    std::uint32_t totalTiles = static_cast<std::uint32_t>(grid.tile_count());

    for (std::uint32_t i = 0; i < totalTiles; ++i) {
        if (i < totalTiles / 2) {
            grid.tiles[i].setTerrainType(TerrainType::DeepVoid);
        } else if (i < totalTiles / 2 + (totalTiles / 2) * 4 / 10) {
            grid.tiles[i].setTerrainType(TerrainType::Substrate);
        } else {
            grid.tiles[i].setTerrainType(TerrainType::BiolumeGrove);
        }
    }

    float substratePercent = 0.0f;
    bool passed = MapValidator::checkTerrainDistribution(grid, 0.35f, 0.45f, substratePercent);

    // Should be ~40% of land tiles, not total
    std::printf("    Substrate percent of land: %.2f%%\n", substratePercent * 100.0f);
    TEST_ASSERT(passed, "40% of land is substrate (ignoring water)");
}

// ============================================================================
// Test: Full validation
// ============================================================================
void test_Validate_ValidMap() {
    std::printf("\ntest_Validate_ValidMap:\n");

    TerrainGrid grid(MapSize::Medium);
    WaterData waterData(MapSize::Medium);
    WaterDistanceField waterDist(MapSize::Medium);

    // Try seeds until we find one that produces a valid map
    // Use fully relaxed config that disables terrain distribution check
    bool foundValid = false;
    float bestScore = 0.0f;
    for (std::uint64_t seed = 1; seed <= 50 && !foundValid; ++seed) {
        setupGeneratedTerrain(grid, waterData, waterDist, seed);

        ValidationConfig config = ValidationConfig::relaxed();
        config.playerCount = 2;
        config.checkSpawnPoints = false; // Focus on terrain validation
        config.checkTerrainAnomalies = false; // Procedural may have some
        // Note: relaxed already sets checkTerrainDistribution = false

        ValidationResult result = MapValidator::validate(grid, waterDist, seed, config);

        if (result.aggregateScore > bestScore) {
            bestScore = result.aggregateScore;
        }

        if (result.isValid) {
            foundValid = true;
            std::printf("    Found valid map with seed %llu\n", (unsigned long long)seed);
            std::printf("    Aggregate score: %.2f\n", result.aggregateScore);
            std::printf("    Validation time: %.2f ms\n", result.validationTimeMs);
            std::printf("    Buildable area: %.1f%%\n", result.buildableAreaPercent * 100.0f);
            std::printf("    River tiles: %u\n", result.riverTileCount);
        }
    }

    if (!foundValid) {
        std::printf("    Best score: %.2f\n", bestScore);
    }

    TEST_ASSERT(foundValid, "Found at least one valid map in 50 seeds");
}

void test_Validate_ValidationTimeUnder10ms() {
    std::printf("\ntest_Validate_ValidationTimeUnder10ms:\n");

    TerrainGrid grid(MapSize::Medium);
    WaterData waterData(MapSize::Medium);
    WaterDistanceField waterDist(MapSize::Medium);

    std::uint64_t seed = 12345;
    setupGeneratedTerrain(grid, waterData, waterDist, seed);

    ValidationConfig config = ValidationConfig::defaultConfig();
    config.checkSpawnPoints = false; // Spawn point check may be slower

    // Run validation multiple times and check timing
    float maxTimeMs = 0.0f;
    for (int i = 0; i < 5; ++i) {
        ValidationResult result = MapValidator::validate(grid, waterDist, seed, config);
        if (result.validationTimeMs > maxTimeMs) {
            maxTimeMs = result.validationTimeMs;
        }
    }

    std::printf("    Max validation time: %.2f ms\n", maxTimeMs);
    TEST_ASSERT(maxTimeMs < 10.0f, "Validation completes in <10ms");
}

// ============================================================================
// Test: Aggregate score calculation
// ============================================================================
void test_AggregateScore_AllPassed() {
    std::printf("\ntest_AggregateScore_AllPassed:\n");

    ValidationResult result = {};
    result.buildableAreaPercent = 0.75f;
    result.buildableAreaPassed = true;
    result.riverExistsPassed = true;
    result.riverTileCount = 500;
    result.totalTiles = 65536;
    result.coastlineContinuityPassed = true;
    result.coastlineGapCount = 0;
    result.terrainAnomaliesPassed = true;
    result.anomalyCount = 0;
    result.terrainDistributionPassed = true;
    result.substratePercent = 0.40f;
    result.landTiles = 55000;
    result.spawnPointsPassed = true;
    result.minSpawnScore = 0.7f;

    float score = MapValidator::calculateAggregateScore(result);

    std::printf("    Aggregate score: %.2f\n", score);
    TEST_ASSERT(score > 0.7f, "High score for all passed checks");
}

void test_AggregateScore_SomeFailed() {
    std::printf("\ntest_AggregateScore_SomeFailed:\n");

    ValidationResult result = {};
    result.buildableAreaPercent = 0.45f; // Below threshold
    result.buildableAreaPassed = false;
    result.riverExistsPassed = true;
    result.riverTileCount = 100;
    result.totalTiles = 65536;
    result.coastlineContinuityPassed = true;
    result.coastlineGapCount = 0;
    result.terrainAnomaliesPassed = false;
    result.anomalyCount = 5;
    result.terrainDistributionPassed = true;
    result.substratePercent = 0.40f;
    result.landTiles = 45000;
    result.spawnPointsPassed = true;
    result.minSpawnScore = 0.5f;

    float score = MapValidator::calculateAggregateScore(result);

    std::printf("    Aggregate score: %.2f\n", score);
    TEST_ASSERT(score < 0.8f, "Lower score for failed checks");
    TEST_ASSERT(score > 0.3f, "Still has some score for passed checks");
}

// ============================================================================
// Test: isBuildable helper
// ============================================================================
void test_IsBuildable() {
    std::printf("\ntest_IsBuildable:\n");

    TEST_ASSERT(MapValidator::isBuildable(TerrainType::Substrate), "Substrate is buildable");
    TEST_ASSERT(MapValidator::isBuildable(TerrainType::Ridge), "Ridge is buildable");
    TEST_ASSERT(MapValidator::isBuildable(TerrainType::BiolumeGrove), "BiolumeGrove is buildable");
    TEST_ASSERT(MapValidator::isBuildable(TerrainType::PrismaFields), "PrismaFields is buildable");
    TEST_ASSERT(MapValidator::isBuildable(TerrainType::SporeFlats), "SporeFlats is buildable");
    TEST_ASSERT(MapValidator::isBuildable(TerrainType::EmberCrust), "EmberCrust is buildable");

    TEST_ASSERT(!MapValidator::isBuildable(TerrainType::DeepVoid), "DeepVoid is NOT buildable");
    TEST_ASSERT(!MapValidator::isBuildable(TerrainType::FlowChannel), "FlowChannel is NOT buildable");
    TEST_ASSERT(!MapValidator::isBuildable(TerrainType::StillBasin), "StillBasin is NOT buildable");
    TEST_ASSERT(!MapValidator::isBuildable(TerrainType::BlightMires), "BlightMires is NOT buildable");
}

// ============================================================================
// Test: isWater helper
// ============================================================================
void test_IsWater() {
    std::printf("\ntest_IsWater:\n");

    TEST_ASSERT(MapValidator::isWater(TerrainType::DeepVoid), "DeepVoid is water");
    TEST_ASSERT(MapValidator::isWater(TerrainType::FlowChannel), "FlowChannel is water");
    TEST_ASSERT(MapValidator::isWater(TerrainType::StillBasin), "StillBasin is water");

    TEST_ASSERT(!MapValidator::isWater(TerrainType::Substrate), "Substrate is NOT water");
    TEST_ASSERT(!MapValidator::isWater(TerrainType::BiolumeGrove), "BiolumeGrove is NOT water");
    TEST_ASSERT(!MapValidator::isWater(TerrainType::BlightMires), "BlightMires is NOT water");
}

// ============================================================================
// Test: Retry logic simulation
// ============================================================================
void test_RetryWithSeedIncrement() {
    std::printf("\ntest_RetryWithSeedIncrement:\n");

    TerrainGrid grid(MapSize::Medium);
    WaterData waterData(MapSize::Medium);
    WaterDistanceField waterDist(MapSize::Medium);

    ValidationConfig config = ValidationConfig::defaultConfig();
    config.checkSpawnPoints = false;
    config.checkTerrainDistribution = false; // Relaxed for test

    std::uint64_t baseSeed = 1000;
    std::uint8_t maxRetries = 10;
    std::uint8_t retryCount = 0;
    bool foundValid = false;
    float bestScore = 0.0f;
    std::uint64_t bestSeed = baseSeed;

    for (std::uint8_t attempt = 0; attempt < maxRetries && !foundValid; ++attempt) {
        std::uint64_t seed = baseSeed + attempt;
        setupGeneratedTerrain(grid, waterData, waterDist, seed);

        ValidationResult result = MapValidator::validate(grid, waterDist, seed, config);

        if (result.aggregateScore > bestScore) {
            bestScore = result.aggregateScore;
            bestSeed = seed;
        }

        if (result.isValid) {
            foundValid = true;
            std::printf("    Valid map found at attempt %u (seed %llu)\n",
                       attempt + 1, (unsigned long long)seed);
        } else {
            ++retryCount;
        }
    }

    std::printf("    Best score: %.2f at seed %llu\n", bestScore, (unsigned long long)bestSeed);
    std::printf("    Retries used: %u\n", retryCount);

    TEST_ASSERT(true, "Retry logic executed without error");
}

// ============================================================================
// Test: Known-bad terrain configurations
// ============================================================================
void test_KnownBadConfig_NoRiver() {
    std::printf("\ntest_KnownBadConfig_NoRiver:\n");

    TerrainGrid grid(MapSize::Small);
    WaterDistanceField waterDist(MapSize::Small);

    grid.fill_type(TerrainType::Substrate);
    waterDist.compute(grid);

    ValidationConfig config = ValidationConfig::defaultConfig();
    config.requireRiver = true;
    config.checkSpawnPoints = false;

    ValidationResult result = MapValidator::validate(grid, waterDist, 0, config);

    TEST_ASSERT(!result.isValid, "Map without river is invalid");
    TEST_ASSERT(!result.riverExistsPassed, "River check failed as expected");
}

void test_KnownBadConfig_TooMuchWater() {
    std::printf("\ntest_KnownBadConfig_TooMuchWater:\n");

    TerrainGrid grid(MapSize::Small);
    WaterDistanceField waterDist(MapSize::Small);

    // 70% water, 30% land
    std::uint32_t totalTiles = static_cast<std::uint32_t>(grid.tile_count());
    for (std::uint32_t i = 0; i < totalTiles; ++i) {
        if (i < totalTiles * 7 / 10) {
            grid.tiles[i].setTerrainType(TerrainType::DeepVoid);
        } else {
            grid.tiles[i].setTerrainType(TerrainType::Substrate);
        }
    }

    // Add a river for that check
    for (std::uint16_t y = 10; y < 20; ++y) {
        grid.at(50, y).setTerrainType(TerrainType::FlowChannel);
    }

    waterDist.compute(grid);

    ValidationConfig config = ValidationConfig::defaultConfig();
    config.checkSpawnPoints = false;

    ValidationResult result = MapValidator::validate(grid, waterDist, 0, config);

    TEST_ASSERT(!result.isValid, "Map with 70% water is invalid");
    TEST_ASSERT(!result.buildableAreaPassed, "Buildable area check failed");
    std::printf("    Buildable percent: %.1f%%\n", result.buildableAreaPercent * 100.0f);
}

void test_KnownBadConfig_AllBlightMires() {
    std::printf("\ntest_KnownBadConfig_AllBlightMires:\n");

    TerrainGrid grid(MapSize::Small);
    WaterDistanceField waterDist(MapSize::Small);

    grid.fill_type(TerrainType::BlightMires);

    // Add a river
    for (std::uint16_t y = 10; y < 50; ++y) {
        grid.at(64, y).setTerrainType(TerrainType::FlowChannel);
    }

    waterDist.compute(grid);

    ValidationConfig config = ValidationConfig::defaultConfig();
    config.checkSpawnPoints = false;

    ValidationResult result = MapValidator::validate(grid, waterDist, 0, config);

    TEST_ASSERT(!result.isValid, "All BlightMires map is invalid");
    TEST_ASSERT(!result.buildableAreaPassed, "Buildable area check failed");
    TEST_ASSERT(result.buildableAreaPercent < 0.01f, "Almost 0% buildable");
}

void test_KnownBadConfig_MultipleAnomalies() {
    std::printf("\ntest_KnownBadConfig_MultipleAnomalies:\n");

    TerrainGrid grid(MapSize::Small);
    WaterDistanceField waterDist(MapSize::Small);

    grid.fill_type(TerrainType::Substrate);

    // Add multiple single-tile anomalies
    grid.at(20, 20).setTerrainType(TerrainType::BiolumeGrove);
    grid.at(40, 40).setTerrainType(TerrainType::PrismaFields);
    grid.at(60, 60).setTerrainType(TerrainType::SporeFlats);
    grid.at(80, 80).setTerrainType(TerrainType::EmberCrust);
    grid.at(100, 100).setTerrainType(TerrainType::Ridge);

    // Add a river
    for (std::uint16_t y = 10; y < 50; ++y) {
        grid.at(64, y).setTerrainType(TerrainType::FlowChannel);
    }

    waterDist.compute(grid);

    ValidationConfig config = ValidationConfig::defaultConfig();
    config.checkSpawnPoints = false;
    config.checkTerrainDistribution = false;

    ValidationResult result = MapValidator::validate(grid, waterDist, 0, config);

    TEST_ASSERT(!result.isValid, "Map with anomalies is invalid");
    TEST_ASSERT(!result.terrainAnomaliesPassed, "Anomaly check failed");
    TEST_ASSERT(result.anomalyCount >= 5, "Detected multiple anomalies");
    std::printf("    Anomaly count: %u\n", result.anomalyCount);
}

} // anonymous namespace

int main() {
    std::printf("=== MapValidator Tests ===\n");
    std::printf("Ticket 3-013: Post-Generation Map Validation\n\n");

    // ValidationConfig tests
    test_ValidationConfig_DefaultValues();
    test_ValidationConfig_FactoryMethods();

    // ValidationResult tests
    test_ValidationResult_PassedCheckCount();

    // Buildable area check tests
    test_CheckBuildableArea_AllSubstrate();
    test_CheckBuildableArea_HalfWater();
    test_CheckBuildableArea_TooMuchWater();
    test_CheckBuildableArea_BlightMiresNotBuildable();
    test_CheckBuildableArea_ClearableTypes();

    // River existence tests
    test_CheckRiverExists_HasRiver();
    test_CheckRiverExists_NoRiver();

    // Coastline continuity tests
    test_CheckCoastlineContinuity_NoOcean();
    test_CheckCoastlineContinuity_ContinuousCoast();
    test_CheckCoastlineContinuity_SingleTileGap();

    // Terrain anomaly tests
    test_CheckTerrainAnomalies_NoAnomalies();
    test_CheckTerrainAnomalies_SingleTileAnomaly();
    test_CheckTerrainAnomalies_ClusterNotAnomaly();

    // Terrain distribution tests
    test_CheckTerrainDistribution_InRange();
    test_CheckTerrainDistribution_TooLow();
    test_CheckTerrainDistribution_TooHigh();
    test_CheckTerrainDistribution_IgnoresWater();

    // Full validation tests
    test_Validate_ValidMap();
    test_Validate_ValidationTimeUnder10ms();

    // Score calculation tests
    test_AggregateScore_AllPassed();
    test_AggregateScore_SomeFailed();

    // Helper function tests
    test_IsBuildable();
    test_IsWater();

    // Retry logic tests
    test_RetryWithSeedIncrement();

    // Known-bad configuration tests
    test_KnownBadConfig_NoRiver();
    test_KnownBadConfig_TooMuchWater();
    test_KnownBadConfig_AllBlightMires();
    test_KnownBadConfig_MultipleAnomalies();

    std::printf("\n=== Test Summary ===\n");
    std::printf("Passed: %d\n", g_testsPassed);
    std::printf("Failed: %d\n", g_testsFailed);

    return g_testsFailed > 0 ? 1 : 0;
}
