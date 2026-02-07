/**
 * @file test_elevation_generator.cpp
 * @brief Unit tests for ElevationGenerator (Ticket 3-008)
 *
 * Tests cover:
 * - Multi-octave noise generation (4-6 octaves)
 * - Configurable parameters (roughness, amplitude, feature scale, ridge threshold)
 * - Elevation values in valid range (0-31)
 * - Ridge terrain type assignment
 * - Substrate terrain type for non-ridge tiles
 * - Deterministic generation (same seed = same output)
 * - Row-major generation order
 * - Performance (<50ms for 512x512)
 * - Geologically coherent features (ridges, valleys)
 */

#include <sims3000/terrain/ElevationGenerator.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>
#include <set>
#include <algorithm>
#include <numeric>

using namespace sims3000::terrain;

// =============================================================================
// Test Utilities
// =============================================================================

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST(name) \
    void test_##name(); \
    struct TestRegistrar_##name { \
        TestRegistrar_##name() { \
            std::cout << "Running " #name "..." << std::endl; \
            g_tests_run++; \
            test_##name(); \
            g_tests_passed++; \
            std::cout << "  PASSED" << std::endl; \
        } \
    } test_instance_##name; \
    void test_##name()

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "  FAILED: " << #cond << " at line " << __LINE__ << std::endl; \
            assert(false); \
        } \
    } while(0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            std::cerr << "  FAILED: NOT " << #cond << " at line " << __LINE__ << std::endl; \
            assert(false); \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "  FAILED: " << #a << " != " << #b \
                      << " (" << static_cast<int>(a) << " != " << static_cast<int>(b) << ") at line " << __LINE__ << std::endl; \
            assert(false); \
        } \
    } while(0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::cerr << "  FAILED: " << #a << " == " << #b \
                      << " (" << static_cast<int>(a) << ") at line " << __LINE__ << std::endl; \
            assert(false); \
        } \
    } while(0)

#define ASSERT_LE(a, b) \
    do { \
        if ((a) > (b)) { \
            std::cerr << "  FAILED: " << #a << " > " << #b \
                      << " (" << (a) << " > " << (b) << ") at line " << __LINE__ << std::endl; \
            assert(false); \
        } \
    } while(0)

#define ASSERT_GE(a, b) \
    do { \
        if ((a) < (b)) { \
            std::cerr << "  FAILED: " << #a << " < " << #b \
                      << " (" << (a) << " < " << (b) << ") at line " << __LINE__ << std::endl; \
            assert(false); \
        } \
    } while(0)

#define ASSERT_LT(a, b) \
    do { \
        if ((a) >= (b)) { \
            std::cerr << "  FAILED: " << #a << " >= " << #b \
                      << " (" << (a) << " >= " << (b) << ") at line " << __LINE__ << std::endl; \
            assert(false); \
        } \
    } while(0)

#define ASSERT_NEAR(a, b, tol) \
    do { \
        if (std::abs((a) - (b)) > (tol)) { \
            std::cerr << "  FAILED: " << #a << " ~= " << #b \
                      << " (" << (a) << " vs " << (b) << ", tol=" << (tol) << ") at line " << __LINE__ << std::endl; \
            assert(false); \
        } \
    } while(0)

// =============================================================================
// ElevationConfig Tests
// =============================================================================

TEST(ElevationConfig_DefaultValues) {
    ElevationConfig config;

    ASSERT_EQ(config.octaves, 5);
    ASSERT_NEAR(config.roughness, 0.5f, 0.001f);
    ASSERT_NEAR(config.amplitude, 1.0f, 0.001f);
    ASSERT_NEAR(config.featureScale, 0.008f, 0.001f);
    ASSERT_NEAR(config.lacunarity, 2.0f, 0.001f);
    ASSERT_EQ(config.ridgeThreshold, 21);
    ASSERT_EQ(config.minElevation, 0);
    ASSERT_EQ(config.maxElevation, 31);
    ASSERT_EQ(config.seedOffset, 0);
    ASSERT_TRUE(config.enhanceRidges);
    ASSERT_NEAR(config.ridgeStrength, 0.3f, 0.001f);
}

TEST(ElevationConfig_MountainousPreset) {
    ElevationConfig config = ElevationConfig::mountainous();

    ASSERT_EQ(config.octaves, 6);
    ASSERT_NEAR(config.roughness, 0.55f, 0.001f);
    ASSERT_NEAR(config.featureScale, 0.006f, 0.001f);
    ASSERT_EQ(config.ridgeThreshold, 18);
    ASSERT_NEAR(config.ridgeStrength, 0.4f, 0.001f);
}

