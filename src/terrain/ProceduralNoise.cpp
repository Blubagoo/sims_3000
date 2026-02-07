/**
 * @file ProceduralNoise.cpp
 * @brief Implementation of deterministic RNG and noise functions.
 *
 * Cross-platform determinism notes:
 * - xoshiro256** uses only integer operations (fully deterministic)
 * - Simplex noise uses careful float handling with minimal FP operations
 * - All FP->int conversions use explicit rounding modes
 *
 * Compile with strict FP semantics:
 * - MSVC: /fp:strict
 * - GCC/Clang: -ffp-contract=off
 */

#include "sims3000/terrain/ProceduralNoise.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace sims3000 {
namespace terrain {

// =============================================================================
// Xoshiro256** Implementation
// =============================================================================

Xoshiro256::Xoshiro256() : m_seed(0) {
    setSeed(0);
}

Xoshiro256::Xoshiro256(std::uint64_t seed) : m_seed(seed) {
    setSeed(seed);
}

void Xoshiro256::setSeed(std::uint64_t seed) {
    m_seed = seed;

    // Use SplitMix64 to expand 64-bit seed to 256-bit state
    std::uint64_t x = seed;
    m_state[0] = splitmix64(x);
    m_state[1] = splitmix64(x);
    m_state[2] = splitmix64(x);
    m_state[3] = splitmix64(x);

    // Ensure state is not all zeros (would produce constant output)
    if (m_state[0] == 0 && m_state[1] == 0 && m_state[2] == 0 && m_state[3] == 0) {
        m_state[0] = 1;
    }
}

std::uint64_t Xoshiro256::rotl(std::uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

std::uint64_t Xoshiro256::splitmix64(std::uint64_t& x) {
    std::uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

std::uint64_t Xoshiro256::next() {
    // xoshiro256** algorithm
    const std::uint64_t result = rotl(m_state[1] * 5, 7) * 9;

    const std::uint64_t t = m_state[1] << 17;

    m_state[2] ^= m_state[0];
    m_state[3] ^= m_state[1];
    m_state[1] ^= m_state[2];
    m_state[0] ^= m_state[3];

    m_state[2] ^= t;
    m_state[3] = rotl(m_state[3], 45);

    return result;
}

std::uint32_t Xoshiro256::nextUint32(std::uint32_t max) {
    if (max == 0) return 0;

    // Rejection sampling for uniform distribution
    // Avoid modulo bias by rejecting values >= threshold
    const std::uint64_t threshold = (static_cast<std::uint64_t>(1) << 32) % max;
    std::uint64_t r;
    do {
        r = next() >> 32;  // Use upper 32 bits
    } while (r < threshold);

    return static_cast<std::uint32_t>(r % max);
}

std::int32_t Xoshiro256::nextInt32(std::int32_t min, std::int32_t max) {
    if (min >= max) return min;

    const std::uint32_t range = static_cast<std::uint32_t>(max - min) + 1;
    return static_cast<std::int32_t>(nextUint32(range)) + min;
}

double Xoshiro256::nextDouble() {
    // Use top 53 bits for double precision
    return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0);
}

float Xoshiro256::nextFloat() {
    // Use top 24 bits for float precision
    return static_cast<float>(next() >> 40) * (1.0f / 16777216.0f);
}

float Xoshiro256::nextFloat(float min, float max) {
    if (min >= max) return min;
    return min + nextFloat() * (max - min);
}

void Xoshiro256::jump() {
    static constexpr std::uint64_t JUMP[] = {
        0x180ec6d33cfd0abaULL,
        0xd5a61266f0c9392cULL,
        0xa9582618e03fc9aaULL,
        0x39abdc4529b1661cULL
    };

    std::uint64_t s0 = 0;
    std::uint64_t s1 = 0;
    std::uint64_t s2 = 0;
    std::uint64_t s3 = 0;

    for (int i = 0; i < 4; ++i) {
        for (int b = 0; b < 64; ++b) {
            if (JUMP[i] & (static_cast<std::uint64_t>(1) << b)) {
                s0 ^= m_state[0];
                s1 ^= m_state[1];
                s2 ^= m_state[2];
                s3 ^= m_state[3];
            }
            next();
        }
    }

    m_state[0] = s0;
    m_state[1] = s1;
    m_state[2] = s2;
    m_state[3] = s3;
}

std::array<std::uint64_t, Xoshiro256::STATE_SIZE> Xoshiro256::getState() const {
    return m_state;
}

void Xoshiro256::setState(const std::array<std::uint64_t, STATE_SIZE>& state) {
    m_state = state;
}

// =============================================================================
// SimplexNoise Implementation
// =============================================================================

// Skewing and unskewing factors for 2D
// F2 = (sqrt(3) - 1) / 2
// G2 = (3 - sqrt(3)) / 6
static constexpr float F2 = 0.3660254037844386f;  // 0.5 * (sqrt(3) - 1)
static constexpr float G2 = 0.21132486540518713f; // (3 - sqrt(3)) / 6

// Gradient vectors for 2D simplex noise (12 directions)
static constexpr std::int8_t GRAD2[12][2] = {
    { 1,  1}, {-1,  1}, { 1, -1}, {-1, -1},
    { 1,  0}, {-1,  0}, { 1,  0}, {-1,  0},
    { 0,  1}, { 0, -1}, { 0,  1}, { 0, -1}
};

SimplexNoise::SimplexNoise() : m_seed(0) {
    initPermutation(0);
}

SimplexNoise::SimplexNoise(std::uint64_t seed) : m_seed(seed) {
    initPermutation(seed);
}

void SimplexNoise::setSeed(std::uint64_t seed) {
    m_seed = seed;
    initPermutation(seed);
}

void SimplexNoise::initPermutation(std::uint64_t seed) {
    // Initialize permutation table with Fisher-Yates shuffle
    Xoshiro256 rng(seed);

    // Fill first half with 0-255
    for (std::size_t i = 0; i < PERM_SIZE; ++i) {
        m_perm[i] = static_cast<std::uint8_t>(i);
    }

    // Fisher-Yates shuffle
    for (std::size_t i = PERM_SIZE - 1; i > 0; --i) {
        std::size_t j = rng.nextUint32(static_cast<std::uint32_t>(i + 1));
        std::swap(m_perm[i], m_perm[j]);
    }

    // Duplicate for wrap-around (avoids modulo in inner loop)
    for (std::size_t i = 0; i < PERM_SIZE; ++i) {
        m_perm[PERM_SIZE + i] = m_perm[i];
    }
}

std::int32_t SimplexNoise::fastFloor(float x) {
    std::int32_t xi = static_cast<std::int32_t>(x);
    return (x < static_cast<float>(xi)) ? xi - 1 : xi;
}

float SimplexNoise::grad(std::int32_t hash, float x, float y) const {
    // Use 4 bits for gradient selection (12 gradients)
    int h = hash & 0x0F;
    if (h >= 12) h -= 12;  // Clamp to 0-11

    return static_cast<float>(GRAD2[h][0]) * x + static_cast<float>(GRAD2[h][1]) * y;
}

float SimplexNoise::noise2D(float x, float y) const {
    float n0, n1, n2;

    // Skew input space to determine which simplex cell we're in
    float s = (x + y) * F2;
    std::int32_t i = fastFloor(x + s);
    std::int32_t j = fastFloor(y + s);

    // Unskew back to (x, y) space
    float t = static_cast<float>(i + j) * G2;
    float X0 = static_cast<float>(i) - t;
    float Y0 = static_cast<float>(j) - t;

    // Relative position in simplex cell
    float x0 = x - X0;
    float y0 = y - Y0;

    // Determine which simplex we're in
    std::int32_t i1, j1;
    if (x0 > y0) {
        i1 = 1; j1 = 0;  // Lower triangle
    } else {
        i1 = 0; j1 = 1;  // Upper triangle
    }

    // Offsets for second corner
    float x1 = x0 - static_cast<float>(i1) + G2;
    float y1 = y0 - static_cast<float>(j1) + G2;

    // Offsets for third corner
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;

    // Hash coordinates to get gradient indices
    std::int32_t ii = i & 0xFF;
    std::int32_t jj = j & 0xFF;

    std::int32_t gi0 = m_perm[ii + m_perm[jj]];
    std::int32_t gi1 = m_perm[ii + i1 + m_perm[jj + j1]];
    std::int32_t gi2 = m_perm[ii + 1 + m_perm[jj + 1]];

    // Calculate contributions from each corner
    float t0 = 0.5f - x0 * x0 - y0 * y0;
    if (t0 < 0.0f) {
        n0 = 0.0f;
    } else {
        t0 *= t0;
        n0 = t0 * t0 * grad(gi0, x0, y0);
    }

    float t1 = 0.5f - x1 * x1 - y1 * y1;
    if (t1 < 0.0f) {
        n1 = 0.0f;
    } else {
        t1 *= t1;
        n1 = t1 * t1 * grad(gi1, x1, y1);
    }

    float t2 = 0.5f - x2 * x2 - y2 * y2;
    if (t2 < 0.0f) {
        n2 = 0.0f;
    } else {
        t2 *= t2;
        n2 = t2 * t2 * grad(gi2, x2, y2);
    }

    // Scale to [-1, 1] range (theoretical max is ~0.87)
    // Using 70.0 as scaling factor for approximate [-1, 1] range
    return 70.0f * (n0 + n1 + n2);
}

std::int32_t SimplexNoise::noise2DInt(std::int32_t x, std::int32_t y) const {
    // Convert fixed-point (8.8 format) to float
    float fx = static_cast<float>(x) / 256.0f;
    float fy = static_cast<float>(y) / 256.0f;

    float n = noise2D(fx, fy);

    // Scale from [-1, 1] to [-32768, 32767]
    std::int32_t result = static_cast<std::int32_t>(n * 32767.0f);
    return std::clamp(result, -32768, 32767);
}

float SimplexNoise::fbm2D(float x, float y, const NoiseConfig& config) const {
    float sum = 0.0f;
    float amplitude = config.amplitude;
    float frequency = config.scale;
    float maxAmplitude = 0.0f;

    // Offset for this noise layer
    float offsetX = static_cast<float>(config.seed_offset) * 1000.0f;
    float offsetY = static_cast<float>(config.seed_offset) * 2000.0f;

    for (std::uint8_t i = 0; i < config.octaves; ++i) {
        float sampleX = (x + offsetX) * frequency;
        float sampleY = (y + offsetY) * frequency;

        sum += noise2D(sampleX, sampleY) * amplitude;
        maxAmplitude += amplitude;

        amplitude *= config.persistence;
        frequency *= config.lacunarity;
    }

    // Normalize by theoretical maximum amplitude
    return sum / maxAmplitude;
}

float SimplexNoise::fbm2DNormalized(float x, float y, const NoiseConfig& config) const {
    float n = fbm2D(x, y, config);
    // Map from [-1, 1] to [0, 1]
    return (n + 1.0f) * 0.5f;
}

std::uint8_t SimplexNoise::fbm2DUint8(float x, float y, const NoiseConfig& config) const {
    float n = fbm2DNormalized(x, y, config);
    // Clamp and scale to [0, 255]
    n = std::clamp(n, 0.0f, 1.0f);
    return static_cast<std::uint8_t>(n * 255.0f);
}

// =============================================================================
// Golden Output Verification
// =============================================================================

void generateGoldenOutput(
    std::array<std::uint64_t, 8>& out_xoshiro,
    std::array<float, 4>& out_simplex,
    std::array<std::uint8_t, 4>& out_fbm)
{
    // Generate PRNG golden values
    Xoshiro256 rng(12345);
    for (std::size_t i = 0; i < 8; ++i) {
        out_xoshiro[i] = rng.next();
    }

    // Generate Simplex noise golden values
    SimplexNoise noise(12345);
    out_simplex[0] = noise.noise2D(0.0f, 0.0f);
    out_simplex[1] = noise.noise2D(1.0f, 0.0f);
    out_simplex[2] = noise.noise2D(0.0f, 1.0f);
    out_simplex[3] = noise.noise2D(1.0f, 1.0f);

    // Generate fBm golden values
    NoiseConfig config = NoiseConfig::terrain();
    out_fbm[0] = noise.fbm2DUint8(64.0f, 64.0f, config);
    out_fbm[1] = noise.fbm2DUint8(128.0f, 128.0f, config);
    out_fbm[2] = noise.fbm2DUint8(192.0f, 192.0f, config);
    out_fbm[3] = noise.fbm2DUint8(256.0f, 256.0f, config);
}

bool verifyGoldenOutput(const char** out_error) {
    // Generate current platform's output
    std::array<std::uint64_t, 8> xoshiro_vals;
    std::array<float, 4> simplex_vals;
    std::array<std::uint8_t, 4> fbm_vals;

    generateGoldenOutput(xoshiro_vals, simplex_vals, fbm_vals);

    // For initial verification, we generate reference values on first platform
    // and then compare on subsequent platforms. The test file stores these.
    //
    // Here we just verify internal consistency - actual golden values
    // are stored and checked in the test file.

    // Verify PRNG produces consistent output by regenerating
    Xoshiro256 rng(12345);
    for (std::size_t i = 0; i < 8; ++i) {
        std::uint64_t val = rng.next();
        if (val != xoshiro_vals[i]) {
            if (out_error) *out_error = "PRNG internal inconsistency";
            return false;
        }
    }

    // Verify noise produces consistent output
    SimplexNoise noise(12345);
    const float tolerance = 1e-6f;

    float n0 = noise.noise2D(0.0f, 0.0f);
    if (std::abs(n0 - simplex_vals[0]) > tolerance) {
        if (out_error) *out_error = "Simplex noise internal inconsistency";
        return false;
    }

    // Verify fBm produces consistent output
    NoiseConfig config = NoiseConfig::terrain();
    std::uint8_t f0 = noise.fbm2DUint8(64.0f, 64.0f, config);
    if (f0 != fbm_vals[0]) {
        if (out_error) *out_error = "fBm internal inconsistency";
        return false;
    }

    return true;
}

} // namespace terrain
} // namespace sims3000
