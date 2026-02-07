/**
 * @file test_viewport_bounds.cpp
 * @brief Unit tests for viewport bounds calculation and map boundary clamping.
 *
 * Tests cover:
 * - GridRect construction and operations
 * - FrustumFootprint AABB and point containment
 * - Frustum footprint calculation from camera state
 * - Visible tile range calculation
 * - Soft boundary deceleration calculation
 * - Focus point clamping to map boundaries
 * - Boundary deceleration application
 * - Utility functions (worldToGrid, gridToWorld)
 */

#include "sims3000/render/ViewportBounds.h"
#include "sims3000/render/CameraState.h"
#include "sims3000/render/ViewMatrix.h"
#include "sims3000/render/ProjectionMatrix.h"
#include <glm/glm.hpp>
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

/// Check if float is valid (not NaN or Inf)
bool isValidFloat(float f) {
    return !std::isnan(f) && !std::isinf(f);
}

// ============================================================================
// GridRect Tests
// ============================================================================

void test_GridRect_DefaultConstruction() {
    printf("Testing GridRect default construction...\n");

    GridRect rect;

    assert(rect.min.x == 0);
    assert(rect.min.y == 0);
    assert(rect.max.x == 0);
    assert(rect.max.y == 0);
    assert(rect.width() == 1);  // Inclusive: 0-0 has width 1
    assert(rect.height() == 1);
    assert(rect.tileCount() == 1);
    assert(rect.isValid());

    printf("  PASS: Default construction creates unit rect at origin\n");
}

void test_GridRect_ExplicitConstruction() {
    printf("Testing GridRect explicit construction...\n");

    // From explicit coordinates
    GridRect rect1(10, 20, 50, 80);
    assert(rect1.min.x == 10);
    assert(rect1.min.y == 20);
    assert(rect1.max.x == 50);
    assert(rect1.max.y == 80);
    assert(rect1.width() == 41);   // 50 - 10 + 1
    assert(rect1.height() == 61);  // 80 - 20 + 1
    assert(rect1.tileCount() == 41 * 61);

    // From GridPosition
    GridRect rect2({5, 10}, {15, 25});
    assert(rect2.min.x == 5);
    assert(rect2.min.y == 10);
    assert(rect2.max.x == 15);
    assert(rect2.max.y == 25);

    printf("  PASS: Explicit construction works correctly\n");
}

void test_GridRect_Contains() {
    printf("Testing GridRect contains...\n");

    GridRect rect(10, 10, 20, 20);

    // Inside
    assert(rect.contains({15, 15}));
    assert(rect.contains({10, 10}));  // Min corner
    assert(rect.contains({20, 20}));  // Max corner

    // Outside
    assert(!rect.contains({5, 15}));
    assert(!rect.contains({25, 15}));
    assert(!rect.contains({15, 5}));
    assert(!rect.contains({15, 25}));

    printf("  PASS: Contains test works correctly\n");
}

void test_GridRect_Overlaps() {
    printf("Testing GridRect overlaps...\n");

    GridRect rect(10, 10, 20, 20);

    // Overlapping
    assert(rect.overlaps({15, 15, 25, 25}));  // Partial overlap
    assert(rect.overlaps({5, 5, 15, 15}));    // Partial overlap
    assert(rect.overlaps({12, 12, 18, 18}));  // Fully inside
    assert(rect.overlaps({5, 5, 25, 25}));    // Fully contains

    // Not overlapping
    assert(!rect.overlaps({25, 25, 30, 30}));  // To the right and below
    assert(!rect.overlaps({0, 0, 5, 5}));      // To the left and above

    // Edge touching counts as overlap
    assert(rect.overlaps({20, 20, 30, 30}));  // Touches corner

    printf("  PASS: Overlaps test works correctly\n");
}

void test_GridRect_StaticAssert() {
    printf("Testing GridRect size...\n");

    static_assert(sizeof(GridRect) == 8, "GridRect must be 8 bytes");

    printf("  PASS: GridRect is 8 bytes\n");
}

// ============================================================================
// FrustumFootprint Tests
// ============================================================================

