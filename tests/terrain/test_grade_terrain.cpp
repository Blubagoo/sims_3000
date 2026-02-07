/**
 * @file test_grade_terrain.cpp
 * @brief Unit tests for grade terrain (leveling) operation (Ticket 3-020)
 *
 * Tests cover:
 * - Validation of grade requests (bounds, water tiles, authority)
 * - Cost calculation (base_cost * elevation_delta)
 * - Multi-tick operation (one level per tick)
 * - TerrainModifiedEvent firing each tick
 * - Slope flag recalculation for affected tiles
 * - Chunk dirty marking
 * - Cancel support (partial result stays)
 * - Single-level and multi-level grading
 * - Rejection of water tiles
 */

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
// TerrainModificationComponent Tests
// =============================================================================

TEST(component_size_is_32_bytes) {
    ASSERT_EQ(sizeof(TerrainModificationComponent), 32);
}

TEST(component_is_trivially_copyable) {
    ASSERT(std::is_trivially_copyable<TerrainModificationComponent>::value);
}

TEST(grading_operation_size_is_4_bytes) {
    ASSERT_EQ(sizeof(GradingOperation), 4);
}

TEST(component_default_construction) {
    TerrainModificationComponent comp;
    ASSERT_EQ(comp.tile_x, 0);
    ASSERT_EQ(comp.tile_y, 0);
    ASSERT_EQ(comp.player_id, 0);
    ASSERT_EQ(comp.operation_type, TerrainOperationType::None);
    ASSERT_EQ(comp.cancelled, 0);
    ASSERT(comp.isComplete()); // None type completes immediately
}

TEST(component_is_grading) {
    TerrainModificationComponent comp;
    ASSERT(!comp.isGrading());

    comp.operation_type = TerrainOperationType::GradeTerrain;
    ASSERT(comp.isGrading());
}

TEST(component_is_complete) {
    TerrainModificationComponent comp;
    comp.operation_type = TerrainOperationType::GradeTerrain;
    comp.grading.ticks_remaining = 5;

    ASSERT(!comp.isComplete());

    comp.grading.ticks_remaining = 0;
    ASSERT(comp.isComplete());
}

TEST(component_cancelled_is_complete) {
    TerrainModificationComponent comp;
    comp.operation_type = TerrainOperationType::GradeTerrain;
    comp.grading.ticks_remaining = 5;
    comp.cancelled = 1;

    ASSERT(comp.isComplete());
}

TEST(component_cancel) {
    TerrainModificationComponent comp;
    comp.operation_type = TerrainOperationType::GradeTerrain;
    ASSERT_EQ(comp.cancelled, 0);

    comp.cancel();
    ASSERT_EQ(comp.cancelled, 1);
}

TEST(component_get_current_elevation_raising) {
    TerrainModificationComponent comp;
    comp.operation_type = TerrainOperationType::GradeTerrain;
    comp.grading.start_elevation = 10;
    comp.grading.target_elevation = 15;
    comp.grading.ticks_remaining = 3; // 2 changes made (10 -> 12)

    ASSERT_EQ(comp.getCurrentElevation(), 12);
}

TEST(component_get_current_elevation_lowering) {
    TerrainModificationComponent comp;
    comp.operation_type = TerrainOperationType::GradeTerrain;
    comp.grading.start_elevation = 20;
    comp.grading.target_elevation = 15;
    comp.grading.ticks_remaining = 2; // 3 changes made (20 -> 17)

    ASSERT_EQ(comp.getCurrentElevation(), 17);
}

// =============================================================================
// Validation Tests
// =============================================================================

TEST(validation_valid_request) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    auto result = op.validate_grade_request(64, 64, 15, 1, registry);
    ASSERT_EQ(result, GradeValidationResult::Valid);
}

TEST(validation_out_of_bounds) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    // Negative coordinates
    auto result = op.validate_grade_request(-1, 64, 15, 1, registry);
    ASSERT_EQ(result, GradeValidationResult::OutOfBounds);

    // Beyond map size
    result = op.validate_grade_request(200, 64, 15, 1, registry);
    ASSERT_EQ(result, GradeValidationResult::OutOfBounds);
}

