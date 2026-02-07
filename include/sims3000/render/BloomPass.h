/**
 * @file BloomPass.h
 * @brief Bloom post-process pass for bioluminescent rendering.
 *
 * Implements a mandatory bloom effect for the bioluminescent art direction.
 * Bloom is applied as a required pipeline stage, extracting bright pixels,
 * blurring them, and blending back into the final image.
 *
 * Pipeline stages:
 * 1. Bright pixel extraction (conservative threshold for dark environment)
 * 2. Gaussian blur (horizontal + vertical separable blur)
 * 3. Additive blend back to final image
 *
 * Quality tiers:
 * - High: 1/2 resolution blur (best quality, ~0.5ms)
 * - Medium: 1/4 resolution blur (default, ~0.3ms)
 * - Low: 1/8 resolution blur (performance, ~0.15ms)
 *
 * Resource ownership:
 * - BloomPass owns bloom render targets and blur resources
 * - BloomPass does NOT own the input texture (scene color)
 * - GPUDevice must outlive BloomPass
 *
 * Usage:
 * @code
 *   BloomPass bloom(device, width, height);
 *
 *   // In render loop after scene render:
 *   bloom.execute(cmdBuffer, sceneColorTexture, outputTexture);
 * @endcode
 */

#ifndef SIMS3000_RENDER_BLOOM_PASS_H
#define SIMS3000_RENDER_BLOOM_PASS_H

#include <SDL3/SDL.h>
#include <cstdint>
#include <string>

namespace sims3000 {

// Forward declarations
class GPUDevice;

/**
 * @enum BloomQuality
 * @brief Bloom quality tiers affecting resolution and performance.
 */
enum class BloomQuality {
    High,   ///< 1/2 resolution blur (~0.5ms at 1080p)
    Medium, ///< 1/4 resolution blur (~0.3ms at 1080p, default)
    Low     ///< 1/8 resolution blur (~0.15ms at 1080p)
};

/**
 * @struct BloomConfig
 * @brief Configuration parameters for bloom effect.
 */
struct BloomConfig {
    /// Brightness threshold for bloom extraction.
    /// Pixels brighter than this contribute to bloom.
    /// Lower values = more bloom, higher = more selective.
    /// Conservative threshold for dark bioluminescent environment.
    float threshold = 0.7f;

    /// Bloom intensity multiplier.
    /// Controls strength of the glow effect.
    float intensity = 1.0f;

    /// Quality tier affecting resolution and performance.
    BloomQuality quality = BloomQuality::Medium;

    /// Minimum bloom intensity (bloom cannot be fully disabled per canon).
    static constexpr float MIN_INTENSITY = 0.1f;
};

/**
 * @struct BloomStats
 * @brief Statistics about bloom pass execution.
 */
struct BloomStats {
    float extractionTimeMs = 0.0f;  ///< Time for bright pixel extraction
    float blurTimeMs = 0.0f;        ///< Time for blur passes
    float compositeTimeMs = 0.0f;   ///< Time for final composite
    float totalTimeMs = 0.0f;       ///< Total bloom pass time
    std::uint32_t bloomWidth = 0;   ///< Width of bloom render target
    std::uint32_t bloomHeight = 0;  ///< Height of bloom render target
};

/**
 * @class BloomPass
 * @brief Mandatory bloom post-process for bioluminescent rendering.
 *
 * Extracts bright pixels from the scene, blurs them, and blends back
 * to create a glow effect around emissive surfaces.
 */
class BloomPass {
public:
    /**
     * Create a bloom pass with default configuration.
     * @param device GPU device for resource creation
     * @param width Render target width (scene width)
     * @param height Render target height (scene height)
     */
    BloomPass(GPUDevice& device, std::uint32_t width, std::uint32_t height);

    /**
     * Create a bloom pass with specified configuration.
     * @param device GPU device for resource creation
     * @param width Render target width
     * @param height Render target height
     * @param config Bloom configuration
     */
    BloomPass(GPUDevice& device, std::uint32_t width, std::uint32_t height,
              const BloomConfig& config);

    ~BloomPass();

    // Non-copyable
    BloomPass(const BloomPass&) = delete;
    BloomPass& operator=(const BloomPass&) = delete;

    // Movable
    BloomPass(BloomPass&& other) noexcept;
    BloomPass& operator=(BloomPass&& other) noexcept;

    /**
     * Check if bloom pass is valid and ready to use.
     * @return true if resources were created successfully
     */
    bool isValid() const;

    /**
     * Execute the bloom pass.
     *
     * This performs the full bloom pipeline:
     * 1. Extract bright pixels from input to bloom target
     * 2. Apply horizontal blur
     * 3. Apply vertical blur
     * 4. Composite bloom with input to output
     *
     * @param cmdBuffer Command buffer for recording
     * @param inputTexture Scene color texture to read
     * @param outputTexture Output texture (may be same as input for in-place)
     * @return true if execution succeeded
     */
    bool execute(SDL_GPUCommandBuffer* cmdBuffer,
                 SDL_GPUTexture* inputTexture,
                 SDL_GPUTexture* outputTexture);

