/**
 * @file ZoomController.h
 * @brief Zoom controller with cursor-centering and smooth interpolation.
 *
 * Implements zoom controls for the camera system:
 * - Mouse wheel adjusts camera distance (perspective projection)
 * - Zoom centers on cursor position (focus point adjusts to keep cursor world-point stable)
 * - Map-size-aware zoom range (wider range for larger maps)
 * - Smooth interpolation with perceptually consistent zoom speed
 * - Soft boundaries with deceleration at zoom limits
 *
 * Resource ownership: None (pure logic, no GPU/SDL resources).
 */

#ifndef SIMS3000_INPUT_ZOOMCONTROLLER_H
#define SIMS3000_INPUT_ZOOMCONTROLLER_H

#include "sims3000/render/CameraState.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace sims3000 {

// Forward declarations
class InputSystem;

// ============================================================================
// Zoom Configuration
// ============================================================================

/**
 * @struct ZoomConfig
 * @brief Configuration for zoom behavior.
 */
struct ZoomConfig {
    // Distance limits (map-size-aware)
    float minDistance = CameraConfig::DISTANCE_MIN;   ///< Minimum camera distance (closest zoom)
    float maxDistance = CameraConfig::DISTANCE_MAX;   ///< Maximum camera distance (furthest zoom)

    // Zoom speed
    float zoomSpeed = 0.15f;         ///< Zoom factor per wheel notch (logarithmic)
    float smoothingFactor = 12.0f;   ///< Interpolation smoothing (higher = faster)

    // Soft boundary configuration
    float softBoundaryStart = 0.1f;  ///< Fraction of range where soft boundary begins (0.1 = 10%)
    float softBoundaryPower = 2.0f;  ///< Exponent for deceleration curve

    // Cursor centering
    bool centerOnCursor = true;      ///< Enable zoom-to-cursor behavior

    /**
     * @brief Configure zoom range based on map size.
     *
     * Larger maps need a wider zoom range to navigate effectively.
     *
     * @param mapSize Map dimension (128, 256, or 512).
     */
    void configureForMapSize(int mapSize);

    /**
     * @brief Get default config for small maps (128x128).
     */
    static ZoomConfig defaultSmall();

    /**
     * @brief Get default config for medium maps (256x256).
     */
    static ZoomConfig defaultMedium();

    /**
     * @brief Get default config for large maps (512x512).
     */
    static ZoomConfig defaultLarge();
};

// ============================================================================
// Zoom Controller
// ============================================================================

/**
 * @class ZoomController
 * @brief Controls camera zoom with smooth interpolation and cursor-centering.
 *
 * Usage:
 * @code
 *   ZoomController zoom;
 *
 *   // In input processing:
 *   zoom.handleInput(input, cameraState, viewProjection, windowWidth, windowHeight);
 *
 *   // In update loop:
 *   zoom.update(deltaTime, cameraState);
 * @endcode
 */
class ZoomController {
public:
    /**
     * @brief Construct zoom controller with default configuration.
     */
    ZoomController();

    /**
     * @brief Construct zoom controller with custom configuration.
     * @param config Zoom configuration.
     */
    explicit ZoomController(const ZoomConfig& config);

    ~ZoomController() = default;

    // Non-copyable (stateful controller)
    ZoomController(const ZoomController&) = delete;
    ZoomController& operator=(const ZoomController&) = delete;

    // Movable
    ZoomController(ZoomController&&) = default;
    ZoomController& operator=(ZoomController&&) = default;

    // ========================================================================
    // Input Handling
    // ========================================================================

    /**
     * @brief Handle input and calculate zoom target.
     *
     * Reads mouse wheel input and calculates the target distance and
     * focus point adjustment for zoom-to-cursor behavior.
     *
     * @param input Input system to read wheel delta from.
     * @param cameraState Current camera state.
     * @param viewProjection Current view-projection matrix.
     * @param windowWidth Window width in pixels.
     * @param windowHeight Window height in pixels.
     * @return true if zoom input was processed (wheel was moved).
     */
    bool handleInput(
        const InputSystem& input,
        const CameraState& cameraState,
        const glm::mat4& viewProjection,
        float windowWidth,
        float windowHeight);

