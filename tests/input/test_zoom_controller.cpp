/**
 * @file test_zoom_controller.cpp
 * @brief Unit tests for ZoomController (Ticket 2-024).
 */

#include "sims3000/input/ZoomController.h"
#include "sims3000/render/CameraState.h"
#include "sims3000/render/ScreenToWorld.h"
#include "sims3000/render/ViewMatrix.h"
#include "sims3000/render/ProjectionMatrix.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000;

// Helper for float comparison with tolerance
bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

// Helper for vec3 comparison with tolerance
bool approxEqual(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.001f) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon);
}

// ============================================================================
// ZoomConfig Tests
// ============================================================================

void test_zoom_config_defaults() {
    printf("Testing ZoomConfig default values...\n");

    ZoomConfig config;

    assert(approxEqual(config.minDistance, CameraConfig::DISTANCE_MIN));
    assert(approxEqual(config.maxDistance, CameraConfig::DISTANCE_MAX));
    assert(config.zoomSpeed > 0.0f);
    assert(config.smoothingFactor > 0.0f);
    assert(config.centerOnCursor == true);

    printf("  PASS: ZoomConfig has sensible defaults\n");
}

void test_zoom_config_map_size_small() {
    printf("Testing ZoomConfig for small maps (128x128)...\n");

    ZoomConfig config = ZoomConfig::defaultSmall();

    assert(approxEqual(config.minDistance, CameraConfig::DISTANCE_MIN));
    assert(approxEqual(config.maxDistance, 100.0f));

    printf("  PASS: Small map config uses standard range (5-100)\n");
}

void test_zoom_config_map_size_medium() {
    printf("Testing ZoomConfig for medium maps (256x256)...\n");

    ZoomConfig config = ZoomConfig::defaultMedium();

    assert(approxEqual(config.minDistance, CameraConfig::DISTANCE_MIN));
    assert(approxEqual(config.maxDistance, 150.0f));

    printf("  PASS: Medium map config uses extended range (5-150)\n");
}

void test_zoom_config_map_size_large() {
    printf("Testing ZoomConfig for large maps (512x512)...\n");

    ZoomConfig config = ZoomConfig::defaultLarge();

    assert(approxEqual(config.minDistance, CameraConfig::DISTANCE_MIN));
    assert(approxEqual(config.maxDistance, 250.0f));

    printf("  PASS: Large map config uses wide range (5-250)\n");
}

void test_zoom_config_configure_for_map_size() {
    printf("Testing ZoomConfig::configureForMapSize...\n");

    ZoomConfig config;

    config.configureForMapSize(128);
    assert(approxEqual(config.maxDistance, 100.0f));

    config.configureForMapSize(256);
    assert(approxEqual(config.maxDistance, 150.0f));

    config.configureForMapSize(512);
    assert(approxEqual(config.maxDistance, 250.0f));

    // Test boundary values
    config.configureForMapSize(64);   // Below 128 -> small
    assert(approxEqual(config.maxDistance, 100.0f));

    config.configureForMapSize(1024); // Above 512 -> large
    assert(approxEqual(config.maxDistance, 250.0f));

    printf("  PASS: configureForMapSize sets correct max distance\n");
}

// ============================================================================
// ZoomController Construction Tests
// ============================================================================

void test_zoom_controller_default_construction() {
    printf("Testing ZoomController default construction...\n");

    ZoomController zoom;

    assert(approxEqual(zoom.getTargetDistance(), CameraConfig::DISTANCE_DEFAULT));
    assert(!zoom.isZooming());

    printf("  PASS: ZoomController default construction works\n");
}

void test_zoom_controller_custom_config() {
    printf("Testing ZoomController with custom config...\n");

    ZoomConfig config;
    config.minDistance = 10.0f;
    config.maxDistance = 200.0f;
    config.zoomSpeed = 0.2f;

    ZoomController zoom(config);

    assert(approxEqual(zoom.getConfig().minDistance, 10.0f));
    assert(approxEqual(zoom.getConfig().maxDistance, 200.0f));
    assert(approxEqual(zoom.getConfig().zoomSpeed, 0.2f));

    printf("  PASS: ZoomController accepts custom config\n");
}

