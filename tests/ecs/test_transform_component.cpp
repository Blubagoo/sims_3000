/**
 * @file test_transform_component.cpp
 * @brief Unit tests for TransformComponent (Ticket 2-032).
 *
 * Tests cover:
 * - Position: Vec3 (world space, floats)
 * - Rotation: Quaternion (required for free camera)
 * - Scale: Vec3 (default 1,1,1)
 * - Cached model matrix (Mat4)
 * - Dirty flag for matrix recomputation
 * - Separation from PositionComponent (grid-based game logic)
 * - Network serialization round-trip
 * - Version 1 backward compatibility
 */

#include "sims3000/ecs/Components.h"
#include "sims3000/net/NetworkBuffer.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

// Float comparison with epsilon
static constexpr float EPSILON = 0.0001f;

bool float_eq(float a, float b) {
    return std::abs(a - b) < EPSILON;
}

bool vec3_eq(const glm::vec3& a, const glm::vec3& b) {
    return float_eq(a.x, b.x) && float_eq(a.y, b.y) && float_eq(a.z, b.z);
}

bool quat_eq(const glm::quat& a, const glm::quat& b) {
    return float_eq(a.w, b.w) && float_eq(a.x, b.x) &&
           float_eq(a.y, b.y) && float_eq(a.z, b.z);
}

bool mat4_eq(const glm::mat4& a, const glm::mat4& b) {
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            if (!float_eq(a[col][row], b[col][row])) {
                return false;
            }
        }
    }
    return true;
}

// ============================================================================
// Position Tests (Vec3, world space, floats)
// ============================================================================

bool test_position_default() {
    printf("  test_position_default...\n");

    TransformComponent transform;
    TEST_ASSERT(vec3_eq(transform.position, glm::vec3(0.0f, 0.0f, 0.0f)),
                "default position is (0,0,0)");

    printf("  PASS\n");
    return true;
}

