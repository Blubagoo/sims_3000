/**
 * @file SimulationCore.cpp
 * @brief Implementation of SimulationCore tick scheduler (Ticket E10-001)
 */

#include "sims3000/sim/SimulationCore.h"

namespace sims3000 {
namespace sim {

void SimulationCore::register_system(ISimulatable* system) {
    if (system) {
        m_systems.push_back(system);
        m_sorted = false;
    }
}

void SimulationCore::unregister_system(ISimulatable* system) {
    auto it = std::find(m_systems.begin(), m_systems.end(), system);
    if (it != m_systems.end()) {
        m_systems.erase(it);
        // No need to re-sort on removal; relative order preserved
    }
}

void SimulationCore::update(float delta_time) {
    // Apply speed multiplier before accumulating (E10-002)
    const float multiplier = get_speed_multiplier();
    m_accumulator += delta_time * multiplier;

    // Sort systems by priority if needed (lower = earlier)
    if (!m_sorted) {
        std::stable_sort(m_systems.begin(), m_systems.end(),
            [](const ISimulatable* a, const ISimulatable* b) {
                return a->getPriority() < b->getPriority();
            });
        m_sorted = true;
    }

    // Fire ticks while we have enough accumulated time
    while (m_accumulator >= SIMULATION_TICK_DELTA) {
        m_accumulator -= SIMULATION_TICK_DELTA;
        m_tick++;

        // Record tick start event (E10-005)
        m_last_tick_start = TickStartEvent{m_tick, SIMULATION_TICK_DELTA};

        for (auto* system : m_systems) {
            system->tick(*this);
        }

        // Record tick complete event (E10-005)
        m_last_tick_complete = TickCompleteEvent{m_tick, SIMULATION_TICK_DELTA};
    }

    // Compute interpolation factor for rendering
    m_interpolation = m_accumulator / SIMULATION_TICK_DELTA;
}

size_t SimulationCore::system_count() const {
    return m_systems.size();
}

SimulationTick SimulationCore::getCurrentTick() const {
    return m_tick;
}

float SimulationCore::getTickDelta() const {
    return SIMULATION_TICK_DELTA;
}

float SimulationCore::getInterpolation() const {
    return m_interpolation;
}

double SimulationCore::getTotalTime() const {
    return static_cast<double>(m_tick) * static_cast<double>(SIMULATION_TICK_DELTA);
}

// =========================================================================
// Speed control (E10-002)
// =========================================================================

void SimulationCore::set_speed(SimulationSpeed speed) {
    m_speed = speed;
}

SimulationSpeed SimulationCore::get_speed() const {
    return m_speed;
}

float SimulationCore::get_speed_multiplier() const {
    return static_cast<float>(static_cast<uint8_t>(m_speed));
}

bool SimulationCore::is_paused() const {
    return m_speed == SimulationSpeed::Paused;
}

// =========================================================================
// Time progression (E10-003)
// =========================================================================

uint32_t SimulationCore::get_current_cycle() const {
    return static_cast<uint32_t>(m_tick / TICKS_PER_CYCLE);
}

uint8_t SimulationCore::get_current_phase() const {
    return static_cast<uint8_t>((m_tick / TICKS_PER_PHASE) % PHASES_PER_CYCLE);
}

// =========================================================================
// Simulation events (E10-005)
// =========================================================================

TickStartEvent SimulationCore::get_last_tick_start() const {
    return m_last_tick_start;
}

TickCompleteEvent SimulationCore::get_last_tick_complete() const {
    return m_last_tick_complete;
}

} // namespace sim
} // namespace sims3000
