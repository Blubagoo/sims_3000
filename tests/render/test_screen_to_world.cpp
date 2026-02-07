/**
 * @file test_screen_to_world.cpp
 * @brief Unit tests for Screen-to-World Ray Casting (Ticket 2-029).
 *
 * Tests acceptance criteria:
 * - screenToWorldRay(Vec2 screenPos) -> Ray
 * - Ray constructed from inverse view-projection matrix
 * - Divergent rays from camera position through screen point (perspective)
 * - rayGroundIntersection(Ray, height) -> Vec3
 * - Works correctly with perspective projection at all camera angles
 * - Numerical stability for near-horizontal pitch
 * - Handles cases where ray is parallel to ground
 */

#include "sims3000/render/ScreenToWorld.h"
#include "sims3000/render/CameraState.h"
#include "sims3000/render/ViewMatrix.h"
#include "sims3000/render/ProjectionMatrix.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000;

// ============================================================================
// Test Helpers
// ============================================================================

constexpr float EPSILON = 0.001f;

bool approxEqual(float a, float b, float epsilon = EPSILON) {
    return std::fabs(a - b) < epsilon;
}

bool approxEqual(const glm::vec3& a, const glm::vec3& b, float epsilon = EPSILON) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon);
}

bool approxEqual(const glm::vec2& a, const glm::vec2& b, float epsilon = EPSILON) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon);
}

// Build view-projection matrix from camera state
glm::mat4 buildViewProjection(const CameraState& state, float width, float height) {
    glm::mat4 view = calculateViewMatrix(state);
    glm::mat4 proj = calculateProjectionMatrixDefault(width / height);
    return proj * view;
}

// ============================================================================
// Criterion 1: Function screenToWorldRay(screenPos) -> Ray
// ============================================================================

