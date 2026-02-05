/**
 * @file Random.cpp
 * @brief Deterministic RNG implementation.
 */

#include "sims3000/core/Random.h"
#include <algorithm>
#include <sstream>

namespace sims3000 {

Random::Random() : m_engine(0), m_seed(0) {}

Random::Random(std::uint64_t seed) : m_engine(static_cast<std::uint32_t>(seed)), m_seed(seed) {}

void Random::setSeed(std::uint64_t seed) {
    m_seed = seed;
    m_engine.seed(static_cast<std::uint32_t>(seed));
}

int Random::nextInt(int min, int max) {
    if (min >= max) return min;
    std::uniform_int_distribution<int> dist(min, max);
    return dist(m_engine);
}

int Random::nextInt(int max) {
    if (max <= 0) return 0;
    std::uniform_int_distribution<int> dist(0, max - 1);
    return dist(m_engine);
}

float Random::nextFloat() {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(m_engine);
}

float Random::nextFloat(float min, float max) {
    if (min >= max) return min;
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_engine);
}

double Random::nextDouble() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(m_engine);
}

bool Random::nextBool() {
    return nextInt(2) == 1;
}

bool Random::nextBool(float probability) {
    return nextFloat() < probability;
}

void Random::getState(std::uint32_t* state) const {
    // Extract internal state from mt19937
    std::stringstream ss;
    ss << m_engine;

    // Parse the state (mt19937 outputs 624 32-bit integers + index)
    for (std::size_t i = 0; i < STATE_SIZE; ++i) {
        ss >> state[i];
    }
}

void Random::setState(const std::uint32_t* state) {
    // Reconstruct engine state
    std::stringstream ss;
    for (std::size_t i = 0; i < STATE_SIZE; ++i) {
        if (i > 0) ss << ' ';
        ss << state[i];
    }
    ss << ' ' << 624; // Index at end of state
    ss >> m_engine;
}

} // namespace sims3000
