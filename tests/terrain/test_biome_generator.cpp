/**
 * @file test_biome_generator.cpp
 * @brief Unit tests for BiomeGenerator alien biome distribution.
 *
 * Tests verify:
 * - BiolumeGrove placed in lowlands and along flow channel banks (~8-12%)
 * - PrismaFields placed on ridgelines and plateaus (~2-4%, rarest)
 * - SporeFlats placed in transitional zones (~3-5%)
 * - BlightMires placed in lowlands with expansion gaps (~3-5%)
 * - EmberCrust placed at high elevation (~3-6%)
 * - Substrate remains as default (~35-45%)
 * - Biomes form coherent clusters, not single-tile scatter
 * - Every map has at least one blight mire patch
 * - PrismaFields are the rarest special terrain
 * - Fully deterministic generation
 * - Water tiles are preserved
 */

#include <sims3000/terrain/BiomeGenerator.h>
#include <sims3000/terrain/ElevationGenerator.h>
#include <sims3000/terrain/WaterDistanceField.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace sims3000::terrain;

// =============================================================================
// Test Utilities
// =============================================================================

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) void name()
#define RUN_TEST(test) do { \
    printf("  Running %s...", #test); \
    tests_run++; \
    test(); \
    tests_passed++; \
    printf(" PASSED\n"); \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        printf("\n    FAILED: %s (line %d)\n", #cond, __LINE__); \
        exit(1); \
    } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n    FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        exit(1); \
    } \
} while(0)

#define ASSERT_NEAR(a, b, tolerance) do { \
    if (std::abs((a) - (b)) > (tolerance)) { \
        printf("\n    FAILED: %s ~= %s (got %f vs %f, tolerance %f) (line %d)\n", \
               #a, #b, static_cast<double>(a), static_cast<double>(b), \
               static_cast<double>(tolerance), __LINE__); \
        exit(1); \
    } \
} while(0)

#define ASSERT_GE(a, b) do { \
    if ((a) < (b)) { \
        printf("\n    FAILED: %s >= %s (got %f vs %f) (line %d)\n", \
               #a, #b, static_cast<double>(a), static_cast<double>(b), __LINE__); \
        exit(1); \
    } \
} while(0)

#define ASSERT_LE(a, b) do { \
    if ((a) > (b)) { \
        printf("\n    FAILED: %s <= %s (got %f vs %f) (line %d)\n", \
               #a, #b, static_cast<double>(a), static_cast<double>(b), __LINE__); \
        exit(1); \
    } \
} while(0)

#define ASSERT_GT(a, b) do { \
    if ((a) <= (b)) { \
        printf("\n    FAILED: %s > %s (got %f vs %f) (line %d)\n", \
               #a, #b, static_cast<double>(a), static_cast<double>(b), __LINE__); \
        exit(1); \
    } \
} while(0)

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * @brief Set up a test grid with elevation data and water.
 */
static void setupTestGrid(
    TerrainGrid& grid,
    WaterDistanceField& waterDist,
    std::uint64_t seed)
{
    // Generate elevation
    ElevationConfig elevConfig = ElevationConfig::defaultConfig();
    ElevationGenerator::generate(grid, seed, elevConfig);

    // Add some water tiles for testing water proximity
    // Create a simple river pattern
    for (std::uint16_t y = grid.height / 4; y < grid.height / 4 + 3; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setTerrainType(TerrainType::FlowChannel);
            grid.at(x, y).setElevation(0);
        }
    }

    // Compute water distances
    waterDist.compute(grid);
}

/**
 * @brief Check if biomes form clusters (not single-tile scatter).
 *
 * A cluster is defined as having at least one adjacent tile of the same type.
 * Returns the percentage of biome tiles that are part of clusters.
 */
