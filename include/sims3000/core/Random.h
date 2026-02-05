/**
 * @file Random.h
 * @brief Deterministic random number generator.
 *
 * Server-controlled seeded RNG for reproducible simulation.
 * All random values in the game should come from this class.
 */

#ifndef SIMS3000_CORE_RANDOM_H
#define SIMS3000_CORE_RANDOM_H

#include <cstdint>
#include <random>

namespace sims3000 {

/**
 * @class Random
 * @brief Deterministic random number generator using Mersenne Twister.
 *
 * Provides seeded random number generation for deterministic simulation.
 * The server sets the seed at game start; clients use the same seed
 * for prediction if needed.
 */
class Random {
public:
    /**
     * Create RNG with default seed (0).
     */
    Random();

    /**
     * Create RNG with specified seed.
     * @param seed Initial seed value
     */
    explicit Random(std::uint64_t seed);

    /**
     * Set the seed and reset state.
     * @param seed New seed value
     */
    void setSeed(std::uint64_t seed);

    /**
     * Get the current seed.
     */
    std::uint64_t getSeed() const { return m_seed; }

    /**
     * Generate random integer in range [min, max] (inclusive).
     * @param min Minimum value
     * @param max Maximum value
     * @return Random integer in range
     */
    int nextInt(int min, int max);

    /**
     * Generate random integer in range [0, max) (exclusive).
     * @param max Exclusive upper bound
     * @return Random integer in range [0, max)
     */
    int nextInt(int max);

    /**
     * Generate random float in range [0.0, 1.0).
     * @return Random float
     */
    float nextFloat();

    /**
     * Generate random float in range [min, max).
     * @param min Minimum value
     * @param max Maximum value
     * @return Random float in range
     */
    float nextFloat(float min, float max);

    /**
     * Generate random double in range [0.0, 1.0).
     * @return Random double
     */
    double nextDouble();

    /**
     * Generate random boolean with 50% probability.
     * @return Random boolean
     */
    bool nextBool();

    /**
     * Generate random boolean with specified probability of true.
     * @param probability Probability of returning true [0.0, 1.0]
     * @return Random boolean
     */
    bool nextBool(float probability);

    /**
     * Get internal state for serialization.
     * @param state Output array of 624 uint32_t values
     */
    void getState(std::uint32_t* state) const;

    /**
     * Set internal state from serialization.
     * @param state Input array of 624 uint32_t values
     */
    void setState(const std::uint32_t* state);

    /**
     * State size for serialization (Mersenne Twister state).
     */
    static constexpr std::size_t STATE_SIZE = 624;

private:
    std::mt19937 m_engine;
    std::uint64_t m_seed = 0;
};

} // namespace sims3000

#endif // SIMS3000_CORE_RANDOM_H
