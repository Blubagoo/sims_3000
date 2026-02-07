/**
 * @file CameraUniforms.cpp
 * @brief Implementation of view-projection matrix integration.
 */

#include "sims3000/render/CameraUniforms.h"
#include "sims3000/render/ViewMatrix.h"
#include "sims3000/render/ProjectionMatrix.h"

namespace sims3000 {

CameraUniforms::CameraUniforms(
    std::uint32_t windowWidth,
    std::uint32_t windowHeight,
    float fovDegrees,
    float nearPlane,
    float farPlane)
    : m_windowWidth(windowWidth > 0 ? windowWidth : 1)
    , m_windowHeight(windowHeight > 0 ? windowHeight : 1)
    , m_fovDegrees(clampFOV(fovDegrees))
    , m_nearPlane(nearPlane > 0.0f ? nearPlane : ProjectionConfig::NEAR_PLANE)
    , m_farPlane(farPlane > nearPlane ? farPlane : nearPlane + 1.0f)
    , m_aspectRatio(1.0f)
    , m_projectionDirty(true)
{
    // Calculate initial aspect ratio
    updateAspectRatio();

    // Calculate initial projection matrix
    recalculateProjection();

    // Initialize UBO with identity matrix until first update
    m_ubo.viewProjection = glm::mat4(1.0f);
}

void CameraUniforms::update(const CameraState& state) {
    // Clear the dirty flag at the start of update
    // If projection was recalculated during this frame (e.g., resize),
    // the flag remains true for one frame so callers can detect it
    bool wasRecalculated = m_projectionDirty;

    // Recalculate projection if dirty (e.g., after resize)
    if (m_projectionDirty) {
        m_projection = calculateProjectionMatrix(
            m_fovDegrees,
            m_aspectRatio,
            m_nearPlane,
            m_farPlane);
        m_projectionDirty = false;
    }

    // Calculate view matrix from camera state
    m_view = calculateViewMatrix(state);

    // Combine view and projection matrices
    // Order: projection * view (column-major, applied right to left)
    m_viewProjection = calculateViewProjectionMatrix(m_view, m_projection);

    // Update UBO data
    m_ubo.viewProjection = m_viewProjection;

    // Preserve the recalculated flag for this frame
    m_projectionDirty = wasRecalculated;
}

void CameraUniforms::onWindowResize(std::uint32_t newWidth, std::uint32_t newHeight) {
    // Validate dimensions (prevent zero division)
    if (newWidth == 0) newWidth = 1;
    if (newHeight == 0) newHeight = 1;

    // Check if dimensions actually changed
    if (newWidth == m_windowWidth && newHeight == m_windowHeight) {
        return;  // No change, nothing to do
    }

    // Update dimensions
    m_windowWidth = newWidth;
    m_windowHeight = newHeight;

    // Recalculate aspect ratio
    updateAspectRatio();

    // Mark projection as dirty - will be recalculated on next update()
    m_projectionDirty = true;
}

void CameraUniforms::setFOV(float fovDegrees) {
    float clampedFOV = clampFOV(fovDegrees);

    if (clampedFOV != m_fovDegrees) {
        m_fovDegrees = clampedFOV;
        m_projectionDirty = true;
    }
}

void CameraUniforms::setClippingPlanes(float nearPlane, float farPlane) {
    // Validate planes
    if (nearPlane <= 0.0f) {
        nearPlane = ProjectionConfig::NEAR_PLANE;
    }
    if (farPlane <= nearPlane) {
        farPlane = nearPlane + 1.0f;
    }

    if (nearPlane != m_nearPlane || farPlane != m_farPlane) {
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
        m_projectionDirty = true;
    }
}

void CameraUniforms::recalculateProjection() {
    m_projection = calculateProjectionMatrix(
        m_fovDegrees,
        m_aspectRatio,
        m_nearPlane,
        m_farPlane);

    // Recalculate view-projection with new projection
    m_viewProjection = calculateViewProjectionMatrix(m_view, m_projection);

    // Update UBO
    m_ubo.viewProjection = m_viewProjection;

    // Mark as recalculated (will be cleared on next update cycle)
    m_projectionDirty = true;
}

void CameraUniforms::updateAspectRatio() {
    m_aspectRatio = calculateAspectRatio(
        static_cast<int>(m_windowWidth),
        static_cast<int>(m_windowHeight));
}

} // namespace sims3000