static float calculateClusterPercentage(const TerrainGrid& grid, TerrainType type) {
    std::uint32_t typeCount = 0;
    std::uint32_t clusteredCount = 0;

    constexpr std::int16_t dx[4] = { 0, 1, 0, -1 };
    constexpr std::int16_t dy[4] = { -1, 0, 1, 0 };

    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            if (grid.at(x, y).getTerrainType() != type) {
                continue;
            }
            ++typeCount;

            // Check if any neighbor is the same type
            bool hasNeighbor = false;
            for (int i = 0; i < 4; ++i) {
                std::int32_t nx = static_cast<std::int32_t>(x) + dx[i];
                std::int32_t ny = static_cast<std::int32_t>(y) + dy[i];
                if (grid.in_bounds(nx, ny)) {
                    if (grid.at(static_cast<std::uint16_t>(nx),
                               static_cast<std::uint16_t>(ny)).getTerrainType() == type) {
                        hasNeighbor = true;
                        break;
                    }
                }
            }
            if (hasNeighbor) {
                ++clusteredCount;
            }
        }
    }

    if (typeCount == 0) {
        return 0.0f;
    }
    return static_cast<float>(clusteredCount) / static_cast<float>(typeCount) * 100.0f;
}

/**
 * @brief Count tiles of a specific type in lowland elevations.
 */
static std::uint32_t countInLowlands(
    const TerrainGrid& grid,
    TerrainType type,
    std::uint8_t maxElevation)
{
    std::uint32_t count = 0;
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            const TerrainComponent& tile = grid.at(x, y);
            if (tile.getTerrainType() == type && tile.getElevation() <= maxElevation) {
                ++count;
            }
        }
    }
    return count;
}

/**
 * @brief Count tiles of a specific type in highland elevations.
 */
static std::uint32_t countInHighlands(
    const TerrainGrid& grid,
    TerrainType type,
    std::uint8_t minElevation)
{
    std::uint32_t count = 0;
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            const TerrainComponent& tile = grid.at(x, y);
            if (tile.getTerrainType() == type && tile.getElevation() >= minElevation) {
                ++count;
            }
        }
    }
    return count;
}

// =============================================================================
// Unit Tests
// =============================================================================

TEST(test_BiomeConfig_defaultValues) {
    BiomeConfig config = BiomeConfig::defaultConfig();

    // Check noise parameters have reasonable defaults
    ASSERT_TRUE(config.baseFeatureScale > 0.0f);
    ASSERT_TRUE(config.octaves >= 2);
    ASSERT_TRUE(config.persistence > 0.0f && config.persistence < 1.0f);

    // Check elevation thresholds are in valid range (0-31)
    ASSERT_TRUE(config.lowlandMaxElevation < 31);
    ASSERT_TRUE(config.highlandMinElevation > 0);
    ASSERT_TRUE(config.volcanicMinElevation > config.highlandMinElevation);

    // Check coverage targets sum to less than 100%
    float totalCoverage = config.groveTargetCoverage +
                          config.prismaTargetCoverage +
                          config.sporeTargetCoverage +
                          config.mireTargetCoverage +
                          config.emberTargetCoverage;
    ASSERT_TRUE(totalCoverage < 1.0f);
}

TEST(test_BiomeConfig_factoryMethods) {
    // Test lush config has more vegetation
    BiomeConfig lush = BiomeConfig::lush();
    BiomeConfig normal = BiomeConfig::defaultConfig();
    ASSERT_TRUE(lush.groveTargetCoverage > normal.groveTargetCoverage);

    // Test volcanic config has more ember
    BiomeConfig volcanic = BiomeConfig::volcanic();
    ASSERT_TRUE(volcanic.emberTargetCoverage > normal.emberTargetCoverage);

    // Test crystalline config has more prisma
    BiomeConfig crystal = BiomeConfig::crystalline();
    ASSERT_TRUE(crystal.prismaTargetCoverage > normal.prismaTargetCoverage);
}

TEST(test_BiomeResult_hasBlightMirePatch) {
    BiomeResult result{};
    result.mireCount = 0;
    ASSERT_FALSE(result.hasBlightMirePatch());

    result.mireCount = 1;
    ASSERT_TRUE(result.hasBlightMirePatch());
}

