/**
 * @file InterpolatedTransformComponent.h
 * @brief Component for smooth transform interpolation between simulation ticks.
 *
 * Stores previous and current tick transforms to enable smooth 60fps rendering
 * from a 20Hz simulation. Position uses lerp, rotation uses slerp.
 *
 * Ticket: 2-044
 */

#ifndef SIMS3000_ECS_INTERPOLATEDTRANSFORMCOMPONENT_H
#define SIMS3000_ECS_INTERPOLATEDTRANSFORMCOMPONENT_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>

namespace sims3000 {

/**
 * @struct InterpolatedTransformComponent
 * @brief Double-buffered transform state for smooth rendering interpolation.
 *
 * This component stores both the previous and current tick's transform values,
 * enabling the renderer to interpolate between them for smooth visuals at
 * framerates higher than the 20Hz simulation tick rate.
 *
 * Usage:
 * 1. At the start of each simulation tick, call rotateTick() to shift current -> previous
 * 2. During simulation, systems update the current_* fields
 * 3. During rendering, call getInterpolatedPosition/Rotation(alpha) where alpha
 *    is from ISimulationTime::getInterpolation()
 *
 * Note: Scale is NOT interpolated (rarely changes between ticks).
 *
 * @see TransformInterpolationSystem which manages this component
 */
struct InterpolatedTransformComponent {
    // =========================================================================
    // Previous Tick State (read during rendering)
    // =========================================================================

    /// Position at the previous tick
    glm::vec3 previous_position{0.0f, 0.0f, 0.0f};

    /// Rotation at the previous tick (quaternion)
    glm::quat previous_rotation{1.0f, 0.0f, 0.0f, 0.0f};

    // =========================================================================
    // Current Tick State (updated during simulation)
    // =========================================================================

    /// Position at the current tick
    glm::vec3 current_position{0.0f, 0.0f, 0.0f};

    /// Rotation at the current tick (quaternion)
    glm::quat current_rotation{1.0f, 0.0f, 0.0f, 0.0f};

    // =========================================================================
    // Helper Methods
    // =========================================================================

    /**
     * @brief Rotate buffers: current becomes previous.
     *
     * Call this at the START of each simulation tick, BEFORE updating current values.
     * This preserves the previous state for interpolation during the next frame.
     */
    void rotateTick() {
        previous_position = current_position;
        previous_rotation = current_rotation;
    }

    /**
     * @brief Set both previous and current to the same value.
     *
     * Use when teleporting or initializing to avoid interpolation artifacts.
     * After calling this, interpolation will return the same value regardless of alpha.
     *
     * @param position Position for both previous and current
     * @param rotation Rotation for both previous and current
     */
    void setBoth(const glm::vec3& position, const glm::quat& rotation) {
        previous_position = position;
        current_position = position;
        previous_rotation = rotation;
        current_rotation = rotation;
    }

    /**
     * @brief Set current position (used during simulation updates).
     * @param position New current position
     */
    void setPosition(const glm::vec3& position) {
        current_position = position;
    }

    /**
     * @brief Set current rotation (used during simulation updates).
     * @param rotation New current rotation
     */
    void setRotation(const glm::quat& rotation) {
        current_rotation = rotation;
    }
};

// Size check: 2 * vec3 (24) + 2 * quat (32) = 56 bytes
static_assert(sizeof(InterpolatedTransformComponent) == 56,
              "InterpolatedTransformComponent size check");

/**
 * @struct StaticEntityTag
 * @brief Tag component to mark entities that should NOT use interpolation.
 *
 * Buildings and other static entities that don't move between ticks should
 * have this tag. The TransformInterpolationSystem skips these entities,
 * using their current TransformComponent values directly for better performance.
 *
 * Entities WITHOUT this tag (e.g., beings, vehicles) will be interpolated.
 */
struct StaticEntityTag {};

// Tag components should be zero-size or minimal
static_assert(sizeof(StaticEntityTag) == 1, "StaticEntityTag is empty struct (1 byte minimum)");

} // namespace sims3000

#endif // SIMS3000_ECS_INTERPOLATEDTRANSFORMCOMPONENT_H
