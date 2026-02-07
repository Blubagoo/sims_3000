/**
 * @file DepthState.h
 * @brief Depth state configuration for graphics pipeline depth testing.
 *
 * Provides pre-configured depth state structures for opaque and transparent
 * rendering passes. The opaque pass uses depth test and depth write enabled
 * with LESS comparison. The transparent pass uses depth test enabled but
 * depth write disabled (read-only) to ensure correct ordering.
 *
 * Usage:
 * @code
 *   // Creating pipeline for opaque geometry
 *   SDL_GPUDepthStencilState opaqueDepth = DepthState::opaque();
 *
 *   // Creating pipeline for transparent geometry
 *   SDL_GPUDepthStencilState transparentDepth = DepthState::transparent();
 *
 *   // Custom depth state
 *   SDL_GPUDepthStencilState custom = DepthState::custom(
 *       true,                      // enable depth test
 *       true,                      // enable depth write
 *       SDL_GPU_COMPAREOP_LESS_OR_EQUAL
 *   );
 * @endcode
 *
 * Resource ownership:
 * - DepthState is a static factory class, no resources owned
 * - Returned SDL_GPUDepthStencilState is a value type, copied to pipeline creation
 */

#ifndef SIMS3000_RENDER_DEPTH_STATE_H
#define SIMS3000_RENDER_DEPTH_STATE_H

#include <SDL3/SDL.h>

namespace sims3000 {

/**
 * @class DepthState
 * @brief Factory class for creating depth state configurations.
 *
 * Provides static methods to create pre-configured depth states for common
 * rendering scenarios (opaque pass, transparent pass) and custom configurations.
 *
 * Key configuration options:
 * - **Depth Test:** Whether to compare fragment depth against depth buffer
 * - **Depth Write:** Whether to write fragment depth to depth buffer
 * - **Compare Operation:** How to compare depths (LESS, LESS_OR_EQUAL, etc.)
 *
 * Standard configurations:
 * - **Opaque:** Test=ON, Write=ON, Compare=LESS (near objects occlude far)
 * - **Transparent:** Test=ON, Write=OFF, Compare=LESS (read-only depth test)
 * - **Disabled:** Test=OFF, Write=OFF (no depth processing)
 */
class DepthState {
public:
    // Static factory class - no instances
    DepthState() = delete;

    /**
     * Create depth state for opaque geometry pass.
     *
     * Configuration:
     * - Depth test enabled
     * - Depth write enabled
     * - Compare operation: LESS (near objects occlude far)
     * - Stencil test disabled
     *
     * Use this for all opaque geometry. Objects will write their depth
     * values and be occluded by closer objects.
     *
     * @return Configured SDL_GPUDepthStencilState for opaque rendering
     */
    static SDL_GPUDepthStencilState opaque();

    /**
     * Create depth state for transparent geometry pass.
     *
     * Configuration:
     * - Depth test enabled
     * - Depth write DISABLED (read-only)
     * - Compare operation: LESS
     * - Stencil test disabled
     *
     * Use this for transparent objects rendered after the opaque pass.
     * Objects will be occluded by opaque geometry but won't write depth,
     * preventing transparent-on-transparent sorting issues.
     *
     * @return Configured SDL_GPUDepthStencilState for transparent rendering
     */
    static SDL_GPUDepthStencilState transparent();

    /**
     * Create depth state with depth testing disabled.
     *
     * Configuration:
     * - Depth test disabled
     * - Depth write disabled
     * - Stencil test disabled
     *
     * Use this for UI overlays or post-processing effects that should
     * always render regardless of depth.
     *
     * @return Configured SDL_GPUDepthStencilState with depth disabled
     */
    static SDL_GPUDepthStencilState disabled();

    /**
     * Create custom depth state configuration.
     *
     * @param enableDepthTest Whether to enable depth testing
     * @param enableDepthWrite Whether to enable depth writing
     * @param compareOp Depth comparison operation (default: LESS)
     * @return Configured SDL_GPUDepthStencilState with custom settings
     */
    static SDL_GPUDepthStencilState custom(
        bool enableDepthTest,
        bool enableDepthWrite,
        SDL_GPUCompareOp compareOp = SDL_GPU_COMPAREOP_LESS);

    /**
     * Create custom depth state with stencil configuration.
     *
     * @param enableDepthTest Whether to enable depth testing
     * @param enableDepthWrite Whether to enable depth writing
     * @param compareOp Depth comparison operation
     * @param enableStencilTest Whether to enable stencil testing
     * @param stencilReadMask Stencil read mask (default: 0xFF)
     * @param stencilWriteMask Stencil write mask (default: 0xFF)
     * @return Configured SDL_GPUDepthStencilState with depth and stencil settings
     */
    static SDL_GPUDepthStencilState customWithStencil(
        bool enableDepthTest,
        bool enableDepthWrite,
        SDL_GPUCompareOp compareOp,
        bool enableStencilTest,
        uint8_t stencilReadMask = 0xFF,
        uint8_t stencilWriteMask = 0xFF);

    // =========================================================================
    // Stencil State Presets
    // =========================================================================

    /**
     * Create stencil state for writing stencil values.
     *
     * @param referenceValue Value to write/compare against
     * @return Configured SDL_GPUStencilOpState for stencil writes
     */
    static SDL_GPUStencilOpState stencilWrite(uint8_t referenceValue = 1);

    /**
     * Create stencil state for reading/testing stencil values.
     *
     * @param referenceValue Value to compare against
     * @param compareOp Stencil comparison operation (default: EQUAL)
     * @return Configured SDL_GPUStencilOpState for stencil reads
     */
    static SDL_GPUStencilOpState stencilRead(
        uint8_t referenceValue = 1,
        SDL_GPUCompareOp compareOp = SDL_GPU_COMPAREOP_EQUAL);

    // =========================================================================
    // Utility
    // =========================================================================

    /**
     * Get a human-readable description of the depth state configuration.
     *
     * @param state The depth stencil state to describe
     * @return String description of the configuration
     */
    static const char* describe(const SDL_GPUDepthStencilState& state);

    /**
     * Get human-readable name for a compare operation.
     *
     * @param op The compare operation
     * @return String name of the operation
     */
    static const char* getCompareOpName(SDL_GPUCompareOp op);
};

} // namespace sims3000

#endif // SIMS3000_RENDER_DEPTH_STATE_H