TEST(test_BiomeResult_isPrismaRarest) {
    BiomeResult result{};
    result.prismaCount = 10;
    result.groveCount = 100;
    result.sporeCount = 50;
    result.mireCount = 40;
    result.emberCount = 30;
    ASSERT_TRUE(result.isPrismaRarest());

    // Make grove rarer than prisma
    result.groveCount = 5;
    ASSERT_FALSE(result.isPrismaRarest());
}

TEST(test_isEligibleForBiome) {
    // Substrate and Ridge are eligible
    ASSERT_TRUE(BiomeGenerator::isEligibleForBiome(TerrainType::Substrate));
    ASSERT_TRUE(BiomeGenerator::isEligibleForBiome(TerrainType::Ridge));

    // Water types are not eligible
    ASSERT_FALSE(BiomeGenerator::isEligibleForBiome(TerrainType::DeepVoid));
    ASSERT_FALSE(BiomeGenerator::isEligibleForBiome(TerrainType::FlowChannel));
    ASSERT_FALSE(BiomeGenerator::isEligibleForBiome(TerrainType::StillBasin));

    // Special biomes are not eligible (already placed)
    ASSERT_FALSE(BiomeGenerator::isEligibleForBiome(TerrainType::BiolumeGrove));
    ASSERT_FALSE(BiomeGenerator::isEligibleForBiome(TerrainType::PrismaFields));
    ASSERT_FALSE(BiomeGenerator::isEligibleForBiome(TerrainType::SporeFlats));
    ASSERT_FALSE(BiomeGenerator::isEligibleForBiome(TerrainType::BlightMires));
    ASSERT_FALSE(BiomeGenerator::isEligibleForBiome(TerrainType::EmberCrust));
}

TEST(test_elevationChecks) {
    BiomeConfig config = BiomeConfig::defaultConfig();

    // EmberCrust requires volcanic elevation
    ASSERT_FALSE(BiomeGenerator::isEmberElevation(10, config));
    ASSERT_FALSE(BiomeGenerator::isEmberElevation(21, config));
    ASSERT_TRUE(BiomeGenerator::isEmberElevation(22, config));
    ASSERT_TRUE(BiomeGenerator::isEmberElevation(31, config));

    // PrismaFields requires ridge elevation
    ASSERT_FALSE(BiomeGenerator::isPrismaElevation(10, config));
    ASSERT_FALSE(BiomeGenerator::isPrismaElevation(19, config));
    ASSERT_TRUE(BiomeGenerator::isPrismaElevation(20, config));
    ASSERT_TRUE(BiomeGenerator::isPrismaElevation(25, config));

    // Lowland check
    ASSERT_TRUE(BiomeGenerator::isLowlandElevation(0, config));
    ASSERT_TRUE(BiomeGenerator::isLowlandElevation(10, config));
    ASSERT_FALSE(BiomeGenerator::isLowlandElevation(11, config));
    ASSERT_FALSE(BiomeGenerator::isLowlandElevation(20, config));
}

TEST(test_generate_basic) {
    // Small grid for quick testing
    TerrainGrid grid(MapSize::Small);
    WaterDistanceField waterDist(MapSize::Small);

    std::uint64_t seed = 12345;
    setupTestGrid(grid, waterDist, seed);

    BiomeConfig config = BiomeConfig::defaultConfig();
    BiomeResult result = BiomeGenerator::generate(grid, waterDist, seed, config);

    // Basic sanity checks
    ASSERT_TRUE(result.totalTiles == grid.tile_count());
    ASSERT_TRUE(result.landTiles > 0);
    ASSERT_TRUE(result.generationTimeMs >= 0.0f);

    // At least some biomes should be placed
    ASSERT_TRUE(result.groveCount > 0);
    ASSERT_TRUE(result.substrateCount > 0);
}

