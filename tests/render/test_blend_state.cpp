/**
 * @file test_blend_state.cpp
 * @brief Unit tests for BlendState configuration class.
 *
 * Tests blend state factory methods for opaque and transparent passes,
 * verifying correct blend enable, blend factors, and blend operations.
 * These tests do NOT require GPU hardware as they only test state configuration.
 */

#include "sims3000/render/BlendState.h"
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

#define EXPECT_STR_CONTAINS(str, substr) \
    do { \
        if (strstr((str), (substr)) != nullptr) { \
            g_testsPassed++; \
            printf("  [PASS] \"%s\" contains \"%s\"\n", str, substr); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] \"%s\" does not contain \"%s\" (line %d)\n", str, substr, __LINE__); \
        } \
    } while(0)

// =============================================================================
// Test: Opaque Blend State Configuration
// =============================================================================
void test_OpaqueBlendState() {
    TEST_CASE("Opaque blend state configuration");

    SDL_GPUColorTargetBlendState state = BlendState::opaque();

    // Blend must be disabled for opaque pass (Acceptance Criterion: Blend state configured for opaque)
    EXPECT_FALSE(state.enable_blend);

    // Write mask should include all channels
    SDL_GPUColorComponentFlags fullMask = BlendState::fullWriteMask();
    EXPECT_EQ(state.color_write_mask, fullMask);

    printf("  [INFO] Opaque: blend=OFF, write_mask=RGBA\n");
}

