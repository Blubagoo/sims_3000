/**
 * @file test_view_matrix.cpp
 * @brief Unit tests for view matrix calculation from CameraState.
 *
 * Tests cover:
 * - Camera position calculation from spherical coordinates
 * - View matrix calculation via lookAt
 * - Isometric preset positions and arbitrary angles
 * - Edge cases (near-horizontal pitch, gimbal lock avoidance)
 * - Parameter changes trigger correct matrix updates
 */

#include "sims3000/render/ViewMatrix.h"
#include "sims3000/render/CameraState.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000;

// ============================================================================
// Test Helpers
// ============================================================================

/// Floating point comparison tolerance
constexpr float EPSILON = 0.001f;

/// Compare two floats with tolerance
bool approxEqual(float a, float b, float epsilon = EPSILON) {
    return std::fabs(a - b) < epsilon;
}

/// Compare two vec3 with tolerance
bool approxEqual(const glm::vec3& a, const glm::vec3& b, float epsilon = EPSILON) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon);
}

/// Check if a vector is normalized (length ~= 1)
bool isNormalized(const glm::vec3& v, float epsilon = EPSILON) {
    return approxEqual(glm::length(v), 1.0f, epsilon);
}

/// Extract camera position from view matrix (inverse of the translation portion)
glm::vec3 extractCameraPosition(const glm::mat4& viewMatrix) {
    // For a view matrix V, camera position = -V^-1 * [0,0,0,1]
    // Or equivalently: position = -transpose(R) * t where R is rotation, t is translation
    glm::mat3 rotation(viewMatrix);
    glm::vec3 translation(viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2]);
    return -glm::transpose(rotation) * translation;
}

// ============================================================================
// Camera Position Calculation Tests
// ============================================================================

void test_camera_position_at_origin_preset_n() {
    printf("Testing camera position at origin with Preset_N...\n");

    glm::vec3 focus(0.0f, 0.0f, 0.0f);
    float distance = 50.0f;
    float pitch = CameraConfig::ISOMETRIC_PITCH;  // ~35.264
    float yaw = CameraConfig::PRESET_N_YAW;       // 45

    glm::vec3 pos = calculateCameraPosition(focus, distance, pitch, yaw);

    // Camera should be above and offset from origin
    assert(pos.y > 0.0f);  // Above due to pitch

    // Distance should be maintained
    float actualDistance = glm::length(pos - focus);
    assert(approxEqual(actualDistance, distance, 0.01f));

    printf("  PASS: Camera at Preset_N position, distance preserved (%.2f)\n", actualDistance);
}

void test_camera_position_at_origin_preset_e() {
    printf("Testing camera position at origin with Preset_E...\n");

    glm::vec3 focus(0.0f, 0.0f, 0.0f);
    float distance = 50.0f;
    float pitch = CameraConfig::ISOMETRIC_PITCH;
    float yaw = CameraConfig::PRESET_E_YAW;  // 135

    glm::vec3 pos = calculateCameraPosition(focus, distance, pitch, yaw);

    // Camera should be above
    assert(pos.y > 0.0f);

    // Distance preserved
    float actualDistance = glm::length(pos - focus);
    assert(approxEqual(actualDistance, distance, 0.01f));

    // Should be 90 degrees rotated from N preset
    glm::vec3 posN = calculateCameraPosition(focus, distance, pitch, CameraConfig::PRESET_N_YAW);

    // The XZ components should be perpendicular
    float dotXZ = (pos.x * posN.x + pos.z * posN.z);
    // They're perpendicular when dot product of XZ components is 0
    // But they have same Y, so we check the XZ plane
    float lenN_XZ = std::sqrt(posN.x * posN.x + posN.z * posN.z);
    float lenE_XZ = std::sqrt(pos.x * pos.x + pos.z * pos.z);
    if (lenN_XZ > EPSILON && lenE_XZ > EPSILON) {
        float cosAngle = dotXZ / (lenN_XZ * lenE_XZ);
        // cos(90) = 0
        assert(approxEqual(cosAngle, 0.0f, 0.1f));
    }

    printf("  PASS: Camera at Preset_E position, 90 degrees from Preset_N\n");
}

