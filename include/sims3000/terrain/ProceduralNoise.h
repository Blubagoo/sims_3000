/**
 * @file ProceduralNoise.h
 * @brief Deterministic RNG and noise functions for procedural terrain generation.
 *
 * Provides cross-platform deterministic random number generation and noise:
 * - xoshiro256**: Fast, high-quality PRNG with 256-bit state
 * - Simplex noise: 2D gradient noise, deterministic across platforms
 * - fBm: Fractal Brownian motion for multi-octave noise
 *
 * Cross-platform determinism is achieved by:
 * - Using xoshiro256** (portable, well-defined algorithm)
 * - Fixed-point intermediate calculations where precision matters
 * - Strict floating-point semantics (compile with /fp:strict or -ffp-contract=off)
 * - Single-threaded generation to ensure consistent RNG call order
 *
 * @note All generation MUST be single-threaded for deterministic RNG call order.
 * @note Compile with /fp:strict (MSVC) or -ffp-contract=off (GCC/Clang).
 *
 * @see /docs/canon/patterns.yaml (multiplayer.synchronization.rules)
 */

#ifndef SIMS3000_TERRAIN_PROCEDURALNOISE_H
#define SIMS3000_TERRAIN_PROCEDURALNOISE_H

#include <cstdint>
#include <array>

namespace sims3000 {
namespace terrain {

/**
 * @class Xoshiro256
 * @brief xoshiro256** pseudo-random number generator.
 *
 * Fast, high-quality PRNG with 256-bit state and period 2^256 - 1.
 * Passes all statistical tests and provides deterministic output
 * across all platforms when initialized with the same seed.
 *
 * Reference: https://prng.di.unimi.it/
 *
 * @note NOT thread-safe. Use one instance per thread if needed.
 */
class Xoshiro256 {
public:
    /**
     * @brief State size in 64-bit words.
     */
    static constexpr std::size_t STATE_SIZE = 4;

    /**
     * @brief Create PRNG with seed 0.
     *
     * Initializes state using SplitMix64 to expand seed to 256 bits.
     */
    Xoshiro256();

    /**
     * @brief Create PRNG with specified seed.
     *
     * Uses SplitMix64 to expand the 64-bit seed into 256-bit state.
     * Same seed always produces same sequence.
     *
     * @param seed Initial 64-bit seed value.
     */
    explicit Xoshiro256(std::uint64_t seed);

    /**
     * @brief Set seed and reset state.
     *
     * Reinitializes the generator with a new seed.
     *
     * @param seed New 64-bit seed value.
     */
    void setSeed(std::uint64_t seed);

    /**
     * @brief Get the current seed.
     *
     * @return The seed used to initialize the generator.
     */
    std::uint64_t getSeed() const { return m_seed; }

    /**
     * @brief Generate next 64-bit random value.
     *
     * Advances the internal state and returns the scrambled result.
     *
     * @return Random 64-bit value with uniform distribution.
     */
    std::uint64_t next();

    /**
     * @brief Generate random integer in range [0, max) (exclusive).
     *
     * Uses rejection sampling for uniform distribution.
     *
     * @param max Exclusive upper bound (must be > 0).
     * @return Random integer in [0, max).
     */
    std::uint32_t nextUint32(std::uint32_t max);

    /**
     * @brief Generate random integer in range [min, max] (inclusive).
     *
     * @param min Minimum value (inclusive).
     * @param max Maximum value (inclusive).
     * @return Random integer in [min, max].
     */
    std::int32_t nextInt32(std::int32_t min, std::int32_t max);

    /**
     * @brief Generate random float in range [0.0, 1.0).
     *
     * Uses top 53 bits for maximum precision in the [0,1) range.
     *
     * @return Random double in [0.0, 1.0).
     */
    double nextDouble();

    /**
     * @brief Generate random float in range [0.0f, 1.0f).
     *
     * @return Random float in [0.0f, 1.0f).
     */
    float nextFloat();

    /**
     * @brief Generate random float in range [min, max).
     *
     * @param min Minimum value (inclusive).
     * @param max Maximum value (exclusive).
     * @return Random float in [min, max).
     */
    float nextFloat(float min, float max);

    /**
     * @brief Jump forward 2^128 calls.
     *
     * Useful for creating non-overlapping subsequences.
     * Equivalent to calling next() 2^128 times.
     */
    void jump();

