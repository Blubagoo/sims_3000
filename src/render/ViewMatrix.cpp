/**
 * @file ViewMatrix.cpp
 * @brief View matrix calculation implementation.
 */

#include "sims3000/render/ViewMatrix.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace sims3000 {

glm::vec3 calculateCameraPosition(
    const glm::vec3& focus_point,
    float distance,
    float pitch_degrees,
    float yaw_degrees)
{
    // Convert degrees to radians
    const float pitch_rad = pitch_degrees * ViewMatrixConfig::DEG_TO_RAD;
    const float yaw_rad = yaw_degrees * ViewMatrixConfig::DEG_TO_RAD;

    // Calculate the camera offset from the focus point using spherical coordinates.
    //
    // Pitch is the angle from horizontal:
    // - pitch = 0 means camera is at same height as focus point (horizontal view)
    // - pitch = 90 would mean camera is directly above focus point (straight down)
    //
    // The camera position is calculated as:
    // - Vertical component (Y): distance * sin(pitch)
    // - Horizontal distance: distance * cos(pitch)
    //
    // The horizontal distance is then distributed in X and Z based on yaw:
    // - Our yaw convention: 0 degrees = camera looking South (camera is to the North)
    //   yaw increases clockwise when viewed from above
    //
    // For yaw = 0 (looking South): camera offset should be in +Z direction
    // For yaw = 90 (looking West): camera offset should be in -X direction
    // For yaw = 180 (looking North): camera offset should be in -Z direction
    // For yaw = 270 (looking East): camera offset should be in +X direction
    //
    // Using standard spherical coordinates with our conventions:
    // X offset = horizontal_dist * -sin(yaw)  (negative because yaw is clockwise)
    // Z offset = horizontal_dist * cos(yaw)

    const float horizontal_dist = distance * std::cos(pitch_rad);
    const float vertical_dist = distance * std::sin(pitch_rad);

    // Calculate XZ offset based on yaw
    // Note: We use -sin for X to match our clockwise yaw convention
    const float offset_x = horizontal_dist * (-std::sin(yaw_rad));
    const float offset_z = horizontal_dist * std::cos(yaw_rad);

    // Camera position = focus_point + offset
    return glm::vec3(
        focus_point.x + offset_x,
        focus_point.y + vertical_dist,
        focus_point.z + offset_z
    );
}

glm::vec3 calculateCameraPosition(const CameraState& state)
{
    return calculateCameraPosition(
        state.focus_point,
        state.distance,
        state.pitch,
        state.yaw
    );
}

glm::mat4 calculateViewMatrix(
    const glm::vec3& focus_point,
    float distance,
    float pitch_degrees,
    float yaw_degrees)
{
    // Calculate camera position from spherical coordinates
    const glm::vec3 camera_position = calculateCameraPosition(
        focus_point, distance, pitch_degrees, yaw_degrees);

    // The camera looks at the focus point
    const glm::vec3 target = focus_point;

    // Up vector is always world up (0, 1, 0)
    // Note: This works because pitch is clamped to 15-80 degrees,
    // so we never have the camera looking straight up or down
    // (which would cause gimbal lock with this up vector)
    const glm::vec3 up = ViewMatrixConfig::WORLD_UP;

    // Standard lookAt view matrix
    return glm::lookAt(camera_position, target, up);
}

glm::mat4 calculateViewMatrix(const CameraState& state)
{
    return calculateViewMatrix(
        state.focus_point,
        state.distance,
        state.pitch,
        state.yaw
    );
}

glm::vec3 calculateCameraForward(float pitch_degrees, float yaw_degrees)
{
    // Forward direction is from camera towards target (opposite of position offset)
    const float pitch_rad = pitch_degrees * ViewMatrixConfig::DEG_TO_RAD;
    const float yaw_rad = yaw_degrees * ViewMatrixConfig::DEG_TO_RAD;

    // Forward is the opposite of the position offset direction
    // Position offset: X = -sin(yaw) * cos(pitch), Y = sin(pitch), Z = cos(yaw) * cos(pitch)
    // Forward: X = sin(yaw) * cos(pitch), Y = -sin(pitch), Z = -cos(yaw) * cos(pitch)
    const float horizontal_component = std::cos(pitch_rad);

    return glm::normalize(glm::vec3(
        std::sin(yaw_rad) * horizontal_component,
        -std::sin(pitch_rad),
        -std::cos(yaw_rad) * horizontal_component
    ));
}

glm::vec3 calculateCameraRight(float yaw_degrees)
{
    // Right direction is perpendicular to forward on the XZ plane
    // For yaw = 0 (looking South), right is West (-X direction)
    // For yaw = 90 (looking West), right is South (-Z direction)
    const float yaw_rad = yaw_degrees * ViewMatrixConfig::DEG_TO_RAD;

    // Right vector is 90 degrees clockwise from forward on XZ plane
    // Forward XZ: (sin(yaw), -cos(yaw))
    // Right: rotate 90 degrees clockwise = (-cos(yaw), -sin(yaw))
    // But we want camera-right which is perpendicular to looking direction
    // For standard right-handed coordinate system:
    // right = normalize(cross(forward, up))
    // This simplifies to: (-cos(yaw), 0, -sin(yaw))
    return glm::normalize(glm::vec3(
        -std::cos(yaw_rad),
        0.0f,
        -std::sin(yaw_rad)
    ));
}

} // namespace sims3000
