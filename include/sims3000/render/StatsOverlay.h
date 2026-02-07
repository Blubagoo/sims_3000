/**
 * @file StatsOverlay.h
 * @brief Render statistics display overlay for development and performance monitoring.
 *
 * Displays real-time render statistics including:
 * - FPS (frames per second)
 * - Frame time (milliseconds)
 * - Draw call count
 * - Visible triangle count
 *
 * Uses SDL_GPU for rendering with SDL3_ttf GPU text engine.
 * Toggle visibility via debug key (F3 by default).
 *
 * Resource ownership:
 * - StatsOverlay owns pipeline, text engine, font, and GPU resources
 * - GPUDevice and Window must outlive StatsOverlay
 * - Call release() before destroying to clean up GPU resources
 *
 * Usage:
 * @code
 *   StatsOverlay stats(device, window);
 *
 *   // Toggle with debug key
 *   if (debugStatsKeyPressed) stats.toggle();
 *
 *   // Each frame, after scene rendering:
 *   if (stats.isEnabled()) {
 *       stats.update(frameStats, renderStats);
 *       stats.render(cmdBuffer, swapchainTexture, width, height);
 *   }
 * @endcode
 */

#ifndef SIMS3000_RENDER_STATS_OVERLAY_H
#define SIMS3000_RENDER_STATS_OVERLAY_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdint>
#include <string>
#include <array>

namespace sims3000 {

// Forward declarations
class GPUDevice;
class Window;
class FrameStats;
struct MainRenderPassStats;

/**
 * @struct StatsOverlayConfig
 * @brief Configuration for stats overlay appearance.
 */
struct StatsOverlayConfig {
    /// Font size in points
    float fontSize = 16.0f;

    /// Text color (RGBA)
    std::uint8_t textR = 255;
    std::uint8_t textG = 255;
    std::uint8_t textB = 255;
    std::uint8_t textA = 255;

    /// Background color (RGBA) - semi-transparent dark
    std::uint8_t bgR = 0;
    std::uint8_t bgG = 0;
    std::uint8_t bgB = 0;
    std::uint8_t bgA = 180;

    /// Padding around text in pixels
    float paddingX = 8.0f;
    float paddingY = 4.0f;

    /// Position offset from corner (pixels)
    float offsetX = 10.0f;
    float offsetY = 10.0f;

    /// Line spacing multiplier
    float lineSpacing = 1.2f;

    /// Position: 0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right
    int position = 0;
};

/**
 * @struct StatsData
 * @brief Collected statistics for display.
 */
struct StatsData {
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    float minFrameTimeMs = 0.0f;
    float maxFrameTimeMs = 0.0f;
    std::uint32_t drawCalls = 0;
    std::uint32_t triangles = 0;
    std::uint64_t totalFrames = 0;
};

/**
 * @class StatsOverlay
 * @brief Renders statistics overlay using SDL_GPU and SDL3_ttf.
 *
 * Displays FPS, frame time, draw calls, and triangle count in a
 * semi-transparent overlay. Toggle visibility with debug key.
 */
class StatsOverlay {
public:
    /**
     * Create stats overlay.
     * @param device GPU device for resource creation
     * @param window Window for dimensions
     */
    StatsOverlay(GPUDevice& device, Window& window);

    /**
     * Create stats overlay with custom configuration.
     * @param device GPU device for resource creation
     * @param window Window for dimensions
     * @param config Overlay configuration
     */
    StatsOverlay(GPUDevice& device, Window& window, const StatsOverlayConfig& config);

    ~StatsOverlay();

    // Non-copyable
    StatsOverlay(const StatsOverlay&) = delete;
    StatsOverlay& operator=(const StatsOverlay&) = delete;

    // Movable
    StatsOverlay(StatsOverlay&& other) noexcept;
    StatsOverlay& operator=(StatsOverlay&& other) noexcept;

    /**
     * Check if overlay is valid and ready to use.
     * @return true if resources were created successfully
     */
    bool isValid() const;