    /**
     * @brief Get internal state for serialization.
     *
     * @return Array of 4 64-bit state values.
     */
    std::array<std::uint64_t, STATE_SIZE> getState() const;

    /**
     * @brief Set internal state from serialization.
     *
     * @param state Array of 4 64-bit state values.
     */
    void setState(const std::array<std::uint64_t, STATE_SIZE>& state);

private:
    std::array<std::uint64_t, STATE_SIZE> m_state;
    std::uint64_t m_seed;

    /**
     * @brief Rotate left by k bits.
     */
    static std::uint64_t rotl(std::uint64_t x, int k);

    /**
     * @brief SplitMix64 for seed expansion.
     */
    static std::uint64_t splitmix64(std::uint64_t& x);
};

/**
 * @struct NoiseConfig
 * @brief Configuration for noise generation.
 *
 * Defines parameters for fBm (fractal Brownian motion) noise.
 */
struct NoiseConfig {
    std::uint8_t octaves = 4;          ///< Number of noise octaves (1-8)
    float lacunarity = 2.0f;           ///< Frequency multiplier per octave
    float persistence = 0.5f;          ///< Amplitude multiplier per octave
    float scale = 1.0f;                ///< Base frequency scale
    float amplitude = 1.0f;            ///< Base amplitude
    std::int32_t seed_offset = 0;      ///< Seed offset for different noise layers

    /**
     * @brief Default configuration for terrain heightmaps.
     */
    static NoiseConfig terrain() {
        NoiseConfig cfg;
        cfg.octaves = 6;
        cfg.lacunarity = 2.0f;
        cfg.persistence = 0.5f;
        cfg.scale = 0.01f;
        cfg.amplitude = 1.0f;
        return cfg;
    }

    /**
     * @brief Configuration for moisture/humidity maps.
     */
    static NoiseConfig moisture() {
        NoiseConfig cfg;
        cfg.octaves = 4;
        cfg.lacunarity = 2.2f;
        cfg.persistence = 0.45f;
        cfg.scale = 0.02f;
        cfg.amplitude = 1.0f;
        cfg.seed_offset = 1000;
        return cfg;
    }
};

/**
 * @class SimplexNoise
 * @brief 2D Simplex noise generator with deterministic output.
 *
 * Implements Ken Perlin's Simplex noise algorithm for 2D.
 * Uses a seeded permutation table for deterministic, reproducible output.
 *
 * Simplex noise advantages over Perlin noise:
 * - Lower computational complexity O(n^2) vs O(2^n)
 * - No visible directional artifacts
 * - Well-defined analytical derivative
 *
 * @note Thread-safe after construction (const methods only).
 */
class SimplexNoise {
public:
    /**
     * @brief Create Simplex noise generator with default seed (0).
     */
    SimplexNoise();

    /**
     * @brief Create Simplex noise generator with specified seed.
     *
     * Different seeds produce different but reproducible noise patterns.
     *
     * @param seed 64-bit seed for permutation table generation.
     */
    explicit SimplexNoise(std::uint64_t seed);

    /**
     * @brief Reinitialize with new seed.
     *
     * Regenerates the permutation table.
     *
     * @param seed New 64-bit seed value.
     */
    void setSeed(std::uint64_t seed);

    /**
     * @brief Get the current seed.
     *
     * @return The seed used to initialize the noise generator.
     */
    std::uint64_t getSeed() const { return m_seed; }

    /**
     * @brief Sample 2D Simplex noise at (x, y).
     *
     * Returns value in range approximately [-1.0, 1.0].
     * Actual range may be slightly smaller due to gradient dot products.
     *
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return Noise value in approximately [-1.0, 1.0].
     */
    float noise2D(float x, float y) const;

    /**
     * @brief Sample 2D Simplex noise with integer coordinates.
     *
     * Useful for fixed-point coordinate systems. Coordinates are
     * scaled by 1/256 internally for sub-tile precision.
     *
     * @param x X coordinate (fixed-point, 8.8 format).
     * @param y Y coordinate (fixed-point, 8.8 format).
     * @return Noise value scaled to [-32768, 32767].
     */
    std::int32_t noise2DInt(std::int32_t x, std::int32_t y) const;

