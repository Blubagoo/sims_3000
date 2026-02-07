/**
 * @file PanController.h
 * @brief Pan controller with keyboard, mouse drag, and edge scrolling support.
 *
 * Implements camera panning for the camera system:
 * - WASD/Arrow keys pan the camera
 * - Right mouse button drag pans the camera
 * - Edge-of-screen scrolling (toggleable)
 * - Pan direction is camera-orientation-relative (projected onto ground plane)
 * - Pan speed scales with zoom level (faster when zoomed out)
 * - Smooth momentum with ease-out on stop
 *
 * Resource ownership: None (pure logic, no GPU/SDL resources).
 */

#ifndef SIMS3000_INPUT_PANCONTROLLER_H
#define SIMS3000_INPUT_PANCONTROLLER_H

#include "sims3000/render/CameraState.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace sims3000 {

// Forward declarations
class InputSystem;

// ============================================================================
// Pan Configuration
// ============================================================================

/**
 * @struct PanConfig
 * @brief Configuration for pan behavior.
 */
struct PanConfig {
    // Base pan speed
    float basePanSpeed = 40.0f;          ///< Base units per second at default zoom

    // Zoom scaling
    float zoomSpeedMultiplier = 1.0f;    ///< Multiplier for zoom-based speed scaling
    float minZoomSpeedFactor = 0.3f;     ///< Minimum speed factor at closest zoom
    float maxZoomSpeedFactor = 3.0f;     ///< Maximum speed factor at furthest zoom

    // Mouse drag settings
    float dragSensitivity = 0.5f;        ///< World units per pixel of mouse drag
    bool invertDragY = false;            ///< Invert Y axis for drag (pull vs push)

    // Momentum / smoothing
    float smoothingFactor = 8.0f;        ///< Interpolation smoothing (higher = faster)
    float momentumDecay = 5.0f;          ///< Velocity decay rate when input stops
    bool enableMomentum = true;          ///< Enable momentum/ease-out on stop

    // Edge scrolling
    bool enableEdgeScrolling = true;     ///< Enable edge-of-screen scrolling
    int edgeScrollMargin = 10;           ///< Pixels from edge to trigger scrolling
    float edgeScrollSpeed = 1.0f;        ///< Speed multiplier for edge scrolling

    /**
     * @brief Configure pan speed based on map size.
     *
     * Larger maps may need faster pan speeds for navigation.
     *
     * @param mapSize Map dimension (128, 256, or 512).
     */
    void configureForMapSize(int mapSize);

    /**
     * @brief Get default config for small maps (128x128).
     */
    static PanConfig defaultSmall();

    /**
     * @brief Get default config for medium maps (256x256).
     */
    static PanConfig defaultMedium();

    /**
     * @brief Get default config for large maps (512x512).
     */
    static PanConfig defaultLarge();
};

// ============================================================================
// Pan Controller
// ============================================================================

/**
 * @class PanController
 * @brief Controls camera panning with smooth momentum and multiple input methods.
 *
 * Supports three input methods:
 * 1. Keyboard (WASD/arrows) - pan direction is camera-orientation-relative
 * 2. Right mouse drag - drag to pan the view
 * 3. Edge scrolling - move cursor to screen edge to pan
 *
 * Pan direction is calculated from camera yaw, projected onto the ground plane.
 * Pan speed scales with zoom level (faster when zoomed out, slower when close).
 *
 * Usage:
 * @code
 *   PanController pan;
 *
 *   // In input processing:
 *   pan.handleInput(input, cameraState, windowWidth, windowHeight);
 *
 *   // In update loop:
 *   pan.update(deltaTime, cameraState);
 * @endcode
 */
class PanController {
public:
    /**
     * @brief Construct pan controller with default configuration.
     */
    PanController();

    /**
     * @brief Construct pan controller with custom configuration.
     * @param config Pan configuration.
     */
    explicit PanController(const PanConfig& config);

    ~PanController() = default;

    // Non-copyable (stateful controller)
    PanController(const PanController&) = delete;
    PanController& operator=(const PanController&) = delete;

    // Movable
    PanController(PanController&&) = default;
    PanController& operator=(PanController&&) = default;

    // ========================================================================
    // Input Handling
    // ========================================================================

    /**
     * @brief Handle all pan input sources.
     *
     * Processes keyboard, mouse drag, and edge scrolling input.
     * Calculates pan velocity in world space.
     *
     * @param input Input system for action/mouse queries.
     * @param cameraState Current camera state (for yaw/zoom).
     * @param windowWidth Window width in pixels.
     * @param windowHeight Window height in pixels.
     * @return true if any pan input was processed.
     */
    bool handleInput(
        const InputSystem& input,
        const CameraState& cameraState,
        float windowWidth,
        float windowHeight);

    /**
     * @brief Handle keyboard pan input only.
     *
     * Uses action bindings (PAN_UP, PAN_DOWN, PAN_LEFT, PAN_RIGHT).
     *
     * @param input Input system for action queries.
     * @param cameraState Current camera state.
     * @return true if any keyboard pan input was active.
     */
    bool handleKeyboardInput(
        const InputSystem& input,
        const CameraState& cameraState);