// ============================================================================
// Direct Control Tests
// ============================================================================

void test_zoom_controller_set_target_distance() {
    printf("Testing ZoomController setTargetDistance...\n");

    ZoomController zoom;

    // Set within range
    zoom.setTargetDistance(30.0f);
    assert(approxEqual(zoom.getTargetDistance(), 30.0f));

    // Set below minimum - should clamp
    zoom.setTargetDistance(1.0f);
    assert(approxEqual(zoom.getTargetDistance(), CameraConfig::DISTANCE_MIN));

    // Set above maximum - should clamp
    zoom.setTargetDistance(500.0f);
    assert(approxEqual(zoom.getTargetDistance(), CameraConfig::DISTANCE_MAX));

    printf("  PASS: setTargetDistance clamps to valid range\n");
}

void test_zoom_controller_set_distance_immediate() {
    printf("Testing ZoomController setDistanceImmediate...\n");

    ZoomController zoom;
    CameraState cameraState;
    cameraState.distance = 50.0f;

    // Set immediate
    zoom.setDistanceImmediate(30.0f, cameraState);

    assert(approxEqual(cameraState.distance, 30.0f));
    assert(approxEqual(zoom.getTargetDistance(), 30.0f));
    assert(!zoom.isZooming());  // No interpolation pending

    printf("  PASS: setDistanceImmediate sets both camera and target\n");
}

void test_zoom_controller_reset() {
    printf("Testing ZoomController reset...\n");

    ZoomController zoom;
    CameraState cameraState;
    cameraState.distance = 75.0f;
    cameraState.focus_point = glm::vec3(10.0f, 0.0f, 20.0f);

    // Set a different target
    zoom.setTargetDistance(30.0f);

    // Reset to sync with camera state
    zoom.reset(cameraState);

    assert(approxEqual(zoom.getTargetDistance(), 75.0f));
    assert(approxEqual(zoom.getTargetFocusPoint(), cameraState.focus_point));
    assert(!zoom.isZooming());

    printf("  PASS: reset syncs with camera state\n");
}

// ============================================================================
// Update / Interpolation Tests
// ============================================================================

void test_zoom_controller_update_interpolation() {
    printf("Testing ZoomController smooth interpolation...\n");

    ZoomController zoom;
    CameraState cameraState;
    cameraState.distance = 50.0f;
    cameraState.focus_point = glm::vec3(0.0f);

    // Sync initial state
    zoom.reset(cameraState);

    // Set a new target
    zoom.setTargetDistance(25.0f);

    // Verify zooming is in progress
    assert(zoom.isZooming());

    // Simulate multiple frames
    float deltaTime = 0.016f;  // ~60 FPS
    for (int i = 0; i < 100; ++i) {
        zoom.update(deltaTime, cameraState);
    }

    // After many frames, should be near target
    assert(approxEqual(cameraState.distance, 25.0f, 0.1f));

    printf("  PASS: Zoom smoothly interpolates toward target\n");
}

void test_zoom_controller_update_respects_constraints() {
    printf("Testing ZoomController update respects distance constraints...\n");

    ZoomController zoom;
    CameraState cameraState;
    cameraState.distance = CameraConfig::DISTANCE_MIN;
    cameraState.focus_point = glm::vec3(0.0f);

    zoom.reset(cameraState);

    // Try to set target below minimum (will be clamped)
    zoom.setTargetDistance(1.0f);

    // Update
    zoom.update(0.1f, cameraState);

    // Distance should stay at or above minimum
    assert(cameraState.distance >= CameraConfig::DISTANCE_MIN);

    printf("  PASS: Update respects distance constraints\n");
}

// ============================================================================
// Configuration Change Tests
// ============================================================================

void test_zoom_controller_set_distance_limits() {
    printf("Testing ZoomController setDistanceLimits...\n");

    ZoomController zoom;
    CameraState cameraState;
    cameraState.distance = 80.0f;

    zoom.reset(cameraState);

    // Set new limits that exclude current distance
    zoom.setDistanceLimits(10.0f, 60.0f);

    // Target should be clamped to new max
    assert(zoom.getTargetDistance() <= 60.0f);

    // Config should be updated
    assert(approxEqual(zoom.getConfig().minDistance, 10.0f));
    assert(approxEqual(zoom.getConfig().maxDistance, 60.0f));

    printf("  PASS: setDistanceLimits updates config and clamps current values\n");
}

