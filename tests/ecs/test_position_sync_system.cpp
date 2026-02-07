/**
 * @file test_position_sync_system.cpp
 * @brief Unit tests for PositionSyncSystem (Ticket 2-033).
 *
 * Tests cover:
 * - Automatic sync when PositionComponent changes
 * - Coordinate mapping: grid_x -> X, grid_y -> Z, elevation -> Y
 * - Configurable elevation step (default: 0.25 per level)
 * - Configurable grid unit size (default: 1.0)
 * - TransformComponent model matrix recalculation on change
 */

#include "sims3000/ecs/PositionSyncSystem.h"
#include "sims3000/ecs/Components.h"
#include "sims3000/ecs/Registry.h"
#include "sims3000/core/ISimulationTime.h"

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000;

// ============================================================================
// Test helpers
// ============================================================================

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  FAIL: %s\n", msg); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            printf("  FAIL: %s (expected %lld, got %lld)\n", msg, (long long)(b), (long long)(a)); \
            return false; \
        } \
    } while(0)

static constexpr float EPSILON = 0.0001f;

bool float_eq(float a, float b) {
    return std::abs(a - b) < EPSILON;
}

bool vec3_eq(const glm::vec3& a, const glm::vec3& b) {
    return float_eq(a.x, b.x) && float_eq(a.y, b.y) && float_eq(a.z, b.z);
}

// Mock simulation time for testing
class MockSimulationTime : public ISimulationTime {
public:
    SimulationTick getCurrentTick() const override { return m_tick; }
    float getTickDelta() const override { return 0.05f; }  // 20 Hz
    float getInterpolation() const override { return m_interpolation; }
    double getTotalTime() const override { return static_cast<double>(m_tick) * 0.05; }
    void setTick(SimulationTick tick) { m_tick = tick; }
    void setInterpolation(float interp) { m_interpolation = interp; }
private:
    SimulationTick m_tick = 0;
    float m_interpolation = 0.0f;
};

// ============================================================================
// Coordinate Mapping Tests
// ============================================================================

bool test_grid_x_to_world_x() {
    printf("  test_grid_x_to_world_x...\n");

    Registry registry;
    PositionSyncSystem system(registry);

    // Default config: grid_unit_size = 1.0
    TEST_ASSERT(float_eq(system.gridXToWorldX(0), 0.0f), "grid_x=0 -> world_x=0");
    TEST_ASSERT(float_eq(system.gridXToWorldX(10), 10.0f), "grid_x=10 -> world_x=10");
    TEST_ASSERT(float_eq(system.gridXToWorldX(-5), -5.0f), "grid_x=-5 -> world_x=-5");

    printf("  PASS\n");
    return true;
}

bool test_grid_y_to_world_z() {
    printf("  test_grid_y_to_world_z...\n");

    Registry registry;
    PositionSyncSystem system(registry);

    // grid_y maps to world_z (not world_y!)
    TEST_ASSERT(float_eq(system.gridYToWorldZ(0), 0.0f), "grid_y=0 -> world_z=0");
    TEST_ASSERT(float_eq(system.gridYToWorldZ(20), 20.0f), "grid_y=20 -> world_z=20");
    TEST_ASSERT(float_eq(system.gridYToWorldZ(-10), -10.0f), "grid_y=-10 -> world_z=-10");

    printf("  PASS\n");
    return true;
}

