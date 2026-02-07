/**
 * @file test_gpu_device.cpp
 * @brief Unit tests for GPUDevice wrapper class.
 *
 * Tests GPU device creation, capability detection, error handling,
 * and basic operations. Note: Some tests require a display/GPU.
 */

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
// Test: Backend Name Conversion
// =============================================================================
void test_BackendNameConversion() {
    TEST_CASE("Backend name conversion");

    EXPECT_EQ(std::string(getBackendName(GPUBackend::D3D12)), std::string("Direct3D 12"));
    EXPECT_EQ(std::string(getBackendName(GPUBackend::Vulkan)), std::string("Vulkan"));
    EXPECT_EQ(std::string(getBackendName(GPUBackend::Metal)), std::string("Metal"));
    EXPECT_EQ(std::string(getBackendName(GPUBackend::Unknown)), std::string("Unknown"));
}

// =============================================================================
// Test: Default Constructor (requires GPU)
// =============================================================================
void test_DefaultConstructor() {
    TEST_CASE("Default constructor creates device");

    GPUDevice device;

    // Device may or may not be valid depending on hardware
    // If valid, capabilities should be populated
    if (device.isValid()) {
        EXPECT_NOT_NULL(device.getHandle());
        const auto& caps = device.getCapabilities();
        EXPECT_FALSE(caps.backendName.empty());

        // At least one shader format should be supported
        bool hasShaderFormat = caps.supportsSpirV || caps.supportsDXIL ||
                               caps.supportsDXBC || caps.supportsMetalLib;
        EXPECT_TRUE(hasShaderFormat);

        printf("  [INFO] Created device with backend: %s\n",
               caps.backendName.c_str());
        printf("  [INFO] Debug layers: %s\n",
               caps.debugLayersEnabled ? "enabled" : "disabled");
    } else {
        printf("  [SKIP] No GPU available - device creation failed\n");
        printf("  [INFO] Last error: %s\n", device.getLastError().c_str());
        // Not a test failure - just no GPU available
    }
}

// =============================================================================
// Test: Explicit Debug Mode Constructor
// =============================================================================
void test_ExplicitDebugMode() {
    TEST_CASE("Explicit debug mode constructor");

    // Create with debug layers disabled
    GPUDevice deviceNoDebug(false);

    if (deviceNoDebug.isValid()) {
        EXPECT_FALSE(deviceNoDebug.getCapabilities().debugLayersEnabled);
        printf("  [INFO] Device created without debug layers\n");
    } else {
        printf("  [SKIP] No GPU available\n");
    }
}

// =============================================================================
// Test: Move Semantics
// =============================================================================
void test_MoveSemantics() {
    TEST_CASE("Move constructor and assignment");

    GPUDevice device1;

    if (device1.isValid()) {
        SDL_GPUDevice* originalHandle = device1.getHandle();
        std::string originalBackend = device1.getCapabilities().backendName;

        // Move construct
        GPUDevice device2(std::move(device1));

        EXPECT_FALSE(device1.isValid());  // Moved-from should be invalid
        EXPECT_NULL(device1.getHandle());
        EXPECT_TRUE(device2.isValid());
        EXPECT_EQ(device2.getHandle(), originalHandle);
        EXPECT_EQ(device2.getCapabilities().backendName, originalBackend);

        // Move assign
        GPUDevice device3;
        if (device3.isValid()) {
            device3 = std::move(device2);

            EXPECT_FALSE(device2.isValid());
            EXPECT_TRUE(device3.isValid());
            EXPECT_EQ(device3.getHandle(), originalHandle);
        }
    } else {
        printf("  [SKIP] No GPU available\n");
    }
}

// =============================================================================
// Test: Capability Detection
// =============================================================================
void test_CapabilityDetection() {
    TEST_CASE("Capability detection");

    GPUDevice device;

    if (device.isValid()) {
        const auto& caps = device.getCapabilities();

        // Backend should be known on modern systems
        bool knownBackend = (caps.backend == GPUBackend::D3D12 ||
                             caps.backend == GPUBackend::Vulkan ||
                             caps.backend == GPUBackend::Metal);
        // Unknown is also acceptable on unusual systems
        EXPECT_TRUE(knownBackend || caps.backend == GPUBackend::Unknown);

        // Driver info should not be empty
        EXPECT_FALSE(caps.driverInfo.empty());

        // Test supportsShaderFormat() method
        if (caps.supportsSpirV) {
            EXPECT_TRUE(device.supportsShaderFormat(SDL_GPU_SHADERFORMAT_SPIRV));
        }
        if (caps.supportsDXIL) {
            EXPECT_TRUE(device.supportsShaderFormat(SDL_GPU_SHADERFORMAT_DXIL));
        }

        device.logCapabilities();
    } else {
        printf("  [SKIP] No GPU available\n");
    }
}