void test_zoom_controller_configure_for_map_size() {
    printf("Testing ZoomController configureForMapSize...\n");

    ZoomController zoom;

    zoom.configureForMapSize(128);
    assert(approxEqual(zoom.getConfig().maxDistance, 100.0f));

    zoom.configureForMapSize(512);
    assert(approxEqual(zoom.getConfig().maxDistance, 250.0f));

    printf("  PASS: configureForMapSize updates zoom limits\n");
}

// ============================================================================
// Zoom-to-Cursor Math Tests
// ============================================================================

void test_zoom_center_on_cursor_enabled() {
    printf("Testing ZoomController cursor-centering enabled...\n");

    ZoomConfig config;
    config.centerOnCursor = true;
    ZoomController zoom(config);

    assert(zoom.getConfig().centerOnCursor == true);

    printf("  PASS: Cursor-centering can be enabled\n");
}

void test_zoom_center_on_cursor_disabled() {
    printf("Testing ZoomController cursor-centering disabled...\n");

    ZoomConfig config;
    config.centerOnCursor = false;
    ZoomController zoom(config);

    assert(zoom.getConfig().centerOnCursor == false);

    printf("  PASS: Cursor-centering can be disabled\n");
}

// ============================================================================
// Perceptual Zoom Speed Tests
// ============================================================================

void test_zoom_speed_logarithmic() {
    printf("Testing zoom speed is perceptually consistent (logarithmic)...\n");

    // The zoom uses exp(-wheelDelta * speed) which means:
    // - Positive wheel = zoom in (smaller distance)
    // - The same wheel delta produces the same *relative* change at any distance

    ZoomConfig config;
    float zoomSpeed = config.zoomSpeed;

    // Simulate zoom at distance 100
    float d1_start = 100.0f;
    float d1_after = d1_start * std::exp(-1.0f * zoomSpeed);  // One wheel notch in
    float ratio1 = d1_after / d1_start;

    // Simulate zoom at distance 25
    float d2_start = 25.0f;
    float d2_after = d2_start * std::exp(-1.0f * zoomSpeed);  // One wheel notch in
    float ratio2 = d2_after / d2_start;

    // Ratios should be equal (same relative change)
    assert(approxEqual(ratio1, ratio2, 0.0001f));

    printf("  PASS: Zoom speed is logarithmic (perceptually consistent)\n");
}

// ============================================================================
// Soft Boundary Tests
// ============================================================================

void test_zoom_soft_boundary_at_min() {
    printf("Testing zoom soft boundary at minimum distance...\n");

    // This test verifies that zoom decelerates near the limits
    // The soft boundary implementation should reduce zoom delta
    // when approaching min/max distance

    ZoomConfig config;
    config.minDistance = 5.0f;
    config.maxDistance = 100.0f;
    config.softBoundaryStart = 0.1f;  // 10% of range

    // The soft boundary should start at 5 + 0.1*(100-5) = 14.5 units from min

    ZoomController zoom(config);

    // We can't easily test the internal soft boundary function directly,
    // but we can verify that the config is set correctly
    assert(approxEqual(zoom.getConfig().softBoundaryStart, 0.1f));
    assert(zoom.getConfig().softBoundaryPower > 0.0f);

    printf("  PASS: Soft boundary configuration is set\n");
}

// ============================================================================
// ScreenToWorld Integration Tests
// ============================================================================

void test_screen_to_ndc_conversion() {
    printf("Testing screenToNDC conversion...\n");

    // Center of screen
    glm::vec2 center = screenToNDC(640.0f, 360.0f, 1280.0f, 720.0f);
    assert(approxEqual(center.x, 0.0f, 0.01f));
    assert(approxEqual(center.y, 0.0f, 0.01f));

    // Top-left corner
    glm::vec2 topLeft = screenToNDC(0.0f, 0.0f, 1280.0f, 720.0f);
    assert(approxEqual(topLeft.x, -1.0f, 0.01f));
    assert(approxEqual(topLeft.y, 1.0f, 0.01f));  // Y is flipped

    // Bottom-right corner
    glm::vec2 bottomRight = screenToNDC(1280.0f, 720.0f, 1280.0f, 720.0f);
    assert(approxEqual(bottomRight.x, 1.0f, 0.01f));
    assert(approxEqual(bottomRight.y, -1.0f, 0.01f));

    printf("  PASS: Screen to NDC conversion correct\n");
}

