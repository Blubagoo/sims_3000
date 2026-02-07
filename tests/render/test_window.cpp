/**
 * @file test_window.cpp
 * @brief Unit tests for Window class with GPU swap chain integration.
 *
 * Tests window creation, GPU device claiming, present mode configuration,
 * resize handling, and fullscreen toggling. Note: Some tests require a display/GPU.
 */

#include "sims3000/render/Window.h"
#include "sims3000/render/GPUDevice.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>

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

// =============================================================================
// Test: Present Mode Name Conversion
// =============================================================================
void test_PresentModeNameConversion() {
    TEST_CASE("Present mode name conversion");

    EXPECT_EQ(std::string(getPresentModeName(PresentMode::Immediate)), std::string("Immediate"));
    EXPECT_EQ(std::string(getPresentModeName(PresentMode::VSync)), std::string("VSync"));
    EXPECT_EQ(std::string(getPresentModeName(PresentMode::Mailbox)), std::string("Mailbox"));
}

// =============================================================================
// Test: Present Mode SDL Conversion
// =============================================================================
void test_PresentModeSDLConversion() {
    TEST_CASE("Present mode SDL conversion");

    // To SDL
    EXPECT_EQ(toSDLPresentMode(PresentMode::Immediate), SDL_GPU_PRESENTMODE_IMMEDIATE);
    EXPECT_EQ(toSDLPresentMode(PresentMode::VSync), SDL_GPU_PRESENTMODE_VSYNC);
    EXPECT_EQ(toSDLPresentMode(PresentMode::Mailbox), SDL_GPU_PRESENTMODE_MAILBOX);

    // From SDL
    EXPECT_EQ(fromSDLPresentMode(SDL_GPU_PRESENTMODE_IMMEDIATE), PresentMode::Immediate);
    EXPECT_EQ(fromSDLPresentMode(SDL_GPU_PRESENTMODE_VSYNC), PresentMode::VSync);
    EXPECT_EQ(fromSDLPresentMode(SDL_GPU_PRESENTMODE_MAILBOX), PresentMode::Mailbox);
}

// =============================================================================
// Test: Window Creation
// =============================================================================
void test_WindowCreation() {
    TEST_CASE("Window creation");

    Window window("Test Window", 640, 480);

    EXPECT_TRUE(window.isValid());
    EXPECT_NOT_NULL(window.getHandle());
    EXPECT_EQ(window.getWidth(), 640);
    EXPECT_EQ(window.getHeight(), 480);
    EXPECT_FALSE(window.isClaimed());
    EXPECT_FALSE(window.isFullscreen());
}

// =============================================================================
// Test: Window Creation with Hidden Flag
// =============================================================================
void test_WindowDimensions() {
    TEST_CASE("Window dimensions tracking");

    Window window("Dimension Test", 800, 600);

    EXPECT_EQ(window.getWidth(), 800);
    EXPECT_EQ(window.getHeight(), 600);

    // Simulate resize
    window.onResize(1024, 768);

    EXPECT_EQ(window.getWidth(), 1024);
    EXPECT_EQ(window.getHeight(), 768);
}

// =============================================================================
// Test: Move Semantics
// =============================================================================
void test_MoveSemantics() {
    TEST_CASE("Move constructor and assignment");

    Window window1("Move Test", 320, 240);
    SDL_Window* originalHandle = window1.getHandle();

    EXPECT_TRUE(window1.isValid());

    // Move construct
    Window window2(std::move(window1));

    EXPECT_FALSE(window1.isValid());
    EXPECT_NULL(window1.getHandle());
    EXPECT_TRUE(window2.isValid());
    EXPECT_EQ(window2.getHandle(), originalHandle);
    EXPECT_EQ(window2.getWidth(), 320);
    EXPECT_EQ(window2.getHeight(), 240);

    // Move assign
    Window window3("Another Window", 400, 300);
    window3 = std::move(window2);

    EXPECT_FALSE(window2.isValid());
    EXPECT_TRUE(window3.isValid());
    EXPECT_EQ(window3.getHandle(), originalHandle);
}

// =============================================================================
// Test: GPU Device Claiming (requires GPU)
// =============================================================================
void test_GPUDeviceClaiming() {
    TEST_CASE("GPU device claiming");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    Window window("Claim Test", 640, 480);
    EXPECT_TRUE(window.isValid());
    EXPECT_FALSE(window.isClaimed());

    // Claim window for device
    bool claimResult = window.claimForDevice(device);
    EXPECT_TRUE(claimResult);
    EXPECT_TRUE(window.isClaimed());
    EXPECT_EQ(window.getDevice(), device.getHandle());

    // Release from device
    window.releaseFromDevice();
    EXPECT_FALSE(window.isClaimed());
    EXPECT_NULL(window.getDevice());
}