    /**
     * Resize bloom render targets.
     * Call when window/render target size changes.
     *
     * @param width New width
     * @param height New height
     * @return true if resize succeeded
     */
    bool resize(std::uint32_t width, std::uint32_t height);

    /**
     * Get current bloom configuration.
     * @return Current configuration
     */
    const BloomConfig& getConfig() const { return m_config; }

    /**
     * Set bloom configuration.
     * Takes effect on next execute().
     *
     * @param config New configuration
     */
    void setConfig(const BloomConfig& config);

    /**
     * Set bloom threshold.
     * @param threshold New threshold [0.0, 1.0]
     */
    void setThreshold(float threshold);

    /**
     * Set bloom intensity.
     * @param intensity New intensity [MIN_INTENSITY, 2.0]
     */
    void setIntensity(float intensity);

    /**
     * Set bloom quality tier.
     * @param quality New quality tier
     */
    void setQuality(BloomQuality quality);

    /**
     * Get execution statistics from last execute() call.
     * @return Bloom statistics
     */
    const BloomStats& getStats() const { return m_stats; }

    /**
     * Get the last error message.
     * @return Error string
     */
    const std::string& getLastError() const { return m_lastError; }

    /**
     * Get the bloom render target width.
     * @return Width based on quality tier
     */
    std::uint32_t getBloomWidth() const;

    /**
     * Get the bloom render target height.
     * @return Height based on quality tier
     */
    std::uint32_t getBloomHeight() const;

private:
    /**
     * Create bloom render targets and resources.
     * @return true if creation succeeded
     */
    bool createResources();

    /**
     * Release all bloom resources.
     */
    void releaseResources();

    /**
     * Calculate bloom resolution based on quality tier.
     */
    void calculateBloomResolution();

    /**
     * Get resolution divisor for quality tier.
     * @param quality Quality tier
     * @return Divisor (1, 2, 4, or 8)
     */
    static int getQualityDivisor(BloomQuality quality);

    GPUDevice* m_device = nullptr;

    // Source resolution
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;

    // Bloom target resolution (based on quality)
    std::uint32_t m_bloomWidth = 0;
    std::uint32_t m_bloomHeight = 0;

    // Configuration
    BloomConfig m_config;

    // Render targets
    SDL_GPUTexture* m_extractionTarget = nullptr;  // Bright pixels
    SDL_GPUTexture* m_blurTargetA = nullptr;       // Ping for blur
    SDL_GPUTexture* m_blurTargetB = nullptr;       // Pong for blur

    // Sampler for texture reads
    SDL_GPUSampler* m_sampler = nullptr;

    // Graphics pipelines for bloom passes
    SDL_GPUGraphicsPipeline* m_extractPipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_blurPipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_compositePipeline = nullptr;

    // Shaders
    SDL_GPUShader* m_fullscreenVertShader = nullptr;
    SDL_GPUShader* m_extractFragShader = nullptr;
    SDL_GPUShader* m_blurFragShader = nullptr;
    SDL_GPUShader* m_compositeFragShader = nullptr;

    // Color format for render targets
    SDL_GPUTextureFormat m_colorFormat = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;

    // Statistics
    BloomStats m_stats;

    std::string m_lastError;

    /**
     * Create graphics pipelines for bloom passes.
     * @return true if all pipelines created successfully
     */
    bool createPipelines();

    /**
     * Load shaders for bloom passes.
     * @return true if all shaders loaded successfully
     */
    bool loadShaders();

    /**
     * Execute bright pixel extraction pass.
     * @param cmdBuffer Command buffer
     * @param inputTexture Scene color texture
     * @return true if successful
     */
    bool executeExtraction(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPUTexture* inputTexture);

    /**
     * Execute Gaussian blur passes (horizontal + vertical).
     * @param cmdBuffer Command buffer
     * @return true if successful
     */
    bool executeBlur(SDL_GPUCommandBuffer* cmdBuffer);

    /**
     * Execute composite pass (blend bloom with original).
     * @param cmdBuffer Command buffer
     * @param inputTexture Original scene color
     * @param outputTexture Final output texture
     * @return true if successful
     */
    bool executeComposite(SDL_GPUCommandBuffer* cmdBuffer,
                          SDL_GPUTexture* inputTexture,
                          SDL_GPUTexture* outputTexture);
};

/**
 * Convert BloomQuality to string.
 * @param quality Quality tier
 * @return Human-readable string
 */
const char* getBloomQualityName(BloomQuality quality);

} // namespace sims3000

#endif // SIMS3000_RENDER_BLOOM_PASS_H
