/**
 * @file TransformInterpolationSystem.h
 * @brief System to interpolate TransformComponent between simulation ticks.
 *
 * Provides smooth 60fps rendering from a 20Hz simulation by interpolating
 * transform values. Position uses lerp, rotation uses slerp.
 *
 * This system is NOT an ISimulatable - it runs during rendering, not simulation.
 * It reads the interpolation factor from ISimulationTime::getInterpolation()
 * and updates TransformComponent for smooth rendering.
 *
 * Ticket: 2-044
 */

#ifndef SIMS3000_ECS_TRANSFORMINTERPOLATIONSYSTEM_H
#define SIMS3000_ECS_TRANSFORMINTERPOLATIONSYSTEM_H

#include "sims3000/core/ISimulationTime.h"
#include <cstddef>

namespace sims3000 {

// Forward declarations
class Registry;

/**
 * @class TransformInterpolationSystem
 * @brief Interpolates transforms between simulation ticks for smooth rendering.
 *
 * This system manages the interpolation of entity transforms for smooth visual
 * rendering at framerates higher than the simulation tick rate (20Hz).
 *
 * Two-phase operation:
 * 1. preSimulationTick() - Called BEFORE each simulation tick
 *    - Rotates interpolation buffers (current -> previous)
 *
 * 2. interpolate() - Called each RENDER frame
 *    - Calculates interpolated transforms based on alpha from SimulationClock
 *    - Updates TransformComponent for rendering
 *
 * Moving entities (those with InterpolatedTransformComponent) are interpolated.
 * Static entities (those with StaticEntityTag) use their current transform directly.
 *
 * Coordinate mapping:
 * - Position: Linear interpolation (lerp) between previous and current
 * - Rotation: Spherical linear interpolation (slerp) between quaternions
 * - Scale: NOT interpolated (taken from TransformComponent directly)
 *
 * @note This system does NOT implement ISimulatable because it runs during
 *       rendering, not during simulation ticks.
 */
class TransformInterpolationSystem {
public:
    /**
     * @brief Construct a TransformInterpolationSystem.
     * @param registry Reference to the ECS registry.
     */
    explicit TransformInterpolationSystem(Registry& registry);

    ~TransformInterpolationSystem() = default;

    // Non-copyable
    TransformInterpolationSystem(const TransformInterpolationSystem&) = delete;
    TransformInterpolationSystem& operator=(const TransformInterpolationSystem&) = delete;

    // =========================================================================
    // Pre-Simulation Tick Phase
    // =========================================================================

    /**
     * @brief Called BEFORE each simulation tick to rotate interpolation buffers.
     *
     * This method should be called at the START of each simulation tick,
     * BEFORE any systems update entity transforms. It copies current -> previous
     * for all entities with InterpolatedTransformComponent.
     *
     * Call order:
     * 1. preSimulationTick()    <- This method
     * 2. Simulation systems run (update current transforms)
     * 3. interpolate() called each render frame
     */
    void preSimulationTick();

    // =========================================================================
    // Render-Time Interpolation Phase
    // =========================================================================

    /**
     * @brief Interpolate transforms for smooth rendering.
     *
     * Called each render frame to update TransformComponent with interpolated
     * values. The interpolation factor (alpha) is obtained from ISimulationTime.
     *
     * - Alpha = 0.0: Use previous tick values
     * - Alpha = 1.0: Use current tick values
     * - Alpha = 0.5: Halfway between previous and current
     *
     * @param time Read-only access to simulation time (for getInterpolation())
     */
    void interpolate(const ISimulationTime& time);

    // =========================================================================
    // Snapshot Management
    // =========================================================================

    /**
     * @brief Snapshot current TransformComponent state to InterpolatedTransformComponent.
     *
     * Call this AFTER simulation systems have updated TransformComponent but
     * BEFORE preSimulationTick() is called. This captures the current frame's
     * final transform values into the interpolation buffer.
     *
     * Typical tick flow:
     * 1. preSimulationTick() - rotate buffers
     * 2. Simulation systems run
     * 3. captureCurrentState() - snapshot new transforms
     */
    void captureCurrentState();

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * @brief Get the number of moving entities interpolated in the last call.
     * @return Count of entities with InterpolatedTransformComponent processed
     */
    std::size_t getLastInterpolatedCount() const { return m_lastInterpolatedCount; }

    /**
     * @brief Get the number of static entities skipped in the last call.
     * @return Count of entities with StaticEntityTag skipped
     */
    std::size_t getLastStaticCount() const { return m_lastStaticCount; }

    /**
     * @brief Get system name (for debugging/logging).
     */
    const char* getName() const { return "TransformInterpolationSystem"; }

private:
    Registry& m_registry;
    std::size_t m_lastInterpolatedCount = 0;
    std::size_t m_lastStaticCount = 0;
};

} // namespace sims3000

#endif // SIMS3000_ECS_TRANSFORMINTERPOLATIONSYSTEM_H