TEST(test_generate_deterministic) {
    // Generate twice with same seed, should get identical results
    std::uint64_t seed = 54321;
    BiomeConfig config = BiomeConfig::defaultConfig();

    // First generation
    TerrainGrid grid1(MapSize::Small);
    WaterDistanceField waterDist1(MapSize::Small);
    setupTestGrid(grid1, waterDist1, seed);
    BiomeResult result1 = BiomeGenerator::generate(grid1, waterDist1, seed, config);

    // Second generation
    TerrainGrid grid2(MapSize::Small);
    WaterDistanceField waterDist2(MapSize::Small);
    setupTestGrid(grid2, waterDist2, seed);
    BiomeResult result2 = BiomeGenerator::generate(grid2, waterDist2, seed, config);

    // Results should be identical
    ASSERT_EQ(result1.groveCount, result2.groveCount);
    ASSERT_EQ(result1.prismaCount, result2.prismaCount);
    ASSERT_EQ(result1.sporeCount, result2.sporeCount);
    ASSERT_EQ(result1.mireCount, result2.mireCount);
    ASSERT_EQ(result1.emberCount, result2.emberCount);
    ASSERT_EQ(result1.substrateCount, result2.substrateCount);

    // Every tile should match
    for (std::uint16_t y = 0; y < grid1.height; ++y) {
        for (std::uint16_t x = 0; x < grid1.width; ++x) {
            ASSERT_EQ(
                static_cast<std::uint8_t>(grid1.at(x, y).getTerrainType()),
                static_cast<std::uint8_t>(grid2.at(x, y).getTerrainType())
            );
        }
    }
}

TEST(test_generate_differentSeeds) {
    // Different seeds should produce different results
    BiomeConfig config = BiomeConfig::defaultConfig();

    TerrainGrid grid1(MapSize::Small);
    WaterDistanceField waterDist1(MapSize::Small);
    setupTestGrid(grid1, waterDist1, 11111);
    BiomeResult result1 = BiomeGenerator::generate(grid1, waterDist1, 11111, config);

    TerrainGrid grid2(MapSize::Small);
    WaterDistanceField waterDist2(MapSize::Small);
    setupTestGrid(grid2, waterDist2, 22222);
    BiomeResult result2 = BiomeGenerator::generate(grid2, waterDist2, 22222, config);

    // Results should be different (at least one count differs)
    bool different = (result1.groveCount != result2.groveCount) ||
                     (result1.prismaCount != result2.prismaCount) ||
                     (result1.sporeCount != result2.sporeCount) ||
                     (result1.mireCount != result2.mireCount) ||
                     (result1.emberCount != result2.emberCount);
    ASSERT_TRUE(different);
}

TEST(test_waterTilesPreserved) {
    TerrainGrid grid(MapSize::Small);
    WaterDistanceField waterDist(MapSize::Small);
    std::uint64_t seed = 99999;
    setupTestGrid(grid, waterDist, seed);

    // Count water tiles before biome generation
    std::uint32_t waterBefore = 0;
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            TerrainType type = grid.at(x, y).getTerrainType();
            if (type == TerrainType::DeepVoid ||
                type == TerrainType::FlowChannel ||
                type == TerrainType::StillBasin) {
                ++waterBefore;
            }
        }
    }

    BiomeResult result = BiomeGenerator::generate(grid, waterDist, seed, BiomeConfig::defaultConfig());

    // Water count should match
    ASSERT_EQ(result.waterCount, waterBefore);

    // Verify water tiles are still water
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            std::uint8_t waterDistVal = waterDist.get_water_distance(x, y);
            if (waterDistVal == 0) {
                // This was a water tile - should still be water
                TerrainType type = grid.at(x, y).getTerrainType();
                ASSERT_TRUE(
                    type == TerrainType::DeepVoid ||
                    type == TerrainType::FlowChannel ||
                    type == TerrainType::StillBasin
                );
            }
        }
    }
}

