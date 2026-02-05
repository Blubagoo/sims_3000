/**
 * @file SimulationClock.h
 * @brief Manages simulation time and tick advancement.
 */

#ifndef SIMS3000_APP_SIMULATIONCLOCK_H
#define SIMS3000_APP_SIMULATIONCLOCK_H

#include "sims3000/core/ISimulationTime.h"

namespace sims3000 {

/**
 * @class SimulationClock
 * @brief Implements ISimulationTime and manages tick advancement.
 *
 * The clock accumulates real time and produces discrete ticks
 * at a fixed 20 Hz rate. The accumulator pattern ensures consistent
 * simulation speed regardless of frame rate.
 */
class SimulationClock : public ISimulationTime {
public:
    SimulationClock();
    ~SimulationClock() override = default;

    // Non-copyable, movable
    SimulationClock(const SimulationClock&) = delete;
    SimulationClock& operator=(const SimulationClock&) = delete;
    SimulationClock(SimulationClock&&) = default;
    SimulationClock& operator=(SimulationClock&&) = default;

    // ISimulationTime implementation
    SimulationTick getCurrentTick() const override;
    float getTickDelta() const override;
    float getInterpolation() const override;
    double getTotalTime() const override;

    /**
     * Accumulate time and return number of ticks to process.
     * Call this once per frame with the frame delta time.
     * @param deltaSeconds Time elapsed since last frame
     * @return Number of simulation ticks to process
     */
    int accumulate(float deltaSeconds);

    /**
     * Advance the clock by one tick.
     * Call this for each tick returned by accumulate().
     */
    void advanceTick();

    /**
     * Reset the clock to initial state.
     */
    void reset();

    /**
     * Pause/unpause the simulation.
     * When paused, accumulate() always returns 0.
     */
    void setPaused(bool paused);

    /**
     * Check if simulation is paused.
     */
    bool isPaused() const;

private:
    SimulationTick m_currentTick = 0;
    float m_accumulator = 0.0f;
    float m_interpolation = 0.0f;
    bool m_paused = false;

    static constexpr float MAX_ACCUMULATOR = 0.25f; // Cap at 5 ticks per frame
};

} // namespace sims3000

#endif // SIMS3000_APP_SIMULATIONCLOCK_H
