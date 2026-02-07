/**
 * @file TerraformOperation.h
 * @brief Terraform operation for converting terrain types (multi-tick)
 *
 * TerraformOperation handles expensive, multi-tick terrain type conversions.
 * This is a late-game operation that allows players to convert hazardous or
 * special terrain types to standard buildable terrain (Substrate).
 *
 * Supported conversions:
 * - BlightMires -> Substrate: Removes contamination source, very high cost, longest duration
 * - EmberCrust -> Substrate: Removes geothermal bonus and build cost modifier, high cost
 *
 * Usage pattern:
 * 1. Validate with validate_terraform_request() - checks terraformability, authority
 * 2. Calculate cost with calculate_terraform_cost()
 * 3. Create operation entity with create_terraform_operation()
 * 4. Each tick, call tick_terraform_operations() to progress all active operations
 * 5. Cancel support via cancel_terraform_operation()
 *
 * Server-authoritative: All validation happens on the server.
 * On completion, contamination source cache is invalidated for BlightMires removal.
 *
 * @see TerrainModificationComponent for operation state tracking
 * @see ITerrainModifier for the interface contract
 * @see ContaminationSourceQuery for cache invalidation
 */

#ifndef SIMS3000_TERRAIN_TERRAFORMOPERATION_H
#define SIMS3000_TERRAIN_TERRAFORMOPERATION_H

#include <sims3000/terrain/TerrainModificationComponent.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainEvents.h>
#include <sims3000/terrain/ChunkDirtyTracker.h>
#include <sims3000/terrain/TerrainTypeInfo.h>
#include <sims3000/terrain/ContaminationSourceQuery.h>
#include <sims3000/core/types.h>
#include <entt/entt.hpp>
#include <cstdint>
#include <vector>
#include <functional>

namespace sims3000 {
namespace terrain {

/**
 * @brief Configurable cost and duration table for terraform operations.
 *
 * Allows game designers to tune costs and durations per source terrain type.
 */
struct TerraformCostConfig {
    /// Cost to terraform BlightMires to Substrate (very high - late game)
    std::int64_t blight_mires_cost = 10000;

    /// Ticks required to terraform BlightMires (longest duration)
    std::uint16_t blight_mires_ticks = 100;

    /// Cost to terraform EmberCrust to Substrate (high cost)
    std::int64_t ember_crust_cost = 5000;

    /// Ticks required to terraform EmberCrust
    std::uint16_t ember_crust_ticks = 50;

    /// Refund percentage on cancel (0-100)
    std::uint8_t cancel_refund_percent = 50;
};

/**
 * @brief Result of a terraform terrain request validation.
 */
enum class TerraformValidationResult : std::uint8_t {
    Valid = 0,              ///< Request is valid, can proceed
    OutOfBounds = 1,        ///< Tile coordinates out of bounds
    NotTerraformable = 2,   ///< Terrain type cannot be terraformed
    NoAuthority = 3,        ///< Player doesn't have authority over tile
    AlreadyTerraforming = 4,///< A terraform operation is already in progress for this tile
    AlreadySubstrate = 5,   ///< Tile is already Substrate type
    InsufficientFunds = 6,  ///< Player cannot afford the terraform cost
};

/**
 * @brief Event callback type for terrain modification events.
 */
using TerrainEventCallback = std::function<void(const TerrainModifiedEvent&)>;

/**
 * @brief Callback type for contamination cache invalidation.
 *
 * Called when a BlightMires tile is converted, so the contamination
 * source query can be invalidated.
 */
using ContaminationCacheInvalidator = std::function<void()>;

/**
 * @brief Callback type for checking player authority over a tile.
 *
 * @param x X coordinate of the tile.
 * @param y Y coordinate of the tile.
 * @param player_id Player to check authority for.
 * @return true if player has authority to terraform this tile.
 */
using AuthorityChecker = std::function<bool(std::int32_t, std::int32_t, PlayerID)>;

/**
 * @brief Callback type for querying player credits.
 *
 * @param player_id Player to query.
 * @return Current credit balance.
 */
using CreditsQuery = std::function<Credits(PlayerID)>;

/**
 * @class TerraformOperation
 * @brief Manages terraform terrain type conversion operations.
 *
 * This class provides the logic for multi-tick terrain type conversions.
 * It works with the ECS registry to create and manage temporary entities
 * representing in-progress operations.
 *
 * Thread safety: NOT thread-safe. All methods must be called from the
 * main simulation thread.
 */
class TerraformOperation {
public:
    /**
     * @brief Construct with references to terrain data structures.
     *
     * @param grid Reference to the terrain grid for reading/writing terrain type.
     * @param dirty_tracker Reference to chunk dirty tracker for marking changes.
     * @param config Optional cost/duration configuration.
     */
    TerraformOperation(TerrainGrid& grid,
                       ChunkDirtyTracker& dirty_tracker,
                       const TerraformCostConfig& config = TerraformCostConfig{});

    /**
     * @brief Set the callback for terrain modification events.
     *
     * The callback is invoked on completion when terrain type changes.
     *
     * @param callback Function to call with TerrainModifiedEvent.
     */
    void setEventCallback(TerrainEventCallback callback);

    /**
     * @brief Set the callback for contamination cache invalidation.
     *
     * Called when BlightMires is removed to invalidate the cache.
     *
     * @param invalidator Function to call for cache invalidation.
     */
    void setContaminationCacheInvalidator(ContaminationCacheInvalidator invalidator);

    /**
     * @brief Set the callback for checking player authority over tiles.
     *
     * If not set, authority is always granted (test mode).
     *
     * @param checker Function to call for authority checks.
     */
    void setAuthorityChecker(AuthorityChecker checker);

