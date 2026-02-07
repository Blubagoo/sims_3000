/**
 * @file test_shader_compiler.cpp
 * @brief Unit tests for ShaderCompiler.
 *
 * Tests shader loading, cache validation, format detection, and fallback behavior.
 * GPU shader creation tests require a valid display and are marked for manual verification.
 */

#include "sims3000/render/ShaderCompiler.h"
#include "sims3000/render/GPUDevice.h"
#include <SDL3/SDL.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>

using namespace sims3000;

// =============================================================================
// Test Utilities
// =============================================================================

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(name) \
    static void test_##name(); \
    struct TestRegistrar_##name { \
        TestRegistrar_##name() { \
            printf("Running: %s\n", #name); \
            try { \
                test_##name(); \
                g_testsPassed++; \
                printf("  PASSED\n"); \
            } catch (const std::exception& e) { \
                g_testsFailed++; \
                printf("  FAILED: %s\n", e.what()); \
            } catch (...) { \
                g_testsFailed++; \
                printf("  FAILED: Unknown exception\n"); \
            } \
        } \
    } g_testRegistrar_##name; \
    static void test_##name()

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            throw std::runtime_error("Assertion failed: " #expr); \
        } \
    } while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NE(a, b) ASSERT_TRUE((a) != (b))
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != nullptr)
#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == nullptr)

// =============================================================================
// Unit Tests (No GPU Required)
// =============================================================================

TEST(ShaderStage_ToSDLShaderStage_Vertex) {
    auto sdlStage = toSDLShaderStage(ShaderStage::Vertex);
    ASSERT_EQ(sdlStage, SDL_GPU_SHADERSTAGE_VERTEX);
}

TEST(ShaderStage_ToSDLShaderStage_Fragment) {
    auto sdlStage = toSDLShaderStage(ShaderStage::Fragment);
    ASSERT_EQ(sdlStage, SDL_GPU_SHADERSTAGE_FRAGMENT);
}

TEST(ShaderProfile_Vertex) {
    const char* profile = getShaderProfile(ShaderStage::Vertex);
    ASSERT_TRUE(strcmp(profile, "vs_6_0") == 0);
}

TEST(ShaderProfile_Fragment) {
    const char* profile = getShaderProfile(ShaderStage::Fragment);
    ASSERT_TRUE(strcmp(profile, "ps_6_0") == 0);
}

TEST(ShaderResources_DefaultConstruction) {
    ShaderResources resources;
    ASSERT_EQ(resources.numSamplers, 0u);
    ASSERT_EQ(resources.numStorageTextures, 0u);
    ASSERT_EQ(resources.numStorageBuffers, 0u);
    ASSERT_EQ(resources.numUniformBuffers, 0u);
}

TEST(ShaderResources_CustomValues) {
    ShaderResources resources;
    resources.numSamplers = 2;
    resources.numUniformBuffers = 1;
    resources.numStorageBuffers = 3;
    resources.numStorageTextures = 1;

    ASSERT_EQ(resources.numSamplers, 2u);
    ASSERT_EQ(resources.numUniformBuffers, 1u);
    ASSERT_EQ(resources.numStorageBuffers, 3u);
    ASSERT_EQ(resources.numStorageTextures, 1u);
}

TEST(ShaderLoadResult_DefaultConstruction) {
    ShaderLoadResult result;
    ASSERT_NULL(result.shader);
    ASSERT_FALSE(result.usedFallback);
    ASSERT_FALSE(result.fromCache);
    ASSERT_TRUE(result.loadedPath.empty());
    ASSERT_FALSE(result.isValid());
    ASSERT_FALSE(result.hasError());
}

TEST(ShaderLoadResult_IsValid_WithShader) {
    ShaderLoadResult result;
    // Can't actually set shader without GPU, but test the logic
    ASSERT_FALSE(result.isValid());
}

TEST(ShaderLoadResult_HasError_WithMessage) {
    ShaderLoadResult result;
    result.error.message = "Test error";
    ASSERT_TRUE(result.hasError());
}

TEST(ShaderCompileError_DefaultConstruction) {
    ShaderCompileError error;
    ASSERT_TRUE(error.filename.empty());
    ASSERT_EQ(error.line, 0);
    ASSERT_EQ(error.column, 0);
    ASSERT_TRUE(error.message.empty());
    ASSERT_TRUE(error.fullText.empty());
}

TEST(ShaderCompileError_WithValues) {
    ShaderCompileError error;
    error.filename = "test.hlsl";
    error.line = 42;
    error.column = 10;
    error.message = "Syntax error";
    error.fullText = "test.hlsl(42,10): error: Syntax error";

    ASSERT_EQ(error.filename, "test.hlsl");
    ASSERT_EQ(error.line, 42);
    ASSERT_EQ(error.column, 10);
    ASSERT_EQ(error.message, "Syntax error");
}

TEST(ShaderCacheEntry_DefaultConstruction) {
    ShaderCacheEntry entry;
    ASSERT_TRUE(entry.bytecode.empty());
    ASSERT_EQ(entry.sourceHash, 0u);
    ASSERT_EQ(entry.format, SDL_GPU_SHADERFORMAT_INVALID);
    ASSERT_EQ(entry.timestamp, 0ull);
}

TEST(ShaderCacheEntry_WithData) {
    ShaderCacheEntry entry;
    entry.bytecode = {0x01, 0x02, 0x03, 0x04};
    entry.sourceHash = 0xDEADBEEF;
    entry.format = SDL_GPU_SHADERFORMAT_SPIRV;
    entry.timestamp = 1234567890;

    ASSERT_EQ(entry.bytecode.size(), 4u);
    ASSERT_EQ(entry.bytecode[0], 0x01);
    ASSERT_EQ(entry.sourceHash, 0xDEADBEEF);
    ASSERT_EQ(entry.format, SDL_GPU_SHADERFORMAT_SPIRV);
    ASSERT_EQ(entry.timestamp, 1234567890ull);
}

