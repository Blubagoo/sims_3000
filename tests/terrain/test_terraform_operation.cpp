/**
 * @file test_terraform_operation.cpp
 * @brief Unit tests for terraform terrain type conversion operation (Ticket 3-021)
 *
 * Tests cover:
 * - Validation of terraform requests (terraformable types, authority, bounds)
 * - Cost calculation per source terrain type
 * - Duration calculation per source terrain type (BlightMires longest)
 * - Multi-tick operation (countdown to completion)
 * - TerrainModifiedEvent firing on completion with Terraformed type
 * - Contamination source cache invalidation when BlightMires removed
 * - Chunk dirty marking
 * - Cancel support with partial refund calculation
 * - Rejection of non-terraformable types (water, vegetation, etc.)
 * - BlightMires -> Substrate conversion
 * - EmberCrust -> Substrate conversion
 */

#include <sims3000/terrain/TerraformOperation.h>
#include <sims3000/terrain/GradeTerrainOperation.h>
#include <sims3000/terrain/TerrainModificationComponent.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/ChunkDirtyTracker.h>
#include <sims3000/terrain/TerrainEvents.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace sims3000::terrain;

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n  FAILED: %s (line %d)\n", #condition, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// TerraformingOperation Struct Tests
// =============================================================================

TEST(terraforming_operation_size_is_8_bytes) {
    ASSERT_EQ(sizeof(TerraformingOperation), 8);
}

TEST(terraforming_operation_is_trivially_copyable) {
    ASSERT(std::is_trivially_copyable<TerraformingOperation>::value);
}

TEST(component_size_is_32_bytes) {
    ASSERT_EQ(sizeof(TerrainModificationComponent), 32);
}

TEST(component_is_trivially_copyable) {
    ASSERT(std::is_trivially_copyable<TerrainModificationComponent>::value);
}

TEST(component_is_terraforming) {
    TerrainModificationComponent comp;
    ASSERT(!comp.isTerraforming());

    comp.operation_type = TerrainOperationType::TerraformTerrain;
    ASSERT(comp.isTerraforming());
}

TEST(component_is_complete_terraform) {
    TerrainModificationComponent comp;
    comp.operation_type = TerrainOperationType::TerraformTerrain;
    comp.terraforming.ticks_remaining = 50;

    ASSERT(!comp.isComplete());

    comp.terraforming.ticks_remaining = 0;
    ASSERT(comp.isComplete());
}

TEST(component_terraform_progress) {
    TerrainModificationComponent comp;
    comp.operation_type = TerrainOperationType::TerraformTerrain;
    comp.terraforming.total_ticks = 100;
    comp.terraforming.ticks_remaining = 100;

    ASSERT_EQ(comp.getTerraformProgress(), 0);

    comp.terraforming.ticks_remaining = 50;
    ASSERT_EQ(comp.getTerraformProgress(), 50);

    comp.terraforming.ticks_remaining = 0;
    ASSERT_EQ(comp.getTerraformProgress(), 100);
}

// =============================================================================
// isTerraformable Tests
// =============================================================================

TEST(is_terraformable_blight_mires) {
    ASSERT(TerraformOperation::isTerraformable(TerrainType::BlightMires));
}

TEST(is_terraformable_ember_crust) {
    ASSERT(TerraformOperation::isTerraformable(TerrainType::EmberCrust));
}

TEST(is_not_terraformable_substrate) {
    ASSERT(!TerraformOperation::isTerraformable(TerrainType::Substrate));
}

TEST(is_not_terraformable_ridge) {
    ASSERT(!TerraformOperation::isTerraformable(TerrainType::Ridge));
}

TEST(is_not_terraformable_water_types) {
    ASSERT(!TerraformOperation::isTerraformable(TerrainType::DeepVoid));
    ASSERT(!TerraformOperation::isTerraformable(TerrainType::FlowChannel));
    ASSERT(!TerraformOperation::isTerraformable(TerrainType::StillBasin));
}

TEST(is_not_terraformable_vegetation) {
    ASSERT(!TerraformOperation::isTerraformable(TerrainType::BiolumeGrove));
    ASSERT(!TerraformOperation::isTerraformable(TerrainType::PrismaFields));
    ASSERT(!TerraformOperation::isTerraformable(TerrainType::SporeFlats));
}

