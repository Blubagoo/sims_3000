/**
 * @file CursorRenderer.h
 * @brief Renderer for other players' cursors in multiplayer.
 *
 * Renders cursor indicators at other players' world positions to show
 * their activity and presence. Cursors are rendered as:
 * - 3D cone/arrow indicators pointing down at the world position
 * - Colored by faction color for player identification
 * - Pulsing glow effect to make them visible
 * - Fading out when stale (player idle or disconnected)
 *
 * Cursors are rendered in the UIWorld layer to always be visible on top
 * of the scene, using the worldToScreen projection for positioning.
 *
 * Resource ownership:
 * - CursorRenderer does NOT own the ICursorSync provider
 * - Caller must ensure ICursorSync outlives CursorRenderer
 *
 * @see ICursorSync for cursor data source
 * @see PlayerCursor for cursor data structure
 */

#ifndef SIMS3000_RENDER_CURSORRENDERER_H
#define SIMS3000_RENDER_CURSORRENDERER_H

#include "sims3000/render/PlayerCursor.h"
#include "sims3000/render/ScreenToWorld.h"
#include "sims3000/sync/ICursorSync.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace sims3000 {

// Forward declarations
class GPUDevice;
struct CameraState;

/**
 * @struct CursorRenderData
 * @brief Data prepared for rendering a single cursor.
 *
 * Combines cursor data with rendering information (model matrix,
 * colors, visibility).
 */
struct CursorRenderData {
    /// Model matrix for cursor transform
    glm::mat4 model_matrix{1.0f};

    /// Tint color (faction color)
    glm::vec4 tint_color{1.0f};

    /// Emissive color (glow)
    glm::vec4 emissive_color{0.0f};

    /// Whether to render this cursor
    bool visible = true;

    /// Player ID (for debug/label purposes)
    PlayerID player_id = 0;
};

/**
 * @class CursorRenderer
 * @brief Renders other players' cursor indicators.
 *
 * Queries ICursorSync for cursor positions and renders them as 3D
 * indicators. Handles:
 * - Position updates from sync
 * - Pulse animation
 * - Staleness fading
 * - Frustum culling (optional)
 */
class CursorRenderer {
public:
    /**
     * @brief Construct a cursor renderer.
     * @param cursorSync Cursor sync provider (not owned).
     * @param config Cursor indicator configuration.
     */
    explicit CursorRenderer(
        ICursorSync* cursorSync = nullptr,
        const CursorIndicatorConfig& config = {});

    ~CursorRenderer() = default;

    // Non-copyable
    CursorRenderer(const CursorRenderer&) = delete;
    CursorRenderer& operator=(const CursorRenderer&) = delete;

    // Movable
    CursorRenderer(CursorRenderer&&) = default;
    CursorRenderer& operator=(CursorRenderer&&) = default;

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * @brief Set the cursor sync provider.
     * @param cursorSync Cursor sync provider (not owned).
     */
    void setCursorSync(ICursorSync* cursorSync) { m_cursorSync = cursorSync; }

    /**
     * @brief Get the current configuration.
     * @return Configuration reference.
     */
    const CursorIndicatorConfig& getConfig() const { return m_config; }

    /**
     * @brief Set the configuration.
     * @param config New configuration.
     */
    void setConfig(const CursorIndicatorConfig& config) { m_config = config; }

    // =========================================================================
    // Update
    // =========================================================================

    /**
     * @brief Update cursor states (animation, staleness).
     *
     * Call this once per frame to update:
     * - Pulse animation phase
     * - Staleness timers for local cursor state
     *
     * @param deltaTime Time since last frame in seconds.
     */
    void update(float deltaTime);

    /**
     * @brief Update local cursor position from input.
     *
     * Called when the mouse moves to report new cursor position.
     * The position is forwarded to ICursorSync for network broadcast.
     *
     * @param worldPosition World-space cursor position.
     */
    void updateLocalCursorPosition(const glm::vec3& worldPosition);

    // =========================================================================
    // Rendering
    // =========================================================================

    /**
     * @brief Prepare cursor data for rendering.
     *
     * Queries ICursorSync for current cursor positions and prepares
     * render data (transforms, colors) for each cursor.
     *
     * @param cameraState Current camera state for view calculations.
     * @param viewProjection View-projection matrix for visibility checks.
     * @param windowWidth Window width for screen projection.
     * @param windowHeight Window height for screen projection.
     * @return Vector of cursor render data ready for rendering.
     */
    std::vector<CursorRenderData> prepareCursors(
        const CameraState& cameraState,
        const glm::mat4& viewProjection,
        float windowWidth,
        float windowHeight);

    /**
     * @brief Get the number of visible cursors.
     * @return Number of cursors that will be rendered.
     */
    std::uint32_t getVisibleCursorCount() const { return m_visibleCursorCount; }

    /**
     * @brief Check if cursor rendering is enabled.
     * @return true if cursors will be rendered.
     */
    bool isEnabled() const { return m_enabled && m_cursorSync != nullptr; }

    /**
     * @brief Enable or disable cursor rendering.
     * @param enabled Whether to render cursors.
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

private:
    /**
     * @brief Calculate model matrix for a cursor at the given position.
     *
     * @param worldPosition World position of the cursor.
     * @param scale Scale factor (including pulse animation).
     * @return Model matrix for the cursor indicator.
     */
    glm::mat4 calculateCursorTransform(
        const glm::vec3& worldPosition,
        float scale) const;

    /**
     * @brief Calculate cursor color with staleness fade.
     *
     * @param cursor Cursor data.
     * @return Tint color with alpha based on staleness.
     */
    glm::vec4 calculateCursorColor(const PlayerCursor& cursor) const;

    /**
     * @brief Calculate emissive color for cursor glow.
     *
     * @param cursor Cursor data.
     * @param pulsePhase Current pulse animation phase.
     * @return Emissive color with intensity.
     */
    glm::vec4 calculateEmissiveColor(
        const PlayerCursor& cursor,
        float pulsePhase) const;

    ICursorSync* m_cursorSync = nullptr;
    CursorIndicatorConfig m_config;

    float m_animationTime = 0.0f;  ///< Accumulated animation time
    std::uint32_t m_visibleCursorCount = 0;
    bool m_enabled = true;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_CURSORRENDERER_H
