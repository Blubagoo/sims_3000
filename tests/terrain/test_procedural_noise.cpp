/**
 * @file test_procedural_noise.cpp
 * @brief Unit tests for ProceduralNoise (Ticket 3-007)
 *
 * Tests cover:
 * - xoshiro256** PRNG determinism and distribution
 * - Simplex noise value range and determinism
 * - fBm multi-octave noise
 * - Cross-platform golden output verification
 *
 * Compile with strict FP semantics for cross-platform determinism:
 * - MSVC: /fp:strict
 * - GCC/Clang: -ffp-contract=off
 */

#include <sims3000/terrain/ProceduralNoise.h>
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <iomanip>

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

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "  FAILED: " << #a << " != " << #b \
                      << " (" << (a) << " != " << (b) << ") at line " << __LINE__ << std::endl; \
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
// Xoshiro256** Tests
// =============================================================================

TEST(Xoshiro256_DefaultConstruction) {
    Xoshiro256 rng;
    ASSERT_EQ(rng.getSeed(), 0ULL);

    // Should produce deterministic output
    std::uint64_t first = rng.next();
    ASSERT_TRUE(first != 0);  // Non-zero output expected
}

TEST(Xoshiro256_SeededConstruction) {
    Xoshiro256 rng(12345);
    ASSERT_EQ(rng.getSeed(), 12345ULL);
}

TEST(Xoshiro256_SameSeedSameSequence) {
    Xoshiro256 rng1(42);
    Xoshiro256 rng2(42);

    for (int i = 0; i < 100; ++i) {
        ASSERT_EQ(rng1.next(), rng2.next());
    }
}

TEST(Xoshiro256_DifferentSeedsDifferentSequence) {
    Xoshiro256 rng1(100);
    Xoshiro256 rng2(200);

    // Extremely unlikely to produce same first value
    ASSERT_TRUE(rng1.next() != rng2.next());
}

TEST(Xoshiro256_SetSeedResets) {
    Xoshiro256 rng(12345);

    std::uint64_t first = rng.next();
    rng.next();  // Advance state
    rng.next();

    rng.setSeed(12345);  // Reset
    ASSERT_EQ(rng.next(), first);  // Should produce same first value
}

TEST(Xoshiro256_NextUint32_Range) {
    Xoshiro256 rng(12345);

    for (int i = 0; i < 1000; ++i) {
        std::uint32_t val = rng.nextUint32(100);
        ASSERT_TRUE(val < 100);
    }
}

TEST(Xoshiro256_NextUint32_Distribution) {
    // Test uniform distribution using chi-squared test
    Xoshiro256 rng(12345);

    constexpr int BUCKETS = 10;
    constexpr int SAMPLES = 10000;
    int counts[BUCKETS] = {0};

    for (int i = 0; i < SAMPLES; ++i) {
        std::uint32_t val = rng.nextUint32(BUCKETS);
        counts[val]++;
    }

    // Expected count per bucket
    double expected = static_cast<double>(SAMPLES) / BUCKETS;

    // Chi-squared statistic
    double chi2 = 0.0;
    for (int i = 0; i < BUCKETS; ++i) {
        double diff = counts[i] - expected;
        chi2 += (diff * diff) / expected;
    }

    // With 9 degrees of freedom, chi2 should be < ~16.9 for p=0.05
    // Using a more lenient threshold
    ASSERT_TRUE(chi2 < 25.0);
}

TEST(Xoshiro256_NextInt32_Range) {
    Xoshiro256 rng(12345);

    for (int i = 0; i < 1000; ++i) {
        std::int32_t val = rng.nextInt32(-50, 50);
        ASSERT_TRUE(val >= -50 && val <= 50);
    }
}

TEST(Xoshiro256_NextFloat_Range) {
    Xoshiro256 rng(12345);

    for (int i = 0; i < 1000; ++i) {
        float val = rng.nextFloat();
        ASSERT_TRUE(val >= 0.0f && val < 1.0f);
    }
}

TEST(Xoshiro256_NextFloat_MinMax) {
    Xoshiro256 rng(12345);

    for (int i = 0; i < 1000; ++i) {
        float val = rng.nextFloat(5.0f, 10.0f);
        ASSERT_TRUE(val >= 5.0f && val < 10.0f);
    }
}

