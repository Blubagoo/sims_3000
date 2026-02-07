/**
 * @file GradeTerrainOperation.cpp
 * @brief Implementation of grade terrain (leveling) operation
 */

#include <sims3000/terrain/GradeTerrainOperation.h>
#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace terrain {

GradeTerrainOperation::GradeTerrainOperation(
    TerrainGrid& grid,
    ChunkDirtyTracker& dirty_tracker,
    const GradeCostConfig& config)
    : grid_(grid)
    , dirty_tracker_(dirty_tracker)
    , config_(config)
    , event_callback_(nullptr)
{
}

void GradeTerrainOperation::setEventCallback(TerrainEventCallback callback) {
    event_callback_ = std::move(callback);
}

bool GradeTerrainOperation::isWaterType(TerrainType type) const {
    // Water types cannot be graded
    switch (type) {
        case TerrainType::DeepVoid:
        case TerrainType::FlowChannel:
        case TerrainType::StillBasin:
        case TerrainType::BlightMires:  // Toxic also cannot be graded
            return true;
        default:
            return false;
    }
}

GradeValidationResult GradeTerrainOperation::validate_grade_request(
    std::int32_t x, std::int32_t y,
    std::uint8_t target_elevation,
    PlayerID player_id,
    const entt::registry& registry) const
{
    // Check bounds
    if (!grid_.in_bounds(x, y)) {
        return GradeValidationResult::OutOfBounds;
    }

    // Check target elevation range
    if (target_elevation > TerrainComponent::MAX_ELEVATION) {
        return GradeValidationResult::TargetOutOfRange;
    }

    // Get tile data
    const auto& tile = grid_.at(x, y);
    TerrainType type = tile.getTerrainType();

    // Check for water types
    if (isWaterType(type)) {
        return GradeValidationResult::WaterTile;
    }

    // Check for existing operation on this tile
    if (find_operation_for_tile(registry, x, y) != entt::null) {
        return GradeValidationResult::AlreadyGrading;
    }

    // Check max elevation delta if configured
    if (config_.max_elevation_delta > 0) {
        std::uint8_t current = tile.getElevation();
        std::uint8_t delta = (current > target_elevation)
            ? (current - target_elevation)
            : (target_elevation - current);

        if (delta > config_.max_elevation_delta) {
            return GradeValidationResult::DeltaTooLarge;
        }
    }

    // Player authority check
    // For now, always allowed. Future: check OwnershipComponent on tile
    (void)player_id;

    return GradeValidationResult::Valid;
}

std::int64_t GradeTerrainOperation::calculate_grade_cost(
    std::int32_t x, std::int32_t y,
    std::uint8_t target_elevation) const
{
    // Bounds check
    if (!grid_.in_bounds(x, y)) {
        return -1;
    }

    // Target range check
    if (target_elevation > TerrainComponent::MAX_ELEVATION) {
        return -1;
    }

    // Get tile data
    const auto& tile = grid_.at(x, y);
    TerrainType type = tile.getTerrainType();

    // Water types cannot be graded
    if (isWaterType(type)) {
        return -1;
    }

    std::uint8_t current = tile.getElevation();

    // Already at target - no cost
    if (current == target_elevation) {
        return 0;
    }

    // Calculate delta
    std::uint8_t delta = (current > target_elevation)
        ? (current - target_elevation)
        : (target_elevation - current);

    // Cost = base_cost * delta
    std::int64_t cost = config_.base_cost_per_level * static_cast<std::int64_t>(delta);

    // Apply minimum cost
    if (cost < config_.minimum_cost) {
        cost = config_.minimum_cost;
    }

    return cost;
}

entt::entity GradeTerrainOperation::create_grade_operation(
    entt::registry& registry,
    std::int32_t x, std::int32_t y,
    std::uint8_t target_elevation,
    PlayerID player_id)
{
    // Re-validate (caller should have validated, but be safe)
    auto validation = validate_grade_request(x, y, target_elevation, player_id, registry);
    if (validation != GradeValidationResult::Valid) {
        return entt::null;
    }

    // Get current elevation
    const auto& tile = grid_.at(x, y);
    std::uint8_t current_elevation = tile.getElevation();

    // Already at target - no operation needed
    if (current_elevation == target_elevation) {
        return entt::null;
    }

    // Calculate ticks required (one tick per elevation level)
    std::uint8_t delta = (current_elevation > target_elevation)
        ? (current_elevation - target_elevation)
        : (target_elevation - current_elevation);

    // Create the operation entity
    entt::entity entity = registry.create();

    // Build the component
    TerrainModificationComponent& comp = registry.emplace<TerrainModificationComponent>(entity);
    comp.tile_x = static_cast<std::int16_t>(x);
    comp.tile_y = static_cast<std::int16_t>(y);
    comp.player_id = player_id;
    comp.operation_type = TerrainOperationType::GradeTerrain;
    comp.cancelled = 0;
    comp.grading.start_elevation = current_elevation;
    comp.grading.target_elevation = target_elevation;
    comp.grading.ticks_remaining = delta;
    comp.total_cost = calculate_grade_cost(x, y, target_elevation);
    comp.cost_paid = 0;

    return entity;
}