// =============================================================================
// Test: Transparent Blend State Configuration
// =============================================================================
void test_TransparentBlendState() {
    TEST_CASE("Transparent blend state configuration");

    SDL_GPUColorTargetBlendState state = BlendState::transparent();

    // Blend must be enabled for transparent pass (Acceptance Criterion: Blend state configured for transparent)
    EXPECT_TRUE(state.enable_blend);

    // Standard alpha blending: srcAlpha * src + (1 - srcAlpha) * dst
    EXPECT_EQ(state.src_color_blendfactor, SDL_GPU_BLENDFACTOR_SRC_ALPHA);
    EXPECT_EQ(state.dst_color_blendfactor, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
    EXPECT_EQ(state.color_blend_op, SDL_GPU_BLENDOP_ADD);

    // Alpha blending: one * src + (1 - srcAlpha) * dst
    EXPECT_EQ(state.src_alpha_blendfactor, SDL_GPU_BLENDFACTOR_ONE);
    EXPECT_EQ(state.dst_alpha_blendfactor, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
    EXPECT_EQ(state.alpha_blend_op, SDL_GPU_BLENDOP_ADD);

    // Write mask should include all channels
    SDL_GPUColorComponentFlags fullMask = BlendState::fullWriteMask();
    EXPECT_EQ(state.color_write_mask, fullMask);

    printf("  [INFO] Transparent: blend=ON, src=SRC_ALPHA, dst=ONE_MINUS_SRC_ALPHA\n");
}

// =============================================================================
// Test: Additive Blend State Configuration
// =============================================================================
void test_AdditiveBlendState() {
    TEST_CASE("Additive blend state configuration");

    SDL_GPUColorTargetBlendState state = BlendState::additive();

    // Blend must be enabled
    EXPECT_TRUE(state.enable_blend);

    // Additive blending: one * src + one * dst
    EXPECT_EQ(state.src_color_blendfactor, SDL_GPU_BLENDFACTOR_ONE);
    EXPECT_EQ(state.dst_color_blendfactor, SDL_GPU_BLENDFACTOR_ONE);
    EXPECT_EQ(state.color_blend_op, SDL_GPU_BLENDOP_ADD);

    // Alpha should also be additive
    EXPECT_EQ(state.src_alpha_blendfactor, SDL_GPU_BLENDFACTOR_ONE);
    EXPECT_EQ(state.dst_alpha_blendfactor, SDL_GPU_BLENDFACTOR_ONE);
    EXPECT_EQ(state.alpha_blend_op, SDL_GPU_BLENDOP_ADD);

    printf("  [INFO] Additive: blend=ON, src=ONE, dst=ONE\n");
}

// =============================================================================
// Test: Premultiplied Alpha Blend State Configuration
// =============================================================================
void test_PremultipliedBlendState() {
    TEST_CASE("Premultiplied alpha blend state configuration");

    SDL_GPUColorTargetBlendState state = BlendState::premultiplied();

    // Blend must be enabled
    EXPECT_TRUE(state.enable_blend);

    // Premultiplied: one * src + (1 - srcAlpha) * dst
    EXPECT_EQ(state.src_color_blendfactor, SDL_GPU_BLENDFACTOR_ONE);
    EXPECT_EQ(state.dst_color_blendfactor, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
    EXPECT_EQ(state.color_blend_op, SDL_GPU_BLENDOP_ADD);

    printf("  [INFO] Premultiplied: blend=ON, src=ONE, dst=ONE_MINUS_SRC_ALPHA\n");
}

// =============================================================================
// Test: Custom Blend State Configuration
// =============================================================================
void test_CustomBlendState() {
    TEST_CASE("Custom blend state configuration");

    // Test custom configuration with reverse subtract
    SDL_GPUColorTargetBlendState state = BlendState::custom(
        SDL_GPU_BLENDFACTOR_DST_COLOR,
        SDL_GPU_BLENDFACTOR_SRC_COLOR,
        SDL_GPU_BLENDOP_SUBTRACT,
        SDL_GPU_BLENDFACTOR_DST_ALPHA,
        SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        SDL_GPU_BLENDOP_REVERSE_SUBTRACT);

    EXPECT_TRUE(state.enable_blend);
    EXPECT_EQ(state.src_color_blendfactor, SDL_GPU_BLENDFACTOR_DST_COLOR);
    EXPECT_EQ(state.dst_color_blendfactor, SDL_GPU_BLENDFACTOR_SRC_COLOR);
    EXPECT_EQ(state.color_blend_op, SDL_GPU_BLENDOP_SUBTRACT);
    EXPECT_EQ(state.src_alpha_blendfactor, SDL_GPU_BLENDFACTOR_DST_ALPHA);
    EXPECT_EQ(state.dst_alpha_blendfactor, SDL_GPU_BLENDFACTOR_SRC_ALPHA);
    EXPECT_EQ(state.alpha_blend_op, SDL_GPU_BLENDOP_REVERSE_SUBTRACT);
}

// =============================================================================
// Test: Write Mask Configuration
// =============================================================================
void test_WriteMaskConfiguration() {
    TEST_CASE("Write mask configuration");

    // Test with only RGB write (no alpha)
    SDL_GPUColorComponentFlags rgbMask = static_cast<SDL_GPUColorComponentFlags>(
        SDL_GPU_COLORCOMPONENT_R |
        SDL_GPU_COLORCOMPONENT_G |
        SDL_GPU_COLORCOMPONENT_B
    );

    SDL_GPUColorTargetBlendState state = BlendState::withWriteMask(false, rgbMask);

    EXPECT_FALSE(state.enable_blend);
    EXPECT_EQ(state.color_write_mask, rgbMask);

    // Test with only alpha write
    SDL_GPUColorComponentFlags alphaMask = static_cast<SDL_GPUColorComponentFlags>(
        SDL_GPU_COLORCOMPONENT_A
    );

    state = BlendState::withWriteMask(true, alphaMask);

    EXPECT_TRUE(state.enable_blend);
    EXPECT_EQ(state.color_write_mask, alphaMask);
}

// =============================================================================
// Test: Full Write Mask Constant
// =============================================================================
void test_FullWriteMask() {
    TEST_CASE("Full write mask constant");

    SDL_GPUColorComponentFlags fullMask = BlendState::fullWriteMask();

    // Should include all components
    EXPECT_TRUE((fullMask & SDL_GPU_COLORCOMPONENT_R) != 0);
    EXPECT_TRUE((fullMask & SDL_GPU_COLORCOMPONENT_G) != 0);
    EXPECT_TRUE((fullMask & SDL_GPU_COLORCOMPONENT_B) != 0);
    EXPECT_TRUE((fullMask & SDL_GPU_COLORCOMPONENT_A) != 0);

    printf("  [INFO] Full write mask: RGBA = 0x%X\n", fullMask);
}

// =============================================================================
// Test: Describe Utility Function
// =============================================================================
void test_DescribeUtility() {
    TEST_CASE("Describe utility function");

    SDL_GPUColorTargetBlendState opaqueState = BlendState::opaque();
    const char* opaqueDesc = BlendState::describe(opaqueState);

    EXPECT_STR_CONTAINS(opaqueDesc, "blend=OFF");

    SDL_GPUColorTargetBlendState transparentState = BlendState::transparent();
    const char* transparentDesc = BlendState::describe(transparentState);

    EXPECT_STR_CONTAINS(transparentDesc, "blend=ON");
    EXPECT_STR_CONTAINS(transparentDesc, "SRC_ALPHA");
}

// =============================================================================
// Test: Blend Factor Names
// =============================================================================
void test_BlendFactorNames() {
    TEST_CASE("Blend factor name conversion");

    EXPECT_EQ(strcmp(BlendState::getBlendFactorName(SDL_GPU_BLENDFACTOR_ZERO), "ZERO"), 0);
    EXPECT_EQ(strcmp(BlendState::getBlendFactorName(SDL_GPU_BLENDFACTOR_ONE), "ONE"), 0);
    EXPECT_EQ(strcmp(BlendState::getBlendFactorName(SDL_GPU_BLENDFACTOR_SRC_COLOR), "SRC_COLOR"), 0);
    EXPECT_EQ(strcmp(BlendState::getBlendFactorName(SDL_GPU_BLENDFACTOR_SRC_ALPHA), "SRC_ALPHA"), 0);
    EXPECT_EQ(strcmp(BlendState::getBlendFactorName(SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA), "ONE_MINUS_SRC_ALPHA"), 0);
    EXPECT_EQ(strcmp(BlendState::getBlendFactorName(SDL_GPU_BLENDFACTOR_DST_ALPHA), "DST_ALPHA"), 0);
}

// =============================================================================
// Test: Blend Operation Names
// =============================================================================
void test_BlendOpNames() {
    TEST_CASE("Blend operation name conversion");

    EXPECT_EQ(strcmp(BlendState::getBlendOpName(SDL_GPU_BLENDOP_ADD), "ADD"), 0);
    EXPECT_EQ(strcmp(BlendState::getBlendOpName(SDL_GPU_BLENDOP_SUBTRACT), "SUBTRACT"), 0);
    EXPECT_EQ(strcmp(BlendState::getBlendOpName(SDL_GPU_BLENDOP_REVERSE_SUBTRACT), "REVERSE_SUBTRACT"), 0);
    EXPECT_EQ(strcmp(BlendState::getBlendOpName(SDL_GPU_BLENDOP_MIN), "MIN"), 0);
    EXPECT_EQ(strcmp(BlendState::getBlendOpName(SDL_GPU_BLENDOP_MAX), "MAX"), 0);
}

// =============================================================================
// Test: Opaque vs Transparent State Difference
// =============================================================================
void test_OpaqueVsTransparentDifference() {
    TEST_CASE("Opaque vs transparent state key difference");

    SDL_GPUColorTargetBlendState opaqueState = BlendState::opaque();
    SDL_GPUColorTargetBlendState transparentState = BlendState::transparent();

    // KEY DIFFERENCE: blend enable
    EXPECT_FALSE(opaqueState.enable_blend);      // Opaque: blend disabled
    EXPECT_TRUE(transparentState.enable_blend);  // Transparent: blend enabled

    // Both should write all color components
    EXPECT_EQ(opaqueState.color_write_mask, transparentState.color_write_mask);

    printf("  [INFO] Critical difference: opaque has blend OFF, transparent has blend ON\n");
}

// =============================================================================
// Main
// =============================================================================
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("BlendState Unit Tests (Ticket 2-007)\n");
    printf("========================================\n");

    // Run all tests
    test_OpaqueBlendState();
    test_TransparentBlendState();
    test_AdditiveBlendState();
    test_PremultipliedBlendState();
    test_CustomBlendState();
    test_WriteMaskConfiguration();
    test_FullWriteMask();
    test_DescribeUtility();
    test_BlendFactorNames();
    test_BlendOpNames();
    test_OpaqueVsTransparentDifference();

    // Summary
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("========================================\n");

    printf("\nAcceptance Criteria Verification:\n");
    printf("  [x] Blend state configured for opaque mode - Verified in test_OpaqueBlendState\n");
    printf("  [x] Blend state configured for transparent mode - Verified in test_TransparentBlendState\n");
    printf("  [x] Standard alpha blend (srcAlpha, 1-srcAlpha) - Verified in test_TransparentBlendState\n");

    return g_testsFailed > 0 ? 1 : 0;
}
