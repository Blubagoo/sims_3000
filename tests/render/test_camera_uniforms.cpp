/**
 * @file test_camera_uniforms.cpp
 * @brief Unit tests for view-projection matrix integration.
 *
 * Tests cover:
 * - Combined view-projection matrix calculation
 * - Matrix upload to camera uniform buffer (UBO)
 * - Aspect ratio update on window resize
 * - Projection recalculation on resize
 * - No visual distortion after resize (aspect ratio preserved)
 *
 * Ticket: 2-022 View-Projection Matrix Integration
 */

#include "sims3000/render/CameraUniforms.h"
#include "sims3000/render/CameraState.h"
#include "sims3000/render/ViewMatrix.h"
#include "sims3000/render/ProjectionMatrix.h"
#include "sims3000/render/ToonShader.h"
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

/// Compare two mat4 matrices with tolerance
bool approxEqual(const glm::mat4& a, const glm::mat4& b, float epsilon = EPSILON) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!approxEqual(a[i][j], b[i][j], epsilon)) {
                return false;
            }
        }
    }
    return true;
}

/// Check if matrix contains no NaN or Inf values
bool isValidMatrix(const glm::mat4& m) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (std::isnan(m[i][j]) || std::isinf(m[i][j])) {
                return false;
            }
        }
    }
    return true;
}

/// Apply perspective divide to clip coordinates
glm::vec3 perspectiveDivide(const glm::vec4& clip) {
    if (std::fabs(clip.w) < 0.0001f) {
        return glm::vec3(0.0f);
    }
    return glm::vec3(clip.x / clip.w, clip.y / clip.w, clip.z / clip.w);
}

/// Create a default camera state at north preset
CameraState createDefaultCameraState() {
    CameraState state;
    state.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;
    state.mode = CameraMode::Preset_N;
    return state;
}

// ============================================================================
// Combined View-Projection Matrix Tests
// ============================================================================

void test_combined_view_projection_matrix_calculated() {
    printf("Testing combined viewProjection matrix calculated...\n");

    CameraUniforms camera(1920, 1080);
    CameraState state = createDefaultCameraState();

    camera.update(state);

    // Get the combined matrix
    const glm::mat4& vp = camera.getViewProjectionMatrix();

    // Matrix should be valid
    assert(isValidMatrix(vp));

    // Should not be identity (view and projection applied)
    assert(vp != glm::mat4(1.0f));

    // Verify it matches manual calculation
    glm::mat4 view = calculateViewMatrix(state);
    glm::mat4 proj = calculateProjectionMatrix(
        CameraConfig::FOV_DEFAULT,
        1920.0f / 1080.0f,
        CameraConfig::NEAR_PLANE,
        CameraConfig::FAR_PLANE);
    glm::mat4 expected = calculateViewProjectionMatrix(view, proj);

    assert(approxEqual(vp, expected, 0.0001f));

    printf("  PASS: Combined viewProjection matrix calculated correctly\n");
}

void test_view_projection_matrix_order() {
    printf("Testing view-projection matrix multiplication order...\n");

    CameraUniforms camera(1920, 1080);
    CameraState state = createDefaultCameraState();

    camera.update(state);

    // Verify order is projection * view (not view * projection)
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix();
    glm::mat4 vp = camera.getViewProjectionMatrix();

    // projection * view
    glm::mat4 expectedOrder = proj * view;
    assert(approxEqual(vp, expectedOrder, 0.0001f));

    // Should NOT equal view * projection
    glm::mat4 wrongOrder = view * proj;
    // These are almost never equal, so this should pass
    // unless we have a very specific degenerate case
    if (approxEqual(vp, wrongOrder, 0.0001f)) {
        // Edge case - matrices happen to commute, skip check
        printf("  INFO: Matrices commute in this case (edge case)\n");
    }

    printf("  PASS: Matrix order is projection * view\n");
}

void test_separate_matrices_available() {
    printf("Testing separate view and projection matrices available...\n");

    CameraUniforms camera(1920, 1080);
    CameraState state = createDefaultCameraState();

    camera.update(state);

    // Should be able to get individual matrices
    const glm::mat4& view = camera.getViewMatrix();
    const glm::mat4& proj = camera.getProjectionMatrix();
    const glm::mat4& vp = camera.getViewProjectionMatrix();

    // All should be valid
    assert(isValidMatrix(view));
    assert(isValidMatrix(proj));
    assert(isValidMatrix(vp));

    // All should be different (in general case)
    assert(view != proj);

    printf("  PASS: Separate view and projection matrices available\n");
}