void GradeTerrainOperation::tick_grade_operations(entt::registry& registry) {
    // Collect entities to destroy after iteration (avoid modifying during iteration)
    std::vector<entt::entity> to_destroy;

    // Process all grading operations
    auto view = registry.view<TerrainModificationComponent>();

    for (auto entity : view) {
        auto& comp = view.get<TerrainModificationComponent>(entity);

        // Skip non-grading operations
        if (comp.operation_type != TerrainOperationType::GradeTerrain) {
            continue;
        }

        // Check if cancelled
        if (comp.cancelled != 0) {
            to_destroy.push_back(entity);
            continue;
        }

        // Check if complete
        if (comp.grading.ticks_remaining == 0) {
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

        auto& tile = grid_.at(x, y);
        TerrainType type = tile.getTerrainType();

        // Check if tile became water (shouldn't happen normally)
        if (isWaterType(type)) {
            to_destroy.push_back(entity);
            continue;
        }

        // Apply one level of elevation change
        std::uint8_t current = tile.getElevation();
        std::uint8_t target = comp.grading.target_elevation;
        std::uint8_t new_elevation = current;

        if (current < target) {
            new_elevation = current + 1;
        } else if (current > target) {
            new_elevation = current - 1;
        }

        // Set the new elevation
        tile.setElevation(new_elevation);

        // Update slope flags for this tile and neighbors
        updateSlopeFlags(x, y);

        // Mark chunk dirty
        dirty_tracker_.markTileDirty(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));

        // Fire terrain modified event
        if (event_callback_) {
            TerrainModifiedEvent event(
                static_cast<std::int16_t>(x),
                static_cast<std::int16_t>(y),
                ModificationType::Leveled
            );
            event_callback_(event);
        }

        // Decrement ticks remaining
        comp.grading.ticks_remaining--;

        // Update cost paid (proportional to progress)
        // cost_paid = total_cost * (total_ticks - remaining) / total_ticks
        std::uint8_t total_ticks = (comp.grading.start_elevation > comp.grading.target_elevation)
            ? (comp.grading.start_elevation - comp.grading.target_elevation)
            : (comp.grading.target_elevation - comp.grading.start_elevation);

        if (total_ticks > 0) {
            std::uint8_t ticks_done = total_ticks - comp.grading.ticks_remaining;
            comp.cost_paid = static_cast<std::int32_t>(
                (comp.total_cost * ticks_done) / total_ticks
            );
        }

        // Check if complete after this tick
        if (comp.grading.ticks_remaining == 0) {
            to_destroy.push_back(entity);
        }
    }

    // Destroy completed/cancelled operations
    for (auto entity : to_destroy) {
        registry.destroy(entity);
    }
}

bool GradeTerrainOperation::cancel_grade_operation(
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

    if (comp->operation_type != TerrainOperationType::GradeTerrain) {
        return false;
    }

    // Mark as cancelled - will be destroyed on next tick
    comp->cancelled = 1;

    return true;
}

entt::entity GradeTerrainOperation::find_operation_for_tile(
    const entt::registry& registry,
    std::int32_t x, std::int32_t y) const
{
    auto view = registry.view<const TerrainModificationComponent>();

    for (auto entity : view) {
        const auto& comp = view.get<const TerrainModificationComponent>(entity);

        if (comp.operation_type == TerrainOperationType::GradeTerrain &&
            comp.tile_x == static_cast<std::int16_t>(x) &&
            comp.tile_y == static_cast<std::int16_t>(y) &&
            comp.cancelled == 0)
        {
            return entity;
        }
    }

    return entt::null;
}

void GradeTerrainOperation::updateSlopeFlags(std::int32_t x, std::int32_t y) {
    // Update slope flag for this tile and its 4-connected neighbors
    // A tile is a slope if it has different elevation than any neighbor

    auto updateTileSlope = [this](std::int32_t tx, std::int32_t ty) {
        if (!grid_.in_bounds(tx, ty)) {
            return;
        }

        auto& tile = grid_.at(tx, ty);
        std::uint8_t elevation = tile.getElevation();
        bool is_slope = false;

        // Check all 4 neighbors
        const std::int32_t dx[] = {-1, 1, 0, 0};
        const std::int32_t dy[] = {0, 0, -1, 1};

        for (int i = 0; i < 4; ++i) {
            std::int32_t nx = tx + dx[i];
            std::int32_t ny = ty + dy[i];

            if (grid_.in_bounds(nx, ny)) {
                std::uint8_t neighbor_elev = grid_.at(nx, ny).getElevation();
                if (neighbor_elev != elevation) {
                    is_slope = true;
                    break;
                }
            }
        }

        tile.setSlope(is_slope);
    };

    // Update this tile
    updateTileSlope(x, y);

    // Update neighbors (their slope status may have changed)
    updateTileSlope(x - 1, y);
    updateTileSlope(x + 1, y);
    updateTileSlope(x, y - 1);
    updateTileSlope(x, y + 1);
}

} // namespace terrain
} // namespace sims3000
