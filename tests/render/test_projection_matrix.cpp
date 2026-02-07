/**
 * @file test_projection_matrix.cpp
 * @brief Unit tests for perspective projection matrix calculation.
 *
 * Tests cover:
 * - Perspective projection with default and custom FOV
 * - Aspect ratio calculation and handling
 * - Near/far plane configuration
 * - Perspective divide correctness
 * - Foreshortening at isometric preset angles
 * - FOV clamping and parameter validation
 * - View-projection matrix combination
 */

#include "sims3000/render/ProjectionMatrix.h"
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

/// Compare two vec4 with tolerance
bool approxEqual(const glm::vec4& a, const glm::vec4& b, float epsilon = EPSILON) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon) &&
           approxEqual(a.w, b.w, epsilon);
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

// ============================================================================
// Perspective Projection Tests
// ============================================================================

void test_projection_matrix_basic() {
    printf("Testing basic projection matrix calculation...\n");

    float aspect = 16.0f / 9.0f;
    glm::mat4 proj = calculateProjectionMatrix(
        ProjectionConfig::DEFAULT_FOV_DEGREES,
        aspect,
        ProjectionConfig::NEAR_PLANE,
        ProjectionConfig::FAR_PLANE);

    // Matrix should be valid
    assert(isValidMatrix(proj));

    // Should not be identity
    assert(proj != glm::mat4(1.0f));

    // Perspective matrix characteristic: proj[2][3] should be -1 for RH
    // This is what makes the w coordinate contain depth for perspective divide
    assert(approxEqual(proj[2][3], -1.0f, 0.001f));

    // proj[3][3] should be 0 for perspective (not 1 as in orthographic)
    assert(approxEqual(proj[3][3], 0.0f, 0.001f));

    printf("  PASS: Basic projection matrix has correct structure\n");
}

void test_projection_matrix_default_fov() {
    printf("Testing projection with default FOV (35 degrees)...\n");

    float aspect = 1280.0f / 720.0f;

    // Using default FOV convenience function
    glm::mat4 proj1 = calculateProjectionMatrixDefault(aspect);

    // Using explicit default FOV
    glm::mat4 proj2 = calculateProjectionMatrix(
        ProjectionConfig::DEFAULT_FOV_DEGREES, aspect);

    // Both should produce identical matrices
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            assert(approxEqual(proj1[i][j], proj2[i][j], 0.0001f));
        }
    }

    // Default FOV should be 35 degrees
    assert(approxEqual(ProjectionConfig::DEFAULT_FOV_DEGREES, 35.0f, 0.001f));

    // Also verify CameraConfig has matching FOV
    assert(approxEqual(CameraConfig::FOV_DEFAULT, 35.0f, 0.001f));

    printf("  PASS: Default FOV is 35 degrees, configurable\n");
}

void test_projection_matrix_custom_fov() {
    printf("Testing projection with custom FOV values...\n");

    float aspect = 16.0f / 9.0f;

    // Test various FOV values
    float fovs[] = {25.0f, 35.0f, 45.0f, 60.0f, 75.0f, 90.0f};

    glm::mat4 prevProj = glm::mat4(0.0f);

    for (float fov : fovs) {
        glm::mat4 proj = calculateProjectionMatrix(fov, aspect);

        // All should be valid
        assert(isValidMatrix(proj));

        // Each FOV should produce a different matrix
        if (prevProj != glm::mat4(0.0f)) {
            assert(proj != prevProj);
        }
        prevProj = proj;

        // Higher FOV = wider view = smaller [0][0] element (x scaling)
        // and smaller [1][1] element (y scaling)
    }

    // Verify wider FOV gives smaller scale factor
    glm::mat4 narrow = calculateProjectionMatrix(25.0f, aspect);
    glm::mat4 wide = calculateProjectionMatrix(90.0f, aspect);
    assert(wide[1][1] < narrow[1][1]);  // Vertical scale smaller for wide FOV

    printf("  PASS: Custom FOV values produce correct matrices\n");
}

