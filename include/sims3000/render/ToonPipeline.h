/**
 * @file ToonPipeline.h
 * @brief Graphics pipeline configuration for toon/cel-shaded rendering.
 *
 * Creates and manages SDL_GPUGraphicsPipeline instances configured for the
 * toon rendering style. Supports both opaque and transparent rendering modes
 * with proper depth testing, back-face culling, and blend states.
 *
 * Vertex Input Layout:
 * - Position: vec3 at binding 0, offset 0 (matches ModelLoader::Vertex)
 * - Normal:   vec3 at binding 0, offset 12
 * - TexCoord: vec2 at binding 0, offset 24
 * - Color:    vec4 at binding 0, offset 32 (optional, may be unused in shader)
 *
 * Pipeline States:
 * - Opaque: Depth test ON, depth write ON, blend OFF, cull back
 * - Transparent: Depth test ON, depth write OFF, alpha blend, cull back
 *
 * MRT Consideration (documented for future bloom implementation):
 * - The fragment shader currently outputs to a single color target (SV_Target0)
 * - For bloom, an emissive channel can be output to a second render target
 * - This requires modifying the fragment shader and pipeline color targets
 * - See ToonPipelineConfig::enableEmissiveMRT for configuration
 *
 * Resource ownership:
 * - ToonPipeline owns created SDL_GPUGraphicsPipeline instances
 * - Shaders are owned by caller (typically ShaderCompiler)
 * - GPUDevice must outlive ToonPipeline
 *
 * Usage:
 * @code
 *   GPUDevice device;
 *   ShaderCompiler compiler(device);
 *
 *   // Load shaders
 *   auto vertResult = compiler.loadShader("shaders/toon.vert", ShaderStage::Vertex, ...);
 *   auto fragResult = compiler.loadShader("shaders/toon.frag", ShaderStage::Fragment, ...);
 *
 *   // Create pipeline
 *   ToonPipeline pipeline(device);
 *   if (!pipeline.create(vertResult.shader, fragResult.shader,
 *                        SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
 *                        SDL_GPU_TEXTUREFORMAT_D32_FLOAT)) {
 *       // Handle error
 *   }
 *
 *   // Render with pipeline
 *   SDL_BindGPUGraphicsPipeline(renderPass, pipeline.getOpaquePipeline());
 *   // ... draw opaque geometry ...
 *
 *   SDL_BindGPUGraphicsPipeline(renderPass, pipeline.getTransparentPipeline());
 *   // ... draw transparent geometry (sorted back-to-front) ...
 * @endcode
 */

#ifndef SIMS3000_RENDER_TOON_PIPELINE_H
#define SIMS3000_RENDER_TOON_PIPELINE_H

#include <SDL3/SDL.h>
#include <string>

namespace sims3000 {

// Forward declarations
class GPUDevice;

/**
 * @struct ToonPipelineConfig
 * @brief Configuration options for toon pipeline creation.
 */
struct ToonPipelineConfig {
    // Rasterizer state
    SDL_GPUCullMode cullMode = SDL_GPU_CULLMODE_BACK;       ///< Face culling mode
    SDL_GPUFrontFace frontFace = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE; ///< Winding order

    // Polygon fill mode
    SDL_GPUFillMode fillMode = SDL_GPU_FILLMODE_FILL;       ///< Solid or wireframe

    // Depth bias (for decals/overlays to prevent z-fighting)
    float depthBiasConstant = 0.0f;                         ///< Constant depth bias
    float depthBiasSlope = 0.0f;                            ///< Slope-scaled depth bias
    float depthBiasClamp = 0.0f;                            ///< Maximum depth bias