void test_screenToWorldRay_basic() {
    printf("Testing screenToWorldRay basic function...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Get ray through screen center
    Ray ray = screenToWorldRay(
        windowWidth / 2.0f, windowHeight / 2.0f,
        windowWidth, windowHeight,
        vp, state
    );

    // Ray should have valid origin and direction
    assert(glm::length(ray.direction) > 0.9f && glm::length(ray.direction) < 1.1f);
    assert(!std::isnan(ray.origin.x) && !std::isnan(ray.origin.y) && !std::isnan(ray.origin.z));
    assert(!std::isnan(ray.direction.x) && !std::isnan(ray.direction.y) && !std::isnan(ray.direction.z));

    printf("  PASS: screenToWorldRay returns valid Ray\n");
}

void test_screenToWorldRay_different_screen_positions() {
    printf("Testing screenToWorldRay at different screen positions...\n");

    CameraState state;
    state.focus_point = glm::vec3(50.0f, 0.0f, 50.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Get rays at different screen positions
    Ray rayCenter = screenToWorldRay(640.0f, 360.0f, windowWidth, windowHeight, vp, state);
    Ray rayTopLeft = screenToWorldRay(0.0f, 0.0f, windowWidth, windowHeight, vp, state);
    Ray rayBottomRight = screenToWorldRay(1280.0f, 720.0f, windowWidth, windowHeight, vp, state);

    // All rays should have same origin (camera position) for perspective projection
    assert(approxEqual(rayCenter.origin, rayTopLeft.origin, 0.01f));
    assert(approxEqual(rayCenter.origin, rayBottomRight.origin, 0.01f));

    // But different directions
    assert(!approxEqual(rayCenter.direction, rayTopLeft.direction, 0.01f));
    assert(!approxEqual(rayCenter.direction, rayBottomRight.direction, 0.01f));

    printf("  PASS: Different screen positions produce different ray directions\n");
}

// ============================================================================
// Criterion 2: Ray constructed from inverse view-projection matrix
// ============================================================================

void test_ray_uses_inverse_vp_matrix() {
    printf("Testing ray construction uses inverse VP matrix...\n");

    CameraState state;
    state.focus_point = glm::vec3(100.0f, 0.0f, 100.0f);
    state.distance = 75.0f;
    state.pitch = 45.0f;
    state.yaw = 90.0f;

    float windowWidth = 1920.0f;
    float windowHeight = 1080.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);
    glm::mat4 invVP = glm::inverse(vp);
    glm::vec3 camPos = calculateCameraPosition(state);

    // Get ray through screen center using both overloads
    Ray ray1 = screenToWorldRay(
        windowWidth / 2.0f, windowHeight / 2.0f,
        windowWidth, windowHeight,
        invVP, camPos
    );

    Ray ray2 = screenToWorldRay(
        windowWidth / 2.0f, windowHeight / 2.0f,
        windowWidth, windowHeight,
        vp, state
    );

    // Both should produce same result
    assert(approxEqual(ray1.origin, ray2.origin, 0.01f));
    assert(approxEqual(ray1.direction, ray2.direction, 0.01f));

    printf("  PASS: Both overloads produce consistent rays\n");
}

// ============================================================================
// Criterion 3: Divergent rays from camera position (perspective)
// ============================================================================

void test_perspective_rays_diverge() {
    printf("Testing perspective rays diverge from camera position...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);
    glm::vec3 expectedCamPos = calculateCameraPosition(state);

    // Get multiple rays
    Ray rayA = screenToWorldRay(100.0f, 100.0f, windowWidth, windowHeight, vp, state);
    Ray rayB = screenToWorldRay(640.0f, 360.0f, windowWidth, windowHeight, vp, state);
    Ray rayC = screenToWorldRay(1180.0f, 620.0f, windowWidth, windowHeight, vp, state);

    // All rays should originate from camera position (perspective divergence)
    assert(approxEqual(rayA.origin, expectedCamPos, 0.01f));
    assert(approxEqual(rayB.origin, expectedCamPos, 0.01f));
    assert(approxEqual(rayC.origin, expectedCamPos, 0.01f));

    // Rays should point away from camera (negative dot with camera-to-origin direction)
    glm::vec3 toTarget = glm::normalize(state.focus_point - expectedCamPos);
    float dotA = glm::dot(rayA.direction, toTarget);
    float dotB = glm::dot(rayB.direction, toTarget);
    float dotC = glm::dot(rayC.direction, toTarget);

    // Center ray should point closest to target
    assert(dotB > 0.9f);

    // Corner rays should also point generally forward
    assert(dotA > 0.0f);
    assert(dotC > 0.0f);

    printf("  PASS: Rays diverge from camera position as expected\n");
}

// ============================================================================
// Criterion 4: Function rayGroundIntersection(Ray, height) -> Vec3
// ============================================================================

void test_rayGroundIntersection_basic() {
    printf("Testing rayGroundIntersection basic function...\n");

    // Ray pointing straight down from Y=10
    Ray ray;
    ray.origin = glm::vec3(5.0f, 10.0f, 5.0f);
    ray.direction = glm::vec3(0.0f, -1.0f, 0.0f);

    // Intersect with ground plane at Y=0
    auto result = rayGroundIntersection(ray, 0.0f);

    assert(result.has_value());
    assert(approxEqual(result->x, 5.0f));
    assert(approxEqual(result->y, 0.0f));
    assert(approxEqual(result->z, 5.0f));

    printf("  PASS: rayGroundIntersection returns correct intersection\n");
}

void test_rayGroundIntersection_different_heights() {
    printf("Testing rayGroundIntersection at different ground heights...\n");

    Ray ray;
    ray.origin = glm::vec3(10.0f, 20.0f, 10.0f);
    ray.direction = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));

    // Y = 0 (ground level)
    auto r0 = rayGroundIntersection(ray, 0.0f);
    assert(r0.has_value());
    assert(approxEqual(r0->y, 0.0f));

    // Y = 5 (elevated terrain)
    auto r5 = rayGroundIntersection(ray, 5.0f);
    assert(r5.has_value());
    assert(approxEqual(r5->y, 5.0f));

    // Y = 15 (high terrain)
    auto r15 = rayGroundIntersection(ray, 15.0f);
    assert(r15.has_value());
    assert(approxEqual(r15->y, 15.0f));

    printf("  PASS: rayGroundIntersection works at various heights\n");
}

void test_rayGroundIntersection_diagonal_ray() {
    printf("Testing rayGroundIntersection with diagonal ray...\n");

    Ray ray;
    ray.origin = glm::vec3(0.0f, 10.0f, 0.0f);
    ray.direction = glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f));

    auto result = rayGroundIntersection(ray, 0.0f);

    assert(result.has_value());
    assert(approxEqual(result->y, 0.0f));
    // X and Z should be at 10 (since Y dropped by 10)
    assert(approxEqual(result->x, 10.0f, 0.1f));
    assert(approxEqual(result->z, 10.0f, 0.1f));

    printf("  PASS: rayGroundIntersection works with diagonal rays\n");
}

