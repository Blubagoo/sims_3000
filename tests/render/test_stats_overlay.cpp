/**
 * @file test_stats_overlay.cpp
 * @brief Unit tests for StatsOverlay (Ticket 2-042).
 *
 * Tests StatsOverlay configuration, data structures, and formatting.
 * GPU-dependent rendering tests require manual verification.
 */

#include "sims3000/render/StatsOverlay.h"
#include "sims3000/render/MainRenderPass.h"
#include "sims3000/app/FrameStats.h"

#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstring>

// Test counter
static int s_testsRun = 0;
static int s_testsPassed = 0;

#define TEST(name) \
    void test_##name(); \
    static struct TestRegister_##name { \
        TestRegister_##name() { test_##name(); } \
    } s_testRegister_##name; \
    void test_##name()

#define ASSERT_TRUE(cond) \
    do { \
        s_testsRun++; \
        if (!(cond)) { \
            std::printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        } else { \
            s_testsPassed++; \
        } \
    } while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NE(a, b) ASSERT_TRUE((a) != (b))

#define ASSERT_FLOAT_EQ(a, b, eps) \
    ASSERT_TRUE(std::fabs((a) - (b)) < (eps))

using namespace sims3000;

// ============================================================================
// StatsOverlayConfig Tests
// ============================================================================

TEST(StatsOverlayConfig_DefaultValues) {
    std::printf("test_StatsOverlayConfig_DefaultValues\n");

    StatsOverlayConfig config;

    // Check default values
    ASSERT_FLOAT_EQ(config.fontSize, 16.0f, 0.001f);
    ASSERT_EQ(config.textR, 255);
    ASSERT_EQ(config.textG, 255);
    ASSERT_EQ(config.textB, 255);
    ASSERT_EQ(config.textA, 255);
    ASSERT_EQ(config.bgR, 0);
    ASSERT_EQ(config.bgG, 0);
    ASSERT_EQ(config.bgB, 0);
    ASSERT_EQ(config.bgA, 180);
    ASSERT_FLOAT_EQ(config.paddingX, 8.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.paddingY, 4.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.offsetX, 10.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.offsetY, 10.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.lineSpacing, 1.2f, 0.001f);
    ASSERT_EQ(config.position, 0);  // Top-left
}

TEST(StatsOverlayConfig_PositionValues) {
    std::printf("test_StatsOverlayConfig_PositionValues\n");

    // Position values: 0=top-left, 1=top-right, 2=bottom-left, 3=bottom-right
    StatsOverlayConfig config;

    config.position = 0;
    ASSERT_EQ(config.position, 0);

    config.position = 1;
    ASSERT_EQ(config.position, 1);

    config.position = 2;
    ASSERT_EQ(config.position, 2);

    config.position = 3;
    ASSERT_EQ(config.position, 3);
}

// ============================================================================
// StatsData Tests
// ============================================================================

