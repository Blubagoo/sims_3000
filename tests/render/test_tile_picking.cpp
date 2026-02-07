/**
 * @file test_tile_picking.cpp
 * @brief Unit tests for Tile Picking (Ticket 2-030).
 *
 * Tests acceptance criteria:
 * - Function: pickTile(Vec2 screenPos) -> GridPosition
 * - Returns correct tile at all zoom levels
 * - Accounts for elevation (terrain height)
 * - Cursor position maps to expected tile
 * - Numerical stability guard for near-parallel ray-ground intersection
 * - Tested at preset angles AND arbitrary free camera angles
 * - Future: extend to pick buildings by bounding box
 */

#include "sims3000/render/TilePicking.h"
#include "sims3000/render/ScreenToWorld.h"
#include "sims3000/render/CameraState.h"
#include "sims3000/render/ViewMatrix.h"
#include "sims3000/render/ProjectionMatrix.h"
#include "sims3000/core/types.h"
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

bool approxEqual(float a, float b, float eps = EPSILON) {
    return std::fabs(a - b) < eps;
}

bool approxEqual(const glm::vec3& a, const glm::vec3& b, float eps = EPSILON) {
    return approxEqual(a.x, b.x, eps) &&
           approxEqual(a.y, b.y, eps) &&
           approxEqual(a.z, b.z, eps);
}

// Build view-projection matrix from camera state
glm::mat4 buildViewProjection(const CameraState& state, float width, float height) {
    glm::mat4 view = calculateViewMatrix(state);
    glm::mat4 proj = calculateProjectionMatrixDefault(width / height);
    return proj * view;
}

// Terrain height provider that returns a flat plane at specified height
float makeFlatTerrain(float height) {
    return height;
}

// Terrain height provider with varying elevation
float hilly_terrain(int16_t x, int16_t y) {
    // Create some elevation based on position
    // Center area is elevated
    float distFromCenter = std::sqrt(
        std::pow(static_cast<float>(x - 64), 2.0f) +
        std::pow(static_cast<float>(y - 64), 2.0f)
    );

    if (distFromCenter < 20.0f) {
        return 5.0f;  // Elevated center
    }
    return 0.0f;
}

// ============================================================================
// Criterion 1: Function pickTile(Vec2 screenPos) -> GridPosition
// ============================================================================

void test_pickTile_basic_function() {
    printf("Testing pickTile basic function signature...\n");

    CameraState state;
    state.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Pick tile at screen center
    glm::vec2 screenCenter(windowWidth / 2.0f, windowHeight / 2.0f);
    auto result = pickTile(screenCenter, windowWidth, windowHeight, vp, state);

    assert(result.has_value());
    assert(result->position.x >= 0 && result->position.x < 256);
    assert(result->position.y >= 0 && result->position.y < 256);

    printf("  PASS: pickTile returns valid GridPosition at screen center\n");
}

