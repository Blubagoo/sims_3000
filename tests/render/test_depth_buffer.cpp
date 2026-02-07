/**
 * @file test_depth_buffer.cpp
 * @brief Unit tests for DepthBuffer class.
 *
 * Tests depth buffer creation, format selection, resize handling, and
 * render pass configuration. Note: GPU tests require a display/GPU device.
 */

#include "sims3000/render/DepthBuffer.h"
#include "sims3000/render/GPUDevice.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace sims3000;

// Test counters
static int g_testsPassed = 0;
static int g_testsFailed = 0;

// Test macros
#define TEST_CASE(name) \
    printf("\n[TEST] %s\n", name); \
    fflush(stdout)

#define EXPECT_TRUE(condition) \
    do { \
        if (condition) { \
            g_testsPassed++; \
            printf("  [PASS] %s\n", #condition); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] %s (line %d)\n", #condition, __LINE__); \
        } \
    } while(0)

#define EXPECT_FALSE(condition) \
    do { \
        if (!(condition)) { \
            g_testsPassed++; \
            printf("  [PASS] !(%s)\n", #condition); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] !(%s) (line %d)\n", #condition, __LINE__); \
        } \
    } while(0)

#define EXPECT_NOT_NULL(ptr) \
    do { \
        if ((ptr) != nullptr) { \
            g_testsPassed++; \
            printf("  [PASS] %s != nullptr\n", #ptr); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] %s == nullptr (line %d)\n", #ptr, __LINE__); \
        } \
    } while(0)

#define EXPECT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            g_testsPassed++; \
            printf("  [PASS] %s == nullptr\n", #ptr); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] %s != nullptr (line %d)\n", #ptr, __LINE__); \
        } \
    } while(0)

#define EXPECT_EQ(a, b) \
    do { \
        if ((a) == (b)) { \
            g_testsPassed++; \
            printf("  [PASS] %s == %s\n", #a, #b); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] %s != %s (line %d)\n", #a, #b, __LINE__); \
        } \
    } while(0)

#define EXPECT_FLOAT_EQ(a, b) \
    do { \
        if (std::fabs((a) - (b)) < 0.0001f) { \
            g_testsPassed++; \
            printf("  [PASS] %s == %s (%.4f)\n", #a, #b, static_cast<double>(a)); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] %s != %s (%.4f vs %.4f, line %d)\n", \
                   #a, #b, static_cast<double>(a), static_cast<double>(b), __LINE__); \
        } \
    } while(0)

// =============================================================================
// Test: Depth Format Name Conversion
// =============================================================================
void test_DepthFormatNameConversion() {
    TEST_CASE("Depth format name conversion");

    EXPECT_EQ(std::string(getDepthFormatName(DepthFormat::D32_FLOAT)),
              std::string("D32_FLOAT"));
    EXPECT_EQ(std::string(getDepthFormatName(DepthFormat::D24_UNORM_S8_UINT)),
              std::string("D24_UNORM_S8_UINT"));
}

// =============================================================================
// Test: Depth Buffer Creation with D32_FLOAT (requires GPU)
// =============================================================================
void test_DepthBufferCreationD32() {
    TEST_CASE("Depth buffer creation with D32_FLOAT format");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    DepthBuffer depthBuffer(device, 1280, 720);

    EXPECT_TRUE(depthBuffer.isValid());
    EXPECT_NOT_NULL(depthBuffer.getHandle());
    EXPECT_EQ(depthBuffer.getWidth(), 1280u);
    EXPECT_EQ(depthBuffer.getHeight(), 720u);
    EXPECT_EQ(depthBuffer.getFormat(), DepthFormat::D32_FLOAT);
    EXPECT_EQ(depthBuffer.getSDLFormat(), SDL_GPU_TEXTUREFORMAT_D32_FLOAT);
    EXPECT_FALSE(depthBuffer.hasStencil());
}

// =============================================================================
// Test: Depth Buffer Creation with D24_S8 (requires GPU)
// =============================================================================
void test_DepthBufferCreationD24S8() {
    TEST_CASE("Depth buffer creation with D24_UNORM_S8_UINT format");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    DepthBuffer depthBuffer(device, 800, 600, DepthFormat::D24_UNORM_S8_UINT);

    EXPECT_TRUE(depthBuffer.isValid());
    EXPECT_NOT_NULL(depthBuffer.getHandle());
    EXPECT_EQ(depthBuffer.getWidth(), 800u);
    EXPECT_EQ(depthBuffer.getHeight(), 600u);
    EXPECT_EQ(depthBuffer.getFormat(), DepthFormat::D24_UNORM_S8_UINT);
    EXPECT_EQ(depthBuffer.getSDLFormat(), SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT);
    EXPECT_TRUE(depthBuffer.hasStencil());
}

// =============================================================================
// Test: Move Semantics (requires GPU)
// =============================================================================
void test_MoveSemantics() {
    TEST_CASE("Move constructor and assignment");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    DepthBuffer depthBuffer1(device, 640, 480);
    SDL_GPUTexture* originalHandle = depthBuffer1.getHandle();

    EXPECT_TRUE(depthBuffer1.isValid());

    // Move construct
    DepthBuffer depthBuffer2(std::move(depthBuffer1));

    EXPECT_FALSE(depthBuffer1.isValid());
    EXPECT_NULL(depthBuffer1.getHandle());
    EXPECT_EQ(depthBuffer1.getWidth(), 0u);
    EXPECT_EQ(depthBuffer1.getHeight(), 0u);

    EXPECT_TRUE(depthBuffer2.isValid());
    EXPECT_EQ(depthBuffer2.getHandle(), originalHandle);
    EXPECT_EQ(depthBuffer2.getWidth(), 640u);
    EXPECT_EQ(depthBuffer2.getHeight(), 480u);

    // Move assign
    DepthBuffer depthBuffer3(device, 320, 240);
    depthBuffer3 = std::move(depthBuffer2);

    EXPECT_FALSE(depthBuffer2.isValid());
    EXPECT_TRUE(depthBuffer3.isValid());
    EXPECT_EQ(depthBuffer3.getHandle(), originalHandle);
    EXPECT_EQ(depthBuffer3.getWidth(), 640u);
    EXPECT_EQ(depthBuffer3.getHeight(), 480u);
}

// =============================================================================
// Test: Resize (requires GPU)
// =============================================================================
void test_Resize() {
    TEST_CASE("Depth buffer resize");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    DepthBuffer depthBuffer(device, 640, 480);
    EXPECT_TRUE(depthBuffer.isValid());
    EXPECT_EQ(depthBuffer.getWidth(), 640u);
    EXPECT_EQ(depthBuffer.getHeight(), 480u);

    SDL_GPUTexture* originalHandle = depthBuffer.getHandle();

    // Resize to larger
    EXPECT_TRUE(depthBuffer.resize(1280, 720));
    EXPECT_TRUE(depthBuffer.isValid());
    EXPECT_EQ(depthBuffer.getWidth(), 1280u);
    EXPECT_EQ(depthBuffer.getHeight(), 720u);

    // Texture handle should be different after resize
    EXPECT_TRUE(depthBuffer.getHandle() != originalHandle);
    printf("  [INFO] Texture recreated on resize (new handle)\n");

    // Resize to same dimensions should be no-op
    SDL_GPUTexture* currentHandle = depthBuffer.getHandle();
    EXPECT_TRUE(depthBuffer.resize(1280, 720));
    EXPECT_EQ(depthBuffer.getHandle(), currentHandle);
    printf("  [INFO] Same dimensions - texture preserved\n");

    // Resize to smaller
    EXPECT_TRUE(depthBuffer.resize(320, 240));
    EXPECT_TRUE(depthBuffer.isValid());
    EXPECT_EQ(depthBuffer.getWidth(), 320u);
    EXPECT_EQ(depthBuffer.getHeight(), 240u);
}

// =============================================================================
// Test: Resize to Zero Dimensions (requires GPU)
// =============================================================================
void test_ResizeZeroDimensions() {
    TEST_CASE("Resize to zero dimensions should fail");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    DepthBuffer depthBuffer(device, 640, 480);
    EXPECT_TRUE(depthBuffer.isValid());

    // Resize to zero width should fail
    EXPECT_FALSE(depthBuffer.resize(0, 480));
    EXPECT_FALSE(depthBuffer.getLastError().empty());
    printf("  [INFO] Error: %s\n", depthBuffer.getLastError().c_str());

    // Resize to zero height should fail
    EXPECT_FALSE(depthBuffer.resize(640, 0));

    // Resize to zero both should fail
    EXPECT_FALSE(depthBuffer.resize(0, 0));
}

// =============================================================================
// Test: Depth Stencil Target Info Default (requires GPU)
// =============================================================================
void test_DepthStencilTargetInfoDefault() {
    TEST_CASE("Depth stencil target info with default settings");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    DepthBuffer depthBuffer(device, 640, 480);
    EXPECT_TRUE(depthBuffer.isValid());

    SDL_GPUDepthStencilTargetInfo info = depthBuffer.getDepthStencilTargetInfo();

    EXPECT_EQ(info.texture, depthBuffer.getHandle());
    EXPECT_EQ(info.load_op, SDL_GPU_LOADOP_CLEAR);
    EXPECT_EQ(info.store_op, SDL_GPU_STOREOP_DONT_CARE);
    EXPECT_FLOAT_EQ(info.clear_depth, 1.0f);
    EXPECT_EQ(info.clear_stencil, 0u);
    EXPECT_TRUE(info.cycle);

    printf("  [INFO] Default clear depth: %.1f (far plane)\n", info.clear_depth);
}

// =============================================================================
// Test: Depth Stencil Target Info Custom Clear (requires GPU)
// =============================================================================
void test_DepthStencilTargetInfoCustomClear() {
    TEST_CASE("Depth stencil target info with custom clear values");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    // Test with D24_S8 format to include stencil
    DepthBuffer depthBuffer(device, 640, 480, DepthFormat::D24_UNORM_S8_UINT);
    EXPECT_TRUE(depthBuffer.isValid());

    // Custom clear depth only
    SDL_GPUDepthStencilTargetInfo info1 = depthBuffer.getDepthStencilTargetInfo(0.5f);
    EXPECT_FLOAT_EQ(info1.clear_depth, 0.5f);
    EXPECT_EQ(info1.clear_stencil, 0u);

    // Custom clear depth and stencil
    SDL_GPUDepthStencilTargetInfo info2 = depthBuffer.getDepthStencilTargetInfo(0.0f, 128);
    EXPECT_FLOAT_EQ(info2.clear_depth, 0.0f);
    EXPECT_EQ(info2.clear_stencil, 128u);

    // Stencil load op should be CLEAR when format has stencil
    EXPECT_EQ(info2.stencil_load_op, SDL_GPU_LOADOP_CLEAR);
}

// =============================================================================
// Test: Depth Stencil Target Info Preserve (requires GPU)
// =============================================================================
void test_DepthStencilTargetInfoPreserve() {
    TEST_CASE("Depth stencil target info for preservation");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    DepthBuffer depthBuffer(device, 640, 480, DepthFormat::D24_UNORM_S8_UINT);
    EXPECT_TRUE(depthBuffer.isValid());

    SDL_GPUDepthStencilTargetInfo info = depthBuffer.getDepthStencilTargetInfoPreserve();

    EXPECT_EQ(info.texture, depthBuffer.getHandle());
    EXPECT_EQ(info.load_op, SDL_GPU_LOADOP_LOAD);
    EXPECT_EQ(info.store_op, SDL_GPU_STOREOP_STORE);
    EXPECT_EQ(info.stencil_load_op, SDL_GPU_LOADOP_LOAD);
    EXPECT_EQ(info.stencil_store_op, SDL_GPU_STOREOP_STORE);
    EXPECT_FALSE(info.cycle);

    printf("  [INFO] Preserve mode: LOAD/STORE operations\n");
}

// =============================================================================
// Test: D32_FLOAT No Stencil Operations (requires GPU)
// =============================================================================
void test_D32FloatNoStencil() {
    TEST_CASE("D32_FLOAT format stencil operations");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    DepthBuffer depthBuffer(device, 640, 480, DepthFormat::D32_FLOAT);
    EXPECT_TRUE(depthBuffer.isValid());
    EXPECT_FALSE(depthBuffer.hasStencil());

    // Stencil load op should be DONT_CARE for D32_FLOAT
    SDL_GPUDepthStencilTargetInfo info = depthBuffer.getDepthStencilTargetInfo();
    EXPECT_EQ(info.stencil_load_op, SDL_GPU_LOADOP_DONT_CARE);
    EXPECT_EQ(info.stencil_store_op, SDL_GPU_STOREOP_DONT_CARE);

    printf("  [INFO] D32_FLOAT: stencil ops set to DONT_CARE\n");
}

// =============================================================================
// Test: Multiple Depth Buffers (requires GPU)
// =============================================================================
void test_MultipleDepthBuffers() {
    TEST_CASE("Multiple depth buffers from same device");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    DepthBuffer depthBuffer1(device, 640, 480, DepthFormat::D32_FLOAT);
    DepthBuffer depthBuffer2(device, 1280, 720, DepthFormat::D24_UNORM_S8_UINT);

    EXPECT_TRUE(depthBuffer1.isValid());
    EXPECT_TRUE(depthBuffer2.isValid());

    // Different textures
    EXPECT_TRUE(depthBuffer1.getHandle() != depthBuffer2.getHandle());

    // Different formats
    EXPECT_EQ(depthBuffer1.getFormat(), DepthFormat::D32_FLOAT);
    EXPECT_EQ(depthBuffer2.getFormat(), DepthFormat::D24_UNORM_S8_UINT);

    // Different dimensions
    EXPECT_EQ(depthBuffer1.getWidth(), 640u);
    EXPECT_EQ(depthBuffer2.getWidth(), 1280u);
}

// =============================================================================
// Test: Cleanup on Destruction (requires GPU)
// =============================================================================
void test_CleanupOnDestruction() {
    TEST_CASE("Cleanup on destruction");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    {
        DepthBuffer depthBuffer(device, 1920, 1080);
        EXPECT_TRUE(depthBuffer.isValid());
        // DepthBuffer goes out of scope here
    }

    // Device should still be valid after depth buffer destruction
    EXPECT_TRUE(device.isValid());
    printf("  [PASS] Depth buffer cleanup completed successfully\n");
    g_testsPassed++;
}

// =============================================================================
// Test: Format SDL Conversion
// =============================================================================
void test_FormatSDLConversion() {
    TEST_CASE("Format to SDL format conversion");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    DepthBuffer d32Buffer(device, 640, 480, DepthFormat::D32_FLOAT);
    DepthBuffer d24s8Buffer(device, 640, 480, DepthFormat::D24_UNORM_S8_UINT);

    EXPECT_EQ(d32Buffer.getSDLFormat(), SDL_GPU_TEXTUREFORMAT_D32_FLOAT);
    EXPECT_EQ(d24s8Buffer.getSDLFormat(), SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT);
}

// =============================================================================
// Main
// =============================================================================
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("DepthBuffer Unit Tests (Ticket 2-014)\n");
    printf("========================================\n");

    // Initialize SDL for video (required for GPU)
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("[FATAL] Failed to initialize SDL: %s\n", SDL_GetError());
        printf("Some tests will be skipped.\n");
    }

    // Run tests
    test_DepthFormatNameConversion();
    test_DepthBufferCreationD32();
    test_DepthBufferCreationD24S8();
    test_MoveSemantics();
    test_Resize();
    test_ResizeZeroDimensions();
    test_DepthStencilTargetInfoDefault();
    test_DepthStencilTargetInfoCustomClear();
    test_DepthStencilTargetInfoPreserve();
    test_D32FloatNoStencil();
    test_MultipleDepthBuffers();
    test_CleanupOnDestruction();
    test_FormatSDLConversion();

    // Summary
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("========================================\n");

    SDL_Quit();

    return g_testsFailed > 0 ? 1 : 0;
}