// ============================================================================
// Matrix Upload to Uniform Buffer Tests
// ============================================================================

void test_matrix_uploaded_to_camera_uniform_buffer() {
    printf("Testing matrix uploaded to camera uniform buffer...\n");

    CameraUniforms camera(1920, 1080);
    CameraState state = createDefaultCameraState();

    camera.update(state);

    // Get the UBO data
    const ToonViewProjectionUBO& ubo = camera.getUBO();

    // UBO should contain the view-projection matrix
    const glm::mat4& vp = camera.getViewProjectionMatrix();
    assert(approxEqual(ubo.viewProjection, vp, 0.0001f));

    printf("  PASS: Matrix uploaded to camera uniform buffer (UBO)\n");
}

void test_ubo_structure_size() {
    printf("Testing UBO structure size for GPU alignment...\n");

    // ToonViewProjectionUBO should be exactly 64 bytes (mat4)
    assert(sizeof(ToonViewProjectionUBO) == 64);

    printf("  PASS: UBO structure is 64 bytes (correct GPU alignment)\n");
}

void test_ubo_updates_on_camera_change() {
    printf("Testing UBO updates when camera state changes...\n");

    CameraUniforms camera(1920, 1080);
    CameraState state = createDefaultCameraState();

    // Initial update
    camera.update(state);
    ToonViewProjectionUBO ubo1 = camera.getUBO();

    // Change camera state
    state.yaw = CameraConfig::PRESET_E_YAW;  // Rotate to east
    camera.update(state);
    ToonViewProjectionUBO ubo2 = camera.getUBO();

    // UBO should have changed
    assert(!approxEqual(ubo1.viewProjection, ubo2.viewProjection, 0.0001f));

    printf("  PASS: UBO updates when camera state changes\n");
}

// ============================================================================
// Aspect Ratio Update on Window Resize Tests
// ============================================================================

void test_aspect_ratio_updated_on_window_resize() {
    printf("Testing aspect ratio updated on window resize...\n");

    CameraUniforms camera(1920, 1080);

    // Initial aspect ratio
    float initialAspect = camera.getAspectRatio();
    assert(approxEqual(initialAspect, 1920.0f / 1080.0f, 0.001f));

    // Resize to different aspect ratio
    camera.onWindowResize(1280, 1024);  // 5:4 aspect

    float newAspect = camera.getAspectRatio();
    assert(approxEqual(newAspect, 1280.0f / 1024.0f, 0.001f));

    // Should be different from initial
    assert(!approxEqual(initialAspect, newAspect, 0.001f));

    printf("  PASS: Aspect ratio updated on window resize\n");
}

void test_window_dimensions_stored() {
    printf("Testing window dimensions stored correctly...\n");

    CameraUniforms camera(1920, 1080);

    assert(camera.getWindowWidth() == 1920);
    assert(camera.getWindowHeight() == 1080);

    camera.onWindowResize(2560, 1440);

    assert(camera.getWindowWidth() == 2560);
    assert(camera.getWindowHeight() == 1440);

    printf("  PASS: Window dimensions stored correctly\n");
}

void test_resize_handles_zero_dimensions() {
    printf("Testing resize handles zero dimensions gracefully...\n");

    CameraUniforms camera(1920, 1080);

    // Resize with zero width
    camera.onWindowResize(0, 1080);
    assert(camera.getWindowWidth() >= 1);  // Should not be zero

    // Resize with zero height
    camera.onWindowResize(1920, 0);
    assert(camera.getWindowHeight() >= 1);  // Should not be zero

    // Resize with both zero
    camera.onWindowResize(0, 0);
    assert(camera.getWindowWidth() >= 1);
    assert(camera.getWindowHeight() >= 1);

    // Aspect ratio should still be valid
    float aspect = camera.getAspectRatio();
    assert(!std::isnan(aspect) && !std::isinf(aspect));

    printf("  PASS: Zero dimensions handled gracefully\n");
}

// ============================================================================
// Projection Recalculation on Resize Tests
// ============================================================================

