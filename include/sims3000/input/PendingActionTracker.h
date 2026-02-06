/**
 * @file PendingActionTracker.h
 * @brief Client-side tracking of pending player actions.
 *
 * PendingActionTracker manages actions that have been sent to the server
 * but not yet confirmed. Provides:
 * - Visual state information for ghost building rendering
 * - Timeout detection for pending actions
 * - Rejection feedback handling per Q015 design
 *
 * Ownership: Application owns PendingActionTracker.
 * Thread safety: All methods called from main thread only.
 */

#ifndef SIMS3000_INPUT_PENDINGACTIONTRACKER_H
#define SIMS3000_INPUT_PENDINGACTIONTRACKER_H

#include "sims3000/net/InputMessage.h"
#include "sims3000/net/ServerMessages.h"
#include "sims3000/core/types.h"

#include <queue>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <cstdint>
#include <string>
#include <optional>

namespace sims3000 {

/**
 * @enum PendingActionState
 * @brief State of a pending action.
 */
enum class PendingActionState : std::uint8_t {
    Pending,     ///< Waiting for server response
    Confirmed,   ///< Server accepted the action
    Rejected,    ///< Server rejected the action
    TimedOut     ///< No response within timeout
};

/**
 * @struct ClientPendingAction
 * @brief Client-side pending action data.
 */
struct ClientPendingAction {
    std::uint32_t sequenceNum = 0;       ///< Input sequence number
    InputType type = InputType::None;     ///< Type of action
    GridPosition targetPos{0, 0};         ///< Target position for rendering
    std::uint32_t param1 = 0;             ///< Action parameter (building type, etc.)
    PendingActionState state = PendingActionState::Pending;  ///< Current state
    RejectionReason rejectionReason = RejectionReason::None;  ///< Reason if rejected
    std::string rejectionMessage;         ///< Human-readable rejection message

    // Timing
    std::chrono::steady_clock::time_point sentTime;  ///< When action was sent
    std::chrono::steady_clock::time_point resolvedTime;  ///< When action was resolved
};

/**
 * @struct RejectionFeedback
 * @brief Rejection notification for UI display per Q015.
 */
struct RejectionFeedback {
    GridPosition position{0, 0};          ///< Position for visual feedback
    RejectionReason reason = RejectionReason::None;  ///< Reason code
    std::string message;                  ///< User-facing message
    std::chrono::steady_clock::time_point timestamp;  ///< When rejection occurred
    bool acknowledged = false;            ///< Whether user has seen this
};

/**
 * @class PendingActionTracker
 * @brief Tracks pending actions and provides feedback state.
 *
 * Example usage:
 * @code
 *   PendingActionTracker tracker;
 *
 *   // When sending input to server
 *   InputMessage input;
 *   input.type = InputType::PlaceBuilding;
 *   input.targetPos = {10, 20};
 *   input.param1 = buildingType;
 *   input.sequenceNum = client.getNextSequence();
 *
 *   tracker.trackAction(input);
 *   client.queueInput(input);
 *
 *   // In render loop - draw ghost buildings
 *   for (const auto& pending : tracker.getPendingAtPosition({10, 20})) {
 *       drawGhostBuilding(pending.targetPos, pending.param1);
 *   }
 *
 *   // When receiving rejection from server
 *   tracker.onRejection(rejectionMsg);
 *
 *   // Display rejections in UI
 *   while (auto feedback = tracker.pollRejectionFeedback()) {
 *       showRejectionNotification(*feedback);
 *   }
 * @endcode
 */
class PendingActionTracker {
public:
    /// Callback for when an action is rejected
    using RejectionCallback = std::function<void(const ClientPendingAction&)>;

    /// Default timeout for pending actions (5 seconds)
    static constexpr std::chrono::milliseconds DEFAULT_TIMEOUT{5000};

    /// How long to keep resolved actions for UI feedback (2 seconds)
    static constexpr std::chrono::milliseconds RESOLVED_RETENTION{2000};

    PendingActionTracker();
    ~PendingActionTracker() = default;

    // Non-copyable
    PendingActionTracker(const PendingActionTracker&) = delete;
    PendingActionTracker& operator=(const PendingActionTracker&) = delete;

    // =========================================================================
    // Action Tracking
    // =========================================================================

    /**
     * @brief Start tracking a pending action.
     * @param input The input message being sent to the server.
     */
    void trackAction(const InputMessage& input);

    /**
     * @brief Mark an action as confirmed.
     * @param sequenceNum The sequence number of the confirmed action.
     */
    void confirmAction(std::uint32_t sequenceNum);

    /**
     * @brief Handle a rejection message from the server.
     * @param rejection The rejection message.
     */
    void onRejection(const RejectionMessage& rejection);

    /**
     * @brief Update timeout detection and cleanup.
     *
     * Call once per frame.
     */
    void update();

    // =========================================================================
    // State Queries
    // =========================================================================

    /**
     * @brief Get all pending actions at a specific position.
     * @param pos Grid position to query.
     * @return Vector of pending actions at that position.
     */
    std::vector<ClientPendingAction> getPendingAtPosition(GridPosition pos) const;

    /**
     * @brief Get all currently pending actions.
     */
    std::vector<ClientPendingAction> getAllPending() const;

    /**
     * @brief Check if there's a pending action at a position.
     * @param pos Position to check.
     * @return true if there's at least one pending action.
     */
    bool hasPendingAt(GridPosition pos) const;

    /**
     * @brief Get total count of pending actions.
     */
    std::size_t getPendingCount() const { return m_pending.size(); }

    // =========================================================================
    // Rejection Feedback (Q015)
    // =========================================================================

    /**
     * @brief Poll for the next rejection feedback notification.
     * @return Rejection feedback if available, nullopt otherwise.
     */
    std::optional<RejectionFeedback> pollRejectionFeedback();

    /**
     * @brief Get count of unacknowledged rejections.
     */
    std::size_t getUnacknowledgedRejectionCount() const;

    /**
     * @brief Mark all rejections as acknowledged.
     */
    void acknowledgeAllRejections();

    /**
     * @brief Set callback for rejection events.
     * @param callback Function called when an action is rejected.
     */
    void setRejectionCallback(RejectionCallback callback) {
        m_rejectionCallback = std::move(callback);
    }

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * @brief Set the timeout for pending actions.
     * @param timeout Timeout duration.
     */
    void setTimeout(std::chrono::milliseconds timeout) {
        m_timeout = timeout;
    }

    /**
     * @brief Clear all tracking data.
     */
    void clear();

private:
    /// Pending actions by sequence number
    std::unordered_map<std::uint32_t, ClientPendingAction> m_pending;

    /// Rejection feedback queue
    std::queue<RejectionFeedback> m_rejectionFeedback;

    /// Rejection callback
    RejectionCallback m_rejectionCallback;

    /// Timeout configuration
    std::chrono::milliseconds m_timeout = DEFAULT_TIMEOUT;
};

} // namespace sims3000

#endif // SIMS3000_INPUT_PENDINGACTIONTRACKER_H