TEST(validation_water_tiles_rejected) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    // DeepVoid
    grid.at(10, 10).setTerrainType(TerrainType::DeepVoid);
    ASSERT_EQ(op.validate_grade_request(10, 10, 5, 1, registry),
              GradeValidationResult::WaterTile);

    // FlowChannel
    grid.at(11, 11).setTerrainType(TerrainType::FlowChannel);
    ASSERT_EQ(op.validate_grade_request(11, 11, 5, 1, registry),
              GradeValidationResult::WaterTile);

    // StillBasin
    grid.at(12, 12).setTerrainType(TerrainType::StillBasin);
    ASSERT_EQ(op.validate_grade_request(12, 12, 5, 1, registry),
              GradeValidationResult::WaterTile);

    // BlightMires (toxic)
    grid.at(13, 13).setTerrainType(TerrainType::BlightMires);
    ASSERT_EQ(op.validate_grade_request(13, 13, 5, 1, registry),
              GradeValidationResult::WaterTile);
}

TEST(validation_target_out_of_range) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    // Target > 31
    auto result = op.validate_grade_request(64, 64, 50, 1, registry);
    ASSERT_EQ(result, GradeValidationResult::TargetOutOfRange);

    result = op.validate_grade_request(64, 64, 255, 1, registry);
    ASSERT_EQ(result, GradeValidationResult::TargetOutOfRange);
}

TEST(validation_already_grading) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    // Create first operation
    auto entity1 = op.create_grade_operation(registry, 64, 64, 15, 1);
    ASSERT(entity1 != entt::null);

    // Second operation on same tile should fail
    auto result = op.validate_grade_request(64, 64, 20, 1, registry);
    ASSERT_EQ(result, GradeValidationResult::AlreadyGrading);

    // But different tile should be fine
    grid.at(65, 65).setTerrainType(TerrainType::Substrate);
    result = op.validate_grade_request(65, 65, 20, 1, registry);
    ASSERT_EQ(result, GradeValidationResult::Valid);
}

// =============================================================================
// Cost Calculation Tests
// =============================================================================

TEST(cost_zero_for_same_elevation) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);

    auto cost = op.calculate_grade_cost(64, 64, 10);
    ASSERT_EQ(cost, 0);
}

TEST(cost_scales_with_elevation_delta) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeCostConfig config;
    config.base_cost_per_level = 10;
    GradeTerrainOperation op(grid, tracker, config);

    // +5 elevation
    auto cost = op.calculate_grade_cost(64, 64, 15);
    ASSERT_EQ(cost, 50); // 10 * 5

    // -5 elevation
    cost = op.calculate_grade_cost(64, 64, 5);
    ASSERT_EQ(cost, 50); // 10 * 5

    // +1 elevation
    cost = op.calculate_grade_cost(64, 64, 11);
    ASSERT_EQ(cost, 10); // 10 * 1
}

TEST(cost_returns_invalid_for_water) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::DeepVoid);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);

    auto cost = op.calculate_grade_cost(64, 64, 10);
    ASSERT_EQ(cost, -1);
}

TEST(cost_returns_invalid_for_out_of_bounds) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);

    auto cost = op.calculate_grade_cost(-1, 64, 10);
    ASSERT_EQ(cost, -1);

    cost = op.calculate_grade_cost(200, 64, 10);
    ASSERT_EQ(cost, -1);
}

TEST(cost_applies_minimum) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeCostConfig config;
    config.base_cost_per_level = 0;  // Zero cost per level
    config.minimum_cost = 5;
    GradeTerrainOperation op(grid, tracker, config);

    auto cost = op.calculate_grade_cost(64, 64, 15);
    ASSERT_EQ(cost, 5); // Minimum cost applied
}

// =============================================================================
// Operation Creation Tests
// =============================================================================

TEST(create_operation_returns_entity) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    auto entity = op.create_grade_operation(registry, 64, 64, 15, 1);
    ASSERT(entity != entt::null);
    ASSERT(registry.valid(entity));
}