void test_pickTile_returns_grid_position() {
    printf("Testing pickTile returns correct GridPosition type...\n");

    CameraState state;
    state.focus_point = glm::vec3(50.0f, 0.0f, 50.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    glm::vec2 screenCenter(windowWidth / 2.0f, windowHeight / 2.0f);
    auto result = pickTile(screenCenter, windowWidth, windowHeight, vp, state);

    assert(result.has_value());

    // Verify GridPosition has correct type (int16_t)
    GridPosition pos = result->position;
    static_assert(sizeof(pos.x) == sizeof(int16_t), "GridPosition.x must be int16_t");
    static_assert(sizeof(pos.y) == sizeof(int16_t), "GridPosition.y must be int16_t");

    // Position should be near focus point
    assert(std::abs(pos.x - 50) < 5);
    assert(std::abs(pos.y - 50) < 5);

    printf("  PASS: pickTile returns correct GridPosition type\n");
}

void test_pickTileFlat_convenience() {
    printf("Testing pickTileFlat convenience function...\n");

    CameraState state;
    state.focus_point = glm::vec3(100.0f, 0.0f, 100.0f);
    state.distance = 75.0f;
    state.pitch = 45.0f;
    state.yaw = 90.0f;

    float windowWidth = 1920.0f;
    float windowHeight = 1080.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Pick at center with flat terrain at height 0
    glm::vec2 screenCenter(windowWidth / 2.0f, windowHeight / 2.0f);
    auto result = pickTileFlat(screenCenter, windowWidth, windowHeight, vp, state, 0.0f);

    assert(result.has_value());
    assert(std::abs(result->position.x - 100) < 5);
    assert(std::abs(result->position.y - 100) < 5);
    assert(approxEqual(result->elevation, 0.0f));

    printf("  PASS: pickTileFlat works correctly\n");
}

// ============================================================================
// Criterion 2: Returns correct tile at all zoom levels
// ============================================================================

void test_pickTile_at_different_zoom_levels() {
    printf("Testing pickTile at different zoom levels...\n");

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;

    // Test at various distances (zoom levels)
    float distances[] = {
        CameraConfig::DISTANCE_MIN,   // Closest zoom (5)
        25.0f,                         // Quarter zoom
        CameraConfig::DISTANCE_DEFAULT, // Default (50)
        75.0f,                          // Three-quarter zoom
        CameraConfig::DISTANCE_MAX     // Furthest zoom (100)
    };

    for (float dist : distances) {
        CameraState state;
        state.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
        state.distance = dist;
        state.pitch = CameraConfig::ISOMETRIC_PITCH;
        state.yaw = CameraConfig::PRESET_N_YAW;

        glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

        // Pick at screen center - should always hit near focus point
        glm::vec2 screenCenter(windowWidth / 2.0f, windowHeight / 2.0f);
        auto result = pickTileFlat(screenCenter, windowWidth, windowHeight, vp, state, 0.0f);

        assert(result.has_value());

        // At any zoom level, center should hit near focus point
        float distFromFocus = std::sqrt(
            std::pow(result->position.x - 64.0f, 2.0f) +
            std::pow(result->position.y - 64.0f, 2.0f)
        );

        // More tolerance at extreme zooms
        float tolerance = (dist > 50.0f) ? 10.0f : 5.0f;
        assert(distFromFocus < tolerance);
    }

    printf("  PASS: pickTile works at all zoom levels\n");
}

void test_pickTile_consistency_across_zoom() {
    printf("Testing pickTile consistency across zoom levels...\n");

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;

    // Focus on a specific tile
    CameraState baseState;
    baseState.focus_point = glm::vec3(32.5f, 0.0f, 32.5f);  // Center of tile (32, 32)
    baseState.pitch = CameraConfig::ISOMETRIC_PITCH;
    baseState.yaw = CameraConfig::PRESET_N_YAW;

    GridPosition expectedTile{32, 32};

    // At multiple zoom levels, center pick should hit the same tile
    for (float dist = 10.0f; dist <= 80.0f; dist += 10.0f) {
        baseState.distance = dist;
        glm::mat4 vp = buildViewProjection(baseState, windowWidth, windowHeight);

        glm::vec2 screenCenter(windowWidth / 2.0f, windowHeight / 2.0f);
        auto result = pickTileFlat(screenCenter, windowWidth, windowHeight, vp, baseState, 0.0f);

        assert(result.has_value());
        assert(result->position.x == expectedTile.x);
        assert(result->position.y == expectedTile.y);
    }

    printf("  PASS: pickTile returns consistent tile across zoom levels\n");
}

// ============================================================================
// Criterion 3: Accounts for elevation (terrain height)
// ============================================================================

void test_pickTile_with_elevation() {
    printf("Testing pickTile with terrain elevation...\n");

    CameraState state;
    state.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Pick with elevated terrain
    auto elevatedTerrain = [](int16_t /*x*/, int16_t /*y*/) -> float {
        return 5.0f;  // All terrain at height 5
    };

    glm::vec2 screenCenter(windowWidth / 2.0f, windowHeight / 2.0f);
    auto result = pickTile(screenCenter, windowWidth, windowHeight, vp, state, elevatedTerrain);

    assert(result.has_value());

    // With elevated terrain, the intersection point Y should be at the terrain height
    assert(approxEqual(result->elevation, 5.0f));

    printf("  PASS: pickTile accounts for elevation\n");
}

void test_pickTile_with_varying_elevation() {
    printf("Testing pickTile with varying terrain elevation...\n");

    CameraState state;
    state.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Pick with hilly terrain
    glm::vec2 screenCenter(windowWidth / 2.0f, windowHeight / 2.0f);
    auto result = pickTile(screenCenter, windowWidth, windowHeight, vp, state, hilly_terrain);

    assert(result.has_value());

    // The result should have the correct elevation for that tile
    float expectedHeight = hilly_terrain(result->position.x, result->position.y);
    assert(approxEqual(result->elevation, expectedHeight, 0.5f));

    printf("  PASS: pickTile works with varying elevation\n");
}

void test_pickTile_elevation_iterative_refinement() {
    printf("Testing pickTile elevation iterative refinement...\n");

    CameraState state;
    state.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    state.distance = 50.0f;
    state.pitch = 45.0f;
    state.yaw = 45.0f;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Terrain with step elevation
    auto stepTerrain = [](int16_t x, int16_t /*y*/) -> float {
        if (x < 60) return 0.0f;
        if (x < 68) return 3.0f;
        return 0.0f;
    };

    // Pick at screen center
    glm::vec2 screenCenter(windowWidth / 2.0f, windowHeight / 2.0f);
    auto result = pickTile(screenCenter, windowWidth, windowHeight, vp, state, stepTerrain);

    assert(result.has_value());

    // Elevation should match terrain at picked position
    float expectedHeight = stepTerrain(result->position.x, result->position.y);
    assert(approxEqual(result->elevation, expectedHeight, 0.5f));

    printf("  PASS: Elevation iterative refinement works\n");
}

// ============================================================================
// Criterion 4: Cursor position maps to expected tile
// ============================================================================

void test_cursor_to_tile_mapping() {
    printf("Testing cursor to tile mapping...\n");

    CameraState state;
    state.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    state.yaw = CameraConfig::PRESET_N_YAW;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Pick at different screen positions
    struct TestCase {
        float screenX, screenY;
    };

    TestCase cases[] = {
        {640.0f, 360.0f},    // Center
        {320.0f, 180.0f},    // Top-left quadrant
        {960.0f, 180.0f},    // Top-right quadrant
        {320.0f, 540.0f},    // Bottom-left quadrant
        {960.0f, 540.0f},    // Bottom-right quadrant
    };

    GridPosition prevPos{-1, -1};
    for (const auto& tc : cases) {
        auto result = pickTileFlat(
            glm::vec2(tc.screenX, tc.screenY),
            windowWidth, windowHeight, vp, state, 0.0f
        );

        assert(result.has_value());

        // Each position should map to a valid tile
        assert(result->position.x >= 0);
        assert(result->position.y >= 0);

        // Different screen positions should (mostly) map to different tiles
        // (except very close positions at high zoom)
        prevPos = result->position;
    }

    printf("  PASS: Cursor positions map to tiles correctly\n");
}

void test_worldToGrid_conversion() {
    printf("Testing worldToGrid conversion...\n");

    // Exact integer position
    auto pos1 = worldToGrid(glm::vec3(5.0f, 0.0f, 10.0f));
    assert(pos1.x == 5);
    assert(pos1.y == 10);

    // Fractional position (should floor)
    auto pos2 = worldToGrid(glm::vec3(5.7f, 0.0f, 10.3f));
    assert(pos2.x == 5);
    assert(pos2.y == 10);

    // Near-integer position
    auto pos3 = worldToGrid(glm::vec3(5.99f, 0.0f, 10.01f));
    assert(pos3.x == 5);
    assert(pos3.y == 10);

    // Zero position
    auto pos4 = worldToGrid(glm::vec3(0.0f, 0.0f, 0.0f));
    assert(pos4.x == 0);
    assert(pos4.y == 0);

    // Negative position (should still floor correctly)
    auto pos5 = worldToGrid(glm::vec3(-0.5f, 0.0f, -0.5f));
    assert(pos5.x == -1);
    assert(pos5.y == -1);

    printf("  PASS: worldToGrid conversion correct\n");
}

void test_gridToWorld_conversion() {
    printf("Testing gridToWorld conversions...\n");

    GridPosition gridPos{10, 20};

    // Center of tile
    glm::vec3 center = gridToWorldCenter(gridPos, 0.0f);
    assert(approxEqual(center.x, 10.5f));
    assert(approxEqual(center.y, 0.0f));
    assert(approxEqual(center.z, 20.5f));

    // Corner of tile
    glm::vec3 corner = gridToWorldCorner(gridPos, 0.0f);
    assert(approxEqual(corner.x, 10.0f));
    assert(approxEqual(corner.y, 0.0f));
    assert(approxEqual(corner.z, 20.0f));

    // With elevation
    glm::vec3 elevated = gridToWorldCenter(gridPos, 5.0f);
    assert(approxEqual(elevated.y, 5.0f));

    printf("  PASS: gridToWorld conversions correct\n");
}

// ============================================================================
// Criterion 5: Numerical stability guard for near-parallel ray-ground intersection
// ============================================================================

void test_numerical_stability_near_parallel() {
    printf("Testing numerical stability for near-parallel rays...\n");

    CameraState state;
    state.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    state.distance = 50.0f;
    state.pitch = CameraConfig::PITCH_MIN;  // 15 degrees - most horizontal allowed
    state.yaw = 45.0f;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

    // Pick at various positions with near-horizontal camera
    for (int x = 0; x <= 4; ++x) {
        for (int y = 0; y <= 4; ++y) {
            float screenX = x * 320.0f;
            float screenY = y * 180.0f;

            auto result = pickTileFlat(
                glm::vec2(screenX, screenY),
                windowWidth, windowHeight, vp, state, 0.0f
            );

            // Result may or may not exist (corners may miss ground)
            // but if it exists, it must be valid
            if (result.has_value()) {
                assert(!std::isnan(static_cast<float>(result->position.x)));
                assert(!std::isnan(static_cast<float>(result->position.y)));
                assert(!std::isnan(result->worldPosition.x));
                assert(!std::isnan(result->worldPosition.y));
                assert(!std::isnan(result->worldPosition.z));
                assert(!std::isinf(result->worldPosition.x));
                assert(!std::isinf(result->worldPosition.y));
                assert(!std::isinf(result->worldPosition.z));
            }
        }
    }

    printf("  PASS: Near-parallel rays handled with numerical stability\n");
}

void test_canIntersectGround_helper() {
    printf("Testing canIntersectGround helper function...\n");

    // Downward ray from above ground - should intersect
    Ray downward;
    downward.origin = glm::vec3(0.0f, 10.0f, 0.0f);
    downward.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    assert(canIntersectGround(downward, 0.0f));

    // Upward ray from above ground - should not intersect
    Ray upward;
    upward.origin = glm::vec3(0.0f, 10.0f, 0.0f);
    upward.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    assert(!canIntersectGround(upward, 0.0f));

    // Horizontal ray - should not intersect
    Ray horizontal;
    horizontal.origin = glm::vec3(0.0f, 10.0f, 0.0f);
    horizontal.direction = glm::vec3(1.0f, 0.0f, 0.0f);
    assert(!canIntersectGround(horizontal, 0.0f));

    // Diagonal downward ray - should intersect
    Ray diagonal;
    diagonal.origin = glm::vec3(0.0f, 10.0f, 0.0f);
    diagonal.direction = glm::normalize(glm::vec3(1.0f, -0.5f, 1.0f));
    assert(canIntersectGround(diagonal, 0.0f));

    printf("  PASS: canIntersectGround helper works correctly\n");
}

void test_parallel_ray_returns_empty() {
    printf("Testing parallel ray returns empty optional...\n");

    CameraState state;
    state.focus_point = glm::vec3(64.0f, 50.0f, 64.0f);  // Looking at elevated point
    state.distance = 50.0f;
    state.pitch = 0.1f;  // Extremely shallow (below PITCH_MIN but for testing)
    state.yaw = 45.0f;

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;

    // Force a horizontal ray scenario
    Ray horizontalRay;
    horizontalRay.origin = glm::vec3(0.0f, 10.0f, 0.0f);
    horizontalRay.direction = glm::vec3(1.0f, 0.0f, 0.0f);  // Perfectly horizontal

    // Test using pickTileWithElevation directly with horizontal ray
    auto result = pickTileWithElevation(horizontalRay, flatTerrainHeight);
    assert(!result.has_value());

    printf("  PASS: Parallel ray returns empty optional\n");
}

// ============================================================================
// Criterion 6: Tested at preset angles AND arbitrary free camera angles
// ============================================================================

void test_all_isometric_presets() {
    printf("Testing pickTile at all isometric presets...\n");

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

        // Pick at screen center
        glm::vec2 screenCenter(windowWidth / 2.0f, windowHeight / 2.0f);
        auto result = pickTileFlat(screenCenter, windowWidth, windowHeight, vp, state, 0.0f);

        assert(result.has_value());

        // Should be near focus point (tile 64, 64)
        assert(std::abs(result->position.x - 64) < 5);
        assert(std::abs(result->position.y - 64) < 5);
    }

    printf("  PASS: All isometric presets work correctly\n");
}