    /**
     * Enable or disable the stats overlay.
     * @param enabled True to enable, false to disable
     */
    void setEnabled(bool enabled);

    /**
     * Toggle the stats overlay on/off.
     */
    void toggle();

    /**
     * Check if stats overlay is currently enabled.
     * @return true if enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * Update stats data from frame and render statistics.
     * Call once per frame before render().
     *
     * @param frameStats Frame timing statistics
     * @param renderStats Render pass statistics
     */
    void update(const FrameStats& frameStats, const MainRenderPassStats& renderStats);

    /**
     * Update stats data directly.
     * @param stats Stats data to display
     */
    void update(const StatsData& stats);

    /**
     * Render the stats overlay.
     *
     * Should be called during the UI overlay phase, after scene rendering.
     * Uses SDL_GPU_LOADOP_LOAD to preserve existing framebuffer content.
     *
     * @param cmdBuffer Command buffer for recording
     * @param outputTexture Output texture to render to (swapchain)
     * @param width Texture width
     * @param height Texture height
     * @return true if rendering succeeded
     */
    bool render(
        SDL_GPUCommandBuffer* cmdBuffer,
        SDL_GPUTexture* outputTexture,
        std::uint32_t width,
        std::uint32_t height);

    /**
     * Get current configuration.
     * @return Current configuration
     */
    const StatsOverlayConfig& getConfig() const { return m_config; }

    /**
     * Set configuration.
     * @param config New configuration
     */
    void setConfig(const StatsOverlayConfig& config);

    /**
     * Get the last error message.
     * @return Error string
     */
    const std::string& getLastError() const { return m_lastError; }

    /**
     * Get current stats data (for testing).
     * @return Current stats data
     */
    const StatsData& getStats() const { return m_stats; }

private:
    /**
     * Initialize resources.
     * @return true if initialization succeeded
     */
    bool initialize();

    /**
     * Create GPU resources (pipeline, buffers).
     * @return true if creation succeeded
     */
    bool createResources();

    /**
     * Release all resources.
     */
    void releaseResources();

    /**
     * Load font from system fonts.
     * @return true if font loaded successfully
     */
    bool loadFont();

    /**
     * Create text objects for stats display.
     * @return true if text objects created successfully
     */
    bool createTextObjects();

    /**
     * Update text content with current stats.
     */
    void updateTextContent();

    /**
     * Format stats line.
     * @param label Label text
     * @param value Value to display
     * @param unit Unit suffix
     * @return Formatted string
     */
    static std::string formatLine(const char* label, float value, const char* unit);
    static std::string formatLine(const char* label, std::uint32_t value, const char* unit);
    static std::string formatLine(const char* label, std::uint64_t value, const char* unit);

    GPUDevice* m_device = nullptr;
    Window* m_window = nullptr;

    // Configuration
    StatsOverlayConfig m_config;
    bool m_enabled = false;

    // Current stats
    StatsData m_stats;

    // Text rendering
    TTF_TextEngine* m_textEngine = nullptr;
    TTF_Font* m_font = nullptr;

    // Text objects for each line
    static constexpr int LINE_COUNT = 4;  // FPS, Frame time, Draw calls, Triangles
    std::array<TTF_Text*, LINE_COUNT> m_textObjects = {};
    std::array<std::string, LINE_COUNT> m_textStrings = {};

    // Background quad pipeline
    SDL_GPUGraphicsPipeline* m_bgPipeline = nullptr;
    SDL_GPUBuffer* m_bgVertexBuffer = nullptr;
    SDL_GPUTransferBuffer* m_bgTransferBuffer = nullptr;

    // Sampler for text atlas
    SDL_GPUSampler* m_textSampler = nullptr;

    // Swapchain format (cached for pipeline creation)
    SDL_GPUTextureFormat m_swapchainFormat = SDL_GPU_TEXTUREFORMAT_INVALID;

    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_STATS_OVERLAY_H
