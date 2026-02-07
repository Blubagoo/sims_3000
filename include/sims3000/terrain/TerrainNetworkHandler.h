/**
 * @file TerrainNetworkHandler.h
 * @brief Server-side handler for terrain modification network messages.
 *
 * TerrainNetworkHandler implements INetworkHandler to process incoming
 * TerrainModifyRequest messages from clients:
 * 1. Validates player authority (ownership or adjacent tile ownership)
 * 2. Validates sufficient credits
 * 3. Validates terrain type allows the operation
 * 4. Applies the modification via TerrainModificationSystem/GradeTerrainOperation
 * 5. Sends TerrainModifyResponse to requesting client
 * 6. Broadcasts TerrainModifiedEventMessage to all clients
 *
 * This handler integrates with the Epic 1 network infrastructure and
 * SyncSystem for state synchronization.
 *
 * @see TerrainNetworkMessages.h for message definitions
 * @see TerrainModificationSystem for clear operations
 * @see GradeTerrainOperation for grading operations
 * @see INetworkHandler for handler interface
 */

#ifndef SIMS3000_TERRAIN_TERRAINNETWORKHANDLER_H
#define SIMS3000_TERRAIN_TERRAINNETWORKHANDLER_H

#include "sims3000/net/INetworkHandler.h"
#include "sims3000/net/NetworkMessage.h"
#include "sims3000/terrain/TerrainNetworkMessages.h"
#include "sims3000/terrain/TerrainGrid.h"
#include "sims3000/terrain/ChunkDirtyTracker.h"
#include "sims3000/core/types.h"

#include <functional>
#include <cstdint>
#include <unordered_map>

namespace sims3000 {

// Forward declarations
class NetworkServer;

namespace terrain {

// Forward declarations
class TerrainModificationSystem;
class GradeTerrainOperation;

/**
 * @brief Callback type for querying player credits.
 *
 * @param player_id Player to query.
 * @return Current credit balance.
 */
using CreditsQueryCallback = std::function<Credits(PlayerID)>;

/**
 * @brief Callback type for deducting/adding player credits.
 *
 * @param player_id Player to modify.
 * @param amount Amount to deduct (positive) or add (negative).
 * @return true if successful, false if insufficient funds.
 */
using CreditsModifyCallback = std::function<bool(PlayerID, Credits)>;

/**
 * @brief Callback type for checking tile ownership.
 *
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param player_id Player to check.
 * @return true if player owns the tile or has authority.
 */
using OwnershipCheckCallback = std::function<bool(std::int32_t, std::int32_t, PlayerID)>;

/**
 * @struct TerrainHandlerConfig
 * @brief Configuration for terrain network handler.
 */
struct TerrainHandlerConfig {
    /// Allow operations on tiles adjacent to owned tiles (for expansion).
    bool allow_adjacent_operations = true;

    /// Allow operations on GAME_MASTER (0) owned tiles (unclaimed land).
    bool allow_unclaimed_operations = true;

    /// Maximum pending grade operations per player.
    std::uint32_t max_pending_grades_per_player = 5;
};

/**
 * @class TerrainNetworkHandler
 * @brief Server-side handler for terrain modification requests.
 *
 * Processes TerrainModifyRequest messages and manages the full
 * request-validate-apply-broadcast cycle.
 *
 * Usage:
 * @code
 *   TerrainGrid grid(MapSize::Medium);
 *   ChunkDirtyTracker dirty(grid.width, grid.height);
 *   TerrainModificationSystem modSystem(grid, dirty);
 *   GradeTerrainOperation gradeOp(grid, dirty);
 *   NetworkServer server(...);
 *
 *   TerrainNetworkHandler handler(server, grid, dirty, modSystem, gradeOp);
 *   handler.setCreditsQuery([&](PlayerID p) { return treasury.getBalance(p); });
 *   handler.setCreditsModify([&](PlayerID p, Credits c) { return treasury.deduct(p, c); });
 *   handler.setOwnershipCheck([&](int x, int y, PlayerID p) { return ownership.canModify(x, y, p); });
 *
 *   server.registerHandler(&handler);
 * @endcode
 */
class TerrainNetworkHandler : public INetworkHandler {
public:
    /**
     * @brief Construct the handler with required dependencies.
     *
     * @param server Reference to NetworkServer for sending responses.
     * @param grid Reference to TerrainGrid for reading terrain state.
     * @param dirty_tracker Reference to ChunkDirtyTracker for marking dirty chunks.
     * @param mod_system Reference to TerrainModificationSystem for clear operations.
     * @param grade_op Reference to GradeTerrainOperation for grade operations.
     * @param config Optional configuration.
     */
    TerrainNetworkHandler(
        NetworkServer& server,
        TerrainGrid& grid,
        ChunkDirtyTracker& dirty_tracker,
        TerrainModificationSystem& mod_system,
        GradeTerrainOperation& grade_op,
        const TerrainHandlerConfig& config = TerrainHandlerConfig{});

    ~TerrainNetworkHandler() override = default;

    // Non-copyable
    TerrainNetworkHandler(const TerrainNetworkHandler&) = delete;
    TerrainNetworkHandler& operator=(const TerrainNetworkHandler&) = delete;