// =============================================================================
// Validation Tests
// =============================================================================

TEST(validation_valid_blight_mires) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);
    entt::registry registry;

    auto result = op.validate_terraform_request(64, 64, TerrainType::Substrate, 1, registry);
    ASSERT_EQ(result, TerraformValidationResult::Valid);
}

TEST(validation_valid_ember_crust) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::EmberCrust);

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);
    entt::registry registry;

    auto result = op.validate_terraform_request(64, 64, TerrainType::Substrate, 1, registry);
    ASSERT_EQ(result, TerraformValidationResult::Valid);
}

TEST(validation_out_of_bounds) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);
    entt::registry registry;

    // Negative coordinates
    auto result = op.validate_terraform_request(-1, 64, TerrainType::Substrate, 1, registry);
    ASSERT_EQ(result, TerraformValidationResult::OutOfBounds);

    // Beyond map size
    result = op.validate_terraform_request(200, 64, TerrainType::Substrate, 1, registry);
    ASSERT_EQ(result, TerraformValidationResult::OutOfBounds);
}

TEST(validation_not_terraformable) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);
    entt::registry registry;

    // Substrate (already the target)
    grid.at(10, 10).setTerrainType(TerrainType::Substrate);
    ASSERT_EQ(op.validate_terraform_request(10, 10, TerrainType::Substrate, 1, registry),
              TerraformValidationResult::AlreadySubstrate);

    // DeepVoid (water)
    grid.at(11, 11).setTerrainType(TerrainType::DeepVoid);
    ASSERT_EQ(op.validate_terraform_request(11, 11, TerrainType::Substrate, 1, registry),
              TerraformValidationResult::NotTerraformable);

    // BiolumeGrove (vegetation - must be cleared, not terraformed)
    grid.at(12, 12).setTerrainType(TerrainType::BiolumeGrove);
    ASSERT_EQ(op.validate_terraform_request(12, 12, TerrainType::Substrate, 1, registry),
              TerraformValidationResult::NotTerraformable);
}

TEST(validation_already_terraforming) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);
    entt::registry registry;

    // Create first operation
    auto entity1 = op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);
    ASSERT(entity1 != entt::null);

    // Second operation on same tile should fail
    auto result = op.validate_terraform_request(64, 64, TerrainType::Substrate, 1, registry);
    ASSERT_EQ(result, TerraformValidationResult::AlreadyTerraforming);

    // But different tile should be fine
    grid.at(65, 65).setTerrainType(TerrainType::BlightMires);
    result = op.validate_terraform_request(65, 65, TerrainType::Substrate, 1, registry);
    ASSERT_EQ(result, TerraformValidationResult::Valid);
}

TEST(validation_only_substrate_target) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);
    entt::registry registry;

    // Trying to terraform to Ridge (not allowed)
    auto result = op.validate_terraform_request(64, 64, TerrainType::Ridge, 1, registry);
    ASSERT_EQ(result, TerraformValidationResult::NotTerraformable);
}

TEST(validation_no_authority_with_checker) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);
    entt::registry registry;

    // Set up authority checker that denies player 1 access
    op.setAuthorityChecker([](std::int32_t x, std::int32_t y, sims3000::PlayerID player_id) {
        (void)x; (void)y;
        return player_id != 1; // Only deny player 1
    });

    // Player 1 should be denied
    auto result = op.validate_terraform_request(64, 64, TerrainType::Substrate, 1, registry);
    ASSERT_EQ(result, TerraformValidationResult::NoAuthority);

    // Player 2 should be allowed
    result = op.validate_terraform_request(64, 64, TerrainType::Substrate, 2, registry);
    ASSERT_EQ(result, TerraformValidationResult::Valid);
}

TEST(validation_authority_granted_when_checker_allows) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);
    entt::registry registry;

    // Set up authority checker that allows everyone
    op.setAuthorityChecker([](std::int32_t x, std::int32_t y, sims3000::PlayerID player_id) {
        (void)x; (void)y; (void)player_id;
        return true;
    });

    auto result = op.validate_terraform_request(64, 64, TerrainType::Substrate, 1, registry);
    ASSERT_EQ(result, TerraformValidationResult::Valid);
}