TEST(create_operation_sets_component_correctly) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    auto entity = op.create_grade_operation(registry, 64, 64, 15, 2);
    ASSERT(entity != entt::null);

    auto* comp = registry.try_get<TerrainModificationComponent>(entity);
    ASSERT(comp != nullptr);

    ASSERT_EQ(comp->tile_x, 64);
    ASSERT_EQ(comp->tile_y, 64);
    ASSERT_EQ(comp->player_id, 2);
    ASSERT_EQ(comp->operation_type, TerrainOperationType::GradeTerrain);
    ASSERT_EQ(comp->cancelled, 0);
    ASSERT_EQ(comp->grading.start_elevation, 10);
    ASSERT_EQ(comp->grading.target_elevation, 15);
    ASSERT_EQ(comp->grading.ticks_remaining, 5); // 15 - 10 = 5
}

TEST(create_operation_returns_null_for_invalid) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::DeepVoid); // Water tile

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    auto entity = op.create_grade_operation(registry, 64, 64, 15, 1);
    ASSERT(entity == entt::null);
}

TEST(create_operation_returns_null_for_same_elevation) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(15);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    auto entity = op.create_grade_operation(registry, 64, 64, 15, 1);
    ASSERT(entity == entt::null); // No operation needed
}

// =============================================================================
// Multi-Tick Operation Tests
// =============================================================================

TEST(tick_changes_elevation_by_one_raising) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    op.create_grade_operation(registry, 64, 64, 15, 1);

    // First tick: 10 -> 11
    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 11);

    // Second tick: 11 -> 12
    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 12);

    // Third tick: 12 -> 13
    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 13);

    // Fourth tick: 13 -> 14
    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 14);

    // Fifth tick: 14 -> 15 (complete)
    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 15);
}

TEST(tick_changes_elevation_by_one_lowering) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(15);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    op.create_grade_operation(registry, 64, 64, 10, 1);

    // First tick: 15 -> 14
    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 14);

    // Run remaining ticks
    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 13);

    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 12);

    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 11);

    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 10);
}

TEST(tick_destroys_entity_on_completion) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    auto entity = op.create_grade_operation(registry, 64, 64, 12, 1); // 2 ticks
    ASSERT(registry.valid(entity));

    op.tick_grade_operations(registry); // 10 -> 11
    ASSERT(registry.valid(entity));

    op.tick_grade_operations(registry); // 11 -> 12, complete
    ASSERT(!registry.valid(entity)); // Entity destroyed
}

TEST(tick_fires_event_each_tick) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    std::vector<TerrainModifiedEvent> events;
    op.setEventCallback([&events](const TerrainModifiedEvent& event) {
        events.push_back(event);
    });

    op.create_grade_operation(registry, 64, 64, 13, 1); // 3 ticks

    op.tick_grade_operations(registry);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].modification_type, ModificationType::Leveled);
    ASSERT_EQ(events[0].affected_area.x, 64);
    ASSERT_EQ(events[0].affected_area.y, 64);

    op.tick_grade_operations(registry);
    ASSERT_EQ(events.size(), 2);

    op.tick_grade_operations(registry);
    ASSERT_EQ(events.size(), 3);
}

TEST(tick_marks_chunk_dirty) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    tracker.clearAllDirty(); // Start clean

    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    op.create_grade_operation(registry, 64, 64, 15, 1);

    ASSERT(!tracker.hasAnyDirty());

    op.tick_grade_operations(registry);

    ASSERT(tracker.hasAnyDirty());
    // Chunk for tile (64, 64) is (64/32, 64/32) = (2, 2)
    ASSERT(tracker.isChunkDirty(2, 2));
}

// =============================================================================
// Slope Flag Tests
// =============================================================================