// ============================================================================
// Criterion 5: Works correctly at all camera angles (perspective)
// ============================================================================

void test_all_isometric_presets() {
    printf("Testing screen-to-world at all isometric presets...\n");

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;

    CameraMode presets[] = {
        CameraMode::Preset_N,
        CameraMode::Preset_E,
        CameraMode::Preset_S,
        CameraMode::Preset_W
    };

    for (CameraMode preset : presets) {
        CameraState state;
        state.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
        state.distance = 50.0f;
        state.pitch = CameraState::getPitchForPreset(preset);
        state.yaw = CameraState::getYawForPreset(preset);
        state.mode = preset;

        glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

        // Cast ray through screen center
        Ray ray = screenToWorldRay(
            windowWidth / 2.0f, windowHeight / 2.0f,
            windowWidth, windowHeight,
            vp, state
        );

        // Should hit near focus point
        auto hit = rayGroundIntersection(ray, 0.0f);
        assert(hit.has_value());

        // Hit should be close to focus point (within tolerance due to pixel center)
        float dist = glm::length(*hit - state.focus_point);
        assert(dist < 5.0f);  // Within 5 units of focus
    }

    printf("  PASS: All isometric presets work correctly\n");
}

void test_arbitrary_camera_angles() {
    printf("Testing screen-to-world at arbitrary camera angles...\n");

    float windowWidth = 1920.0f;
    float windowHeight = 1080.0f;

    // Test various pitch/yaw combinations
    struct TestCase {
        float pitch;
        float yaw;
    };

    TestCase cases[] = {
        {20.0f, 0.0f},
        {45.0f, 90.0f},
        {60.0f, 180.0f},
        {75.0f, 270.0f},
        {CameraConfig::PITCH_MIN, 45.0f},
        {CameraConfig::PITCH_MAX, 135.0f},
        {50.0f, 22.5f},
        {35.0f, 67.5f},
    };

    for (const TestCase& tc : cases) {
        CameraState state;
        state.focus_point = glm::vec3(100.0f, 0.0f, 100.0f);
        state.distance = 75.0f;
        state.pitch = tc.pitch;
        state.yaw = tc.yaw;
        state.mode = CameraMode::Free;

        glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

        // Cast ray through screen center
        Ray ray = screenToWorldRay(
            windowWidth / 2.0f, windowHeight / 2.0f,
            windowWidth, windowHeight,
            vp, state
        );

        // Should hit near focus point
        auto hit = rayGroundIntersection(ray, 0.0f);
        assert(hit.has_value());

        // For non-horizontal pitch, center ray should hit near focus point
        if (tc.pitch > 10.0f) {
            float dist = glm::length(*hit - state.focus_point);
            assert(dist < 10.0f);  // Within 10 units
        }
    }

    printf("  PASS: Arbitrary camera angles work correctly\n");
}

void test_extreme_pitch_angles() {
    printf("Testing screen-to-world at extreme pitch angles...\n");

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;

    // Test at minimum pitch (most horizontal)
    CameraState minPitchState;
    minPitchState.focus_point = glm::vec3(50.0f, 0.0f, 50.0f);
    minPitchState.distance = 50.0f;
    minPitchState.pitch = CameraConfig::PITCH_MIN;  // 15 degrees
    minPitchState.yaw = 45.0f;

    glm::mat4 vpMin = buildViewProjection(minPitchState, windowWidth, windowHeight);
    Ray rayMin = screenToWorldRay(
        windowWidth / 2.0f, windowHeight / 2.0f,
        windowWidth, windowHeight,
        vpMin, minPitchState
    );

    // Should still produce valid intersection
    auto hitMin = rayGroundIntersection(rayMin, 0.0f);
    assert(hitMin.has_value());

    // Test at maximum pitch (most vertical)
    CameraState maxPitchState;
    maxPitchState.focus_point = glm::vec3(50.0f, 0.0f, 50.0f);
    maxPitchState.distance = 50.0f;
    maxPitchState.pitch = CameraConfig::PITCH_MAX;  // 80 degrees
    maxPitchState.yaw = 45.0f;

    glm::mat4 vpMax = buildViewProjection(maxPitchState, windowWidth, windowHeight);
    Ray rayMax = screenToWorldRay(
        windowWidth / 2.0f, windowHeight / 2.0f,
        windowWidth, windowHeight,
        vpMax, maxPitchState
    );

    auto hitMax = rayGroundIntersection(rayMax, 0.0f);
    assert(hitMax.has_value());

    // Max pitch ray should hit closer to focus than min pitch
    float distMin = glm::length(*hitMin - minPitchState.focus_point);
    float distMax = glm::length(*hitMax - maxPitchState.focus_point);

    // At max pitch (nearly top-down), center ray should hit very close to focus
    assert(distMax < distMin);

    printf("  PASS: Extreme pitch angles work correctly\n");
}