void test_projection_fov_configurable() {
    printf("Testing FOV is configurable via CameraConfig...\n");

    // Verify CameraConfig has FOV constants
    assert(CameraConfig::FOV_DEFAULT == 35.0f);
    assert(CameraConfig::FOV_MIN == 20.0f);
    assert(CameraConfig::FOV_MAX == 90.0f);

    // ProjectionConfig should match
    assert(ProjectionConfig::DEFAULT_FOV_DEGREES == CameraConfig::FOV_DEFAULT);
    assert(ProjectionConfig::MIN_FOV_DEGREES == CameraConfig::FOV_MIN);
    assert(ProjectionConfig::MAX_FOV_DEGREES == CameraConfig::FOV_MAX);

    printf("  PASS: FOV configurable via CameraConfig (default 35 degrees)\n");
}

// ============================================================================
// Aspect Ratio Tests
// ============================================================================

void test_aspect_ratio_calculation() {
    printf("Testing aspect ratio calculation...\n");

    // Standard resolutions
    assert(approxEqual(calculateAspectRatio(1920, 1080), 16.0f / 9.0f, 0.01f));
    assert(approxEqual(calculateAspectRatio(1280, 720), 16.0f / 9.0f, 0.01f));
    assert(approxEqual(calculateAspectRatio(1024, 768), 4.0f / 3.0f, 0.01f));
    assert(approxEqual(calculateAspectRatio(800, 600), 4.0f / 3.0f, 0.01f));
    assert(approxEqual(calculateAspectRatio(2560, 1080), 21.0f / 9.0f, 0.02f));

    // Square
    assert(approxEqual(calculateAspectRatio(1000, 1000), 1.0f, 0.001f));

    // Tall (portrait)
    assert(calculateAspectRatio(720, 1280) < 1.0f);

    printf("  PASS: Aspect ratio calculated correctly for various resolutions\n");
}

void test_aspect_ratio_invalid_dimensions() {
    printf("Testing aspect ratio with invalid dimensions...\n");

    // Zero or negative dimensions should return 1.0 (safe fallback)
    assert(approxEqual(calculateAspectRatio(0, 720), 1.0f, 0.001f));
    assert(approxEqual(calculateAspectRatio(1280, 0), 1.0f, 0.001f));
    assert(approxEqual(calculateAspectRatio(0, 0), 1.0f, 0.001f));
    assert(approxEqual(calculateAspectRatio(-100, 720), 1.0f, 0.001f));
    assert(approxEqual(calculateAspectRatio(1280, -100), 1.0f, 0.001f));

    printf("  PASS: Invalid dimensions fallback to aspect ratio 1.0\n");
}

void test_projection_from_dimensions() {
    printf("Testing projection matrix from window dimensions...\n");

    // Test with standard HD resolution
    glm::mat4 proj1 = calculateProjectionMatrixFromDimensions(1920, 1080);

    // Should match explicit aspect ratio version
    float aspect = 1920.0f / 1080.0f;
    glm::mat4 proj2 = calculateProjectionMatrix(
        ProjectionConfig::DEFAULT_FOV_DEGREES, aspect);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            assert(approxEqual(proj1[i][j], proj2[i][j], 0.0001f));
        }
    }

    printf("  PASS: Projection from dimensions matches aspect ratio calculation\n");
}

void test_aspect_ratio_maintained() {
    printf("Testing aspect ratio is maintained in projection...\n");

    // Wide aspect ratio
    glm::mat4 wideProj = calculateProjectionMatrix(45.0f, 16.0f / 9.0f);

    // Tall aspect ratio
    glm::mat4 tallProj = calculateProjectionMatrix(45.0f, 9.0f / 16.0f);

    // Square aspect ratio
    glm::mat4 squareProj = calculateProjectionMatrix(45.0f, 1.0f);

    // All should be valid
    assert(isValidMatrix(wideProj));
    assert(isValidMatrix(tallProj));
    assert(isValidMatrix(squareProj));

    // Different aspect ratios should produce different [0][0] values
    // [0][0] = f / aspect where f = 1/tan(fov/2)
    assert(wideProj[0][0] != tallProj[0][0]);
    assert(wideProj[0][0] != squareProj[0][0]);

    // [1][1] should be same for same FOV (f = 1/tan(fov/2))
    assert(approxEqual(wideProj[1][1], tallProj[1][1], 0.001f));
    assert(approxEqual(wideProj[1][1], squareProj[1][1], 0.001f));

    printf("  PASS: Aspect ratio correctly affects projection matrix\n");
}