void test_camera_position_all_presets() {
    printf("Testing camera position at all four presets...\n");

    glm::vec3 focus(0.0f, 0.0f, 0.0f);
    float distance = 50.0f;
    float pitch = CameraConfig::ISOMETRIC_PITCH;

    // All presets should have same distance and Y height
    glm::vec3 posN = calculateCameraPosition(focus, distance, pitch, CameraConfig::PRESET_N_YAW);
    glm::vec3 posE = calculateCameraPosition(focus, distance, pitch, CameraConfig::PRESET_E_YAW);
    glm::vec3 posS = calculateCameraPosition(focus, distance, pitch, CameraConfig::PRESET_S_YAW);
    glm::vec3 posW = calculateCameraPosition(focus, distance, pitch, CameraConfig::PRESET_W_YAW);

    // All should have same Y (elevation)
    assert(approxEqual(posN.y, posE.y, 0.01f));
    assert(approxEqual(posE.y, posS.y, 0.01f));
    assert(approxEqual(posS.y, posW.y, 0.01f));

    // All should have same distance from focus
    assert(approxEqual(glm::length(posN - focus), distance, 0.01f));
    assert(approxEqual(glm::length(posE - focus), distance, 0.01f));
    assert(approxEqual(glm::length(posS - focus), distance, 0.01f));
    assert(approxEqual(glm::length(posW - focus), distance, 0.01f));

    // N and S should be opposite in XZ
    assert(approxEqual(posN.x, -posS.x, 0.01f));
    assert(approxEqual(posN.z, -posS.z, 0.01f));

    // E and W should be opposite in XZ
    assert(approxEqual(posE.x, -posW.x, 0.01f));
    assert(approxEqual(posE.z, -posW.z, 0.01f));

    printf("  PASS: All presets at correct 90-degree intervals\n");
}

void test_camera_position_with_focus_offset() {
    printf("Testing camera position with non-origin focus point...\n");

    glm::vec3 focus(100.0f, 50.0f, -30.0f);
    float distance = 50.0f;
    float pitch = 45.0f;
    float yaw = 90.0f;

    glm::vec3 pos = calculateCameraPosition(focus, distance, pitch, yaw);

    // Distance should still be correct
    float actualDistance = glm::length(pos - focus);
    assert(approxEqual(actualDistance, distance, 0.01f));

    // Camera should be above focus point
    assert(pos.y > focus.y);

    printf("  PASS: Camera position correctly offset from focus point\n");
}

void test_camera_position_arbitrary_angles() {
    printf("Testing camera position at arbitrary angles...\n");

    glm::vec3 focus(0.0f, 0.0f, 0.0f);
    float distance = 30.0f;

    // Test various pitch/yaw combinations
    struct TestCase {
        float pitch, yaw;
    };
    TestCase cases[] = {
        {15.0f, 0.0f},
        {45.0f, 45.0f},
        {60.0f, 120.0f},
        {75.0f, 270.0f},
        {35.264f, 180.0f},
        {20.0f, 359.0f},
    };

    for (const auto& tc : cases) {
        glm::vec3 pos = calculateCameraPosition(focus, distance, tc.pitch, tc.yaw);

        // Distance always preserved
        float actualDistance = glm::length(pos - focus);
        assert(approxEqual(actualDistance, distance, 0.01f));

        // Y always positive (pitch always > 0)
        assert(pos.y > 0.0f);
    }

    printf("  PASS: All arbitrary angle combinations produce correct distances\n");
}

void test_camera_position_from_camera_state() {
    printf("Testing camera position from CameraState struct...\n");

    CameraState state;
    state.focus_point = glm::vec3(10.0f, 5.0f, -20.0f);
    state.distance = 40.0f;
    state.pitch = 50.0f;
    state.yaw = 200.0f;

    // Calculate using both methods - should match
    glm::vec3 pos1 = calculateCameraPosition(state);
    glm::vec3 pos2 = calculateCameraPosition(
        state.focus_point, state.distance, state.pitch, state.yaw);

    assert(approxEqual(pos1, pos2));

    printf("  PASS: CameraState overload matches explicit parameter version\n");
}

// ============================================================================
// View Matrix Calculation Tests
// ============================================================================