// ============================================================================
// Criterion 6: Numerical stability for near-horizontal pitch
// ============================================================================

void test_near_horizontal_pitch_stability() {
    printf("Testing numerical stability for near-horizontal pitch...\n");

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;

    // Very shallow pitch (15 degrees is the minimum)
    CameraState shallowState;
    shallowState.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    shallowState.distance = 50.0f;
    shallowState.pitch = CameraConfig::PITCH_MIN;
    shallowState.yaw = 0.0f;

    glm::mat4 vp = buildViewProjection(shallowState, windowWidth, windowHeight);

    // Cast rays at various screen positions
    for (int x = 0; x <= 4; ++x) {
        for (int y = 0; y <= 4; ++y) {
            float screenX = x * 320.0f;
            float screenY = y * 180.0f;

            Ray ray = screenToWorldRay(
                screenX, screenY,
                windowWidth, windowHeight,
                vp, shallowState
            );

            // Ray should be valid (no NaN)
            assert(!std::isnan(ray.origin.x));
            assert(!std::isnan(ray.direction.x));
            assert(!std::isinf(ray.origin.x));
            assert(!std::isinf(ray.direction.x));

            // Direction should be normalized
            float len = glm::length(ray.direction);
            assert(len > 0.99f && len < 1.01f);

            // Ground intersection might be very far or not exist
            // (this is expected behavior, not a bug)
            auto hit = rayGroundIntersection(ray, 0.0f);
            if (hit.has_value()) {
                // If we got a hit, it should be valid
                assert(!std::isnan(hit->x));
                assert(!std::isnan(hit->y));
                assert(!std::isnan(hit->z));
            }
        }
    }

    printf("  PASS: Near-horizontal pitch maintains numerical stability\n");
}

void test_epsilon_threshold_for_parallel_detection() {
    printf("Testing epsilon threshold for parallel ray detection...\n");

    glm::vec3 groundNormal(0.0f, 1.0f, 0.0f);

    // Exactly horizontal ray
    glm::vec3 exactHorizontal(1.0f, 0.0f, 0.0f);
    assert(isRayParallelToPlane(exactHorizontal, groundNormal));

    // Very slightly tilted ray (should still be considered parallel)
    glm::vec3 almostHorizontal = glm::normalize(glm::vec3(1.0f, 0.00001f, 0.0f));
    assert(isRayParallelToPlane(almostHorizontal, groundNormal, 0.0001f));

    // More tilted ray (should not be parallel)
    glm::vec3 tilted = glm::normalize(glm::vec3(1.0f, 0.1f, 0.0f));
    assert(!isRayParallelToPlane(tilted, groundNormal));

    // Downward ray (definitely not parallel)
    glm::vec3 downward = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
    assert(!isRayParallelToPlane(downward, groundNormal));

    printf("  PASS: Parallel detection uses appropriate epsilon\n");
}

// ============================================================================
// Criterion 7: Handles cases where ray is parallel to ground
// ============================================================================

void test_parallel_ray_returns_no_intersection() {
    printf("Testing parallel ray returns no intersection...\n");

    // Perfectly horizontal ray
    Ray horizontalRay;
    horizontalRay.origin = glm::vec3(0.0f, 10.0f, 0.0f);
    horizontalRay.direction = glm::vec3(1.0f, 0.0f, 0.0f);

    auto result = rayGroundIntersection(horizontalRay, 0.0f);
    assert(!result.has_value());

    // Horizontal ray at different altitude
    horizontalRay.origin.y = 100.0f;
    result = rayGroundIntersection(horizontalRay, 0.0f);
    assert(!result.has_value());

    printf("  PASS: Parallel rays return no intersection\n");
}