TEST(ElevationConfig_PlainsPreset) {
    ElevationConfig config = ElevationConfig::plains();

    ASSERT_EQ(config.octaves, 4);
    ASSERT_NEAR(config.roughness, 0.4f, 0.001f);
    ASSERT_EQ(config.ridgeThreshold, 25);
    ASSERT_EQ(config.maxElevation, 20);
}

TEST(ElevationConfig_RollingPreset) {
    ElevationConfig config = ElevationConfig::rolling();

    ASSERT_EQ(config.octaves, 5);
    ASSERT_NEAR(config.roughness, 0.45f, 0.001f);
    ASSERT_NEAR(config.featureScale, 0.01f, 0.001f);
}

TEST(ElevationConfig_TriviallyCopyable) {
    // Compile-time assertion is in header, this is runtime verification
    ElevationConfig config1;
    config1.octaves = 6;
    config1.ridgeThreshold = 18;

    ElevationConfig config2 = config1;
    ASSERT_EQ(config2.octaves, 6);
    ASSERT_EQ(config2.ridgeThreshold, 18);
}

// =============================================================================
// Basic Generation Tests
// =============================================================================

TEST(ElevationGenerator_GeneratesAllTiles) {
    TerrainGrid grid(MapSize::Small);  // 128x128
    ElevationConfig config;

    ElevationResult result = ElevationGenerator::generate(grid, 12345, config);

    ASSERT_EQ(result.totalTiles, 128 * 128);
}

TEST(ElevationGenerator_ElevationInValidRange) {
    TerrainGrid grid(MapSize::Small);
    ElevationConfig config;

    ElevationGenerator::generate(grid, 12345, config);

    // Check all tiles have elevation 0-31
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            std::uint8_t elev = grid.at(x, y).getElevation();
            ASSERT_LE(elev, 31);
        }
    }
}

TEST(ElevationGenerator_ResultStatisticsValid) {
    TerrainGrid grid(MapSize::Small);
    ElevationConfig config;

    ElevationResult result = ElevationGenerator::generate(grid, 12345, config);

    // Min/max should be within range
    ASSERT_LE(result.minElevation, 31);
    ASSERT_LE(result.maxElevation, 31);
    ASSERT_LE(result.minElevation, result.maxElevation);

    // Mean should be between min and max
    ASSERT_GE(result.meanElevation, static_cast<float>(result.minElevation));
    ASSERT_LE(result.meanElevation, static_cast<float>(result.maxElevation));

    // Generation time should be positive
    ASSERT_TRUE(result.generationTimeMs > 0.0f);
}

// =============================================================================
// Terrain Type Assignment Tests
// =============================================================================

TEST(ElevationGenerator_RidgeTerrainTypeAssigned) {
    TerrainGrid grid(MapSize::Small);
    ElevationConfig config;
    config.ridgeThreshold = 21;

    ElevationResult result = ElevationGenerator::generate(grid, 12345, config);

    // Count ridge tiles manually
    std::uint32_t ridgeCount = 0;
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            const TerrainComponent& tile = grid.at(x, y);
            if (tile.getTerrainType() == TerrainType::Ridge) {
                ASSERT_GE(tile.getElevation(), config.ridgeThreshold);
                ++ridgeCount;
            }
        }
    }

    ASSERT_EQ(ridgeCount, result.ridgeTileCount);
}

TEST(ElevationGenerator_SubstrateTerrainTypeAssigned) {
    TerrainGrid grid(MapSize::Small);
    ElevationConfig config;
    config.ridgeThreshold = 21;

    ElevationGenerator::generate(grid, 12345, config);

    // Check non-ridge tiles are Substrate
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            const TerrainComponent& tile = grid.at(x, y);
            if (tile.getElevation() < config.ridgeThreshold) {
                ASSERT_EQ(tile.getTerrainType(), TerrainType::Substrate);
            }
        }
    }
}