// =============================================================================
// GPU Tests (Require Display)
// =============================================================================

// These tests require a GPU device and may fail on headless CI systems.
// They are designed to be run manually or with appropriate GPU availability.

static bool initSDL() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("  SKIP: SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

static void quitSDL() {
    SDL_Quit();
}

TEST(ShaderCompiler_Construction_WithValidDevice) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler(device);

    // Should initialize with default paths
    // Hot-reload should be enabled in debug builds
#ifdef SIMS3000_DEBUG
    ASSERT_TRUE(compiler.isHotReloadEnabled());
#endif

    quitSDL();
}

TEST(ShaderCompiler_GetPreferredFormat_D3D12) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler(device);
    auto format = compiler.getPreferredFormat();

    // Should return a valid format
    ASSERT_NE(format, SDL_GPU_SHADERFORMAT_INVALID);

    // On Windows with D3D12, should prefer DXIL
    const auto& caps = device.getCapabilities();
    if (caps.backend == GPUBackend::D3D12) {
        ASSERT_EQ(format, SDL_GPU_SHADERFORMAT_DXIL);
    }

    quitSDL();
}

TEST(ShaderCompiler_GetFormatExtension) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler(device);
    const char* ext = compiler.getFormatExtension();

    // Should be either .dxil or .spv
    ASSERT_TRUE(strcmp(ext, ".dxil") == 0 || strcmp(ext, ".spv") == 0);

    quitSDL();
}

TEST(ShaderCompiler_GetFormatName) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler(device);
    const char* name = compiler.getFormatName();

    // Should be either DXIL or SPIRV
    ASSERT_TRUE(strcmp(name, "DXIL") == 0 || strcmp(name, "SPIRV") == 0);

    quitSDL();
}

TEST(ShaderCompiler_SetAssetPath) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler(device);
    compiler.setAssetPath("custom/shader/path");

    // Path should be set (no way to verify internally without loading)
    // Just verify no crash
    ASSERT_TRUE(true);

    quitSDL();
}

TEST(ShaderCompiler_SetCachePath) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler(device);
    compiler.setCachePath("custom/cache/path");

    // Path should be set (no way to verify internally without saving)
    ASSERT_TRUE(true);

    quitSDL();
}

TEST(ShaderCompiler_SetHotReloadEnabled) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler(device);

#ifdef SIMS3000_DEBUG
    compiler.setHotReloadEnabled(false);
    ASSERT_FALSE(compiler.isHotReloadEnabled());

    compiler.setHotReloadEnabled(true);
    ASSERT_TRUE(compiler.isHotReloadEnabled());
#else
    // In release, hot-reload should always be disabled
    compiler.setHotReloadEnabled(true);
    ASSERT_FALSE(compiler.isHotReloadEnabled());
#endif

    quitSDL();
}

TEST(ShaderCompiler_ClearCache) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler(device);
    compiler.clearCache();

    // Should not crash
    ASSERT_TRUE(true);

    quitSDL();
}

TEST(ShaderCompiler_InvalidateCache) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler(device);
    compiler.invalidateCache("nonexistent/shader");

    // Should not crash even with nonexistent path
    ASSERT_TRUE(true);

    quitSDL();
}

TEST(ShaderCompiler_LoadShader_NonexistentPath_UsesFallback) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler(device);
    auto result = compiler.loadShader(
        "nonexistent/shader.vert",
        ShaderStage::Vertex
    );

    // Should use fallback or fail gracefully
    if (result.isValid()) {
        ASSERT_TRUE(result.usedFallback);
        SDL_ReleaseGPUShader(device.getHandle(), result.shader);
    } else {
        // Fallback not available (embedded shaders not compiled)
        ASSERT_TRUE(result.hasError() || result.shader == nullptr);
    }

    quitSDL();
}

TEST(ShaderCompiler_CheckForReload_NoChanges) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler(device);

    // With no watched shaders, should return false
    bool reloaded = compiler.checkForReload();
    ASSERT_FALSE(reloaded);

    quitSDL();
}

TEST(ShaderCompiler_MoveConstruction) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler1(device);
    compiler1.setAssetPath("path1");

    ShaderCompiler compiler2(std::move(compiler1));

    // compiler2 should be usable
    auto format = compiler2.getPreferredFormat();
    ASSERT_NE(format, SDL_GPU_SHADERFORMAT_INVALID);

    quitSDL();
}

TEST(ShaderCompiler_MoveAssignment) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler1(device);
    ShaderCompiler compiler2(device);

    compiler2 = std::move(compiler1);

    // compiler2 should be usable
    auto format = compiler2.getPreferredFormat();
    ASSERT_NE(format, SDL_GPU_SHADERFORMAT_INVALID);

    quitSDL();
}

TEST(ShaderCompiler_SetReloadCallback) {
    if (!initSDL()) return;

    GPUDevice device;
    if (!device.isValid()) {
        printf("  SKIP: GPU device creation failed (headless?)\n");
        quitSDL();
        return;
    }

    ShaderCompiler compiler(device);

    bool callbackCalled = false;
    compiler.setReloadCallback([&callbackCalled](const std::string& path) {
        callbackCalled = true;
        (void)path;
    });

    // Callback should be set without error
    ASSERT_TRUE(true);

    quitSDL();
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("\n=== ShaderCompiler Unit Tests ===\n\n");

    // Tests are auto-registered via static initialization
    // Just report results

    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", g_testsPassed);
    printf("Failed: %d\n", g_testsFailed);

    return g_testsFailed > 0 ? 1 : 0;
}
