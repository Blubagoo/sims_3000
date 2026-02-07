/**
 * @file TerrainClientHandler.h
 * @brief Client-side handler for terrain modification network messages.
 *
 * TerrainClientHandler implements INetworkHandler to process incoming
 * TerrainModifiedEventMessage broadcasts from the server:
 * 1. Receives terrain modification events from the server
 * 2. Applies changes to the local TerrainGrid
 * 3. Marks affected chunks dirty for re-rendering
 *
 * This is the client-side counterpart to TerrainNetworkHandler (server-side).
 * While the server handler validates and applies requests, this handler
 * synchronizes the local terrain state with authoritative server changes.
 *
 * @see TerrainNetworkMessages.h for message definitions
 * @see TerrainNetworkHandler.h for server-side handler
 * @see INetworkHandler for handler interface
 */

#ifndef SIMS3000_TERRAIN_TERRAINCLIENTHANDLER_H
#define SIMS3000_TERRAIN_TERRAINCLIENTHANDLER_H

#include "sims3000/net/INetworkHandler.h"
#include "sims3000/net/NetworkMessage.h"
#include "sims3000/terrain/TerrainNetworkMessages.h"
#include "sims3000/terrain/TerrainNetworkSync.h"
#include "sims3000/terrain/TerrainGrid.h"
#include "sims3000/terrain/WaterData.h"
#include "sims3000/terrain/ChunkDirtyTracker.h"
#include "sims3000/terrain/TerrainEvents.h"
#include "sims3000/core/types.h"

#include <functional>
#include <cstdint>
#include <memory>

namespace sims3000 {
namespace terrain {

/**
 * @brief Callback type for terrain modification events.
 *
 * Called after the client handler applies a terrain modification.
 * Can be used to trigger audio, visual effects, or other client-side feedback.
 */
using ClientTerrainEventCallback = std::function<void(const TerrainModifiedEvent&, PlayerID)>;

/**
 * @class TerrainClientHandler
 * @brief Client-side handler for terrain modification broadcasts and sync.
 *
 * Processes terrain network messages from the server:
 * - TerrainSyncRequest: Regenerates terrain from seed, applies modifications
 * - TerrainModifiedEvent: Applies real-time terrain modifications
 * - TerrainSyncComplete: Handles sync completion/fallback
 *
 * Implements the optimized sync flow (seed + modifications) for bandwidth
 * efficiency. Falls back to full snapshot if deterministic verification fails.
 *
 * Usage:
 * @code
 *   TerrainGrid grid(MapSize::Medium);
 *   WaterData waterData(MapSize::Medium);
 *   ChunkDirtyTracker dirty(grid.width, grid.height);
 *   NetworkClient client(...);
 *
 *   TerrainClientHandler handler(grid, waterData, dirty);
 *   handler.setEventCallback([](const TerrainModifiedEvent& e, PlayerID p) {
 *       // Play sound effect, show particle effect, etc.
 *   });
 *   handler.setSyncCompleteCallback([](bool success) {
 *       // Handle sync completion
 *   });
 *
 *   client.registerHandler(&handler);
 * @endcode
 */
/**
 * @brief Callback type for sync completion notification.
 *
 * Called when terrain sync completes. Parameter is true if successful,
 * false if fallback to full snapshot is required.
 */
using SyncCompleteCallback = std::function<void(bool success)>;

/**
 * @brief Callback type for requesting full snapshot fallback.
 *
 * Called when seed-based sync fails and full snapshot is needed.
 */
using SnapshotFallbackCallback = std::function<void()>;

class TerrainClientHandler : public INetworkHandler {
public:
    /**
     * @brief Construct the handler with required dependencies.
     *
     * @param grid Reference to TerrainGrid for applying terrain changes.
     * @param waterData Reference to WaterData for water body info.
     * @param dirty_tracker Reference to ChunkDirtyTracker for marking dirty chunks.
     */
    TerrainClientHandler(TerrainGrid& grid, WaterData& waterData,
                         ChunkDirtyTracker& dirty_tracker);

    /**
     * @brief Legacy constructor for backward compatibility.
     *
     * @param grid Reference to TerrainGrid for applying terrain changes.
     * @param dirty_tracker Reference to ChunkDirtyTracker for marking dirty chunks.
     */
    TerrainClientHandler(TerrainGrid& grid, ChunkDirtyTracker& dirty_tracker);

