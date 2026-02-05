/**
 * @file SimulationClock.cpp
 * @brief SimulationClock implementation.
 */

#include "sims3000/app/SimulationClock.h"
#include <algorithm>

namespace sims3000 {

SimulationClock::SimulationClock() = default;

SimulationTick SimulationClock::getCurrentTick() const {
    return m_currentTick;
}

float SimulationClock::getTickDelta() const {
    return SIMULATION_TICK_DELTA;
}

float SimulationClock::getInterpolation() const {
    return m_interpolation;
}

double SimulationClock::getTotalTime() const {
    return static_cast<double>(m_currentTick) * SIMULATION_TICK_DELTA;
}

int SimulationClock::accumulate(float deltaSeconds) {
    if (m_paused) {
        m_interpolation = 0.0f;
        return 0;
    }

    // Accumulate time, capped to prevent spiral of death
    m_accumulator += deltaSeconds;
    m_accumulator = std::min(m_accumulator, MAX_ACCUMULATOR);

    // Count how many ticks we can process
    int tickCount = 0;
    while (m_accumulator >= SIMULATION_TICK_DELTA) {
        m_accumulator -= SIMULATION_TICK_DELTA;
        tickCount++;
    }

    // Calculate interpolation for rendering
    m_interpolation = m_accumulator / SIMULATION_TICK_DELTA;

    return tickCount;
}

void SimulationClock::advanceTick() {
    m_currentTick++;
}

void SimulationClock::reset() {
    m_currentTick = 0;
    m_accumulator = 0.0f;
    m_interpolation = 0.0f;
    m_paused = false;
}

void SimulationClock::setPaused(bool paused) {
    m_paused = paused;
    if (paused) {
        m_accumulator = 0.0f;
    }
}

bool SimulationClock::isPaused() const {
    return m_paused;
}

} // namespace sims3000
