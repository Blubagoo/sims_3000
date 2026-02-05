#include "camera.h"
#include <cmath>

// Isometric angles
// Classic isometric: 30 degrees elevation, 45 degrees rotation around Y
static constexpr float ISOMETRIC_ELEVATION_DEGREES = 30.0f;
static constexpr float ISOMETRIC_ROTATION_DEGREES = 45.0f;

// Convert degrees to radians
static constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;

Camera::Camera()
    : m_position(0.0f, 0.0f, 0.0f)
    , m_target(0.0f, 0.0f, 0.0f)
    , m_up(0.0f, 1.0f, 0.0f)
    , m_orthoSize(10.0f)           // Default view shows 20 units vertically
    , m_aspectRatio(4.0f / 3.0f)   // Default 800x600 -> 4:3
    , m_nearPlane(-1000.0f)        // Negative for orthographic (objects behind camera)
    , m_farPlane(1000.0f)
    , m_viewMatrix(1.0f)
    , m_projectionMatrix(1.0f)
    , m_viewProjectionMatrix(1.0f)
    , m_dirty(true)
{
    // Set up default isometric view looking at origin
    SetIsometricView(glm::vec3(0.0f, 0.0f, 0.0f), 20.0f);
}

void Camera::SetPosition(const glm::vec3& position)
{
    m_position = position;
    m_dirty = true;
}

void Camera::SetTarget(const glm::vec3& target)
{
    m_target = target;
    m_dirty = true;
}

glm::mat4 Camera::GetViewMatrix() const
{
    if (m_dirty) {
        UpdateMatrices();
    }
    return m_viewMatrix;
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    if (m_dirty) {
        UpdateMatrices();
    }
    return m_projectionMatrix;
}

glm::mat4 Camera::GetViewProjectionMatrix() const
{
    if (m_dirty) {
        UpdateMatrices();
    }
    return m_viewProjectionMatrix;
}

void Camera::SetOrthoSize(float size)
{
    if (size > 0.0f) {
        m_orthoSize = size;
        m_dirty = true;
    }
}

float Camera::GetOrthoSize() const
{
    return m_orthoSize;
}

void Camera::SetAspectRatio(float aspect)
{
    if (aspect > 0.0f) {
        m_aspectRatio = aspect;
        m_dirty = true;
    }
}

float Camera::GetAspectRatio() const
{
    return m_aspectRatio;
}

glm::vec3 Camera::GetPosition() const
{
    return m_position;
}

glm::vec3 Camera::GetTarget() const
{
    return m_target;
}

void Camera::SetClipPlanes(float nearPlane, float farPlane)
{
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_dirty = true;
}

void Camera::SetIsometricView(const glm::vec3& target, float distance)
{
    m_target = target;

    // Calculate isometric camera position
    // Elevation angle: 30 degrees above horizontal
    // Rotation angle: 45 degrees around Y axis
    float elevationRad = ISOMETRIC_ELEVATION_DEGREES * DEG_TO_RAD;
    float rotationRad = ISOMETRIC_ROTATION_DEGREES * DEG_TO_RAD;

    // Calculate position offset from target
    // First, get horizontal distance (projection onto XZ plane)
    float horizontalDist = distance * std::cos(elevationRad);
    float verticalDist = distance * std::sin(elevationRad);

    // Then apply rotation around Y axis
    float offsetX = horizontalDist * std::sin(rotationRad);
    float offsetZ = horizontalDist * std::cos(rotationRad);

    m_position = target + glm::vec3(offsetX, verticalDist, offsetZ);
    m_up = glm::vec3(0.0f, 1.0f, 0.0f);

    m_dirty = true;
}

void Camera::UpdateMatrices() const
{
    // View matrix - transforms world space to camera space
    m_viewMatrix = glm::lookAt(m_position, m_target, m_up);

    // Orthographic projection matrix
    // orthoSize is half the height, width is calculated from aspect ratio
    float halfHeight = m_orthoSize;
    float halfWidth = m_orthoSize * m_aspectRatio;

    m_projectionMatrix = glm::ortho(
        -halfWidth,   // left
        halfWidth,    // right
        -halfHeight,  // bottom
        halfHeight,   // top
        m_nearPlane,
        m_farPlane
    );

    // Combined view-projection matrix
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;

    m_dirty = false;
}