bool test_elevation_to_world_y() {
    printf("  test_elevation_to_world_y...\n");

    Registry registry;
    PositionSyncSystem system(registry);

    // Default config: elevation_step = 0.25
    TEST_ASSERT(float_eq(system.elevationToWorldY(0), 0.0f), "elevation=0 -> world_y=0");
    TEST_ASSERT(float_eq(system.elevationToWorldY(4), 1.0f), "elevation=4 -> world_y=1.0");
    TEST_ASSERT(float_eq(system.elevationToWorldY(8), 2.0f), "elevation=8 -> world_y=2.0");
    TEST_ASSERT(float_eq(system.elevationToWorldY(1), 0.25f), "elevation=1 -> world_y=0.25");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Configuration Tests
// ============================================================================

bool test_configurable_grid_unit_size() {
    printf("  test_configurable_grid_unit_size...\n");

    Registry registry;
    PositionSyncConfig config;
    config.grid_unit_size = 2.0f;  // 2 world units per grid cell
    PositionSyncSystem system(registry, config);

    TEST_ASSERT(float_eq(system.gridXToWorldX(5), 10.0f), "grid_x=5 with unit=2 -> world_x=10");
    TEST_ASSERT(float_eq(system.gridYToWorldZ(3), 6.0f), "grid_y=3 with unit=2 -> world_z=6");

    printf("  PASS\n");
    return true;
}

bool test_configurable_elevation_step() {
    printf("  test_configurable_elevation_step...\n");

    Registry registry;
    PositionSyncConfig config;
    config.elevation_step = 0.5f;  // 0.5 world units per elevation level
    PositionSyncSystem system(registry, config);

    TEST_ASSERT(float_eq(system.elevationToWorldY(2), 1.0f), "elevation=2 with step=0.5 -> world_y=1.0");
    TEST_ASSERT(float_eq(system.elevationToWorldY(4), 2.0f), "elevation=4 with step=0.5 -> world_y=2.0");

    printf("  PASS\n");
    return true;
}

bool test_set_config_at_runtime() {
    printf("  test_set_config_at_runtime...\n");

    Registry registry;
    PositionSyncSystem system(registry);

    // Change config at runtime
    system.setGridUnitSize(3.0f);
    TEST_ASSERT(float_eq(system.gridXToWorldX(2), 6.0f), "grid_x=2 with unit=3 -> world_x=6");

    system.setElevationStep(1.0f);
    TEST_ASSERT(float_eq(system.elevationToWorldY(5), 5.0f), "elevation=5 with step=1.0 -> world_y=5");

    // Set full config
    PositionSyncConfig newConfig;
    newConfig.grid_unit_size = 0.5f;
    newConfig.elevation_step = 0.1f;
    system.setConfig(newConfig);

    TEST_ASSERT(float_eq(system.gridXToWorldX(4), 2.0f), "grid_x=4 with unit=0.5 -> world_x=2");
    TEST_ASSERT(float_eq(system.elevationToWorldY(10), 1.0f), "elevation=10 with step=0.1 -> world_y=1");

    printf("  PASS\n");
    return true;
}

bool test_config_with_offsets() {
    printf("  test_config_with_offsets...\n");

    Registry registry;
    PositionSyncConfig config;
    config.grid_unit_size = 1.0f;
    config.elevation_step = 0.25f;
    config.grid_x_offset = 100.0f;
    config.grid_y_offset = 50.0f;
    config.elevation_offset = 10.0f;
    PositionSyncSystem system(registry, config);

    TEST_ASSERT(float_eq(system.gridXToWorldX(0), 100.0f), "grid_x=0 with offset=100 -> world_x=100");
    TEST_ASSERT(float_eq(system.gridYToWorldZ(0), 50.0f), "grid_y=0 with offset=50 -> world_z=50");
    TEST_ASSERT(float_eq(system.elevationToWorldY(0), 10.0f), "elevation=0 with offset=10 -> world_y=10");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Sync Behavior Tests
// ============================================================================

bool test_sync_single_entity() {
    printf("  test_sync_single_entity...\n");

    Registry registry;
    PositionSyncSystem system(registry);
    MockSimulationTime time;

    // Create entity with both components
    EntityID entity = registry.create();
    auto& pos = registry.emplace<PositionComponent>(entity);
    pos.pos.x = 10;
    pos.pos.y = 20;
    pos.elevation = 4;

    auto& transform = registry.emplace<TransformComponent>(entity);
    // Initial transform is at origin
    TEST_ASSERT(vec3_eq(transform.position, glm::vec3(0.0f)), "initial transform at origin");

    // Run sync
    system.tick(time);

    // Verify transform was updated
    TEST_ASSERT(float_eq(transform.position.x, 10.0f), "transform.x = grid_x");
    TEST_ASSERT(float_eq(transform.position.y, 1.0f), "transform.y = elevation * 0.25");
    TEST_ASSERT(float_eq(transform.position.z, 20.0f), "transform.z = grid_y");

    TEST_ASSERT_EQ(system.getLastSyncCount(), 1, "one entity synced");

    printf("  PASS\n");
    return true;
}

bool test_sync_multiple_entities() {
    printf("  test_sync_multiple_entities...\n");

    Registry registry;
    PositionSyncSystem system(registry);
    MockSimulationTime time;

    // Create multiple entities
    for (int i = 0; i < 10; ++i) {
        EntityID entity = registry.create();
        auto& pos = registry.emplace<PositionComponent>(entity);
        pos.pos.x = static_cast<std::int16_t>(i * 5);
        pos.pos.y = static_cast<std::int16_t>(i * 3);
        pos.elevation = static_cast<std::int16_t>(i);
        registry.emplace<TransformComponent>(entity);
    }

    // Run sync
    system.tick(time);

    TEST_ASSERT_EQ(system.getLastSyncCount(), 10, "ten entities synced");

    printf("  PASS\n");
    return true;
}

bool test_sync_only_entities_with_both_components() {
    printf("  test_sync_only_entities_with_both_components...\n");

    Registry registry;
    PositionSyncSystem system(registry);
    MockSimulationTime time;

    // Entity with only PositionComponent
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1);

    // Entity with only TransformComponent
    EntityID e2 = registry.create();
    registry.emplace<TransformComponent>(e2);

    // Entity with both
    EntityID e3 = registry.create();
    registry.emplace<PositionComponent>(e3);
    registry.emplace<TransformComponent>(e3);

    // Run sync
    system.tick(time);

    // Only one entity should be synced
    TEST_ASSERT_EQ(system.getLastSyncCount(), 1, "only entity with both components synced");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Model Matrix Recalculation Tests
// ============================================================================

bool test_model_matrix_recalculated_on_change() {
    printf("  test_model_matrix_recalculated_on_change...\n");

    Registry registry;
    PositionSyncSystem system(registry);
    MockSimulationTime time;

    EntityID entity = registry.create();
    auto& pos = registry.emplace<PositionComponent>(entity);
    pos.pos.x = 5;
    pos.pos.y = 10;
    pos.elevation = 8;  // 8 * 0.25 = 2.0

    auto& transform = registry.emplace<TransformComponent>(entity);

    // Initial sync
    system.tick(time);

    // Verify matrix was recalculated (dirty flag should be false now)
    TEST_ASSERT(transform.dirty == false, "dirty flag cleared after sync");

    // Check translation in model matrix
    TEST_ASSERT(float_eq(transform.model_matrix[3][0], 5.0f), "matrix translation x = 5");
    TEST_ASSERT(float_eq(transform.model_matrix[3][1], 2.0f), "matrix translation y = 2");
    TEST_ASSERT(float_eq(transform.model_matrix[3][2], 10.0f), "matrix translation z = 10");

    printf("  PASS\n");
    return true;
}

bool test_no_recalculation_when_unchanged() {
    printf("  test_no_recalculation_when_unchanged...\n");

    Registry registry;
    PositionSyncSystem system(registry);
    MockSimulationTime time;

    EntityID entity = registry.create();
    auto& pos = registry.emplace<PositionComponent>(entity);
    pos.pos.x = 5;
    pos.pos.y = 10;
    pos.elevation = 0;

    auto& transform = registry.emplace<TransformComponent>(entity);

    // First sync
    system.tick(time);
    TEST_ASSERT(transform.dirty == false, "dirty flag cleared after first sync");

    // Store the matrix
    glm::mat4 originalMatrix = transform.model_matrix;

    // Second sync without changing position
    system.tick(time);

    // Matrix should be the same (no recalculation)
    bool matricesEqual = true;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            if (!float_eq(transform.model_matrix[col][row], originalMatrix[col][row])) {
                matricesEqual = false;
                break;
            }
        }
    }
    TEST_ASSERT(matricesEqual, "matrix unchanged when position unchanged");

    printf("  PASS\n");
    return true;
}

