/**
 * @file CameraController.cpp
 * @brief CameraController implementation.
 */

#include "sims3000/input/CameraController.h"
#include "sims3000/input/InputSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace sims3000 {

CameraController::CameraController() = default;

void CameraController::update(const InputSystem& input, float deltaTime) {
    // Calculate movement direction
    glm::vec3 move{0.0f};

    if (input.isActionDown(Action::PAN_UP))    move.y -= 1.0f;
    if (input.isActionDown(Action::PAN_DOWN))  move.y += 1.0f;
    if (input.isActionDown(Action::PAN_LEFT))  move.x -= 1.0f;
    if (input.isActionDown(Action::PAN_RIGHT)) move.x += 1.0f;

    // Normalize diagonal movement
    if (glm::length(move) > 0.0f) {
        move = glm::normalize(move);
    }

    // Apply movement with rotation
    float rotRad = glm::radians(m_rotation);
    float cos_r = std::cos(rotRad);
    float sin_r = std::sin(rotRad);

    glm::vec3 rotatedMove{
        move.x * cos_r - move.y * sin_r,
        move.x * sin_r + move.y * cos_r,
        0.0f
    };

    m_targetPosition += rotatedMove * m_panSpeed * deltaTime / m_zoom;

    // Handle zoom
    float zoomDelta = 0.0f;
    if (input.isActionDown(Action::ZOOM_IN))  zoomDelta += 1.0f;
    if (input.isActionDown(Action::ZOOM_OUT)) zoomDelta -= 1.0f;

    // Also use mouse wheel
    zoomDelta += input.getMouse().wheelY;

    m_targetZoom *= std::pow(m_zoomSpeed, zoomDelta * deltaTime);
    m_targetZoom = std::clamp(m_targetZoom, m_minZoom, m_maxZoom);

    // Handle rotation
    float rotDelta = 0.0f;
    if (input.isActionDown(Action::ROTATE_CW))  rotDelta += 1.0f;
    if (input.isActionDown(Action::ROTATE_CCW)) rotDelta -= 1.0f;

    m_targetRotation += rotDelta * m_rotationSpeed * deltaTime;

    // Smooth interpolation
    float t = std::min(1.0f, m_smoothing * deltaTime);
    m_position = glm::mix(m_position, m_targetPosition, t);
    m_zoom = glm::mix(m_zoom, m_targetZoom, t);
    m_rotation = glm::mix(m_rotation, m_targetRotation, t);

    // Check for debug camera toggle
    if (input.isActionPressed(Action::DEBUG_CAMERA)) {
        toggleDebugMode();
    }
}

const glm::vec3& CameraController::getPosition() const {
    return m_position;
}

void CameraController::setPosition(const glm::vec3& pos) {
    m_position = pos;
    m_targetPosition = pos;
}

float CameraController::getZoom() const {
    return m_zoom;
}

void CameraController::setZoom(float zoom) {
    m_zoom = std::clamp(zoom, m_minZoom, m_maxZoom);
    m_targetZoom = m_zoom;
}

float CameraController::getRotation() const {
    return m_rotation;
}

void CameraController::setRotation(float degrees) {
    m_rotation = degrees;
    m_targetRotation = degrees;
}

void CameraController::toggleDebugMode() {
    m_debugMode = !m_debugMode;
}

bool CameraController::isDebugMode() const {
    return m_debugMode;
}

void CameraController::setPanSpeed(float speed) {
    m_panSpeed = speed;
}

void CameraController::setZoomSpeed(float speed) {
    m_zoomSpeed = speed;
}

void CameraController::setRotationSpeed(float speed) {
    m_rotationSpeed = speed;
}

void CameraController::setZoomLimits(float minZoom, float maxZoom) {
    m_minZoom = minZoom;
    m_maxZoom = maxZoom;
    m_zoom = std::clamp(m_zoom, m_minZoom, m_maxZoom);
    m_targetZoom = std::clamp(m_targetZoom, m_minZoom, m_maxZoom);
}

glm::mat4 CameraController::getViewMatrix() const {
    // Isometric-style camera
    glm::mat4 view = glm::mat4(1.0f);

    // Apply zoom
    view = glm::scale(view, glm::vec3(m_zoom));

    // Apply rotation around Z axis
    view = glm::rotate(view, glm::radians(m_rotation), glm::vec3(0.0f, 0.0f, 1.0f));

    // Apply translation
    view = glm::translate(view, -m_position);

    return view;
}

} // namespace sims3000
