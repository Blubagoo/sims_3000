/**
 * @file test_gpu_resources.cpp
 * @brief Unit tests for GPU resource management infrastructure.
 *
 * Tests UniformBufferPool, SamplerCache, and FrameResources.
 * Note: Some tests require GPU hardware and may be skipped on headless systems.
 */

#include "sims3000/render/UniformBufferPool.h"
#include "sims3000/render/SamplerCache.h"
#include "sims3000/render/FrameResources.h"
#include "sims3000/render/GPUDevice.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

#define EXPECT_GT(a, b) \
    do { \
        if ((a) > (b)) { \
            g_testsPassed++; \
            printf("  [PASS] %s > %s\n", #a, #b); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] %s <= %s (line %d)\n", #a, #b, __LINE__); \
        } \
    } while(0)

// =============================================================================
// UniformBufferPool Tests
// =============================================================================

void test_UniformBufferPool_NullDevice() {
    TEST_CASE("UniformBufferPool with null device");

    UniformBufferPool pool(nullptr);
    EXPECT_FALSE(pool.isValid());

    auto alloc = pool.allocate(256);
    EXPECT_FALSE(alloc.isValid());
}

void test_UniformBufferPool_BasicAllocation() {
    TEST_CASE("UniformBufferPool basic allocation");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    UniformBufferPool pool(device.getHandle());
    EXPECT_TRUE(pool.isValid());

    // Allocate some uniform data
    auto alloc1 = pool.allocate(64);
    EXPECT_TRUE(alloc1.isValid());
    EXPECT_NOT_NULL(alloc1.buffer);
    EXPECT_EQ(alloc1.offset, 0u);
    EXPECT_EQ(alloc1.size, 64u);

    // Allocate more - should be aligned
    auto alloc2 = pool.allocate(128);
    EXPECT_TRUE(alloc2.isValid());
    EXPECT_NOT_NULL(alloc2.buffer);
    EXPECT_GT(alloc2.offset, 0u);  // Should be offset from first
    EXPECT_EQ(alloc2.size, 128u);

    // Check stats
    auto stats = pool.getStats();
    EXPECT_EQ(stats.allocationCount, 2u);
    EXPECT_GT(stats.totalBytesAllocated, 0u);
    EXPECT_EQ(stats.blockCount, 1u);  // Should fit in one block
}

void test_UniformBufferPool_Reset() {
    TEST_CASE("UniformBufferPool reset");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    UniformBufferPool pool(device.getHandle());

    // Allocate and then reset
    pool.allocate(1024);
    pool.allocate(2048);

    auto statsBefore = pool.getStats();
    EXPECT_EQ(statsBefore.allocationCount, 2u);

    pool.reset();

    auto statsAfter = pool.getStats();
    EXPECT_EQ(statsAfter.allocationCount, 0u);
    EXPECT_EQ(statsAfter.totalBytesAllocated, 0u);

    // Should be able to allocate again from beginning
    auto alloc = pool.allocate(64);
    EXPECT_TRUE(alloc.isValid());
    EXPECT_EQ(alloc.offset, 0u);  // Reset should start from 0
}

void test_UniformBufferPool_LargeAllocation() {
    TEST_CASE("UniformBufferPool large allocation (exceeds block size)");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    // Create pool with small block size
    UniformBufferPool pool(device.getHandle(), 4096);

    // Try to allocate more than block size
    auto alloc = pool.allocate(8192);
    EXPECT_FALSE(alloc.isValid());  // Should fail

    // Error message should be set
    EXPECT_FALSE(pool.getLastError().empty());
    printf("  [INFO] Expected error: %s\n", pool.getLastError().c_str());
}

void test_UniformBufferPool_MultipleBlocks() {
    TEST_CASE("UniformBufferPool multiple blocks");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    // Create pool with small block size to force multiple blocks
    UniformBufferPool pool(device.getHandle(), 4096);

    // Allocate more than one block worth
    for (int i = 0; i < 20; ++i) {
        auto alloc = pool.allocate(256);
        if (!alloc.isValid()) {
            printf("  [FAIL] Allocation %d failed\n", i);
            g_testsFailed++;
            return;
        }
    }

    auto stats = pool.getStats();
    EXPECT_GT(stats.blockCount, 1u);  // Should have multiple blocks
    EXPECT_EQ(stats.allocationCount, 20u);

    printf("  [INFO] Created %u blocks for 20 allocations\n", stats.blockCount);
}

// =============================================================================
// SamplerCache Tests
// =============================================================================

void test_SamplerCache_NullDevice() {
    TEST_CASE("SamplerCache with null device");

    SamplerCache cache(nullptr);
    EXPECT_FALSE(cache.isValid());

    auto sampler = cache.getLinear();
    EXPECT_NULL(sampler);
}

void test_SamplerCache_LinearSampler() {
    TEST_CASE("SamplerCache linear sampler");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    SamplerCache cache(device.getHandle());
    EXPECT_TRUE(cache.isValid());

    auto linear = cache.getLinear();
    EXPECT_NOT_NULL(linear);

    // Getting same sampler should return cached version
    auto linear2 = cache.getLinear();
    EXPECT_EQ(linear, linear2);

    EXPECT_EQ(cache.size(), 1u);
}

void test_SamplerCache_NearestSampler() {
    TEST_CASE("SamplerCache nearest sampler");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    SamplerCache cache(device.getHandle());

    auto nearest = cache.getNearest();
    EXPECT_NOT_NULL(nearest);

    // Should be different from linear
    auto linear = cache.getLinear();
    EXPECT_TRUE(nearest != linear);

    EXPECT_EQ(cache.size(), 2u);
}

void test_SamplerCache_ClampSamplers() {
    TEST_CASE("SamplerCache clamp samplers");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    SamplerCache cache(device.getHandle());

    auto linearClamp = cache.getLinearClamp();
    EXPECT_NOT_NULL(linearClamp);

    auto nearestClamp = cache.getNearestClamp();
    EXPECT_NOT_NULL(nearestClamp);

    EXPECT_TRUE(linearClamp != nearestClamp);
}

void test_SamplerCache_AnisotropicSampler() {
    TEST_CASE("SamplerCache anisotropic sampler");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    SamplerCache cache(device.getHandle());

    auto aniso4 = cache.getAnisotropic(4.0f);
    EXPECT_NOT_NULL(aniso4);

    // Different anisotropy levels should create different samplers
    auto aniso8 = cache.getAnisotropic(8.0f);
    EXPECT_NOT_NULL(aniso8);
    EXPECT_TRUE(aniso4 != aniso8);
}

void test_SamplerCache_CustomConfig() {
    TEST_CASE("SamplerCache custom configuration");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    SamplerCache cache(device.getHandle());

    SamplerConfig config;
    config.minFilter = SamplerFilter::Linear;
    config.magFilter = SamplerFilter::Nearest;  // Mixed filtering
    config.addressModeU = SamplerAddressMode::ClampToEdge;
    config.addressModeV = SamplerAddressMode::MirroredRepeat;

    auto custom = cache.getSampler(config);
    EXPECT_NOT_NULL(custom);

    // Same config should return cached sampler
    auto custom2 = cache.getSampler(config);
    EXPECT_EQ(custom, custom2);
}

void test_SamplerCache_Clear() {
    TEST_CASE("SamplerCache clear");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    SamplerCache cache(device.getHandle());

    cache.getLinear();
    cache.getNearest();
    EXPECT_EQ(cache.size(), 2u);

    cache.clear();
    EXPECT_EQ(cache.size(), 0u);

    // Should be able to create new samplers after clear
    auto linear = cache.getLinear();
    EXPECT_NOT_NULL(linear);
    EXPECT_EQ(cache.size(), 1u);
}

// =============================================================================
// FrameResources Tests
// =============================================================================

void test_FrameResources_NullDevice() {
    TEST_CASE("FrameResources with null device");

    FrameResources frames(nullptr);
    EXPECT_FALSE(frames.isValid());
}

void test_FrameResources_DefaultConfig() {
    TEST_CASE("FrameResources default configuration");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    FrameResources frames(device.getHandle());
    EXPECT_TRUE(frames.isValid());

    auto stats = frames.getStats();
    EXPECT_EQ(stats.frameCount, 2u);  // Default double buffering
}

void test_FrameResources_TripleBuffering() {
    TEST_CASE("FrameResources triple buffering");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    FrameResourcesConfig config;
    config.frameCount = 3;

    FrameResources frames(device.getHandle(), config);
    EXPECT_TRUE(frames.isValid());

    auto stats = frames.getStats();
    EXPECT_EQ(stats.frameCount, 3u);
}

void test_FrameResources_FrameCycle() {
    TEST_CASE("FrameResources frame cycle");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    FrameResources frames(device.getHandle());

    // Simulate a few frames
    for (int i = 0; i < 5; ++i) {
        frames.beginFrame();

        auto stats = frames.getStats();
        EXPECT_EQ(stats.totalFramesRendered, static_cast<std::uint64_t>(i + 1));

        frames.endFrame();
    }

    auto finalStats = frames.getStats();
    EXPECT_EQ(finalStats.totalFramesRendered, 5u);
}

void test_FrameResources_UniformAllocation() {
    TEST_CASE("FrameResources uniform data allocation");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    FrameResources frames(device.getHandle());

    frames.beginFrame();

    void* ptr = nullptr;
    std::uint32_t offset = 0;
    bool success = frames.allocateUniformData(64, &ptr, &offset);

    EXPECT_TRUE(success);
    EXPECT_NOT_NULL(ptr);
    EXPECT_EQ(offset, 0u);

    // Write some data
    std::memset(ptr, 0xAB, 64);

    // Second allocation
    void* ptr2 = nullptr;
    std::uint32_t offset2 = 0;
    success = frames.allocateUniformData(128, &ptr2, &offset2);

    EXPECT_TRUE(success);
    EXPECT_NOT_NULL(ptr2);
    EXPECT_GT(offset2, 0u);

    frames.endFrame();

    auto stats = frames.getStats();
    EXPECT_GT(stats.uniformBytesUsed, 0u);
}

void test_FrameResources_TextureAllocation() {
    TEST_CASE("FrameResources texture data allocation");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    FrameResources frames(device.getHandle());

    frames.beginFrame();

    void* ptr = nullptr;
    std::uint32_t offset = 0;
    bool success = frames.allocateTextureData(1024, &ptr, &offset);

    EXPECT_TRUE(success);
    EXPECT_NOT_NULL(ptr);

    frames.endFrame();

    auto stats = frames.getStats();
    EXPECT_GT(stats.textureBytesUsed, 0u);
}

void test_FrameResources_AllocationNotInFrame() {
    TEST_CASE("FrameResources allocation without beginFrame");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    FrameResources frames(device.getHandle());

    // Don't call beginFrame
    void* ptr = nullptr;
    std::uint32_t offset = 0;
    bool success = frames.allocateUniformData(64, &ptr, &offset);

    EXPECT_FALSE(success);  // Should fail
    EXPECT_NULL(ptr);
}

void test_FrameResources_TransferBufferAccess() {
    TEST_CASE("FrameResources transfer buffer access");

    GPUDevice device;
    if (!device.isValid()) {
        printf("  [SKIP] No GPU available\n");
        return;
    }

    FrameResources frames(device.getHandle());

    frames.beginFrame();

    auto uniformBuffer = frames.getUniformTransferBuffer();
    EXPECT_NOT_NULL(uniformBuffer);

    auto textureBuffer = frames.getTextureTransferBuffer();
    EXPECT_NOT_NULL(textureBuffer);

    frames.endFrame();
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("GPU Resources Unit Tests\n");
    printf("========================================\n");

    // Initialize SDL for video (required for GPU device)
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("[WARN] Failed to initialize SDL: %s\n", SDL_GetError());
        printf("Some tests will be skipped.\n");
    }

    // UniformBufferPool tests
    test_UniformBufferPool_NullDevice();
    test_UniformBufferPool_BasicAllocation();
    test_UniformBufferPool_Reset();
    test_UniformBufferPool_LargeAllocation();
    test_UniformBufferPool_MultipleBlocks();

    // SamplerCache tests
    test_SamplerCache_NullDevice();
    test_SamplerCache_LinearSampler();
    test_SamplerCache_NearestSampler();
    test_SamplerCache_ClampSamplers();
    test_SamplerCache_AnisotropicSampler();
    test_SamplerCache_CustomConfig();
    test_SamplerCache_Clear();

    // FrameResources tests
    test_FrameResources_NullDevice();
    test_FrameResources_DefaultConfig();
    test_FrameResources_TripleBuffering();
    test_FrameResources_FrameCycle();
    test_FrameResources_UniformAllocation();
    test_FrameResources_TextureAllocation();
    test_FrameResources_AllocationNotInFrame();
    test_FrameResources_TransferBufferAccess();

    // Summary
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("========================================\n");

    SDL_Quit();

    return g_testsFailed > 0 ? 1 : 0;
}
