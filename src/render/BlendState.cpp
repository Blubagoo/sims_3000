/**
 * @file BlendState.cpp
 * @brief Implementation of blend state configuration for graphics pipeline.
 */

#include "sims3000/render/BlendState.h"
#include <cstdio>

namespace sims3000 {

// =============================================================================
// Primary Factory Methods
// =============================================================================

SDL_GPUColorTargetBlendState BlendState::opaque() {
    SDL_GPUColorTargetBlendState state = {};

    // Blending disabled - fragments replace framebuffer content
    state.enable_blend = false;

    // Color blend factors (ignored when blend disabled, but set for clarity)
    state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    state.color_blend_op = SDL_GPU_BLENDOP_ADD;

    // Alpha blend factors
    state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    // Write all color components
    state.color_write_mask = fullWriteMask();

    return state;
}

SDL_GPUColorTargetBlendState BlendState::transparent() {
    SDL_GPUColorTargetBlendState state = {};

    // Standard alpha blending enabled
    state.enable_blend = true;

    // Color blend: srcAlpha * src + (1 - srcAlpha) * dst
    state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    state.color_blend_op = SDL_GPU_BLENDOP_ADD;

    // Alpha blend: one * src + (1 - srcAlpha) * dst
    // This ensures alpha accumulates correctly for layered transparency
    state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    // Write all color components
    state.color_write_mask = fullWriteMask();

    return state;
}

SDL_GPUColorTargetBlendState BlendState::additive() {
    SDL_GPUColorTargetBlendState state = {};

    // Additive blending enabled
    state.enable_blend = true;

    // Color blend: one * src + one * dst (additive)
    state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    state.color_blend_op = SDL_GPU_BLENDOP_ADD;

    // Alpha blend: same as color
    state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    // Write all color components
    state.color_write_mask = fullWriteMask();

    return state;
}

SDL_GPUColorTargetBlendState BlendState::premultiplied() {
    SDL_GPUColorTargetBlendState state = {};

    // Premultiplied alpha blending enabled
    state.enable_blend = true;

    // Color blend: one * src + (1 - srcAlpha) * dst
    // For premultiplied alpha, source RGB is already multiplied by alpha
    state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    state.color_blend_op = SDL_GPU_BLENDOP_ADD;

    // Alpha blend: same as color
    state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    // Write all color components
    state.color_write_mask = fullWriteMask();

    return state;
}

// =============================================================================
// Custom Configuration
// =============================================================================

SDL_GPUColorTargetBlendState BlendState::custom(
    SDL_GPUBlendFactor srcColorFactor,
    SDL_GPUBlendFactor dstColorFactor,
    SDL_GPUBlendOp colorOp,
    SDL_GPUBlendFactor srcAlphaFactor,
    SDL_GPUBlendFactor dstAlphaFactor,
    SDL_GPUBlendOp alphaOp)
{
    SDL_GPUColorTargetBlendState state = {};

    state.enable_blend = true;

    state.src_color_blendfactor = srcColorFactor;
    state.dst_color_blendfactor = dstColorFactor;
    state.color_blend_op = colorOp;

    state.src_alpha_blendfactor = srcAlphaFactor;
    state.dst_alpha_blendfactor = dstAlphaFactor;
    state.alpha_blend_op = alphaOp;

    state.color_write_mask = fullWriteMask();

    return state;
}

SDL_GPUColorTargetBlendState BlendState::withWriteMask(
    bool enableBlend,
    SDL_GPUColorComponentFlags writeMask)
{
    SDL_GPUColorTargetBlendState state = {};

    state.enable_blend = enableBlend;

    // Default blend factors (standard alpha blend if enabled)
    state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    state.color_blend_op = SDL_GPU_BLENDOP_ADD;

    state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    state.color_write_mask = writeMask;

    return state;
}

// =============================================================================
// Utility
// =============================================================================

const char* BlendState::describe(const SDL_GPUColorTargetBlendState& state) {
    // Static buffer for description (not thread-safe, but fine for debugging)
    static char buffer[256];

    const char* enableStr = state.enable_blend ? "ON" : "OFF";
    const char* srcColorStr = getBlendFactorName(state.src_color_blendfactor);
    const char* dstColorStr = getBlendFactorName(state.dst_color_blendfactor);
    const char* colorOpStr = getBlendOpName(state.color_blend_op);

    snprintf(buffer, sizeof(buffer),
             "BlendState{blend=%s, src=%s, dst=%s, op=%s}",
             enableStr, srcColorStr, dstColorStr, colorOpStr);

    return buffer;
}

const char* BlendState::getBlendFactorName(SDL_GPUBlendFactor factor) {
    switch (factor) {
        case SDL_GPU_BLENDFACTOR_ZERO:                     return "ZERO";
        case SDL_GPU_BLENDFACTOR_ONE:                      return "ONE";
        case SDL_GPU_BLENDFACTOR_SRC_COLOR:                return "SRC_COLOR";
        case SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR:      return "ONE_MINUS_SRC_COLOR";
        case SDL_GPU_BLENDFACTOR_DST_COLOR:                return "DST_COLOR";
        case SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR:      return "ONE_MINUS_DST_COLOR";
        case SDL_GPU_BLENDFACTOR_SRC_ALPHA:                return "SRC_ALPHA";
        case SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:      return "ONE_MINUS_SRC_ALPHA";
        case SDL_GPU_BLENDFACTOR_DST_ALPHA:                return "DST_ALPHA";
        case SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA:      return "ONE_MINUS_DST_ALPHA";
        case SDL_GPU_BLENDFACTOR_CONSTANT_COLOR:           return "CONSTANT_COLOR";
        case SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR: return "ONE_MINUS_CONSTANT_COLOR";
        case SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE:       return "SRC_ALPHA_SATURATE";
        default:                                           return "UNKNOWN";
    }
}

const char* BlendState::getBlendOpName(SDL_GPUBlendOp op) {
    switch (op) {
        case SDL_GPU_BLENDOP_ADD:              return "ADD";
        case SDL_GPU_BLENDOP_SUBTRACT:         return "SUBTRACT";
        case SDL_GPU_BLENDOP_REVERSE_SUBTRACT: return "REVERSE_SUBTRACT";
        case SDL_GPU_BLENDOP_MIN:              return "MIN";
        case SDL_GPU_BLENDOP_MAX:              return "MAX";
        default:                              return "UNKNOWN";
    }
}

} // namespace sims3000