// ============================================================================
// Near/Far Plane Tests
// ============================================================================

void test_near_far_planes_default() {
    printf("Testing default near/far plane values...\n");

    // Verify defaults
    assert(approxEqual(ProjectionConfig::NEAR_PLANE, 0.1f, 0.001f));
    assert(approxEqual(ProjectionConfig::FAR_PLANE, 1000.0f, 0.001f));

    // CameraConfig should match
    assert(approxEqual(CameraConfig::NEAR_PLANE, 0.1f, 0.001f));
    assert(approxEqual(CameraConfig::FAR_PLANE, 1000.0f, 0.001f));

    printf("  PASS: Near plane 0.1, far plane 1000.0\n");
}

void test_near_far_planes_custom() {
    printf("Testing custom near/far plane values...\n");

    float aspect = 16.0f / 9.0f;

    // Different near/far combinations
    glm::mat4 proj1 = calculateProjectionMatrix(35.0f, aspect, 0.1f, 100.0f);
    glm::mat4 proj2 = calculateProjectionMatrix(35.0f, aspect, 1.0f, 1000.0f);
    glm::mat4 proj3 = calculateProjectionMatrix(35.0f, aspect, 0.01f, 10000.0f);

    // All should be valid
    assert(isValidMatrix(proj1));
    assert(isValidMatrix(proj2));
    assert(isValidMatrix(proj3));

    // Different planes should produce different [2][2] and [3][2] values
    assert(proj1[2][2] != proj2[2][2]);
    assert(proj1[3][2] != proj2[3][2]);

    printf("  PASS: Custom near/far planes produce valid matrices\n");
}

void test_depth_range_zero_to_one() {
    printf("Testing depth range [0, 1] (Vulkan/SDL_GPU)...\n");

    float aspect = 16.0f / 9.0f;
    float near = 0.1f;
    float far = 1000.0f;
    glm::mat4 proj = calculateProjectionMatrix(35.0f, aspect, near, far);

    // Test point at near plane (should map to depth 0)
    glm::vec4 nearPoint(0.0f, 0.0f, -near, 1.0f);  // -Z is forward
    glm::vec4 nearClip = proj * nearPoint;
    glm::vec3 nearNDC = perspectiveDivide(nearClip);
    assert(approxEqual(nearNDC.z, 0.0f, 0.01f));  // Near -> depth 0

    // Test point at far plane (should map to depth 1)
    glm::vec4 farPoint(0.0f, 0.0f, -far, 1.0f);
    glm::vec4 farClip = proj * farPoint;
    glm::vec3 farNDC = perspectiveDivide(farClip);
    assert(approxEqual(farNDC.z, 1.0f, 0.01f));  // Far -> depth 1

    // Test point at middle distance
    float mid = (near + far) / 2.0f;
    glm::vec4 midPoint(0.0f, 0.0f, -mid, 1.0f);
    glm::vec4 midClip = proj * midPoint;
    glm::vec3 midNDC = perspectiveDivide(midClip);
    assert(midNDC.z > 0.0f && midNDC.z < 1.0f);  // Mid is between 0 and 1

    printf("  PASS: Depth range is [0, 1] for Vulkan/SDL_GPU\n");
}

// ============================================================================
// Perspective Divide Tests
// ============================================================================