    /**
     * @brief Set the callback for querying player credits.
     *
     * If not set, player funds are not validated (test mode).
     *
     * @param query Function to call for credits query.
     */
    void setCreditsQuery(CreditsQuery query);

    // =========================================================================
    // Validation and Cost Query
    // =========================================================================

    /**
     * @brief Check if a terrain type is terraformable.
     *
     * Currently terraformable types:
     * - BlightMires: Toxic marshes, removes contamination source
     * - EmberCrust: Volcanic rock, removes geothermal bonus and build cost modifier
     *
     * @param type The terrain type to check.
     * @return true if the terrain type can be terraformed.
     */
    static bool isTerraformable(TerrainType type);

    /**
     * @brief Validate a terraform terrain request.
     *
     * Checks all preconditions:
     * - Tile must be within bounds
     * - Terrain type must be terraformable (BlightMires or EmberCrust)
     * - Tile is not already Substrate
     * - No existing terraform operation on this tile
     * - Player must have authority
     *
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     * @param target_type Target terrain type (currently only Substrate supported).
     * @param player_id Player requesting the operation.
     * @param registry ECS registry to check for existing operations.
     * @return Validation result indicating success or failure reason.
     */
    TerraformValidationResult validate_terraform_request(
        std::int32_t x, std::int32_t y,
        TerrainType target_type,
        PlayerID player_id,
        const entt::registry& registry) const;

    /**
     * @brief Calculate the cost of a terraform operation.
     *
     * Cost depends on the source terrain type:
     * - BlightMires: Very high cost (late-game operation)
     * - EmberCrust: High cost
     *
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     * @param target_type Target terrain type (currently only Substrate).
     * @return Cost in credits, or -1 if operation is not possible.
     */
    std::int64_t calculate_terraform_cost(
        std::int32_t x, std::int32_t y,
        TerrainType target_type) const;

    /**
     * @brief Get the duration in ticks for a terraform operation.
     *
     * Duration depends on the source terrain type:
     * - BlightMires: Longest duration
     * - EmberCrust: Shorter duration
     *
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     * @param target_type Target terrain type.
     * @return Duration in ticks, or 0 if operation is not possible.
     */
    std::uint16_t calculate_terraform_duration(
        std::int32_t x, std::int32_t y,
        TerrainType target_type) const;

    // =========================================================================
    // Operation Management
    // =========================================================================

    /**
     * @brief Create a terraform operation entity.
     *
     * Creates a temporary entity with TerrainModificationComponent to track
     * the in-progress operation. The entity is destroyed when the operation
     * completes or is cancelled.
     *
     * @param registry ECS registry to create the entity in.
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     * @param target_type Target terrain type to convert to.
     * @param player_id Player requesting the operation.
     * @return Entity handle to the operation, or entt::null if validation fails.
     *
     * @note Caller is responsible for ensuring validate_terraform_request returned Valid
     *       before calling this method.
     */
    entt::entity create_terraform_operation(
        entt::registry& registry,
        std::int32_t x, std::int32_t y,
        TerrainType target_type,
        PlayerID player_id);

    /**
     * @brief Process one tick of all active terraform operations.
     *
     * For each active operation:
     * 1. Check if cancelled - destroy if so
     * 2. Decrement ticks_remaining
     * 3. If complete:
     *    - Change terrain type to target
     *    - Clear terrain flags (is_cleared = false, keeping other flags)
     *    - Invalidate contamination cache if BlightMires removed
     *    - Mark chunk dirty
     *    - Fire TerrainModifiedEvent with Terraformed type
     *    - Destroy entity
     *
     * @param registry ECS registry containing operation entities.
     */
    void tick_terraform_operations(entt::registry& registry);

    /**
     * @brief Cancel a terraform operation.
     *
     * The operation stops immediately. Partial refund may be available
     * based on cancel_refund_percent in config.
     *
     * @param registry ECS registry containing the operation entity.
     * @param entity Entity handle of the operation to cancel.
     * @return true if the operation was found and cancelled.
     */
    bool cancel_terraform_operation(entt::registry& registry, entt::entity entity);

    /**
     * @brief Calculate partial refund for a cancelled operation.
     *
     * Refund = total_cost * (ticks_remaining / total_ticks) * (cancel_refund_percent / 100)
     *
     * @param registry ECS registry containing the operation entity.
     * @param entity Entity handle of the operation.
     * @return Refund amount in credits, or 0 if invalid.
     */
    std::int64_t calculate_cancel_refund(
        const entt::registry& registry,
        entt::entity entity) const;

    /**
     * @brief Find an existing terraform operation for a tile.
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
     * @return Current cost/duration configuration.
     */
    const TerraformCostConfig& getConfig() const { return config_; }

    /**
     * @brief Set the cost configuration.
     * @param config New cost/duration configuration.
     */
    void setConfig(const TerraformCostConfig& config) { config_ = config; }

private:
    TerrainGrid& grid_;                          ///< Reference to terrain grid
    ChunkDirtyTracker& dirty_tracker_;           ///< Reference to chunk dirty tracker
    TerraformCostConfig config_;                 ///< Cost/duration configuration
    TerrainEventCallback event_callback_;        ///< Event callback (optional)
    ContaminationCacheInvalidator cache_invalidator_; ///< Cache invalidation callback
    AuthorityChecker authority_checker_;         ///< Authority check callback (optional)
    CreditsQuery credits_query_;                 ///< Credits query callback (optional)
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAFORMOPERATION_H