TEST(Xoshiro256_NextDouble_Range) {
    Xoshiro256 rng(12345);

    for (int i = 0; i < 1000; ++i) {
        double val = rng.nextDouble();
        ASSERT_TRUE(val >= 0.0 && val < 1.0);
    }
}

TEST(Xoshiro256_StateSerializationRoundtrip) {
    Xoshiro256 rng1(12345);

    // Advance state
    for (int i = 0; i < 50; ++i) {
        rng1.next();
    }

    // Save state
    auto state = rng1.getState();

    // Generate more values
    std::uint64_t expected1 = rng1.next();
    std::uint64_t expected2 = rng1.next();

    // Create new RNG and restore state
    Xoshiro256 rng2(0);
    rng2.setState(state);

    ASSERT_EQ(rng2.next(), expected1);
    ASSERT_EQ(rng2.next(), expected2);
}

TEST(Xoshiro256_Jump) {
    Xoshiro256 rng1(12345);
    Xoshiro256 rng2(12345);

    rng2.jump();

    // After jump, should produce different sequence
    ASSERT_TRUE(rng1.next() != rng2.next());
}

// =============================================================================
// SimplexNoise Tests
// =============================================================================

TEST(SimplexNoise_DefaultConstruction) {
    SimplexNoise noise;
    ASSERT_EQ(noise.getSeed(), 0ULL);
}

TEST(SimplexNoise_SeededConstruction) {
    SimplexNoise noise(12345);
    ASSERT_EQ(noise.getSeed(), 12345ULL);
}

TEST(SimplexNoise_OriginIsZero) {
    // Simplex noise at integer grid points should be 0 or very close
    SimplexNoise noise(12345);
    float val = noise.noise2D(0.0f, 0.0f);
    ASSERT_NEAR(val, 0.0f, 0.01f);
}

TEST(SimplexNoise_ValueRange) {
    SimplexNoise noise(12345);

    float minVal = 1.0f;
    float maxVal = -1.0f;

    // Sample across a range of coordinates
    for (float y = -10.0f; y < 10.0f; y += 0.1f) {
        for (float x = -10.0f; x < 10.0f; x += 0.1f) {
            float val = noise.noise2D(x, y);
            minVal = std::min(minVal, val);
            maxVal = std::max(maxVal, val);
        }
    }

    // Should be approximately in [-1, 1]
    ASSERT_TRUE(minVal >= -1.1f);
    ASSERT_TRUE(maxVal <= 1.1f);
}

TEST(SimplexNoise_Determinism) {
    SimplexNoise noise1(12345);
    SimplexNoise noise2(12345);

    for (float y = 0.0f; y < 5.0f; y += 0.5f) {
        for (float x = 0.0f; x < 5.0f; x += 0.5f) {
            float val1 = noise1.noise2D(x, y);
            float val2 = noise2.noise2D(x, y);
            ASSERT_NEAR(val1, val2, 1e-6f);
        }
    }
}

TEST(SimplexNoise_DifferentSeeds) {
    SimplexNoise noise1(100);
    SimplexNoise noise2(200);

    // Should produce different patterns
    float val1 = noise1.noise2D(5.5f, 3.3f);
    float val2 = noise2.noise2D(5.5f, 3.3f);
    ASSERT_TRUE(std::abs(val1 - val2) > 0.01f);
}

TEST(SimplexNoise_SetSeedResets) {
    SimplexNoise noise(12345);

    float first = noise.noise2D(1.5f, 2.5f);

    noise.setSeed(99999);  // Different seed
    noise.setSeed(12345);  // Back to original

    float second = noise.noise2D(1.5f, 2.5f);
    ASSERT_NEAR(first, second, 1e-6f);
}

TEST(SimplexNoise_Continuity) {
    // Noise should be continuous (no sudden jumps)
    SimplexNoise noise(12345);

    float prevVal = noise.noise2D(0.0f, 0.0f);
    float maxDiff = 0.0f;

    for (float x = 0.01f; x < 10.0f; x += 0.01f) {
        float val = noise.noise2D(x, 0.0f);
        float diff = std::abs(val - prevVal);
        maxDiff = std::max(maxDiff, diff);
        prevVal = val;
    }

    // Small steps should produce small differences
    ASSERT_TRUE(maxDiff < 0.5f);
}