TEST(tick_updates_slope_flag) {
    TerrainGrid grid(MapSize::Small);
    // Set up a flat area
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);
    grid.at(63, 64).setTerrainType(TerrainType::Substrate);
    grid.at(63, 64).setElevation(10);
    grid.at(65, 64).setTerrainType(TerrainType::Substrate);
    grid.at(65, 64).setElevation(10);
    grid.at(64, 63).setTerrainType(TerrainType::Substrate);
    grid.at(64, 63).setElevation(10);
    grid.at(64, 65).setTerrainType(TerrainType::Substrate);
    grid.at(64, 65).setElevation(10);

    // All should be flat initially (assuming no slope)
    ASSERT(!grid.at(64, 64).isSlope());

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    // Raise center tile
    op.create_grade_operation(registry, 64, 64, 11, 1);
    op.tick_grade_operations(registry);

    // Now center and neighbors should be slopes
    ASSERT(grid.at(64, 64).isSlope()); // Center is higher than neighbors
    ASSERT(grid.at(63, 64).isSlope()); // Adjacent to different elevation
    ASSERT(grid.at(65, 64).isSlope());
    ASSERT(grid.at(64, 63).isSlope());
    ASSERT(grid.at(64, 65).isSlope());
}

// =============================================================================
// Cancel Tests
// =============================================================================

TEST(cancel_stops_operation) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    auto entity = op.create_grade_operation(registry, 64, 64, 20, 1); // 10 ticks

    // Do 3 ticks: 10 -> 13
    op.tick_grade_operations(registry);
    op.tick_grade_operations(registry);
    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 13);

    // Cancel
    bool cancelled = op.cancel_grade_operation(registry, entity);
    ASSERT(cancelled);

    // Next tick destroys entity but doesn't change elevation
    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 13); // Partial result stays
    ASSERT(!registry.valid(entity));
}

TEST(cancel_returns_false_for_invalid_entity) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    bool cancelled = op.cancel_grade_operation(registry, entt::null);
    ASSERT(!cancelled);
}

TEST(cancel_returns_false_for_non_operation_entity) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    // Create entity without component
    auto entity = registry.create();

    bool cancelled = op.cancel_grade_operation(registry, entity);
    ASSERT(!cancelled);
}

// =============================================================================
// Find Operation Tests
// =============================================================================

TEST(find_operation_for_tile) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);
    grid.at(65, 65).setTerrainType(TerrainType::Substrate);
    grid.at(65, 65).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    auto entity1 = op.create_grade_operation(registry, 64, 64, 15, 1);
    auto entity2 = op.create_grade_operation(registry, 65, 65, 20, 1);

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
// Single Level Grading Test
// =============================================================================

TEST(single_level_grading) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    auto entity = op.create_grade_operation(registry, 64, 64, 11, 1); // Just 1 level
    ASSERT(entity != entt::null);

    auto* comp = registry.try_get<TerrainModificationComponent>(entity);
    ASSERT_EQ(comp->grading.ticks_remaining, 1);

    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 11);
    ASSERT(!registry.valid(entity)); // Should be complete
}

// =============================================================================
// Elevation Boundary Tests
// =============================================================================

TEST(grade_to_zero_elevation) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(3);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    auto entity = op.create_grade_operation(registry, 64, 64, 0, 1);
    ASSERT(entity != entt::null);

    // Run 3 ticks
    op.tick_grade_operations(registry);
    op.tick_grade_operations(registry);
    op.tick_grade_operations(registry);

    ASSERT_EQ(grid.at(64, 64).getElevation(), 0);
}

TEST(grade_to_max_elevation) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(28);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    auto entity = op.create_grade_operation(registry, 64, 64, 31, 1);
    ASSERT(entity != entt::null);

    // Run 3 ticks
    op.tick_grade_operations(registry);
    op.tick_grade_operations(registry);
    op.tick_grade_operations(registry);

    ASSERT_EQ(grid.at(64, 64).getElevation(), 31);
}

// =============================================================================
// Multiple Concurrent Operations
// =============================================================================

TEST(multiple_concurrent_operations) {
    TerrainGrid grid(MapSize::Small);
    grid.at(60, 60).setTerrainType(TerrainType::Substrate);
    grid.at(60, 60).setElevation(10);
    grid.at(70, 70).setTerrainType(TerrainType::Substrate);
    grid.at(70, 70).setElevation(20);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);
    entt::registry registry;

    auto entity1 = op.create_grade_operation(registry, 60, 60, 15, 1); // +5
    auto entity2 = op.create_grade_operation(registry, 70, 70, 15, 2); // -5
    ASSERT(entity1 != entt::null);
    ASSERT(entity2 != entt::null);

    // Tick once - both should progress
    op.tick_grade_operations(registry);
    ASSERT_EQ(grid.at(60, 60).getElevation(), 11); // 10 -> 11
    ASSERT_EQ(grid.at(70, 70).getElevation(), 19); // 20 -> 19

    // Tick 4 more times - both complete
    for (int i = 0; i < 4; ++i) {
        op.tick_grade_operations(registry);
    }

    ASSERT_EQ(grid.at(60, 60).getElevation(), 15);
    ASSERT_EQ(grid.at(70, 70).getElevation(), 15);
}