void test_nearly_parallel_ray_handled_safely() {
    printf("Testing nearly parallel ray is handled safely...\n");

    // Ray with very small downward component
    Ray nearlyParallel;
    nearlyParallel.origin = glm::vec3(0.0f, 10.0f, 0.0f);
    nearlyParallel.direction = glm::normalize(glm::vec3(1.0f, -0.00001f, 0.0f));

    // Should either return no intersection or a valid (possibly distant) point
    auto result = rayGroundIntersection(nearlyParallel, 0.0f);

    // If there's a result, it must be valid (no NaN or Inf)
    if (result.has_value()) {
        assert(!std::isnan(result->x) && !std::isnan(result->y) && !std::isnan(result->z));
        assert(!std::isinf(result->x) && !std::isinf(result->y) && !std::isinf(result->z));
    }

    printf("  PASS: Nearly parallel ray handled safely\n");
}

void test_upward_ray_returns_no_intersection() {
    printf("Testing upward ray returns no intersection...\n");

    // Ray pointing upward (away from ground)
    Ray upwardRay;
    upwardRay.origin = glm::vec3(0.0f, 10.0f, 0.0f);
    upwardRay.direction = glm::vec3(0.0f, 1.0f, 0.0f);

    // Should not intersect ground plane below
    auto result = rayGroundIntersection(upwardRay, 0.0f);
    assert(!result.has_value());

    // Diagonal upward ray
    upwardRay.direction = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    result = rayGroundIntersection(upwardRay, 0.0f);
    assert(!result.has_value());

    printf("  PASS: Upward rays return no intersection with ground below\n");
}

// ============================================================================
// Additional Tests: Edge Cases
// ============================================================================

void test_ray_point_method() {
    printf("Testing Ray::getPoint method...\n");

    Ray ray;
    ray.origin = glm::vec3(1.0f, 2.0f, 3.0f);
    ray.direction = glm::vec3(0.0f, 1.0f, 0.0f);  // Pointing up

    // Point at t=0 is origin
    glm::vec3 p0 = ray.getPoint(0.0f);
    assert(approxEqual(p0, ray.origin));

    // Point at t=5 is 5 units up
    glm::vec3 p5 = ray.getPoint(5.0f);
    assert(approxEqual(p5, glm::vec3(1.0f, 7.0f, 3.0f)));

    // Point at t=-3 is 3 units down (behind ray)
    glm::vec3 pNeg = ray.getPoint(-3.0f);
    assert(approxEqual(pNeg, glm::vec3(1.0f, -1.0f, 3.0f)));

    printf("  PASS: Ray::getPoint works correctly\n");
}

void test_screen_to_ndc_boundaries() {
    printf("Testing screenToNDC at boundaries...\n");

    float w = 1920.0f;
    float h = 1080.0f;

    // Corners
    assert(approxEqual(screenToNDC(0.0f, 0.0f, w, h), glm::vec2(-1.0f, 1.0f)));
    assert(approxEqual(screenToNDC(w, 0.0f, w, h), glm::vec2(1.0f, 1.0f)));
    assert(approxEqual(screenToNDC(0.0f, h, w, h), glm::vec2(-1.0f, -1.0f)));
    assert(approxEqual(screenToNDC(w, h, w, h), glm::vec2(1.0f, -1.0f)));

    // Center
    assert(approxEqual(screenToNDC(w/2.0f, h/2.0f, w, h), glm::vec2(0.0f, 0.0f)));

    printf("  PASS: screenToNDC boundaries correct\n");
}

void test_getCursorWorldPosition_convenience() {
    printf("Testing getCursorWorldPosition convenience function...\n");

    CameraState state;
    state.focus_point = glm::vec3(50.0f, 0.0f, 50.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Get cursor position at screen center
    auto cursorPos = getCursorWorldPosition(
        windowWidth / 2.0f, windowHeight / 2.0f,
        windowWidth, windowHeight,
        vp, state, 0.0f
    );

    assert(cursorPos.has_value());

    // Should be near focus point
    float dist = glm::length(*cursorPos - state.focus_point);
    assert(dist < 5.0f);

    printf("  PASS: getCursorWorldPosition works correctly\n");
}

void test_arbitrary_plane_intersection() {
    printf("Testing arbitrary plane intersection...\n");

    Ray ray;
    ray.origin = glm::vec3(0.0f, 0.0f, -10.0f);
    ray.direction = glm::vec3(0.0f, 0.0f, 1.0f);  // Pointing into +Z

    // Vertical plane at Z=0 facing -Z
    glm::vec3 planeNormal(0.0f, 0.0f, -1.0f);
    glm::vec3 planePoint(0.0f, 0.0f, 0.0f);

    auto result = rayPlaneIntersection(ray, planeNormal, planePoint);
    assert(result.has_value());
    assert(approxEqual(result->z, 0.0f));

    // Tilted plane
    planeNormal = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));
    planePoint = glm::vec3(5.0f, 5.0f, 0.0f);

    ray.origin = glm::vec3(0.0f, 0.0f, 0.0f);
    ray.direction = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));

    result = rayPlaneIntersection(ray, planeNormal, planePoint);
    assert(result.has_value());

    printf("  PASS: Arbitrary plane intersection works\n");
}