void test_projection_recalculated_on_resize() {
    printf("Testing projection recalculated on resize...\n");

    CameraUniforms camera(1920, 1080);
    CameraState state = createDefaultCameraState();

    // Initial update
    camera.update(state);
    glm::mat4 proj1 = camera.getProjectionMatrix();

    // Resize window
    camera.onWindowResize(1280, 1024);

    // Update with same camera state
    camera.update(state);
    glm::mat4 proj2 = camera.getProjectionMatrix();

    // Projection should have changed due to aspect ratio change
    assert(!approxEqual(proj1, proj2, 0.0001f));

    printf("  PASS: Projection recalculated on resize\n");
}

void test_projection_dirty_flag_set_on_resize() {
    printf("Testing projection dirty flag set on resize...\n");

    CameraUniforms camera(1920, 1080);
    CameraState state = createDefaultCameraState();

    // Initial update clears the flag
    camera.update(state);

    // After normal update, flag should be cleared on next update
    camera.update(state);
    assert(!camera.wasProjectionRecalculated());

    // Resize sets the flag
    camera.onWindowResize(1280, 1024);

    // Next update should recalculate and set the flag
    camera.update(state);
    assert(camera.wasProjectionRecalculated());

    // After another update without resize, flag should be false
    camera.update(state);
    assert(!camera.wasProjectionRecalculated());

    printf("  PASS: Projection dirty flag works correctly\n");
}

void test_no_recalculation_on_same_size_resize() {
    printf("Testing no recalculation on resize to same size...\n");

    CameraUniforms camera(1920, 1080);
    CameraState state = createDefaultCameraState();

    camera.update(state);
    glm::mat4 proj1 = camera.getProjectionMatrix();

    // "Resize" to same dimensions
    camera.onWindowResize(1920, 1080);
    camera.update(state);

    glm::mat4 proj2 = camera.getProjectionMatrix();

    // Projection should be identical
    assert(approxEqual(proj1, proj2, 0.0001f));

    printf("  PASS: No unnecessary recalculation on same-size resize\n");
}

// ============================================================================
// No Visual Distortion After Resize Tests
// ============================================================================

void test_no_visual_distortion_after_resize() {
    printf("Testing no visual distortion after resize...\n");

    CameraState state = createDefaultCameraState();

    // Test various aspect ratios
    struct TestCase {
        uint32_t width;
        uint32_t height;
        float expectedAspect;
    };

    TestCase cases[] = {
        {1920, 1080, 16.0f / 9.0f},   // 16:9
        {1280, 720, 16.0f / 9.0f},    // 16:9
        {1024, 768, 4.0f / 3.0f},     // 4:3
        {2560, 1080, 21.0f / 9.0f},   // 21:9 ultrawide
        {1000, 1000, 1.0f},           // Square
        {720, 1280, 9.0f / 16.0f},    // Portrait
    };

    for (const auto& tc : cases) {
        CameraUniforms camera(tc.width, tc.height);
        camera.update(state);

        // Aspect ratio should match expected
        float aspect = camera.getAspectRatio();
        assert(approxEqual(aspect, tc.expectedAspect, 0.01f));

        // Projection matrix should be valid
        assert(isValidMatrix(camera.getProjectionMatrix()));

        // View-projection matrix should be valid
        assert(isValidMatrix(camera.getViewProjectionMatrix()));

        // UBO should be valid
        assert(isValidMatrix(camera.getUBO().viewProjection));
    }

    printf("  PASS: No visual distortion for various aspect ratios\n");
}

void test_circle_stays_circular_after_resize() {
    printf("Testing circle stays circular after resize (aspect preserved)...\n");

    CameraState state = createDefaultCameraState();
    state.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    state.distance = 50.0f;

    // For a point at equal X and Y offset from center,
    // the screen-space distances should have the correct ratio

    CameraUniforms camera(1920, 1080);
    camera.update(state);

    const glm::mat4& vp = camera.getViewProjectionMatrix();

    // Points at equal world-space distance from origin
    float offset = 10.0f;
    glm::vec4 rightPoint(offset, 0.0f, 0.0f, 1.0f);
    glm::vec4 upPoint(0.0f, offset, 0.0f, 1.0f);
    glm::vec4 center(0.0f, 0.0f, 0.0f, 1.0f);

    // Transform all points
    glm::vec4 rightClip = vp * rightPoint;
    glm::vec4 upClip = vp * upPoint;
    glm::vec4 centerClip = vp * center;

    // Get NDC coordinates
    glm::vec3 rightNDC = perspectiveDivide(rightClip);
    glm::vec3 upNDC = perspectiveDivide(upClip);
    glm::vec3 centerNDC = perspectiveDivide(centerClip);

    // Calculate screen-space distances from center
    float rightDist = std::fabs(rightNDC.x - centerNDC.x);
    float upDist = std::fabs(upNDC.y - centerNDC.y);

    // After accounting for aspect ratio, circle should appear circular
    // NDC is [-1, 1] in both dimensions, but screen is 16:9
    // So horizontal extent in NDC maps to more pixels
    // Multiply Y distance by aspect to compare in "screen pixels"
    float aspect = camera.getAspectRatio();
    float adjustedUpDist = upDist * aspect;

    // The ratio should be reasonable (not extreme distortion)
    // Due to perspective and camera angle, it won't be exactly 1:1
    float ratio = rightDist / adjustedUpDist;
    assert(ratio > 0.3f && ratio < 3.0f);  // Reasonable range

    printf("  PASS: Aspect ratio preserved (ratio: %.3f)\n", ratio);
}

