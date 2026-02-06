/**
 * @file InputHandler.h
 * @brief Server-side handler for player input messages.
 *
 * InputHandler processes incoming NetInputMessage from clients:
 * - Validates input against game rules
 * - Applies valid actions to server ECS
 * - Sends RejectionMessage for invalid actions
 * - Tracks pending actions per player for disconnect rollback
 *
 * Ownership: Application owns InputHandler.
 * Thread safety: All methods called from main thread only.
 */

#ifndef SIMS3000_NET_INPUTHANDLER_H
#define SIMS3000_NET_INPUTHANDLER_H

#include "sims3000/net/INetworkHandler.h"
#include "sims3000/net/InputMessage.h"
#include "sims3000/net/ServerMessages.h"
#include "sims3000/core/types.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace sims3000 {

// Forward declarations
class Registry;
class NetworkServer;

/**
 * @struct PendingAction
 * @brief Tracks an action awaiting server confirmation.
 *
 * Used for mid-action disconnect rollback per Q010 design.
 */
struct PendingAction {
    std::uint32_t sequenceNum = 0;      ///< Input sequence number
    InputType type = InputType::None;    ///< Type of action
    GridPosition targetPos{0, 0};        ///< Target position
    std::uint32_t param1 = 0;            ///< Action parameter
    EntityID createdEntity = 0;          ///< Entity created by this action (if any)
    SimulationTick tick = 0;             ///< Server tick when action was applied
    bool applied = false;                ///< Whether action has been applied to ECS
};

/**
 * @struct InputValidationResult
 * @brief Result of input validation.
 */
struct InputValidationResult {
    bool valid = false;                           ///< Whether input is valid
    RejectionReason reason = RejectionReason::None;  ///< Reason if invalid
    std::string message;                          ///< Human-readable error message
};

/**
 * @class InputHandler
 * @brief Server-side handler for player input messages.
 *
 * Implements INetworkHandler to receive NetInputMessage from clients.
 * Validates inputs against game rules and either applies them to the
 * server ECS or sends a RejectionMessage back to the client.
 *
 * Example usage:
 * @code
 *   Registry registry;
 *   NetworkServer server(...);
 *   InputHandler inputHandler(registry, server);
 *
 *   // Register with server
 *   server.registerHandler(&inputHandler);
 *
 *   // Server main loop
 *   while (running) {
 *       server.update(deltaTime);
 *       // InputHandler automatically processes input messages
 *   }
 * @endcode
 */
class InputHandler : public INetworkHandler {
public:
    /// Callback type for custom input validation
    using ValidationCallback = std::function<InputValidationResult(
        PlayerID playerId,
        const InputMessage& input
    )>;

    /// Callback type for input application
    using ApplyCallback = std::function<EntityID(
        PlayerID playerId,
        const InputMessage& input,
        Registry& registry
    )>;

    /**
     * @brief Construct an InputHandler.
     * @param registry Reference to the ECS registry for applying actions.
     * @param server Reference to NetworkServer for sending responses.
     */
    InputHandler(Registry& registry, NetworkServer& server);

    ~InputHandler() override = default;

    // Non-copyable
    InputHandler(const InputHandler&) = delete;
    InputHandler& operator=(const InputHandler&) = delete;

    // =========================================================================
    // INetworkHandler interface
    // =========================================================================

    /**
     * @brief Check if this handler processes Input messages.
     */
    bool canHandle(MessageType type) const override {
        return type == MessageType::Input;
    }

    /**
     * @brief Handle an incoming input message.
     * @param peer Source peer ID.
     * @param msg The deserialized message (must be NetInputMessage).
     */
    void handleMessage(PeerID peer, const NetworkMessage& msg) override;

    /**
     * @brief Called when a client disconnects.
     *
     * Rolls back any pending actions from this player per Q010.
     */
    void onClientDisconnected(PeerID peer, bool timedOut) override;

    // =========================================================================
    // Validation and Application
    // =========================================================================