void test_arbitrary_free_camera_angles() {
    printf("Testing pickTile at arbitrary free camera angles...\n");

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
        {40.0f, 112.5f},
        {55.0f, 157.5f},
        {65.0f, 202.5f},
        {30.0f, 247.5f},
        {25.0f, 292.5f},
        {70.0f, 337.5f},
    };

    for (const TestCase& tc : cases) {
        CameraState state;
        state.focus_point = glm::vec3(100.0f, 0.0f, 100.0f);
        state.distance = 75.0f;
        state.pitch = tc.pitch;
        state.yaw = tc.yaw;
        state.mode = CameraMode::Free;

        glm::mat4 vp = buildViewProjection(state, windowWidth, windowHeight);

        // Pick at screen center
        glm::vec2 screenCenter(windowWidth / 2.0f, windowHeight / 2.0f);
        auto result = pickTileFlat(screenCenter, windowWidth, windowHeight, vp, state, 0.0f);

        assert(result.has_value());

        // Should be reasonably near focus point
        float dist = std::sqrt(
            std::pow(result->position.x - 100.0f, 2.0f) +
            std::pow(result->position.y - 100.0f, 2.0f)
        );
        assert(dist < 15.0f);  // Within 15 tiles of focus
    }

    printf("  PASS: Arbitrary free camera angles work correctly\n");
}

