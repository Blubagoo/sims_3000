/**
 * @file PanController.cpp
 * @brief Pan controller implementation with keyboard, mouse drag, and edge scrolling.
 */

#include "sims3000/input/PanController.h"
#include "sims3000/input/InputSystem.h"
#include <cmath>
#include <algorithm>

namespace sims3000 {

// ============================================================================
// PanConfig Implementation
// ============================================================================

void PanConfig::configureForMapSize(int mapSize) {
    // Larger maps need faster pan speeds for comfortable navigation
    if (mapSize <= 128) {
        basePanSpeed = 40.0f;
    } else if (mapSize <= 256) {
        basePanSpeed = 60.0f;
    } else {
        basePanSpeed = 80.0f;
    }
}

PanConfig PanConfig::defaultSmall() {
    PanConfig config;
    config.configureForMapSize(128);
    return config;
}

PanConfig PanConfig::defaultMedium() {
    PanConfig config;
    config.configureForMapSize(256);
    return config;
}

PanConfig PanConfig::defaultLarge() {
    PanConfig config;
    config.configureForMapSize(512);
    return config;
}

// ============================================================================
// PanController Implementation
// ============================================================================

PanController::PanController()
    : m_config()
    , m_velocity(0.0f)
    , m_inputDirection(0.0f)
    , m_isKeyboardPanning(false)
    , m_isMouseDragging(false)
    , m_isEdgeScrolling(false)
    , m_hasActiveInput(false)
    , m_lastDragDeltaX(0)
    , m_lastDragDeltaY(0)
{
}

PanController::PanController(const PanConfig& config)
    : m_config(config)
    , m_velocity(0.0f)
    , m_inputDirection(0.0f)
    , m_isKeyboardPanning(false)
    , m_isMouseDragging(false)
    , m_isEdgeScrolling(false)
    , m_hasActiveInput(false)
    , m_lastDragDeltaX(0)
    , m_lastDragDeltaY(0)
{
}

bool PanController::handleInput(
    const InputSystem& input,
    const CameraState& cameraState,
    float windowWidth,
    float windowHeight)
{
    // Reset input direction for this frame
    m_inputDirection = glm::vec2(0.0f);
    m_hasActiveInput = false;

    // Process all input sources
    bool keyboardActive = handleKeyboardInput(input, cameraState);
    bool mouseActive = handleMouseDragInput(input, cameraState);
    bool edgeActive = false;

    // Edge scrolling only if not mouse dragging (to avoid conflict)
    if (!mouseActive && m_config.enableEdgeScrolling) {
        edgeActive = handleEdgeScrollInput(input, cameraState, windowWidth, windowHeight);
    }

    m_hasActiveInput = keyboardActive || mouseActive || edgeActive;

    return m_hasActiveInput;
}

bool PanController::handleKeyboardInput(
    const InputSystem& input,
    const CameraState& cameraState)
{
    glm::vec2 keyInput(0.0f);

    // Read action bindings for pan directions
    // In screen space: right = +X, forward (up on screen) = -Y (toward top of screen)
    // We map to the ground plane, so PAN_UP moves "forward" in the camera's view direction
    if (input.isActionDown(Action::PAN_UP))    keyInput.y -= 1.0f;  // Forward (screen up)
    if (input.isActionDown(Action::PAN_DOWN))  keyInput.y += 1.0f;  // Backward (screen down)
    if (input.isActionDown(Action::PAN_LEFT))  keyInput.x -= 1.0f;  // Left
    if (input.isActionDown(Action::PAN_RIGHT)) keyInput.x += 1.0f;  // Right

    m_isKeyboardPanning = (glm::length(keyInput) > 0.001f);

    if (m_isKeyboardPanning) {
        // Normalize diagonal movement
        keyInput = glm::normalize(keyInput);

        // Calculate zoom-based speed factor
        float zoomFactor = calculateZoomSpeedFactor(cameraState.distance);

        // Convert to world-space direction and apply speed
        glm::vec2 worldDir = calculateWorldPanDirection(keyInput, cameraState.yaw);
        m_inputDirection += worldDir * m_config.basePanSpeed * zoomFactor;
    }

    return m_isKeyboardPanning;
}

bool PanController::handleMouseDragInput(
    const InputSystem& input,
    const CameraState& cameraState)
{
    // Check for right mouse button drag
    // Note: Drag state is tracked by InputSystem with threshold
    bool rightDown = input.isMouseButtonDown(MouseButton::Right);

    if (rightDown && input.isDragging()) {
        // Get drag delta from InputSystem
        int dragDeltaX = 0, dragDeltaY = 0;
        input.getDragDelta(dragDeltaX, dragDeltaY);

        // Check if drag is from right button
        // We compute frame delta from stored last values
        int frameDeltaX = dragDeltaX - m_lastDragDeltaX;
        int frameDeltaY = dragDeltaY - m_lastDragDeltaY;

        if (frameDeltaX != 0 || frameDeltaY != 0) {
            m_isMouseDragging = true;

            // Convert pixel delta to world units
            // Drag direction is inverted: dragging right moves view left (pushes the map)
            float dragX = -static_cast<float>(frameDeltaX) * m_config.dragSensitivity;
            float dragY = static_cast<float>(frameDeltaY) * m_config.dragSensitivity;

            if (m_config.invertDragY) {
                dragY = -dragY;
            }

            // Apply zoom factor to drag sensitivity
            float zoomFactor = calculateZoomSpeedFactor(cameraState.distance);

            // Create input direction from drag
            glm::vec2 dragDir(dragX, dragY);

            // Convert to world-space and apply
            glm::vec2 worldDir = calculateWorldPanDirection(dragDir, cameraState.yaw);

            // For drag, we set velocity directly (immediate response)
            // Scale by a factor to make it feel responsive
            float dragSpeed = 60.0f * zoomFactor;  // Pixels to units conversion
            m_inputDirection += worldDir * dragSpeed;
        }

        m_lastDragDeltaX = dragDeltaX;
        m_lastDragDeltaY = dragDeltaY;
    } else {
        // Reset drag tracking when not dragging
        if (m_isMouseDragging) {
            m_lastDragDeltaX = 0;
            m_lastDragDeltaY = 0;
        }
        m_isMouseDragging = false;
    }

    return m_isMouseDragging;
}

bool PanController::handleEdgeScrollInput(
    const InputSystem& input,
    const CameraState& cameraState,
    float windowWidth,
    float windowHeight)
{
    if (!m_config.enableEdgeScrolling) {
        m_isEdgeScrolling = false;
        return false;
    }

    const MouseState& mouse = input.getMouse();
    float mx = static_cast<float>(mouse.x);
    float my = static_cast<float>(mouse.y);

    glm::vec2 edgeDir(0.0f);
    int margin = m_config.edgeScrollMargin;

    // Check each edge
    if (mx < static_cast<float>(margin)) {
        edgeDir.x -= 1.0f;  // Scroll left
    } else if (mx > windowWidth - static_cast<float>(margin)) {
        edgeDir.x += 1.0f;  // Scroll right
    }

    if (my < static_cast<float>(margin)) {
        edgeDir.y -= 1.0f;  // Scroll up (forward)
    } else if (my > windowHeight - static_cast<float>(margin)) {
        edgeDir.y += 1.0f;  // Scroll down (backward)
    }

    m_isEdgeScrolling = (glm::length(edgeDir) > 0.001f);

    if (m_isEdgeScrolling) {
        // Normalize diagonal movement
        if (glm::length(edgeDir) > 1.0f) {
            edgeDir = glm::normalize(edgeDir);
        }

        // Calculate zoom-based speed factor
        float zoomFactor = calculateZoomSpeedFactor(cameraState.distance);

        // Convert to world-space direction and apply speed
        glm::vec2 worldDir = calculateWorldPanDirection(edgeDir, cameraState.yaw);
        m_inputDirection += worldDir * m_config.basePanSpeed * m_config.edgeScrollSpeed * zoomFactor;
    }

    return m_isEdgeScrolling;
}

void PanController::update(float deltaTime, CameraState& cameraState) {
    if (m_hasActiveInput) {
        // Smooth toward input direction velocity
        float t = 1.0f - std::exp(-m_config.smoothingFactor * deltaTime);
        t = std::clamp(t, 0.0f, 1.0f);

        m_velocity = m_velocity + (m_inputDirection - m_velocity) * t;
    } else if (m_config.enableMomentum) {
        // Apply momentum decay when no input
        float decay = std::exp(-m_config.momentumDecay * deltaTime);
        m_velocity *= decay;

        // Zero out very small velocities
        if (glm::length(m_velocity) < PAN_VELOCITY_THRESHOLD) {
            m_velocity = glm::vec2(0.0f);
        }
    } else {
        // No momentum - stop immediately when input stops
        m_velocity = glm::vec2(0.0f);
    }

    // Apply velocity to focus point
    if (glm::length(m_velocity) > PAN_VELOCITY_THRESHOLD) {
        // Velocity is in world units per second (X and Z components)
        // Focus point Y stays constant (ground plane)
        cameraState.focus_point.x += m_velocity.x * deltaTime;
        cameraState.focus_point.z += m_velocity.y * deltaTime;  // Note: velocity.y -> world Z

        // Optionally clamp to map bounds (not implemented, depends on map system)
        // cameraState.focus_point = clampToMapBounds(cameraState.focus_point);
    }
}

void PanController::setVelocity(const glm::vec2& velocity) {
    m_velocity = velocity;
}

void PanController::addVelocity(const glm::vec2& velocityDelta) {
    m_velocity += velocityDelta;
}

void PanController::stop() {
    m_velocity = glm::vec2(0.0f);
    m_inputDirection = glm::vec2(0.0f);
    m_hasActiveInput = false;
}

void PanController::reset(const CameraState& cameraState) {
    (void)cameraState;  // Camera state not needed for reset, but kept for API consistency
    m_velocity = glm::vec2(0.0f);
    m_inputDirection = glm::vec2(0.0f);
    m_isKeyboardPanning = false;
    m_isMouseDragging = false;
    m_isEdgeScrolling = false;
    m_hasActiveInput = false;
    m_lastDragDeltaX = 0;
    m_lastDragDeltaY = 0;
}

bool PanController::isPanning() const {
    return glm::length(m_velocity) > PAN_VELOCITY_THRESHOLD || m_hasActiveInput;
}

void PanController::setConfig(const PanConfig& config) {
    m_config = config;
}

void PanController::setEdgeScrollingEnabled(bool enable) {
    m_config.enableEdgeScrolling = enable;
    if (!enable) {
        m_isEdgeScrolling = false;
    }
}

void PanController::configureForMapSize(int mapSize) {
    m_config.configureForMapSize(mapSize);
}

float PanController::calculateZoomSpeedFactor(float distance) const {
    // Calculate speed factor based on zoom level
    // At default distance (50), factor = 1.0
    // Zoomed out (higher distance) = faster pan
    // Zoomed in (lower distance) = slower pan

    float defaultDistance = CameraConfig::DISTANCE_DEFAULT;
    float minDistance = CameraConfig::DISTANCE_MIN;
    float maxDistance = CameraConfig::DISTANCE_MAX;

    // Normalize distance to 0-1 range
    float normalized = (distance - minDistance) / (maxDistance - minDistance);
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    // Interpolate between min and max speed factors
    float factor = m_config.minZoomSpeedFactor +
                   normalized * (m_config.maxZoomSpeedFactor - m_config.minZoomSpeedFactor);

    return factor * m_config.zoomSpeedMultiplier;
}

glm::vec2 PanController::calculateWorldPanDirection(const glm::vec2& inputDir, float yawDegrees) const {
    // Convert camera yaw to radians
    // Yaw 0 = looking along positive Z (north)
    // Yaw 90 = looking along positive X (east)
    float yawRad = glm::radians(yawDegrees);

    // Camera forward direction on the ground plane (based on yaw)
    // Yaw 45 (preset N) looks NE, etc.
    float cosYaw = std::cos(yawRad);
    float sinYaw = std::sin(yawRad);

    // Forward direction in world space (where "up" on screen points)
    // For a camera looking from the south at yaw=0, forward is +Z
    glm::vec2 forward(-sinYaw, -cosYaw);

    // Right direction in world space
    glm::vec2 right(cosYaw, -sinYaw);

    // Combine input with camera orientation
    // inputDir.x = right/left, inputDir.y = forward/backward
    glm::vec2 worldDir = right * inputDir.x + forward * inputDir.y;

    return worldDir;
}

glm::vec3 PanController::clampToMapBounds(const glm::vec3& focusPoint) const {
    // Map bounds clamping would go here if we had map size information
    // For now, return unclamped (infinite map)
    return focusPoint;
}

} // namespace sims3000