    /**
     * @brief Sample fBm (fractal Brownian motion) at (x, y).
     *
     * Combines multiple octaves of Simplex noise with different
     * frequencies and amplitudes for natural-looking terrain.
     *
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param config Noise configuration (octaves, lacunarity, persistence).
     * @return fBm value, range depends on octave count and persistence.
     */
    float fbm2D(float x, float y, const NoiseConfig& config) const;

    /**
     * @brief Sample normalized fBm at (x, y).
     *
     * Returns fBm value normalized to [0.0, 1.0] range.
     *
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param config Noise configuration.
     * @return Normalized fBm value in [0.0, 1.0].
     */
    float fbm2DNormalized(float x, float y, const NoiseConfig& config) const;

    /**
     * @brief Sample fBm with integer output for deterministic grid generation.
     *
     * Returns value in range [0, 255] for elevation/moisture mapping.
     *
     * @param x X grid coordinate.
     * @param y Y grid coordinate.
     * @param config Noise configuration.
     * @return fBm value in [0, 255].
     */
    std::uint8_t fbm2DUint8(float x, float y, const NoiseConfig& config) const;

private:
    static constexpr std::size_t PERM_SIZE = 256;
    static constexpr std::size_t PERM_MASK = PERM_SIZE - 1;

    std::array<std::uint8_t, PERM_SIZE * 2> m_perm;
    std::uint64_t m_seed;

    /**
     * @brief Initialize permutation table from seed.
     */
    void initPermutation(std::uint64_t seed);

    /**
     * @brief Fast floor for positive and negative values.
     */
    static std::int32_t fastFloor(float x);

    /**
     * @brief Gradient dot product at corner.
     */
    float grad(std::int32_t hash, float x, float y) const;
};

/**
 * @brief Golden test output for cross-platform verification.
 *
 * When seed = 12345 and sampling at specific coordinates,
 * these exact values must be produced on all platforms.
 *
 * Use verifyGoldenOutput() to test platform compliance.
 *
 * These values were computed on Windows with MSVC 19.44 /fp:strict
 * and must match on all other platforms.
 */
struct GoldenOutput {
    /// PRNG output: first 8 values from xoshiro256** with seed 12345
    static constexpr std::array<std::uint64_t, 8> XOSHIRO_VALUES = {
        0xbe6a36374160d49bULL,
        0x214aaa0637a688c6ULL,
        0xf69d16de9954d388ULL,
        0x0c60048c4e96e033ULL,
        0x8e2076aeed51c648ULL,
        0x02bbcc1c1fc50f84ULL,
        0x28e72a4fec84f699ULL,
        0x4bb9d7cbb8dddebeuLL
    };

    /// Simplex noise output: noise2D at (0, 0), (1, 0), (0, 1), (1, 1)
    /// Values generated with seed 12345
    static constexpr std::array<float, 4> SIMPLEX_VALUES = {
        0.0f,                    // noise2D(0, 0) - origin is always 0
        0.4950941503f,           // noise2D(1, 0)
        0.4208022058f,           // noise2D(0, 1)
        -0.9173749685f           // noise2D(1, 1)
    };

    /// fBm output at grid positions (64, 64), (128, 128), (192, 192), (256, 256)
    /// Using NoiseConfig::terrain() with seed 12345
    static constexpr std::array<std::uint8_t, 4> FBM_VALUES = {
        133, 172, 163, 107
    };
};

/**
 * @brief Verify cross-platform determinism with golden output.
 *
 * Tests that the PRNG and noise functions produce identical output
 * to the reference values. Call this in CI to verify platform compliance.
 *
 * @param out_error If non-null, receives error description on failure.
 * @return true if all golden values match, false otherwise.
 */
bool verifyGoldenOutput(const char** out_error = nullptr);

/**
 * @brief Generate golden output values for reference.
 *
 * Generates the expected values for seed 12345 that can be used
 * as the reference for cross-platform verification.
 *
 * @param out_xoshiro Output buffer for 8 PRNG values.
 * @param out_simplex Output buffer for 4 simplex noise values.
 * @param out_fbm Output buffer for 4 fBm values.
 */
void generateGoldenOutput(
    std::array<std::uint64_t, 8>& out_xoshiro,
    std::array<float, 4>& out_simplex,
    std::array<std::uint8_t, 4>& out_fbm);

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_PROCEDURALNOISE_H
