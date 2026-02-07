/**
 * @file ICursorSync.h
 * @brief Interface for multiplayer cursor synchronization.
 *
 * Defines the ICursorSync interface for syncing cursor positions between
 * players in multiplayer. The interface allows:
 * - Clients to report their local cursor position to the server
 * - Clients to receive other players' cursor positions for rendering
 *
 * Cursor sync is unreliable UDP at 10-20Hz - visual feedback only, not
 * gameplay-critical. The server broadcasts cursor positions to all clients.
 *
 * Implementation is provided by SyncSystem (Epic 1).
 *
 * @see PlayerCursor for cursor data structure
 * @see CursorRenderer for rendering implementation
 */

#ifndef SIMS3000_SYNC_ICURSORSYNC_H
#define SIMS3000_SYNC_ICURSORSYNC_H

#include "sims3000/render/PlayerCursor.h"
#include <glm/glm.hpp>
#include <vector>

namespace sims3000 {

/**
 * @class ICursorSync
 * @brief Interface for multiplayer cursor synchronization.
 *
 * Provides methods for:
 * - Reporting local cursor position to sync
 * - Retrieving other players' cursor positions for rendering
 *
 * For local/single-player mode, the stub implementation returns an empty
 * cursor list and ignores local cursor updates.
 */
class ICursorSync {
public:
    virtual ~ICursorSync() = default;

    /**
     * @brief Get all remote player cursor positions.
     *
     * Returns cursor positions for all connected players except the local
     * player. Cursors may be stale if not recently updated.
     *
     * @return Vector of PlayerCursor structs for remote players.
     */
    virtual std::vector<PlayerCursor> getPlayerCursors() const = 0;

    /**
     * @brief Update the local player's cursor position.
     *
     * Called by the input system when the cursor moves. The position is
     * sent to the server for broadcast to other clients.
     *
     * @param worldPosition World-space position of the local cursor.
     */
    virtual void updateLocalCursor(const glm::vec3& worldPosition) = 0;

    /**
     * @brief Get the local player's ID.
     *
     * Used to filter out the local player's cursor from the remote list.
     *
     * @return Local player's PlayerID.
     */
    virtual PlayerID getLocalPlayerId() const = 0;

    /**
     * @brief Check if cursor sync is available.
     *
     * Returns false for single-player or when not connected.
     *
     * @return true if cursor sync is active.
     */
    virtual bool isSyncActive() const = 0;
};

/**
 * @class StubCursorSync
 * @brief Stub implementation for single-player mode.
 *
 * Returns empty cursor list and ignores local cursor updates.
 * Used when multiplayer is not active.
 */
class StubCursorSync : public ICursorSync {
public:
    /**
     * @brief Get all remote player cursors (always empty in stub).
     * @return Empty vector.
     */
    std::vector<PlayerCursor> getPlayerCursors() const override {
        return {};
    }

    /**
     * @brief Update local cursor (no-op in stub).
     * @param worldPosition Ignored.
     */
    void updateLocalCursor(const glm::vec3& /*worldPosition*/) override {
        // No-op in single-player
    }

    /**
     * @brief Get local player ID (always 1 in stub for single-player).
     * @return Player 1.
     */
    PlayerID getLocalPlayerId() const override {
        return 1; // Single-player is always player 1
    }

    /**
     * @brief Check if sync is active (always false in stub).
     * @return false.
     */
    bool isSyncActive() const override {
        return false;
    }
};

} // namespace sims3000

#endif // SIMS3000_SYNC_ICURSORSYNC_H
