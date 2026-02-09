/**
 * @file SimulationSpeed.h
 * @brief Simulation speed control enum (Ticket E10-002)
 *
 * Defines speed tiers for the simulation: Paused, Normal, Fast, Fastest.
 * Each tier maps to a multiplier applied to delta_time before accumulation.
 */

#ifndef SIMS3000_SIM_SIMULATIONSPEED_H
#define SIMS3000_SIM_SIMULATIONSPEED_H

#include <cstdint>

namespace sims3000 {
namespace sim {

/**
 * @enum SimulationSpeed
 * @brief Speed tiers for simulation time progression.
 *
 * - Paused:  Multiplier 0 (no ticks fire)
 * - Normal:  Multiplier 1 (real-time, 20 Hz)
 * - Fast:    Multiplier 2 (double speed)
 * - Fastest: Multiplier 3 (triple speed)
 */
enum class SimulationSpeed : uint8_t {
    Paused  = 0,
    Normal  = 1,
    Fast    = 2,
    Fastest = 3
};

} // namespace sim
} // namespace sims3000

#endif // SIMS3000_SIM_SIMULATIONSPEED_H
