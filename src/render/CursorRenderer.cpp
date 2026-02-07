/**
 * @file CursorRenderer.cpp
 * @brief Implementation of CursorRenderer for multiplayer cursor display.
 */

#include "sims3000/render/CursorRenderer.h"
#include "sims3000/render/CameraState.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace sims3000 {

CursorRenderer::CursorRenderer(
    ICursorSync* cursorSync,
    const CursorIndicatorConfig& config)
    : m_cursorSync(cursorSync)
    , m_config(config)
{
}

void CursorRenderer::update(float deltaTime) {
    // Update animation time for pulse effect
    m_animationTime += deltaTime;

    // Wrap animation time to prevent float overflow after long sessions
    constexpr float MAX_ANIMATION_TIME = 1000.0f;
    if (m_animationTime > MAX_ANIMATION_TIME) {
        m_animationTime -= MAX_ANIMATION_TIME;
    }
}

void CursorRenderer::updateLocalCursorPosition(const glm::vec3& worldPosition) {
    if (m_cursorSync != nullptr) {
        m_cursorSync->updateLocalCursor(worldPosition);
    }
}

std::vector<CursorRenderData> CursorRenderer::prepareCursors(
    const CameraState& /*cameraState*/,
    const glm::mat4& viewProjection,
    float windowWidth,
    float windowHeight)
{
    m_visibleCursorCount = 0;
    std::vector<CursorRenderData> renderData;

    // No cursor sync provider or disabled
    if (m_cursorSync == nullptr || !m_enabled) {
        return renderData;
    }

    // No cursors to render if sync is not active
    if (!m_cursorSync->isSyncActive()) {
        return renderData;
    }

    // Get remote player cursors
    std::vector<PlayerCursor> cursors = m_cursorSync->getPlayerCursors();

    if (cursors.empty()) {
        return renderData;
    }

    // Calculate current pulse phase
    float pulsePhase = m_animationTime * m_config.pulse_speed * glm::two_pi<float>();

    // Process each cursor
    for (const PlayerCursor& cursor : cursors) {
        // Skip inactive cursors
        if (!cursor.is_active) {
            continue;
        }

        // Check if cursor is on screen (simple frustum check)
        ScreenProjectionResult projection = worldToScreen(
            cursor.world_position,
            viewProjection,
            windowWidth,
            windowHeight);

        // Skip cursors that are behind the camera
        if (projection.behindCamera) {
            continue;
        }

        // Calculate pulse scale
        float pulseScale = 1.0f + m_config.pulse_amplitude * std::sin(pulsePhase);
        float finalScale = m_config.scale * pulseScale;

        // Calculate staleness fade
        float stalenessFade = 1.0f;
        if (cursor.isStale(m_config.stale_threshold)) {
            float staleTime = cursor.time_since_update - m_config.stale_threshold;
            stalenessFade = 1.0f - glm::clamp(
                staleTime / m_config.stale_fade_duration, 0.0f, 1.0f);

            // Skip fully faded cursors
            if (stalenessFade <= 0.01f) {
                continue;
            }
        }

        // Prepare render data
        CursorRenderData data;
        data.model_matrix = calculateCursorTransform(cursor.world_position, finalScale);
        data.tint_color = calculateCursorColor(cursor);
        data.tint_color.a *= stalenessFade; // Apply staleness fade
        data.emissive_color = calculateEmissiveColor(cursor, pulsePhase);
        data.emissive_color.a *= stalenessFade; // Fade glow too
        data.visible = true;
        data.player_id = cursor.player_id;

        renderData.push_back(data);
        ++m_visibleCursorCount;
    }

    return renderData;
}

glm::mat4 CursorRenderer::calculateCursorTransform(
    const glm::vec3& worldPosition,
    float scale) const
{
    // Position the cursor slightly above the ground to prevent z-fighting
    glm::vec3 position = worldPosition;
    position.y += m_config.vertical_offset;

    // Build transform matrix: translate * scale
    // Cursor indicator points downward (no rotation needed if model is oriented correctly)
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform = glm::scale(transform, glm::vec3(scale));

    return transform;
}

glm::vec4 CursorRenderer::calculateCursorColor(const PlayerCursor& cursor) const {
    // Get faction color
    glm::vec4 color = cursor.getFactionColor();

    // Apply staleness (handled in prepareCursors for alpha)
    return color;
}

glm::vec4 CursorRenderer::calculateEmissiveColor(
    const PlayerCursor& cursor,
    float pulsePhase) const
{
    // Base emissive color is the faction color
    glm::vec4 factionColor = FactionColors::getColorForPlayer(cursor.player_id);

    // Pulse the intensity
    float intensity = m_config.emissive_intensity;
    intensity *= (0.8f + 0.2f * std::sin(pulsePhase)); // Subtle intensity pulse

    // Return emissive color with intensity in alpha channel
    return glm::vec4(factionColor.r, factionColor.g, factionColor.b, intensity);
}

} // namespace sims3000