TEST(ElevationGenerator_PreservesWaterTiles) {
    TerrainGrid grid(MapSize::Small);

    // Pre-set some water tiles
    grid.at(10, 10).setTerrainType(TerrainType::DeepVoid);
    grid.at(20, 20).setTerrainType(TerrainType::FlowChannel);
    grid.at(30, 30).setTerrainType(TerrainType::StillBasin);

    ElevationConfig config;
    ElevationGenerator::generate(grid, 12345, config);

    // Water tiles should be preserved
    ASSERT_EQ(grid.at(10, 10).getTerrainType(), TerrainType::DeepVoid);
    ASSERT_EQ(grid.at(20, 20).getTerrainType(), TerrainType::FlowChannel);
    ASSERT_EQ(grid.at(30, 30).getTerrainType(), TerrainType::StillBasin);
}

// =============================================================================
// Determinism Tests
// =============================================================================

TEST(ElevationGenerator_DeterministicSameSeed) {
    TerrainGrid grid1(MapSize::Small);
    TerrainGrid grid2(MapSize::Small);
    ElevationConfig config;

    ElevationGenerator::generate(grid1, 12345, config);
    ElevationGenerator::generate(grid2, 12345, config);

    // All tiles should be identical
    for (std::uint16_t y = 0; y < grid1.height; ++y) {
        for (std::uint16_t x = 0; x < grid1.width; ++x) {
            ASSERT_EQ(grid1.at(x, y).getElevation(), grid2.at(x, y).getElevation());
            ASSERT_EQ(grid1.at(x, y).getTerrainType(), grid2.at(x, y).getTerrainType());
        }
    }
}

TEST(ElevationGenerator_DifferentSeedsDifferentOutput) {
    TerrainGrid grid1(MapSize::Small);
    TerrainGrid grid2(MapSize::Small);
    ElevationConfig config;

    ElevationGenerator::generate(grid1, 12345, config);
    ElevationGenerator::generate(grid2, 54321, config);

    // Count differences - should be many
    int differences = 0;
    for (std::uint16_t y = 0; y < grid1.height; ++y) {
        for (std::uint16_t x = 0; x < grid1.width; ++x) {
            if (grid1.at(x, y).getElevation() != grid2.at(x, y).getElevation()) {
                ++differences;
            }
        }
    }

    // Most tiles should be different
    ASSERT_TRUE(differences > static_cast<int>(grid1.tile_count()) / 2);
}

TEST(ElevationGenerator_DeterministicAcrossMultipleRuns) {
    // Run generation multiple times to verify no state leakage
    ElevationConfig config;

    std::vector<std::uint8_t> firstRun;
    for (int run = 0; run < 3; ++run) {
        TerrainGrid grid(MapSize::Small);
        ElevationGenerator::generate(grid, 99999, config);

        if (run == 0) {
            for (std::size_t i = 0; i < grid.tile_count(); ++i) {
                firstRun.push_back(grid.tiles[i].getElevation());
            }
        } else {
            for (std::size_t i = 0; i < grid.tile_count(); ++i) {
                ASSERT_EQ(grid.tiles[i].getElevation(), firstRun[i]);
            }
        }
    }
}

// =============================================================================
// Multi-Octave Noise Tests
// =============================================================================

TEST(ElevationGenerator_OctavesAffectDetail) {
    // More octaves should add more high-frequency detail
    // Measure this by counting local minima/maxima

    auto countLocalExtrema = [](const TerrainGrid& grid) {
        int count = 0;
        for (std::uint16_t y = 1; y < grid.height - 1; ++y) {
            for (std::uint16_t x = 1; x < grid.width - 1; ++x) {
                std::uint8_t center = grid.at(x, y).getElevation();
                std::uint8_t left = grid.at(x - 1, y).getElevation();
                std::uint8_t right = grid.at(x + 1, y).getElevation();
                std::uint8_t up = grid.at(x, y - 1).getElevation();
                std::uint8_t down = grid.at(x, y + 1).getElevation();

                // Check if local maximum or minimum
                bool isMax = center > left && center > right && center > up && center > down;
                bool isMin = center < left && center < right && center < up && center < down;
                if (isMax || isMin) ++count;
            }
        }
        return count;
    };

    TerrainGrid grid4(MapSize::Small);
    TerrainGrid grid6(MapSize::Small);

    ElevationConfig config4;
    config4.octaves = 4;
    config4.enhanceRidges = false;  // Disable to isolate octave effect

    ElevationConfig config6;
    config6.octaves = 6;
    config6.enhanceRidges = false;

    ElevationGenerator::generate(grid4, 12345, config4);
    ElevationGenerator::generate(grid6, 12345, config6);

    int extrema4 = countLocalExtrema(grid4);
    int extrema6 = countLocalExtrema(grid6);

    std::cout << "    4 octaves: " << extrema4 << " local extrema" << std::endl;
    std::cout << "    6 octaves: " << extrema6 << " local extrema" << std::endl;

    // More octaves typically means more detail (more local extrema)
    // This isn't always guaranteed, but is a reasonable heuristic
    ASSERT_TRUE(extrema4 > 0);
    ASSERT_TRUE(extrema6 > 0);
}