// =============================================================================
// Test: Command Buffer Operations
// =============================================================================
void test_CommandBufferOperations() {
    TEST_CASE("Command buffer acquire and submit");

    GPUDevice device;

    if (device.isValid()) {
        // Acquire command buffer
        SDL_GPUCommandBuffer* cmdBuffer = device.acquireCommandBuffer();
        EXPECT_NOT_NULL(cmdBuffer);

        if (cmdBuffer) {
            // Submit empty command buffer (valid operation)
            bool submitResult = device.submit(cmdBuffer);
            EXPECT_TRUE(submitResult);
        }

        // Acquire and submit multiple command buffers
        for (int i = 0; i < 3; ++i) {
            SDL_GPUCommandBuffer* cmd = device.acquireCommandBuffer();
            if (cmd) {
                device.submit(cmd);
            }
        }
    } else {
        printf("  [SKIP] No GPU available\n");
    }
}

// =============================================================================
// Test: Error Handling for Invalid Operations
// =============================================================================
void test_ErrorHandling() {
    TEST_CASE("Error handling for invalid operations");

    // Test with invalid device (moved-from state)
    GPUDevice device1;
    GPUDevice device2(std::move(device1));

    // Moved-from device should fail gracefully
    EXPECT_FALSE(device1.isValid());
    EXPECT_NULL(device1.acquireCommandBuffer());
    EXPECT_FALSE(device1.claimWindow(nullptr));
    EXPECT_FALSE(device1.submit(nullptr));

    // Submit with null command buffer should fail
    if (device2.isValid()) {
        EXPECT_FALSE(device2.submit(nullptr));
        EXPECT_FALSE(device2.getLastError().empty());
    }
}

// =============================================================================
// Test: Window Claim (requires window)
// =============================================================================
void test_WindowClaim() {
    TEST_CASE("Window claiming for GPU rendering");

    GPUDevice device;

    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    // Create a test window
    SDL_Window* window = SDL_CreateWindow(
        "GPUDevice Test Window",
        320, 240,
        SDL_WINDOW_HIDDEN  // Hidden window for testing
    );

    if (!window) {
        printf("  [SKIP] Could not create window: %s\n", SDL_GetError());
        return;
    }

    // Claim window
    bool claimResult = device.claimWindow(window);
    EXPECT_TRUE(claimResult);

    if (claimResult) {
        // Acquire command buffer after claiming window
        SDL_GPUCommandBuffer* cmdBuffer = device.acquireCommandBuffer();
        EXPECT_NOT_NULL(cmdBuffer);

        if (cmdBuffer) {
            // Can acquire swapchain texture
            SDL_GPUTexture* swapchainTexture = nullptr;
            Uint32 w, h;
            bool acquired = SDL_WaitAndAcquireGPUSwapchainTexture(
                cmdBuffer, window, &swapchainTexture, &w, &h);

            if (acquired && swapchainTexture) {
                printf("  [INFO] Acquired swapchain texture %ux%u\n", w, h);
            }

            device.submit(cmdBuffer);
        }

        // Release window
        device.releaseWindow(window);
    }

    SDL_DestroyWindow(window);
}

// =============================================================================
// Test: Wait for Idle
// =============================================================================
void test_WaitForIdle() {
    TEST_CASE("Wait for GPU idle");

    GPUDevice device;

    if (device.isValid()) {
        // Submit some work
        for (int i = 0; i < 5; ++i) {
            SDL_GPUCommandBuffer* cmd = device.acquireCommandBuffer();
            if (cmd) {
                device.submit(cmd);
            }
        }

        // Wait for all work to complete
        device.waitForIdle();
        printf("  [PASS] waitForIdle() completed\n");
        g_testsPassed++;
    } else {
        printf("  [SKIP] No GPU available\n");
    }
}

// =============================================================================
// Test: Shader Format Support Query
// =============================================================================
void test_ShaderFormatSupport() {
    TEST_CASE("Shader format support query");

    GPUDevice device;

    if (device.isValid()) {
        const auto& caps = device.getCapabilities();

        printf("  [INFO] SPIR-V supported: %s\n", caps.supportsSpirV ? "yes" : "no");
        printf("  [INFO] DXIL supported: %s\n", caps.supportsDXIL ? "yes" : "no");
        printf("  [INFO] DXBC supported: %s\n", caps.supportsDXBC ? "yes" : "no");
        printf("  [INFO] MetalLib supported: %s\n", caps.supportsMetalLib ? "yes" : "no");

        // Verify consistency between capability struct and method
        EXPECT_EQ(caps.supportsSpirV,
                  device.supportsShaderFormat(SDL_GPU_SHADERFORMAT_SPIRV));
        EXPECT_EQ(caps.supportsDXIL,
                  device.supportsShaderFormat(SDL_GPU_SHADERFORMAT_DXIL));
    } else {
        printf("  [SKIP] No GPU available\n");
    }
}

// =============================================================================
// Main
// =============================================================================
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("GPUDevice Unit Tests\n");
    printf("========================================\n");

    // Initialize SDL for video (required for GPU device)
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("[FATAL] Failed to initialize SDL: %s\n", SDL_GetError());
        printf("Some tests will be skipped.\n");
        // Continue - tests will handle missing GPU gracefully
    }

    // Run tests
    test_BackendNameConversion();
    test_DefaultConstructor();
    test_ExplicitDebugMode();
    test_MoveSemantics();
    test_CapabilityDetection();
    test_CommandBufferOperations();
    test_ErrorHandling();
    test_WindowClaim();
    test_WaitForIdle();
    test_ShaderFormatSupport();

    // Summary
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("========================================\n");

    SDL_Quit();

    return g_testsFailed > 0 ? 1 : 0;
}
