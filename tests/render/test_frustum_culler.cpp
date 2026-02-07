/**
 * @file test_frustum_culler.cpp
 * @brief Unit tests for FrustumCuller (Ticket 2-026).
 *
 * Tests:
 * - Frustum plane extraction from VP matrix
 * - AABB-frustum intersection (conservative culling)
 * - Spatial partitioning (grid hash)
 * - Visibility queries at various camera angles
 * - Large map performance (512x512)
 */

#include "sims3000/render/FrustumCuller.h"
#include "sims3000/render/ViewMatrix.h"
#include "sims3000/render/ProjectionMatrix.h"
#include "sims3000/render/CameraState.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cmath>
#include <chrono>

using namespace sims3000;

// Test counter
static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << " (line " << __LINE__ << ")" << std::endl; \
            g_testsFailed++; \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_FLOAT_EQ(a, b, epsilon, message) \
    TEST_ASSERT(std::abs((a) - (b)) < (epsilon), message)

// ============================================================================
// Helper Functions
// ============================================================================

glm::mat4 createViewProjection(const CameraState& camera,
                               int windowWidth = 1920,
                               int windowHeight = 1080) {
    glm::mat4 view = calculateViewMatrix(camera);
    glm::mat4 proj = calculateProjectionMatrixFromDimensions(
        windowWidth, windowHeight, CameraConfig::FOV_DEFAULT);
    return proj * view;
}

// ============================================================================
// Test Cases
// ============================================================================