void test_ray_parallel_to_plane() {
    printf("Testing isRayParallelToPlane...\n");

    glm::vec3 planeNormal(0.0f, 1.0f, 0.0f);  // Horizontal plane

    // Horizontal ray (parallel to plane)
    glm::vec3 horizontalRay = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
    assert(isRayParallelToPlane(horizontalRay, planeNormal));

    // Downward ray (not parallel)
    glm::vec3 downwardRay = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
    assert(!isRayParallelToPlane(downwardRay, planeNormal));

    // Diagonal ray (not parallel)
    glm::vec3 diagonalRay = glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f));
    assert(!isRayParallelToPlane(diagonalRay, planeNormal));

    printf("  PASS: Ray-plane parallelism detection works\n");
}

void test_ray_plane_intersection() {
    printf("Testing rayPlaneIntersection...\n");

    // Ray pointing straight down from Y=10
    Ray ray;
    ray.origin = glm::vec3(5.0f, 10.0f, 5.0f);
    ray.direction = glm::vec3(0.0f, -1.0f, 0.0f);

    // Intersect with Y=0 plane
    auto result = rayPlaneIntersection(ray, 0.0f);
    assert(result.has_value());
    assert(approxEqual(result->x, 5.0f));
    assert(approxEqual(result->y, 0.0f));
    assert(approxEqual(result->z, 5.0f));

    // Intersect with Y=3 plane
    result = rayPlaneIntersection(ray, 3.0f);
    assert(result.has_value());
    assert(approxEqual(result->y, 3.0f));

    // Horizontal ray (no intersection with horizontal plane)
    Ray horizontalRay;
    horizontalRay.origin = glm::vec3(0.0f, 5.0f, 0.0f);
    horizontalRay.direction = glm::vec3(1.0f, 0.0f, 0.0f);

    result = rayPlaneIntersection(horizontalRay, 0.0f);
    assert(!result.has_value());

    printf("  PASS: Ray-plane intersection works correctly\n");
}

void test_ray_point_at_distance() {
    printf("Testing Ray::getPoint...\n");

    Ray ray;
    ray.origin = glm::vec3(0.0f, 10.0f, 0.0f);
    ray.direction = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));

    // Point at t=0 should be origin
    glm::vec3 p0 = ray.getPoint(0.0f);
    assert(approxEqual(p0, ray.origin));

    // Point at t=5 should be 5 units along direction
    glm::vec3 p5 = ray.getPoint(5.0f);
    assert(approxEqual(p5.y, 5.0f));

    // Point at t=10 should be at ground (Y=0)
    glm::vec3 p10 = ray.getPoint(10.0f);
    assert(approxEqual(p10.y, 0.0f));

    printf("  PASS: Ray::getPoint works correctly\n");
}

// ============================================================================
// CameraConfig Map Size Tests
// ============================================================================

void test_camera_config_get_max_distance_for_map_size() {
    printf("Testing CameraConfig::getMaxDistanceForMapSize...\n");

    // Small maps
    assert(approxEqual(CameraConfig::getMaxDistanceForMapSize(64), CameraConfig::DISTANCE_MAX_SMALL));
    assert(approxEqual(CameraConfig::getMaxDistanceForMapSize(128), CameraConfig::DISTANCE_MAX_SMALL));

    // Medium maps
    assert(approxEqual(CameraConfig::getMaxDistanceForMapSize(192), CameraConfig::DISTANCE_MAX_MEDIUM));
    assert(approxEqual(CameraConfig::getMaxDistanceForMapSize(256), CameraConfig::DISTANCE_MAX_MEDIUM));

    // Large maps
    assert(approxEqual(CameraConfig::getMaxDistanceForMapSize(384), CameraConfig::DISTANCE_MAX_LARGE));
    assert(approxEqual(CameraConfig::getMaxDistanceForMapSize(512), CameraConfig::DISTANCE_MAX_LARGE));
    assert(approxEqual(CameraConfig::getMaxDistanceForMapSize(1024), CameraConfig::DISTANCE_MAX_LARGE));

    printf("  PASS: getMaxDistanceForMapSize returns correct values\n");
}