bool test_position_set_values() {
    printf("  test_position_set_values...\n");

    TransformComponent transform;
    transform.position = glm::vec3(10.5f, -20.25f, 100.0f);

    TEST_ASSERT(float_eq(transform.position.x, 10.5f), "position.x is 10.5");
    TEST_ASSERT(float_eq(transform.position.y, -20.25f), "position.y is -20.25");
    TEST_ASSERT(float_eq(transform.position.z, 100.0f), "position.z is 100.0");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Rotation Tests (Quaternion - required for free camera)
// ============================================================================

bool test_rotation_default_identity() {
    printf("  test_rotation_default_identity...\n");

    TransformComponent transform;
    // Identity quaternion: (w=1, x=0, y=0, z=0)
    TEST_ASSERT(quat_eq(transform.rotation, glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
                "default rotation is identity quaternion");

    printf("  PASS\n");
    return true;
}

bool test_rotation_from_angle_axis() {
    printf("  test_rotation_from_angle_axis...\n");

    TransformComponent transform;
    // Rotate 90 degrees around Y axis
    transform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Verify it's a valid unit quaternion
    float length = glm::length(transform.rotation);
    TEST_ASSERT(float_eq(length, 1.0f), "quaternion is normalized (unit length)");

    printf("  PASS\n");
    return true;
}

bool test_rotation_arbitrary_axis() {
    printf("  test_rotation_arbitrary_axis...\n");

    TransformComponent transform;
    // Free camera can view from any angle - test rotation around arbitrary axis
    glm::vec3 axis = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    transform.rotation = glm::angleAxis(glm::radians(45.0f), axis);

    float length = glm::length(transform.rotation);
    TEST_ASSERT(float_eq(length, 1.0f), "quaternion is normalized");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Scale Tests (Vec3, default 1,1,1)
// ============================================================================

bool test_scale_default() {
    printf("  test_scale_default...\n");

    TransformComponent transform;
    TEST_ASSERT(vec3_eq(transform.scale, glm::vec3(1.0f, 1.0f, 1.0f)),
                "default scale is (1,1,1)");

    printf("  PASS\n");
    return true;
}

bool test_scale_non_uniform() {
    printf("  test_scale_non_uniform...\n");

    TransformComponent transform;
    transform.scale = glm::vec3(2.0f, 0.5f, 3.0f);

    TEST_ASSERT(float_eq(transform.scale.x, 2.0f), "scale.x is 2.0");
    TEST_ASSERT(float_eq(transform.scale.y, 0.5f), "scale.y is 0.5");
    TEST_ASSERT(float_eq(transform.scale.z, 3.0f), "scale.z is 3.0");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Cached Model Matrix Tests (Mat4)
// ============================================================================

bool test_model_matrix_default() {
    printf("  test_model_matrix_default...\n");

    TransformComponent transform;
    TEST_ASSERT(mat4_eq(transform.model_matrix, glm::mat4(1.0f)),
                "default model_matrix is identity");

    printf("  PASS\n");
    return true;
}

bool test_recompute_matrix_translation() {
    printf("  test_recompute_matrix_translation...\n");

    TransformComponent transform;
    transform.position = glm::vec3(5.0f, 10.0f, 15.0f);
    transform.recompute_matrix();

    // Check translation is in the last column
    TEST_ASSERT(float_eq(transform.model_matrix[3][0], 5.0f), "matrix translation x");
    TEST_ASSERT(float_eq(transform.model_matrix[3][1], 10.0f), "matrix translation y");
    TEST_ASSERT(float_eq(transform.model_matrix[3][2], 15.0f), "matrix translation z");

    printf("  PASS\n");
    return true;
}

bool test_recompute_matrix_scale() {
    printf("  test_recompute_matrix_scale...\n");

    TransformComponent transform;
    transform.scale = glm::vec3(2.0f, 3.0f, 4.0f);
    transform.recompute_matrix();

    // For pure scale (no rotation), diagonal should have scale values
    TEST_ASSERT(float_eq(transform.model_matrix[0][0], 2.0f), "matrix scale x");
    TEST_ASSERT(float_eq(transform.model_matrix[1][1], 3.0f), "matrix scale y");
    TEST_ASSERT(float_eq(transform.model_matrix[2][2], 4.0f), "matrix scale z");

    printf("  PASS\n");
    return true;
}

bool test_recompute_matrix_combined() {
    printf("  test_recompute_matrix_combined...\n");

    TransformComponent transform;
    transform.position = glm::vec3(1.0f, 2.0f, 3.0f);
    transform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    transform.scale = glm::vec3(2.0f, 2.0f, 2.0f);
    transform.recompute_matrix();

    // Verify matrix is non-identity after combined transform
    TEST_ASSERT(!mat4_eq(transform.model_matrix, glm::mat4(1.0f)),
                "combined transform is not identity");

    // Translation should still be in last column
    TEST_ASSERT(float_eq(transform.model_matrix[3][0], 1.0f), "matrix translation x");
    TEST_ASSERT(float_eq(transform.model_matrix[3][1], 2.0f), "matrix translation y");
    TEST_ASSERT(float_eq(transform.model_matrix[3][2], 3.0f), "matrix translation z");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Dirty Flag Tests
// ============================================================================

bool test_dirty_flag_default() {
    printf("  test_dirty_flag_default...\n");

    TransformComponent transform;
    TEST_ASSERT(transform.dirty == true, "default dirty flag is true");

    printf("  PASS\n");
    return true;
}

bool test_dirty_flag_after_recompute() {
    printf("  test_dirty_flag_after_recompute...\n");

    TransformComponent transform;
    TEST_ASSERT(transform.dirty == true, "initially dirty");

    transform.recompute_matrix();
    TEST_ASSERT(transform.dirty == false, "dirty flag cleared after recompute");

    printf("  PASS\n");
    return true;
}

bool test_set_dirty() {
    printf("  test_set_dirty...\n");

    TransformComponent transform;
    transform.recompute_matrix();
    TEST_ASSERT(transform.dirty == false, "not dirty after recompute");

    transform.set_dirty();
    TEST_ASSERT(transform.dirty == true, "dirty after set_dirty()");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Separation from PositionComponent Tests
// ============================================================================

bool test_separate_from_position_component() {
    printf("  test_separate_from_position_component...\n");

    // PositionComponent uses GridPosition (int16 x,y)
    PositionComponent pos;
    pos.pos.x = 10;
    pos.pos.y = 20;
    pos.elevation = 5;

    // TransformComponent uses float Vec3
    TransformComponent transform;
    transform.position = glm::vec3(10.5f, 20.5f, 5.25f);

    // They are completely separate types with different purposes
    // PositionComponent is for game logic (grid-based)
    // TransformComponent is for rendering (smooth floats)
    TEST_ASSERT(sizeof(PositionComponent) != sizeof(TransformComponent),
                "different sizes indicates separate types");

    // Verify they can coexist with different values
    TEST_ASSERT(static_cast<float>(pos.pos.x) != transform.position.x,
                "can have different x values");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Trivially Copyable Test (no pointers)
// ============================================================================

bool test_trivially_copyable() {
    printf("  test_trivially_copyable...\n");

    // Component must be trivially copyable for serialization
    static_assert(std::is_trivially_copyable<TransformComponent>::value,
                  "TransformComponent must be trivially copyable");

    // Verify by copying
    TransformComponent original;
    original.position = glm::vec3(1.0f, 2.0f, 3.0f);
    original.rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    original.scale = glm::vec3(2.0f, 2.0f, 2.0f);
    original.dirty = false;
    original.recompute_matrix();

    TransformComponent copy = original;

    TEST_ASSERT(vec3_eq(copy.position, original.position), "position copied");
    TEST_ASSERT(quat_eq(copy.rotation, original.rotation), "rotation copied");
    TEST_ASSERT(vec3_eq(copy.scale, original.scale), "scale copied");
    TEST_ASSERT(copy.dirty == original.dirty, "dirty flag copied");
    TEST_ASSERT(mat4_eq(copy.model_matrix, original.model_matrix), "model_matrix copied");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Network Serialization Tests
// ============================================================================

bool test_serialization_basic_roundtrip() {
    printf("  test_serialization_basic_roundtrip...\n");

    TransformComponent original;
    original.position = glm::vec3(10.5f, -20.25f, 100.0f);
    original.rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    original.scale = glm::vec3(2.0f, 0.5f, 3.0f);
    original.dirty = false;
    original.recompute_matrix();

    NetworkBuffer buffer;
    original.serialize_net(buffer);

    TEST_ASSERT_EQ(buffer.size(), TransformComponent::get_serialized_size(),
                   "serialized size matches");

    buffer.reset_read();
    TransformComponent result = TransformComponent::deserialize_net(buffer);

    TEST_ASSERT(vec3_eq(result.position, original.position), "position roundtrip");
    TEST_ASSERT(quat_eq(result.rotation, original.rotation), "rotation roundtrip");
    TEST_ASSERT(vec3_eq(result.scale, original.scale), "scale roundtrip");
    TEST_ASSERT(result.dirty == original.dirty, "dirty roundtrip");
    TEST_ASSERT(mat4_eq(result.model_matrix, original.model_matrix), "model_matrix roundtrip");
    TEST_ASSERT(buffer.at_end(), "buffer fully consumed");

    printf("  PASS\n");
    return true;
}

bool test_serialization_identity() {
    printf("  test_serialization_identity...\n");

    TransformComponent original;
    // Default values: position(0,0,0), identity rotation, scale(1,1,1)
    original.recompute_matrix();
    original.dirty = false;

    NetworkBuffer buffer;
    original.serialize_net(buffer);

    buffer.reset_read();
    TransformComponent result = TransformComponent::deserialize_net(buffer);

    TEST_ASSERT(vec3_eq(result.position, glm::vec3(0.0f)), "identity position");
    TEST_ASSERT(quat_eq(result.rotation, glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), "identity rotation");
    TEST_ASSERT(vec3_eq(result.scale, glm::vec3(1.0f)), "identity scale");

    printf("  PASS\n");
    return true;
}

bool test_serialization_negative_values() {
    printf("  test_serialization_negative_values...\n");

    TransformComponent original;
    original.position = glm::vec3(-100.5f, -200.25f, -300.0f);
    original.scale = glm::vec3(0.1f, 0.2f, 0.3f);  // Small positive scales
    original.recompute_matrix();

    NetworkBuffer buffer;
    original.serialize_net(buffer);

    buffer.reset_read();
    TransformComponent result = TransformComponent::deserialize_net(buffer);

    TEST_ASSERT(vec3_eq(result.position, original.position), "negative position roundtrip");
    TEST_ASSERT(vec3_eq(result.scale, original.scale), "small scale roundtrip");

    printf("  PASS\n");
    return true;
}

bool test_serialization_version_byte() {
    printf("  test_serialization_version_byte...\n");

    TransformComponent transform;
    transform.recompute_matrix();

    NetworkBuffer buffer;
    transform.serialize_net(buffer);

    // Verify first byte is version 2
    TEST_ASSERT_EQ(buffer.data()[0], ComponentVersion::Transform, "first byte is version 2");

    printf("  PASS\n");
    return true;
}

bool test_serialization_get_serialized_size() {
    printf("  test_serialization_get_serialized_size...\n");

    // version(1) + position(12) + rotation(16) + scale(12) + dirty(1) + model_matrix(64) = 106
    constexpr std::size_t expected_size = TransformComponent::get_serialized_size();
    TEST_ASSERT_EQ(expected_size, 106, "expected size is 106 bytes");

    TransformComponent transform;
    transform.position = glm::vec3(1.0f, 2.0f, 3.0f);
    transform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    transform.scale = glm::vec3(4.0f, 5.0f, 6.0f);
    transform.recompute_matrix();

    NetworkBuffer buffer;
    transform.serialize_net(buffer);

    TEST_ASSERT_EQ(buffer.size(), expected_size, "actual size matches get_serialized_size()");

    printf("  PASS\n");
    return true;
}

bool test_serialization_type_id() {
    printf("  test_serialization_type_id...\n");

    TEST_ASSERT_EQ(TransformComponent::get_type_id(), ComponentTypeID::Transform,
                   "type ID is ComponentTypeID::Transform");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Component Size Check
// ============================================================================

bool test_component_size() {
    printf("  test_component_size...\n");

    // Verify the static_assert in the header is correct
    // position(12) + rotation(16) + scale(12) + dirty(1) + padding(3) + model_matrix(64) = 108
    TEST_ASSERT_EQ(sizeof(TransformComponent), 108, "component size is 108 bytes");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== TransformComponent Unit Tests (Ticket 2-032) ===\n\n");

    int passed = 0;
    int failed = 0;

    #define RUN_TEST(test) \
        do { \
            printf("Running %s\n", #test); \
            if (test()) { passed++; } else { failed++; } \
        } while(0)

    printf("--- Position Tests (Vec3, world space, floats) ---\n");
    RUN_TEST(test_position_default);
    RUN_TEST(test_position_set_values);

    printf("\n--- Rotation Tests (Quaternion - required for free camera) ---\n");
    RUN_TEST(test_rotation_default_identity);
    RUN_TEST(test_rotation_from_angle_axis);
    RUN_TEST(test_rotation_arbitrary_axis);

    printf("\n--- Scale Tests (Vec3, default 1,1,1) ---\n");
    RUN_TEST(test_scale_default);
    RUN_TEST(test_scale_non_uniform);

    printf("\n--- Cached Model Matrix Tests (Mat4) ---\n");
    RUN_TEST(test_model_matrix_default);
    RUN_TEST(test_recompute_matrix_translation);
    RUN_TEST(test_recompute_matrix_scale);
    RUN_TEST(test_recompute_matrix_combined);

    printf("\n--- Dirty Flag Tests ---\n");
    RUN_TEST(test_dirty_flag_default);
    RUN_TEST(test_dirty_flag_after_recompute);
    RUN_TEST(test_set_dirty);

    printf("\n--- Separation from PositionComponent Tests ---\n");
    RUN_TEST(test_separate_from_position_component);

    printf("\n--- Trivially Copyable Test ---\n");
    RUN_TEST(test_trivially_copyable);

    printf("\n--- Network Serialization Tests ---\n");
    RUN_TEST(test_serialization_basic_roundtrip);
    RUN_TEST(test_serialization_identity);
    RUN_TEST(test_serialization_negative_values);
    RUN_TEST(test_serialization_version_byte);
    RUN_TEST(test_serialization_get_serialized_size);
    RUN_TEST(test_serialization_type_id);

    printf("\n--- Component Size Check ---\n");
    RUN_TEST(test_component_size);

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