    // MRT for emissive/bloom (future use)
    // When enabled, the pipeline expects a second color target for emissive output
    // This requires a modified fragment shader that outputs to SV_Target1
    bool enableEmissiveMRT = false;                         ///< Enable multiple render targets
    SDL_GPUTextureFormat emissiveFormat = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT; ///< Emissive target format
};

/**
 * @struct ToonVertexLayout
 * @brief Vertex attribute configuration matching toon shader input.
 *
 * This layout matches:
 * - Shader: toon.vert.hlsl VSInput
 * - C++ struct: ModelLoader::Vertex
 */
struct ToonVertexLayout {
    static constexpr int POSITION_LOCATION = 0;    ///< Attribute location for position
    static constexpr int NORMAL_LOCATION = 1;      ///< Attribute location for normal
    static constexpr int TEXCOORD_LOCATION = 2;    ///< Attribute location for UV

    static constexpr int POSITION_OFFSET = 0;      ///< Byte offset for position (vec3)
    static constexpr int NORMAL_OFFSET = 12;       ///< Byte offset for normal (vec3)
    static constexpr int TEXCOORD_OFFSET = 24;     ///< Byte offset for texCoord (vec2)
    static constexpr int COLOR_OFFSET = 32;        ///< Byte offset for color (vec4)

    static constexpr int VERTEX_STRIDE = 48;       ///< Total stride: vec3+vec3+vec2+vec4 = 48 bytes

    /**
     * Get vertex input state for pipeline creation.
     * @return Configured SDL_GPUVertexInputState
     */
    static SDL_GPUVertexInputState getVertexInputState();

    /**
     * Validate that the vertex layout matches ModelLoader::Vertex struct.
     * Logs warnings if there are discrepancies.
     * @return true if layout is valid
     */
    static bool validate();
};

/**
 * @class ToonPipeline
 * @brief Graphics pipeline manager for toon rendering.
 *
 * Creates and manages opaque and transparent pipelines configured for
 * the toon/cel-shaded rendering style. Supports wireframe mode for debugging
 * mesh geometry via SDL_GPU_FILLMODE_LINE.
 */
class ToonPipeline {
public:
    /**
     * Create toon pipeline manager.
     * @param device GPU device for pipeline creation
     */
    explicit ToonPipeline(GPUDevice& device);

    ~ToonPipeline();

    // Non-copyable
    ToonPipeline(const ToonPipeline&) = delete;
    ToonPipeline& operator=(const ToonPipeline&) = delete;

    // Movable
    ToonPipeline(ToonPipeline&& other) noexcept;
    ToonPipeline& operator=(ToonPipeline&& other) noexcept;

    /**
     * Create the graphics pipelines.
     *
     * Creates both opaque and transparent pipeline variants with the
     * specified shaders and render target formats. Also creates wireframe
     * variants for debug rendering.
     *
     * @param vertexShader Compiled vertex shader (from ShaderCompiler)
     * @param fragmentShader Compiled fragment shader (from ShaderCompiler)
     * @param colorFormat Format of the color render target
     * @param depthFormat Format of the depth render target
     * @param config Optional pipeline configuration
     * @return true if all pipelines created successfully
     */
    bool create(
        SDL_GPUShader* vertexShader,
        SDL_GPUShader* fragmentShader,
        SDL_GPUTextureFormat colorFormat,
        SDL_GPUTextureFormat depthFormat,
        const ToonPipelineConfig& config = {});

    /**
     * Release all pipeline resources.
     */
    void destroy();

    /**
     * Check if pipelines are valid and ready for use.
     * @return true if both pipelines are created
     */
    bool isValid() const;

    /**
     * Get the opaque rendering pipeline.
     *
     * Configuration:
     * - Depth test: LESS comparison
     * - Depth write: enabled
     * - Blend: disabled
     * - Cull: back faces
     *
     * @return Pipeline handle, or nullptr if not created
     */
    SDL_GPUGraphicsPipeline* getOpaquePipeline() const;

    /**
     * Get the transparent rendering pipeline.
     *
     * Configuration:
     * - Depth test: LESS comparison
     * - Depth write: disabled (read-only)
     * - Blend: standard alpha blend (srcAlpha, 1-srcAlpha)
     * - Cull: back faces
     *
     * @return Pipeline handle, or nullptr if not created
     */
    SDL_GPUGraphicsPipeline* getTransparentPipeline() const;

