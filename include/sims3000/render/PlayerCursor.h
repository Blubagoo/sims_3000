/**
 * @file PlayerCursor.h
 * @brief Player cursor data structures and faction color definitions.
 *
 * Defines the PlayerCursor structure for rendering other players' cursors
 * in multiplayer. Each player has a distinct faction color for identification.
 *
 * Cursors are rendered as 3D indicators at world positions, showing where
 * other overseers are looking/acting. This provides social presence and
 * awareness of other players' activity in the shared world.
 *
 * Resource ownership: None (pure data structs).
 *
 * @see ICursorSync for the sync interface
 * @see CursorRenderer for rendering implementation
 */

#ifndef SIMS3000_RENDER_PLAYERCURSOR_H
#define SIMS3000_RENDER_PLAYERCURSOR_H

#include "sims3000/core/types.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <array>

namespace sims3000 {

// ============================================================================
// Faction Color Palette
// ============================================================================

/**
 * @brief Bioluminescent faction color palette.
 *
 * Colors are chosen to be:
 * - Distinct from each other (easy to identify)
 * - Consistent with the bioluminescent art direction
 * - Readable in both light and dark environments
 * - Not clashing with terrain/building colors
 *
 * Colors are stored as RGBA with full alpha (1.0).
 */
namespace FactionColors {
    /// Player 1: Cyan/Teal - primary bioluminescent accent
    constexpr glm::vec4 PLAYER_1{0.0f, 0.9f, 0.9f, 1.0f};

    /// Player 2: Amber/Orange - warm energy accent
    constexpr glm::vec4 PLAYER_2{1.0f, 0.6f, 0.1f, 1.0f};

    /// Player 3: Magenta/Pink - special structure accent
    constexpr glm::vec4 PLAYER_3{0.9f, 0.2f, 0.6f, 1.0f};

    /// Player 4: Lime Green - healthy zone accent
    constexpr glm::vec4 PLAYER_4{0.5f, 1.0f, 0.2f, 1.0f};

    /// Game Master/Neutral: White (for unowned cursors, if applicable)
    constexpr glm::vec4 NEUTRAL{1.0f, 1.0f, 1.0f, 1.0f};

    /// Inactive/Stale cursor color (semi-transparent gray)
    constexpr glm::vec4 INACTIVE{0.5f, 0.5f, 0.5f, 0.5f};

    /// Maximum number of player faction colors
    constexpr std::size_t MAX_PLAYERS = 4;

    /**
     * @brief Get faction color for a player ID.
     *
     * @param playerId Player identifier (1-4 for players, 0 for neutral).
     * @return Faction color as RGBA vec4.
     */
    inline glm::vec4 getColorForPlayer(PlayerID playerId) {
        switch (playerId) {
            case 1: return PLAYER_1;
            case 2: return PLAYER_2;
            case 3: return PLAYER_3;
            case 4: return PLAYER_4;
            case 0: return NEUTRAL;  // GAME_MASTER
            default: return NEUTRAL; // Unknown player
        }
    }

    /**
     * @brief Get all player faction colors as an array.
     *
     * Index 0 = neutral, Index 1-4 = players 1-4.
     * Useful for shader uniform arrays.
     *
     * @return Array of 5 faction colors.
     */
    inline std::array<glm::vec4, 5> getAllColors() {
        return {NEUTRAL, PLAYER_1, PLAYER_2, PLAYER_3, PLAYER_4};
    }
} // namespace FactionColors

// ============================================================================
// Player Cursor Structure
// ============================================================================

/**
 * @struct PlayerCursor
 * @brief Represents a remote player's cursor position in the world.
 *
 * Used for rendering other players' cursors as 3D indicators at their
 * current world position. The cursor shows:
 * - Where the player is looking
 * - What tile they might be about to interact with
 * - That they are active in the game
 *
 * Cursor sync is unreliable UDP at 10-20Hz - not every tick.
 * This is visual feedback only, not gameplay-critical.
 */
struct PlayerCursor {
    /// Player who owns this cursor
    PlayerID player_id = 0;

    /// Whether the cursor is currently active (player is connected and active)
    bool is_active = false;

    /// Padding for alignment (2 bytes to align world_position to 4-byte boundary)
    std::uint8_t padding[2] = {};

    /// World-space position of the cursor (on ground plane or selected tile)
    glm::vec3 world_position{0.0f, 0.0f, 0.0f};

    /// Time since last update (in seconds) - for stale cursor detection
    float time_since_update = 0.0f;

    // =========================================================================
    // Helper Methods
    // =========================================================================

    /**
     * @brief Get the faction color for this cursor.
     * @return RGBA color for rendering.
     */
    glm::vec4 getFactionColor() const {
        if (!is_active) {
            return FactionColors::INACTIVE;
        }
        return FactionColors::getColorForPlayer(player_id);
    }

    /**
     * @brief Check if the cursor is stale (hasn't been updated recently).
     *
     * Stale cursors are rendered differently or hidden to indicate
     * the player may have disconnected or is idle.
     *
     * @param threshold Staleness threshold in seconds (default 2.0s).
     * @return true if cursor is stale.
     */
    bool isStale(float threshold = 2.0f) const {
        return time_since_update > threshold;
    }

    /**
     * @brief Update the time since last update.
     * @param deltaTime Time elapsed since last frame.
     */
    void updateTime(float deltaTime) {
        time_since_update += deltaTime;
    }

    /**
     * @brief Reset the time since last update (called when cursor position is received).
     */
    void resetTime() {
        time_since_update = 0.0f;
    }
};

static_assert(sizeof(PlayerCursor) == 20, "PlayerCursor size check");

// ============================================================================
// Cursor Indicator Configuration
// ============================================================================

/**
 * @struct CursorIndicatorConfig
 * @brief Configuration for cursor indicator rendering.
 */
struct CursorIndicatorConfig {
    /// Cursor indicator scale (world units)
    float scale = 0.5f;

    /// Vertical offset above ground (to prevent z-fighting)
    float vertical_offset = 0.1f;

    /// Emissive glow intensity for faction color
    float emissive_intensity = 0.8f;

    /// Pulse animation speed (cycles per second)
    float pulse_speed = 2.0f;

    /// Pulse amplitude (scale variation)
    float pulse_amplitude = 0.1f;

    /// Staleness fade duration (seconds to fade out stale cursors)
    float stale_fade_duration = 1.0f;

    /// Staleness threshold (seconds before cursor is considered stale)
    float stale_threshold = 2.0f;

    /// Show cursor labels (player name/number)
    bool show_labels = true;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_PLAYERCURSOR_H