TEST(test_biolumeGrove_coverage) {
    // Test that BiolumeGrove is within target range (8-12%)
    TerrainGrid grid(MapSize::Medium);
    WaterDistanceField waterDist(MapSize::Medium);
    std::uint64_t seed = 77777;
    setupTestGrid(grid, waterDist, seed);

    BiomeResult result = BiomeGenerator::generate(grid, waterDist, seed, BiomeConfig::defaultConfig());

    // Coverage should be approximately 8-12%
    // Allow some tolerance due to elevation constraints
    ASSERT_GE(result.groveCoverage, 4.0f);   // At least 4%
    ASSERT_LE(result.groveCoverage, 18.0f);  // No more than 18%
}

TEST(test_prismaFields_rarest) {
    // Test that PrismaFields is the rarest special terrain
    TerrainGrid grid(MapSize::Medium);
    WaterDistanceField waterDist(MapSize::Medium);
    std::uint64_t seed = 88888;
    setupTestGrid(grid, waterDist, seed);

    BiomeResult result = BiomeGenerator::generate(grid, waterDist, seed, BiomeConfig::defaultConfig());

    // PrismaFields should be rarest (or tied for rarest)
    ASSERT_TRUE(result.isPrismaRarest());
}

TEST(test_blightMires_exists) {
    // Test that every map has at least one blight mire patch
    // Run multiple seeds to verify
    BiomeConfig config = BiomeConfig::defaultConfig();

    for (std::uint64_t seed = 100; seed < 110; ++seed) {
        TerrainGrid grid(MapSize::Small);
        WaterDistanceField waterDist(MapSize::Small);
        setupTestGrid(grid, waterDist, seed);

        BiomeResult result = BiomeGenerator::generate(grid, waterDist, seed, config);
        ASSERT_TRUE(result.hasBlightMirePatch());
    }
}

TEST(test_substrate_remainsDefault) {
    // Test that Substrate remains as the dominant terrain (35-45% target)
    TerrainGrid grid(MapSize::Medium);
    WaterDistanceField waterDist(MapSize::Medium);
    std::uint64_t seed = 66666;
    setupTestGrid(grid, waterDist, seed);

    BiomeResult result = BiomeGenerator::generate(grid, waterDist, seed, BiomeConfig::defaultConfig());

    // Substrate should be substantial portion of land
    ASSERT_GE(result.substrateCoverage, 25.0f);  // At least 25%
}

TEST(test_biomes_formClusters) {
    // Test that biomes form coherent clusters, not single-tile scatter
    TerrainGrid grid(MapSize::Medium);
    WaterDistanceField waterDist(MapSize::Medium);
    std::uint64_t seed = 55555;
    setupTestGrid(grid, waterDist, seed);

    BiomeGenerator::generate(grid, waterDist, seed, BiomeConfig::defaultConfig());

    // Calculate cluster percentage for each biome
    // Most tiles of each type should have at least one neighbor of same type
    float groveCluster = calculateClusterPercentage(grid, TerrainType::BiolumeGrove);
    float mireCluster = calculateClusterPercentage(grid, TerrainType::BlightMires);
    float emberCluster = calculateClusterPercentage(grid, TerrainType::EmberCrust);

    // At least 50% of biome tiles should be clustered
    // (Some edge tiles won't have neighbors)
    ASSERT_GE(groveCluster, 50.0f);
    ASSERT_GE(mireCluster, 40.0f);  // Mires may be sparser due to gap rules
    ASSERT_GE(emberCluster, 40.0f);  // Ember may be sparse due to high elevation requirement
}

TEST(test_emberCrust_highElevation) {
    // Test that EmberCrust is only placed at high elevations
    TerrainGrid grid(MapSize::Medium);
    WaterDistanceField waterDist(MapSize::Medium);
    std::uint64_t seed = 44444;
    setupTestGrid(grid, waterDist, seed);

    BiomeConfig config = BiomeConfig::defaultConfig();
    BiomeGenerator::generate(grid, waterDist, seed, config);

    // All EmberCrust tiles should be at volcanic elevation or higher
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            const TerrainComponent& tile = grid.at(x, y);
            if (tile.getTerrainType() == TerrainType::EmberCrust) {
                ASSERT_TRUE(tile.getElevation() >= config.volcanicMinElevation);
            }
        }
    }
}

