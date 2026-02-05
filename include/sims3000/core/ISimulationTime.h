/**
 * @file ISimulationTime.h
 * @brief Read-only interface for simulation time access.
 *
 * Provides systems with access to simulation timing information
 * without the ability to modify the clock state.
 */

#ifndef SIMS3000_CORE_ISIMULATIONTIME_H
#define SIMS3000_CORE_ISIMULATIONTIME_H

#include "sims3000/core/types.h"

namespace sims3000 {

/**
 * @interface ISimulationTime
 * @brief Read-only interface for accessing simulation time.
 *
 * Systems use this interface to query the current simulation state
 * without being able to advance the clock. This enforces separation
 * between time management (SimulationClock) and time consumption (systems).
 *
 * The simulation runs at a fixed 20 Hz (50ms per tick). Render frames
 * interpolate between ticks for smooth visuals.
 */
class ISimulationTime {
public:
    virtual ~ISimulationTime() = default;

    /**
     * Get the current simulation tick.
     * Starts at 0 and increments by 1 each simulation step.
     * @return Current tick count
     */
    virtual SimulationTick getCurrentTick() const = 0;

    /**
     * Get the fixed time delta between ticks.
     * Always returns 0.05f (50ms) for the 20 Hz simulation rate.
     * @return Time delta in seconds (always 0.05f)
     */
    virtual float getTickDelta() const = 0;

    /**
     * Get the interpolation factor for rendering.
     * Value between 0.0 and 1.0 representing progress between
     * the previous tick and the next tick.
     *
     * Used by the renderer to smoothly interpolate entity positions
     * between discrete simulation states.
     *
     * @return Interpolation factor (0.0 to 1.0)
     */
    virtual float getInterpolation() const = 0;

    /**
     * Get the total elapsed simulation time in seconds.
     * Equal to getCurrentTick() * getTickDelta().
     * @return Total simulation time in seconds
     */
    virtual double getTotalTime() const = 0;
};

/// Fixed simulation rate: 20 ticks per second
constexpr float SIMULATION_TICK_RATE = 20.0f;

/// Fixed time step: 50ms per tick
constexpr float SIMULATION_TICK_DELTA = 1.0f / SIMULATION_TICK_RATE;

/// Fixed time step in milliseconds
constexpr int SIMULATION_TICK_MS = 50;

} // namespace sims3000

#endif // SIMS3000_CORE_ISIMULATIONTIME_H