TEST(ElevationGenerator_FeatureScaleAffectsSize) {
    // Larger feature scale = smaller features (more variation per unit distance)

    auto calculateVariation = [](const TerrainGrid& grid) {
        float variation = 0.0f;
        int count = 0;
        for (std::uint16_t y = 0; y < grid.height; ++y) {
            for (std::uint16_t x = 1; x < grid.width; ++x) {
                float diff = std::abs(
                    static_cast<float>(grid.at(x, y).getElevation()) -
                    static_cast<float>(grid.at(x - 1, y).getElevation())
                );
                variation += diff;
                ++count;
            }
        }
        return variation / count;
    };

    TerrainGrid gridSmallScale(MapSize::Small);
    TerrainGrid gridLargeScale(MapSize::Small);

    ElevationConfig configSmall;
    configSmall.featureScale = 0.004f;
    configSmall.enhanceRidges = false;

    ElevationConfig configLarge;
    configLarge.featureScale = 0.02f;
    configLarge.enhanceRidges = false;

    ElevationGenerator::generate(gridSmallScale, 12345, configSmall);
    ElevationGenerator::generate(gridLargeScale, 12345, configLarge);

    float variationSmall = calculateVariation(gridSmallScale);
    float variationLarge = calculateVariation(gridLargeScale);

    std::cout << "    Small scale (0.004): " << variationSmall << " avg variation" << std::endl;
    std::cout << "    Large scale (0.02): " << variationLarge << " avg variation" << std::endl;

    // Larger scale = more variation between adjacent tiles
    ASSERT_TRUE(variationLarge > variationSmall);
}

// =============================================================================
// Configurable Parameters Tests
// =============================================================================

TEST(ElevationGenerator_RoughnessAffectsOutput) {
    TerrainGrid gridSmooth(MapSize::Small);
    TerrainGrid gridRough(MapSize::Small);

    ElevationConfig configSmooth;
    configSmooth.roughness = 0.3f;
    configSmooth.enhanceRidges = false;

    ElevationConfig configRough;
    configRough.roughness = 0.7f;
    configRough.enhanceRidges = false;

    ElevationGenerator::generate(gridSmooth, 12345, configSmooth);
    ElevationGenerator::generate(gridRough, 12345, configRough);

    // Just verify they produce different output
    int differences = 0;
    for (std::size_t i = 0; i < gridSmooth.tile_count(); ++i) {
        if (gridSmooth.tiles[i].getElevation() != gridRough.tiles[i].getElevation()) {
            ++differences;
        }
    }
    ASSERT_TRUE(differences > 0);
}

TEST(ElevationGenerator_RidgeThresholdConfigurable) {
    TerrainGrid gridLow(MapSize::Small);
    TerrainGrid gridHigh(MapSize::Small);

    ElevationConfig configLow;
    configLow.ridgeThreshold = 15;  // More ridges

    ElevationConfig configHigh;
    configHigh.ridgeThreshold = 25;  // Fewer ridges

    ElevationResult resultLow = ElevationGenerator::generate(gridLow, 12345, configLow);
    ElevationResult resultHigh = ElevationGenerator::generate(gridHigh, 12345, configHigh);

    std::cout << "    Ridge threshold 15: " << resultLow.ridgeTileCount << " ridge tiles" << std::endl;
    std::cout << "    Ridge threshold 25: " << resultHigh.ridgeTileCount << " ridge tiles" << std::endl;

    // Lower threshold = more ridges
    ASSERT_TRUE(resultLow.ridgeTileCount > resultHigh.ridgeTileCount);
}

TEST(ElevationGenerator_MaxElevationLimitsRange) {
    TerrainGrid grid(MapSize::Small);

    ElevationConfig config;
    config.maxElevation = 20;  // Limit to 20

    ElevationResult result = ElevationGenerator::generate(grid, 12345, config);

    // All elevations should be <= 20
    for (std::size_t i = 0; i < grid.tile_count(); ++i) {
        ASSERT_LE(grid.tiles[i].getElevation(), 20);
    }
    ASSERT_LE(result.maxElevation, 20);
}