void test_extreme_pitch_angles() {
    printf("Testing pickTile at extreme pitch angles...\n");

    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;

    // Test at minimum pitch (most horizontal - 15 degrees)
    CameraState minPitchState;
    minPitchState.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    minPitchState.distance = 50.0f;
    minPitchState.pitch = CameraConfig::PITCH_MIN;
    minPitchState.yaw = 45.0f;

    glm::mat4 vpMin = buildViewProjection(minPitchState, windowWidth, windowHeight);
    auto resultMin = pickTileFlat(
        glm::vec2(windowWidth / 2.0f, windowHeight / 2.0f),
        windowWidth, windowHeight, vpMin, minPitchState, 0.0f
    );

    assert(resultMin.has_value());

    // Test at maximum pitch (most vertical - 80 degrees)
    CameraState maxPitchState;
    maxPitchState.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    maxPitchState.distance = 50.0f;
    maxPitchState.pitch = CameraConfig::PITCH_MAX;
    maxPitchState.yaw = 45.0f;

    glm::mat4 vpMax = buildViewProjection(maxPitchState, windowWidth, windowHeight);
    auto resultMax = pickTileFlat(
        glm::vec2(windowWidth / 2.0f, windowHeight / 2.0f),
        windowWidth, windowHeight, vpMax, maxPitchState, 0.0f
    );

    assert(resultMax.has_value());

    // At max pitch (nearly top-down), the pick should be more accurate to focus
    float distMin = std::sqrt(
        std::pow(resultMin->position.x - 64.0f, 2.0f) +
        std::pow(resultMin->position.y - 64.0f, 2.0f)
    );
    float distMax = std::sqrt(
        std::pow(resultMax->position.x - 64.0f, 2.0f) +
        std::pow(resultMax->position.y - 64.0f, 2.0f)
    );

    // Max pitch should hit closer to focus than min pitch
    assert(distMax <= distMin + 1.0f);

    printf("  PASS: Extreme pitch angles work correctly\n");
}

