/**
 * @file SimulationCore.h
 * @brief Core simulation tick scheduler (Ticket E10-001)
 *
 * Orchestrates tick scheduling for all ISimulatable systems.
 * Uses a fixed-timestep accumulator pattern: delta_time is accumulated
 * each frame, and when >= SIMULATION_TICK_DELTA (50ms), all registered
 * systems are ticked in priority order (lower priority value = earlier).
 *
 * Implements ISimulationTime to provide read-only timing information
 * to systems during their tick() calls.
 *
 * @see ISimulatable
 * @see ISimulationTime
 */

#ifndef SIMS3000_SIM_SIMULATIONCORE_H
#define SIMS3000_SIM_SIMULATIONCORE_H

#include "sims3000/core/ISimulatable.h"
#include "sims3000/core/ISimulationTime.h"
#include "sims3000/sim/SimulationSpeed.h"
#include "sims3000/sim/SimulationEvents.h"

#include <algorithm>
#include <vector>

namespace sims3000 {
namespace sim {

/**
 * @class SimulationCore
 * @brief Orchestrates fixed-timestep simulation ticks.
 *
 * Call update(delta_time) each frame with wall-clock delta.
 * When accumulated time exceeds SIMULATION_TICK_DELTA (50ms),
 * all registered ISimulatable systems are ticked in priority order.
 *
 * Multiple ticks may fire in a single update() call if the frame
 * delta is large (e.g., after a stall), ensuring simulation
 * consistency regardless of frame rate.
 */
class SimulationCore : public ISimulationTime {
public:
    /**
     * @brief Register a system for tick scheduling.
     *
     * Systems are sorted by getPriority() before each tick batch
     * (only re-sorted when the system list changes).
     *
     * @param system Pointer to the system (caller retains ownership).
     */
    void register_system(ISimulatable* system);

    /**
     * @brief Unregister a system from tick scheduling.
     *
     * @param system Pointer to the system to remove.
     */
    void unregister_system(ISimulatable* system);

    /**
     * @brief Accumulate time and tick systems when ready.
     *
     * Should be called once per frame with the wall-clock delta.
     * Fires zero or more simulation ticks depending on accumulated time.
     *
     * @param delta_time Frame delta in seconds.
     */
    void update(float delta_time);

    /**
     * @brief Get the number of registered systems.
     * @return System count.
     */
    size_t system_count() const;

    // =========================================================================
    // Speed control (E10-002)
    // =========================================================================

    /**
     * @brief Set the simulation speed.
     * @param speed New speed tier.
     */
    void set_speed(SimulationSpeed speed);

    /**
     * @brief Get the current simulation speed.
     * @return Current speed tier.
     */
    SimulationSpeed get_speed() const;

    /**
     * @brief Get the speed multiplier for the current speed tier.
     * @return 0.0f (Paused), 1.0f (Normal), 2.0f (Fast), 3.0f (Fastest).
     */
    float get_speed_multiplier() const;

    /**
     * @brief Check if the simulation is paused.
     * @return True if speed is Paused.
     */
    bool is_paused() const;

    // =========================================================================
    // Time progression (E10-003)
    // =========================================================================

    /// Number of ticks per phase (season equivalent)
    static constexpr uint32_t TICKS_PER_PHASE = 500;

    /// Number of phases per cycle (year equivalent)
    static constexpr uint32_t PHASES_PER_CYCLE = 4;

    /// Number of ticks per cycle
    static constexpr uint32_t TICKS_PER_CYCLE = TICKS_PER_PHASE * PHASES_PER_CYCLE;

    /**
     * @brief Get the current cycle (year equivalent).
     * Derived from tick count: tick / TICKS_PER_CYCLE.
     * @return Current cycle number (starts at 0).
     */
    uint32_t get_current_cycle() const;

    /**
     * @brief Get the current phase (season equivalent).
     * Derived from tick count: (tick / TICKS_PER_PHASE) % PHASES_PER_CYCLE.
     * @return Phase index 0-3.
     */
    uint8_t get_current_phase() const;

    // =========================================================================
    // Simulation events (E10-005)
    // =========================================================================

    /**
     * @brief Get the last TickStartEvent.
     * @return Most recent tick start event data.
     */
    TickStartEvent get_last_tick_start() const;

    /**
     * @brief Get the last TickCompleteEvent.
     * @return Most recent tick complete event data.
     */
    TickCompleteEvent get_last_tick_complete() const;

    // =========================================================================
    // ISimulationTime interface
    // =========================================================================

    /**
     * @brief Get the current simulation tick count.
     * @return Current tick (starts at 0, increments each simulation step).
     */
    SimulationTick getCurrentTick() const override;

    /**
     * @brief Get the fixed time delta between ticks.
     * @return Always SIMULATION_TICK_DELTA (0.05f).
     */
    float getTickDelta() const override;

    /**
     * @brief Get the interpolation factor for rendering.
     * @return Value between 0.0 and 1.0.
     */
    float getInterpolation() const override;

    /**
     * @brief Get the total elapsed simulation time.
     * @return Total time in seconds (getCurrentTick() * getTickDelta()).
     */
    double getTotalTime() const override;

private:
    /// Registered systems (sorted by priority before ticking)
    std::vector<ISimulatable*> m_systems;

    /// Whether the system list needs re-sorting
    bool m_sorted = false;

    /// Accumulated time from update() calls (seconds)
    float m_accumulator = 0.0f;

    /// Current simulation tick count
    SimulationTick m_tick = 0;

    /// Interpolation factor between ticks (0.0 to 1.0)
    float m_interpolation = 0.0f;

    /// Current simulation speed (E10-002)
    SimulationSpeed m_speed = SimulationSpeed::Normal;

    /// Latest tick start event (E10-005)
    TickStartEvent m_last_tick_start{0, 0.0f};

    /// Latest tick complete event (E10-005)
    TickCompleteEvent m_last_tick_complete{0, 0.0f};
};

} // namespace sim
} // namespace sims3000

#endif // SIMS3000_SIM_SIMULATIONCORE_H