    ~TerrainClientHandler() override = default;

    // Non-copyable
    TerrainClientHandler(const TerrainClientHandler&) = delete;
    TerrainClientHandler& operator=(const TerrainClientHandler&) = delete;

    // =========================================================================
    // INetworkHandler Interface
    // =========================================================================

    /**
     * @brief Check if this handler processes terrain event messages.
     */
    bool canHandle(MessageType type) const override;

    /**
     * @brief Handle an incoming terrain modification message.
     * @param peer Source peer ID (server).
     * @param msg The deserialized message.
     */
    void handleMessage(PeerID peer, const NetworkMessage& msg) override;

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * @brief Set the callback for terrain modification events.
     *
     * Called after each terrain modification is applied to the local grid.
     * Can be used for client-side effects (sound, particles, etc.).
     *
     * @param callback Function to call when terrain is modified.
     */
    void setEventCallback(ClientTerrainEventCallback callback);

    /**
     * @brief Set the callback for sync completion.
     *
     * Called when terrain synchronization completes (success or failure).
     *
     * @param callback Function to call when sync completes.
     */
    void setSyncCompleteCallback(SyncCompleteCallback callback);

    /**
     * @brief Set the callback for requesting snapshot fallback.
     *
     * Called when seed-based sync fails and full snapshot is needed.
     *
     * @param callback Function to call to request snapshot.
     */
    void setSnapshotFallbackCallback(SnapshotFallbackCallback callback);

    // =========================================================================
    // Sync State
    // =========================================================================

    /**
     * @brief Get the current sync state.
     */
    TerrainSyncState getSyncState() const;

    /**
     * @brief Check if terrain sync is complete.
     */
    bool isSyncComplete() const;

    /**
     * @brief Check if sync needs fallback to full snapshot.
     */
    bool needsSnapshotFallback() const;

    /**
     * @brief Get verification message to send to server.
     *
     * Call after all modifications have been applied during sync.
     *
     * @return TerrainSyncVerifyMessage to send to server.
     */
    TerrainSyncVerifyMessage createVerifyMessage() const;

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * @brief Get total events received.
     */
    std::uint64_t getEventsReceived() const { return m_eventsReceived; }

    /**
     * @brief Get total events successfully applied.
     */
    std::uint64_t getEventsApplied() const { return m_eventsApplied; }

    /**
     * @brief Get total events that failed to apply.
     */
    std::uint64_t getEventsFailed() const { return m_eventsFailed; }

private:
    /**
     * @brief Apply a terrain modification event to the local grid.
     *
     * @param event The event data to apply.
     * @return true if the modification was applied successfully.
     */
    bool applyModification(const TerrainModifiedEventData& event);

    /**
     * @brief Apply a clear operation to the local grid.
     *
     * @param area The affected area.
     * @return true if successful.
     */
    bool applyClear(const GridRect& area);

    /**
     * @brief Apply a level operation to the local grid.
     *
     * @param area The affected area.
     * @param new_elevation The new elevation value.
     * @return true if successful.
     */
    bool applyLevel(const GridRect& area, std::uint8_t new_elevation);

    // =========================================================================
    // Sync Message Handlers
    // =========================================================================

    /**
     * @brief Handle a terrain sync request from server.
     *
     * Generates terrain from seed and prepares for modification replay.
     *
     * @param request The sync request message.
     * @return true if generation succeeded, false if fallback needed.
     */
    bool handleSyncRequest(const TerrainSyncRequestMessage& request);

    /**
     * @brief Handle a terrain sync complete message.
     *
     * @param complete The sync complete message.
     */
    void handleSyncComplete(const TerrainSyncCompleteMessage& complete);

    // =========================================================================
    // Member Variables
    // =========================================================================

    TerrainGrid& m_grid;
    WaterData* m_waterData = nullptr;  // Optional, may be null for legacy usage
    ChunkDirtyTracker& m_dirtyTracker;
    ClientTerrainEventCallback m_eventCallback;
    SyncCompleteCallback m_syncCompleteCallback;
    SnapshotFallbackCallback m_snapshotFallbackCallback;

    // Network sync manager
    std::unique_ptr<TerrainNetworkSync> m_syncManager;

    // Statistics
    std::uint64_t m_eventsReceived = 0;
    std::uint64_t m_eventsApplied = 0;
    std::uint64_t m_eventsFailed = 0;
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINCLIENTHANDLER_H