bool test_recalculation_on_position_change() {
    printf("  test_recalculation_on_position_change...\n");

    Registry registry;
    PositionSyncSystem system(registry);
    MockSimulationTime time;

    EntityID entity = registry.create();
    auto& pos = registry.emplace<PositionComponent>(entity);
    pos.pos.x = 0;
    pos.pos.y = 0;
    pos.elevation = 0;

    auto& transform = registry.emplace<TransformComponent>(entity);

    // First sync
    system.tick(time);
    TEST_ASSERT(float_eq(transform.position.x, 0.0f), "initial x = 0");

    // Change position
    pos.pos.x = 100;

    // Second sync
    system.tick(time);
    TEST_ASSERT(float_eq(transform.position.x, 100.0f), "x updated to 100");
    TEST_ASSERT(float_eq(transform.model_matrix[3][0], 100.0f), "matrix translation x = 100");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// System Priority and Name Tests
// ============================================================================

bool test_system_priority() {
    printf("  test_system_priority...\n");

    Registry registry;
    PositionSyncSystem system(registry);

    // Priority should be 50 (runs early)
    TEST_ASSERT_EQ(system.getPriority(), 50, "priority is 50");

    printf("  PASS\n");
    return true;
}

bool test_system_name() {
    printf("  test_system_name...\n");

    Registry registry;
    PositionSyncSystem system(registry);

    const char* name = system.getName();
    TEST_ASSERT(name != nullptr, "name is not null");
    TEST_ASSERT(strcmp(name, "PositionSyncSystem") == 0, "name is PositionSyncSystem");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Edge Cases
// ============================================================================

bool test_negative_coordinates() {
    printf("  test_negative_coordinates...\n");

    Registry registry;
    PositionSyncSystem system(registry);
    MockSimulationTime time;

    EntityID entity = registry.create();
    auto& pos = registry.emplace<PositionComponent>(entity);
    pos.pos.x = -50;
    pos.pos.y = -100;
    pos.elevation = -4;  // Negative elevation

    auto& transform = registry.emplace<TransformComponent>(entity);

    system.tick(time);

    TEST_ASSERT(float_eq(transform.position.x, -50.0f), "negative grid_x -> negative world_x");
    TEST_ASSERT(float_eq(transform.position.z, -100.0f), "negative grid_y -> negative world_z");
    TEST_ASSERT(float_eq(transform.position.y, -1.0f), "negative elevation -> negative world_y");

    printf("  PASS\n");
    return true;
}

bool test_large_coordinates() {
    printf("  test_large_coordinates...\n");

    Registry registry;
    PositionSyncSystem system(registry);
    MockSimulationTime time;

    EntityID entity = registry.create();
    auto& pos = registry.emplace<PositionComponent>(entity);
    pos.pos.x = 32000;  // Near int16 max
    pos.pos.y = 32000;
    pos.elevation = 31;  // Max elevation per patterns.yaml

    auto& transform = registry.emplace<TransformComponent>(entity);

    system.tick(time);

    TEST_ASSERT(float_eq(transform.position.x, 32000.0f), "large grid_x");
    TEST_ASSERT(float_eq(transform.position.z, 32000.0f), "large grid_y");
    TEST_ASSERT(float_eq(transform.position.y, 7.75f), "max elevation (31 * 0.25)");

    printf("  PASS\n");
    return true;
}

bool test_empty_registry() {
    printf("  test_empty_registry...\n");

    Registry registry;
    PositionSyncSystem system(registry);
    MockSimulationTime time;

    // Should not crash with empty registry
    system.tick(time);

    TEST_ASSERT_EQ(system.getLastSyncCount(), 0, "zero entities synced");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== PositionSyncSystem Unit Tests (Ticket 2-033) ===\n\n");

    int passed = 0;
    int failed = 0;

    #define RUN_TEST(test) \
        do { \
            printf("Running %s\n", #test); \
            if (test()) { passed++; } else { failed++; } \
        } while(0)

    printf("--- Coordinate Mapping Tests ---\n");
    RUN_TEST(test_grid_x_to_world_x);
    RUN_TEST(test_grid_y_to_world_z);
    RUN_TEST(test_elevation_to_world_y);

    printf("\n--- Configuration Tests ---\n");
    RUN_TEST(test_configurable_grid_unit_size);
    RUN_TEST(test_configurable_elevation_step);
    RUN_TEST(test_set_config_at_runtime);
    RUN_TEST(test_config_with_offsets);

    printf("\n--- Sync Behavior Tests ---\n");
    RUN_TEST(test_sync_single_entity);
    RUN_TEST(test_sync_multiple_entities);
    RUN_TEST(test_sync_only_entities_with_both_components);

    printf("\n--- Model Matrix Recalculation Tests ---\n");
    RUN_TEST(test_model_matrix_recalculated_on_change);
    RUN_TEST(test_no_recalculation_when_unchanged);
    RUN_TEST(test_recalculation_on_position_change);

    printf("\n--- System Priority and Name Tests ---\n");
    RUN_TEST(test_system_priority);
    RUN_TEST(test_system_name);

    printf("\n--- Edge Cases ---\n");
    RUN_TEST(test_negative_coordinates);
    RUN_TEST(test_large_coordinates);
    RUN_TEST(test_empty_registry);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);

    if (failed > 0) {
        printf("\n=== TESTS FAILED ===\n");
        return 1;
    }

    printf("\n=== All tests passed! ===\n");
    return 0;
}
