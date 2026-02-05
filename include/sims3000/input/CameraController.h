/**
 * @file CameraController.h
 * @brief Camera control using input actions.
 */

#ifndef SIMS3000_INPUT_CAMERACONTROLLER_H
#define SIMS3000_INPUT_CAMERACONTROLLER_H

#include <glm/glm.hpp>

namespace sims3000 {

// Forward declarations
class InputSystem;
class ISimulationTime;

/**
 * @class CameraController
 * @brief Controls camera movement based on input.
 *
 * Provides smooth camera panning and zooming using
 * the action mapping system.
 */
class CameraController {
public:
    CameraController();
    ~CameraController() = default;

    /**
     * Update camera position based on input.
     * @param input Input system for action queries
     * @param deltaTime Frame delta in seconds
     */
    void update(const InputSystem& input, float deltaTime);

    /**
     * Get camera position.
     */
    const glm::vec3& getPosition() const;

    /**
     * Set camera position.
     */
    void setPosition(const glm::vec3& pos);

    /**
     * Get zoom level (1.0 = default).
     */
    float getZoom() const;

    /**
     * Set zoom level.
     */
    void setZoom(float zoom);

    /**
     * Get rotation angle in degrees.
     */
    float getRotation() const;

    /**
     * Set rotation angle.
     */
    void setRotation(float degrees);

    /**
     * Toggle debug camera mode.
     * In debug mode, camera has free movement.
     */
    void toggleDebugMode();

    /**
     * Check if debug mode is active.
     */
    bool isDebugMode() const;

    /**
     * Set pan speed.
     * @param speed Units per second
     */
    void setPanSpeed(float speed);

    /**
     * Set zoom speed.
     * @param speed Zoom factor per second
     */
    void setZoomSpeed(float speed);

    /**
     * Set rotation speed.
     * @param speed Degrees per second
     */
    void setRotationSpeed(float speed);

    /**
     * Set zoom limits.
     * @param minZoom Minimum zoom level
     * @param maxZoom Maximum zoom level
     */
    void setZoomLimits(float minZoom, float maxZoom);

    /**
     * Get view matrix for rendering.
     */
    glm::mat4 getViewMatrix() const;

private:
    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
    glm::vec3 m_targetPosition{0.0f, 0.0f, 0.0f};
    float m_zoom = 1.0f;
    float m_targetZoom = 1.0f;
    float m_rotation = 0.0f;
    float m_targetRotation = 0.0f;

    float m_panSpeed = 500.0f;
    float m_zoomSpeed = 2.0f;
    float m_rotationSpeed = 90.0f;
    float m_smoothing = 10.0f;

    float m_minZoom = 0.25f;
    float m_maxZoom = 4.0f;

    bool m_debugMode = false;
};

} // namespace sims3000

#endif // SIMS3000_INPUT_CAMERACONTROLLER_H