TEST(validation_insufficient_funds) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_cost = 10000;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    // Set up credits query that returns 5000 (not enough for 10000 cost)
    op.setCreditsQuery([](sims3000::PlayerID player_id) -> sims3000::Credits {
        (void)player_id;
        return 5000;
    });

    auto result = op.validate_terraform_request(64, 64, TerrainType::Substrate, 1, registry);
    ASSERT_EQ(result, TerraformValidationResult::InsufficientFunds);
}

TEST(validation_sufficient_funds) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_cost = 10000;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    // Set up credits query that returns exactly enough
    op.setCreditsQuery([](sims3000::PlayerID player_id) -> sims3000::Credits {
        (void)player_id;
        return 10000;
    });

    auto result = op.validate_terraform_request(64, 64, TerrainType::Substrate, 1, registry);
    ASSERT_EQ(result, TerraformValidationResult::Valid);
}

TEST(validation_funds_and_authority_both_checked) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_cost = 10000;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    // Set up authority checker (player 1 has authority)
    op.setAuthorityChecker([](std::int32_t x, std::int32_t y, sims3000::PlayerID player_id) {
        (void)x; (void)y;
        return player_id == 1;
    });

    // Set up credits query (player 1 has 5000, player 2 has 20000)
    op.setCreditsQuery([](sims3000::PlayerID player_id) -> sims3000::Credits {
        return (player_id == 1) ? 5000 : 20000;
    });

    // Player 1: has authority but not enough funds
    auto result = op.validate_terraform_request(64, 64, TerrainType::Substrate, 1, registry);
    ASSERT_EQ(result, TerraformValidationResult::InsufficientFunds);

    // Player 2: has funds but no authority (authority check fails first)
    result = op.validate_terraform_request(64, 64, TerrainType::Substrate, 2, registry);
    ASSERT_EQ(result, TerraformValidationResult::NoAuthority);
}

// =============================================================================
// Cost Calculation Tests
// =============================================================================

TEST(cost_blight_mires) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_cost = 10000;
    TerraformOperation op(grid, tracker, config);

    auto cost = op.calculate_terraform_cost(64, 64, TerrainType::Substrate);
    ASSERT_EQ(cost, 10000);
}

TEST(cost_ember_crust) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::EmberCrust);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.ember_crust_cost = 5000;
    TerraformOperation op(grid, tracker, config);

    auto cost = op.calculate_terraform_cost(64, 64, TerrainType::Substrate);
    ASSERT_EQ(cost, 5000);
}

TEST(cost_invalid_for_non_terraformable) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::DeepVoid);

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);

    auto cost = op.calculate_terraform_cost(64, 64, TerrainType::Substrate);
    ASSERT_EQ(cost, -1);
}

TEST(cost_zero_for_already_substrate) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);

    auto cost = op.calculate_terraform_cost(64, 64, TerrainType::Substrate);
    ASSERT_EQ(cost, 0);
}

TEST(cost_invalid_for_out_of_bounds) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);

    auto cost = op.calculate_terraform_cost(-1, 64, TerrainType::Substrate);
    ASSERT_EQ(cost, -1);
}

// =============================================================================
// Duration Calculation Tests
// =============================================================================

TEST(duration_blight_mires_longest) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);
    grid.at(65, 65).setTerrainType(TerrainType::EmberCrust);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_ticks = 100;
    config.ember_crust_ticks = 50;
    TerraformOperation op(grid, tracker, config);

    auto blight_duration = op.calculate_terraform_duration(64, 64, TerrainType::Substrate);
    auto ember_duration = op.calculate_terraform_duration(65, 65, TerrainType::Substrate);

    ASSERT_EQ(blight_duration, 100);
    ASSERT_EQ(ember_duration, 50);
    ASSERT(blight_duration > ember_duration); // BlightMires takes longer
}

TEST(duration_zero_for_non_terraformable) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::DeepVoid);

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);

    auto duration = op.calculate_terraform_duration(64, 64, TerrainType::Substrate);
    ASSERT_EQ(duration, 0);
}

// =============================================================================
// Operation Creation Tests
// =============================================================================

TEST(create_operation_returns_entity) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);
    entt::registry registry;

    auto entity = op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);
    ASSERT(entity != entt::null);
    ASSERT(registry.valid(entity));
}