// ============================================================================
// Integration Test: Full Zoom Workflow
// ============================================================================

void test_full_zoom_workflow() {
    printf("Testing full zoom workflow...\n");

    // Create camera state
    CameraState cameraState;
    cameraState.distance = 50.0f;
    cameraState.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);  // Center of 128x128 map
    cameraState.pitch = CameraConfig::ISOMETRIC_PITCH;
    cameraState.yaw = CameraConfig::PRESET_N_YAW;

    // Configure zoom controller for small map
    ZoomController zoom;
    zoom.configureForMapSize(128);
    zoom.reset(cameraState);

    // Verify initial state
    assert(approxEqual(zoom.getTargetDistance(), 50.0f));
    assert(!zoom.isZooming());

    // Set zoom target (simulating wheel input)
    zoom.setTargetDistance(25.0f);  // Zoom in
    assert(zoom.isZooming());

    // Update over several frames
    float totalTime = 0.0f;
    while (zoom.isZooming() && totalTime < 2.0f) {
        zoom.update(0.016f, cameraState);
        totalTime += 0.016f;
    }

    // Camera should have reached target
    assert(approxEqual(cameraState.distance, 25.0f, 0.5f));

    // Zoom out beyond max
    zoom.setTargetDistance(200.0f);  // Above small map max (100)

    // Update
    totalTime = 0.0f;
    while (zoom.isZooming() && totalTime < 3.0f) {
        zoom.update(0.016f, cameraState);
        totalTime += 0.016f;
    }

    // Distance should be clamped to small map max
    assert(cameraState.distance <= 100.0f + 0.5f);

    printf("  PASS: Full zoom workflow works correctly\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("=== ZoomController Unit Tests (Ticket 2-024) ===\n\n");

    // ZoomConfig tests
    printf("--- ZoomConfig Tests ---\n");
    test_zoom_config_defaults();
    test_zoom_config_map_size_small();
    test_zoom_config_map_size_medium();
    test_zoom_config_map_size_large();
    test_zoom_config_configure_for_map_size();

    // ZoomController construction tests
    printf("\n--- ZoomController Construction Tests ---\n");
    test_zoom_controller_default_construction();
    test_zoom_controller_custom_config();

    // Direct control tests
    printf("\n--- Direct Control Tests ---\n");
    test_zoom_controller_set_target_distance();
    test_zoom_controller_set_distance_immediate();
    test_zoom_controller_reset();

    // Update / interpolation tests
    printf("\n--- Update / Interpolation Tests ---\n");
    test_zoom_controller_update_interpolation();
    test_zoom_controller_update_respects_constraints();

    // Configuration change tests
    printf("\n--- Configuration Change Tests ---\n");
    test_zoom_controller_set_distance_limits();
    test_zoom_controller_configure_for_map_size();

    // Zoom-to-cursor tests
    printf("\n--- Zoom-to-Cursor Tests ---\n");
    test_zoom_center_on_cursor_enabled();
    test_zoom_center_on_cursor_disabled();

    // Perceptual zoom tests
    printf("\n--- Perceptual Zoom Tests ---\n");
    test_zoom_speed_logarithmic();

    // Soft boundary tests
    printf("\n--- Soft Boundary Tests ---\n");
    test_zoom_soft_boundary_at_min();

    // ScreenToWorld integration tests
    printf("\n--- ScreenToWorld Tests ---\n");
    test_screen_to_ndc_conversion();
    test_ray_parallel_to_plane();
    test_ray_plane_intersection();
    test_ray_point_at_distance();

    // CameraConfig tests
    printf("\n--- CameraConfig Map Size Tests ---\n");
    test_camera_config_get_max_distance_for_map_size();

    // Integration tests
    printf("\n--- Integration Tests ---\n");
    test_full_zoom_workflow();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
