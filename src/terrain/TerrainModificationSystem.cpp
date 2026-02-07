/**
 * @file TerrainModificationSystem.cpp
 * @brief Implementation of TerrainModificationSystem
 */

#include <sims3000/terrain/TerrainModificationSystem.h>
#include <sims3000/terrain/TerrainTypeInfo.h>
#include <sims3000/terrain/TerrainTypes.h>

namespace sims3000 {
namespace terrain {

TerrainModificationSystem::TerrainModificationSystem(TerrainGrid& grid,
                                                     ChunkDirtyTracker& dirty_tracker)
    : grid_(grid)
    , dirty_tracker_(dirty_tracker)
    , event_callback_(nullptr)
{
}

void TerrainModificationSystem::setEventCallback(TerrainEventCallback callback) {
    event_callback_ = std::move(callback);
}

bool TerrainModificationSystem::clear_terrain(std::int32_t x, std::int32_t y, PlayerID player_id) {
    // Validate bounds
    if (!grid_.in_bounds(x, y)) {
        return false;
    }

    // Check player authority (for now, accept any valid player_id)
    if (!checkPlayerAuthority(player_id)) {
        return false;
    }

    auto& tile = grid_.at(x, y);
    TerrainType type = tile.getTerrainType();

    // Check if terrain type is clearable (BiolumeGrove, PrismaFields, SporeFlats)
    if (!isClearable(type)) {
        return false;
    }

    // Check if already cleared
    if (tile.isCleared()) {
        return false;
    }

    // Perform the clear operation - instant (single tick)
    tile.setCleared(true);

    // Fire event and mark chunk dirty
    fireEvent(x, y, ModificationType::Cleared);

    return true;
}

bool TerrainModificationSystem::level_terrain(std::int32_t x, std::int32_t y,
                                               std::uint8_t /*target_elevation*/,
                                               PlayerID /*player_id*/) {
    // Not implemented in ticket 3-019
    // Validate bounds for consistency
    if (!grid_.in_bounds(x, y)) {
        return false;
    }

    // Level terrain operation is outside scope of this ticket
    return false;
}

std::int64_t TerrainModificationSystem::get_clear_cost(std::int32_t x, std::int32_t y) const {
    // Validate bounds
    if (!grid_.in_bounds(x, y)) {
        return -1;
    }

    const auto& tile = grid_.at(x, y);
    TerrainType type = tile.getTerrainType();

    // Already cleared - no cost
    if (tile.isCleared()) {
        return 0;
    }

    // Check if terrain type is clearable
    if (!isClearable(type)) {
        return -1;
    }

    // Return cost from TerrainTypeInfo
    // Negative cost means revenue (e.g., PrismaFields crystal harvesting)
    const auto& info = getTerrainInfo(type);
    return info.clear_cost;
}

std::int64_t TerrainModificationSystem::get_level_cost(std::int32_t x, std::int32_t y,
                                                        std::uint8_t target_elevation) const {
    // Validate bounds
    if (!grid_.in_bounds(x, y)) {
        return -1;
    }

    // Validate target elevation
    if (target_elevation > TerrainComponent::MAX_ELEVATION) {
        return -1;
    }

    const auto& tile = grid_.at(x, y);
    TerrainType type = tile.getTerrainType();

    // Water types and toxic marshes cannot be leveled
    if (type == TerrainType::DeepVoid ||
        type == TerrainType::FlowChannel ||
        type == TerrainType::StillBasin ||
        type == TerrainType::BlightMires) {
        return -1;
    }

    std::uint8_t current = tile.getElevation();

    // Already at target - no cost
    if (current == target_elevation) {
        return 0;
    }

    // Cost scales with absolute elevation difference
    std::int64_t diff = (current > target_elevation)
        ? (current - target_elevation)
        : (target_elevation - current);

    return LEVEL_BASE_COST * diff;
}

void TerrainModificationSystem::fireEvent(std::int32_t x, std::int32_t y, ModificationType type) {
    // Mark the affected chunk as dirty for rendering rebuild
    dirty_tracker_.markTileDirty(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));

    // Fire the event callback if registered
    if (event_callback_) {
        TerrainModifiedEvent event(
            static_cast<std::int16_t>(x),
            static_cast<std::int16_t>(y),
            type
        );
        event_callback_(event);
    }
}

bool TerrainModificationSystem::checkPlayerAuthority(PlayerID /*player_id*/) const {
    // For now, accept any player_id
    // Future implementation will check tile ownership via OwnershipComponent
    return true;
}

} // namespace terrain
} // namespace sims3000