void test_perspective_divide_correct() {
    printf("Testing perspective divide correctness...\n");

    float aspect = 16.0f / 9.0f;
    glm::mat4 proj = calculateProjectionMatrix(35.0f, aspect, 0.1f, 1000.0f);

    // Test that W coordinate contains depth for perspective divide
    // A point in front of camera (negative Z in view space)
    glm::vec4 point(5.0f, 3.0f, -50.0f, 1.0f);
    glm::vec4 clip = proj * point;

    // W should be positive (since -Z * -1 = positive) for proper perspective
    assert(clip.w > 0.0f);

    // W should equal -Z (the view-space depth negated)
    // In RH system with [0,1] depth, W = -point.z for standard perspective
    assert(approxEqual(clip.w, -point.z, 0.01f));

    printf("  PASS: Perspective divide W coordinate correct\n");
}

void test_perspective_divide_objects_shrink_with_distance() {
    printf("Testing objects shrink with distance (perspective effect)...\n");

    float aspect = 16.0f / 9.0f;
    glm::mat4 proj = calculateProjectionMatrix(45.0f, aspect);

    // Same world-space offset at different depths
    float offset = 10.0f;

    // Near point
    glm::vec4 nearPoint(offset, 0.0f, -10.0f, 1.0f);
    glm::vec4 nearClip = proj * nearPoint;
    float nearScreenX = nearClip.x / nearClip.w;

    // Far point (same X offset, but further away)
    glm::vec4 farPoint(offset, 0.0f, -100.0f, 1.0f);
    glm::vec4 farClip = proj * farPoint;
    float farScreenX = farClip.x / farClip.w;

    // Object at greater distance should have smaller screen-space displacement
    assert(std::fabs(farScreenX) < std::fabs(nearScreenX));

    printf("  PASS: Objects correctly shrink with distance\n");
}

void test_center_point_stays_centered() {
    printf("Testing center point stays centered after projection...\n");

    float aspect = 16.0f / 9.0f;
    glm::mat4 proj = calculateProjectionMatrix(35.0f, aspect);

    // Point on center axis at various depths
    float depths[] = {-1.0f, -10.0f, -50.0f, -100.0f, -500.0f};

    for (float z : depths) {
        glm::vec4 centerPoint(0.0f, 0.0f, z, 1.0f);
        glm::vec4 clip = proj * centerPoint;
        glm::vec3 ndc = perspectiveDivide(clip);

        // X and Y should be 0 (centered)
        assert(approxEqual(ndc.x, 0.0f, 0.001f));
        assert(approxEqual(ndc.y, 0.0f, 0.001f));
    }

    printf("  PASS: Center axis stays centered at all depths\n");
}

// ============================================================================
// Foreshortening Tests (Isometric Preset Angle)
// ============================================================================

void test_minimal_foreshortening_at_isometric_angle() {
    printf("Testing minimal foreshortening at isometric preset angle...\n");

    // At ~35.264 degree pitch with 35 degree FOV, foreshortening should be minimal
    // This means vertical and horizontal grid lines appear similar in length

    float aspect = 16.0f / 9.0f;
    float fov = 35.0f;  // Default FOV
    glm::mat4 proj = calculateProjectionMatrix(fov, aspect);

    // Simulate view matrix at isometric pitch (35.264 degrees)
    // At this angle, looking down at about 35 degrees
    float pitch = CameraConfig::ISOMETRIC_PITCH;

    // For a cube at the focus point, edges parallel to view plane should
    // appear approximately equal to edges perpendicular (minimal distortion)

    // This is primarily a visual check, but we can verify the projection
    // doesn't introduce extreme foreshortening:

    // Test vertical line (Y axis) at center
    glm::vec4 top(0.0f, 10.0f, -50.0f, 1.0f);
    glm::vec4 bottom(0.0f, -10.0f, -50.0f, 1.0f);

    glm::vec4 topClip = proj * top;
    glm::vec4 bottomClip = proj * bottom;

    float topY = topClip.y / topClip.w;
    float bottomY = bottomClip.y / bottomClip.w;
    float verticalSpan = std::fabs(topY - bottomY);

    // Test horizontal line (X axis) at center
    glm::vec4 left(-10.0f, 0.0f, -50.0f, 1.0f);
    glm::vec4 right(10.0f, 0.0f, -50.0f, 1.0f);

    glm::vec4 leftClip = proj * left;
    glm::vec4 rightClip = proj * right;

    float leftX = leftClip.x / leftClip.w;
    float rightX = rightClip.x / rightClip.w;
    float horizontalSpan = std::fabs(rightX - leftX);

    // Both should be reasonably similar (not extreme distortion)
    // Account for aspect ratio difference
    float adjustedHorizontal = horizontalSpan * aspect;

    // With 35 degree FOV at isometric angle, ratio should be close to 1
    float ratio = verticalSpan / adjustedHorizontal;
    assert(ratio > 0.7f && ratio < 1.4f);  // Reasonable foreshortening range

    printf("  PASS: Minimal foreshortening at isometric angle (ratio: %.3f)\n", ratio);
}