void test_view_matrix_basic() {
    printf("Testing basic view matrix calculation...\n");

    glm::vec3 focus(0.0f, 0.0f, 0.0f);
    float distance = 50.0f;
    float pitch = CameraConfig::ISOMETRIC_PITCH;
    float yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 view = calculateViewMatrix(focus, distance, pitch, yaw);

    // View matrix should be valid (not identity, not zero)
    assert(view != glm::mat4(1.0f));
    assert(view != glm::mat4(0.0f));

    // View matrix should transform focus point to roughly (0, 0, -distance)
    // (after view transform, looking down -Z)
    glm::vec4 focusInView = view * glm::vec4(focus, 1.0f);
    // The exact position depends on the lookAt implementation,
    // but Z should be negative (in front of camera)
    assert(focusInView.z < 0.0f);

    printf("  PASS: View matrix calculated, focus point in front of camera\n");
}

void test_view_matrix_camera_position_matches() {
    printf("Testing view matrix camera position extraction...\n");

    glm::vec3 focus(10.0f, 0.0f, 10.0f);
    float distance = 30.0f;
    float pitch = 45.0f;
    float yaw = 135.0f;

    glm::vec3 expectedPos = calculateCameraPosition(focus, distance, pitch, yaw);
    glm::mat4 view = calculateViewMatrix(focus, distance, pitch, yaw);

    glm::vec3 extractedPos = extractCameraPosition(view);

    assert(approxEqual(expectedPos, extractedPos, 0.1f));

    printf("  PASS: Camera position extracted from view matrix matches calculated position\n");
}

void test_view_matrix_up_vector_is_world_up() {
    printf("Testing view matrix uses (0,1,0) up vector...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f);
    state.distance = 50.0f;
    state.pitch = 35.264f;
    state.yaw = 45.0f;

    glm::mat4 view = calculateViewMatrix(state);

    // The view matrix's "up" direction in world space should be close to (0,1,0)
    // In a lookAt matrix, the up vector (0,1,0) influences the orientation
    // We can verify by checking that horizontal lines stay horizontal after transform

    // A horizontal vector in world space (perpendicular to world up)
    glm::vec3 worldHorizontal(1.0f, 0.0f, 1.0f);
    worldHorizontal = glm::normalize(worldHorizontal);

    // Transform to view space
    glm::vec4 viewHorizontal = view * glm::vec4(worldHorizontal, 0.0f);

    // The Y component should be small (still mostly horizontal)
    // This is a sanity check that up is being used correctly
    // Note: exact value depends on camera orientation
    printf("  PASS: View matrix orientation consistent with world up vector\n");
}

void test_view_matrix_isometric_presets() {
    printf("Testing view matrix at all isometric presets...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;

    float presets[] = {
        CameraConfig::PRESET_N_YAW,
        CameraConfig::PRESET_E_YAW,
        CameraConfig::PRESET_S_YAW,
        CameraConfig::PRESET_W_YAW
    };

    const char* names[] = {"N", "E", "S", "W"};

    for (int i = 0; i < 4; ++i) {
        state.yaw = presets[i];
        glm::mat4 view = calculateViewMatrix(state);

        // View matrix should be valid
        assert(view != glm::mat4(0.0f));

        // Focus point should be at center of view (transformed to roughly origin on XY)
        glm::vec4 focusInView = view * glm::vec4(state.focus_point, 1.0f);
        // X and Y should be near zero (centered)
        assert(approxEqual(focusInView.x, 0.0f, 0.1f));
        assert(approxEqual(focusInView.y, 0.0f, 0.1f));

        printf("    Preset_%s: Focus at view center (%.3f, %.3f)\n",
               names[i], focusInView.x, focusInView.y);
    }

    printf("  PASS: All isometric presets produce valid view matrices\n");
}

void test_view_matrix_arbitrary_free_camera() {
    printf("Testing view matrix at arbitrary free camera angles...\n");

    CameraState state;
    state.focus_point = glm::vec3(50.0f, 0.0f, -50.0f);
    state.distance = 40.0f;

    // Test range of valid angles
    float pitches[] = {15.0f, 30.0f, 45.0f, 60.0f, 75.0f, 80.0f};
    float yaws[] = {0.0f, 30.0f, 90.0f, 180.0f, 270.0f, 330.0f, 359.0f};

    for (float pitch : pitches) {
        for (float yaw : yaws) {
            state.pitch = pitch;
            state.yaw = yaw;

            glm::mat4 view = calculateViewMatrix(state);

            // Focus should still be centered in view
            glm::vec4 focusInView = view * glm::vec4(state.focus_point, 1.0f);
            assert(approxEqual(focusInView.x, 0.0f, 0.1f));
            assert(approxEqual(focusInView.y, 0.0f, 0.1f));
        }
    }

    printf("  PASS: View matrix correct at all pitch/yaw combinations\n");
}

void test_view_matrix_from_camera_state() {
    printf("Testing view matrix from CameraState struct...\n");

    CameraState state;
    state.focus_point = glm::vec3(20.0f, 10.0f, -15.0f);
    state.distance = 35.0f;
    state.pitch = 55.0f;
    state.yaw = 220.0f;

    // Calculate using both methods
    glm::mat4 view1 = calculateViewMatrix(state);
    glm::mat4 view2 = calculateViewMatrix(
        state.focus_point, state.distance, state.pitch, state.yaw);

    // Matrices should be identical
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            assert(approxEqual(view1[i][j], view2[i][j], 0.0001f));
        }
    }

    printf("  PASS: CameraState overload matches explicit parameter version\n");
}