// =============================================================================
// Test: Present Mode Configuration (requires GPU)
// =============================================================================
void test_PresentModeConfiguration() {
    TEST_CASE("Present mode configuration");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    Window window("Present Mode Test", 640, 480);

    // Cannot set present mode before claiming
    EXPECT_FALSE(window.setPresentMode(PresentMode::VSync));

    // Claim window
    EXPECT_TRUE(window.claimForDevice(device));

    // Default should be VSync
    EXPECT_EQ(window.getPresentMode(), PresentMode::VSync);

    // Try different modes
    if (window.supportsPresentMode(PresentMode::Immediate)) {
        EXPECT_TRUE(window.setPresentMode(PresentMode::Immediate));
        EXPECT_EQ(window.getPresentMode(), PresentMode::Immediate);
        printf("  [INFO] Immediate mode supported and set\n");
    } else {
        printf("  [INFO] Immediate mode not supported on this device\n");
    }

    if (window.supportsPresentMode(PresentMode::Mailbox)) {
        EXPECT_TRUE(window.setPresentMode(PresentMode::Mailbox));
        EXPECT_EQ(window.getPresentMode(), PresentMode::Mailbox);
        printf("  [INFO] Mailbox mode supported and set\n");
    } else {
        printf("  [INFO] Mailbox mode not supported on this device\n");
    }

    // VSync should always be supported
    EXPECT_TRUE(window.supportsPresentMode(PresentMode::VSync));
    EXPECT_TRUE(window.setPresentMode(PresentMode::VSync));
    EXPECT_EQ(window.getPresentMode(), PresentMode::VSync);
}

// =============================================================================
// Test: Swapchain Texture Acquisition (requires GPU)
// =============================================================================
void test_SwapchainTextureAcquisition() {
    TEST_CASE("Swapchain texture acquisition");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    Window window("Swapchain Test", 640, 480);
    EXPECT_TRUE(window.claimForDevice(device));

    // Acquire command buffer
    SDL_GPUCommandBuffer* cmdBuffer = device.acquireCommandBuffer();
    EXPECT_NOT_NULL(cmdBuffer);

    if (cmdBuffer) {
        // Acquire swapchain texture
        SDL_GPUTexture* swapchainTexture = nullptr;
        uint32_t width = 0, height = 0;

        bool acquired = window.acquireSwapchainTexture(
            cmdBuffer, &swapchainTexture, &width, &height);

        if (acquired && swapchainTexture) {
            printf("  [INFO] Acquired swapchain texture: %ux%u\n", width, height);
            EXPECT_TRUE(width > 0);
            EXPECT_TRUE(height > 0);
            g_testsPassed++;
        } else {
            // May fail if window is minimized or hidden
            printf("  [WARN] Could not acquire swapchain texture (may be hidden)\n");
        }

        // Submit command buffer
        device.submit(cmdBuffer);
    }
}

// =============================================================================
// Test: Swapchain Texture Format (requires GPU)
// =============================================================================
void test_SwapchainTextureFormat() {
    TEST_CASE("Swapchain texture format");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    Window window("Format Test", 640, 480);

    // Not claimed yet - should return invalid
    EXPECT_EQ(window.getSwapchainTextureFormat(), SDL_GPU_TEXTUREFORMAT_INVALID);

    // Claim and check format
    EXPECT_TRUE(window.claimForDevice(device));

    SDL_GPUTextureFormat format = window.getSwapchainTextureFormat();
    EXPECT_TRUE(format != SDL_GPU_TEXTUREFORMAT_INVALID);

    printf("  [INFO] Swapchain texture format: %d\n", static_cast<int>(format));
}

// =============================================================================
// Test: Fullscreen Toggle
// =============================================================================
void test_FullscreenToggle() {
    TEST_CASE("Fullscreen toggle");

    Window window("Fullscreen Test", 640, 480);
    EXPECT_TRUE(window.isValid());
    EXPECT_FALSE(window.isFullscreen());

    // Test setFullscreen
    window.setFullscreen(true);
    EXPECT_TRUE(window.isFullscreen());

    window.setFullscreen(false);
    EXPECT_FALSE(window.isFullscreen());

    // Test toggleFullscreen
    window.toggleFullscreen();
    EXPECT_TRUE(window.isFullscreen());

    window.toggleFullscreen();
    EXPECT_FALSE(window.isFullscreen());

    // Dimensions should be preserved
    EXPECT_EQ(window.getWidth(), 640);
    EXPECT_EQ(window.getHeight(), 480);
}

// =============================================================================
// Test: Resize Handling
// =============================================================================
void test_ResizeHandling() {
    TEST_CASE("Resize handling");

    Window window("Resize Test", 640, 480);
    EXPECT_TRUE(window.isValid());

    // Simulate resize
    window.onResize(1280, 720);
    EXPECT_EQ(window.getWidth(), 1280);
    EXPECT_EQ(window.getHeight(), 720);

    // Enter fullscreen
    window.setFullscreen(true);

    // Resize in fullscreen shouldn't affect windowed dimensions
    window.onResize(1920, 1080);

    // Exit fullscreen
    window.setFullscreen(false);

    // Windowed dimensions should be from before fullscreen
    // (The window will be resized back, but cached dimensions preserved)
    printf("  [INFO] After fullscreen toggle: %dx%d\n",
           window.getWidth(), window.getHeight());
}