TEST(create_operation_sets_component_correctly) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_cost = 10000;
    config.blight_mires_ticks = 100;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    auto entity = op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 2);
    ASSERT(entity != entt::null);

    auto* comp = registry.try_get<TerrainModificationComponent>(entity);
    ASSERT(comp != nullptr);

    ASSERT_EQ(comp->tile_x, 64);
    ASSERT_EQ(comp->tile_y, 64);
    ASSERT_EQ(comp->player_id, 2);
    ASSERT_EQ(comp->operation_type, TerrainOperationType::TerraformTerrain);
    ASSERT_EQ(comp->cancelled, 0);
    ASSERT_EQ(comp->terraforming.source_type, static_cast<std::uint8_t>(TerrainType::BlightMires));
    ASSERT_EQ(comp->terraforming.target_type, static_cast<std::uint8_t>(TerrainType::Substrate));
    ASSERT_EQ(comp->terraforming.ticks_remaining, 100);
    ASSERT_EQ(comp->terraforming.total_ticks, 100);
    ASSERT_EQ(comp->total_cost, 10000);
}

TEST(create_operation_returns_null_for_invalid) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::DeepVoid); // Not terraformable

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);
    entt::registry registry;

    auto entity = op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);
    ASSERT(entity == entt::null);
}

// =============================================================================
// Multi-Tick Operation Tests
// =============================================================================

TEST(tick_decrements_remaining) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_ticks = 5;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    auto entity = op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);
    auto* comp = registry.try_get<TerrainModificationComponent>(entity);
    ASSERT_EQ(comp->terraforming.ticks_remaining, 5);

    op.tick_terraform_operations(registry);
    ASSERT_EQ(comp->terraforming.ticks_remaining, 4);

    op.tick_terraform_operations(registry);
    ASSERT_EQ(comp->terraforming.ticks_remaining, 3);
}

TEST(tick_changes_terrain_on_completion) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_ticks = 3;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);

    // Terrain should still be BlightMires during operation
    ASSERT_EQ(grid.at(64, 64).getTerrainType(), TerrainType::BlightMires);

    op.tick_terraform_operations(registry); // 3 -> 2
    ASSERT_EQ(grid.at(64, 64).getTerrainType(), TerrainType::BlightMires);

    op.tick_terraform_operations(registry); // 2 -> 1
    ASSERT_EQ(grid.at(64, 64).getTerrainType(), TerrainType::BlightMires);

    op.tick_terraform_operations(registry); // 1 -> 0, complete
    ASSERT_EQ(grid.at(64, 64).getTerrainType(), TerrainType::Substrate);
}

TEST(tick_destroys_entity_on_completion) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::EmberCrust);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.ember_crust_ticks = 2;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    auto entity = op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);
    ASSERT(registry.valid(entity));

    op.tick_terraform_operations(registry); // 2 -> 1
    ASSERT(registry.valid(entity));

    op.tick_terraform_operations(registry); // 1 -> 0, complete
    ASSERT(!registry.valid(entity)); // Entity destroyed
}

TEST(tick_fires_event_on_completion) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_ticks = 2;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    std::vector<TerrainModifiedEvent> events;
    op.setEventCallback([&events](const TerrainModifiedEvent& event) {
        events.push_back(event);
    });

    op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);

    op.tick_terraform_operations(registry); // 2 -> 1, no event yet
    ASSERT_EQ(events.size(), 0);

    op.tick_terraform_operations(registry); // 1 -> 0, complete
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].modification_type, ModificationType::Terraformed);
    ASSERT_EQ(events[0].affected_area.x, 64);
    ASSERT_EQ(events[0].affected_area.y, 64);
}

TEST(tick_marks_chunk_dirty_on_completion) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    tracker.clearAllDirty(); // Start clean

    TerraformCostConfig config;
    config.blight_mires_ticks = 1;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);

    ASSERT(!tracker.hasAnyDirty());

    op.tick_terraform_operations(registry); // Complete

    ASSERT(tracker.hasAnyDirty());
    // Chunk for tile (64, 64) is (64/32, 64/32) = (2, 2)
    ASSERT(tracker.isChunkDirty(2, 2));
}

TEST(tick_invalidates_contamination_cache_for_blight_mires) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_ticks = 1;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    bool cache_invalidated = false;
    op.setContaminationCacheInvalidator([&cache_invalidated]() {
        cache_invalidated = true;
    });

    op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);

    ASSERT(!cache_invalidated);

    op.tick_terraform_operations(registry); // Complete

    ASSERT(cache_invalidated);
}