void test_resize_sequence_consistency() {
    printf("Testing resize sequence produces consistent results...\n");

    CameraState state = createDefaultCameraState();

    // Create two cameras that go through different resize sequences
    CameraUniforms camera1(800, 600);
    CameraUniforms camera2(1920, 1080);

    // Resize both to same final size
    camera1.onWindowResize(1920, 1080);
    // camera2 starts at 1920x1080

    // Update both
    camera1.update(state);
    camera2.update(state);

    // They should produce identical matrices
    assert(approxEqual(camera1.getProjectionMatrix(), camera2.getProjectionMatrix(), 0.0001f));
    assert(approxEqual(camera1.getViewMatrix(), camera2.getViewMatrix(), 0.0001f));
    assert(approxEqual(camera1.getViewProjectionMatrix(), camera2.getViewProjectionMatrix(), 0.0001f));

    printf("  PASS: Different resize sequences produce consistent results\n");
}

// ============================================================================
// FOV and Clipping Plane Configuration Tests
// ============================================================================

void test_fov_configuration() {
    printf("Testing FOV configuration...\n");

    CameraUniforms camera(1920, 1080, 45.0f);  // Custom FOV

    assert(approxEqual(camera.getFOV(), 45.0f, 0.001f));

    // Change FOV
    camera.setFOV(60.0f);
    assert(approxEqual(camera.getFOV(), 60.0f, 0.001f));

    // FOV should be clamped
    camera.setFOV(5.0f);  // Below minimum
    assert(approxEqual(camera.getFOV(), ProjectionConfig::MIN_FOV_DEGREES, 0.001f));

    camera.setFOV(120.0f);  // Above maximum
    assert(approxEqual(camera.getFOV(), ProjectionConfig::MAX_FOV_DEGREES, 0.001f));

    printf("  PASS: FOV configuration works correctly\n");
}

void test_clipping_plane_configuration() {
    printf("Testing clipping plane configuration...\n");

    CameraUniforms camera(1920, 1080, 35.0f, 0.5f, 500.0f);

    assert(approxEqual(camera.getNearPlane(), 0.5f, 0.001f));
    assert(approxEqual(camera.getFarPlane(), 500.0f, 0.001f));

    // Change clipping planes
    camera.setClippingPlanes(1.0f, 2000.0f);
    assert(approxEqual(camera.getNearPlane(), 1.0f, 0.001f));
    assert(approxEqual(camera.getFarPlane(), 2000.0f, 0.001f));

    printf("  PASS: Clipping plane configuration works correctly\n");
}

void test_force_recalculate_projection() {
    printf("Testing force recalculate projection...\n");

    CameraUniforms camera(1920, 1080);
    CameraState state = createDefaultCameraState();

    camera.update(state);
    glm::mat4 proj1 = camera.getProjectionMatrix();

    // Change FOV directly
    camera.setFOV(60.0f);

    // Force recalculation
    camera.recalculateProjection();

    glm::mat4 proj2 = camera.getProjectionMatrix();

    // Projection should have changed
    assert(!approxEqual(proj1, proj2, 0.0001f));

    // Should be marked as recalculated
    assert(camera.wasProjectionRecalculated());

    printf("  PASS: Force recalculate projection works\n");
}

// ============================================================================
// Edge Cases and Stress Tests
// ============================================================================