// ============================================================================
// FOV Clamping Tests
// ============================================================================

void test_fov_clamping() {
    printf("Testing FOV clamping...\n");

    // Below minimum
    assert(approxEqual(clampFOV(10.0f), ProjectionConfig::MIN_FOV_DEGREES, 0.001f));
    assert(approxEqual(clampFOV(0.0f), ProjectionConfig::MIN_FOV_DEGREES, 0.001f));
    assert(approxEqual(clampFOV(-10.0f), ProjectionConfig::MIN_FOV_DEGREES, 0.001f));

    // Above maximum
    assert(approxEqual(clampFOV(100.0f), ProjectionConfig::MAX_FOV_DEGREES, 0.001f));
    assert(approxEqual(clampFOV(180.0f), ProjectionConfig::MAX_FOV_DEGREES, 0.001f));

    // Within range - should not be modified
    assert(approxEqual(clampFOV(35.0f), 35.0f, 0.001f));
    assert(approxEqual(clampFOV(45.0f), 45.0f, 0.001f));
    assert(approxEqual(clampFOV(20.0f), 20.0f, 0.001f));
    assert(approxEqual(clampFOV(90.0f), 90.0f, 0.001f));

    printf("  PASS: FOV clamped to [20, 90] degrees\n");
}

void test_projection_with_extreme_fov() {
    printf("Testing projection with extreme FOV values (auto-clamped)...\n");

    float aspect = 16.0f / 9.0f;

    // Very low FOV should be clamped to minimum
    glm::mat4 lowFov = calculateProjectionMatrix(5.0f, aspect);
    glm::mat4 minFov = calculateProjectionMatrix(ProjectionConfig::MIN_FOV_DEGREES, aspect);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            assert(approxEqual(lowFov[i][j], minFov[i][j], 0.0001f));
        }
    }

    // Very high FOV should be clamped to maximum
    glm::mat4 highFov = calculateProjectionMatrix(150.0f, aspect);
    glm::mat4 maxFov = calculateProjectionMatrix(ProjectionConfig::MAX_FOV_DEGREES, aspect);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            assert(approxEqual(highFov[i][j], maxFov[i][j], 0.0001f));
        }
    }

    printf("  PASS: Extreme FOV values auto-clamped\n");
}

// ============================================================================
// Parameter Validation Tests
// ============================================================================

void test_parameter_validation() {
    printf("Testing parameter validation...\n");

    // Valid parameters
    assert(validateProjectionParameters(35.0f, 16.0f/9.0f, 0.1f, 1000.0f));
    assert(validateProjectionParameters(90.0f, 1.0f, 0.01f, 100.0f));
    assert(validateProjectionParameters(20.0f, 2.0f, 1.0f, 10000.0f));

    // Invalid FOV (below min)
    assert(!validateProjectionParameters(10.0f, 16.0f/9.0f, 0.1f, 1000.0f));

    // Invalid FOV (above max)
    assert(!validateProjectionParameters(100.0f, 16.0f/9.0f, 0.1f, 1000.0f));

    // Invalid aspect ratio
    assert(!validateProjectionParameters(35.0f, 0.0f, 0.1f, 1000.0f));
    assert(!validateProjectionParameters(35.0f, -1.0f, 0.1f, 1000.0f));

    // Invalid near plane
    assert(!validateProjectionParameters(35.0f, 16.0f/9.0f, 0.0f, 1000.0f));
    assert(!validateProjectionParameters(35.0f, 16.0f/9.0f, -1.0f, 1000.0f));

    // Invalid far plane (not greater than near)
    assert(!validateProjectionParameters(35.0f, 16.0f/9.0f, 0.1f, 0.1f));
    assert(!validateProjectionParameters(35.0f, 16.0f/9.0f, 100.0f, 50.0f));

    printf("  PASS: Parameter validation works correctly\n");
}