TEST(tick_does_not_invalidate_cache_for_ember_crust) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::EmberCrust);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.ember_crust_ticks = 1;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    bool cache_invalidated = false;
    op.setContaminationCacheInvalidator([&cache_invalidated]() {
        cache_invalidated = true;
    });

    op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);

    op.tick_terraform_operations(registry); // Complete

    ASSERT(!cache_invalidated); // EmberCrust doesn't generate contamination
}

// =============================================================================
// Cancel Tests
// =============================================================================

TEST(cancel_stops_operation) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_ticks = 10;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    auto entity = op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);

    // Do 5 ticks
    for (int i = 0; i < 5; ++i) {
        op.tick_terraform_operations(registry);
    }

    // Terrain should still be BlightMires
    ASSERT_EQ(grid.at(64, 64).getTerrainType(), TerrainType::BlightMires);

    // Cancel
    bool cancelled = op.cancel_terraform_operation(registry, entity);
    ASSERT(cancelled);

    // Next tick destroys entity but doesn't change terrain
    op.tick_terraform_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getTerrainType(), TerrainType::BlightMires); // No change
    ASSERT(!registry.valid(entity));
}

TEST(cancel_returns_false_for_invalid_entity) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);
    entt::registry registry;

    bool cancelled = op.cancel_terraform_operation(registry, entt::null);
    ASSERT(!cancelled);
}

TEST(cancel_returns_false_for_non_terraform_entity) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation terraform_op(grid, tracker);
    GradeTerrainOperation grade_op(grid, tracker);
    entt::registry registry;

    // Create a grading operation
    auto entity = grade_op.create_grade_operation(registry, 64, 64, 15, 1);
    ASSERT(entity != entt::null);

    // Try to cancel via terraform - should fail
    bool cancelled = terraform_op.cancel_terraform_operation(registry, entity);
    ASSERT(!cancelled);
}

TEST(cancel_refund_calculation) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_cost = 10000;
    config.blight_mires_ticks = 100;
    config.cancel_refund_percent = 50;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    auto entity = op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);

    // No ticks done - full remaining, 50% refund
    // Refund = 10000 * (100/100) * 0.50 = 5000
    auto refund = op.calculate_cancel_refund(registry, entity);
    ASSERT_EQ(refund, 5000);

    // Do 50 ticks (half done)
    for (int i = 0; i < 50; ++i) {
        op.tick_terraform_operations(registry);
    }

    // 50 ticks remaining - half remaining, 50% refund
    // Refund = 10000 * (50/100) * 0.50 = 2500
    refund = op.calculate_cancel_refund(registry, entity);
    ASSERT_EQ(refund, 2500);
}

TEST(cancel_refund_zero_for_completed) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::EmberCrust);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.ember_crust_ticks = 2;
    config.cancel_refund_percent = 50;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    auto entity = op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);

    // Complete the operation
    op.tick_terraform_operations(registry);
    op.tick_terraform_operations(registry);

    // Entity is destroyed
    auto refund = op.calculate_cancel_refund(registry, entity);
    ASSERT_EQ(refund, 0); // Invalid entity
}

// =============================================================================
// Find Operation Tests
// =============================================================================

TEST(find_operation_for_tile) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);
    grid.at(65, 65).setTerrainType(TerrainType::EmberCrust);

    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);
    entt::registry registry;

    auto entity1 = op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);
    auto entity2 = op.create_terraform_operation(registry, 65, 65, TerrainType::Substrate, 1);

    // Find first operation
    auto found = op.find_operation_for_tile(registry, 64, 64);
    ASSERT_EQ(found, entity1);

    // Find second operation
    found = op.find_operation_for_tile(registry, 65, 65);
    ASSERT_EQ(found, entity2);

    // No operation at unused tile
    found = op.find_operation_for_tile(registry, 66, 66);
    ASSERT(found == entt::null);
}

// =============================================================================
// BlightMires -> Substrate Specific Tests
// =============================================================================