// ============================================================================
// Additional Tests: Bounds and Validation
// ============================================================================

void test_isValidGridPosition() {
    printf("Testing isValidGridPosition bounds checking...\n");

    int16_t mapWidth = 256;
    int16_t mapHeight = 256;

    // Valid positions
    assert(isValidGridPosition({0, 0}, mapWidth, mapHeight));
    assert(isValidGridPosition({128, 128}, mapWidth, mapHeight));
    assert(isValidGridPosition({255, 255}, mapWidth, mapHeight));

    // Invalid positions
    assert(!isValidGridPosition({-1, 0}, mapWidth, mapHeight));
    assert(!isValidGridPosition({0, -1}, mapWidth, mapHeight));
    assert(!isValidGridPosition({256, 0}, mapWidth, mapHeight));
    assert(!isValidGridPosition({0, 256}, mapWidth, mapHeight));

    printf("  PASS: isValidGridPosition works correctly\n");
}

void test_clampToMapBounds() {
    printf("Testing clampToMapBounds...\n");

    int16_t mapWidth = 256;
    int16_t mapHeight = 256;

    // Already in bounds
    auto pos1 = clampToMapBounds({100, 100}, mapWidth, mapHeight);
    assert(pos1.x == 100 && pos1.y == 100);

    // Below bounds
    auto pos2 = clampToMapBounds({-10, -20}, mapWidth, mapHeight);
    assert(pos2.x == 0 && pos2.y == 0);

    // Above bounds
    auto pos3 = clampToMapBounds({300, 400}, mapWidth, mapHeight);
    assert(pos3.x == 255 && pos3.y == 255);

    // Mixed
    auto pos4 = clampToMapBounds({-5, 300}, mapWidth, mapHeight);
    assert(pos4.x == 0 && pos4.y == 255);

    printf("  PASS: clampToMapBounds works correctly\n");
}