    // =========================================================================
    // INetworkHandler Interface
    // =========================================================================

    /**
     * @brief Check if this handler processes terrain modification messages.
     */
    bool canHandle(MessageType type) const override;

    /**
     * @brief Handle an incoming terrain modification message.
     * @param peer Source peer ID.
     * @param msg The deserialized message.
     */
    void handleMessage(PeerID peer, const NetworkMessage& msg) override;

    /**
     * @brief Called when a client disconnects.
     *
     * Cancels any pending grade operations for that player.
     */
    void onClientDisconnected(PeerID peer, bool timedOut) override;

    // =========================================================================
    // Configuration Callbacks
    // =========================================================================

    /**
     * @brief Set the callback for querying player credits.
     */
    void setCreditsQuery(CreditsQueryCallback callback);

    /**
     * @brief Set the callback for modifying player credits.
     */
    void setCreditsModify(CreditsModifyCallback callback);

    /**
     * @brief Set the callback for checking tile ownership.
     */
    void setOwnershipCheck(OwnershipCheckCallback callback);

    /**
     * @brief Set the peer-to-player mapping callback.
     *
     * Used to look up PlayerID from PeerID.
     */
    void setPeerToPlayerCallback(std::function<PlayerID(PeerID)> callback);

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * @brief Get total requests received.
     */
    std::uint64_t getRequestsReceived() const { return m_requestsReceived; }

    /**
     * @brief Get total requests approved.
     */
    std::uint64_t getRequestsApproved() const { return m_requestsApproved; }

    /**
     * @brief Get total requests rejected.
     */
    std::uint64_t getRequestsRejected() const { return m_requestsRejected; }

    /**
     * @brief Get the configuration.
     */
    const TerrainHandlerConfig& getConfig() const { return m_config; }

    /**
     * @brief Update the configuration.
     */
    void setConfig(const TerrainHandlerConfig& config) { m_config = config; }

private:
    // =========================================================================
    // Internal Validation Methods
    // =========================================================================

    /**
     * @brief Validate a clear terrain request.
     * @param data Request data.
     * @param out_cost Output: cost if valid.
     * @return Result code.
     */
    TerrainModifyResult validateClearRequest(
        const TerrainModifyRequestData& data,
        Credits& out_cost);

    /**
     * @brief Validate a grade terrain request.
     * @param data Request data.
     * @param out_cost Output: cost if valid.
     * @return Result code.
     */
    TerrainModifyResult validateGradeRequest(
        const TerrainModifyRequestData& data,
        Credits& out_cost);

    /**
     * @brief Check if player has authority over a tile.
     *
     * Checks:
     * - Direct ownership
     * - Adjacent tile ownership (if allow_adjacent_operations)
     * - GAME_MASTER ownership (if allow_unclaimed_operations)
     *
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param player_id Player to check.
     * @return true if player has authority.
     */
    bool hasAuthority(std::int32_t x, std::int32_t y, PlayerID player_id);

    /**
     * @brief Check if player owns any adjacent tile.
     */
    bool ownsAdjacentTile(std::int32_t x, std::int32_t y, PlayerID player_id);

    // =========================================================================
    // Internal Application Methods
    // =========================================================================

    /**
     * @brief Apply a clear operation and broadcast result.
     */
    void applyClearOperation(
        PeerID peer,
        const TerrainModifyRequestData& data,
        Credits cost);

    /**
     * @brief Start a grade operation and send initial response.
     *
     * Note: Full grade operations using ECS entities would require
     * registry access. This simplified version handles the network
     * messaging without the multi-tick ECS entity approach.
     */
    void applyGradeOperation(
        PeerID peer,
        const TerrainModifyRequestData& data,
        Credits cost);

    /**
     * @brief Send a response to the requesting client.
     */
    void sendResponse(
        PeerID peer,
        std::uint32_t sequence_num,
        TerrainModifyResult result,
        Credits cost_applied);

    /**
     * @brief Broadcast terrain modification to all clients.
     */
    void broadcastModification(
        const GridRect& area,
        ModificationType type,
        PlayerID player_id,
        std::uint8_t new_elevation);

    /**
     * @brief Get player ID from peer ID.
     */
    PlayerID getPlayerIdFromPeer(PeerID peer) const;

    // =========================================================================
    // Member Variables
    // =========================================================================

    NetworkServer& m_server;
    TerrainGrid& m_grid;
    ChunkDirtyTracker& m_dirtyTracker;
    TerrainModificationSystem& m_modSystem;
    GradeTerrainOperation& m_gradeOp;
    TerrainHandlerConfig m_config;

    // Callbacks
    CreditsQueryCallback m_creditsQuery;
    CreditsModifyCallback m_creditsModify;
    OwnershipCheckCallback m_ownershipCheck;
    std::function<PlayerID(PeerID)> m_peerToPlayer;

    // Statistics
    std::uint64_t m_requestsReceived = 0;
    std::uint64_t m_requestsApproved = 0;
    std::uint64_t m_requestsRejected = 0;

    // Pending operations per player (for limit enforcement)
    std::unordered_map<PlayerID, std::uint32_t> m_pendingGradeCount;
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINNETWORKHANDLER_H