    /**
     * @brief Handle zoom input with explicit cursor position.
     *
     * Overload for cases where cursor position is known externally.
     *
     * @param wheelDelta Mouse wheel scroll amount (positive = zoom in).
     * @param cursorX Cursor X position in pixels.
     * @param cursorY Cursor Y position in pixels.
     * @param cameraState Current camera state.
     * @param viewProjection Current view-projection matrix.
     * @param windowWidth Window width in pixels.
     * @param windowHeight Window height in pixels.
     * @return true if zoom input was processed.
     */
    bool handleZoom(
        float wheelDelta,
        float cursorX,
        float cursorY,
        const CameraState& cameraState,
        const glm::mat4& viewProjection,
        float windowWidth,
        float windowHeight);

    // ========================================================================
    // Update
    // ========================================================================

    /**
     * @brief Update zoom interpolation.
     *
     * Smoothly interpolates camera distance and focus point toward
     * target values. Call every frame.
     *
     * @param deltaTime Frame delta time in seconds.
     * @param cameraState Camera state to modify (distance and focus_point).
     */
    void update(float deltaTime, CameraState& cameraState);

    // ========================================================================
    // Direct Control
    // ========================================================================

    /**
     * @brief Set target distance directly (bypasses input handling).
     *
     * Useful for programmatic zoom changes (e.g., "zoom to fit" feature).
     * The controller will smoothly interpolate to this distance.
     *
     * @param distance Target camera distance.
     */
    void setTargetDistance(float distance);

    /**
     * @brief Set distance immediately (no interpolation).
     *
     * Snaps the camera to the specified distance without animation.
     *
     * @param distance Camera distance to set.
     * @param cameraState Camera state to modify.
     */
    void setDistanceImmediate(float distance, CameraState& cameraState);

    /**
     * @brief Reset zoom state.
     *
     * Clears any pending zoom animation and resets to current camera state.
     *
     * @param cameraState Current camera state to sync with.
     */
    void reset(const CameraState& cameraState);

    // ========================================================================
    // State Query
    // ========================================================================

    /**
     * @brief Check if zoom animation is in progress.
     * @return true if interpolating toward target distance.
     */
    bool isZooming() const;

    /**
     * @brief Get current target distance.
     * @return Target distance being interpolated toward.
     */
    float getTargetDistance() const { return m_targetDistance; }

    /**
     * @brief Get current target focus point.
     * @return Target focus point being interpolated toward.
     */
    const glm::vec3& getTargetFocusPoint() const { return m_targetFocusPoint; }

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Get current configuration.
     * @return Current zoom configuration.
     */
    const ZoomConfig& getConfig() const { return m_config; }

    /**
     * @brief Set configuration.
     * @param config New zoom configuration.
     */
    void setConfig(const ZoomConfig& config);

    /**
     * @brief Set distance limits.
     *
     * Convenience method to adjust zoom range without replacing entire config.
     *
     * @param minDistance Minimum camera distance.
     * @param maxDistance Maximum camera distance.
     */
    void setDistanceLimits(float minDistance, float maxDistance);

    /**
     * @brief Configure for map size.
     *
     * Adjusts zoom limits based on map dimensions.
     *
     * @param mapSize Map dimension (128, 256, or 512).
     */
    void configureForMapSize(int mapSize);

private:
    /**
     * @brief Apply soft boundaries to zoom delta.
     *
     * Reduces zoom speed near the limits for a smooth deceleration effect.
     *
     * @param currentDistance Current camera distance.
     * @param desiredDistance Desired target distance.
     * @return Adjusted target distance with soft boundary applied.
     */
    float applySoftBoundary(float currentDistance, float desiredDistance) const;

    /**
     * @brief Calculate focus point adjustment for zoom-to-cursor.
     *
     * When zooming, the focus point is adjusted so that the world point
     * under the cursor remains stationary on screen.
     *
     * @param currentFocus Current focus point.
     * @param cursorWorldPos World position under cursor.
     * @param currentDistance Current camera distance.
     * @param targetDistance Target camera distance.
     * @return Adjusted focus point.
     */
    glm::vec3 calculateFocusAdjustment(
        const glm::vec3& currentFocus,
        const glm::vec3& cursorWorldPos,
        float currentDistance,
        float targetDistance) const;

    ZoomConfig m_config;

    // Target state for interpolation
    float m_targetDistance = CameraConfig::DISTANCE_DEFAULT;
    glm::vec3 m_targetFocusPoint{0.0f};

    // Current interpolated state (separate from camera state for smooth updates)
    float m_currentDistance = CameraConfig::DISTANCE_DEFAULT;
    glm::vec3 m_currentFocusPoint{0.0f};

    // Threshold for considering zoom complete
    static constexpr float ZOOM_COMPLETE_THRESHOLD = 0.01f;
};

} // namespace sims3000

#endif // SIMS3000_INPUT_ZOOMCONTROLLER_H
