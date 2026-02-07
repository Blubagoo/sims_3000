/**
 * @file TerraformOperation.cpp
 * @brief Implementation of terraform terrain type conversion operation
 */

#include <sims3000/terrain/TerraformOperation.h>
#include <algorithm>
#include <vector>

namespace sims3000 {
namespace terrain {

TerraformOperation::TerraformOperation(
    TerrainGrid& grid,
    ChunkDirtyTracker& dirty_tracker,
    const TerraformCostConfig& config)
    : grid_(grid)
    , dirty_tracker_(dirty_tracker)
    , config_(config)
    , event_callback_(nullptr)
    , cache_invalidator_(nullptr)
    , authority_checker_(nullptr)
    , credits_query_(nullptr)
{
}

void TerraformOperation::setEventCallback(TerrainEventCallback callback) {
    event_callback_ = std::move(callback);
}

void TerraformOperation::setContaminationCacheInvalidator(ContaminationCacheInvalidator invalidator) {
    cache_invalidator_ = std::move(invalidator);
}

void TerraformOperation::setAuthorityChecker(AuthorityChecker checker) {
    authority_checker_ = std::move(checker);
}

void TerraformOperation::setCreditsQuery(CreditsQuery query) {
    credits_query_ = std::move(query);
}

bool TerraformOperation::isTerraformable(TerrainType type) {
    switch (type) {
        case TerrainType::BlightMires:
        case TerrainType::EmberCrust:
            return true;
        default:
            return false;
    }
}

TerraformValidationResult TerraformOperation::validate_terraform_request(
    std::int32_t x, std::int32_t y,
    TerrainType target_type,
    PlayerID player_id,
    const entt::registry& registry) const
{
    // Check bounds
    if (!grid_.in_bounds(x, y)) {
        return TerraformValidationResult::OutOfBounds;
    }

    // Get tile data
    const auto& tile = grid_.at(x, y);
    TerrainType source_type = tile.getTerrainType();

    // Check if already the target type
    if (source_type == target_type) {
        return TerraformValidationResult::AlreadySubstrate;
    }

    // Check if terraformable
    if (!isTerraformable(source_type)) {
        return TerraformValidationResult::NotTerraformable;
    }

    // Currently only Substrate is a valid target
    if (target_type != TerrainType::Substrate) {
        return TerraformValidationResult::NotTerraformable;
    }

    // Check for existing operation on this tile
    if (find_operation_for_tile(registry, x, y) != entt::null) {
        return TerraformValidationResult::AlreadyTerraforming;
    }

    // Player authority check (if checker is configured)
    if (authority_checker_) {
        if (!authority_checker_(x, y, player_id)) {
            return TerraformValidationResult::NoAuthority;
        }
    }

    // Player funds check (if credits query is configured)
    if (credits_query_) {
        std::int64_t cost = calculate_terraform_cost(x, y, target_type);
        Credits player_credits = credits_query_(player_id);
        if (player_credits < cost) {
            return TerraformValidationResult::InsufficientFunds;
        }
    }

    return TerraformValidationResult::Valid;
}

std::int64_t TerraformOperation::calculate_terraform_cost(
    std::int32_t x, std::int32_t y,
    TerrainType target_type) const
{
    // Bounds check
    if (!grid_.in_bounds(x, y)) {
        return -1;
    }

    // Get tile data
    const auto& tile = grid_.at(x, y);
    TerrainType source_type = tile.getTerrainType();

    // Already at target - no cost
    if (source_type == target_type) {
        return 0;
    }

    // Not terraformable
    if (!isTerraformable(source_type)) {
        return -1;
    }

    // Only Substrate is a valid target
    if (target_type != TerrainType::Substrate) {
        return -1;
    }

    // Return cost based on source type
    switch (source_type) {
        case TerrainType::BlightMires:
            return config_.blight_mires_cost;
        case TerrainType::EmberCrust:
            return config_.ember_crust_cost;
        default:
            return -1;
    }
}

std::uint16_t TerraformOperation::calculate_terraform_duration(
    std::int32_t x, std::int32_t y,
    TerrainType target_type) const
{
    // Bounds check
    if (!grid_.in_bounds(x, y)) {
        return 0;
    }

    // Get tile data
    const auto& tile = grid_.at(x, y);
    TerrainType source_type = tile.getTerrainType();

    // Already at target - no duration
    if (source_type == target_type) {
        return 0;
    }

    // Not terraformable
    if (!isTerraformable(source_type)) {
        return 0;
    }

    // Only Substrate is a valid target
    if (target_type != TerrainType::Substrate) {
        return 0;
    }

    // Return duration based on source type
    switch (source_type) {
        case TerrainType::BlightMires:
            return config_.blight_mires_ticks;
        case TerrainType::EmberCrust:
            return config_.ember_crust_ticks;
        default:
            return 0;
    }
}

entt::entity TerraformOperation::create_terraform_operation(
    entt::registry& registry,
    std::int32_t x, std::int32_t y,
    TerrainType target_type,
    PlayerID player_id)
{
    // Re-validate (caller should have validated, but be safe)
    auto validation = validate_terraform_request(x, y, target_type, player_id, registry);
    if (validation != TerraformValidationResult::Valid) {
        return entt::null;
    }

    // Get source type
    const auto& tile = grid_.at(x, y);
    TerrainType source_type = tile.getTerrainType();

    // Calculate duration
    std::uint16_t duration = calculate_terraform_duration(x, y, target_type);
    if (duration == 0) {
        return entt::null;
    }

    // Create the operation entity
    entt::entity entity = registry.create();

    // Build the component
    TerrainModificationComponent& comp = registry.emplace<TerrainModificationComponent>(entity);
    comp.tile_x = static_cast<std::int16_t>(x);
    comp.tile_y = static_cast<std::int16_t>(y);
    comp.player_id = player_id;
    comp.operation_type = TerrainOperationType::TerraformTerrain;
    comp.cancelled = 0;
    comp.terraforming.source_type = static_cast<std::uint8_t>(source_type);
    comp.terraforming.target_type = static_cast<std::uint8_t>(target_type);
    comp.terraforming.ticks_remaining = duration;
    comp.terraforming.total_ticks = duration;
    comp.total_cost = calculate_terraform_cost(x, y, target_type);
    comp.cost_paid = static_cast<std::int32_t>(comp.total_cost); // Full cost paid upfront

    return entity;
}

void TerraformOperation::tick_terraform_operations(entt::registry& registry) {
    // Collect entities to destroy after iteration (avoid modifying during iteration)
    std::vector<entt::entity> to_destroy;

    // Process all terraform operations
    auto view = registry.view<TerrainModificationComponent>();

    for (auto entity : view) {
        auto& comp = view.get<TerrainModificationComponent>(entity);

        // Skip non-terraform operations
        if (comp.operation_type != TerrainOperationType::TerraformTerrain) {
            continue;
        }

        // Check if cancelled
        if (comp.cancelled != 0) {
            to_destroy.push_back(entity);
            continue;
        }

        // Check if complete
        if (comp.terraforming.ticks_remaining == 0) {
            to_destroy.push_back(entity);
            continue;
        }

        std::int32_t x = comp.tile_x;
        std::int32_t y = comp.tile_y;

        // Per-tick validation: check tile is still valid
        if (!grid_.in_bounds(x, y)) {
            // Tile went out of bounds somehow - abort
            to_destroy.push_back(entity);
            continue;
        }

        // Decrement ticks remaining
        comp.terraforming.ticks_remaining--;

        // Check if complete after this tick
        if (comp.terraforming.ticks_remaining == 0) {
            // Operation complete - change terrain type
            auto& tile = grid_.at(x, y);
            TerrainType source_type = static_cast<TerrainType>(comp.terraforming.source_type);
            TerrainType target_type = static_cast<TerrainType>(comp.terraforming.target_type);

            // Change the terrain type
            tile.setTerrainType(target_type);

            // Clear the cleared flag since this is now fresh substrate
            tile.setCleared(false);

            // If removing BlightMires, invalidate contamination cache
            if (source_type == TerrainType::BlightMires && cache_invalidator_) {
                cache_invalidator_();
            }

            // Mark chunk dirty
            dirty_tracker_.markTileDirty(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));

            // Fire terrain modified event
            if (event_callback_) {
                TerrainModifiedEvent event(
                    static_cast<std::int16_t>(x),
                    static_cast<std::int16_t>(y),
                    ModificationType::Terraformed
                );
                event_callback_(event);
            }

            to_destroy.push_back(entity);
        }
    }