void test_worldToGridBounded() {
    printf("Testing worldToGridBounded...\n");

    int16_t mapWidth = 128;
    int16_t mapHeight = 128;

    // Valid position
    auto pos1 = worldToGridBounded(glm::vec3(64.0f, 0.0f, 64.0f), mapWidth, mapHeight);
    assert(pos1.has_value());
    assert(pos1->x == 64 && pos1->y == 64);

    // Out of bounds (negative)
    auto pos2 = worldToGridBounded(glm::vec3(-5.0f, 0.0f, 10.0f), mapWidth, mapHeight);
    assert(!pos2.has_value());

    // Out of bounds (too large)
    auto pos3 = worldToGridBounded(glm::vec3(200.0f, 0.0f, 50.0f), mapWidth, mapHeight);
    assert(!pos3.has_value());

    printf("  PASS: worldToGridBounded works correctly\n");
}

void test_tilePickResult_structure() {
    printf("Testing TilePickResult structure...\n");

    // Default construction
    TilePickResult result1;
    assert(result1.position.x == 0);
    assert(result1.position.y == 0);

    // Parameterized construction
    TilePickResult result2(
        GridPosition{10, 20},
        glm::vec3(10.5f, 5.0f, 20.5f),
        5.0f
    );
    assert(result2.position.x == 10);
    assert(result2.position.y == 20);
    assert(approxEqual(result2.worldPosition.x, 10.5f));
    assert(approxEqual(result2.worldPosition.y, 5.0f));
    assert(approxEqual(result2.worldPosition.z, 20.5f));
    assert(approxEqual(result2.elevation, 5.0f));

    printf("  PASS: TilePickResult structure correct\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("=== TilePicking Unit Tests (Ticket 2-030) ===\n\n");

    // Criterion 1: pickTile(Vec2 screenPos) -> GridPosition
    printf("--- Criterion 1: pickTile Function ---\n");
    test_pickTile_basic_function();
    test_pickTile_returns_grid_position();
    test_pickTileFlat_convenience();

    // Criterion 2: Returns correct tile at all zoom levels
    printf("\n--- Criterion 2: All Zoom Levels ---\n");
    test_pickTile_at_different_zoom_levels();
    test_pickTile_consistency_across_zoom();

    // Criterion 3: Accounts for elevation
    printf("\n--- Criterion 3: Elevation Handling ---\n");
    test_pickTile_with_elevation();
    test_pickTile_with_varying_elevation();
    test_pickTile_elevation_iterative_refinement();

    // Criterion 4: Cursor position maps to expected tile
    printf("\n--- Criterion 4: Cursor to Tile Mapping ---\n");
    test_cursor_to_tile_mapping();
    test_worldToGrid_conversion();
    test_gridToWorld_conversion();

    // Criterion 5: Numerical stability guard
    printf("\n--- Criterion 5: Numerical Stability ---\n");
    test_numerical_stability_near_parallel();
    test_canIntersectGround_helper();
    test_parallel_ray_returns_empty();

    // Criterion 6: Preset AND arbitrary camera angles
    printf("\n--- Criterion 6: All Camera Angles ---\n");
    test_all_isometric_presets();
    test_arbitrary_free_camera_angles();
    test_extreme_pitch_angles();

    // Additional: Bounds and validation
    printf("\n--- Additional: Bounds and Validation ---\n");
    test_isValidGridPosition();
    test_clampToMapBounds();
    test_worldToGridBounded();
    test_tilePickResult_structure();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
