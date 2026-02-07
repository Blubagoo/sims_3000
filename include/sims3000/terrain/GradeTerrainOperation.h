/**
 * @file GradeTerrainOperation.h
 * @brief Grade terrain (leveling) operation implementation
 *
 * GradeTerrainOperation handles multi-tick terrain elevation changes.
 * The operation raises or lowers terrain one elevation level per tick
 * until the target elevation is reached.
 *
 * Usage pattern:
 * 1. Validate with validate_grade_request() - checks bounds, water type, authority
 * 2. Calculate cost with calculate_grade_cost()
 * 3. Create operation entity with create_grade_operation()
 * 4. Each tick, call tick_grade_operations() to progress all active operations
 * 5. Cancel support via cancel_grade_operation()
 *
 * Server-authoritative: All validation happens on the server.
 * Per-tick validation: Each tick re-validates tile state before modification.
 *
 * @see TerrainModificationComponent for operation state tracking
 * @see ITerrainModifier for the interface contract
 */

#ifndef SIMS3000_TERRAIN_GRADETERRAINOPERATION_H
#define SIMS3000_TERRAIN_GRADETERRAINOPERATION_H

#include <sims3000/terrain/TerrainModificationComponent.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainEvents.h>
#include <sims3000/terrain/ChunkDirtyTracker.h>
#include <sims3000/terrain/TerrainTypeInfo.h>
#include <entt/entt.hpp>
#include <cstdint>
#include <vector>
#include <functional>

namespace sims3000 {
namespace terrain {

/**
 * @brief Configurable cost table for terrain grading operations.
 *
 * Allows game designers to tune the base cost per elevation level change.
 */
struct GradeCostConfig {
    /// Base cost per elevation level change (default: 10 credits)
    std::int64_t base_cost_per_level = 10;

    /// Minimum cost for any grading operation (prevents zero-cost exploits)
    std::int64_t minimum_cost = 1;

    /// Maximum elevation difference allowed per operation (0 = unlimited)
    std::uint8_t max_elevation_delta = 0;
};

/**
 * @brief Result of a grade terrain request validation.
 */
enum class GradeValidationResult : std::uint8_t {
    Valid = 0,              ///< Request is valid, can proceed
    OutOfBounds = 1,        ///< Tile coordinates out of bounds
    WaterTile = 2,          ///< Cannot grade water tiles
    TargetOutOfRange = 3,   ///< Target elevation > 31 or < 0
    NoAuthority = 4,        ///< Player doesn't have authority over tile
    AlreadyGrading = 5,     ///< A grading operation is already in progress for this tile
    DeltaTooLarge = 6,      ///< Elevation change exceeds max_elevation_delta
};

/**
 * @brief Event callback type for terrain modification events.
 *
 * Systems can register callbacks to receive TerrainModifiedEvent
 * each tick as grading progresses.
 */
using TerrainEventCallback = std::function<void(const TerrainModifiedEvent&)>;

/**
 * @class GradeTerrainOperation
 * @brief Manages grade terrain (leveling) operations.
 *
 * This class provides the logic for multi-tick terrain elevation changes.
 * It works with the ECS registry to create and manage temporary entities
 * representing in-progress operations.
 *
 * Thread safety: NOT thread-safe. All methods must be called from the
 * main simulation thread.
 */
class GradeTerrainOperation {
public:
    /**
     * @brief Construct with references to terrain data structures.
     *
     * @param grid Reference to the terrain grid for reading/writing elevation.
     * @param dirty_tracker Reference to chunk dirty tracker for marking changes.
     * @param config Optional cost configuration (uses defaults if not provided).
     */
    GradeTerrainOperation(TerrainGrid& grid,
                          ChunkDirtyTracker& dirty_tracker,
                          const GradeCostConfig& config = GradeCostConfig{});

    /**
     * @brief Set the event callback for terrain modification events.
     *
     * The callback is invoked each tick as elevation changes.
     *
     * @param callback Function to call with TerrainModifiedEvent.
     */
    void setEventCallback(TerrainEventCallback callback);

    // =========================================================================
    // Validation and Cost Query
    // =========================================================================