TEST(blight_mires_to_substrate_removes_contamination_source) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_ticks = 1;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    // BlightMires generates contamination
    const auto& info_before = getTerrainInfo(TerrainType::BlightMires);
    ASSERT(info_before.generates_contamination);

    op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);
    op.tick_terraform_operations(registry);

    // Now it's Substrate, which doesn't generate contamination
    ASSERT_EQ(grid.at(64, 64).getTerrainType(), TerrainType::Substrate);
    const auto& info_after = getTerrainInfo(TerrainType::Substrate);
    ASSERT(!info_after.generates_contamination);
}

TEST(blight_mires_high_cost_long_duration) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);
    grid.at(65, 65).setTerrainType(TerrainType::EmberCrust);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_cost = 10000;
    config.blight_mires_ticks = 100;
    config.ember_crust_cost = 5000;
    config.ember_crust_ticks = 50;
    TerraformOperation op(grid, tracker, config);

    auto blight_cost = op.calculate_terraform_cost(64, 64, TerrainType::Substrate);
    auto ember_cost = op.calculate_terraform_cost(65, 65, TerrainType::Substrate);

    ASSERT(blight_cost > ember_cost); // BlightMires more expensive

    auto blight_duration = op.calculate_terraform_duration(64, 64, TerrainType::Substrate);
    auto ember_duration = op.calculate_terraform_duration(65, 65, TerrainType::Substrate);

    ASSERT(blight_duration > ember_duration); // BlightMires takes longer
}

// =============================================================================
// EmberCrust -> Substrate Specific Tests
// =============================================================================

TEST(ember_crust_to_substrate_removes_build_cost_modifier) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::EmberCrust);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.ember_crust_ticks = 1;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    // EmberCrust has build cost modifier > 1.0
    const auto& info_before = getTerrainInfo(TerrainType::EmberCrust);
    ASSERT(info_before.build_cost_modifier > 1.0f);

    op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);
    op.tick_terraform_operations(registry);

    // Now it's Substrate with normal build cost modifier
    ASSERT_EQ(grid.at(64, 64).getTerrainType(), TerrainType::Substrate);
    const auto& info_after = getTerrainInfo(TerrainType::Substrate);
    ASSERT_EQ(info_after.build_cost_modifier, 1.0f);
}

// =============================================================================
// Multiple Concurrent Operations
// =============================================================================

TEST(multiple_concurrent_operations) {
    TerrainGrid grid(MapSize::Small);
    grid.at(60, 60).setTerrainType(TerrainType::BlightMires);
    grid.at(70, 70).setTerrainType(TerrainType::EmberCrust);

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_ticks = 5;
    config.ember_crust_ticks = 3;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    auto entity1 = op.create_terraform_operation(registry, 60, 60, TerrainType::Substrate, 1);
    auto entity2 = op.create_terraform_operation(registry, 70, 70, TerrainType::Substrate, 2);
    ASSERT(entity1 != entt::null);
    ASSERT(entity2 != entt::null);

    // Both still original types
    ASSERT_EQ(grid.at(60, 60).getTerrainType(), TerrainType::BlightMires);
    ASSERT_EQ(grid.at(70, 70).getTerrainType(), TerrainType::EmberCrust);

    // Tick 3 times - EmberCrust should complete
    op.tick_terraform_operations(registry);
    op.tick_terraform_operations(registry);
    op.tick_terraform_operations(registry);

    ASSERT_EQ(grid.at(60, 60).getTerrainType(), TerrainType::BlightMires); // Still in progress
    ASSERT_EQ(grid.at(70, 70).getTerrainType(), TerrainType::Substrate);   // Complete

    // Tick 2 more times - BlightMires should complete
    op.tick_terraform_operations(registry);
    op.tick_terraform_operations(registry);

    ASSERT_EQ(grid.at(60, 60).getTerrainType(), TerrainType::Substrate); // Complete
}

// =============================================================================
// Config Tests
// =============================================================================

TEST(config_getConfig) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_cost = 12345;
    TerraformOperation op(grid, tracker, config);

    ASSERT_EQ(op.getConfig().blight_mires_cost, 12345);
}

TEST(config_setConfig) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerraformOperation op(grid, tracker);

    ASSERT_EQ(op.getConfig().blight_mires_cost, 10000); // Default

    TerraformCostConfig newConfig;
    newConfig.blight_mires_cost = 50000;
    op.setConfig(newConfig);

    ASSERT_EQ(op.getConfig().blight_mires_cost, 50000);
}

// =============================================================================
// Cleared Flag Test
// =============================================================================