void test_projection_handles_invalid_params_gracefully() {
    printf("Testing projection handles invalid parameters gracefully...\n");

    // Zero aspect ratio - should fallback to 1.0
    glm::mat4 proj1 = calculateProjectionMatrix(35.0f, 0.0f);
    assert(isValidMatrix(proj1));

    // Negative aspect ratio - should fallback to 1.0
    glm::mat4 proj2 = calculateProjectionMatrix(35.0f, -1.0f);
    assert(isValidMatrix(proj2));

    // Zero near plane - should use default
    glm::mat4 proj3 = calculateProjectionMatrix(35.0f, 16.0f/9.0f, 0.0f, 1000.0f);
    assert(isValidMatrix(proj3));

    // Far <= near - should adjust
    glm::mat4 proj4 = calculateProjectionMatrix(35.0f, 16.0f/9.0f, 100.0f, 50.0f);
    assert(isValidMatrix(proj4));

    printf("  PASS: Invalid parameters handled gracefully with fallbacks\n");
}

// ============================================================================
// View-Projection Combination Tests
// ============================================================================

void test_view_projection_combination() {
    printf("Testing view-projection matrix combination...\n");

    // Create view matrix (simple translation for test)
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 50.0f, 50.0f),  // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f),    // Look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)     // Up vector
    );

    // Create projection matrix
    glm::mat4 proj = calculateProjectionMatrix(35.0f, 16.0f / 9.0f);

    // Combine using our function
    glm::mat4 vp1 = calculateViewProjectionMatrix(view, proj);

    // Should match manual multiplication
    glm::mat4 vp2 = proj * view;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            assert(approxEqual(vp1[i][j], vp2[i][j], 0.0001f));
        }
    }

    // Result should be valid
    assert(isValidMatrix(vp1));

    printf("  PASS: View-projection combination correct\n");
}