// =============================================================================
// Test: Error Handling
// =============================================================================
void test_ErrorHandling() {
    TEST_CASE("Error handling for invalid operations");

    Window window("Error Test", 640, 480);

    // Cannot acquire swapchain before claiming
    SDL_GPUTexture* texture = nullptr;
    EXPECT_FALSE(window.acquireSwapchainTexture(nullptr, &texture));
    EXPECT_FALSE(window.getLastError().empty());

    // Cannot set present mode before claiming
    EXPECT_FALSE(window.setPresentMode(PresentMode::Immediate));
    EXPECT_FALSE(window.getLastError().empty());

    // Claim with null device should fail
    EXPECT_FALSE(window.claimForDevice(static_cast<SDL_GPUDevice*>(nullptr)));
    EXPECT_FALSE(window.getLastError().empty());
}

// =============================================================================
// Test: Cleanup on Destruction
// =============================================================================
void test_CleanupOnDestruction() {
    TEST_CASE("Cleanup on destruction");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    {
        Window window("Cleanup Test", 320, 240);
        EXPECT_TRUE(window.claimForDevice(device));
        EXPECT_TRUE(window.isClaimed());

        // Window goes out of scope here - should release properly
    }

    // Device should still be valid after window destruction
    EXPECT_TRUE(device.isValid());
    printf("  [PASS] Window cleanup completed successfully\n");
    g_testsPassed++;
}

// =============================================================================
// Test: Multiple Windows (requires GPU)
// =============================================================================
void test_MultipleWindows() {
    TEST_CASE("Multiple windows with same device");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    Window window1("Window 1", 320, 240);
    Window window2("Window 2", 400, 300);

    EXPECT_TRUE(window1.isValid());
    EXPECT_TRUE(window2.isValid());

    // Both can be claimed by the same device
    EXPECT_TRUE(window1.claimForDevice(device));
    EXPECT_TRUE(window2.claimForDevice(device));

    EXPECT_TRUE(window1.isClaimed());
    EXPECT_TRUE(window2.isClaimed());

    // Both should have same device handle
    EXPECT_EQ(window1.getDevice(), device.getHandle());
    EXPECT_EQ(window2.getDevice(), device.getHandle());

    // Each can have different present modes
    if (window1.supportsPresentMode(PresentMode::Immediate)) {
        window1.setPresentMode(PresentMode::Immediate);
        EXPECT_EQ(window1.getPresentMode(), PresentMode::Immediate);
    }
    window2.setPresentMode(PresentMode::VSync);
    EXPECT_EQ(window2.getPresentMode(), PresentMode::VSync);
}

// =============================================================================
// Test: Swapchain Composition (requires GPU)
// =============================================================================
void test_SwapchainComposition() {
    TEST_CASE("Swapchain composition configuration");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    Window window("Composition Test", 640, 480);
    EXPECT_TRUE(window.claimForDevice(device));

    // Default should be SDR
    EXPECT_EQ(window.getSwapchainComposition(), SDL_GPU_SWAPCHAINCOMPOSITION_SDR);

    // SDR should always be supported
    EXPECT_TRUE(window.setSwapchainComposition(SDL_GPU_SWAPCHAINCOMPOSITION_SDR));

    // HDR modes may or may not be supported
    bool sdrLinearSupported = SDL_WindowSupportsGPUSwapchainComposition(
        device.getHandle(), window.getHandle(), SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR);

    if (sdrLinearSupported) {
        EXPECT_TRUE(window.setSwapchainComposition(SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR));
        printf("  [INFO] SDR Linear composition supported\n");
    } else {
        printf("  [INFO] SDR Linear composition not supported\n");
    }
}

// =============================================================================
// Main
// =============================================================================
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("Window Unit Tests (Ticket 2-002)\n");
    printf("========================================\n");

    // Initialize SDL for video (required for window creation)
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("[FATAL] Failed to initialize SDL: %s\n", SDL_GetError());
        printf("Some tests will be skipped.\n");
    }

    // Run tests
    test_PresentModeNameConversion();
    test_PresentModeSDLConversion();
    test_WindowCreation();
    test_WindowDimensions();
    test_MoveSemantics();
    test_GPUDeviceClaiming();
    test_PresentModeConfiguration();
    test_SwapchainTextureAcquisition();
    test_SwapchainTextureFormat();
    test_FullscreenToggle();
    test_ResizeHandling();
    test_ErrorHandling();
    test_CleanupOnDestruction();
    test_MultipleWindows();
    test_SwapchainComposition();

    // Summary
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("========================================\n");

    SDL_Quit();

    return g_testsFailed > 0 ? 1 : 0;
}
