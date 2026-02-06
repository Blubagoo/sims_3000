/**
 * @file InputHandler.cpp
 * @brief Server-side input handler implementation.
 */

#include "sims3000/net/InputHandler.h"
#include "sims3000/net/NetworkServer.h"
#include "sims3000/net/ClientMessages.h"
#include "sims3000/ecs/Registry.h"
#include "sims3000/ecs/Components.h"
#include "sims3000/core/Logger.h"

namespace sims3000 {

// =============================================================================
// Constructor
// =============================================================================

InputHandler::InputHandler(Registry& registry, NetworkServer& server)
    : m_registry(registry)
    , m_server(server)
{
    LOG_DEBUG("InputHandler created");
}

// =============================================================================
// INetworkHandler Implementation
// =============================================================================

void InputHandler::handleMessage(PeerID peer, const NetworkMessage& msg) {
    // Verify message type
    if (msg.getType() != MessageType::Input) {
        LOG_WARN("InputHandler received non-input message type %u",
                 static_cast<unsigned>(msg.getType()));
        return;
    }

    const auto& netInput = static_cast<const NetInputMessage&>(msg);
    m_inputsReceived++;

    // Validate the message structure
    if (!netInput.isValid()) {
        LOG_WARN("Invalid input message from peer %u", peer);
        sendRejection(peer, netInput.input.sequenceNum,
                     RejectionReason::InvalidInput,
                     "Malformed input message");
        m_inputsRejected++;
        return;
    }

    // Get player ID from the server's client tracking
    PlayerID playerId = getPlayerIdFromPeer(peer);
    if (playerId == 0) {
        LOG_WARN("Input from unknown peer %u", peer);
        sendRejection(peer, netInput.input.sequenceNum,
                     RejectionReason::ActionNotAllowed,
                     "Player not connected");
        m_inputsRejected++;
        return;
    }

    // Verify player ID matches (server-authoritative check)
    if (netInput.input.playerId != playerId) {
        LOG_WARN("Input playerId mismatch: got %u, expected %u",
                 netInput.input.playerId, playerId);
        // Accept the input but use the verified playerId
    }

    const InputMessage& input = netInput.input;

    // Skip client-only input types
    if (input.type == InputType::CameraMove ||
        input.type == InputType::CameraZoom) {
        // Camera inputs should not be sent to server
        LOG_DEBUG("Ignoring client-only input type %u", static_cast<unsigned>(input.type));
        return;
    }

    // Validate the input against game rules
    InputValidationResult result = validateInput(playerId, input);

    if (!result.valid) {
        LOG_INFO("Rejecting input seq %u from player %u: %s",
                 input.sequenceNum, playerId, result.message.c_str());
        sendRejection(peer, input.sequenceNum, result.reason, result.message);
        m_inputsRejected++;
        return;
    }

    // Apply the input to the ECS
    EntityID createdEntity = applyInput(playerId, input);

    // Track as pending action for potential rollback
    PendingAction pending;
    pending.sequenceNum = input.sequenceNum;
    pending.type = input.type;
    pending.targetPos = input.targetPos;
    pending.param1 = input.param1;
    pending.createdEntity = createdEntity;
    pending.tick = m_server.getCurrentTick();
    pending.applied = true;

    m_pendingActions[playerId].push_back(pending);

    m_inputsAccepted++;

    LOG_DEBUG("Applied input seq %u from player %u (type %u, created entity %u)",
              input.sequenceNum, playerId, static_cast<unsigned>(input.type),
              createdEntity);
}

void InputHandler::onClientDisconnected(PeerID peer, bool timedOut) {
    PlayerID playerId = getPlayerIdFromPeer(peer);
    if (playerId == 0) {
        return;
    }

    // Per Q010: Rollback pending actions on disconnect
    auto it = m_pendingActions.find(playerId);
    if (it == m_pendingActions.end() || it->second.empty()) {
        return;
    }

    LOG_INFO("Rolling back %zu pending actions for player %u (disconnect: %s)",
             it->second.size(), playerId, timedOut ? "timeout" : "graceful");

    // Rollback in reverse order (most recent first)
    auto& actions = it->second;
    for (auto rit = actions.rbegin(); rit != actions.rend(); ++rit) {
        if (rit->applied) {
            rollbackAction(*rit);
        }
    }

    // Clear the pending actions
    m_pendingActions.erase(it);
}

// =============================================================================
// Validation and Application Configuration
// =============================================================================

void InputHandler::setValidator(InputType type, ValidationCallback callback) {
    m_validators[type] = std::move(callback);
}

void InputHandler::setApplicator(InputType type, ApplyCallback callback) {
    m_applicators[type] = std::move(callback);
}

// =============================================================================
// Pending Action Management
// =============================================================================

std::vector<PendingAction> InputHandler::getPendingActions(PlayerID playerId) const {
    auto it = m_pendingActions.find(playerId);
    if (it != m_pendingActions.end()) {
        return it->second;
    }
    return {};
}

void InputHandler::clearPendingActions(PlayerID playerId) {
    m_pendingActions.erase(playerId);
}

std::size_t InputHandler::getTotalPendingCount() const {
    std::size_t total = 0;
    for (const auto& [_, actions] : m_pendingActions) {
        total += actions.size();
    }
    return total;
}

// =============================================================================
// Input Validation
// =============================================================================

InputValidationResult InputHandler::validateInput(PlayerID playerId, const InputMessage& input) {
    // Check for custom validator first
    auto it = m_validators.find(input.type);
    if (it != m_validators.end()) {
        return it->second(playerId, input);
    }

    // Default validation by type
    switch (input.type) {
        case InputType::PlaceBuilding:
            return validatePlaceBuilding(playerId, input);

        case InputType::DemolishBuilding:
            return validateDemolish(playerId, input);

        case InputType::SetZone:
        case InputType::ClearZone:
            return validateSetZone(playerId, input);

        case InputType::PlaceRoad:
        case InputType::PlacePipe:
        case InputType::PlacePowerLine:
            return validatePlaceInfrastructure(playerId, input);

        case InputType::SetTaxRate:
        case InputType::TakeLoan:
        case InputType::RepayLoan:
        case InputType::PauseGame:
        case InputType::SetGameSpeed:
        case InputType::UpgradeBuilding:
            // Economic and game control actions - accept by default
            return {true, RejectionReason::None, ""};

        case InputType::None:
            return {false, RejectionReason::InvalidInput, "Invalid input type"};

        default:
            // Unknown input type - reject
            return {false, RejectionReason::InvalidInput, "Unknown input type"};
    }
}

InputValidationResult InputHandler::validatePlaceBuilding(PlayerID playerId, const InputMessage& input) {
    (void)playerId;  // Will be used for ownership checks

    // Check building type is valid
    if (input.param1 == 0) {
        return {false, RejectionReason::InvalidBuildingType, "Invalid building type"};
    }

    // Check position is within map bounds
    // TODO: Get actual map size from server config
    const int16_t MAP_SIZE = 256;  // Default medium map
    if (input.targetPos.x < 0 || input.targetPos.x >= MAP_SIZE ||
        input.targetPos.y < 0 || input.targetPos.y >= MAP_SIZE) {
        return {false, RejectionReason::InvalidLocation, "Position out of bounds"};
    }

    // TODO: Check tile ownership - player must own the tile
    // TODO: Check if area is occupied
    // TODO: Check if player has sufficient funds

    return {true, RejectionReason::None, ""};
}

InputValidationResult InputHandler::validateDemolish(PlayerID playerId, const InputMessage& input) {
    (void)playerId;
    (void)input;

    // TODO: Verify target entity exists
    // TODO: Verify player owns the building

    return {true, RejectionReason::None, ""};
}

InputValidationResult InputHandler::validateSetZone(PlayerID playerId, const InputMessage& input) {
    (void)playerId;

    // Validate zone type
    std::uint8_t zoneType = static_cast<std::uint8_t>(input.param1);
    if (zoneType > 3) {  // 0=none, 1=residential, 2=commercial, 3=industrial
        return {false, RejectionReason::ZoneConflict, "Invalid zone type"};
    }

    // Check position bounds
    const int16_t MAP_SIZE = 256;
    if (input.targetPos.x < 0 || input.targetPos.x >= MAP_SIZE ||
        input.targetPos.y < 0 || input.targetPos.y >= MAP_SIZE) {
        return {false, RejectionReason::InvalidLocation, "Position out of bounds"};
    }

    return {true, RejectionReason::None, ""};
}

InputValidationResult InputHandler::validatePlaceInfrastructure(PlayerID playerId, const InputMessage& input) {
    (void)playerId;

    // Check position bounds
    const int16_t MAP_SIZE = 256;
    if (input.targetPos.x < 0 || input.targetPos.x >= MAP_SIZE ||
        input.targetPos.y < 0 || input.targetPos.y >= MAP_SIZE) {
        return {false, RejectionReason::InvalidLocation, "Position out of bounds"};
    }

    return {true, RejectionReason::None, ""};
}

// =============================================================================
// Input Application
// =============================================================================

EntityID InputHandler::applyInput(PlayerID playerId, const InputMessage& input) {
    // Check for custom applicator first
    auto it = m_applicators.find(input.type);
    if (it != m_applicators.end()) {
        return it->second(playerId, input, m_registry);
    }

    // Default application by type
    switch (input.type) {
        case InputType::PlaceBuilding:
            return applyPlaceBuilding(playerId, input);

        case InputType::DemolishBuilding:
            return applyDemolish(playerId, input);

        case InputType::SetZone:
        case InputType::ClearZone:
            return applySetZone(playerId, input);

        case InputType::PlaceRoad:
        case InputType::PlacePipe:
        case InputType::PlacePowerLine:
            return applyPlaceInfrastructure(playerId, input);

        default:
            // Other input types don't create entities
            return 0;
    }
}

EntityID InputHandler::applyPlaceBuilding(PlayerID playerId, const InputMessage& input) {
    // Create a new entity for the building
    EntityID entity = m_registry.create();

    // Add position component
    PositionComponent pos;
    pos.pos = input.targetPos;
    pos.elevation = 0;  // Ground level by default
    m_registry.emplace<PositionComponent>(entity, pos);

    // Add ownership component
    OwnershipComponent ownership;
    ownership.owner = playerId;
    ownership.state = OwnershipState::Owned;
    ownership.state_changed_at = m_server.getCurrentTick();
    m_registry.emplace<OwnershipComponent>(entity, ownership);

    // Add building component
    BuildingComponent building;
    building.buildingType = input.param1;
    building.level = 1;
    building.health = 100;
    building.flags = 0;
    m_registry.emplace<BuildingComponent>(entity, building);

    LOG_DEBUG("Created building entity %u at (%d, %d) for player %u",
              entity, input.targetPos.x, input.targetPos.y, playerId);

    return entity;
}

EntityID InputHandler::applyDemolish(PlayerID playerId, const InputMessage& input) {
    (void)playerId;

    // param1 contains the entity ID to demolish
    EntityID targetEntity = static_cast<EntityID>(input.param1);

    if (m_registry.valid(targetEntity)) {
        m_registry.destroy(targetEntity);
        LOG_DEBUG("Demolished entity %u", targetEntity);
    }

    return 0;  // Demolish doesn't create entities
}

EntityID InputHandler::applySetZone(PlayerID playerId, const InputMessage& input) {
    // Create or update zone entity at position
    // For now, create a new entity with zone component
    EntityID entity = m_registry.create();

    PositionComponent pos;
    pos.pos = input.targetPos;
    pos.elevation = 0;
    m_registry.emplace<PositionComponent>(entity, pos);

    OwnershipComponent ownership;
    ownership.owner = playerId;
    ownership.state = OwnershipState::Owned;
    ownership.state_changed_at = m_server.getCurrentTick();
    m_registry.emplace<OwnershipComponent>(entity, ownership);

    ZoneComponent zone;
    zone.zoneType = static_cast<std::uint8_t>(input.param1);
    zone.density = static_cast<std::uint8_t>(input.param2);
    if (zone.density == 0) zone.density = 1;  // Default to low density
    zone.desirability = 50;
    m_registry.emplace<ZoneComponent>(entity, zone);

    LOG_DEBUG("Created zone entity %u at (%d, %d) type %u for player %u",
              entity, input.targetPos.x, input.targetPos.y, zone.zoneType, playerId);

    return entity;
}

EntityID InputHandler::applyPlaceInfrastructure(PlayerID playerId, const InputMessage& input) {
    // Create infrastructure entity
    EntityID entity = m_registry.create();

    PositionComponent pos;
    pos.pos = input.targetPos;
    pos.elevation = 0;
    m_registry.emplace<PositionComponent>(entity, pos);

    OwnershipComponent ownership;
    ownership.owner = playerId;
    ownership.state = OwnershipState::Owned;
    ownership.state_changed_at = m_server.getCurrentTick();
    m_registry.emplace<OwnershipComponent>(entity, ownership);

    // Add transport component for roads
    if (input.type == InputType::PlaceRoad) {
        TransportComponent transport;
        transport.roadConnectionId = 0;  // Will be assigned by transport system
        transport.trafficLoad = 0;
        transport.accessibility = 50;
        m_registry.emplace<TransportComponent>(entity, transport);
    }

    // Add energy component for power lines
    if (input.type == InputType::PlacePowerLine) {
        EnergyComponent energy;
        energy.consumption = 0;  // Conduits don't consume
        energy.capacity = 100;   // Transfer capacity
        energy.connected = 0;
        m_registry.emplace<EnergyComponent>(entity, energy);
    }

    LOG_DEBUG("Created infrastructure entity %u at (%d, %d) for player %u",
              entity, input.targetPos.x, input.targetPos.y, playerId);

    return entity;
}

// =============================================================================
// Rejection Sending
// =============================================================================

void InputHandler::sendRejection(PeerID peer, std::uint32_t sequenceNum,
                                  RejectionReason reason, const std::string& message) {
    RejectionMessage rejection;
    rejection.inputSequenceNum = sequenceNum;
    rejection.reason = reason;
    rejection.message = message.empty()
        ? RejectionMessage::getDefaultMessage(reason)
        : message;

    m_server.sendTo(peer, rejection, ChannelID::Reliable);
}

// =============================================================================
// Rollback
// =============================================================================

void InputHandler::rollbackAction(const PendingAction& action) {
    // Only rollback actions that created entities
    if (action.createdEntity != 0 && m_registry.valid(action.createdEntity)) {
        LOG_DEBUG("Rolling back action seq %u: destroying entity %u",
                  action.sequenceNum, action.createdEntity);
        m_registry.destroy(action.createdEntity);
    }
}

// =============================================================================
// Helpers
// =============================================================================

PlayerID InputHandler::getPlayerIdFromPeer(PeerID peer) const {
    const ClientConnection* client = m_server.getClient(peer);
    if (client) {
        return client->playerId;
    }
    return 0;
}

} // namespace sims3000