// =============================================================================
// Config Tests
// =============================================================================

TEST(config_custom_base_cost) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeCostConfig config;
    config.base_cost_per_level = 25;
    GradeTerrainOperation op(grid, tracker, config);

    auto cost = op.calculate_grade_cost(64, 64, 15);
    ASSERT_EQ(cost, 125); // 25 * 5
}

TEST(config_setConfig) {
    TerrainGrid grid(MapSize::Small);
    grid.at(64, 64).setTerrainType(TerrainType::Substrate);
    grid.at(64, 64).setElevation(10);

    ChunkDirtyTracker tracker(128, 128);
    GradeTerrainOperation op(grid, tracker);

    ASSERT_EQ(op.getConfig().base_cost_per_level, 10); // Default

    GradeCostConfig newConfig;
    newConfig.base_cost_per_level = 50;
    op.setConfig(newConfig);

    ASSERT_EQ(op.getConfig().base_cost_per_level, 50);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Grade Terrain Operation Unit Tests ===\n\n");

    // Component tests
    RUN_TEST(component_size_is_32_bytes);
    RUN_TEST(component_is_trivially_copyable);
    RUN_TEST(grading_operation_size_is_4_bytes);
    RUN_TEST(component_default_construction);
    RUN_TEST(component_is_grading);
    RUN_TEST(component_is_complete);
    RUN_TEST(component_cancelled_is_complete);
    RUN_TEST(component_cancel);
    RUN_TEST(component_get_current_elevation_raising);
    RUN_TEST(component_get_current_elevation_lowering);

    // Validation tests
    RUN_TEST(validation_valid_request);
    RUN_TEST(validation_out_of_bounds);
    RUN_TEST(validation_water_tiles_rejected);
    RUN_TEST(validation_target_out_of_range);
    RUN_TEST(validation_already_grading);

    // Cost calculation tests
    RUN_TEST(cost_zero_for_same_elevation);
    RUN_TEST(cost_scales_with_elevation_delta);
    RUN_TEST(cost_returns_invalid_for_water);
    RUN_TEST(cost_returns_invalid_for_out_of_bounds);
    RUN_TEST(cost_applies_minimum);

    // Operation creation tests
    RUN_TEST(create_operation_returns_entity);
    RUN_TEST(create_operation_sets_component_correctly);
    RUN_TEST(create_operation_returns_null_for_invalid);
    RUN_TEST(create_operation_returns_null_for_same_elevation);

    // Multi-tick operation tests
    RUN_TEST(tick_changes_elevation_by_one_raising);
    RUN_TEST(tick_changes_elevation_by_one_lowering);
    RUN_TEST(tick_destroys_entity_on_completion);
    RUN_TEST(tick_fires_event_each_tick);
    RUN_TEST(tick_marks_chunk_dirty);

    // Slope flag tests
    RUN_TEST(tick_updates_slope_flag);

    // Cancel tests
    RUN_TEST(cancel_stops_operation);
    RUN_TEST(cancel_returns_false_for_invalid_entity);
    RUN_TEST(cancel_returns_false_for_non_operation_entity);

    // Find operation tests
    RUN_TEST(find_operation_for_tile);

    // Single level grading
    RUN_TEST(single_level_grading);

    // Elevation boundary tests
    RUN_TEST(grade_to_zero_elevation);
    RUN_TEST(grade_to_max_elevation);

    // Multiple concurrent operations
    RUN_TEST(multiple_concurrent_operations);

    // Config tests
    RUN_TEST(config_custom_base_cost);
    RUN_TEST(config_setConfig);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
