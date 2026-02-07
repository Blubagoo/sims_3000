/**
 * @file PositionSyncSystem.cpp
 * @brief Implementation of position-to-transform synchronization.
 *
 * Ticket: 2-033
 */

#include "sims3000/ecs/PositionSyncSystem.h"
#include "sims3000/ecs/Registry.h"
#include "sims3000/ecs/Components.h"

namespace sims3000 {

PositionSyncSystem::PositionSyncSystem(Registry& registry)
    : m_registry(registry)
    , m_config()
{
}

PositionSyncSystem::PositionSyncSystem(Registry& registry, const PositionSyncConfig& config)
    : m_registry(registry)
    , m_config(config)
{
}

void PositionSyncSystem::tick(const ISimulationTime& /*time*/) {
    m_lastSyncCount = 0;

    // Query all entities with both PositionComponent and TransformComponent
    auto view = m_registry.view<PositionComponent, TransformComponent>();

    for (auto entity : view) {
        auto& pos = view.get<PositionComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);

        // Calculate world position from grid coordinates
        float worldX = gridXToWorldX(pos.pos.x);
        float worldY = elevationToWorldY(pos.elevation);
        float worldZ = gridYToWorldZ(pos.pos.y);

        // Check if position has changed (avoids unnecessary matrix recalculation)
        if (transform.position.x != worldX ||
            transform.position.y != worldY ||
            transform.position.z != worldZ) {

            // Update transform position
            transform.position.x = worldX;
            transform.position.y = worldY;
            transform.position.z = worldZ;

            // Mark transform as dirty so model matrix is recalculated
            transform.set_dirty();

            // Recompute the model matrix immediately
            // This ensures the matrix is ready for rendering this frame
            transform.recompute_matrix();
        }

        ++m_lastSyncCount;
    }
}

void PositionSyncSystem::setGridUnitSize(float size) {
    m_config.grid_unit_size = size;
}

void PositionSyncSystem::setElevationStep(float step) {
    m_config.elevation_step = step;
}

void PositionSyncSystem::setConfig(const PositionSyncConfig& config) {
    m_config = config;
}

float PositionSyncSystem::gridXToWorldX(int grid_x) const {
    return static_cast<float>(grid_x) * m_config.grid_unit_size + m_config.grid_x_offset;
}

float PositionSyncSystem::gridYToWorldZ(int grid_y) const {
    return static_cast<float>(grid_y) * m_config.grid_unit_size + m_config.grid_y_offset;
}

float PositionSyncSystem::elevationToWorldY(int elevation) const {
    return static_cast<float>(elevation) * m_config.elevation_step + m_config.elevation_offset;
}

} // namespace sims3000