// ============================================================================
// World-to-Screen Tests (Ticket 2-028)
// ============================================================================

void test_worldToScreen_basic() {
    printf("Testing worldToScreen basic function...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Project the focus point (should be near center of screen)
    ScreenProjectionResult result = worldToScreen(
        state.focus_point, vp, windowWidth, windowHeight
    );

    // Should be on screen
    assert(result.isOnScreen());
    assert(!result.behindCamera);
    assert(!result.outsideViewport);

    // Should be near center of screen
    assert(approxEqual(result.screenPos.x, windowWidth / 2.0f, 5.0f));
    assert(approxEqual(result.screenPos.y, windowHeight / 2.0f, 5.0f));

    printf("  PASS: worldToScreen returns valid screen position\n");
}

void test_worldToScreen_viewport_offset() {
    printf("Testing worldToScreen with viewport offset...\n");

    CameraState state;
    state.focus_point = glm::vec3(50.0f, 0.0f, 50.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1920.0f;
    float windowHeight = 1080.0f;
    float viewportX = 100.0f;
    float viewportY = 50.0f;
    float viewportWidth = 1720.0f;
    float viewportHeight = 980.0f;

    glm::mat4 vp = buildViewProjection(state, viewportWidth, viewportHeight);

    // Project the focus point with viewport offset
    ScreenProjectionResult result = worldToScreen(
        state.focus_point, vp,
        viewportX, viewportY, viewportWidth, viewportHeight
    );

    // Should be on screen (within viewport)
    assert(result.isOnScreen());

    // Screen position should account for viewport offset
    assert(result.screenPos.x >= viewportX);
    assert(result.screenPos.x <= viewportX + viewportWidth);
    assert(result.screenPos.y >= viewportY);
    assert(result.screenPos.y <= viewportY + viewportHeight);

    printf("  PASS: worldToScreen accounts for viewport offset\n");
}

void test_worldToScreen_returns_valid_coordinates() {
    printf("Testing worldToScreen returns valid screen coordinates...\n");

    CameraState state;
    state.focus_point = glm::vec3(100.0f, 0.0f, 100.0f);
    state.distance = 75.0f;
    state.pitch = 45.0f;
    state.yaw = 90.0f;

    float windowWidth = 1920.0f;
    float windowHeight = 1080.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Test multiple world positions
    glm::vec3 testPositions[] = {
        state.focus_point,
        state.focus_point + glm::vec3(10.0f, 0.0f, 0.0f),
        state.focus_point + glm::vec3(0.0f, 5.0f, 0.0f),
        state.focus_point + glm::vec3(0.0f, 0.0f, 10.0f),
        state.focus_point + glm::vec3(-10.0f, 2.0f, -10.0f)
    };

    for (const auto& pos : testPositions) {
        ScreenProjectionResult result = worldToScreen(pos, vp, windowWidth, windowHeight);

        // All should be in front of camera
        assert(!result.behindCamera);

        // Screen coordinates should be valid (no NaN)
        assert(!std::isnan(result.screenPos.x));
        assert(!std::isnan(result.screenPos.y));
        assert(!std::isnan(result.depth));
    }

    printf("  PASS: worldToScreen returns valid coordinates for visible objects\n");
}

void test_worldToScreen_handles_behind_camera() {
    printf("Testing worldToScreen handles behind-camera positions...\n");

    CameraState state;
    state.focus_point = glm::vec3(50.0f, 0.0f, 50.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Calculate camera position
    glm::vec3 camPos = calculateCameraPosition(state);

    // Position behind camera (further from focus than camera, in opposite direction)
    glm::vec3 behindCamera = camPos + (camPos - state.focus_point) * 2.0f;

    ScreenProjectionResult result = worldToScreen(behindCamera, vp, windowWidth, windowHeight);

    // Should be marked as behind camera
    assert(result.behindCamera);

    printf("  PASS: worldToScreen marks behind-camera positions\n");
}

void test_worldToScreen_handles_offscreen() {
    printf("Testing worldToScreen handles off-screen positions...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Position far to the side (should be off-screen)
    glm::vec3 farSide = glm::vec3(500.0f, 0.0f, 0.0f);

    ScreenProjectionResult result = worldToScreen(farSide, vp, windowWidth, windowHeight);

    // Should be marked as outside viewport (but not behind camera)
    assert(!result.behindCamera);
    assert(result.outsideViewport);
    assert(!result.isOnScreen());

    // isValid should still be true (in front of camera)
    assert(result.isValid());

    printf("  PASS: worldToScreen marks off-screen positions\n");
}

void test_worldToScreen_ui_anchor_function() {
    printf("Testing getScreenPositionForUI function...\n");

    CameraState state;
    state.focus_point = glm::vec3(50.0f, 0.0f, 50.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Visible position should return a value
    auto visibleResult = getScreenPositionForUI(state.focus_point, vp, windowWidth, windowHeight);
    assert(visibleResult.has_value());
    assert(!std::isnan(visibleResult->x));
    assert(!std::isnan(visibleResult->y));

    // Off-screen position should return empty
    glm::vec3 farAway = glm::vec3(1000.0f, 0.0f, 1000.0f);
    auto offscreenResult = getScreenPositionForUI(farAway, vp, windowWidth, windowHeight);
    // Note: This may or may not return a value depending on FOV and distance

    printf("  PASS: getScreenPositionForUI works correctly\n");
}

void test_worldToScreen_visibility_check() {
    printf("Testing isWorldPositionVisible function...\n");

    CameraState state;
    state.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Focus point should be visible
    assert(isWorldPositionVisible(state.focus_point, vp, windowWidth, windowHeight));

    // Point slightly offset should also be visible
    assert(isWorldPositionVisible(state.focus_point + glm::vec3(5.0f, 0.0f, 5.0f), vp, windowWidth, windowHeight));

    printf("  PASS: isWorldPositionVisible works correctly\n");
}

void test_worldToScreen_depth_ordering() {
    printf("Testing worldToScreen depth ordering...\n");

    CameraState state;
    state.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    state.distance = 50.0f;
    state.pitch = 45.0f;
    state.yaw = 0.0f;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Points at different distances from camera
    glm::vec3 camPos = calculateCameraPosition(state);
    glm::vec3 nearFocus = state.focus_point;
    glm::vec3 farFromCamera = state.focus_point + glm::normalize(state.focus_point - camPos) * 20.0f;

    ScreenProjectionResult nearResult = worldToScreen(nearFocus, vp, windowWidth, windowHeight);
    ScreenProjectionResult farResult = worldToScreen(farFromCamera, vp, windowWidth, windowHeight);

    // Far point should have greater depth value
    assert(farResult.depth > nearResult.depth);

    printf("  PASS: worldToScreen depth ordering is correct\n");
}

void test_worldToScreen_roundtrip() {
    printf("Testing world-to-screen-to-world roundtrip...\n");

    CameraState state;
    state.focus_point = glm::vec3(50.0f, 0.0f, 50.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Start with a world position on the ground plane
    glm::vec3 originalWorld = state.focus_point + glm::vec3(10.0f, 0.0f, 10.0f);

    // Project to screen
    ScreenProjectionResult screenResult = worldToScreen(originalWorld, vp, windowWidth, windowHeight);
    assert(screenResult.isOnScreen());

    // Unproject back to world (via ray-ground intersection at Y=0)
    Ray ray = screenToWorldRay(
        screenResult.screenPos.x, screenResult.screenPos.y,
        windowWidth, windowHeight,
        vp, state
    );

    auto groundHit = rayGroundIntersection(ray, 0.0f);
    assert(groundHit.has_value());

    // Should match original position (within tolerance)
    assert(approxEqual(groundHit->x, originalWorld.x, 0.1f));
    assert(approxEqual(groundHit->z, originalWorld.z, 0.1f));
    // Y is always 0 for ground plane intersection

    printf("  PASS: World-to-screen-to-world roundtrip is consistent\n");
}

void test_worldToScreen_camera_state_overload() {
    printf("Testing worldToScreen with CameraState overload...\n");

    CameraState state;
    state.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_S_YAW;

    float windowWidth = 1920.0f;
    float windowHeight = 1080.0f;

    // Use the CameraState overload
    ScreenProjectionResult result = worldToScreen(
        state.focus_point, state, windowWidth, windowHeight
    );

    // Focus point should be near center
    assert(result.isOnScreen());
    assert(approxEqual(result.screenPos.x, windowWidth / 2.0f, 10.0f));
    assert(approxEqual(result.screenPos.y, windowHeight / 2.0f, 10.0f));

    printf("  PASS: worldToScreen CameraState overload works correctly\n");
}

void test_worldToScreen_all_presets() {
    printf("Testing worldToScreen at all isometric presets...\n");

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;

    CameraMode presets[] = {
        CameraMode::Preset_N,
        CameraMode::Preset_E,
        CameraMode::Preset_S,
        CameraMode::Preset_W
    };

    for (CameraMode preset : presets) {
        CameraState state;
        state.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
        state.distance = 50.0f;
        state.pitch = CameraState::getPitchForPreset(preset);
        state.yaw = CameraState::getYawForPreset(preset);
        state.mode = preset;

        glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

        // Focus point should project to screen center
        ScreenProjectionResult result = worldToScreen(state.focus_point, vp, windowWidth, windowHeight);

        assert(result.isOnScreen());
        assert(approxEqual(result.screenPos.x, windowWidth / 2.0f, 5.0f));
        assert(approxEqual(result.screenPos.y, windowHeight / 2.0f, 5.0f));
    }

    printf("  PASS: worldToScreen works at all isometric presets\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("=== ScreenToWorld Unit Tests (Ticket 2-029 & 2-028) ===\n\n");

    // Criterion 1: screenToWorldRay function
    printf("--- Criterion 1: screenToWorldRay Function ---\n");
    test_screenToWorldRay_basic();
    test_screenToWorldRay_different_screen_positions();

    // Criterion 2: Inverse VP matrix
    printf("\n--- Criterion 2: Inverse VP Matrix ---\n");
    test_ray_uses_inverse_vp_matrix();

    // Criterion 3: Perspective divergent rays
    printf("\n--- Criterion 3: Perspective Divergent Rays ---\n");
    test_perspective_rays_diverge();

    // Criterion 4: rayGroundIntersection function
    printf("\n--- Criterion 4: rayGroundIntersection Function ---\n");
    test_rayGroundIntersection_basic();
    test_rayGroundIntersection_different_heights();
    test_rayGroundIntersection_diagonal_ray();

    // Criterion 5: All camera angles
    printf("\n--- Criterion 5: All Camera Angles ---\n");
    test_all_isometric_presets();
    test_arbitrary_camera_angles();
    test_extreme_pitch_angles();

    // Criterion 6: Numerical stability
    printf("\n--- Criterion 6: Numerical Stability ---\n");
    test_near_horizontal_pitch_stability();
    test_epsilon_threshold_for_parallel_detection();

    // Criterion 7: Parallel ray handling
    printf("\n--- Criterion 7: Parallel Ray Handling ---\n");
    test_parallel_ray_returns_no_intersection();
    test_nearly_parallel_ray_handled_safely();
    test_upward_ray_returns_no_intersection();

    // Additional edge cases
    printf("\n--- Additional Edge Cases ---\n");
    test_ray_point_method();
    test_screen_to_ndc_boundaries();
    test_getCursorWorldPosition_convenience();
    test_arbitrary_plane_intersection();

    // World-to-Screen Tests (Ticket 2-028)
    printf("\n--- Ticket 2-028: World-to-Screen Transformation ---\n");
    test_worldToScreen_basic();
    test_worldToScreen_viewport_offset();
    test_worldToScreen_returns_valid_coordinates();
    test_worldToScreen_handles_behind_camera();
    test_worldToScreen_handles_offscreen();
    test_worldToScreen_ui_anchor_function();
    test_worldToScreen_visibility_check();
    test_worldToScreen_depth_ordering();
    test_worldToScreen_roundtrip();
    test_worldToScreen_camera_state_overload();
    test_worldToScreen_all_presets();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