TEST(terraformed_tile_cleared_flag_reset) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::BlightMires);
    grid.at(64, 64).setCleared(true); // Shouldn't matter, but set it

    ChunkDirtyTracker tracker(128, 128);
    TerraformCostConfig config;
    config.blight_mires_ticks = 1;
    TerraformOperation op(grid, tracker, config);
    entt::registry registry;

    op.create_terraform_operation(registry, 64, 64, TerrainType::Substrate, 1);
    op.tick_terraform_operations(registry);

    // Terraformed tile should not be "cleared" - it's fresh substrate
    ASSERT(!grid.at(64, 64).isCleared());
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Terraform Operation Unit Tests ===\n\n");

    // Component/struct tests
    RUN_TEST(terraforming_operation_size_is_8_bytes);
    RUN_TEST(terraforming_operation_is_trivially_copyable);
    RUN_TEST(component_size_is_32_bytes);
    RUN_TEST(component_is_trivially_copyable);
    RUN_TEST(component_is_terraforming);
    RUN_TEST(component_is_complete_terraform);
    RUN_TEST(component_terraform_progress);

    // isTerraformable tests
    RUN_TEST(is_terraformable_blight_mires);
    RUN_TEST(is_terraformable_ember_crust);
    RUN_TEST(is_not_terraformable_substrate);
    RUN_TEST(is_not_terraformable_ridge);
    RUN_TEST(is_not_terraformable_water_types);
    RUN_TEST(is_not_terraformable_vegetation);

    // Validation tests
    RUN_TEST(validation_valid_blight_mires);
    RUN_TEST(validation_valid_ember_crust);
    RUN_TEST(validation_out_of_bounds);
    RUN_TEST(validation_not_terraformable);
    RUN_TEST(validation_already_terraforming);
    RUN_TEST(validation_only_substrate_target);
    RUN_TEST(validation_no_authority_with_checker);
    RUN_TEST(validation_authority_granted_when_checker_allows);
    RUN_TEST(validation_insufficient_funds);
    RUN_TEST(validation_sufficient_funds);
    RUN_TEST(validation_funds_and_authority_both_checked);

    // Cost calculation tests
    RUN_TEST(cost_blight_mires);
    RUN_TEST(cost_ember_crust);
    RUN_TEST(cost_invalid_for_non_terraformable);
    RUN_TEST(cost_zero_for_already_substrate);
    RUN_TEST(cost_invalid_for_out_of_bounds);

    // Duration calculation tests
    RUN_TEST(duration_blight_mires_longest);
    RUN_TEST(duration_zero_for_non_terraformable);

    // Operation creation tests
    RUN_TEST(create_operation_returns_entity);
    RUN_TEST(create_operation_sets_component_correctly);
    RUN_TEST(create_operation_returns_null_for_invalid);

    // Multi-tick operation tests
    RUN_TEST(tick_decrements_remaining);
    RUN_TEST(tick_changes_terrain_on_completion);
    RUN_TEST(tick_destroys_entity_on_completion);
    RUN_TEST(tick_fires_event_on_completion);
    RUN_TEST(tick_marks_chunk_dirty_on_completion);
    RUN_TEST(tick_invalidates_contamination_cache_for_blight_mires);
    RUN_TEST(tick_does_not_invalidate_cache_for_ember_crust);

    // Cancel tests
    RUN_TEST(cancel_stops_operation);
    RUN_TEST(cancel_returns_false_for_invalid_entity);
    RUN_TEST(cancel_returns_false_for_non_terraform_entity);
    RUN_TEST(cancel_refund_calculation);
    RUN_TEST(cancel_refund_zero_for_completed);

    // Find operation tests
    RUN_TEST(find_operation_for_tile);

    // BlightMires specific tests
    RUN_TEST(blight_mires_to_substrate_removes_contamination_source);
    RUN_TEST(blight_mires_high_cost_long_duration);

    // EmberCrust specific tests
    RUN_TEST(ember_crust_to_substrate_removes_build_cost_modifier);

    // Multiple concurrent operations
    RUN_TEST(multiple_concurrent_operations);

    // Config tests
    RUN_TEST(config_getConfig);
    RUN_TEST(config_setConfig);

    // Cleared flag test
    RUN_TEST(terraformed_tile_cleared_flag_reset);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