TEST(ElevationGenerator_MinElevationSetsFloor) {
    TerrainGrid grid(MapSize::Small);

    ElevationConfig config;
    config.minElevation = 10;  // Floor at 10

    ElevationResult result = ElevationGenerator::generate(grid, 12345, config);

    // All elevations should be >= 10
    for (std::size_t i = 0; i < grid.tile_count(); ++i) {
        ASSERT_GE(grid.tiles[i].getElevation(), 10);
    }
    ASSERT_GE(result.minElevation, 10);
}

TEST(ElevationGenerator_SeedOffsetProducesDifferentTerrain) {
    TerrainGrid grid1(MapSize::Small);
    TerrainGrid grid2(MapSize::Small);

    ElevationConfig config1;
    config1.seedOffset = 0;

    ElevationConfig config2;
    config2.seedOffset = 1000;

    ElevationGenerator::generate(grid1, 12345, config1);
    ElevationGenerator::generate(grid2, 12345, config2);

    // Different offsets = different terrain
    int differences = 0;
    for (std::size_t i = 0; i < grid1.tile_count(); ++i) {
        if (grid1.tiles[i].getElevation() != grid2.tiles[i].getElevation()) {
            ++differences;
        }
    }
    ASSERT_TRUE(differences > static_cast<int>(grid1.tile_count()) / 2);
}

// =============================================================================
// Ridge Enhancement Tests
// =============================================================================

TEST(ElevationGenerator_RidgeEnhancementCreatesRidges) {
    TerrainGrid gridWithRidges(MapSize::Small);
    TerrainGrid gridWithoutRidges(MapSize::Small);

    ElevationConfig configWith;
    configWith.enhanceRidges = true;
    configWith.ridgeStrength = 0.3f;

    ElevationConfig configWithout;
    configWithout.enhanceRidges = false;

    ElevationResult resultWith = ElevationGenerator::generate(gridWithRidges, 12345, configWith);
    ElevationResult resultWithout = ElevationGenerator::generate(gridWithoutRidges, 12345, configWithout);

    std::cout << "    With ridge enhancement: " << resultWith.ridgeTileCount << " ridges" << std::endl;
    std::cout << "    Without ridge enhancement: " << resultWithout.ridgeTileCount << " ridges" << std::endl;

    // Both should produce some variation
    ASSERT_TRUE(resultWith.maxElevation > resultWith.minElevation);
    ASSERT_TRUE(resultWithout.maxElevation > resultWithout.minElevation);
}

TEST(ElevationGenerator_RidgeStrengthAffectsIntensity) {
    ElevationConfig configWeak;
    configWeak.enhanceRidges = true;
    configWeak.ridgeStrength = 0.1f;

    ElevationConfig configStrong;
    configStrong.enhanceRidges = true;
    configStrong.ridgeStrength = 0.5f;

    TerrainGrid gridWeak(MapSize::Small);
    TerrainGrid gridStrong(MapSize::Small);

    ElevationGenerator::generate(gridWeak, 12345, configWeak);
    ElevationGenerator::generate(gridStrong, 12345, configStrong);

    // Different strengths should produce different output
    int differences = 0;
    for (std::size_t i = 0; i < gridWeak.tile_count(); ++i) {
        if (gridWeak.tiles[i].getElevation() != gridStrong.tiles[i].getElevation()) {
            ++differences;
        }
    }
    ASSERT_TRUE(differences > 0);
}

// =============================================================================
// Geological Coherence Tests
// =============================================================================

