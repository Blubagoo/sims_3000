/**
 * @file TerrainNetworkHandler.cpp
 * @brief Implementation of server-side terrain modification handler.
 */

#include "sims3000/terrain/TerrainNetworkHandler.h"
#include "sims3000/terrain/TerrainModificationSystem.h"
#include "sims3000/terrain/GradeTerrainOperation.h"
#include "sims3000/terrain/TerrainTypeInfo.h"
#include "sims3000/net/NetworkServer.h"
#include "sims3000/core/Logger.h"

namespace sims3000 {
namespace terrain {

// =============================================================================
// Constructor
// =============================================================================

TerrainNetworkHandler::TerrainNetworkHandler(
    NetworkServer& server,
    TerrainGrid& grid,
    ChunkDirtyTracker& dirty_tracker,
    TerrainModificationSystem& mod_system,
    GradeTerrainOperation& grade_op,
    const TerrainHandlerConfig& config)
    : m_server(server)
    , m_grid(grid)
    , m_dirtyTracker(dirty_tracker)
    , m_modSystem(mod_system)
    , m_gradeOp(grade_op)
    , m_config(config)
{
    // Default callbacks (no-op / permissive for testing)
    m_creditsQuery = [](PlayerID) { return Credits{1000000}; };  // Default: rich players
    m_creditsModify = [](PlayerID, Credits) { return true; };    // Default: always succeed
    m_ownershipCheck = [](std::int32_t, std::int32_t, PlayerID) { return true; };  // Default: allow all
    m_peerToPlayer = [](PeerID peer) { return static_cast<PlayerID>(peer % 255 + 1); };  // Default: simple mapping
}

// =============================================================================
// INetworkHandler Interface
// =============================================================================

bool TerrainNetworkHandler::canHandle(MessageType type) const {
    return type == MessageType::TerrainModifyRequest;
}

void TerrainNetworkHandler::handleMessage(PeerID peer, const NetworkMessage& msg) {
    if (msg.getType() != MessageType::TerrainModifyRequest) {
        return;
    }

    const auto& request = static_cast<const TerrainModifyRequestMessage&>(msg);
    m_requestsReceived++;

    // Validate message format
    if (!request.isValid()) {
        sendResponse(peer, request.data.sequence_num,
                     TerrainModifyResult::InvalidOperation, 0);
        m_requestsRejected++;
        return;
    }

    const auto& data = request.data;

    // Verify player ID matches the peer (security check)
    PlayerID expectedPlayer = getPlayerIdFromPeer(peer);
    if (data.player_id != expectedPlayer && data.player_id != 0) {
        // Player ID mismatch - could be spoofed request
        LOG_WARN("TerrainNetworkHandler: Player ID mismatch. Peer %u claimed player %u",
                 peer, static_cast<unsigned>(data.player_id));
        sendResponse(peer, data.sequence_num,
                     TerrainModifyResult::NotOwner, 0);
        m_requestsRejected++;
        return;
    }

    // Use the expected player ID for all operations
    PlayerID player_id = expectedPlayer;

    // Dispatch based on operation type
    switch (data.operation) {
        case TerrainNetOpType::Clear: {
            Credits cost = 0;
            TerrainModifyResult result = validateClearRequest(data, cost);

            if (result != TerrainModifyResult::Success) {
                sendResponse(peer, data.sequence_num, result, 0);
                m_requestsRejected++;
                return;
            }

            // Check funds
            Credits balance = m_creditsQuery(player_id);
            if (balance < cost && cost > 0) {
                sendResponse(peer, data.sequence_num,
                             TerrainModifyResult::InsufficientFunds, 0);
                m_requestsRejected++;
                return;
            }

            // Check authority
            if (!hasAuthority(data.x, data.y, player_id)) {
                sendResponse(peer, data.sequence_num,
                             TerrainModifyResult::NotOwner, 0);
                m_requestsRejected++;
                return;
            }

            // Apply the operation
            applyClearOperation(peer, data, cost);
            m_requestsApproved++;
            break;
        }

        case TerrainNetOpType::Grade: {
            // For grade, we need the ECS registry - this is a simplified version
            // In a full implementation, the handler would have access to the registry
            Credits cost = 0;
            TerrainModifyResult result = validateGradeRequest(data, cost);

            if (result != TerrainModifyResult::Success) {
                sendResponse(peer, data.sequence_num, result, 0);
                m_requestsRejected++;
                return;
            }

            // Check funds
            Credits balance = m_creditsQuery(player_id);
            if (balance < cost && cost > 0) {
                sendResponse(peer, data.sequence_num,
                             TerrainModifyResult::InsufficientFunds, 0);
                m_requestsRejected++;
                return;
            }

            // Check authority
            if (!hasAuthority(data.x, data.y, player_id)) {
                sendResponse(peer, data.sequence_num,
                             TerrainModifyResult::NotOwner, 0);
                m_requestsRejected++;
                return;
            }

            // Check pending grade limit
            if (m_pendingGradeCount[player_id] >= m_config.max_pending_grades_per_player) {
                sendResponse(peer, data.sequence_num,
                             TerrainModifyResult::OperationInProgress, 0);
                m_requestsRejected++;
                return;
            }

            // Deduct cost upfront
            if (cost > 0 && !m_creditsModify(player_id, cost)) {
                sendResponse(peer, data.sequence_num,
                             TerrainModifyResult::InsufficientFunds, 0);
                m_requestsRejected++;
                return;
            }

            // Send success response
            sendResponse(peer, data.sequence_num,
                         TerrainModifyResult::Success, cost);

            // Track pending operation
            m_pendingGradeCount[player_id]++;

            // Broadcast initial event (grading started)
            broadcastModification(
                GridRect::singleTile(data.x, data.y),
                ModificationType::Leveled,
                player_id,
                m_grid.at(data.x, data.y).getElevation());

            m_requestsApproved++;
            break;
        }

        case TerrainNetOpType::Terraform:
            // Not implemented yet
            sendResponse(peer, data.sequence_num,
                         TerrainModifyResult::InvalidOperation, 0);
            m_requestsRejected++;
            break;

        default:
            sendResponse(peer, data.sequence_num,
                         TerrainModifyResult::InvalidOperation, 0);
            m_requestsRejected++;
            break;
    }
}

void TerrainNetworkHandler::onClientDisconnected(PeerID peer, bool /*timedOut*/) {
    PlayerID player_id = getPlayerIdFromPeer(peer);

    // Clear pending grade count for this player
    m_pendingGradeCount.erase(player_id);

    // Note: In a full implementation, we would also cancel any in-progress
    // grade operations for this player by destroying the operation entities.
}

// =============================================================================
// Configuration Callbacks
// =============================================================================

void TerrainNetworkHandler::setCreditsQuery(CreditsQueryCallback callback) {
    m_creditsQuery = std::move(callback);
}

void TerrainNetworkHandler::setCreditsModify(CreditsModifyCallback callback) {
    m_creditsModify = std::move(callback);
}

void TerrainNetworkHandler::setOwnershipCheck(OwnershipCheckCallback callback) {
    m_ownershipCheck = std::move(callback);
}

void TerrainNetworkHandler::setPeerToPlayerCallback(std::function<PlayerID(PeerID)> callback) {
    m_peerToPlayer = std::move(callback);
}

// =============================================================================
// Internal Validation Methods
// =============================================================================

TerrainModifyResult TerrainNetworkHandler::validateClearRequest(
    const TerrainModifyRequestData& data,
    Credits& out_cost) {

    // Check bounds
    if (!m_grid.in_bounds(data.x, data.y)) {
        return TerrainModifyResult::InvalidLocation;
    }

    // Get clear cost
    std::int64_t cost = m_modSystem.get_clear_cost(data.x, data.y);

    if (cost == -1) {
        // Check if already cleared or not clearable
        const auto& tile = m_grid.at(data.x, data.y);
        if (tile.isCleared()) {
            return TerrainModifyResult::AlreadyCleared;
        }
        return TerrainModifyResult::NotClearable;
    }

    out_cost = cost;
    return TerrainModifyResult::Success;
}

TerrainModifyResult TerrainNetworkHandler::validateGradeRequest(
    const TerrainModifyRequestData& data,
    Credits& out_cost) {

    // Check bounds
    if (!m_grid.in_bounds(data.x, data.y)) {
        return TerrainModifyResult::InvalidLocation;
    }

    // Check target elevation
    if (data.target_value > 31) {
        return TerrainModifyResult::InvalidLocation;
    }

    // Get current elevation
    const auto& tile = m_grid.at(data.x, data.y);
    std::uint8_t current_elevation = tile.getElevation();

    // Already at target?
    if (current_elevation == data.target_value) {
        return TerrainModifyResult::AlreadyAtElevation;
    }

    // Check if terrain type allows grading (no water/toxic types)
    TerrainType type = tile.getTerrainType();
    const auto& info = getTerrainInfo(type);
    if (!info.buildable && info.clear_cost == 0) {
        // Water or toxic type - can't grade
        return TerrainModifyResult::NotGradeable;
    }

    // Calculate cost
    std::int64_t cost = m_gradeOp.calculate_grade_cost(data.x, data.y, data.target_value);
    if (cost < 0) {
        return TerrainModifyResult::NotGradeable;
    }

    out_cost = cost;
    return TerrainModifyResult::Success;
}


bool TerrainNetworkHandler::hasAuthority(std::int32_t x, std::int32_t y, PlayerID player_id) {
    // Check direct ownership
    if (m_ownershipCheck(x, y, player_id)) {
        return true;
    }

    // Check adjacent tile ownership (if allowed)
    if (m_config.allow_adjacent_operations && ownsAdjacentTile(x, y, player_id)) {
        return true;
    }

    // Check GAME_MASTER ownership (unclaimed land)
    if (m_config.allow_unclaimed_operations && m_ownershipCheck(x, y, 0)) {
        return true;
    }

    return false;
}

bool TerrainNetworkHandler::ownsAdjacentTile(std::int32_t x, std::int32_t y, PlayerID player_id) {
    // Check 4-connected neighbors
    const int dx[] = {-1, 1, 0, 0};
    const int dy[] = {0, 0, -1, 1};

    for (int i = 0; i < 4; ++i) {
        std::int32_t nx = x + dx[i];
        std::int32_t ny = y + dy[i];

        if (m_grid.in_bounds(nx, ny) && m_ownershipCheck(nx, ny, player_id)) {
            return true;
        }
    }

    return false;
}

// =============================================================================
// Internal Application Methods
// =============================================================================

void TerrainNetworkHandler::applyClearOperation(
    PeerID peer,
    const TerrainModifyRequestData& data,
    Credits cost) {

    PlayerID player_id = getPlayerIdFromPeer(peer);

    // Deduct cost (or add credits for resource-yielding terrain)
    if (cost != 0) {
        if (!m_creditsModify(player_id, cost)) {
            sendResponse(peer, data.sequence_num,
                         TerrainModifyResult::InsufficientFunds, 0);
            return;
        }
    }

    // Apply the clear operation
    if (!m_modSystem.clear_terrain(data.x, data.y, player_id)) {
        // Should not fail if validation passed, but handle gracefully
        // Refund if we already deducted
        if (cost > 0) {
            m_creditsModify(player_id, -cost);  // Refund
        }
        sendResponse(peer, data.sequence_num,
                     TerrainModifyResult::ServerError, 0);
        return;
    }

    // Send success response
    sendResponse(peer, data.sequence_num, TerrainModifyResult::Success, cost);

    // Broadcast to all clients
    broadcastModification(
        GridRect::singleTile(data.x, data.y),
        ModificationType::Cleared,
        player_id,
        0);  // No elevation change for clear
}

void TerrainNetworkHandler::applyGradeOperation(
    PeerID /*peer*/,
    const TerrainModifyRequestData& /*data*/,
    Credits /*cost*/) {
    // Grade operations are multi-tick and handled differently.
    // The GradeTerrainOperation class creates an entity that is ticked
    // each simulation frame. This method would create that entity.
    //
    // In a full implementation:
    // 1. Create grade operation entity via m_gradeOp.create_grade_operation()
    // 2. Store mapping from entity to peer for completion notification
    // 3. TerrainModifiedEventMessage broadcasts happen each tick as elevation changes
    //
    // For this ticket, we handle grade in handleMessage directly.
}

void TerrainNetworkHandler::sendResponse(
    PeerID peer,
    std::uint32_t sequence_num,
    TerrainModifyResult result,
    Credits cost_applied) {

    TerrainModifyResponseMessage response;
    response.data.sequence_num = sequence_num;
    response.data.result = result;
    response.data.cost_applied = cost_applied;

    // Send to the requesting client
    m_server.sendTo(peer, response);
}

void TerrainNetworkHandler::broadcastModification(
    const GridRect& area,
    ModificationType type,
    PlayerID player_id,
    std::uint8_t new_elevation) {

    TerrainModifiedEventMessage event;
    event.data.affected_area = area;
    event.data.modification_type = type;
    event.data.new_elevation = new_elevation;
    event.data.player_id = player_id;

    // Broadcast to all connected clients
    m_server.broadcast(event);

    // Also mark chunks dirty locally (server is authoritative)
    m_dirtyTracker.markTilesDirty(area);
}

PlayerID TerrainNetworkHandler::getPlayerIdFromPeer(PeerID peer) const {
    return m_peerToPlayer(peer);
}

} // namespace terrain
} // namespace sims3000
