/**
 * @file test_cursor_renderer.cpp
 * @brief Unit tests for CursorRenderer (Ticket 2-045).
 *
 * Tests:
 * - PlayerCursor struct
 * - FactionColors palette
 * - CursorRenderer update and prepare methods
 * - Staleness detection and fading
 * - ICursorSync interface
 */

#include "sims3000/render/CursorRenderer.h"
#include "sims3000/render/PlayerCursor.h"
#include "sims3000/render/CameraState.h"
#include "sims3000/sync/ICursorSync.h"

#include <cstdio>
#include <cmath>
#include <vector>

// ============================================================================
// Test Helpers
// ============================================================================

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::printf("FAIL: %s\n  %s:%d\n", message, __FILE__, __LINE__); \
            ++g_testsFailed; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQ(a, b, epsilon, message) \
    TEST_ASSERT(std::abs((a) - (b)) < (epsilon), message)

#define TEST_PASS() \
    do { \
        ++g_testsPassed; \
    } while(0)

// ============================================================================
// Mock ICursorSync for Testing
// ============================================================================

class MockCursorSync : public sims3000::ICursorSync {
public:
    std::vector<sims3000::PlayerCursor> cursors;
    glm::vec3 lastLocalCursorPos{0.0f};
    sims3000::PlayerID localPlayerId = 1;
    bool syncActive = true;

    std::vector<sims3000::PlayerCursor> getPlayerCursors() const override {
        return cursors;
    }

    void updateLocalCursor(const glm::vec3& worldPosition) override {
        lastLocalCursorPos = worldPosition;
    }

    sims3000::PlayerID getLocalPlayerId() const override {
        return localPlayerId;
    }

    bool isSyncActive() const override {
        return syncActive;
    }
};

// ============================================================================
// PlayerCursor Tests
// ============================================================================

