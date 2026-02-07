/**
 * @file BlendState.h
 * @brief Blend state configuration for graphics pipeline color blending.
 *
 * Provides pre-configured blend state structures for opaque and transparent
 * rendering passes. Opaque pass disables blending, transparent pass uses
 * standard alpha blending (src_alpha, one_minus_src_alpha).
 *
 * Usage:
 * @code
 *   // Creating pipeline for opaque geometry
 *   SDL_GPUColorTargetBlendState opaqueBlend = BlendState::opaque();
 *
 *   // Creating pipeline for transparent geometry
 *   SDL_GPUColorTargetBlendState transparentBlend = BlendState::transparent();
 *
 *   // Additive blending (for particles, glow effects)
 *   SDL_GPUColorTargetBlendState additiveBlend = BlendState::additive();
 *
 *   // Custom blend state
 *   SDL_GPUColorTargetBlendState custom = BlendState::custom(
 *       SDL_GPU_BLENDFACTOR_SRC_ALPHA,
 *       SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
 *       SDL_GPU_BLENDOP_ADD,
 *       SDL_GPU_BLENDFACTOR_ONE,
 *       SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
 *       SDL_GPU_BLENDOP_ADD
 *   );
 * @endcode
 *
 * Resource ownership:
 * - BlendState is a static factory class, no resources owned
 * - Returned SDL_GPUColorTargetBlendState is a value type, copied to pipeline creation
 */

#ifndef SIMS3000_RENDER_BLEND_STATE_H
#define SIMS3000_RENDER_BLEND_STATE_H

#include <SDL3/SDL.h>

namespace sims3000 {

/**
 * @class BlendState
 * @brief Factory class for creating blend state configurations.
 *
 * Provides static methods to create pre-configured blend states for common
 * rendering scenarios (opaque pass, transparent pass, additive blending)
 * and custom configurations.
 *
 * Key configuration options:
 * - **Enable Blend:** Whether to enable blending for this target
 * - **Color Blend:** How to blend RGB components (srcFactor, dstFactor, operation)
 * - **Alpha Blend:** How to blend alpha component (srcFactor, dstFactor, operation)
 * - **Color Write Mask:** Which color channels to write (R, G, B, A)
 *
 * Standard configurations:
 * - **Opaque:** Blend disabled, all channels written
 * - **Transparent:** Standard alpha blend (srcAlpha, 1-srcAlpha)
 * - **Additive:** Additive blend (one, one) for glow/particles
 * - **Premultiplied:** For premultiplied alpha textures (one, 1-srcAlpha)
 */
class BlendState {
public:
    // Static factory class - no instances
    BlendState() = delete;

    /**
     * Create blend state for opaque geometry pass.
     *
     * Configuration:
     * - Blending disabled
     * - All color channels written (RGBA)
     *
     * Use this for all opaque geometry. Fragments completely replace
     * the existing framebuffer content.
     *
     * @return Configured SDL_GPUColorTargetBlendState for opaque rendering
     */
    static SDL_GPUColorTargetBlendState opaque();

    /**
     * Create blend state for transparent geometry pass.
     *
     * Configuration:
     * - Blending enabled
     * - Color blend: srcAlpha * src + (1 - srcAlpha) * dst
     * - Alpha blend: one * src + (1 - srcAlpha) * dst
     * - All color channels written (RGBA)
     *
     * Use this for transparent objects with standard alpha blending.
     * Requires back-to-front rendering order for correct results.
     *
     * @return Configured SDL_GPUColorTargetBlendState for transparent rendering
     */
    static SDL_GPUColorTargetBlendState transparent();

    /**
     * Create blend state for additive blending.
     *
     * Configuration:
     * - Blending enabled
     * - Color blend: one * src + one * dst
     * - Alpha blend: one * src + one * dst
     * - All color channels written (RGBA)
     *
     * Use this for particles, glow effects, and other additive effects.
     * Order-independent (can be rendered in any order).
     *
     * @return Configured SDL_GPUColorTargetBlendState for additive blending
     */
    static SDL_GPUColorTargetBlendState additive();

    /**
     * Create blend state for premultiplied alpha textures.
     *
     * Configuration:
     * - Blending enabled
     * - Color blend: one * src + (1 - srcAlpha) * dst
     * - Alpha blend: one * src + (1 - srcAlpha) * dst
     * - All color channels written (RGBA)
     *
     * Use this for textures where RGB is already multiplied by alpha.
     * Common for anti-aliased text and pre-processed sprites.
     *
     * @return Configured SDL_GPUColorTargetBlendState for premultiplied alpha
     */
    static SDL_GPUColorTargetBlendState premultiplied();

    /**
     * Create custom blend state configuration.
     *
     * @param srcColorFactor Source factor for color blend
     * @param dstColorFactor Destination factor for color blend
     * @param colorOp Blend operation for color
     * @param srcAlphaFactor Source factor for alpha blend
     * @param dstAlphaFactor Destination factor for alpha blend
     * @param alphaOp Blend operation for alpha
     * @return Configured SDL_GPUColorTargetBlendState with custom settings
     */
    static SDL_GPUColorTargetBlendState custom(
        SDL_GPUBlendFactor srcColorFactor,
        SDL_GPUBlendFactor dstColorFactor,
        SDL_GPUBlendOp colorOp,
        SDL_GPUBlendFactor srcAlphaFactor,
        SDL_GPUBlendFactor dstAlphaFactor,
        SDL_GPUBlendOp alphaOp);

    /**
     * Create blend state with color write mask control.
     *
     * @param enableBlend Whether to enable blending
     * @param writeMask Combination of SDL_GPU_COLORCOMPONENT_* flags
     * @return Configured SDL_GPUColorTargetBlendState with write mask
     */
    static SDL_GPUColorTargetBlendState withWriteMask(
        bool enableBlend,
        SDL_GPUColorComponentFlags writeMask);

    // =========================================================================
    // Utility
    // =========================================================================

    /**
     * Get a human-readable description of the blend state configuration.
     *
     * @param state The blend state to describe
     * @return String description of the configuration
     */
    static const char* describe(const SDL_GPUColorTargetBlendState& state);

    /**
     * Get human-readable name for a blend factor.
     *
     * @param factor The blend factor
     * @return String name of the factor
     */
    static const char* getBlendFactorName(SDL_GPUBlendFactor factor);

    /**
     * Get human-readable name for a blend operation.
     *
     * @param op The blend operation
     * @return String name of the operation
     */
    static const char* getBlendOpName(SDL_GPUBlendOp op);

    /**
     * Get full color write mask (RGBA).
     * @return Flags for writing all color components
     */
    static constexpr SDL_GPUColorComponentFlags fullWriteMask() {
        return static_cast<SDL_GPUColorComponentFlags>(
            SDL_GPU_COLORCOMPONENT_R |
            SDL_GPU_COLORCOMPONENT_G |
            SDL_GPU_COLORCOMPONENT_B |
            SDL_GPU_COLORCOMPONENT_A
        );
    }
};

} // namespace sims3000

#endif // SIMS3000_RENDER_BLEND_STATE_H
