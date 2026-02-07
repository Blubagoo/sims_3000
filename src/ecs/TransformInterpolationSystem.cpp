/**
 * @file TransformInterpolationSystem.cpp
 * @brief Implementation of transform interpolation for smooth rendering.
 *
 * Ticket: 2-044
 */

#include "sims3000/ecs/TransformInterpolationSystem.h"
#include "sims3000/ecs/Registry.h"
#include "sims3000/ecs/Components.h"
#include "sims3000/ecs/InterpolatedTransformComponent.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext/quaternion_common.hpp>

namespace sims3000 {

TransformInterpolationSystem::TransformInterpolationSystem(Registry& registry)
    : m_registry(registry)
{
}

void TransformInterpolationSystem::preSimulationTick() {
    // Rotate buffers for all entities with InterpolatedTransformComponent
    // This copies current -> previous to preserve state for interpolation
    auto view = m_registry.view<InterpolatedTransformComponent>();

    for (auto entity : view) {
        auto& interp = view.get<InterpolatedTransformComponent>(entity);
        interp.rotateTick();
    }
}

void TransformInterpolationSystem::captureCurrentState() {
    // Capture current TransformComponent state into InterpolatedTransformComponent
    // This should be called AFTER simulation systems have updated TransformComponent
    auto view = m_registry.view<TransformComponent, InterpolatedTransformComponent>();

    for (auto entity : view) {
        const auto& transform = view.get<TransformComponent>(entity);
        auto& interp = view.get<InterpolatedTransformComponent>(entity);

        // Copy current transform values to interpolation buffer
        interp.current_position = transform.position;
        interp.current_rotation = transform.rotation;
    }
}

void TransformInterpolationSystem::interpolate(const ISimulationTime& time) {
    m_lastInterpolatedCount = 0;
    m_lastStaticCount = 0;

    // Get interpolation factor from simulation clock
    // 0.0 = at previous tick, 1.0 = at current tick
    const float alpha = time.getInterpolation();

    // Process moving entities (have InterpolatedTransformComponent, no StaticEntityTag)
    // These entities get smooth interpolation between ticks
    // Use raw() to access EnTT's exclude feature
    {
        auto& rawRegistry = m_registry.raw();
        auto view = rawRegistry.view<TransformComponent, InterpolatedTransformComponent>(
            entt::exclude<StaticEntityTag>
        );

        for (auto entity : view) {
            auto& transform = view.get<TransformComponent>(entity);
            const auto& interp = view.get<InterpolatedTransformComponent>(entity);

            // Interpolate position using lerp (linear interpolation)
            transform.position = glm::mix(interp.previous_position, interp.current_position, alpha);

            // Interpolate rotation using slerp (spherical linear interpolation)
            // slerp ensures smooth rotation along the shortest arc on the quaternion sphere
            transform.rotation = glm::slerp(interp.previous_rotation, interp.current_rotation, alpha);

            // Scale is NOT interpolated - taken directly from TransformComponent
            // (scale rarely changes between ticks, and interpolating it could cause visual artifacts)

            // Mark transform as dirty so model matrix is recalculated
            transform.set_dirty();

            // Recompute the model matrix immediately for rendering
            transform.recompute_matrix();

            ++m_lastInterpolatedCount;
        }
    }

    // Count static entities (have StaticEntityTag)
    // These entities use their current transform directly - no interpolation needed
    {
        auto view = m_registry.view<TransformComponent, StaticEntityTag>();
        m_lastStaticCount = static_cast<std::size_t>(std::distance(view.begin(), view.end()));
    }
}

} // namespace sims3000