    /**
     * @brief Handle mouse drag pan input.
     *
     * Right mouse button drag pans the camera.
     *
     * @param input Input system for mouse state.
     * @param cameraState Current camera state.
     * @return true if mouse drag panning is active.
     */
    bool handleMouseDragInput(
        const InputSystem& input,
        const CameraState& cameraState);

    /**
     * @brief Handle edge-of-screen scrolling.
     *
     * @param input Input system for mouse position.
     * @param cameraState Current camera state.
     * @param windowWidth Window width in pixels.
     * @param windowHeight Window height in pixels.
     * @return true if edge scrolling is active.
     */
    bool handleEdgeScrollInput(
        const InputSystem& input,
        const CameraState& cameraState,
        float windowWidth,
        float windowHeight);

    // ========================================================================
    // Update
    // ========================================================================

    /**
     * @brief Update pan interpolation and apply to camera.
     *
     * Smoothly interpolates camera focus point using current velocity.
     * Applies momentum decay when no input is active. Call every frame.
     *
     * @param deltaTime Frame delta time in seconds.
     * @param cameraState Camera state to modify (focus_point).
     */
    void update(float deltaTime, CameraState& cameraState);

    // ========================================================================
    // Direct Control
    // ========================================================================

    /**
     * @brief Set pan velocity directly.
     *
     * Useful for programmatic camera movement. Velocity is in world units/sec.
     *
     * @param velocity Pan velocity in world space (X, Z components).
     */
    void setVelocity(const glm::vec2& velocity);

    /**
     * @brief Add to current pan velocity.
     *
     * @param velocityDelta Velocity to add.
     */
    void addVelocity(const glm::vec2& velocityDelta);

    /**
     * @brief Stop all panning immediately.
     *
     * Clears velocity without momentum decay.
     */
    void stop();

    /**
     * @brief Reset pan state.
     *
     * Clears velocity and syncs with camera state.
     *
     * @param cameraState Current camera state to sync with.
     */
    void reset(const CameraState& cameraState);

    // ========================================================================
    // State Query
    // ========================================================================

    /**
     * @brief Check if panning is in progress.
     * @return true if there is active pan velocity.
     */
    bool isPanning() const;

    /**
     * @brief Check if keyboard pan input is active.
     * @return true if keyboard pan keys are held.
     */
    bool isKeyboardPanning() const { return m_isKeyboardPanning; }

    /**
     * @brief Check if mouse drag pan is active.
     * @return true if right mouse drag is active.
     */
    bool isMouseDragging() const { return m_isMouseDragging; }

    /**
     * @brief Check if edge scrolling is active.
     * @return true if cursor is in edge scroll zone.
     */
    bool isEdgeScrolling() const { return m_isEdgeScrolling; }

    /**
     * @brief Get current pan velocity.
     * @return Current velocity in world units per second.
     */
    const glm::vec2& getVelocity() const { return m_velocity; }

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Get current configuration.
     * @return Current pan configuration.
     */
    const PanConfig& getConfig() const { return m_config; }

    /**
     * @brief Set configuration.
     * @param config New pan configuration.
     */
    void setConfig(const PanConfig& config);

    /**
     * @brief Enable or disable edge scrolling.
     * @param enable true to enable edge scrolling.
     */
    void setEdgeScrollingEnabled(bool enable);

    /**
     * @brief Check if edge scrolling is enabled.
     * @return true if edge scrolling is enabled.
     */
    bool isEdgeScrollingEnabled() const { return m_config.enableEdgeScrolling; }

    /**
     * @brief Configure for map size.
     *
     * Adjusts pan speed based on map dimensions.
     *
     * @param mapSize Map dimension (128, 256, or 512).
     */
    void configureForMapSize(int mapSize);

private:
    /**
     * @brief Calculate pan speed based on current zoom level.
     *
     * @param distance Current camera distance.
     * @return Speed multiplier for pan operations.
     */
    float calculateZoomSpeedFactor(float distance) const;

    /**
     * @brief Calculate camera-relative pan direction.
     *
     * Converts input direction to world-space direction based on camera yaw.
     *
     * @param inputDir Input direction in screen space (right = +X, up = -Y).
     * @param yawDegrees Camera yaw in degrees.
     * @return World-space pan direction (X, Z components).
     */
    glm::vec2 calculateWorldPanDirection(const glm::vec2& inputDir, float yawDegrees) const;

    /**
     * @brief Clamp focus point to map bounds if needed.
     *
     * @param focusPoint Focus point to clamp.
     * @return Clamped focus point.
     */
    glm::vec3 clampToMapBounds(const glm::vec3& focusPoint) const;

    PanConfig m_config;

    // Current velocity (world units per second, X and Z components)
    glm::vec2 m_velocity{0.0f};

    // Input state tracking
    glm::vec2 m_inputDirection{0.0f};  // Accumulated input direction
    bool m_isKeyboardPanning = false;
    bool m_isMouseDragging = false;
    bool m_isEdgeScrolling = false;
    bool m_hasActiveInput = false;

    // Mouse drag state
    int m_lastDragDeltaX = 0;
    int m_lastDragDeltaY = 0;

    // Threshold for considering pan complete
    static constexpr float PAN_VELOCITY_THRESHOLD = 0.01f;
};

} // namespace sims3000

#endif // SIMS3000_INPUT_PANCONTROLLER_H
