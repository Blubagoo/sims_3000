/**
 * @file DepthState.cpp
 * @brief Implementation of depth state configuration for graphics pipeline.
 */

#include "sims3000/render/DepthState.h"
#include <cstdio>

namespace sims3000 {

// =============================================================================
// Primary Factory Methods
// =============================================================================

SDL_GPUDepthStencilState DepthState::opaque() {
    SDL_GPUDepthStencilState state = {};

    // Depth testing: enabled with LESS comparison
    // Near objects (smaller depth) pass and occlude far objects
    state.enable_depth_test = true;
    state.enable_depth_write = true;
    state.compare_op = SDL_GPU_COMPAREOP_LESS;

    // Stencil: disabled for standard opaque pass
    state.enable_stencil_test = false;
    state.front_stencil_state = {};
    state.back_stencil_state = {};

    // Stencil masks (used if stencil is enabled later)
    state.compare_mask = 0xFF;
    state.write_mask = 0xFF;

    return state;
}

SDL_GPUDepthStencilState DepthState::transparent() {
    SDL_GPUDepthStencilState state = {};

    // Depth testing: enabled but write DISABLED
    // Transparent objects are occluded by opaque geometry but don't write depth
    // This prevents transparent-on-transparent depth conflicts
    state.enable_depth_test = true;
    state.enable_depth_write = false;  // KEY: Read-only depth test
    state.compare_op = SDL_GPU_COMPAREOP_LESS;

    // Stencil: disabled
    state.enable_stencil_test = false;
    state.front_stencil_state = {};
    state.back_stencil_state = {};

    state.compare_mask = 0xFF;
    state.write_mask = 0xFF;

    return state;
}

SDL_GPUDepthStencilState DepthState::disabled() {
    SDL_GPUDepthStencilState state = {};

    // Both depth test and write disabled
    // Used for fullscreen passes, UI, post-processing
    state.enable_depth_test = false;
    state.enable_depth_write = false;
    state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;

    // Stencil: disabled
    state.enable_stencil_test = false;
    state.front_stencil_state = {};
    state.back_stencil_state = {};

    state.compare_mask = 0xFF;
    state.write_mask = 0xFF;

    return state;
}

// =============================================================================
// Custom Configuration
// =============================================================================

SDL_GPUDepthStencilState DepthState::custom(
    bool enableDepthTest,
    bool enableDepthWrite,
    SDL_GPUCompareOp compareOp)
{
    SDL_GPUDepthStencilState state = {};

    state.enable_depth_test = enableDepthTest;
    state.enable_depth_write = enableDepthWrite;
    state.compare_op = compareOp;

    state.enable_stencil_test = false;
    state.front_stencil_state = {};
    state.back_stencil_state = {};

    state.compare_mask = 0xFF;
    state.write_mask = 0xFF;

    return state;
}

SDL_GPUDepthStencilState DepthState::customWithStencil(
    bool enableDepthTest,
    bool enableDepthWrite,
    SDL_GPUCompareOp compareOp,
    bool enableStencilTest,
    uint8_t stencilReadMask,
    uint8_t stencilWriteMask)
{
    SDL_GPUDepthStencilState state = {};

    state.enable_depth_test = enableDepthTest;
    state.enable_depth_write = enableDepthWrite;
    state.compare_op = compareOp;

    state.enable_stencil_test = enableStencilTest;

    // Default stencil operations (can be customized per use case)
    if (enableStencilTest) {
        SDL_GPUStencilOpState stencilOp = {};
        stencilOp.fail_op = SDL_GPU_STENCILOP_KEEP;
        stencilOp.pass_op = SDL_GPU_STENCILOP_KEEP;
        stencilOp.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
        stencilOp.compare_op = SDL_GPU_COMPAREOP_ALWAYS;

        state.front_stencil_state = stencilOp;
        state.back_stencil_state = stencilOp;
    } else {
        state.front_stencil_state = {};
        state.back_stencil_state = {};
    }

    state.compare_mask = stencilReadMask;
    state.write_mask = stencilWriteMask;

    return state;
}

// =============================================================================
// Stencil State Presets
// =============================================================================

SDL_GPUStencilOpState DepthState::stencilWrite(uint8_t referenceValue) {
    (void)referenceValue;  // Reference value is set per-draw, not in state

    SDL_GPUStencilOpState stencilOp = {};
    stencilOp.fail_op = SDL_GPU_STENCILOP_KEEP;
    stencilOp.pass_op = SDL_GPU_STENCILOP_REPLACE;  // Write reference value on pass
    stencilOp.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
    stencilOp.compare_op = SDL_GPU_COMPAREOP_ALWAYS;  // Always pass (write mode)

    return stencilOp;
}

SDL_GPUStencilOpState DepthState::stencilRead(
    uint8_t referenceValue,
    SDL_GPUCompareOp compareOp)
{
    (void)referenceValue;  // Reference value is set per-draw, not in state

    SDL_GPUStencilOpState stencilOp = {};
    stencilOp.fail_op = SDL_GPU_STENCILOP_KEEP;
    stencilOp.pass_op = SDL_GPU_STENCILOP_KEEP;
    stencilOp.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
    stencilOp.compare_op = compareOp;

    return stencilOp;
}

// =============================================================================
// Utility
// =============================================================================

const char* DepthState::describe(const SDL_GPUDepthStencilState& state) {
    // Static buffer for description (not thread-safe, but fine for debugging)
    static char buffer[256];

    const char* testStr = state.enable_depth_test ? "ON" : "OFF";
    const char* writeStr = state.enable_depth_write ? "ON" : "OFF";
    const char* stencilStr = state.enable_stencil_test ? "ON" : "OFF";
    const char* compareStr = getCompareOpName(state.compare_op);

    snprintf(buffer, sizeof(buffer),
             "DepthState{test=%s, write=%s, compare=%s, stencil=%s}",
             testStr, writeStr, compareStr, stencilStr);

    return buffer;
}

const char* DepthState::getCompareOpName(SDL_GPUCompareOp op) {
    switch (op) {
        case SDL_GPU_COMPAREOP_NEVER:            return "NEVER";
        case SDL_GPU_COMPAREOP_LESS:             return "LESS";
        case SDL_GPU_COMPAREOP_EQUAL:            return "EQUAL";
        case SDL_GPU_COMPAREOP_LESS_OR_EQUAL:    return "LESS_OR_EQUAL";
        case SDL_GPU_COMPAREOP_GREATER:          return "GREATER";
        case SDL_GPU_COMPAREOP_NOT_EQUAL:        return "NOT_EQUAL";
        case SDL_GPU_COMPAREOP_GREATER_OR_EQUAL: return "GREATER_OR_EQUAL";
        case SDL_GPU_COMPAREOP_ALWAYS:           return "ALWAYS";
        default:                                 return "UNKNOWN";
    }
}

} // namespace sims3000