    // Destroy completed/cancelled operations
    for (auto entity : to_destroy) {
        registry.destroy(entity);
    }
}

bool TerraformOperation::cancel_terraform_operation(
    entt::registry& registry,
    entt::entity entity)
{
    if (!registry.valid(entity)) {
        return false;
    }

    auto* comp = registry.try_get<TerrainModificationComponent>(entity);
    if (!comp) {
        return false;
    }

    if (comp->operation_type != TerrainOperationType::TerraformTerrain) {
        return false;
    }

    // Mark as cancelled - will be destroyed on next tick
    comp->cancelled = 1;

    return true;
}

std::int64_t TerraformOperation::calculate_cancel_refund(
    const entt::registry& registry,
    entt::entity entity) const
{
    if (!registry.valid(entity)) {
        return 0;
    }

    const auto* comp = registry.try_get<TerrainModificationComponent>(entity);
    if (!comp) {
        return 0;
    }

    if (comp->operation_type != TerrainOperationType::TerraformTerrain) {
        return 0;
    }

    // Calculate how much work remains
    if (comp->terraforming.total_ticks == 0) {
        return 0;
    }

    // Refund = total_cost * (ticks_remaining / total_ticks) * (refund_percent / 100)
    std::int64_t remaining_fraction_cost =
        (comp->total_cost * comp->terraforming.ticks_remaining) / comp->terraforming.total_ticks;

    std::int64_t refund =
        (remaining_fraction_cost * config_.cancel_refund_percent) / 100;

    return refund;
}

entt::entity TerraformOperation::find_operation_for_tile(
    const entt::registry& registry,
    std::int32_t x, std::int32_t y) const
{
    auto view = registry.view<const TerrainModificationComponent>();

    for (auto entity : view) {
        const auto& comp = view.get<const TerrainModificationComponent>(entity);

        if (comp.operation_type == TerrainOperationType::TerraformTerrain &&
            comp.tile_x == static_cast<std::int16_t>(x) &&
            comp.tile_y == static_cast<std::int16_t>(y) &&
            comp.cancelled == 0)
        {
            return entity;
        }
    }

    return entt::null;
}

} // namespace terrain
} // namespace sims3000