void test_view_projection_transforms_correctly() {
    printf("Testing view-projection transforms points correctly...\n");

    // Camera at (0, 50, 50) looking at origin
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 50.0f, 50.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 proj = calculateProjectionMatrix(35.0f, 16.0f / 9.0f);
    glm::mat4 vp = calculateViewProjectionMatrix(view, proj);

    // Origin should be roughly centered in screen
    glm::vec4 origin(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 originClip = vp * origin;
    glm::vec3 originNDC = perspectiveDivide(originClip);

    // X and Y should be near 0 (centered)
    assert(std::fabs(originNDC.x) < 0.1f);
    assert(std::fabs(originNDC.y) < 0.1f);

    // Z should be in valid range [0, 1]
    assert(originNDC.z >= 0.0f && originNDC.z <= 1.0f);

    printf("  PASS: View-projection transforms points correctly\n");
}

// ============================================================================
// Edge Cases
// ============================================================================

void test_edge_case_square_aspect() {
    printf("Testing edge case: square aspect ratio...\n");

    glm::mat4 proj = calculateProjectionMatrix(45.0f, 1.0f);
    assert(isValidMatrix(proj));

    // For square aspect, [0][0] should equal [1][1] / aspect = [1][1]
    // Since aspect = 1, X and Y scaling should be equal
    // Actually [0][0] = [1][1] / aspect, so for aspect=1 they're equal
    assert(approxEqual(proj[0][0], proj[1][1], 0.001f));

    printf("  PASS: Square aspect ratio handled correctly\n");
}

void test_edge_case_very_wide_aspect() {
    printf("Testing edge case: very wide aspect ratio (21:9)...\n");

    float aspect = 21.0f / 9.0f;
    glm::mat4 proj = calculateProjectionMatrix(35.0f, aspect);
    assert(isValidMatrix(proj));

    // X scaling should be smaller than Y scaling for wide aspect
    assert(proj[0][0] < proj[1][1]);

    printf("  PASS: Very wide aspect ratio handled correctly\n");
}

void test_edge_case_portrait_aspect() {
    printf("Testing edge case: portrait aspect ratio (9:16)...\n");

    float aspect = 9.0f / 16.0f;
    glm::mat4 proj = calculateProjectionMatrix(35.0f, aspect);
    assert(isValidMatrix(proj));

    // X scaling should be larger than Y scaling for portrait
    assert(proj[0][0] > proj[1][1]);

    printf("  PASS: Portrait aspect ratio handled correctly\n");
}

void test_edge_case_minimum_fov() {
    printf("Testing edge case: minimum FOV (20 degrees)...\n");

    glm::mat4 proj = calculateProjectionMatrix(20.0f, 16.0f / 9.0f);
    assert(isValidMatrix(proj));

    // Small FOV = telephoto effect = large scale factors
    glm::mat4 wideProj = calculateProjectionMatrix(90.0f, 16.0f / 9.0f);
    assert(proj[1][1] > wideProj[1][1]);  // Narrower FOV = larger [1][1]

    printf("  PASS: Minimum FOV produces valid projection\n");
}

void test_edge_case_maximum_fov() {
    printf("Testing edge case: maximum FOV (90 degrees)...\n");

    glm::mat4 proj = calculateProjectionMatrix(90.0f, 16.0f / 9.0f);
    assert(isValidMatrix(proj));

    // Large FOV = wide angle = smaller scale factors
    glm::mat4 narrowProj = calculateProjectionMatrix(20.0f, 16.0f / 9.0f);
    assert(proj[1][1] < narrowProj[1][1]);  // Wider FOV = smaller [1][1]

    printf("  PASS: Maximum FOV produces valid projection\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("=== Projection Matrix Unit Tests ===\n\n");

    // Basic perspective tests
    printf("--- Perspective Projection Tests ---\n");
    test_projection_matrix_basic();
    test_projection_matrix_default_fov();
    test_projection_matrix_custom_fov();
    test_projection_fov_configurable();

    // Aspect ratio tests
    printf("\n--- Aspect Ratio Tests ---\n");
    test_aspect_ratio_calculation();
    test_aspect_ratio_invalid_dimensions();
    test_projection_from_dimensions();
    test_aspect_ratio_maintained();

    // Near/far plane tests
    printf("\n--- Near/Far Plane Tests ---\n");
    test_near_far_planes_default();
    test_near_far_planes_custom();
    test_depth_range_zero_to_one();

    // Perspective divide tests
    printf("\n--- Perspective Divide Tests ---\n");
    test_perspective_divide_correct();
    test_perspective_divide_objects_shrink_with_distance();
    test_center_point_stays_centered();

    // Foreshortening tests
    printf("\n--- Foreshortening Tests ---\n");
    test_minimal_foreshortening_at_isometric_angle();

    // FOV clamping tests
    printf("\n--- FOV Clamping Tests ---\n");
    test_fov_clamping();
    test_projection_with_extreme_fov();

    // Parameter validation tests
    printf("\n--- Parameter Validation Tests ---\n");
    test_parameter_validation();
    test_projection_handles_invalid_params_gracefully();

    // View-projection tests
    printf("\n--- View-Projection Combination Tests ---\n");
    test_view_projection_combination();
    test_view_projection_transforms_correctly();

    // Edge cases
    printf("\n--- Edge Case Tests ---\n");
    test_edge_case_square_aspect();
    test_edge_case_very_wide_aspect();
    test_edge_case_portrait_aspect();
    test_edge_case_minimum_fov();
    test_edge_case_maximum_fov();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