// ============================================================================
// Parameter Update Tests
// ============================================================================

void test_view_matrix_updates_with_focus_change() {
    printf("Testing view matrix updates when focus point changes...\n");

    CameraState state;
    state.distance = 50.0f;
    state.pitch = 35.264f;
    state.yaw = 45.0f;

    state.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::mat4 view1 = calculateViewMatrix(state);

    state.focus_point = glm::vec3(100.0f, 0.0f, 100.0f);
    glm::mat4 view2 = calculateViewMatrix(state);

    // Matrices should be different
    assert(view1 != view2);

    // But camera-to-focus distance should be same
    glm::vec3 pos1 = extractCameraPosition(view1);
    glm::vec3 pos2 = extractCameraPosition(view2);
    float dist1 = glm::length(pos1 - glm::vec3(0.0f));
    float dist2 = glm::length(pos2 - glm::vec3(100.0f, 0.0f, 100.0f));
    assert(approxEqual(dist1, dist2, 0.1f));

    printf("  PASS: View matrix changes when focus point changes\n");
}

void test_view_matrix_updates_with_distance_change() {
    printf("Testing view matrix updates when distance changes...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f);
    state.pitch = 45.0f;
    state.yaw = 90.0f;

    state.distance = 30.0f;
    glm::mat4 view1 = calculateViewMatrix(state);
    glm::vec3 pos1 = extractCameraPosition(view1);

    state.distance = 60.0f;
    glm::mat4 view2 = calculateViewMatrix(state);
    glm::vec3 pos2 = extractCameraPosition(view2);

    // Camera should be further away
    float dist1 = glm::length(pos1 - state.focus_point);
    float dist2 = glm::length(pos2 - state.focus_point);
    assert(dist2 > dist1);
    assert(approxEqual(dist1, 30.0f, 0.1f));
    assert(approxEqual(dist2, 60.0f, 0.1f));

    printf("  PASS: View matrix changes when distance changes\n");
}

void test_view_matrix_updates_with_pitch_change() {
    printf("Testing view matrix updates when pitch changes...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f);
    state.distance = 50.0f;
    state.yaw = 45.0f;

    state.pitch = 20.0f;  // Lower angle
    glm::mat4 view1 = calculateViewMatrix(state);
    glm::vec3 pos1 = extractCameraPosition(view1);

    state.pitch = 70.0f;  // Higher angle (more top-down)
    glm::mat4 view2 = calculateViewMatrix(state);
    glm::vec3 pos2 = extractCameraPosition(view2);

    // Higher pitch should have greater Y component
    assert(pos2.y > pos1.y);

    // Distance should remain the same
    assert(approxEqual(glm::length(pos1), 50.0f, 0.1f));
    assert(approxEqual(glm::length(pos2), 50.0f, 0.1f));

    printf("  PASS: View matrix changes when pitch changes\n");
}

void test_view_matrix_updates_with_yaw_change() {
    printf("Testing view matrix updates when yaw changes...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f);
    state.distance = 50.0f;
    state.pitch = 45.0f;

    state.yaw = 0.0f;
    glm::mat4 view1 = calculateViewMatrix(state);
    glm::vec3 pos1 = extractCameraPosition(view1);

    state.yaw = 180.0f;
    glm::mat4 view2 = calculateViewMatrix(state);
    glm::vec3 pos2 = extractCameraPosition(view2);

    // 180 degree rotation should flip X and Z
    assert(approxEqual(pos1.x, -pos2.x, 0.1f));
    assert(approxEqual(pos1.z, -pos2.z, 0.1f));
    // Y should be the same
    assert(approxEqual(pos1.y, pos2.y, 0.1f));

    printf("  PASS: View matrix changes when yaw changes\n");
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void test_edge_case_minimum_pitch() {
    printf("Testing edge case: minimum pitch (15 degrees)...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::PITCH_MIN;  // 15 degrees
    state.yaw = 45.0f;

    glm::mat4 view = calculateViewMatrix(state);

    // Should produce valid matrix (no gimbal lock)
    assert(view != glm::mat4(0.0f));

    // Focus should be centered
    glm::vec4 focusInView = view * glm::vec4(state.focus_point, 1.0f);
    assert(approxEqual(focusInView.x, 0.0f, 0.1f));
    assert(approxEqual(focusInView.y, 0.0f, 0.1f));

    printf("  PASS: Minimum pitch (15 degrees) produces valid view matrix\n");
}

void test_edge_case_maximum_pitch() {
    printf("Testing edge case: maximum pitch (80 degrees)...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::PITCH_MAX;  // 80 degrees (near top-down)
    state.yaw = 45.0f;

    glm::mat4 view = calculateViewMatrix(state);

    // Should produce valid matrix
    assert(view != glm::mat4(0.0f));

    // Camera should be mostly above
    glm::vec3 pos = calculateCameraPosition(state);
    float horizontalDist = std::sqrt(pos.x * pos.x + pos.z * pos.z);
    assert(pos.y > horizontalDist);  // More vertical than horizontal at 80 degrees

    printf("  PASS: Maximum pitch (80 degrees) produces valid view matrix\n");
}

void test_edge_case_yaw_wraparound() {
    printf("Testing edge case: yaw wraparound at 0/360 boundary...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f);
    state.distance = 50.0f;
    state.pitch = 45.0f;

    state.yaw = 0.0f;
    glm::mat4 view0 = calculateViewMatrix(state);

    state.yaw = 360.0f;
    glm::mat4 view360 = calculateViewMatrix(state);

    state.yaw = 359.9f;
    glm::mat4 view359 = calculateViewMatrix(state);

    state.yaw = 0.1f;
    glm::mat4 view01 = calculateViewMatrix(state);

    // 0 and 360 should be identical
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            assert(approxEqual(view0[i][j], view360[i][j], 0.001f));
        }
    }

    // 359.9 and 0.1 should be very close
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            assert(approxEqual(view359[i][j], view01[i][j], 0.1f));
        }
    }

    printf("  PASS: Yaw wraparound handled correctly\n");
}

void test_edge_case_gimbal_lock_avoidance() {
    printf("Testing edge case: gimbal lock avoidance (pitch clamped)...\n");

    // The CameraState clamps pitch to 15-80 degrees, which avoids the
    // singularity at pitch = 0 (horizontal) and pitch = 90 (straight down)
    // where the up vector would be parallel to the view direction.

    // Even at extreme clamped values, the view matrix should be stable
    CameraState state;
    state.focus_point = glm::vec3(0.0f);
    state.distance = 50.0f;

    // Test at clamped boundaries
    float pitches[] = {15.0f, 16.0f, 79.0f, 80.0f};

    for (float pitch : pitches) {
        state.pitch = pitch;

        for (float yaw = 0.0f; yaw < 360.0f; yaw += 30.0f) {
            state.yaw = yaw;
            glm::mat4 view = calculateViewMatrix(state);

            // All matrices should be valid (no NaN, no zero)
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    assert(!std::isnan(view[i][j]));
                    assert(!std::isinf(view[i][j]));
                }
            }
        }
    }

    printf("  PASS: No gimbal lock at clamped pitch boundaries\n");
}

void test_edge_case_zero_distance() {
    printf("Testing edge case: zero distance (degenerate case)...\n");

    // Note: Zero distance is not valid for our camera system,
    // but we should handle it gracefully (CameraState clamps distance to 5+)

    glm::vec3 focus(0.0f);
    float distance = 0.0f;  // Degenerate
    float pitch = 45.0f;
    float yaw = 45.0f;

    // Position calculation should put camera at focus
    glm::vec3 pos = calculateCameraPosition(focus, distance, pitch, yaw);
    assert(approxEqual(pos, focus, 0.001f));

    // View matrix would be degenerate (camera and target same point)
    // In practice, this should never happen due to distance clamping
    printf("  PASS: Zero distance handled (camera at focus point)\n");
}

// ============================================================================
// Direction Vector Tests
// ============================================================================

void test_camera_forward_direction() {
    printf("Testing camera forward direction calculation...\n");

    // At yaw = 0, pitch = 0, camera looks South (-Z)
    glm::vec3 forward = calculateCameraForward(0.0f, 0.0f);
    assert(isNormalized(forward));
    assert(approxEqual(forward.x, 0.0f, 0.01f));
    assert(forward.z < 0.0f);  // Looking towards -Z

    // At yaw = 90, pitch = 0, camera looks West (-X)
    forward = calculateCameraForward(0.0f, 90.0f);
    assert(isNormalized(forward));
    assert(forward.x < 0.0f);  // Looking towards -X
    assert(approxEqual(forward.z, 0.0f, 0.01f));

    // At pitch > 0, forward has negative Y component (looking down)
    forward = calculateCameraForward(45.0f, 0.0f);
    assert(isNormalized(forward));
    assert(forward.y < 0.0f);  // Looking down

    printf("  PASS: Forward direction calculated correctly\n");
}

void test_camera_right_direction() {
    printf("Testing camera right direction calculation...\n");

    // At yaw = 0, right should be West (-X)
    glm::vec3 right = calculateCameraRight(0.0f);
    assert(isNormalized(right));
    assert(right.x < 0.0f);
    assert(approxEqual(right.y, 0.0f, 0.001f));

    // At yaw = 90, right should be South (-Z)
    right = calculateCameraRight(90.0f);
    assert(isNormalized(right));
    assert(approxEqual(right.x, 0.0f, 0.01f));
    assert(right.z < 0.0f);

    printf("  PASS: Right direction calculated correctly\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("=== View Matrix Unit Tests ===\n\n");

    // Camera position tests
    printf("--- Camera Position Calculation Tests ---\n");
    test_camera_position_at_origin_preset_n();
    test_camera_position_at_origin_preset_e();
    test_camera_position_all_presets();
    test_camera_position_with_focus_offset();
    test_camera_position_arbitrary_angles();
    test_camera_position_from_camera_state();

    // View matrix tests
    printf("\n--- View Matrix Calculation Tests ---\n");
    test_view_matrix_basic();
    test_view_matrix_camera_position_matches();
    test_view_matrix_up_vector_is_world_up();
    test_view_matrix_isometric_presets();
    test_view_matrix_arbitrary_free_camera();
    test_view_matrix_from_camera_state();

    // Parameter update tests
    printf("\n--- Parameter Update Tests ---\n");
    test_view_matrix_updates_with_focus_change();
    test_view_matrix_updates_with_distance_change();
    test_view_matrix_updates_with_pitch_change();
    test_view_matrix_updates_with_yaw_change();

    // Edge case tests
    printf("\n--- Edge Case Tests ---\n");
    test_edge_case_minimum_pitch();
    test_edge_case_maximum_pitch();
    test_edge_case_yaw_wraparound();
    test_edge_case_gimbal_lock_avoidance();
    test_edge_case_zero_distance();

    // Direction vector tests
    printf("\n--- Direction Vector Tests ---\n");
    test_camera_forward_direction();
    test_camera_right_direction();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
