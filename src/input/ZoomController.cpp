/**
 * @file ZoomController.cpp
 * @brief Zoom controller implementation with cursor-centering and smooth interpolation.
 */

#include "sims3000/input/ZoomController.h"
#include "sims3000/input/InputSystem.h"
#include "sims3000/render/ScreenToWorld.h"
#include <cmath>
#include <algorithm>

namespace sims3000 {

// ============================================================================
// ZoomConfig Implementation
// ============================================================================

void ZoomConfig::configureForMapSize(int mapSize) {
    // Base minimum distance stays the same for all map sizes
    minDistance = CameraConfig::DISTANCE_MIN;

    // Maximum distance scales with map size for proper navigation
    if (mapSize <= 128) {
        // Small maps: standard range
        maxDistance = 100.0f;
    } else if (mapSize <= 256) {
        // Medium maps: extended range
        maxDistance = 150.0f;
    } else {
        // Large maps (512+): wide range for overview
        maxDistance = 250.0f;
    }
}

ZoomConfig ZoomConfig::defaultSmall() {
    ZoomConfig config;
    config.configureForMapSize(128);
    return config;
}

ZoomConfig ZoomConfig::defaultMedium() {
    ZoomConfig config;
    config.configureForMapSize(256);
    return config;
}

ZoomConfig ZoomConfig::defaultLarge() {
    ZoomConfig config;
    config.configureForMapSize(512);
    return config;
}

// ============================================================================
// ZoomController Implementation
// ============================================================================

ZoomController::ZoomController()
    : m_config()
    , m_targetDistance(CameraConfig::DISTANCE_DEFAULT)
    , m_targetFocusPoint(0.0f)
    , m_currentDistance(CameraConfig::DISTANCE_DEFAULT)
    , m_currentFocusPoint(0.0f)
{
}

ZoomController::ZoomController(const ZoomConfig& config)
    : m_config(config)
    , m_targetDistance(CameraConfig::DISTANCE_DEFAULT)
    , m_targetFocusPoint(0.0f)
    , m_currentDistance(CameraConfig::DISTANCE_DEFAULT)
    , m_currentFocusPoint(0.0f)
{
}

bool ZoomController::handleInput(
    const InputSystem& input,
    const CameraState& cameraState,
    const glm::mat4& viewProjection,
    float windowWidth,
    float windowHeight)
{
    const MouseState& mouse = input.getMouse();

    // Check for mouse wheel input
    if (std::fabs(mouse.wheelY) < 0.001f) {
        return false;  // No zoom input
    }

    return handleZoom(
        mouse.wheelY,
        static_cast<float>(mouse.x),
        static_cast<float>(mouse.y),
        cameraState,
        viewProjection,
        windowWidth,
        windowHeight);
}

bool ZoomController::handleZoom(
    float wheelDelta,
    float cursorX,
    float cursorY,
    const CameraState& cameraState,
    const glm::mat4& viewProjection,
    float windowWidth,
    float windowHeight)
{
    if (std::fabs(wheelDelta) < 0.001f) {
        return false;
    }

    // Sync current state on first zoom input after reset
    m_currentDistance = cameraState.distance;
    m_currentFocusPoint = cameraState.focus_point;

    // Calculate zoom factor using logarithmic scaling for perceptual consistency
    // Positive wheel = zoom in (smaller distance)
    // The exponential formula ensures zoom "feels" the same at all distances
    float zoomMultiplier = std::exp(-wheelDelta * m_config.zoomSpeed);
    float desiredDistance = m_currentDistance * zoomMultiplier;

    // Apply soft boundaries
    float boundedDistance = applySoftBoundary(m_currentDistance, desiredDistance);

    // Calculate focus point adjustment for zoom-to-cursor
    glm::vec3 newFocusPoint = m_currentFocusPoint;

    if (m_config.centerOnCursor) {
        // Get world position under cursor
        std::optional<glm::vec3> cursorWorldPos = getCursorWorldPosition(
            cursorX, cursorY,
            windowWidth, windowHeight,
            viewProjection,
            cameraState,
            0.0f);  // Ground plane at Y=0

        if (cursorWorldPos.has_value()) {
            newFocusPoint = calculateFocusAdjustment(
                m_currentFocusPoint,
                cursorWorldPos.value(),
                m_currentDistance,
                boundedDistance);
        }
    }

    // Set targets for interpolation
    m_targetDistance = boundedDistance;
    m_targetFocusPoint = newFocusPoint;

    return true;
}

void ZoomController::update(float deltaTime, CameraState& cameraState) {
    // Exponential smoothing for natural feel
    float t = 1.0f - std::exp(-m_config.smoothingFactor * deltaTime);
    t = std::clamp(t, 0.0f, 1.0f);

    // Interpolate distance
    m_currentDistance = m_currentDistance + (m_targetDistance - m_currentDistance) * t;

    // Interpolate focus point
    m_currentFocusPoint = m_currentFocusPoint + (m_targetFocusPoint - m_currentFocusPoint) * t;

    // Apply to camera state
    cameraState.distance = m_currentDistance;
    cameraState.focus_point = m_currentFocusPoint;

    // Apply constraints
    cameraState.clampDistance();
}

void ZoomController::setTargetDistance(float distance) {
    // Clamp to configured limits
    m_targetDistance = std::clamp(distance, m_config.minDistance, m_config.maxDistance);
}

void ZoomController::setDistanceImmediate(float distance, CameraState& cameraState) {
    float clampedDistance = std::clamp(distance, m_config.minDistance, m_config.maxDistance);

    m_targetDistance = clampedDistance;
    m_currentDistance = clampedDistance;
    cameraState.distance = clampedDistance;
}

void ZoomController::reset(const CameraState& cameraState) {
    m_targetDistance = cameraState.distance;
    m_targetFocusPoint = cameraState.focus_point;
    m_currentDistance = cameraState.distance;
    m_currentFocusPoint = cameraState.focus_point;
}

bool ZoomController::isZooming() const {
    float distanceDelta = std::fabs(m_targetDistance - m_currentDistance);
    float focusDelta = glm::length(m_targetFocusPoint - m_currentFocusPoint);

    return distanceDelta > ZOOM_COMPLETE_THRESHOLD || focusDelta > ZOOM_COMPLETE_THRESHOLD;
}

void ZoomController::setConfig(const ZoomConfig& config) {
    m_config = config;

    // Ensure current values are within new limits
    m_targetDistance = std::clamp(m_targetDistance, m_config.minDistance, m_config.maxDistance);
    m_currentDistance = std::clamp(m_currentDistance, m_config.minDistance, m_config.maxDistance);
}

void ZoomController::setDistanceLimits(float minDistance, float maxDistance) {
    if (minDistance > 0.0f && maxDistance > minDistance) {
        m_config.minDistance = minDistance;
        m_config.maxDistance = maxDistance;

        // Clamp current values to new limits
        m_targetDistance = std::clamp(m_targetDistance, minDistance, maxDistance);
        m_currentDistance = std::clamp(m_currentDistance, minDistance, maxDistance);
    }
}

void ZoomController::configureForMapSize(int mapSize) {
    m_config.configureForMapSize(mapSize);

    // Clamp current values to new limits
    m_targetDistance = std::clamp(m_targetDistance, m_config.minDistance, m_config.maxDistance);
    m_currentDistance = std::clamp(m_currentDistance, m_config.minDistance, m_config.maxDistance);
}

float ZoomController::applySoftBoundary(float currentDistance, float desiredDistance) const {
    float minDist = m_config.minDistance;
    float maxDist = m_config.maxDistance;
    float range = maxDist - minDist;

    if (range <= 0.0f) {
        return std::clamp(desiredDistance, minDist, maxDist);
    }

    // Calculate soft boundary regions
    float softRegionSize = range * m_config.softBoundaryStart;
    float softMinThreshold = minDist + softRegionSize;
    float softMaxThreshold = maxDist - softRegionSize;

    // If within normal range, no adjustment needed
    if (desiredDistance >= softMinThreshold && desiredDistance <= softMaxThreshold) {
        return desiredDistance;
    }

    // Calculate delta from current position
    float delta = desiredDistance - currentDistance;

    // Apply deceleration in soft boundary regions
    if (desiredDistance < softMinThreshold) {
        // Approaching minimum - decelerate zoom in
        if (delta < 0.0f) {  // Zooming in
            float normalizedPos = (currentDistance - minDist) / softRegionSize;
            normalizedPos = std::clamp(normalizedPos, 0.0f, 1.0f);

            // Apply power curve for smooth deceleration
            float factor = std::pow(normalizedPos, m_config.softBoundaryPower);
            delta *= factor;
        }
    } else if (desiredDistance > softMaxThreshold) {
        // Approaching maximum - decelerate zoom out
        if (delta > 0.0f) {  // Zooming out
            float normalizedPos = (maxDist - currentDistance) / softRegionSize;
            normalizedPos = std::clamp(normalizedPos, 0.0f, 1.0f);

            // Apply power curve for smooth deceleration
            float factor = std::pow(normalizedPos, m_config.softBoundaryPower);
            delta *= factor;
        }
    }

    // Calculate adjusted distance and hard clamp
    float adjusted = currentDistance + delta;
    return std::clamp(adjusted, minDist, maxDist);
}

glm::vec3 ZoomController::calculateFocusAdjustment(
    const glm::vec3& currentFocus,
    const glm::vec3& cursorWorldPos,
    float currentDistance,
    float targetDistance) const
{
    // The zoom-to-cursor algorithm:
    // We want the cursor to point to the same world position after zooming.
    //
    // The key insight is that the focus point should move toward the cursor
    // world position by an amount proportional to the zoom change.
    //
    // When zooming in (targetDistance < currentDistance):
    //   - Move focus toward cursor position
    //   - Closer zoom = larger apparent movement of focus toward cursor
    //
    // When zooming out (targetDistance > currentDistance):
    //   - Move focus away from cursor position
    //   - Further zoom = focus moves away proportionally
    //
    // The formula: newFocus = currentFocus + (cursorWorld - currentFocus) * (1 - targetDist/currentDist)
    //
    // This ensures the cursor world point stays "fixed" on screen.

    if (currentDistance <= 0.0f) {
        return currentFocus;  // Avoid division by zero
    }

    float distanceRatio = targetDistance / currentDistance;

    // Calculate the vector from focus to cursor world position
    glm::vec3 focusToCursor = cursorWorldPos - currentFocus;

    // Scale factor: 1 - ratio means:
    // - When zooming in (ratio < 1): positive factor, move toward cursor
    // - When zooming out (ratio > 1): negative factor, move away from cursor
    float scaleFactor = 1.0f - distanceRatio;

    // New focus point
    glm::vec3 newFocus = currentFocus + focusToCursor * scaleFactor;

    return newFocus;
}

} // namespace sims3000
