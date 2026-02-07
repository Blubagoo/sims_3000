/**
 * @file test_transform_interpolation.cpp
 * @brief Unit tests for TransformInterpolationSystem (Ticket 2-044).
 *
 * Tests cover:
 * - Store previous and current tick transforms
 * - Calculate interpolation factor: t = time_since_tick / tick_duration
 * - Position: lerp(prev, curr, t)
 * - Rotation: slerp(prev, curr, t)
 * - Moving entities (beings) interpolate smoothly
 * - Static entities (buildings) use current state
 */

#include "sims3000/ecs/TransformInterpolationSystem.h"
#include "sims3000/ecs/InterpolatedTransformComponent.h"
#include "sims3000/ecs/Components.h"
#include "sims3000/ecs/Registry.h"
#include "sims3000/core/ISimulationTime.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>
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

bool quat_eq(const glm::quat& a, const glm::quat& b) {
    // Quaternions q and -q represent the same rotation, so check both
    bool directMatch = float_eq(a.w, b.w) && float_eq(a.x, b.x) &&
                       float_eq(a.y, b.y) && float_eq(a.z, b.z);
    bool negatedMatch = float_eq(a.w, -b.w) && float_eq(a.x, -b.x) &&
                        float_eq(a.y, -b.y) && float_eq(a.z, -b.z);
    return directMatch || negatedMatch;
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
// InterpolatedTransformComponent Tests
// ============================================================================

bool test_interpolated_transform_component_default() {
    printf("  test_interpolated_transform_component_default...\n");

    InterpolatedTransformComponent interp;

    TEST_ASSERT(vec3_eq(interp.previous_position, glm::vec3(0.0f)), "default previous_position is (0,0,0)");
    TEST_ASSERT(vec3_eq(interp.current_position, glm::vec3(0.0f)), "default current_position is (0,0,0)");
    TEST_ASSERT(quat_eq(interp.previous_rotation, glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), "default previous_rotation is identity");
    TEST_ASSERT(quat_eq(interp.current_rotation, glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), "default current_rotation is identity");

    printf("  PASS\n");
    return true;
}

bool test_interpolated_transform_component_rotate_tick() {
    printf("  test_interpolated_transform_component_rotate_tick...\n");

    InterpolatedTransformComponent interp;
    interp.current_position = glm::vec3(10.0f, 20.0f, 30.0f);
    interp.current_rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Rotate buffers: current -> previous
    interp.rotateTick();

    TEST_ASSERT(vec3_eq(interp.previous_position, glm::vec3(10.0f, 20.0f, 30.0f)), "previous_position copied from current");
    // Current should still be the same after rotate (not cleared)
    TEST_ASSERT(vec3_eq(interp.current_position, glm::vec3(10.0f, 20.0f, 30.0f)), "current_position unchanged");

    printf("  PASS\n");
    return true;
}

bool test_interpolated_transform_component_set_both() {
    printf("  test_interpolated_transform_component_set_both...\n");

    InterpolatedTransformComponent interp;
    interp.previous_position = glm::vec3(0.0f);
    interp.current_position = glm::vec3(100.0f);

    glm::vec3 newPos(50.0f, 60.0f, 70.0f);
    glm::quat newRot = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    interp.setBoth(newPos, newRot);

    TEST_ASSERT(vec3_eq(interp.previous_position, newPos), "previous_position set");
    TEST_ASSERT(vec3_eq(interp.current_position, newPos), "current_position set");
    TEST_ASSERT(quat_eq(interp.previous_rotation, newRot), "previous_rotation set");
    TEST_ASSERT(quat_eq(interp.current_rotation, newRot), "current_rotation set");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Position Interpolation Tests (lerp)
// ============================================================================

bool test_position_lerp_alpha_0() {
    printf("  test_position_lerp_alpha_0...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    // Create moving entity (no StaticEntityTag)
    EntityID entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

    interp.previous_position = glm::vec3(0.0f, 0.0f, 0.0f);
    interp.current_position = glm::vec3(100.0f, 200.0f, 300.0f);

    // Alpha = 0.0 -> use previous position
    time.setInterpolation(0.0f);
    system.interpolate(time);

    TEST_ASSERT(vec3_eq(transform.position, glm::vec3(0.0f, 0.0f, 0.0f)), "alpha=0 -> previous position");

    printf("  PASS\n");
    return true;
}

bool test_position_lerp_alpha_1() {
    printf("  test_position_lerp_alpha_1...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    EntityID entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

    interp.previous_position = glm::vec3(0.0f, 0.0f, 0.0f);
    interp.current_position = glm::vec3(100.0f, 200.0f, 300.0f);

    // Alpha = 1.0 -> use current position
    time.setInterpolation(1.0f);
    system.interpolate(time);

    TEST_ASSERT(vec3_eq(transform.position, glm::vec3(100.0f, 200.0f, 300.0f)), "alpha=1 -> current position");

    printf("  PASS\n");
    return true;
}

bool test_position_lerp_alpha_0_5() {
    printf("  test_position_lerp_alpha_0_5...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    EntityID entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

    interp.previous_position = glm::vec3(0.0f, 0.0f, 0.0f);
    interp.current_position = glm::vec3(100.0f, 200.0f, 300.0f);

    // Alpha = 0.5 -> halfway between previous and current
    time.setInterpolation(0.5f);
    system.interpolate(time);

    TEST_ASSERT(vec3_eq(transform.position, glm::vec3(50.0f, 100.0f, 150.0f)), "alpha=0.5 -> midpoint");

    printf("  PASS\n");
    return true;
}

bool test_position_lerp_alpha_0_25() {
    printf("  test_position_lerp_alpha_0_25...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    EntityID entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

    interp.previous_position = glm::vec3(0.0f, 0.0f, 0.0f);
    interp.current_position = glm::vec3(100.0f, 200.0f, 300.0f);

    // Alpha = 0.25 -> 25% of the way
    time.setInterpolation(0.25f);
    system.interpolate(time);

    TEST_ASSERT(vec3_eq(transform.position, glm::vec3(25.0f, 50.0f, 75.0f)), "alpha=0.25 -> quarter way");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Rotation Interpolation Tests (slerp)
// ============================================================================

bool test_rotation_slerp_alpha_0() {
    printf("  test_rotation_slerp_alpha_0...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    EntityID entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

    // Rotate from identity to 90 degrees around Y
    interp.previous_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity
    interp.current_rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Alpha = 0.0 -> use previous rotation
    time.setInterpolation(0.0f);
    system.interpolate(time);

    TEST_ASSERT(quat_eq(transform.rotation, interp.previous_rotation), "alpha=0 -> previous rotation");

    printf("  PASS\n");
    return true;
}

bool test_rotation_slerp_alpha_1() {
    printf("  test_rotation_slerp_alpha_1...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    EntityID entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

    interp.previous_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity
    interp.current_rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Alpha = 1.0 -> use current rotation
    time.setInterpolation(1.0f);
    system.interpolate(time);

    TEST_ASSERT(quat_eq(transform.rotation, interp.current_rotation), "alpha=1 -> current rotation");

    printf("  PASS\n");
    return true;
}

bool test_rotation_slerp_alpha_0_5() {
    printf("  test_rotation_slerp_alpha_0_5...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    EntityID entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

    // Rotate from identity to 90 degrees around Y
    interp.previous_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity
    interp.current_rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Alpha = 0.5 -> 45 degrees around Y
    time.setInterpolation(0.5f);
    system.interpolate(time);

    glm::quat expected = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    TEST_ASSERT(quat_eq(transform.rotation, expected), "alpha=0.5 -> 45 degree rotation");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Static Entity Tests (Buildings - use current state)
// ============================================================================

bool test_static_entity_not_interpolated() {
    printf("  test_static_entity_not_interpolated...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    // Create static entity (building) with StaticEntityTag
    EntityID entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    transform.position = glm::vec3(50.0f, 60.0f, 70.0f);  // Set initial position
    transform.rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // Use raw() for tag component (EnTT returns void for empty types)
    registry.raw().emplace<StaticEntityTag>(static_cast<entt::entity>(entity));

    // Store original values
    glm::vec3 originalPos = transform.position;
    glm::quat originalRot = transform.rotation;

    // Run interpolation with alpha = 0.5
    time.setInterpolation(0.5f);
    system.interpolate(time);

    // Static entity should NOT be modified
    TEST_ASSERT(vec3_eq(transform.position, originalPos), "static entity position unchanged");
    TEST_ASSERT(quat_eq(transform.rotation, originalRot), "static entity rotation unchanged");

    printf("  PASS\n");
    return true;
}

bool test_static_entity_counted() {
    printf("  test_static_entity_counted...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    // Create 3 static entities
    for (int i = 0; i < 3; ++i) {
        EntityID entity = registry.create();
        registry.emplace<TransformComponent>(entity);
        // Use raw() for tag component (EnTT returns void for empty types)
        registry.raw().emplace<StaticEntityTag>(static_cast<entt::entity>(entity));
    }

    // Create 2 moving entities
    for (int i = 0; i < 2; ++i) {
        EntityID entity = registry.create();
        registry.emplace<TransformComponent>(entity);
        registry.emplace<InterpolatedTransformComponent>(entity);
    }

    time.setInterpolation(0.5f);
    system.interpolate(time);

    TEST_ASSERT_EQ(system.getLastStaticCount(), 3, "3 static entities counted");
    TEST_ASSERT_EQ(system.getLastInterpolatedCount(), 2, "2 moving entities interpolated");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Moving Entity Tests (Beings - interpolate smoothly)
// ============================================================================

bool test_moving_entity_interpolated() {
    printf("  test_moving_entity_interpolated...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    // Create moving entity (being/vehicle)
    EntityID entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

    // Set up movement from (0,0,0) to (100,0,0)
    interp.previous_position = glm::vec3(0.0f, 0.0f, 0.0f);
    interp.current_position = glm::vec3(100.0f, 0.0f, 0.0f);

    // Test multiple alpha values
    float testAlphas[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    for (float alpha : testAlphas) {
        time.setInterpolation(alpha);
        system.interpolate(time);

        float expectedX = 100.0f * alpha;
        TEST_ASSERT(float_eq(transform.position.x, expectedX),
                    "moving entity position interpolated correctly");
    }

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Pre-Simulation Tick Tests
// ============================================================================

bool test_pre_simulation_tick_rotates_buffers() {
    printf("  test_pre_simulation_tick_rotates_buffers...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);

    EntityID entity = registry.create();
    auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

    // Set current position
    interp.current_position = glm::vec3(100.0f, 200.0f, 300.0f);
    interp.current_rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Previous should still be default
    TEST_ASSERT(vec3_eq(interp.previous_position, glm::vec3(0.0f)), "previous is default before rotate");

    // Call preSimulationTick to rotate buffers
    system.preSimulationTick();

    // Previous should now have the old current values
    TEST_ASSERT(vec3_eq(interp.previous_position, glm::vec3(100.0f, 200.0f, 300.0f)),
                "previous has current values after rotate");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Capture Current State Tests
// ============================================================================

bool test_capture_current_state() {
    printf("  test_capture_current_state...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);

    EntityID entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

    // Set transform values (as if simulation updated them)
    transform.position = glm::vec3(50.0f, 60.0f, 70.0f);
    transform.rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Capture should copy transform -> interp.current
    system.captureCurrentState();

    TEST_ASSERT(vec3_eq(interp.current_position, transform.position), "current_position captured");
    TEST_ASSERT(quat_eq(interp.current_rotation, transform.rotation), "current_rotation captured");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Model Matrix Recalculation Tests
// ============================================================================

bool test_model_matrix_recalculated_after_interpolation() {
    printf("  test_model_matrix_recalculated_after_interpolation...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    EntityID entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

    interp.previous_position = glm::vec3(0.0f, 0.0f, 0.0f);
    interp.current_position = glm::vec3(100.0f, 0.0f, 0.0f);

    time.setInterpolation(0.5f);
    system.interpolate(time);

    // Check that model matrix was updated (translation in last column)
    TEST_ASSERT(float_eq(transform.model_matrix[3][0], 50.0f), "model matrix translation x");
    TEST_ASSERT(transform.dirty == false, "dirty flag cleared after interpolation");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Full Simulation Workflow Test
// ============================================================================

bool test_full_simulation_workflow() {
    printf("  test_full_simulation_workflow...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    // Create moving entity
    EntityID entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

    // Initial state at position (0,0,0)
    transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    interp.setBoth(glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    // === Tick 1 ===
    // Step 1: Pre-tick (rotate buffers)
    system.preSimulationTick();

    // Step 2: Simulation updates transform to (10, 0, 0)
    transform.position = glm::vec3(10.0f, 0.0f, 0.0f);

    // Step 3: Capture current state
    system.captureCurrentState();

    // Step 4: Render at alpha = 0.5
    time.setInterpolation(0.5f);
    system.interpolate(time);

    // Should be at (5, 0, 0) - halfway between (0,0,0) and (10,0,0)
    TEST_ASSERT(vec3_eq(transform.position, glm::vec3(5.0f, 0.0f, 0.0f)),
                "tick 1: alpha=0.5 -> position (5,0,0)");

    // === Tick 2 ===
    // Step 1: Pre-tick (rotate buffers: current (10,0,0) -> previous)
    system.preSimulationTick();

    // Step 2: Simulation updates transform to (20, 0, 0)
    transform.position = glm::vec3(20.0f, 0.0f, 0.0f);

    // Step 3: Capture current state
    system.captureCurrentState();

    // Step 4: Render at alpha = 0.25
    time.setInterpolation(0.25f);
    system.interpolate(time);

    // previous = (10,0,0), current = (20,0,0)
    // alpha = 0.25 -> 10 + 0.25*(20-10) = 12.5
    TEST_ASSERT(vec3_eq(transform.position, glm::vec3(12.5f, 0.0f, 0.0f)),
                "tick 2: alpha=0.25 -> position (12.5,0,0)");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Component Size Tests
// ============================================================================

bool test_component_sizes() {
    printf("  test_component_sizes...\n");

    TEST_ASSERT_EQ(sizeof(InterpolatedTransformComponent), 56, "InterpolatedTransformComponent is 56 bytes");
    TEST_ASSERT_EQ(sizeof(StaticEntityTag), 1, "StaticEntityTag is 1 byte");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// System Name Test
// ============================================================================

bool test_system_name() {
    printf("  test_system_name...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);

    const char* name = system.getName();
    TEST_ASSERT(name != nullptr, "name is not null");
    TEST_ASSERT(strcmp(name, "TransformInterpolationSystem") == 0, "name is TransformInterpolationSystem");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Edge Cases
// ============================================================================

bool test_empty_registry() {
    printf("  test_empty_registry...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    // Should not crash with empty registry
    system.preSimulationTick();
    system.captureCurrentState();
    time.setInterpolation(0.5f);
    system.interpolate(time);

    TEST_ASSERT_EQ(system.getLastInterpolatedCount(), 0, "zero entities interpolated");
    TEST_ASSERT_EQ(system.getLastStaticCount(), 0, "zero static entities");

    printf("  PASS\n");
    return true;
}

bool test_multiple_moving_entities() {
    printf("  test_multiple_moving_entities...\n");

    Registry registry;
    TransformInterpolationSystem system(registry);
    MockSimulationTime time;

    // Create 10 moving entities
    for (int i = 0; i < 10; ++i) {
        EntityID entity = registry.create();
        auto& transform = registry.emplace<TransformComponent>(entity);
        auto& interp = registry.emplace<InterpolatedTransformComponent>(entity);

        interp.previous_position = glm::vec3(static_cast<float>(i), 0.0f, 0.0f);
        interp.current_position = glm::vec3(static_cast<float>(i + 10), 0.0f, 0.0f);
    }

    time.setInterpolation(0.5f);
    system.interpolate(time);

    TEST_ASSERT_EQ(system.getLastInterpolatedCount(), 10, "10 entities interpolated");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== TransformInterpolationSystem Unit Tests (Ticket 2-044) ===\n\n");

    int passed = 0;
    int failed = 0;

    #define RUN_TEST(test) \
        do { \
            printf("Running %s\n", #test); \
            if (test()) { passed++; } else { failed++; } \
        } while(0)

    printf("--- InterpolatedTransformComponent Tests ---\n");
    RUN_TEST(test_interpolated_transform_component_default);
    RUN_TEST(test_interpolated_transform_component_rotate_tick);
    RUN_TEST(test_interpolated_transform_component_set_both);

    printf("\n--- Position Interpolation Tests (lerp) ---\n");
    RUN_TEST(test_position_lerp_alpha_0);
    RUN_TEST(test_position_lerp_alpha_1);
    RUN_TEST(test_position_lerp_alpha_0_5);
    RUN_TEST(test_position_lerp_alpha_0_25);

    printf("\n--- Rotation Interpolation Tests (slerp) ---\n");
    RUN_TEST(test_rotation_slerp_alpha_0);
    RUN_TEST(test_rotation_slerp_alpha_1);
    RUN_TEST(test_rotation_slerp_alpha_0_5);

    printf("\n--- Static Entity Tests (Buildings - use current state) ---\n");
    RUN_TEST(test_static_entity_not_interpolated);
    RUN_TEST(test_static_entity_counted);

    printf("\n--- Moving Entity Tests (Beings - interpolate smoothly) ---\n");
    RUN_TEST(test_moving_entity_interpolated);

    printf("\n--- Pre-Simulation Tick Tests ---\n");
    RUN_TEST(test_pre_simulation_tick_rotates_buffers);

    printf("\n--- Capture Current State Tests ---\n");
    RUN_TEST(test_capture_current_state);

    printf("\n--- Model Matrix Recalculation Tests ---\n");
    RUN_TEST(test_model_matrix_recalculated_after_interpolation);

    printf("\n--- Full Simulation Workflow Test ---\n");
    RUN_TEST(test_full_simulation_workflow);

    printf("\n--- Component Size Tests ---\n");
    RUN_TEST(test_component_sizes);

    printf("\n--- System Name Test ---\n");
    RUN_TEST(test_system_name);

    printf("\n--- Edge Cases ---\n");
    RUN_TEST(test_empty_registry);
    RUN_TEST(test_multiple_moving_entities);

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