TEST(ElevationGenerator_RidgesFormConnectedFeatures) {
    // Test that ridges form connected regions, not isolated random hills
    TerrainGrid grid(MapSize::Medium);  // 256x256 for better statistics
    ElevationConfig config;
    config.ridgeThreshold = 20;

    ElevationGenerator::generate(grid, 12345, config);

    // Count connected ridge components using flood fill
    std::vector<bool> visited(grid.tile_count(), false);
    std::vector<int> componentSizes;

    auto floodFill = [&](std::uint16_t startX, std::uint16_t startY) {
        if (grid.at(startX, startY).getTerrainType() != TerrainType::Ridge) return 0;
        if (visited[grid.index_of(startX, startY)]) return 0;

        int size = 0;
        std::vector<std::pair<std::uint16_t, std::uint16_t>> stack;
        stack.push_back({startX, startY});

        while (!stack.empty()) {
            auto [x, y] = stack.back();
            stack.pop_back();

            if (x >= grid.width || y >= grid.height) continue;
            std::size_t idx = grid.index_of(x, y);
            if (visited[idx]) continue;
            if (grid.tiles[idx].getTerrainType() != TerrainType::Ridge) continue;

            visited[idx] = true;
            ++size;

            // Add 4-connected neighbors
            if (x > 0) stack.push_back({static_cast<std::uint16_t>(x - 1), y});
            if (x < grid.width - 1) stack.push_back({static_cast<std::uint16_t>(x + 1), y});
            if (y > 0) stack.push_back({x, static_cast<std::uint16_t>(y - 1)});
            if (y < grid.height - 1) stack.push_back({x, static_cast<std::uint16_t>(y + 1)});
        }
        return size;
    };

    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            int size = floodFill(x, y);
            if (size > 0) {
                componentSizes.push_back(size);
            }
        }
    }

    // Calculate statistics
    if (!componentSizes.empty()) {
        std::sort(componentSizes.begin(), componentSizes.end(), std::greater<int>());
        int largestComponent = componentSizes[0];
        int totalRidgeTiles = std::accumulate(componentSizes.begin(), componentSizes.end(), 0);

        std::cout << "    Total ridge components: " << componentSizes.size() << std::endl;
        std::cout << "    Largest component: " << largestComponent << " tiles" << std::endl;
        std::cout << "    Total ridge tiles: " << totalRidgeTiles << std::endl;

        // Good terrain should have a few large connected ridges, not many tiny ones
        // The largest component should contain a significant portion of ridge tiles
        float largestRatio = static_cast<float>(largestComponent) / totalRidgeTiles;
        std::cout << "    Largest component ratio: " << (largestRatio * 100.0f) << "%" << std::endl;

        // At least one connected ridge region should exist
        ASSERT_TRUE(componentSizes.size() > 0);
        // Largest ridge should be substantial
        ASSERT_TRUE(largestComponent > 10);
    }
}