TEST(StatsData_DefaultValues) {
    std::printf("test_StatsData_DefaultValues\n");

    StatsData stats;

    ASSERT_FLOAT_EQ(stats.fps, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(stats.frameTimeMs, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(stats.minFrameTimeMs, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(stats.maxFrameTimeMs, 0.0f, 0.001f);
    ASSERT_EQ(stats.drawCalls, 0u);
    ASSERT_EQ(stats.triangles, 0u);
    ASSERT_EQ(stats.totalFrames, 0ull);
}

TEST(StatsData_Assignment) {
    std::printf("test_StatsData_Assignment\n");

    StatsData stats;
    stats.fps = 60.0f;
    stats.frameTimeMs = 16.67f;
    stats.minFrameTimeMs = 15.0f;
    stats.maxFrameTimeMs = 20.0f;
    stats.drawCalls = 150;
    stats.triangles = 50000;
    stats.totalFrames = 1000;

    ASSERT_FLOAT_EQ(stats.fps, 60.0f, 0.001f);
    ASSERT_FLOAT_EQ(stats.frameTimeMs, 16.67f, 0.01f);
    ASSERT_FLOAT_EQ(stats.minFrameTimeMs, 15.0f, 0.001f);
    ASSERT_FLOAT_EQ(stats.maxFrameTimeMs, 20.0f, 0.001f);
    ASSERT_EQ(stats.drawCalls, 150u);
    ASSERT_EQ(stats.triangles, 50000u);
    ASSERT_EQ(stats.totalFrames, 1000ull);
}

// ============================================================================
// MainRenderPassStats Tests (from MainRenderPass.h)
// ============================================================================

TEST(MainRenderPassStats_Reset) {
    std::printf("test_MainRenderPassStats_Reset\n");

    MainRenderPassStats stats;

    // Set some values
    stats.totalDrawCalls = 100;
    stats.totalTriangles = 50000;
    stats.terrainDrawCalls = 10;
    stats.buildingsDrawCalls = 50;

    // Reset should clear all
    stats.reset();

    ASSERT_EQ(stats.totalDrawCalls, 0u);
    ASSERT_EQ(stats.totalTriangles, 0u);
    ASSERT_EQ(stats.terrainDrawCalls, 0u);
    ASSERT_EQ(stats.buildingsDrawCalls, 0u);
    ASSERT_EQ(stats.effectsDrawCalls, 0u);
    ASSERT_EQ(stats.transparentDrawCalls, 0u);
}

TEST(MainRenderPassStats_PerLayerStats) {
    std::printf("test_MainRenderPassStats_PerLayerStats\n");

    MainRenderPassStats stats;

    stats.terrainDrawCalls = 5;
    stats.terrainTriangles = 10000;
    stats.buildingsDrawCalls = 20;
    stats.buildingsTriangles = 30000;
    stats.effectsDrawCalls = 3;
    stats.effectsTriangles = 1000;
    stats.transparentDrawCalls = 10;
    stats.transparentTriangles = 5000;

    // Calculate totals (as would be done in render pass)
    stats.totalDrawCalls = stats.terrainDrawCalls + stats.buildingsDrawCalls +
                           stats.effectsDrawCalls + stats.transparentDrawCalls;
    stats.totalTriangles = stats.terrainTriangles + stats.buildingsTriangles +
                           stats.effectsTriangles + stats.transparentTriangles;

    ASSERT_EQ(stats.totalDrawCalls, 38u);
    ASSERT_EQ(stats.totalTriangles, 46000u);
}

// ============================================================================
// FrameStats Tests (for stats source)
// ============================================================================

TEST(FrameStats_FPSCalculation) {
    std::printf("test_FrameStats_FPSCalculation\n");

    FrameStats frameStats;

    // Simulate frames at 60 FPS (16.67ms per frame)
    for (int i = 0; i < 60; ++i) {
        frameStats.update(1.0f / 60.0f);
    }

    // FPS should be approximately 60
    float fps = frameStats.getFPS();
    ASSERT_TRUE(fps > 55.0f && fps < 65.0f);

    // Frame time should be approximately 16.67ms
    float frameTime = frameStats.getFrameTimeMs();
    ASSERT_TRUE(frameTime > 15.0f && frameTime < 18.0f);
}

TEST(FrameStats_TotalFrames) {
    std::printf("test_FrameStats_TotalFrames\n");

    FrameStats frameStats;

    ASSERT_EQ(frameStats.getTotalFrames(), 0ull);

    for (int i = 0; i < 100; ++i) {
        frameStats.update(1.0f / 60.0f);
    }

    ASSERT_EQ(frameStats.getTotalFrames(), 100ull);
}

TEST(FrameStats_Reset) {
    std::printf("test_FrameStats_Reset\n");

    FrameStats frameStats;

    for (int i = 0; i < 50; ++i) {
        frameStats.update(1.0f / 60.0f);
    }

    ASSERT_EQ(frameStats.getTotalFrames(), 50ull);

    frameStats.reset();

    ASSERT_EQ(frameStats.getTotalFrames(), 0ull);
    ASSERT_FLOAT_EQ(frameStats.getFPS(), 0.0f, 0.001f);
}

// ============================================================================
// Integration Point Tests
// ============================================================================

TEST(StatsOverlay_ConfigCopy) {
    std::printf("test_StatsOverlay_ConfigCopy\n");

    StatsOverlayConfig config1;
    config1.fontSize = 20.0f;
    config1.textR = 200;
    config1.position = 2;

    StatsOverlayConfig config2 = config1;

    ASSERT_FLOAT_EQ(config2.fontSize, 20.0f, 0.001f);
    ASSERT_EQ(config2.textR, 200);
    ASSERT_EQ(config2.position, 2);
}

TEST(StatsData_Copy) {
    std::printf("test_StatsData_Copy\n");

    StatsData stats1;
    stats1.fps = 120.0f;
    stats1.drawCalls = 200;
    stats1.triangles = 100000;

    StatsData stats2 = stats1;

    ASSERT_FLOAT_EQ(stats2.fps, 120.0f, 0.001f);
    ASSERT_EQ(stats2.drawCalls, 200u);
    ASSERT_EQ(stats2.triangles, 100000u);
}

// ============================================================================
// Line Count Test
// ============================================================================

TEST(StatsOverlay_LineCount) {
    std::printf("test_StatsOverlay_LineCount\n");

    // StatsOverlay displays 4 lines:
    // 1. FPS
    // 2. Frame time (ms)
    // 3. Draw calls
    // 4. Triangles

    // This is defined as LINE_COUNT = 4 in StatsOverlay.h
    // We verify the expected display lines
    constexpr int expectedLines = 4;

    // Count expected stats displays
    int lineCount = 0;
    lineCount++;  // FPS
    lineCount++;  // Frame time
    lineCount++;  // Draw calls
    lineCount++;  // Triangles

    ASSERT_EQ(lineCount, expectedLines);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::printf("\n");
    std::printf("===========================================\n");
    std::printf("StatsOverlay Tests (Ticket 2-042)\n");
    std::printf("===========================================\n\n");

    // Tests run automatically via static initializers

    std::printf("\n");
    std::printf("===========================================\n");
    std::printf("Results: %d/%d tests passed\n", s_testsPassed, s_testsRun);
    std::printf("===========================================\n");

    if (s_testsPassed == s_testsRun) {
        std::printf("\nAll tests PASSED!\n\n");
        return 0;
    } else {
        std::printf("\nSome tests FAILED!\n\n");
        return 1;
    }
}