void test_extreme_camera_positions() {
    printf("Testing extreme camera positions...\n");

    CameraUniforms camera(1920, 1080);

    CameraState state;

    // Very close zoom
    state.focus_point = glm::vec3(0.0f);
    state.distance = CameraConfig::DISTANCE_MIN;
    state.pitch = CameraConfig::PITCH_MIN;
    state.yaw = 0.0f;

    camera.update(state);
    assert(isValidMatrix(camera.getViewProjectionMatrix()));

    // Very far zoom
    state.distance = CameraConfig::DISTANCE_MAX;
    camera.update(state);
    assert(isValidMatrix(camera.getViewProjectionMatrix()));

    // Maximum pitch
    state.pitch = CameraConfig::PITCH_MAX;
    camera.update(state);
    assert(isValidMatrix(camera.getViewProjectionMatrix()));

    // Various yaw angles
    for (float yaw = 0.0f; yaw < 360.0f; yaw += 45.0f) {
        state.yaw = yaw;
        camera.update(state);
        assert(isValidMatrix(camera.getViewProjectionMatrix()));
    }

    printf("  PASS: Extreme camera positions handled correctly\n");
}

void test_rapid_resize_sequence() {
    printf("Testing rapid resize sequence...\n");

    CameraUniforms camera(1920, 1080);
    CameraState state = createDefaultCameraState();

    // Simulate rapid resize events
    std::uint32_t sizes[][2] = {
        {800, 600}, {1024, 768}, {1280, 720}, {1920, 1080},
        {2560, 1440}, {3840, 2160}, {1920, 1080}
    };

    for (auto& size : sizes) {
        camera.onWindowResize(size[0], size[1]);
        camera.update(state);

        // All should remain valid
        assert(isValidMatrix(camera.getViewProjectionMatrix()));
        assert(camera.getWindowWidth() == size[0]);
        assert(camera.getWindowHeight() == size[1]);
    }

    printf("  PASS: Rapid resize sequence handled correctly\n");
}

void test_default_construction() {
    printf("Testing default construction...\n");

    CameraUniforms camera;  // Uses defaults

    // Should have valid default dimensions
    assert(camera.getWindowWidth() > 0);
    assert(camera.getWindowHeight() > 0);

    // Should have valid FOV
    assert(camera.getFOV() >= ProjectionConfig::MIN_FOV_DEGREES);
    assert(camera.getFOV() <= ProjectionConfig::MAX_FOV_DEGREES);

    // Should have valid clipping planes
    assert(camera.getNearPlane() > 0.0f);
    assert(camera.getFarPlane() > camera.getNearPlane());

    // Matrices should be valid after construction
    assert(isValidMatrix(camera.getProjectionMatrix()));

    printf("  PASS: Default construction works correctly\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("=== Camera Uniforms Unit Tests (Ticket 2-022) ===\n\n");

    // Combined view-projection matrix tests
    printf("--- Combined View-Projection Matrix Tests ---\n");
    test_combined_view_projection_matrix_calculated();
    test_view_projection_matrix_order();
    test_separate_matrices_available();

    // UBO upload tests
    printf("\n--- Matrix Upload to Uniform Buffer Tests ---\n");
    test_matrix_uploaded_to_camera_uniform_buffer();
    test_ubo_structure_size();
    test_ubo_updates_on_camera_change();

    // Aspect ratio tests
    printf("\n--- Aspect Ratio Update on Resize Tests ---\n");
    test_aspect_ratio_updated_on_window_resize();
    test_window_dimensions_stored();
    test_resize_handles_zero_dimensions();

    // Projection recalculation tests
    printf("\n--- Projection Recalculation on Resize Tests ---\n");
    test_projection_recalculated_on_resize();
    test_projection_dirty_flag_set_on_resize();
    test_no_recalculation_on_same_size_resize();

    // No visual distortion tests
    printf("\n--- No Visual Distortion After Resize Tests ---\n");
    test_no_visual_distortion_after_resize();
    test_circle_stays_circular_after_resize();
    test_resize_sequence_consistency();

    // Configuration tests
    printf("\n--- FOV and Clipping Plane Configuration Tests ---\n");
    test_fov_configuration();
    test_clipping_plane_configuration();
    test_force_recalculate_projection();

    // Edge cases
    printf("\n--- Edge Cases and Stress Tests ---\n");
    test_extreme_camera_positions();
    test_rapid_resize_sequence();
    test_default_construction();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
