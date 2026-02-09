/**
 * @file SimulationEvents.h
 * @brief Simulation event structs (Ticket E10-005)
 *
 * Data-only event structs for simulation tick lifecycle.
 * Actual dispatching is deferred to when an event bus is implemented.
 * For now, SimulationCore stores the latest events for polling.
 */

#ifndef SIMS3000_SIM_SIMULATIONEVENTS_H
#define SIMS3000_SIM_SIMULATIONEVENTS_H

#include <cstdint>

namespace sims3000 {
namespace sim {

/**
 * @struct TickStartEvent
 * @brief Emitted at the start of each simulation tick.
 */
struct TickStartEvent {
    uint64_t tick_number;  ///< The tick number about to execute
    float delta_time;      ///< Fixed delta for this tick
};

/**
 * @struct TickCompleteEvent
 * @brief Emitted at the end of each simulation tick.
 */
struct TickCompleteEvent {
    uint64_t tick_number;  ///< The tick number that just completed
    float delta_time;      ///< Fixed delta for this tick
};

/**
 * @struct PhaseChangedEvent
 * @brief Emitted when the simulation phase (season equivalent) changes.
 */
struct PhaseChangedEvent {
    uint32_t cycle;       ///< Current cycle when the phase changed
    uint8_t new_phase;    ///< New phase index (0-3)
    uint8_t old_phase;    ///< Previous phase index (0-3)
};

/**
 * @struct CycleChangedEvent
 * @brief Emitted when the simulation cycle (year equivalent) changes.
 */
struct CycleChangedEvent {
    uint32_t new_cycle;   ///< The new cycle number
    uint32_t old_cycle;   ///< The previous cycle number
};

} // namespace sim
} // namespace sims3000

#endif // SIMS3000_SIM_SIMULATIONEVENTS_H
