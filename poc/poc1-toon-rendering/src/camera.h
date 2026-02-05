#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * Camera - Isometric camera for city builder view
 *
 * Provides an orthographic isometric view with:
 * - 30 degree elevation angle
 * - 45 degree rotation (classic isometric)
 * - Orthographic projection for consistent object sizing
 */
class Camera {
public:
    /**
     * Construct a camera with default isometric settings.
     * Default looks at origin from an elevated isometric angle.
     */
    Camera();

    /**
     * Set the camera position in world space.
     * The camera will look from this position toward the target.
     *
     * @param position Camera position in world coordinates
     */
    void SetPosition(const glm::vec3& position);

    /**
     * Set the point the camera looks at.
     *
     * @param target Target position in world coordinates
     */
    void SetTarget(const glm::vec3& target);

    /**
     * Get the view matrix (world to camera space).
     * @return View transformation matrix
     */
    glm::mat4 GetViewMatrix() const;

    /**
     * Get the orthographic projection matrix.
     * @return Projection matrix
     */
    glm::mat4 GetProjectionMatrix() const;

    /**
     * Get the combined view-projection matrix.
     * @return View * Projection matrix for transforming world coordinates to clip space
     */
    glm::mat4 GetViewProjectionMatrix() const;

    /**
     * Set the orthographic size (half-height of the view in world units).
     * Affects the zoom level - smaller values zoom in.
     *
     * @param size Orthographic half-height (must be positive)
     */
    void SetOrthoSize(float size);

    /**
     * Get the current orthographic size.
     * @return Current ortho half-height
     */
    float GetOrthoSize() const;

    /**
     * Set the aspect ratio (width / height).
     * @param aspect Aspect ratio
     */
    void SetAspectRatio(float aspect);

    /**
     * Get the current aspect ratio.
     * @return Current aspect ratio
     */
    float GetAspectRatio() const;

    /**
     * Get the current camera position.
     * @return Camera position in world coordinates
     */
    glm::vec3 GetPosition() const;

    /**
     * Get the current target position.
     * @return Target position in world coordinates
     */
    glm::vec3 GetTarget() const;

    /**
     * Set near and far clipping planes.
     * @param nearPlane Near clipping distance
     * @param farPlane Far clipping distance
     */
    void SetClipPlanes(float nearPlane, float farPlane);

    /**
     * Configure the camera for classic isometric view.
     * Sets up 30 degree elevation and 45 degree rotation from target.
     *
     * @param target The point to look at
     * @param distance Distance from target
     */
    void SetIsometricView(const glm::vec3& target, float distance);

private:
    void UpdateMatrices() const;

    glm::vec3 m_position;
    glm::vec3 m_target;
    glm::vec3 m_up;

    float m_orthoSize;    // Half-height of orthographic view
    float m_aspectRatio;  // Width / Height
    float m_nearPlane;
    float m_farPlane;

    // Cached matrices (mutable for lazy evaluation)
    mutable glm::mat4 m_viewMatrix;
    mutable glm::mat4 m_projectionMatrix;
    mutable glm::mat4 m_viewProjectionMatrix;
    mutable bool m_dirty;
};

#endif // CAMERA_H