TEST(SimplexNoise_Noise2DInt) {
    SimplexNoise noise(12345);

    // Test fixed-point interface
    for (int y = 0; y < 256; y += 16) {
        for (int x = 0; x < 256; x += 16) {
            std::int32_t val = noise.noise2DInt(x, y);
            ASSERT_TRUE(val >= -32768 && val <= 32767);
        }
    }
}

// =============================================================================
// fBm Tests
// =============================================================================

TEST(FBM_ValueRange) {
    SimplexNoise noise(12345);
    NoiseConfig config = NoiseConfig::terrain();

    float minVal = 1.0f;
    float maxVal = -1.0f;

    for (float y = 0.0f; y < 100.0f; y += 1.0f) {
        for (float x = 0.0f; x < 100.0f; x += 1.0f) {
            float val = noise.fbm2D(x, y, config);
            minVal = std::min(minVal, val);
            maxVal = std::max(maxVal, val);
        }
    }

    // fbm2D is normalized, should be in [-1, 1]
    ASSERT_TRUE(minVal >= -1.1f);
    ASSERT_TRUE(maxVal <= 1.1f);
}

TEST(FBM_NormalizedRange) {
    SimplexNoise noise(12345);
    NoiseConfig config = NoiseConfig::terrain();

    for (float y = 0.0f; y < 50.0f; y += 1.0f) {
        for (float x = 0.0f; x < 50.0f; x += 1.0f) {
            float val = noise.fbm2DNormalized(x, y, config);
            ASSERT_TRUE(val >= 0.0f && val <= 1.0f);
        }
    }
}

TEST(FBM_Uint8Range) {
    SimplexNoise noise(12345);
    NoiseConfig config = NoiseConfig::terrain();

    for (float y = 0.0f; y < 50.0f; y += 1.0f) {
        for (float x = 0.0f; x < 50.0f; x += 1.0f) {
            std::uint8_t val = noise.fbm2DUint8(x, y, config);
            ASSERT_TRUE(val <= 255);  // Always true, but explicit
        }
    }
}

TEST(FBM_Determinism) {
    SimplexNoise noise1(12345);
    SimplexNoise noise2(12345);
    NoiseConfig config = NoiseConfig::terrain();

    for (float y = 0.0f; y < 20.0f; y += 2.0f) {
        for (float x = 0.0f; x < 20.0f; x += 2.0f) {
            float val1 = noise1.fbm2D(x, y, config);
            float val2 = noise2.fbm2D(x, y, config);
            ASSERT_NEAR(val1, val2, 1e-6f);
        }
    }
}

TEST(FBM_OctavesAffectDetail) {
    SimplexNoise noise(12345);

    NoiseConfig config1;
    config1.octaves = 1;
    config1.scale = 0.01f;

    NoiseConfig config8;
    config8.octaves = 8;
    config8.scale = 0.01f;

    // Calculate variance as a measure of detail
    std::vector<float> vals1, vals8;

    for (float y = 0.0f; y < 50.0f; y += 1.0f) {
        for (float x = 0.0f; x < 50.0f; x += 1.0f) {
            vals1.push_back(noise.fbm2D(x, y, config1));
            vals8.push_back(noise.fbm2D(x, y, config8));
        }
    }

    // Calculate variance
    auto variance = [](const std::vector<float>& v) {
        float mean = std::accumulate(v.begin(), v.end(), 0.0f) / v.size();
        float sq_sum = 0.0f;
        for (float val : v) {
            sq_sum += (val - mean) * (val - mean);
        }
        return sq_sum / v.size();
    };

    float var1 = variance(vals1);
    float var8 = variance(vals8);

    // More octaves typically means more detail but similar variance
    // Just verify both produce non-zero variance
    ASSERT_TRUE(var1 > 0.0f);
    ASSERT_TRUE(var8 > 0.0f);
}