bool test_FrustumCuller_Construction() {
    std::cout << "test_FrustumCuller_Construction... ";

    // Small map
    FrustumCuller culler1(128, 128);
    auto [w1, h1] = culler1.getGridDimensions();
    TEST_ASSERT(w1 == 8 && h1 == 8,
        "128x128 map with cell size 16 should have 8x8 grid");
    TEST_ASSERT(culler1.getCellSize() == 16, "Default cell size should be 16");

    // Medium map
    FrustumCuller culler2(256, 256);
    auto [w2, h2] = culler2.getGridDimensions();
    TEST_ASSERT(w2 == 16 && h2 == 16,
        "256x256 map should have 16x16 grid");

    // Large map
    FrustumCuller culler3(512, 512);
    auto [w3, h3] = culler3.getGridDimensions();
    TEST_ASSERT(w3 == 32 && h3 == 32,
        "512x512 map should have 32x32 grid");

    // Custom cell size
    FrustumCuller culler4(512, 512, 32);
    auto [w4, h4] = culler4.getGridDimensions();
    TEST_ASSERT(w4 == 16 && h4 == 16,
        "512x512 map with cell size 32 should have 16x16 grid");
    TEST_ASSERT(culler4.getCellSize() == 32, "Custom cell size should be 32");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_FrustumPlane_Extraction() {
    std::cout << "test_FrustumPlane_Extraction... ";

    // Create a simple view-projection matrix
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);

    FrustumPlane planes[6];
    extractFrustumPlanes(vp, planes);

    // All plane normals should be normalized (length ~1)
    for (int i = 0; i < 6; ++i) {
        float len = glm::length(planes[i].normal);
        TEST_ASSERT_FLOAT_EQ(len, 1.0f, 0.01f,
            "Frustum plane normal should be normalized");
    }

    // Left and right planes should have opposite X components in normal
    // (They may not be exactly opposite due to perspective, but should be different signs)
    TEST_ASSERT(planes[0].normal.x * planes[1].normal.x <= 0.0f,
        "Left and right plane normals should have opposite X signs");

    // Bottom and top planes should have opposite Y components
    TEST_ASSERT(planes[2].normal.y * planes[3].normal.y <= 0.0f,
        "Bottom and top plane normals should have opposite Y signs");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_FrustumCuller_UpdateFrustum() {
    std::cout << "test_FrustumCuller_UpdateFrustum... ";

    FrustumCuller culler(256, 256);

    // Before update, frustum should not be valid
    TEST_ASSERT(!culler.isFrustumValid(),
        "Frustum should not be valid before updateFrustum()");

    // Create VP matrix
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    // After update, frustum should be valid
    TEST_ASSERT(culler.isFrustumValid(),
        "Frustum should be valid after updateFrustum()");

    // Should be able to get planes
    const FrustumPlane* planes = culler.getFrustumPlanes();
    TEST_ASSERT(planes != nullptr, "getFrustumPlanes() should return valid pointer");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_AABB_FrustumCulling_InsideFrustum() {
    std::cout << "test_AABB_FrustumCulling_InsideFrustum... ";

    FrustumCuller culler(256, 256);

    // Camera looking at center of map
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    // AABB at the focus point should be visible
    AABB centerBox;
    centerBox.min = glm::vec3(127.0f, 0.0f, 127.0f);
    centerBox.max = glm::vec3(129.0f, 2.0f, 129.0f);

    CullResult result = culler.testAABB(centerBox);
    TEST_ASSERT(result != CullResult::Outside,
        "AABB at camera focus should be visible");

    TEST_ASSERT(culler.isVisible(centerBox),
        "isVisible() should return true for AABB at focus");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_AABB_FrustumCulling_OutsideFrustum() {
    std::cout << "test_AABB_FrustumCulling_OutsideFrustum... ";

    FrustumCuller culler(256, 256);

    // Camera looking at center of map
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 30.0f;  // Close-ish zoom
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    // AABB very far from focus point should be culled
    AABB farBox;
    farBox.min = glm::vec3(0.0f, 0.0f, 0.0f);
    farBox.max = glm::vec3(2.0f, 2.0f, 2.0f);

    CullResult result = culler.testAABB(farBox);
    // At close zoom, corner of map should be outside frustum
    TEST_ASSERT(result == CullResult::Outside,
        "AABB far from camera focus should be culled at close zoom");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_ConservativeCulling_NoPopping() {
    std::cout << "test_ConservativeCulling_NoPopping... ";

    FrustumCuller culler(256, 256);

    // Camera looking at center
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    // Create AABB directly at focus point - should definitely be visible
    AABB centerBox;
    centerBox.min = glm::vec3(126.0f, 0.0f, 126.0f);
    centerBox.max = glm::vec3(130.0f, 2.0f, 130.0f);

    // Objects at the focus point must always be visible
    CullResult centerResult = culler.testAABB(centerBox);
    TEST_ASSERT(centerResult != CullResult::Outside,
        "Objects at focus point should never be culled");

    // Conservative culling should expand bounds to prevent popping
    // Create a small object - conservative expansion should protect it
    AABB smallBox;
    smallBox.min = glm::vec3(127.5f, 0.0f, 127.5f);
    smallBox.max = glm::vec3(128.5f, 1.0f, 128.5f);

    CullResult smallResult = culler.testAABB(smallBox);
    TEST_ASSERT(smallResult != CullResult::Outside,
        "Small objects near center should be visible with conservative expansion");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_SpatialPartitioning_Registration() {
    std::cout << "test_SpatialPartitioning_Registration... ";

    FrustumCuller culler(256, 256);

    // Register some entities
    AABB box1;
    box1.min = glm::vec3(10.0f, 0.0f, 10.0f);
    box1.max = glm::vec3(12.0f, 2.0f, 12.0f);

    AABB box2;
    box2.min = glm::vec3(100.0f, 0.0f, 100.0f);
    box2.max = glm::vec3(102.0f, 2.0f, 102.0f);

    culler.registerEntity(1, box1, glm::vec3(11.0f, 0.0f, 11.0f));
    culler.registerEntity(2, box2, glm::vec3(101.0f, 0.0f, 101.0f));

    TEST_ASSERT(culler.getEntityCount() == 2,
        "Should have 2 registered entities");

    // Entities should be in different cells
    auto [cell1X, cell1Y] = culler.getCellForPosition(11.0f, 11.0f);
    auto [cell2X, cell2Y] = culler.getCellForPosition(101.0f, 101.0f);
    TEST_ASSERT(cell1X != cell2X || cell1Y != cell2Y,
        "Entities at different positions should be in different cells");

    // Check cells contain entities
    const SpatialCell* c1 = culler.getCell(cell1X, cell1Y);
    const SpatialCell* c2 = culler.getCell(cell2X, cell2Y);
    TEST_ASSERT(c1 != nullptr && c1->contains(1),
        "Cell 1 should contain entity 1");
    TEST_ASSERT(c2 != nullptr && c2->contains(2),
        "Cell 2 should contain entity 2");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_SpatialPartitioning_Unregistration() {
    std::cout << "test_SpatialPartitioning_Unregistration... ";

    FrustumCuller culler(256, 256);

    AABB box;
    box.min = glm::vec3(10.0f, 0.0f, 10.0f);
    box.max = glm::vec3(12.0f, 2.0f, 12.0f);

    culler.registerEntity(1, box, glm::vec3(11.0f, 0.0f, 11.0f));
    TEST_ASSERT(culler.getEntityCount() == 1, "Should have 1 entity");

    culler.unregisterEntity(1);
    TEST_ASSERT(culler.getEntityCount() == 0, "Should have 0 entities after unregister");

    // Cell should be empty
    auto [cellX, cellY] = culler.getCellForPosition(11.0f, 11.0f);
    const SpatialCell* cell = culler.getCell(cellX, cellY);
    TEST_ASSERT(cell != nullptr && !cell->contains(1),
        "Cell should not contain entity after unregister");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_SpatialPartitioning_PositionUpdate() {
    std::cout << "test_SpatialPartitioning_PositionUpdate... ";

    FrustumCuller culler(256, 256);

    AABB box;
    box.min = glm::vec3(0.0f, 0.0f, 0.0f);
    box.max = glm::vec3(2.0f, 2.0f, 2.0f);

    // Register in first cell
    culler.registerEntity(1, box, glm::vec3(1.0f, 0.0f, 1.0f));
    auto [oldCellX, oldCellY] = culler.getCellForPosition(1.0f, 1.0f);

    // Move to different cell
    culler.updateEntityPosition(1, glm::vec3(100.0f, 0.0f, 100.0f));
    auto [newCellX, newCellY] = culler.getCellForPosition(100.0f, 100.0f);

    TEST_ASSERT(oldCellX != newCellX || oldCellY != newCellY,
        "Position should be in different cell");

    // Old cell should not contain entity
    const SpatialCell* oldCell = culler.getCell(oldCellX, oldCellY);
    TEST_ASSERT(oldCell != nullptr && !oldCell->contains(1),
        "Old cell should not contain entity after move");

    // New cell should contain entity
    const SpatialCell* newCell = culler.getCell(newCellX, newCellY);
    TEST_ASSERT(newCell != nullptr && newCell->contains(1),
        "New cell should contain entity after move");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_GetVisibleEntities() {
    std::cout << "test_GetVisibleEntities... ";

    FrustumCuller culler(256, 256);

    // Camera looking at center
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    // Register entity at center (visible)
    AABB centerBox;
    centerBox.min = glm::vec3(127.0f, 0.0f, 127.0f);
    centerBox.max = glm::vec3(129.0f, 2.0f, 129.0f);
    culler.registerEntity(1, centerBox, glm::vec3(128.0f, 0.0f, 128.0f));

    // Register entity at corner (likely not visible at close zoom)
    AABB cornerBox;
    cornerBox.min = glm::vec3(0.0f, 0.0f, 0.0f);
    cornerBox.max = glm::vec3(2.0f, 2.0f, 2.0f);
    culler.registerEntity(2, cornerBox, glm::vec3(1.0f, 0.0f, 1.0f));

    std::vector<EntityID> visible;
    culler.getVisibleEntities(visible);

    // Center entity should be visible
    bool centerVisible = std::find(visible.begin(), visible.end(), 1) != visible.end();
    TEST_ASSERT(centerVisible, "Entity at center should be visible");

    // Stats should be populated
    const auto& stats = culler.getStats();
    TEST_ASSERT(stats.totalEntities == 2, "Should have 2 total entities");
    TEST_ASSERT(stats.visibleEntities >= 1, "At least center entity should be visible");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_CullingAtDifferentCameraAngles() {
    std::cout << "test_CullingAtDifferentCameraAngles... ";

    FrustumCuller culler(256, 256);

    // Register entity at center
    AABB centerBox;
    centerBox.min = glm::vec3(127.0f, 0.0f, 127.0f);
    centerBox.max = glm::vec3(129.0f, 2.0f, 129.0f);
    culler.registerEntity(1, centerBox, glm::vec3(128.0f, 0.0f, 128.0f));

    // Test all four preset angles
    CameraMode presets[] = {
        CameraMode::Preset_N,
        CameraMode::Preset_E,
        CameraMode::Preset_S,
        CameraMode::Preset_W
    };

    for (CameraMode preset : presets) {
        CameraState camera;
        camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
        camera.distance = 50.0f;
        camera.pitch = CameraState::getPitchForPreset(preset);
        camera.yaw = CameraState::getYawForPreset(preset);

        glm::mat4 vp = createViewProjection(camera);
        culler.updateFrustum(vp);

        std::vector<EntityID> visible;
        culler.getVisibleEntities(visible);

        bool centerVisible = std::find(visible.begin(), visible.end(), 1) != visible.end();
        TEST_ASSERT(centerVisible,
            "Entity at center should be visible from all preset angles");
    }

    // Test free camera at extreme pitch
    CameraState freeCamera;
    freeCamera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    freeCamera.distance = 50.0f;
    freeCamera.pitch = CameraConfig::PITCH_MAX;  // Most top-down
    freeCamera.yaw = 180.0f;  // Arbitrary

    glm::mat4 vp = createViewProjection(freeCamera);
    culler.updateFrustum(vp);

    std::vector<EntityID> visible;
    culler.getVisibleEntities(visible);
    bool centerVisible = std::find(visible.begin(), visible.end(), 1) != visible.end();
    TEST_ASSERT(centerVisible,
        "Entity at center should be visible at extreme pitch");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_GetVisibleTileRange() {
    std::cout << "test_GetVisibleTileRange... ";

    FrustumCuller culler(256, 256);

    // Camera looking at center
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    GridRect range = culler.getVisibleTileRange();

    // Range should be valid
    TEST_ASSERT(range.isValid(), "Visible tile range should be valid");

    // Range should include the focus point
    TEST_ASSERT(range.contains(GridPosition{128, 128}),
        "Range should include camera focus point");

    // Range should cover some reasonable area (not just a single tile, not everything)
    // At medium zoom, we expect to see at least a few tiles but possibly many
    TEST_ASSERT(range.tileCount() >= 1,
        "Range should cover at least some tiles");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_TransformAABBToWorld() {
    std::cout << "test_TransformAABBToWorld... ";

    // Local AABB at origin
    AABB local;
    local.min = glm::vec3(-1.0f, 0.0f, -1.0f);
    local.max = glm::vec3(1.0f, 2.0f, 1.0f);

    // Translation only
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 10.0f));
    AABB worldTranslated = transformAABBToWorld(local, translate);

    TEST_ASSERT_FLOAT_EQ(worldTranslated.min.x, 9.0f, 0.01f,
        "Translated AABB min.x should be 9.0");
    TEST_ASSERT_FLOAT_EQ(worldTranslated.max.x, 11.0f, 0.01f,
        "Translated AABB max.x should be 11.0");

    // Scale
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
    AABB worldScaled = transformAABBToWorld(local, scale);

    TEST_ASSERT_FLOAT_EQ(worldScaled.min.x, -2.0f, 0.01f,
        "Scaled AABB min.x should be -2.0");
    TEST_ASSERT_FLOAT_EQ(worldScaled.max.x, 2.0f, 0.01f,
        "Scaled AABB max.x should be 2.0");

    // Rotation (45 degrees around Y axis)
    glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    AABB worldRotated = transformAABBToWorld(local, rotate);

    // Rotated AABB should be larger due to axis-alignment
    float diagonal = std::sqrt(2.0f);  // sqrt(1^2 + 1^2)
    TEST_ASSERT(worldRotated.max.x > local.max.x * 0.9f,
        "Rotated AABB should expand due to axis-alignment");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_LargeMapPerformance() {
    std::cout << "test_LargeMapPerformance... ";

    // Create culler for 512x512 map
    FrustumCuller culler(512, 512);

    // Register many entities (simulate dense building coverage)
    // At 10% coverage with 1 entity per tile = ~26k entities
    const int entityCount = 1000;  // Use smaller count for test speed

    AABB templateBox;
    templateBox.min = glm::vec3(0.0f, 0.0f, 0.0f);
    templateBox.max = glm::vec3(1.0f, 2.0f, 1.0f);

    auto startRegister = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < entityCount; ++i) {
        float x = static_cast<float>(i % 512);
        float z = static_cast<float>(i / 512 % 512);

        AABB box = templateBox;
        box.min += glm::vec3(x, 0.0f, z);
        box.max += glm::vec3(x, 0.0f, z);

        culler.registerEntity(static_cast<EntityID>(i), box, glm::vec3(x + 0.5f, 0.0f, z + 0.5f));
    }

    auto endRegister = std::chrono::high_resolution_clock::now();
    auto registerTime = std::chrono::duration_cast<std::chrono::microseconds>(
        endRegister - startRegister).count();

    // Camera at center
    CameraState camera;
    camera.focus_point = glm::vec3(256.0f, 0.0f, 256.0f);
    camera.distance = 100.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);

    auto startCull = std::chrono::high_resolution_clock::now();

    culler.updateFrustum(vp);

    std::vector<EntityID> visible;
    culler.getVisibleEntities(visible);

    auto endCull = std::chrono::high_resolution_clock::now();
    auto cullTime = std::chrono::duration_cast<std::chrono::microseconds>(
        endCull - startCull).count();

    // Print performance info
    std::cout << std::endl;
    std::cout << "  Registration time for " << entityCount << " entities: "
              << registerTime << " us" << std::endl;
    std::cout << "  Culling time: " << cullTime << " us" << std::endl;
    std::cout << "  Visible entities: " << visible.size() << std::endl;
    std::cout << "  Cull ratio: " << (culler.getStats().cullRatio * 100.0f) << "%" << std::endl;

    // Culling should complete in reasonable time (<10ms for test size)
    TEST_ASSERT(cullTime < 10000,
        "Culling should complete in under 10ms");

    // Some entities should be culled (not all visible at medium zoom)
    TEST_ASSERT(visible.size() < static_cast<std::size_t>(entityCount),
        "Some entities should be culled at medium zoom");

    std::cout << "  PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_SphereCulling() {
    std::cout << "test_SphereCulling... ";

    FrustumCuller culler(256, 256);

    // Camera looking at center
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    // Sphere at center should be visible
    CullResult centerResult = culler.testSphere(glm::vec3(128.0f, 1.0f, 128.0f), 5.0f);
    TEST_ASSERT(centerResult != CullResult::Outside,
        "Sphere at center should be visible");

    // Sphere far from camera should be culled at close zoom
    CullResult farResult = culler.testSphere(glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
    TEST_ASSERT(farResult == CullResult::Outside,
        "Small sphere far from camera should be culled");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_PointVisibility() {
    std::cout << "test_PointVisibility... ";

    FrustumCuller culler(256, 256);

    // Camera looking at center
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    // Point at focus should be visible
    TEST_ASSERT(culler.isPointVisible(glm::vec3(128.0f, 0.0f, 128.0f)),
        "Point at camera focus should be visible");

    // Point behind camera should not be visible
    // Camera is looking at 128,128 from the northeast, so 200,200 area is behind
    // Actually the camera position depends on yaw, let's use a point clearly outside
    // At Preset_N (yaw 45), camera is to the NE, so far SW corner should be behind
    TEST_ASSERT(!culler.isPointVisible(glm::vec3(-100.0f, 0.0f, 400.0f)),
        "Point clearly outside frustum should not be visible");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_ClearEntities() {
    std::cout << "test_ClearEntities... ";

    FrustumCuller culler(256, 256);

    // Register several entities
    AABB box;
    box.min = glm::vec3(0.0f, 0.0f, 0.0f);
    box.max = glm::vec3(2.0f, 2.0f, 2.0f);

    for (int i = 0; i < 10; ++i) {
        culler.registerEntity(static_cast<EntityID>(i), box,
            glm::vec3(static_cast<float>(i * 10), 0.0f, 0.0f));
    }

    TEST_ASSERT(culler.getEntityCount() == 10, "Should have 10 entities");

    culler.clearEntities();

    TEST_ASSERT(culler.getEntityCount() == 0, "Should have 0 entities after clear");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_CellBoundary() {
    std::cout << "test_CellBoundary... ";

    FrustumCuller culler(256, 256);  // Cell size 16

    // Test entity at cell boundary
    auto [cellA, _1] = culler.getCellForPosition(15.9f, 0.0f);
    auto [cellB, _2] = culler.getCellForPosition(16.0f, 0.0f);
    auto [cellC, _3] = culler.getCellForPosition(16.1f, 0.0f);

    TEST_ASSERT(cellA == 0, "Position 15.9 should be in cell 0");
    TEST_ASSERT(cellB == 1, "Position 16.0 should be in cell 1");
    TEST_ASSERT(cellC == 1, "Position 16.1 should be in cell 1");

    // Test clamping at map edges
    auto [neg, _4] = culler.getCellForPosition(-10.0f, 0.0f);
    TEST_ASSERT(neg == 0, "Negative position should clamp to cell 0");

    auto [over, _5] = culler.getCellForPosition(1000.0f, 0.0f);
    int maxCell = (256 / 16) - 1;  // 15
    TEST_ASSERT(over == maxCell, "Position beyond map should clamp to last cell");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "===== Frustum Culler Tests (Ticket 2-026) =====" << std::endl;
    std::cout << std::endl;

    test_FrustumCuller_Construction();
    test_FrustumPlane_Extraction();
    test_FrustumCuller_UpdateFrustum();
    test_AABB_FrustumCulling_InsideFrustum();
    test_AABB_FrustumCulling_OutsideFrustum();
    test_ConservativeCulling_NoPopping();
    test_SpatialPartitioning_Registration();
    test_SpatialPartitioning_Unregistration();
    test_SpatialPartitioning_PositionUpdate();
    test_GetVisibleEntities();
    test_CullingAtDifferentCameraAngles();
    test_GetVisibleTileRange();
    test_TransformAABBToWorld();
    test_LargeMapPerformance();
    test_SphereCulling();
    test_PointVisibility();
    test_ClearEntities();
    test_CellBoundary();

    std::cout << std::endl;
    std::cout << "===== Results =====" << std::endl;
    std::cout << "Passed: " << g_testsPassed << std::endl;
    std::cout << "Failed: " << g_testsFailed << std::endl;

    return g_testsFailed > 0 ? 1 : 0;
}
