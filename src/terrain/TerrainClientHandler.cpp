/**
 * @file TerrainClientHandler.cpp
 * @brief Implementation of client-side terrain modification handler.
 */

#include "sims3000/terrain/TerrainClientHandler.h"
#include "sims3000/terrain/TerrainTypes.h"
#include "sims3000/core/Logger.h"

namespace sims3000 {
namespace terrain {

// =============================================================================
// Constructors
// =============================================================================

TerrainClientHandler::TerrainClientHandler(
    TerrainGrid& grid,
    WaterData& waterData,
    ChunkDirtyTracker& dirty_tracker)
    : m_grid(grid)
    , m_waterData(&waterData)
    , m_dirtyTracker(dirty_tracker)
    , m_syncManager(std::make_unique<TerrainNetworkSync>())
{
}

TerrainClientHandler::TerrainClientHandler(
    TerrainGrid& grid,
    ChunkDirtyTracker& dirty_tracker)
    : m_grid(grid)
    , m_waterData(nullptr)
    , m_dirtyTracker(dirty_tracker)
    , m_syncManager(std::make_unique<TerrainNetworkSync>())
{
}

// =============================================================================
// INetworkHandler Interface
// =============================================================================

bool TerrainClientHandler::canHandle(MessageType type) const {
    return type == MessageType::TerrainModifiedEvent ||
           type == MessageType::TerrainSyncRequest ||
           type == MessageType::TerrainSyncComplete;
}

void TerrainClientHandler::handleMessage(PeerID /*peer*/, const NetworkMessage& msg) {
    switch (msg.getType()) {
        case MessageType::TerrainSyncRequest: {
            const auto& request = static_cast<const TerrainSyncRequestMessage&>(msg);
            handleSyncRequest(request);
            break;
        }

        case MessageType::TerrainSyncComplete: {
            const auto& complete = static_cast<const TerrainSyncCompleteMessage&>(msg);
            handleSyncComplete(complete);
            break;
        }

        case MessageType::TerrainModifiedEvent: {
            const auto& event = static_cast<const TerrainModifiedEventMessage&>(msg);
            m_eventsReceived++;

            // Apply the modification to local grid
            if (applyModification(event.data)) {
                m_eventsApplied++;

                // Fire callback for client-side effects
                if (m_eventCallback) {
                    TerrainModifiedEvent localEvent;
                    localEvent.affected_area = event.data.affected_area;
                    localEvent.modification_type = event.data.modification_type;
                    m_eventCallback(localEvent, event.data.player_id);
                }
            } else {
                m_eventsFailed++;
                LOG_WARN("TerrainClientHandler: Failed to apply terrain modification at (%d, %d)",
                         event.data.affected_area.x, event.data.affected_area.y);
            }
            break;
        }

        default:
            // Ignore unknown message types
            break;
    }
}

// =============================================================================
// Configuration
// =============================================================================

void TerrainClientHandler::setEventCallback(ClientTerrainEventCallback callback) {
    m_eventCallback = std::move(callback);
}

void TerrainClientHandler::setSyncCompleteCallback(SyncCompleteCallback callback) {
    m_syncCompleteCallback = std::move(callback);
}

void TerrainClientHandler::setSnapshotFallbackCallback(SnapshotFallbackCallback callback) {
    m_snapshotFallbackCallback = std::move(callback);
}

// =============================================================================
// Sync State
// =============================================================================

TerrainSyncState TerrainClientHandler::getSyncState() const {
    if (m_syncManager) {
        return m_syncManager->getState();
    }
    return TerrainSyncState::Idle;
}

bool TerrainClientHandler::isSyncComplete() const {
    return getSyncState() == TerrainSyncState::Complete;
}

bool TerrainClientHandler::needsSnapshotFallback() const {
    if (m_syncManager) {
        return m_syncManager->needsSnapshotFallback();
    }
    return false;
}

TerrainSyncVerifyMessage TerrainClientHandler::createVerifyMessage() const {
    if (m_syncManager) {
        return m_syncManager->createVerifyMessage(m_grid);
    }
    // Return a failure message if no sync manager
    TerrainSyncVerifyMessage msg;
    msg.data.success = 0;
    return msg;
}

// =============================================================================
// Sync Message Handlers
// =============================================================================

bool TerrainClientHandler::handleSyncRequest(const TerrainSyncRequestMessage& request) {
    LOG_INFO("TerrainClientHandler: Received sync request (seed=%llu, %u modifications)",
             static_cast<unsigned long long>(request.data.map_seed),
             request.data.modification_count);

    if (!m_syncManager) {
        LOG_ERROR("TerrainClientHandler: No sync manager available");
        return false;
    }

    // Create temporary water data if not provided
    WaterData tempWaterData;
    WaterData* waterDataPtr = m_waterData ? m_waterData : &tempWaterData;

    // Handle the sync request - generates terrain from seed
    if (!m_syncManager->handleSyncRequest(request, m_grid, *waterDataPtr)) {
        LOG_ERROR("TerrainClientHandler: Failed to generate terrain from seed");

        // Request fallback to full snapshot
        if (m_snapshotFallbackCallback) {
            m_snapshotFallbackCallback();
        }
        return false;
    }

    // Apply all modifications
    std::size_t applied = m_syncManager->applyAllModifications(m_grid);
    LOG_INFO("TerrainClientHandler: Applied %zu modifications", applied);

    // Mark all chunks dirty for re-rendering
    GridRect fullGrid;
    fullGrid.x = 0;
    fullGrid.y = 0;
    fullGrid.width = m_grid.width;
    fullGrid.height = m_grid.height;
    m_dirtyTracker.markTilesDirty(fullGrid);

    return true;
}

void TerrainClientHandler::handleSyncComplete(const TerrainSyncCompleteMessage& complete) {
    if (!m_syncManager) {
        return;
    }

    bool success = m_syncManager->handleSyncComplete(complete);

    LOG_INFO("TerrainClientHandler: Sync complete (result=%d, success=%d)",
             static_cast<int>(complete.data.result), success ? 1 : 0);

    if (!success) {
        // Need fallback to full snapshot
        if (m_snapshotFallbackCallback) {
            m_snapshotFallbackCallback();
        }
    }

    // Notify callback
    if (m_syncCompleteCallback) {
        m_syncCompleteCallback(success);
    }
}

// =============================================================================
// Internal Methods
// =============================================================================

bool TerrainClientHandler::applyModification(const TerrainModifiedEventData& event) {
    // Validate area is within bounds
    const GridRect& area = event.affected_area;

    if (area.isEmpty()) {
        return false;
    }

    // Check if any part of the area is within bounds
    if (area.x >= static_cast<std::int16_t>(m_grid.width) ||
        area.y >= static_cast<std::int16_t>(m_grid.height) ||
        area.right() <= 0 ||
        area.bottom() <= 0) {
        LOG_WARN("TerrainClientHandler: Modification area entirely out of bounds");
        return false;
    }

    // Apply based on modification type
    switch (event.modification_type) {
        case ModificationType::Cleared:
            return applyClear(area);

        case ModificationType::Leveled:
            return applyLevel(area, event.new_elevation);

        case ModificationType::Terraformed:
            // Terraformed requires changing terrain type, which would need
            // additional data. For now, just mark chunks dirty.
            m_dirtyTracker.markTilesDirty(area);
            return true;

        case ModificationType::Generated:
            // Full terrain generation - just mark all affected chunks dirty
            m_dirtyTracker.markTilesDirty(area);
            return true;

        case ModificationType::SeaLevelChanged:
            // Sea level change - mark all affected chunks dirty
            m_dirtyTracker.markTilesDirty(area);
            return true;

        default:
            LOG_WARN("TerrainClientHandler: Unknown modification type %d",
                     static_cast<int>(event.modification_type));
            return false;
    }
}

bool TerrainClientHandler::applyClear(const GridRect& area) {
    bool success = false;

    // Iterate over all tiles in the area
    for (std::int16_t y = area.y; y < area.bottom(); ++y) {
        for (std::int16_t x = area.x; x < area.right(); ++x) {
            // Skip out-of-bounds tiles
            if (!m_grid.in_bounds(x, y)) {
                continue;
            }

            // Get the tile and set cleared flag
            TerrainComponent& tile = m_grid.at(x, y);

            // Only clear if not already cleared
            if (!tile.isCleared()) {
                tile.setCleared(true);
                success = true;
            }
        }
    }

    // Mark affected chunks dirty
    if (success) {
        m_dirtyTracker.markTilesDirty(area);
    }

    return success;
}

bool TerrainClientHandler::applyLevel(const GridRect& area, std::uint8_t new_elevation) {
    bool success = false;

    // Validate elevation
    if (new_elevation > 31) {
        LOG_WARN("TerrainClientHandler: Invalid elevation %d (max 31)", new_elevation);
        return false;
    }

    // Iterate over all tiles in the area
    for (std::int16_t y = area.y; y < area.bottom(); ++y) {
        for (std::int16_t x = area.x; x < area.right(); ++x) {
            // Skip out-of-bounds tiles
            if (!m_grid.in_bounds(x, y)) {
                continue;
            }

            // Get the tile and update elevation
            TerrainComponent& tile = m_grid.at(x, y);
            tile.setElevation(new_elevation);
            success = true;
        }
    }

    // Mark affected chunks dirty
    if (success) {
        m_dirtyTracker.markTilesDirty(area);
    }

    return success;
}

} // namespace terrain
} // namespace sims3000