    /**
     * @brief Set custom validation callback for an input type.
     * @param type Input type to customize validation for.
     * @param callback Validation function.
     *
     * The default validation is permissive. Use this to add game-specific rules.
     */
    void setValidator(InputType type, ValidationCallback callback);

    /**
     * @brief Set custom application callback for an input type.
     * @param type Input type to customize application for.
     * @param callback Application function.
     *
     * The default application creates entities or modifies the registry.
     */
    void setApplicator(InputType type, ApplyCallback callback);

    // =========================================================================
    // Pending Action Management
    // =========================================================================

    /**
     * @brief Get pending actions for a player.
     * @param playerId Player to query.
     * @return Vector of pending actions (empty if none).
     */
    std::vector<PendingAction> getPendingActions(PlayerID playerId) const;

    /**
     * @brief Clear pending actions for a player.
     *
     * Called after actions are confirmed or when player disconnects.
     */
    void clearPendingActions(PlayerID playerId);

    /**
     * @brief Get the number of pending actions across all players.
     */
    std::size_t getTotalPendingCount() const;

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * @brief Get total inputs received.
     */
    std::uint64_t getInputsReceived() const { return m_inputsReceived; }

    /**
     * @brief Get total inputs accepted.
     */
    std::uint64_t getInputsAccepted() const { return m_inputsAccepted; }

    /**
     * @brief Get total inputs rejected.
     */
    std::uint64_t getInputsRejected() const { return m_inputsRejected; }

private:
    /**
     * @brief Validate an input message.
     * @param playerId Source player.
     * @param input Input message to validate.
     * @return Validation result.
     */
    InputValidationResult validateInput(PlayerID playerId, const InputMessage& input);

    /**
     * @brief Apply a validated input to the ECS.
     * @param playerId Source player.
     * @param input Input to apply.
     * @return Created entity ID, or 0 if no entity was created.
     */
    EntityID applyInput(PlayerID playerId, const InputMessage& input);

    /**
     * @brief Send a rejection message to the client.
     * @param peer Target peer.
     * @param sequenceNum Input sequence number being rejected.
     * @param reason Rejection reason code.
     * @param message Human-readable explanation.
     */
    void sendRejection(PeerID peer, std::uint32_t sequenceNum,
                       RejectionReason reason, const std::string& message);

    /**
     * @brief Rollback a single pending action.
     * @param action The action to rollback.
     */
    void rollbackAction(const PendingAction& action);

    /**
     * @brief Get player ID from peer ID.
     * @param peer Peer ID to look up.
     * @return Player ID, or 0 if not found.
     */
    PlayerID getPlayerIdFromPeer(PeerID peer) const;

    // =========================================================================
    // Default Validators
    // =========================================================================

    InputValidationResult validatePlaceBuilding(PlayerID playerId, const InputMessage& input);
    InputValidationResult validateDemolish(PlayerID playerId, const InputMessage& input);
    InputValidationResult validateSetZone(PlayerID playerId, const InputMessage& input);
    InputValidationResult validatePlaceInfrastructure(PlayerID playerId, const InputMessage& input);

    // =========================================================================
    // Default Applicators
    // =========================================================================

    EntityID applyPlaceBuilding(PlayerID playerId, const InputMessage& input);
    EntityID applyDemolish(PlayerID playerId, const InputMessage& input);
    EntityID applySetZone(PlayerID playerId, const InputMessage& input);
    EntityID applyPlaceInfrastructure(PlayerID playerId, const InputMessage& input);

    // =========================================================================
    // State
    // =========================================================================

    Registry& m_registry;
    NetworkServer& m_server;

    /// Pending actions per player
    std::unordered_map<PlayerID, std::vector<PendingAction>> m_pendingActions;

    /// Custom validators by input type
    std::unordered_map<InputType, ValidationCallback> m_validators;

    /// Custom applicators by input type
    std::unordered_map<InputType, ApplyCallback> m_applicators;

    /// Statistics
    std::uint64_t m_inputsReceived = 0;
    std::uint64_t m_inputsAccepted = 0;
    std::uint64_t m_inputsRejected = 0;
};

} // namespace sims3000

#endif // SIMS3000_NET_INPUTHANDLER_H
