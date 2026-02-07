/**
 * @file PositionSyncSystem.h
 * @brief System to synchronize PositionComponent to TransformComponent.
 *
 * This system bridges the gap between game logic (grid coordinates) and
 * rendering (world-space floats). It maps:
 *   - grid_x -> world X
 *   - grid_y -> world Z
 *   - elevation -> world Y
 *
 * Ticket: 2-033
 */

#ifndef SIMS3000_ECS_POSITIONSYNCSYSTEM_H
#define SIMS3000_ECS_POSITIONSYNCSYSTEM_H

#include "sims3000/core/ISimulatable.h"

namespace sims3000 {

// Forward declarations
class Registry;

/**
 * @struct PositionSyncConfig
 * @brief Configuration for position-to-transform synchronization.
 *
 * Controls how grid coordinates are mapped to world-space positions.
 */
struct PositionSyncConfig {
    /// World units per grid cell (default: 1.0)
    float grid_unit_size = 1.0f;

    /// World units per elevation level (default: 0.25)
    float elevation_step = 0.25f;

    /// Offset applied to grid X when converting to world X
    float grid_x_offset = 0.0f;

    /// Offset applied to grid Y when converting to world Z
    float grid_y_offset = 0.0f;

    /// Offset applied to elevation when converting to world Y
    float elevation_offset = 0.0f;
};

/**
 * @class PositionSyncSystem
 * @brief Synchronizes PositionComponent (grid) to TransformComponent (world).
 *
 * This system runs each tick and updates the TransformComponent of all entities
 * that have both PositionComponent and TransformComponent. It converts:
 *   - grid_x -> position.x (using grid_unit_size)
 *   - grid_y -> position.z (using grid_unit_size)
 *   - elevation -> position.y (using elevation_step)
 *
 * When a PositionComponent changes, the TransformComponent's dirty flag is set,
 * and the model matrix is recalculated.
 *
 * The system uses EnTT to query entities efficiently. It runs early in the
 * tick order (priority 50) so that other systems have updated transforms.
 *
 * Coordinate mapping rationale (from patterns.yaml grid section):
 *   - X-axis: East (right) - maps from grid_x
 *   - Y-axis: South (down) in 2D, but in 3D this is elevation (up)
 *   - Z-axis: In 3D rendering, this is the forward/depth axis
 *
 * So grid_y (which is "south" in the 2D grid) maps to world Z.
 */
class PositionSyncSystem : public ISimulatable {
public:
    /**
     * @brief Construct a PositionSyncSystem.
     * @param registry Reference to the ECS registry.
     */
    explicit PositionSyncSystem(Registry& registry);

    /**
     * @brief Construct with custom configuration.
     * @param registry Reference to the ECS registry.
     * @param config Configuration for coordinate mapping.
     */
    PositionSyncSystem(Registry& registry, const PositionSyncConfig& config);

    ~PositionSyncSystem() override = default;

    // Non-copyable
    PositionSyncSystem(const PositionSyncSystem&) = delete;
    PositionSyncSystem& operator=(const PositionSyncSystem&) = delete;

    // =========================================================================
    // ISimulatable interface
    // =========================================================================

    /**
     * @brief Called each simulation tick.
     *
     * Iterates all entities with both PositionComponent and TransformComponent,
     * updates the TransformComponent's position from the grid coordinates,
     * and recalculates the model matrix.
     */
    void tick(const ISimulationTime& time) override;

    /**
     * @brief Get system priority.
     *
     * Runs early (priority 50) so other systems have updated transforms.
     * Lower than core simulation systems but before rendering preparation.
     */
    int getPriority() const override { return 50; }

    /**
     * @brief Get system name.
     */
    const char* getName() const override { return "PositionSyncSystem"; }

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * @brief Get the current configuration.
     * @return Const reference to the configuration.
     */
    const PositionSyncConfig& getConfig() const { return m_config; }

    /**
     * @brief Set the grid unit size.
     * @param size World units per grid cell.
     */
    void setGridUnitSize(float size);

    /**
     * @brief Set the elevation step.
     * @param step World units per elevation level.
     */
    void setElevationStep(float step);

    /**
     * @brief Set the full configuration.
     * @param config New configuration.
     */
    void setConfig(const PositionSyncConfig& config);

    // =========================================================================
    // Coordinate Conversion Utilities
    // =========================================================================

    /**
     * @brief Convert grid X to world X.
     * @param grid_x Grid X coordinate.
     * @return World X position.
     */
    float gridXToWorldX(int grid_x) const;

    /**
     * @brief Convert grid Y to world Z.
     * @param grid_y Grid Y coordinate.
     * @return World Z position.
     */
    float gridYToWorldZ(int grid_y) const;

    /**
     * @brief Convert elevation to world Y.
     * @param elevation Elevation level.
     * @return World Y position.
     */
    float elevationToWorldY(int elevation) const;

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * @brief Get the number of entities synced in the last tick.
     * @return Count of entities processed.
     */
    std::size_t getLastSyncCount() const { return m_lastSyncCount; }

private:
    Registry& m_registry;
    PositionSyncConfig m_config;
    std::size_t m_lastSyncCount = 0;
};

} // namespace sims3000

#endif // SIMS3000_ECS_POSITIONSYNCSYSTEM_H