void test_PlayerCursor_DefaultConstruction() {
    std::printf("test_PlayerCursor_DefaultConstruction...");

    sims3000::PlayerCursor cursor;

    TEST_ASSERT(cursor.player_id == 0, "Default player_id should be 0");
    TEST_ASSERT(cursor.is_active == false, "Default is_active should be false");
    TEST_ASSERT_FLOAT_EQ(cursor.time_since_update, 0.0f, 0.001f, "Default time_since_update should be 0");
    TEST_ASSERT_FLOAT_EQ(cursor.world_position.x, 0.0f, 0.001f, "Default world_position.x should be 0");
    TEST_ASSERT_FLOAT_EQ(cursor.world_position.y, 0.0f, 0.001f, "Default world_position.y should be 0");
    TEST_ASSERT_FLOAT_EQ(cursor.world_position.z, 0.0f, 0.001f, "Default world_position.z should be 0");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_PlayerCursor_SizeCheck() {
    std::printf("test_PlayerCursor_SizeCheck...");

    // Size should be 20 bytes as per static_assert
    // Layout: player_id(1) + is_active(1) + padding(2) + world_position(12) + time_since_update(4) = 20
    TEST_ASSERT(sizeof(sims3000::PlayerCursor) == 20, "PlayerCursor size should be 20 bytes");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_PlayerCursor_Staleness() {
    std::printf("test_PlayerCursor_Staleness...");

    sims3000::PlayerCursor cursor;
    cursor.is_active = true;
    cursor.time_since_update = 0.0f;

    // Fresh cursor should not be stale
    TEST_ASSERT(!cursor.isStale(2.0f), "Fresh cursor should not be stale");

    // Update time
    cursor.updateTime(1.0f);
    TEST_ASSERT(!cursor.isStale(2.0f), "Cursor at 1s should not be stale with 2s threshold");

    // Update more
    cursor.updateTime(1.5f);
    TEST_ASSERT(cursor.isStale(2.0f), "Cursor at 2.5s should be stale with 2s threshold");

    // Reset time
    cursor.resetTime();
    TEST_ASSERT(!cursor.isStale(2.0f), "Reset cursor should not be stale");
    TEST_ASSERT_FLOAT_EQ(cursor.time_since_update, 0.0f, 0.001f, "Reset time should be 0");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_PlayerCursor_FactionColor() {
    std::printf("test_PlayerCursor_FactionColor...");

    sims3000::PlayerCursor cursor;

    // Inactive cursor should return inactive color
    cursor.is_active = false;
    cursor.player_id = 1;
    glm::vec4 inactiveColor = cursor.getFactionColor();
    TEST_ASSERT_FLOAT_EQ(inactiveColor.a, 0.5f, 0.001f, "Inactive cursor should have 0.5 alpha");

    // Active cursor should return faction color
    cursor.is_active = true;
    cursor.player_id = 1;
    glm::vec4 color1 = cursor.getFactionColor();
    TEST_ASSERT_FLOAT_EQ(color1.a, 1.0f, 0.001f, "Active cursor should have 1.0 alpha");

    // Different players have different colors
    cursor.player_id = 2;
    glm::vec4 color2 = cursor.getFactionColor();
    TEST_ASSERT(color1.r != color2.r || color1.g != color2.g || color1.b != color2.b,
                "Different players should have different colors");

    std::printf(" PASSED\n");
    TEST_PASS();
}

// ============================================================================
// FactionColors Tests
// ============================================================================

void test_FactionColors_DistinctColors() {
    std::printf("test_FactionColors_DistinctColors...");

    // Get all player colors
    glm::vec4 c1 = sims3000::FactionColors::PLAYER_1;
    glm::vec4 c2 = sims3000::FactionColors::PLAYER_2;
    glm::vec4 c3 = sims3000::FactionColors::PLAYER_3;
    glm::vec4 c4 = sims3000::FactionColors::PLAYER_4;

    // Verify they are all different
    auto colorsDifferent = [](const glm::vec4& a, const glm::vec4& b) {
        return a.r != b.r || a.g != b.g || a.b != b.b;
    };

    TEST_ASSERT(colorsDifferent(c1, c2), "Player 1 and 2 should have different colors");
    TEST_ASSERT(colorsDifferent(c1, c3), "Player 1 and 3 should have different colors");
    TEST_ASSERT(colorsDifferent(c1, c4), "Player 1 and 4 should have different colors");
    TEST_ASSERT(colorsDifferent(c2, c3), "Player 2 and 3 should have different colors");
    TEST_ASSERT(colorsDifferent(c2, c4), "Player 2 and 4 should have different colors");
    TEST_ASSERT(colorsDifferent(c3, c4), "Player 3 and 4 should have different colors");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_FactionColors_GetColorForPlayer() {
    std::printf("test_FactionColors_GetColorForPlayer...");

    // Test each player ID
    glm::vec4 c0 = sims3000::FactionColors::getColorForPlayer(0);
    glm::vec4 c1 = sims3000::FactionColors::getColorForPlayer(1);
    glm::vec4 c2 = sims3000::FactionColors::getColorForPlayer(2);
    glm::vec4 c3 = sims3000::FactionColors::getColorForPlayer(3);
    glm::vec4 c4 = sims3000::FactionColors::getColorForPlayer(4);
    glm::vec4 c5 = sims3000::FactionColors::getColorForPlayer(5); // Unknown

    TEST_ASSERT(c0 == sims3000::FactionColors::NEUTRAL, "Player 0 should be NEUTRAL");
    TEST_ASSERT(c1 == sims3000::FactionColors::PLAYER_1, "Player 1 color mismatch");
    TEST_ASSERT(c2 == sims3000::FactionColors::PLAYER_2, "Player 2 color mismatch");
    TEST_ASSERT(c3 == sims3000::FactionColors::PLAYER_3, "Player 3 color mismatch");
    TEST_ASSERT(c4 == sims3000::FactionColors::PLAYER_4, "Player 4 color mismatch");
    TEST_ASSERT(c5 == sims3000::FactionColors::NEUTRAL, "Unknown player should be NEUTRAL");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_FactionColors_GetAllColors() {
    std::printf("test_FactionColors_GetAllColors...");

    auto colors = sims3000::FactionColors::getAllColors();

    TEST_ASSERT(colors.size() == 5, "getAllColors should return 5 colors");
    TEST_ASSERT(colors[0] == sims3000::FactionColors::NEUTRAL, "Index 0 should be NEUTRAL");
    TEST_ASSERT(colors[1] == sims3000::FactionColors::PLAYER_1, "Index 1 should be PLAYER_1");
    TEST_ASSERT(colors[2] == sims3000::FactionColors::PLAYER_2, "Index 2 should be PLAYER_2");
    TEST_ASSERT(colors[3] == sims3000::FactionColors::PLAYER_3, "Index 3 should be PLAYER_3");
    TEST_ASSERT(colors[4] == sims3000::FactionColors::PLAYER_4, "Index 4 should be PLAYER_4");

    std::printf(" PASSED\n");
    TEST_PASS();
}

// ============================================================================
// StubCursorSync Tests
// ============================================================================

void test_StubCursorSync_Returns_Empty() {
    std::printf("test_StubCursorSync_Returns_Empty...");

    sims3000::StubCursorSync stub;

    auto cursors = stub.getPlayerCursors();
    TEST_ASSERT(cursors.empty(), "Stub should return empty cursor list");
    TEST_ASSERT(stub.getLocalPlayerId() == 1, "Stub should return player 1 as local");
    TEST_ASSERT(!stub.isSyncActive(), "Stub should report sync as inactive");

    std::printf(" PASSED\n");
    TEST_PASS();
}

// ============================================================================
// CursorRenderer Tests
// ============================================================================

void test_CursorRenderer_Construction() {
    std::printf("test_CursorRenderer_Construction...");

    sims3000::CursorIndicatorConfig config;
    config.scale = 0.5f;
    config.emissive_intensity = 0.8f;

    sims3000::CursorRenderer renderer(nullptr, config);

    TEST_ASSERT(renderer.isEnabled() == false, "Renderer without sync should not be enabled");
    TEST_ASSERT(renderer.getVisibleCursorCount() == 0, "Initial visible count should be 0");
    TEST_ASSERT_FLOAT_EQ(renderer.getConfig().scale, 0.5f, 0.001f, "Config scale should match");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_CursorRenderer_SetCursorSync() {
    std::printf("test_CursorRenderer_SetCursorSync...");

    MockCursorSync mockSync;
    sims3000::CursorRenderer renderer;

    TEST_ASSERT(!renderer.isEnabled(), "Renderer should be disabled without sync");

    renderer.setCursorSync(&mockSync);
    TEST_ASSERT(renderer.isEnabled(), "Renderer should be enabled with sync");

    renderer.setEnabled(false);
    TEST_ASSERT(!renderer.isEnabled(), "Renderer should be disabled when explicitly disabled");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_CursorRenderer_UpdateLocalCursor() {
    std::printf("test_CursorRenderer_UpdateLocalCursor...");

    MockCursorSync mockSync;
    sims3000::CursorRenderer renderer(&mockSync);

    glm::vec3 pos{10.0f, 0.0f, 20.0f};
    renderer.updateLocalCursorPosition(pos);

    TEST_ASSERT_FLOAT_EQ(mockSync.lastLocalCursorPos.x, 10.0f, 0.001f, "Local cursor X should be forwarded");
    TEST_ASSERT_FLOAT_EQ(mockSync.lastLocalCursorPos.z, 20.0f, 0.001f, "Local cursor Z should be forwarded");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_CursorRenderer_PrepareCursors_Empty() {
    std::printf("test_CursorRenderer_PrepareCursors_Empty...");

    MockCursorSync mockSync;
    mockSync.cursors.clear(); // No cursors

    sims3000::CursorRenderer renderer(&mockSync);
    sims3000::CameraState cameraState;
    glm::mat4 viewProj = glm::mat4(1.0f);

    auto renderData = renderer.prepareCursors(cameraState, viewProj, 1920.0f, 1080.0f);

    TEST_ASSERT(renderData.empty(), "Should return empty for no cursors");
    TEST_ASSERT(renderer.getVisibleCursorCount() == 0, "Visible count should be 0");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_CursorRenderer_PrepareCursors_SyncInactive() {
    std::printf("test_CursorRenderer_PrepareCursors_SyncInactive...");

    MockCursorSync mockSync;
    mockSync.syncActive = false;

    sims3000::PlayerCursor cursor;
    cursor.player_id = 2;
    cursor.is_active = true;
    cursor.world_position = glm::vec3(10.0f, 0.0f, 10.0f);
    mockSync.cursors.push_back(cursor);

    sims3000::CursorRenderer renderer(&mockSync);
    sims3000::CameraState cameraState;
    glm::mat4 viewProj = glm::mat4(1.0f);

    auto renderData = renderer.prepareCursors(cameraState, viewProj, 1920.0f, 1080.0f);

    TEST_ASSERT(renderData.empty(), "Should return empty when sync is inactive");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_CursorRenderer_PrepareCursors_ActiveCursor() {
    std::printf("test_CursorRenderer_PrepareCursors_ActiveCursor...");

    MockCursorSync mockSync;

    sims3000::PlayerCursor cursor;
    cursor.player_id = 2;
    cursor.is_active = true;
    cursor.world_position = glm::vec3(10.0f, 0.0f, 10.0f);
    cursor.time_since_update = 0.0f;
    mockSync.cursors.push_back(cursor);

    sims3000::CursorRenderer renderer(&mockSync);
    sims3000::CameraState cameraState;

    // Simple identity view-proj (cursor should be visible)
    glm::mat4 viewProj = glm::mat4(1.0f);

    auto renderData = renderer.prepareCursors(cameraState, viewProj, 1920.0f, 1080.0f);

    // With identity matrix, the cursor at (10, 0, 10) should be visible
    TEST_ASSERT(renderData.size() == 1, "Should render 1 cursor");
    TEST_ASSERT(renderData[0].visible, "Cursor should be visible");
    TEST_ASSERT(renderData[0].player_id == 2, "Player ID should match");
    TEST_ASSERT_FLOAT_EQ(renderData[0].tint_color.a, 1.0f, 0.001f, "Fresh cursor should have full alpha");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_CursorRenderer_PrepareCursors_InactiveCursor() {
    std::printf("test_CursorRenderer_PrepareCursors_InactiveCursor...");

    MockCursorSync mockSync;

    sims3000::PlayerCursor cursor;
    cursor.player_id = 2;
    cursor.is_active = false; // Inactive
    cursor.world_position = glm::vec3(10.0f, 0.0f, 10.0f);
    mockSync.cursors.push_back(cursor);

    sims3000::CursorRenderer renderer(&mockSync);
    sims3000::CameraState cameraState;
    glm::mat4 viewProj = glm::mat4(1.0f);

    auto renderData = renderer.prepareCursors(cameraState, viewProj, 1920.0f, 1080.0f);

    TEST_ASSERT(renderData.empty(), "Inactive cursors should not be rendered");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_CursorRenderer_PrepareCursors_StaleCursor() {
    std::printf("test_CursorRenderer_PrepareCursors_StaleCursor...");

    MockCursorSync mockSync;

    sims3000::PlayerCursor cursor;
    cursor.player_id = 2;
    cursor.is_active = true;
    cursor.world_position = glm::vec3(10.0f, 0.0f, 10.0f);
    cursor.time_since_update = 3.0f; // Stale (default threshold is 2.0)
    mockSync.cursors.push_back(cursor);

    sims3000::CursorIndicatorConfig config;
    config.stale_threshold = 2.0f;
    config.stale_fade_duration = 1.0f;

    sims3000::CursorRenderer renderer(&mockSync, config);
    sims3000::CameraState cameraState;
    glm::mat4 viewProj = glm::mat4(1.0f);

    auto renderData = renderer.prepareCursors(cameraState, viewProj, 1920.0f, 1080.0f);

    // Cursor is stale but within fade duration (3.0 - 2.0 = 1.0s into fade)
    // At exactly fade duration end, alpha should be 0
    TEST_ASSERT(renderData.size() == 0 || renderData[0].tint_color.a < 1.0f,
                "Stale cursor should have reduced alpha or be hidden");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_CursorRenderer_PrepareCursors_MultipleCursors() {
    std::printf("test_CursorRenderer_PrepareCursors_MultipleCursors...");

    MockCursorSync mockSync;

    // Add 3 active cursors
    for (sims3000::PlayerID i = 2; i <= 4; ++i) {
        sims3000::PlayerCursor cursor;
        cursor.player_id = i;
        cursor.is_active = true;
        cursor.world_position = glm::vec3(static_cast<float>(i) * 10.0f, 0.0f, 10.0f);
        cursor.time_since_update = 0.0f;
        mockSync.cursors.push_back(cursor);
    }

    sims3000::CursorRenderer renderer(&mockSync);
    sims3000::CameraState cameraState;
    glm::mat4 viewProj = glm::mat4(1.0f);

    auto renderData = renderer.prepareCursors(cameraState, viewProj, 1920.0f, 1080.0f);

    TEST_ASSERT(renderData.size() == 3, "Should render 3 cursors");
    TEST_ASSERT(renderer.getVisibleCursorCount() == 3, "Visible count should be 3");

    // Check faction colors are different
    TEST_ASSERT(renderData[0].tint_color != renderData[1].tint_color,
                "Different players should have different colors");
    TEST_ASSERT(renderData[1].tint_color != renderData[2].tint_color,
                "Different players should have different colors");

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_CursorRenderer_Update_Animation() {
    std::printf("test_CursorRenderer_Update_Animation...");

    sims3000::CursorRenderer renderer;

    // Update should not crash without sync
    renderer.update(0.016f);
    renderer.update(0.016f);
    renderer.update(0.016f);

    // Should handle large delta times
    renderer.update(10.0f);

    std::printf(" PASSED\n");
    TEST_PASS();
}

void test_CursorIndicatorConfig_Defaults() {
    std::printf("test_CursorIndicatorConfig_Defaults...");

    sims3000::CursorIndicatorConfig config;

    TEST_ASSERT_FLOAT_EQ(config.scale, 0.5f, 0.001f, "Default scale should be 0.5");
    TEST_ASSERT_FLOAT_EQ(config.vertical_offset, 0.1f, 0.001f, "Default vertical_offset should be 0.1");
    TEST_ASSERT_FLOAT_EQ(config.emissive_intensity, 0.8f, 0.001f, "Default emissive_intensity should be 0.8");
    TEST_ASSERT_FLOAT_EQ(config.stale_threshold, 2.0f, 0.001f, "Default stale_threshold should be 2.0");
    TEST_ASSERT(config.show_labels, "Default show_labels should be true");

    std::printf(" PASSED\n");
    TEST_PASS();
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::printf("=== CursorRenderer Tests (Ticket 2-045) ===\n\n");

    // PlayerCursor tests
    test_PlayerCursor_DefaultConstruction();
    test_PlayerCursor_SizeCheck();
    test_PlayerCursor_Staleness();
    test_PlayerCursor_FactionColor();

    // FactionColors tests
    test_FactionColors_DistinctColors();
    test_FactionColors_GetColorForPlayer();
    test_FactionColors_GetAllColors();

    // StubCursorSync tests
    test_StubCursorSync_Returns_Empty();

    // CursorRenderer tests
    test_CursorRenderer_Construction();
    test_CursorRenderer_SetCursorSync();
    test_CursorRenderer_UpdateLocalCursor();
    test_CursorRenderer_PrepareCursors_Empty();
    test_CursorRenderer_PrepareCursors_SyncInactive();
    test_CursorRenderer_PrepareCursors_ActiveCursor();
    test_CursorRenderer_PrepareCursors_InactiveCursor();
    test_CursorRenderer_PrepareCursors_StaleCursor();
    test_CursorRenderer_PrepareCursors_MultipleCursors();
    test_CursorRenderer_Update_Animation();
    test_CursorIndicatorConfig_Defaults();

    std::printf("\n=== Results ===\n");
    std::printf("Passed: %d\n", g_testsPassed);
    std::printf("Failed: %d\n", g_testsFailed);

    return g_testsFailed > 0 ? 1 : 0;
}