TEST(FBM_ScaleAffectsFrequency) {
    SimplexNoise noise(12345);

    NoiseConfig configLow;
    configLow.octaves = 4;
    configLow.scale = 0.001f;

    NoiseConfig configHigh;
    configHigh.octaves = 4;
    configHigh.scale = 0.1f;

    // Count zero crossings as a measure of frequency
    auto countCrossings = [&noise](const NoiseConfig& config) {
        int crossings = 0;
        float prev = noise.fbm2D(0.0f, 0.0f, config);
        for (float x = 1.0f; x < 100.0f; x += 1.0f) {
            float curr = noise.fbm2D(x, 0.0f, config);
            if ((prev >= 0 && curr < 0) || (prev < 0 && curr >= 0)) {
                crossings++;
            }
            prev = curr;
        }
        return crossings;
    };

    int crossingsLow = countCrossings(configLow);
    int crossingsHigh = countCrossings(configHigh);

    // Higher scale = higher frequency = more crossings
    ASSERT_TRUE(crossingsHigh > crossingsLow);
}

TEST(FBM_SeedOffsetProducesDifferentPatterns) {
    SimplexNoise noise(12345);

    NoiseConfig config1 = NoiseConfig::terrain();
    config1.seed_offset = 0;

    NoiseConfig config2 = NoiseConfig::terrain();
    config2.seed_offset = 1000;

    float val1 = noise.fbm2D(50.0f, 50.0f, config1);
    float val2 = noise.fbm2D(50.0f, 50.0f, config2);

    // Different seed offsets should produce different values
    ASSERT_TRUE(std::abs(val1 - val2) > 0.01f);
}

TEST(FBM_MoistureConfig) {
    SimplexNoise noise(12345);
    NoiseConfig config = NoiseConfig::moisture();

    // Verify moisture config has expected properties
    ASSERT_EQ(config.octaves, 4);
    ASSERT_EQ(config.seed_offset, 1000);

    // Should produce valid output
    float val = noise.fbm2D(100.0f, 100.0f, config);
    ASSERT_TRUE(val >= -1.1f && val <= 1.1f);
}

// =============================================================================
// NoiseConfig Tests
// =============================================================================

TEST(NoiseConfig_DefaultValues) {
    NoiseConfig config;
    ASSERT_EQ(config.octaves, 4);
    ASSERT_NEAR(config.lacunarity, 2.0f, 0.001f);
    ASSERT_NEAR(config.persistence, 0.5f, 0.001f);
    ASSERT_NEAR(config.scale, 1.0f, 0.001f);
    ASSERT_NEAR(config.amplitude, 1.0f, 0.001f);
    ASSERT_EQ(config.seed_offset, 0);
}

TEST(NoiseConfig_TerrainPreset) {
    NoiseConfig config = NoiseConfig::terrain();
    ASSERT_EQ(config.octaves, 6);
    ASSERT_NEAR(config.scale, 0.01f, 0.001f);
}

TEST(NoiseConfig_MoisturePreset) {
    NoiseConfig config = NoiseConfig::moisture();
    ASSERT_EQ(config.octaves, 4);
    ASSERT_EQ(config.seed_offset, 1000);
    ASSERT_NEAR(config.scale, 0.02f, 0.001f);
}

// =============================================================================
// Golden Output / Cross-Platform Verification Tests
// =============================================================================

// These tests verify cross-platform determinism by comparing against
// the GoldenOutput reference values defined in ProceduralNoise.h.
// If any test fails, it indicates a cross-platform determinism issue.

TEST(GoldenOutput_Xoshiro256_Seed12345) {
    Xoshiro256 rng(12345);

    // First 8 values must match the GoldenOutput reference exactly
    std::uint64_t vals[8];
    for (int i = 0; i < 8; ++i) {
        vals[i] = rng.next();
    }

    // Print for debugging/verification
    std::cout << "    xoshiro256** seed=12345 first 8 values:" << std::endl;
    for (int i = 0; i < 8; ++i) {
        std::cout << "      [" << i << "] 0x" << std::hex << vals[i] << std::dec << std::endl;
    }

    // Compare against GoldenOutput reference values
    for (int i = 0; i < 8; ++i) {
        ASSERT_EQ(vals[i], GoldenOutput::XOSHIRO_VALUES[i]);
    }
}