    /**
     * @brief Validate a grade terrain request.
     *
     * Checks all preconditions:
     * - Tile must be within bounds
     * - Tile must not be a water type (DeepVoid, FlowChannel, StillBasin, BlightMires)
     * - Target elevation must be 0-31
     * - Player must have authority (for now, always allowed - future: ownership check)
     *
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     * @param target_elevation Desired elevation (0-31).
     * @param player_id Player requesting the operation.
     * @param registry ECS registry to check for existing operations.
     * @return Validation result indicating success or failure reason.
     */
    GradeValidationResult validate_grade_request(
        std::int32_t x, std::int32_t y,
        std::uint8_t target_elevation,
        PlayerID player_id,
        const entt::registry& registry) const;

    /**
     * @brief Calculate the cost of a grading operation.
     *
     * Cost = base_cost_per_level * abs(current_elevation - target_elevation)
     *
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     * @param target_elevation Desired elevation (0-31).
     * @return Cost in credits, or -1 if operation is not possible.
     */
    std::int64_t calculate_grade_cost(
        std::int32_t x, std::int32_t y,
        std::uint8_t target_elevation) const;

    // =========================================================================
    // Operation Management
    // =========================================================================

    /**
     * @brief Create a grading operation entity.
     *
     * Creates a temporary entity with TerrainModificationComponent to track
     * the in-progress operation. The entity is destroyed when the operation
     * completes or is cancelled.
     *
     * @param registry ECS registry to create the entity in.
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     * @param target_elevation Desired elevation (0-31).
     * @param player_id Player requesting the operation.
     * @return Entity handle to the operation, or entt::null if validation fails.
     *
     * @note Caller is responsible for ensuring validate_grade_request returned Valid
     *       before calling this method.
     */
    entt::entity create_grade_operation(
        entt::registry& registry,
        std::int32_t x, std::int32_t y,
        std::uint8_t target_elevation,
        PlayerID player_id);

    /**
     * @brief Process one tick of all active grading operations.
     *
     * For each active operation:
     * 1. Re-validate tile state (abort if invalid)
     * 2. Change elevation by 1 level toward target
     * 3. Update is_slope flag for affected tile and neighbors
     * 4. Mark chunk dirty
     * 5. Fire TerrainModifiedEvent
     * 6. Decrement ticks_remaining
     * 7. Destroy entity if complete
     *
     * @param registry ECS registry containing operation entities.
     */
    void tick_grade_operations(entt::registry& registry);

    /**
     * @brief Cancel a grading operation.
     *
     * The operation stops immediately, leaving terrain at its current
     * (partially modified) state. The operation entity is destroyed.
     *
     * @param registry ECS registry containing the operation entity.
     * @param entity Entity handle of the operation to cancel.
     * @return true if the operation was found and cancelled.
     */
    bool cancel_grade_operation(entt::registry& registry, entt::entity entity);

    /**
     * @brief Find an existing grading operation for a tile.
     *
     * @param registry ECS registry to search.
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     * @return Entity handle if found, entt::null otherwise.
     */
    entt::entity find_operation_for_tile(
        const entt::registry& registry,
        std::int32_t x, std::int32_t y) const;

    /**
     * @brief Get the cost configuration.
     * @return Current cost configuration.
     */
    const GradeCostConfig& getConfig() const { return config_; }

    /**
     * @brief Set the cost configuration.
     * @param config New cost configuration.
     */
    void setConfig(const GradeCostConfig& config) { config_ = config; }

private:
    /**
     * @brief Check if a terrain type is a water type that cannot be graded.
     * @param type The terrain type to check.
     * @return true if the terrain is water or toxic (cannot be graded).
     */
    bool isWaterType(TerrainType type) const;

    /**
     * @brief Update the is_slope flag for a tile and its neighbors.
     *
     * A tile is considered a slope if it has different elevation than
     * any of its 4-connected neighbors.
     *
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     */
    void updateSlopeFlags(std::int32_t x, std::int32_t y);

    TerrainGrid& grid_;               ///< Reference to terrain grid
    ChunkDirtyTracker& dirty_tracker_; ///< Reference to chunk dirty tracker
    GradeCostConfig config_;          ///< Cost configuration
    TerrainEventCallback event_callback_; ///< Event callback (optional)
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_GRADETERRAINOPERATION_H
