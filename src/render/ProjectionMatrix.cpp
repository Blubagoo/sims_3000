/**
 * @file ProjectionMatrix.cpp
 * @brief Perspective projection matrix calculation implementation.
 */

#include "sims3000/render/ProjectionMatrix.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace sims3000 {

float clampFOV(float fov_degrees) {
    if (fov_degrees < ProjectionConfig::MIN_FOV_DEGREES) {
        return ProjectionConfig::MIN_FOV_DEGREES;
    }
    if (fov_degrees > ProjectionConfig::MAX_FOV_DEGREES) {
        return ProjectionConfig::MAX_FOV_DEGREES;
    }
    return fov_degrees;
}

float calculateAspectRatio(int width, int height) {
    if (width <= 0 || height <= 0) {
        return 1.0f;
    }
    return static_cast<float>(width) / static_cast<float>(height);
}

bool validateProjectionParameters(
    float fov_degrees,
    float aspect_ratio,
    float near_plane,
    float far_plane)
{
    // FOV must be in valid range
    if (fov_degrees < ProjectionConfig::MIN_FOV_DEGREES ||
        fov_degrees > ProjectionConfig::MAX_FOV_DEGREES) {
        return false;
    }

    // Aspect ratio must be positive
    if (aspect_ratio <= 0.0f) {
        return false;
    }

    // Near plane must be positive
    if (near_plane <= 0.0f) {
        return false;
    }

    // Far plane must be greater than near plane
    if (far_plane <= near_plane) {
        return false;
    }

    return true;
}

glm::mat4 calculateProjectionMatrix(
    float fov_degrees,
    float aspect_ratio,
    float near_plane,
    float far_plane)
{
    // Clamp FOV to valid range
    fov_degrees = clampFOV(fov_degrees);

    // Handle invalid aspect ratio
    if (aspect_ratio <= 0.0f) {
        aspect_ratio = 1.0f;
    }

    // Handle invalid near/far planes
    if (near_plane <= 0.0f) {
        near_plane = ProjectionConfig::NEAR_PLANE;
    }
    if (far_plane <= near_plane) {
        far_plane = near_plane + 1.0f;
    }

    // Convert FOV from degrees to radians
    const float fov_radians = fov_degrees * ProjectionConfig::DEG_TO_RAD;

    // Use glm::perspectiveRH_ZO for right-handed coordinates with [0, 1] depth range.
    // This is the correct function for Vulkan/SDL_GPU which expects:
    // - Right-handed coordinate system
    // - Depth range [0, 1] (not [-1, 1] like OpenGL)
    //
    // The perspective matrix applies the perspective divide:
    // - Objects further away appear smaller (depth causes size reduction)
    // - Parallel lines converge at vanishing points
    // - Z is mapped to depth buffer range [0, 1]
    return glm::perspectiveRH_ZO(fov_radians, aspect_ratio, near_plane, far_plane);
}

glm::mat4 calculateProjectionMatrixDefault(
    float aspect_ratio,
    float near_plane,
    float far_plane)
{
    return calculateProjectionMatrix(
        ProjectionConfig::DEFAULT_FOV_DEGREES,
        aspect_ratio,
        near_plane,
        far_plane);
}

glm::mat4 calculateProjectionMatrixFromDimensions(
    int window_width,
    int window_height,
    float fov_degrees,
    float near_plane,
    float far_plane)
{
    const float aspect_ratio = calculateAspectRatio(window_width, window_height);
    return calculateProjectionMatrix(fov_degrees, aspect_ratio, near_plane, far_plane);
}

glm::mat4 calculateViewProjectionMatrix(
    const glm::mat4& view_matrix,
    const glm::mat4& projection_matrix)
{
    // Standard matrix multiplication order: projection * view
    // This transforms vertices: projection * view * model * vertex
    // The GPU applies perspective divide after vertex shader:
    // ndc = clip.xyz / clip.w
    return projection_matrix * view_matrix;
}

} // namespace sims3000