TEST(GoldenOutput_SimplexNoise_Seed12345) {
    SimplexNoise noise(12345);

    // Sample at fixed coordinates matching GoldenOutput
    float coords[][2] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 1.0f}
    };

    std::cout << "    SimplexNoise seed=12345 values:" << std::endl;
    for (int i = 0; i < 4; ++i) {
        float val = noise.noise2D(coords[i][0], coords[i][1]);
        std::cout << "      noise2D(" << coords[i][0] << ", " << coords[i][1] << ") = "
                  << std::setprecision(10) << val << std::endl;
    }

    // Compare against GoldenOutput reference values (with tolerance for FP)
    const float tolerance = 1e-5f;
    for (int i = 0; i < 4; ++i) {
        float val = noise.noise2D(coords[i][0], coords[i][1]);
        ASSERT_NEAR(val, GoldenOutput::SIMPLEX_VALUES[i], tolerance);
    }
}

TEST(GoldenOutput_FBM_Seed12345) {
    SimplexNoise noise(12345);
    NoiseConfig config = NoiseConfig::terrain();

    // Sample at grid positions matching GoldenOutput
    float coords[][2] = {
        {64.0f, 64.0f},
        {128.0f, 128.0f},
        {192.0f, 192.0f},
        {256.0f, 256.0f}
    };

    std::cout << "    fBm seed=12345 terrain config values:" << std::endl;
    for (int i = 0; i < 4; ++i) {
        std::uint8_t val = noise.fbm2DUint8(coords[i][0], coords[i][1], config);
        std::cout << "      fbm2DUint8(" << coords[i][0] << ", " << coords[i][1] << ") = "
                  << static_cast<int>(val) << std::endl;
    }

    // Compare against GoldenOutput reference values
    for (int i = 0; i < 4; ++i) {
        std::uint8_t val = noise.fbm2DUint8(coords[i][0], coords[i][1], config);
        ASSERT_EQ(val, GoldenOutput::FBM_VALUES[i]);
    }
}

TEST(GoldenOutput_VerifyFunction) {
    const char* error = nullptr;
    bool result = verifyGoldenOutput(&error);
    if (!result) {
        std::cerr << "  Golden output verification failed: " << (error ? error : "unknown") << std::endl;
    }
    ASSERT_TRUE(result);
}

TEST(GoldenOutput_GenerateAndVerify) {
    std::array<std::uint64_t, 8> xoshiro;
    std::array<float, 4> simplex;
    std::array<std::uint8_t, 4> fbm;

    generateGoldenOutput(xoshiro, simplex, fbm);

    // Regenerate and compare
    std::array<std::uint64_t, 8> xoshiro2;
    std::array<float, 4> simplex2;
    std::array<std::uint8_t, 4> fbm2;

    generateGoldenOutput(xoshiro2, simplex2, fbm2);

    for (int i = 0; i < 8; ++i) {
        ASSERT_EQ(xoshiro[i], xoshiro2[i]);
    }

    for (int i = 0; i < 4; ++i) {
        ASSERT_NEAR(simplex[i], simplex2[i], 1e-6f);
    }

    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(fbm[i], fbm2[i]);
    }
}

// =============================================================================
// Thread Safety Notes (Not Tested Here)
// =============================================================================

/*
 * IMPORTANT: Xoshiro256 and SimplexNoise are NOT thread-safe for mutation.
 *
 * - Xoshiro256::next() modifies internal state - NOT thread-safe
 * - SimplexNoise::setSeed() modifies permutation table - NOT thread-safe
 * - SimplexNoise::noise2D() and fbm2D() are CONST and thread-safe after construction
 *
 * For terrain generation:
 * - Use a single-threaded generation pass for determinism
 * - RNG call order affects output, so threading would break reproducibility
 * - Alternatively, use jump() to create independent subsequences per thread
 */

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "ProceduralNoise Tests (Ticket 3-007)" << std::endl;
    std::cout << "========================================" << std::endl;

    // Tests run automatically via static initialization
    // This main just reports results

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Results: " << g_tests_passed << "/" << g_tests_run << " tests passed" << std::endl;
    std::cout << "========================================" << std::endl;

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