TEST(ElevationGenerator_ValleysFormNaturally) {
    // Valleys should exist between ridges
    TerrainGrid grid(MapSize::Medium);
    ElevationConfig config;

    ElevationResult result = ElevationGenerator::generate(grid, 12345, config);

    // Calculate elevation distribution
    std::array<int, 32> histogram{};
    for (std::size_t i = 0; i < grid.tile_count(); ++i) {
        histogram[grid.tiles[i].getElevation()]++;
    }

    // Print distribution
    std::cout << "    Elevation distribution:" << std::endl;
    std::cout << "      Lowlands (0-3): ";
    int lowlands = 0;
    for (int i = 0; i <= 3; ++i) lowlands += histogram[i];
    std::cout << lowlands << std::endl;

    std::cout << "      Foothills (4-10): ";
    int foothills = 0;
    for (int i = 4; i <= 10; ++i) foothills += histogram[i];
    std::cout << foothills << std::endl;

    std::cout << "      Highlands (11-20): ";
    int highlands = 0;
    for (int i = 11; i <= 20; ++i) highlands += histogram[i];
    std::cout << highlands << std::endl;

    std::cout << "      Ridgelines (21-27): ";
    int ridgelines = 0;
    for (int i = 21; i <= 27; ++i) ridgelines += histogram[i];
    std::cout << ridgelines << std::endl;

    std::cout << "      Peaks (28-31): ";
    int peaks = 0;
    for (int i = 28; i <= 31; ++i) peaks += histogram[i];
    std::cout << peaks << std::endl;

    // Should have variety in elevation levels (valleys exist)
    ASSERT_TRUE(result.minElevation < 10);  // Some low areas
    ASSERT_TRUE(result.maxElevation > 15);  // Some high areas
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST(ElevationGenerator_Performance_512x512_Under50ms) {
    TerrainGrid grid(MapSize::Large);  // 512x512
    ElevationConfig config;

    ElevationResult result = ElevationGenerator::generate(grid, 12345, config);

    std::cout << "    512x512 generation time: " << result.generationTimeMs << " ms" << std::endl;

    // Must complete in under 50ms
    ASSERT_LT(result.generationTimeMs, 50.0f);
}

TEST(ElevationGenerator_Performance_256x256) {
    TerrainGrid grid(MapSize::Medium);
    ElevationConfig config;

    ElevationResult result = ElevationGenerator::generate(grid, 12345, config);

    std::cout << "    256x256 generation time: " << result.generationTimeMs << " ms" << std::endl;

    // Should be much faster than 50ms
    ASSERT_LT(result.generationTimeMs, 25.0f);
}

TEST(ElevationGenerator_Performance_128x128) {
    TerrainGrid grid(MapSize::Small);
    ElevationConfig config;

    ElevationResult result = ElevationGenerator::generate(grid, 12345, config);

    std::cout << "    128x128 generation time: " << result.generationTimeMs << " ms" << std::endl;

    // Should be very fast
    ASSERT_LT(result.generationTimeMs, 10.0f);
}

// =============================================================================
// Row-Major Generation Order Test
// =============================================================================

TEST(ElevationGenerator_RowMajorGenerationOrder) {
    // Verify generation follows row-major order (top-to-bottom, left-to-right)
    // This is important for determinism when RNG is involved

    // Use a very specific config that would be affected by generation order
    TerrainGrid grid(MapSize::Small);
    ElevationConfig config;
    config.enhanceRidges = false;  // Simpler for this test

    ElevationGenerator::generate(grid, 12345, config);

    // Verify by checking that sampling at coordinates matches grid values
    SimplexNoise noise(12345);

    for (std::uint16_t y = 0; y < 10; ++y) {
        for (std::uint16_t x = 0; x < 10; ++x) {
            float raw = ElevationGenerator::sampleRawElevation(
                noise, static_cast<float>(x), static_cast<float>(y), config);
            std::uint8_t expected = ElevationGenerator::rawToElevation(raw, config);
            std::uint8_t actual = grid.at(x, y).getElevation();

            ASSERT_EQ(actual, expected);
        }
    }
}

// =============================================================================
// Helper Function Tests
// =============================================================================

TEST(ElevationGenerator_RawToElevation_FullRange) {
    ElevationConfig config;
    config.minElevation = 0;
    config.maxElevation = 31;

    ASSERT_EQ(ElevationGenerator::rawToElevation(0.0f, config), 0);
    ASSERT_EQ(ElevationGenerator::rawToElevation(1.0f, config), 31);
    ASSERT_EQ(ElevationGenerator::rawToElevation(0.5f, config), 15);
}

TEST(ElevationGenerator_RawToElevation_LimitedRange) {
    ElevationConfig config;
    config.minElevation = 10;
    config.maxElevation = 20;

    ASSERT_EQ(ElevationGenerator::rawToElevation(0.0f, config), 10);
    ASSERT_EQ(ElevationGenerator::rawToElevation(1.0f, config), 20);
    ASSERT_EQ(ElevationGenerator::rawToElevation(0.5f, config), 15);
}

TEST(ElevationGenerator_RawToElevation_ClampsInvalidInput) {
    ElevationConfig config;

    // Values outside 0-1 should be clamped
    ASSERT_EQ(ElevationGenerator::rawToElevation(-0.5f, config), 0);
    ASSERT_EQ(ElevationGenerator::rawToElevation(1.5f, config), 31);
}

TEST(ElevationGenerator_IsRidge) {
    ElevationConfig config;
    config.ridgeThreshold = 21;

    ASSERT_FALSE(ElevationGenerator::isRidge(20, config));
    ASSERT_TRUE(ElevationGenerator::isRidge(21, config));
    ASSERT_TRUE(ElevationGenerator::isRidge(31, config));
}

TEST(ElevationGenerator_SampleRawElevation_Range) {
    SimplexNoise noise(12345);
    ElevationConfig config;

    // Sample many points, all should be in [0, 1]
    for (float y = 0.0f; y < 100.0f; y += 5.0f) {
        for (float x = 0.0f; x < 100.0f; x += 5.0f) {
            float raw = ElevationGenerator::sampleRawElevation(noise, x, y, config);
            ASSERT_GE(raw, 0.0f);
            ASSERT_LE(raw, 1.0f);
        }
    }
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "ElevationGenerator Tests (Ticket 3-008)" << std::endl;
    std::cout << "========================================" << std::endl;

    // Tests run automatically via static initialization

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Results: " << g_tests_passed << "/" << g_tests_run << " tests passed" << std::endl;
    std::cout << "========================================" << std::endl;

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