    // =========================================================================
    // Wireframe Mode (Ticket 2-041)
    // =========================================================================

    /**
     * Check if wireframe mode is enabled.
     * @return true if wireframe rendering is active
     */
    bool isWireframeEnabled() const;

    /**
     * Toggle wireframe mode on/off.
     * @return New wireframe mode state (true = enabled)
     */
    bool toggleWireframe();

    /**
     * Set wireframe mode.
     * @param enabled true to enable wireframe, false for solid fill
     */
    void setWireframeEnabled(bool enabled);

    /**
     * Get the opaque wireframe pipeline.
     *
     * Same configuration as opaque pipeline but with SDL_GPU_FILLMODE_LINE.
     * Shows all triangle edges for debugging mesh geometry.
     *
     * @return Pipeline handle, or nullptr if not created
     */
    SDL_GPUGraphicsPipeline* getOpaqueWireframePipeline() const;

    /**
     * Get the transparent wireframe pipeline.
     *
     * Same configuration as transparent pipeline but with SDL_GPU_FILLMODE_LINE.
     *
     * @return Pipeline handle, or nullptr if not created
     */
    SDL_GPUGraphicsPipeline* getTransparentWireframePipeline() const;

    /**
     * Get the appropriate opaque pipeline based on wireframe mode.
     *
     * @return Wireframe or solid opaque pipeline based on current mode
     */
    SDL_GPUGraphicsPipeline* getCurrentOpaquePipeline() const;

    /**
     * Get the appropriate transparent pipeline based on wireframe mode.
     *
     * @return Wireframe or solid transparent pipeline based on current mode
     */
    SDL_GPUGraphicsPipeline* getCurrentTransparentPipeline() const;

    /**
     * Get the last error message.
     * @return Error string from last failed operation
     */
    const std::string& getLastError() const;

    /**
     * Log pipeline configuration details.
     */
    void logConfiguration() const;

    // =========================================================================
    // Utility
    // =========================================================================

    /**
     * Get color target description for opaque rendering.
     *
     * @param format Color target format
     * @return Configured color target description
     */
    static SDL_GPUColorTargetDescription getOpaqueColorTarget(
        SDL_GPUTextureFormat format);

    /**
     * Get color target description for transparent rendering.
     *
     * @param format Color target format
     * @return Configured color target description
     */
    static SDL_GPUColorTargetDescription getTransparentColorTarget(
        SDL_GPUTextureFormat format);

private:
    /**
     * Create a single pipeline with specified configuration.
     *
     * @param vertexShader Vertex shader
     * @param fragmentShader Fragment shader
     * @param colorFormat Color target format
     * @param depthFormat Depth target format
     * @param enableDepthWrite Whether to enable depth writes
     * @param enableBlend Whether to enable alpha blending
     * @param config Pipeline configuration
     * @return Created pipeline, or nullptr on failure
     */
    SDL_GPUGraphicsPipeline* createPipeline(
        SDL_GPUShader* vertexShader,
        SDL_GPUShader* fragmentShader,
        SDL_GPUTextureFormat colorFormat,
        SDL_GPUTextureFormat depthFormat,
        bool enableDepthWrite,
        bool enableBlend,
        const ToonPipelineConfig& config);

    /**
     * Clean up resources.
     */
    void cleanup();

    GPUDevice* m_device = nullptr;
    SDL_GPUGraphicsPipeline* m_opaquePipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_transparentPipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_opaqueWireframePipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_transparentWireframePipeline = nullptr;
    std::string m_lastError;

    // Wireframe mode state
    bool m_wireframeEnabled = false;

    // Store format for logging
    SDL_GPUTextureFormat m_colorFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUTextureFormat m_depthFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    ToonPipelineConfig m_config;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_TOON_PIPELINE_H