void test_FrustumFootprint_AABB() {
    printf("Testing FrustumFootprint AABB calculation...\n");

    FrustumFootprint footprint;
    footprint.corners[0] = glm::vec3(10.0f, 0.0f, 20.0f);
    footprint.corners[1] = glm::vec3(50.0f, 0.0f, 25.0f);
    footprint.corners[2] = glm::vec3(60.0f, 0.0f, 80.0f);
    footprint.corners[3] = glm::vec3(5.0f, 0.0f, 75.0f);

    glm::vec4 aabb = footprint.getAABB();

    assert(approxEqual(aabb.x, 5.0f));   // minX
    assert(approxEqual(aabb.y, 20.0f));  // minZ
    assert(approxEqual(aabb.z, 60.0f));  // maxX
    assert(approxEqual(aabb.w, 80.0f));  // maxZ

    printf("  PASS: AABB calculated correctly\n");
}

void test_FrustumFootprint_ContainsPoint() {
    printf("Testing FrustumFootprint point containment...\n");

    // Create a simple square footprint
    FrustumFootprint footprint;
    footprint.corners[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    footprint.corners[1] = glm::vec3(10.0f, 0.0f, 0.0f);
    footprint.corners[2] = glm::vec3(10.0f, 0.0f, 10.0f);
    footprint.corners[3] = glm::vec3(0.0f, 0.0f, 10.0f);

    // Inside
    assert(footprint.containsPoint(5.0f, 5.0f));
    assert(footprint.containsPoint(1.0f, 1.0f));
    assert(footprint.containsPoint(9.0f, 9.0f));

    // Outside
    assert(!footprint.containsPoint(-1.0f, 5.0f));
    assert(!footprint.containsPoint(11.0f, 5.0f));
    assert(!footprint.containsPoint(5.0f, -1.0f));
    assert(!footprint.containsPoint(5.0f, 11.0f));

    printf("  PASS: Point containment works correctly\n");
}

void test_FrustumFootprint_IsValid() {
    printf("Testing FrustumFootprint validity...\n");

    FrustumFootprint valid;
    valid.corners[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    valid.corners[1] = glm::vec3(10.0f, 0.0f, 0.0f);
    valid.corners[2] = glm::vec3(10.0f, 0.0f, 10.0f);
    valid.corners[3] = glm::vec3(0.0f, 0.0f, 10.0f);
    assert(valid.isValid());

    FrustumFootprint invalid;
    invalid.corners[0] = glm::vec3(std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f);
    invalid.corners[1] = glm::vec3(10.0f, 0.0f, 0.0f);
    invalid.corners[2] = glm::vec3(10.0f, 0.0f, 10.0f);
    invalid.corners[3] = glm::vec3(0.0f, 0.0f, 10.0f);
    assert(!invalid.isValid());

    printf("  PASS: Validity check works correctly\n");
}

// ============================================================================
// MapBoundary Tests
// ============================================================================

void test_MapBoundary_Construction() {
    printf("Testing MapBoundary construction...\n");

    // Default (medium)
    MapBoundary defaultBoundary;
    assert(defaultBoundary.width == 256);
    assert(defaultBoundary.height == 256);
    assert(defaultBoundary.sizeTier == MapSizeTier::Medium);

    // From tier
    MapBoundary small(MapSizeTier::Small);
    assert(small.width == 128);
    assert(small.height == 128);

    MapBoundary large(MapSizeTier::Large);
    assert(large.width == 512);
    assert(large.height == 512);

    // Explicit dimensions
    MapBoundary custom(300, 400);
    assert(custom.width == 300);
    assert(custom.height == 400);

    printf("  PASS: MapBoundary construction works correctly\n");
}

void test_MapBoundary_Center() {
    printf("Testing MapBoundary center calculation...\n");

    MapBoundary medium;
    glm::vec3 center = medium.getCenter();

    assert(approxEqual(center.x, 128.0f));
    assert(approxEqual(center.y, 0.0f));
    assert(approxEqual(center.z, 128.0f));

    MapBoundary small(MapSizeTier::Small);
    center = small.getCenter();
    assert(approxEqual(center.x, 64.0f));
    assert(approxEqual(center.z, 64.0f));

    printf("  PASS: Map center calculated correctly\n");
}

void test_MapBoundary_Bounds() {
    printf("Testing MapBoundary bounds...\n");

    MapBoundary boundary;
    boundary.width = 256;
    boundary.height = 256;
    boundary.maxOvershoot = 2.0f;

    glm::vec2 minBound = boundary.getMinBound();
    glm::vec2 maxBound = boundary.getMaxBound();

    assert(approxEqual(minBound.x, -2.0f));
    assert(approxEqual(minBound.y, -2.0f));
    assert(approxEqual(maxBound.x, 258.0f));  // 256 + 2
    assert(approxEqual(maxBound.y, 258.0f));

    printf("  PASS: Map bounds calculated correctly\n");
}

// ============================================================================
// Frustum Footprint Calculation Tests
// ============================================================================

void test_calculateFrustumFootprint_Basic() {
    printf("Testing frustum footprint calculation...\n");

    // Set up camera looking at map center from isometric angle
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    float fov = CameraConfig::FOV_DEFAULT;
    float aspect = 16.0f / 9.0f;

    FrustumFootprint footprint = calculateFrustumFootprint(camera, fov, aspect);

    // Should be valid
    assert(footprint.isValid());

    // All corners should be on ground plane (Y = 0)
    for (const auto& corner : footprint.corners) {
        assert(approxEqual(corner.y, 0.0f, 0.1f));
    }

    // AABB should contain the focus point
    glm::vec4 aabb = footprint.getAABB();
    assert(aabb.x < camera.focus_point.x);  // minX < focus.x
    assert(aabb.z > camera.focus_point.x);  // maxX > focus.x
    assert(aabb.y < camera.focus_point.z);  // minZ < focus.z
    assert(aabb.w > camera.focus_point.z);  // maxZ > focus.z

    printf("  PASS: Frustum footprint calculated correctly\n");
}

void test_calculateFrustumFootprint_DifferentAngles() {
    printf("Testing frustum footprint at different yaw angles...\n");

    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;

    float fov = CameraConfig::FOV_DEFAULT;
    float aspect = 16.0f / 9.0f;

    // Test all four preset angles
    float yaws[] = {
        CameraConfig::PRESET_N_YAW,
        CameraConfig::PRESET_E_YAW,
        CameraConfig::PRESET_S_YAW,
        CameraConfig::PRESET_W_YAW
    };

    for (float yaw : yaws) {
        camera.yaw = yaw;
        FrustumFootprint footprint = calculateFrustumFootprint(camera, fov, aspect);

        assert(footprint.isValid());

        // Focus point should be approximately in the center of the footprint
        glm::vec4 aabb = footprint.getAABB();
        float centerX = (aabb.x + aabb.z) / 2.0f;
        float centerZ = (aabb.y + aabb.w) / 2.0f;

        // Allow some tolerance for trapezoid shape
        assert(std::fabs(centerX - camera.focus_point.x) < 30.0f);
        assert(std::fabs(centerZ - camera.focus_point.z) < 30.0f);
    }

    printf("  PASS: Frustum footprint correct at all angles\n");
}

void test_calculateFrustumFootprint_ZoomLevels() {
    printf("Testing frustum footprint at different zoom levels...\n");

    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    float fov = CameraConfig::FOV_DEFAULT;
    float aspect = 16.0f / 9.0f;

    // Test various distances
    float distances[] = {10.0f, 30.0f, 50.0f, 100.0f, 150.0f};
    float prevArea = 0.0f;

    for (float dist : distances) {
        camera.distance = dist;
        FrustumFootprint footprint = calculateFrustumFootprint(camera, fov, aspect);

        assert(footprint.isValid());

        glm::vec4 aabb = footprint.getAABB();
        float area = (aabb.z - aabb.x) * (aabb.w - aabb.y);

        // Larger distance should give larger visible area
        if (prevArea > 0.0f) {
            assert(area > prevArea);
        }
        prevArea = area;
    }

    printf("  PASS: Visible area scales with distance\n");
}

// ============================================================================
// Visible Tile Range Tests
// ============================================================================

void test_getVisibleTileRange_Basic() {
    printf("Testing visible tile range calculation...\n");

    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    MapBoundary boundary(MapSizeTier::Medium);
    float fov = CameraConfig::FOV_DEFAULT;
    float aspect = 16.0f / 9.0f;

    GridRect visible = getVisibleTileRange(camera, fov, aspect, boundary);

    // Should be valid
    assert(visible.isValid());

    // Should contain the focus point tile
    GridPosition focusTile = worldToGrid(camera.focus_point.x, camera.focus_point.z);
    assert(visible.contains(focusTile));

    // Should be within map bounds
    assert(visible.min.x >= 0);
    assert(visible.min.y >= 0);
    assert(visible.max.x < boundary.width);
    assert(visible.max.y < boundary.height);

    printf("  PASS: Visible tile range calculated correctly\n");
}

void test_getVisibleTileRange_EdgeOfMap() {
    printf("Testing visible tile range at map edge...\n");

    CameraState camera;
    camera.focus_point = glm::vec3(10.0f, 0.0f, 10.0f);  // Near corner
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    MapBoundary boundary(MapSizeTier::Medium);
    float fov = CameraConfig::FOV_DEFAULT;
    float aspect = 16.0f / 9.0f;

    GridRect visible = getVisibleTileRange(camera, fov, aspect, boundary);

    // Should be clamped to map bounds
    assert(visible.min.x >= 0);
    assert(visible.min.y >= 0);
    assert(visible.max.x < boundary.width);
    assert(visible.max.y < boundary.height);

    printf("  PASS: Tile range clamped at map edge\n");
}

void test_getVisibleTileRange_UpdatesWithZoom() {
    printf("Testing visible tile range updates with zoom...\n");

    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    MapBoundary boundary(MapSizeTier::Medium);
    float fov = CameraConfig::FOV_DEFAULT;
    float aspect = 16.0f / 9.0f;

    // Close zoom
    camera.distance = 20.0f;
    GridRect closeRange = getVisibleTileRange(camera, fov, aspect, boundary);

    // Far zoom
    camera.distance = 100.0f;
    GridRect farRange = getVisibleTileRange(camera, fov, aspect, boundary);

    // Far zoom should have more tiles
    assert(farRange.tileCount() > closeRange.tileCount());

    printf("  PASS: Visible range updates with zoom\n");
}

// ============================================================================
// Boundary Deceleration Tests
// ============================================================================

void test_calculateBoundaryDeceleration_Center() {
    printf("Testing boundary deceleration at center...\n");

    // At center of map, should be full speed
    float decel = calculateBoundaryDeceleration(
        128.0f,  // position at center
        0.0f,    // minBound
        256.0f,  // maxBound
        16.0f);  // softMargin

    assert(approxEqual(decel, 1.0f));

    printf("  PASS: Full speed at map center\n");
}

void test_calculateBoundaryDeceleration_SoftZone() {
    printf("Testing boundary deceleration in soft zone...\n");

    float softMargin = 16.0f;

    // At edge of soft zone (16 tiles from edge)
    float decelAtEdge = calculateBoundaryDeceleration(
        softMargin,  // position at soft zone edge
        0.0f,
        256.0f,
        softMargin);
    assert(approxEqual(decelAtEdge, 1.0f, 0.01f));

    // Halfway through soft zone
    float decelMid = calculateBoundaryDeceleration(
        softMargin / 2.0f,  // 8 tiles from edge
        0.0f,
        256.0f,
        softMargin);
    // Should be between 0 and 1
    assert(decelMid > 0.0f && decelMid < 1.0f);

    // At hard boundary
    float decelAtBoundary = calculateBoundaryDeceleration(
        0.0f,  // at boundary
        0.0f,
        256.0f,
        softMargin);
    assert(approxEqual(decelAtBoundary, ViewportConfig::MIN_DECELERATION_FACTOR));

    printf("  PASS: Deceleration varies in soft zone\n");
}

void test_calculateBoundaryDeceleration_2D() {
    printf("Testing 2D boundary deceleration...\n");

    MapBoundary boundary(MapSizeTier::Medium);

    // At center
    glm::vec3 center(128.0f, 0.0f, 128.0f);
    float decelCenter = calculateBoundaryDeceleration(center, boundary);
    assert(approxEqual(decelCenter, 1.0f));

    // Near X edge
    glm::vec3 nearX(5.0f, 0.0f, 128.0f);
    float decelNearX = calculateBoundaryDeceleration(nearX, boundary);
    assert(decelNearX < 1.0f);

    // Near Z edge
    glm::vec3 nearZ(128.0f, 0.0f, 5.0f);
    float decelNearZ = calculateBoundaryDeceleration(nearZ, boundary);
    assert(decelNearZ < 1.0f);

    // Near corner (should use minimum of both)
    glm::vec3 nearCorner(5.0f, 0.0f, 5.0f);
    float decelCorner = calculateBoundaryDeceleration(nearCorner, boundary);
    assert(decelCorner < decelNearX || decelCorner < decelNearZ);

    printf("  PASS: 2D deceleration uses minimum of axes\n");
}

void test_calculateBoundaryDeceleration_SmoothCurve() {
    printf("Testing deceleration curve smoothness...\n");

    float softMargin = 16.0f;
    float prevDecel = 0.0f;

    // Sample deceleration at multiple points approaching center
    for (float pos = 0.0f; pos <= softMargin; pos += 1.0f) {
        float decel = calculateBoundaryDeceleration(
            pos, 0.0f, 256.0f, softMargin);

        // Deceleration should increase as we move away from boundary
        assert(decel >= prevDecel);
        prevDecel = decel;
    }

    printf("  PASS: Deceleration curve is monotonically increasing\n");
}

// ============================================================================
// Focus Point Clamping Tests
// ============================================================================

void test_clampFocusPointToBoundary_Inside() {
    printf("Testing focus point clamping when inside...\n");

    MapBoundary boundary(MapSizeTier::Medium);
    glm::vec3 inside(100.0f, 0.0f, 150.0f);

    glm::vec3 clamped = clampFocusPointToBoundary(inside, boundary);

    // Should be unchanged
    assert(approxEqual(clamped, inside));

    printf("  PASS: Inside focus point unchanged\n");
}

void test_clampFocusPointToBoundary_Outside() {
    printf("Testing focus point clamping when outside...\n");

    MapBoundary boundary(MapSizeTier::Medium);
    boundary.maxOvershoot = 2.0f;

    // Beyond X max
    glm::vec3 outsideX(300.0f, 0.0f, 128.0f);
    glm::vec3 clampedX = clampFocusPointToBoundary(outsideX, boundary);
    assert(clampedX.x <= boundary.width + boundary.maxOvershoot);
    assert(approxEqual(clampedX.z, 128.0f));

    // Beyond Z min
    glm::vec3 outsideZ(128.0f, 0.0f, -50.0f);
    glm::vec3 clampedZ = clampFocusPointToBoundary(outsideZ, boundary);
    assert(clampedZ.z >= -boundary.maxOvershoot);
    assert(approxEqual(clampedZ.x, 128.0f));

    // Y should be preserved
    glm::vec3 withY(300.0f, 5.0f, 128.0f);
    glm::vec3 clampedY = clampFocusPointToBoundary(withY, boundary);
    assert(approxEqual(clampedY.y, 5.0f));

    printf("  PASS: Outside focus point clamped correctly\n");
}

// ============================================================================
// Soft Boundary Zone Tests
// ============================================================================

void test_isInSoftBoundaryZone() {
    printf("Testing soft boundary zone detection...\n");

    MapBoundary boundary(MapSizeTier::Medium);

    // Center - not in zone
    assert(!isInSoftBoundaryZone({128.0f, 0.0f, 128.0f}, boundary));

    // Near edge - in zone
    assert(isInSoftBoundaryZone({5.0f, 0.0f, 128.0f}, boundary));
    assert(isInSoftBoundaryZone({128.0f, 0.0f, 250.0f}, boundary));

    printf("  PASS: Soft boundary zone detection works\n");
}

void test_isAtHardBoundary() {
    printf("Testing hard boundary detection...\n");

    MapBoundary boundary(MapSizeTier::Medium);
    boundary.maxOvershoot = 2.0f;

    // Center - not at boundary
    assert(!isAtHardBoundary({128.0f, 0.0f, 128.0f}, boundary));

    // At hard boundary
    assert(isAtHardBoundary({-2.0f, 0.0f, 128.0f}, boundary));
    assert(isAtHardBoundary({258.0f, 0.0f, 128.0f}, boundary));

    printf("  PASS: Hard boundary detection works\n");
}

// ============================================================================
// Velocity Deceleration Tests
// ============================================================================

void test_applyBoundaryDeceleration_Center() {
    printf("Testing velocity deceleration at center...\n");

    MapBoundary boundary(MapSizeTier::Medium);
    glm::vec3 center(128.0f, 0.0f, 128.0f);
    glm::vec3 velocity(10.0f, 0.0f, 10.0f);

    glm::vec3 adjusted = applyBoundaryDeceleration(center, velocity, boundary);

    // At center, velocity should be unchanged
    assert(approxEqual(adjusted, velocity));

    printf("  PASS: Velocity unchanged at center\n");
}

void test_applyBoundaryDeceleration_NearEdge() {
    printf("Testing velocity deceleration near edge...\n");

    MapBoundary boundary(MapSizeTier::Medium);

    // Near left edge, moving left (towards boundary)
    glm::vec3 nearEdge(5.0f, 0.0f, 128.0f);
    glm::vec3 moveTowards(-10.0f, 0.0f, 0.0f);

    glm::vec3 adjusted = applyBoundaryDeceleration(nearEdge, moveTowards, boundary);

    // Movement towards boundary should be slowed
    assert(std::fabs(adjusted.x) < std::fabs(moveTowards.x));

    printf("  PASS: Movement towards boundary is slowed\n");
}

void test_applyBoundaryDeceleration_MovingAway() {
    printf("Testing velocity deceleration when moving away from edge...\n");

    MapBoundary boundary(MapSizeTier::Medium);

    // Near left edge, moving right (away from boundary)
    glm::vec3 nearEdge(5.0f, 0.0f, 128.0f);
    glm::vec3 moveAway(10.0f, 0.0f, 0.0f);

    glm::vec3 adjusted = applyBoundaryDeceleration(nearEdge, moveAway, boundary);

    // Movement away from boundary should not be slowed
    assert(approxEqual(adjusted.x, moveAway.x));

    printf("  PASS: Movement away from boundary not slowed\n");
}

// ============================================================================
// Utility Function Tests
// ============================================================================

void test_worldToGrid() {
    printf("Testing worldToGrid conversion...\n");

    // Exact integers
    GridPosition p1 = worldToGrid(10.0f, 20.0f);
    assert(p1.x == 10);
    assert(p1.y == 20);

    // Fractional (should floor)
    GridPosition p2 = worldToGrid(10.7f, 20.9f);
    assert(p2.x == 10);
    assert(p2.y == 20);

    // Negative (should floor)
    GridPosition p3 = worldToGrid(-5.5f, -3.2f);
    assert(p3.x == -6);
    assert(p3.y == -4);

    printf("  PASS: worldToGrid works correctly\n");
}

void test_gridToWorld() {
    printf("Testing gridToWorld conversion...\n");

    // Returns center of tile
    glm::vec3 w1 = gridToWorld({10, 20});
    assert(approxEqual(w1.x, 10.5f));
    assert(approxEqual(w1.y, 0.0f));
    assert(approxEqual(w1.z, 20.5f));

    // With height
    glm::vec3 w2 = gridToWorld({10, 20}, 5.0f);
    assert(approxEqual(w2.y, 5.0f));

    printf("  PASS: gridToWorld works correctly\n");
}

void test_expandGridRect() {
    printf("Testing GridRect expansion...\n");

    MapBoundary boundary(MapSizeTier::Medium);
    GridRect rect(50, 50, 100, 100);

    GridRect expanded = expandGridRect(rect, 5, boundary);

    assert(expanded.min.x == 45);
    assert(expanded.min.y == 45);
    assert(expanded.max.x == 105);
    assert(expanded.max.y == 105);

    // Test clamping at boundary
    GridRect nearEdge(5, 5, 10, 10);
    GridRect clampedExpand = expandGridRect(nearEdge, 10, boundary);

    assert(clampedExpand.min.x == 0);
    assert(clampedExpand.min.y == 0);
    assert(clampedExpand.max.x == 20);
    assert(clampedExpand.max.y == 20);

    printf("  PASS: GridRect expansion works correctly\n");
}

void test_getDirectionToMapCenter() {
    printf("Testing direction to map center...\n");

    MapBoundary boundary(MapSizeTier::Medium);  // 256x256, center at (128, 128)

    // From corner
    glm::vec3 fromCorner = getDirectionToMapCenter({0.0f, 0.0f, 0.0f}, boundary);
    assert(fromCorner.x > 0.0f);  // Should point towards positive X
    assert(fromCorner.z > 0.0f);  // Should point towards positive Z
    assert(approxEqual(glm::length(fromCorner), 1.0f));  // Normalized

    // From center (should be zero)
    glm::vec3 fromCenter = getDirectionToMapCenter(boundary.getCenter(), boundary);
    assert(approxEqual(glm::length(fromCenter), 0.0f, 0.01f));

    printf("  PASS: Direction to center calculated correctly\n");
}

// ============================================================================
// Configuration Tests
// ============================================================================

void test_ViewportConfig_MapDimensions() {
    printf("Testing ViewportConfig map dimensions...\n");

    assert(ViewportConfig::getMapDimension(MapSizeTier::Small) == 128);
    assert(ViewportConfig::getMapDimension(MapSizeTier::Medium) == 256);
    assert(ViewportConfig::getMapDimension(MapSizeTier::Large) == 512);

    printf("  PASS: Map dimensions correct for all tiers\n");
}

void test_ViewportConfig_Defaults() {
    printf("Testing ViewportConfig defaults...\n");

    assert(ViewportConfig::DEFAULT_MAP_SIZE == MapSizeTier::Medium);
    assert(ViewportConfig::SOFT_BOUNDARY_MARGIN > 0.0f);
    assert(ViewportConfig::MAX_BOUNDARY_OVERSHOOT >= 0.0f);
    assert(ViewportConfig::CULLING_PADDING > 0);

    printf("  PASS: ViewportConfig defaults are reasonable\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("=== ViewportBounds Unit Tests ===\n\n");

    // GridRect tests
    printf("--- GridRect Tests ---\n");
    test_GridRect_DefaultConstruction();
    test_GridRect_ExplicitConstruction();
    test_GridRect_Contains();
    test_GridRect_Overlaps();
    test_GridRect_StaticAssert();
    printf("\n");

    // FrustumFootprint tests
    printf("--- FrustumFootprint Tests ---\n");
    test_FrustumFootprint_AABB();
    test_FrustumFootprint_ContainsPoint();
    test_FrustumFootprint_IsValid();
    printf("\n");

    // MapBoundary tests
    printf("--- MapBoundary Tests ---\n");
    test_MapBoundary_Construction();
    test_MapBoundary_Center();
    test_MapBoundary_Bounds();
    printf("\n");

    // Frustum calculation tests
    printf("--- Frustum Calculation Tests ---\n");
    test_calculateFrustumFootprint_Basic();
    test_calculateFrustumFootprint_DifferentAngles();
    test_calculateFrustumFootprint_ZoomLevels();
    printf("\n");

    // Visible tile range tests
    printf("--- Visible Tile Range Tests ---\n");
    test_getVisibleTileRange_Basic();
    test_getVisibleTileRange_EdgeOfMap();
    test_getVisibleTileRange_UpdatesWithZoom();
    printf("\n");

    // Boundary deceleration tests
    printf("--- Boundary Deceleration Tests ---\n");
    test_calculateBoundaryDeceleration_Center();
    test_calculateBoundaryDeceleration_SoftZone();
    test_calculateBoundaryDeceleration_2D();
    test_calculateBoundaryDeceleration_SmoothCurve();
    printf("\n");

    // Focus point clamping tests
    printf("--- Focus Point Clamping Tests ---\n");
    test_clampFocusPointToBoundary_Inside();
    test_clampFocusPointToBoundary_Outside();
    printf("\n");

    // Soft boundary tests
    printf("--- Soft Boundary Tests ---\n");
    test_isInSoftBoundaryZone();
    test_isAtHardBoundary();
    printf("\n");

    // Velocity deceleration tests
    printf("--- Velocity Deceleration Tests ---\n");
    test_applyBoundaryDeceleration_Center();
    test_applyBoundaryDeceleration_NearEdge();
    test_applyBoundaryDeceleration_MovingAway();
    printf("\n");

    // Utility function tests
    printf("--- Utility Function Tests ---\n");
    test_worldToGrid();
    test_gridToWorld();
    test_expandGridRect();
    test_getDirectionToMapCenter();
    printf("\n");

    // Configuration tests
    printf("--- Configuration Tests ---\n");
    test_ViewportConfig_MapDimensions();
    test_ViewportConfig_Defaults();
    printf("\n");

    printf("=== All ViewportBounds Tests Passed! ===\n");

    return 0;
}