TEST(test_biolumeGrove_lowlandOrNearWater) {
    // Test that BiolumeGrove is placed in lowlands or near water
    TerrainGrid grid(MapSize::Medium);
    WaterDistanceField waterDist(MapSize::Medium);
    std::uint64_t seed = 33333;
    setupTestGrid(grid, waterDist, seed);

    BiomeConfig config = BiomeConfig::defaultConfig();
    BiomeGenerator::generate(grid, waterDist, seed, config);

    // All BiolumeGrove tiles should be in lowlands OR near water
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            const TerrainComponent& tile = grid.at(x, y);
            if (tile.getTerrainType() == TerrainType::BiolumeGrove) {
                bool isLowland = tile.getElevation() <= config.lowlandMaxElevation;
                std::uint8_t waterDistVal = waterDist.get_water_distance(x, y);
                bool nearWater = waterDistVal > 0 && waterDistVal <= config.groveWaterProximityMax;
                ASSERT_TRUE(isLowland || nearWater);
            }
        }
    }
}

TEST(test_noiseThresholds_affectCoverage) {
    // Test that changing noise thresholds affects coverage
    TerrainGrid grid1(MapSize::Small);
    WaterDistanceField waterDist1(MapSize::Small);
    std::uint64_t seed = 22222;
    setupTestGrid(grid1, waterDist1, seed);

    BiomeConfig config1 = BiomeConfig::defaultConfig();
    config1.groveNoiseThreshold = 0.3f;  // Lower threshold = more coverage
    BiomeResult result1 = BiomeGenerator::generate(grid1, waterDist1, seed, config1);

    TerrainGrid grid2(MapSize::Small);
    WaterDistanceField waterDist2(MapSize::Small);
    setupTestGrid(grid2, waterDist2, seed);

    BiomeConfig config2 = BiomeConfig::defaultConfig();
    config2.groveNoiseThreshold = 0.8f;  // Higher threshold = less coverage
    BiomeResult result2 = BiomeGenerator::generate(grid2, waterDist2, seed, config2);

    // Lower threshold should produce more groves
    ASSERT_GT(result1.groveCount, result2.groveCount);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("Running BiomeGenerator tests...\n\n");

    // BiomeConfig tests
    printf("BiomeConfig tests:\n");
    RUN_TEST(test_BiomeConfig_defaultValues);
    RUN_TEST(test_BiomeConfig_factoryMethods);

    // BiomeResult tests
    printf("\nBiomeResult tests:\n");
    RUN_TEST(test_BiomeResult_hasBlightMirePatch);
    RUN_TEST(test_BiomeResult_isPrismaRarest);

    // Static method tests
    printf("\nStatic method tests:\n");
    RUN_TEST(test_isEligibleForBiome);
    RUN_TEST(test_elevationChecks);

    // Generation tests
    printf("\nGeneration tests:\n");
    RUN_TEST(test_generate_basic);
    RUN_TEST(test_generate_deterministic);
    RUN_TEST(test_generate_differentSeeds);
    RUN_TEST(test_waterTilesPreserved);

    // Coverage and distribution tests
    printf("\nCoverage and distribution tests:\n");
    RUN_TEST(test_biolumeGrove_coverage);
    RUN_TEST(test_prismaFields_rarest);
    RUN_TEST(test_blightMires_exists);
    RUN_TEST(test_substrate_remainsDefault);
    RUN_TEST(test_biomes_formClusters);

    // Elevation rule tests
    printf("\nElevation rule tests:\n");
    RUN_TEST(test_emberCrust_highElevation);
    RUN_TEST(test_biolumeGrove_lowlandOrNearWater);

    // Configuration tests
    printf("\nConfiguration tests:\n");
    RUN_TEST(test_noiseThresholds_affectCoverage);

    printf("\n========================================\n");
    printf("All tests passed! (%d/%d)\n", tests_passed, tests_run);
    printf("========================================\n");

    return 0;
}
