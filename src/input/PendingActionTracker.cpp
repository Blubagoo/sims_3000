/**
 * @file PendingActionTracker.cpp
 * @brief Client-side pending action tracker implementation.
 */

#include "sims3000/input/PendingActionTracker.h"

namespace sims3000 {

PendingActionTracker::PendingActionTracker() = default;

// =============================================================================
// Action Tracking
// =============================================================================

void PendingActionTracker::trackAction(const InputMessage& input) {
    ClientPendingAction action;
    action.sequenceNum = input.sequenceNum;
    action.type = input.type;
    action.targetPos = input.targetPos;
    action.param1 = input.param1;
    action.state = PendingActionState::Pending;
    action.sentTime = std::chrono::steady_clock::now();

    m_pending[input.sequenceNum] = action;
}

void PendingActionTracker::confirmAction(std::uint32_t sequenceNum) {
    auto it = m_pending.find(sequenceNum);
    if (it != m_pending.end()) {
        it->second.state = PendingActionState::Confirmed;
        it->second.resolvedTime = std::chrono::steady_clock::now();
        // Remove immediately on confirmation - no need to keep
        m_pending.erase(it);
    }
}

void PendingActionTracker::onRejection(const RejectionMessage& rejection) {
    auto it = m_pending.find(rejection.inputSequenceNum);
    if (it == m_pending.end()) {
        // May have already timed out or been processed
        return;
    }

    ClientPendingAction& action = it->second;
    action.state = PendingActionState::Rejected;
    action.rejectionReason = rejection.reason;
    action.rejectionMessage = rejection.message;
    action.resolvedTime = std::chrono::steady_clock::now();

    // Create feedback for UI
    RejectionFeedback feedback;
    feedback.position = action.targetPos;
    feedback.reason = rejection.reason;
    feedback.message = rejection.message;
    feedback.timestamp = std::chrono::steady_clock::now();
    feedback.acknowledged = false;

    m_rejectionFeedback.push(feedback);

    // Notify callback
    if (m_rejectionCallback) {
        m_rejectionCallback(action);
    }

    // Remove from pending
    m_pending.erase(it);
}

void PendingActionTracker::update() {
    auto now = std::chrono::steady_clock::now();

    // Check for timed-out actions
    std::vector<std::uint32_t> timedOut;
    for (auto& [seq, action] : m_pending) {
        if (action.state == PendingActionState::Pending) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - action.sentTime
            );

            if (elapsed >= m_timeout) {
                action.state = PendingActionState::TimedOut;
                action.resolvedTime = now;
                timedOut.push_back(seq);

                // Create feedback for timeout
                RejectionFeedback feedback;
                feedback.position = action.targetPos;
                feedback.reason = RejectionReason::ServerBusy;
                feedback.message = "Action timed out - server may be busy";
                feedback.timestamp = now;
                feedback.acknowledged = false;

                m_rejectionFeedback.push(feedback);

                if (m_rejectionCallback) {
                    m_rejectionCallback(action);
                }
            }
        }
    }

    // Remove timed-out actions
    for (std::uint32_t seq : timedOut) {
        m_pending.erase(seq);
    }
}

// =============================================================================
// State Queries
// =============================================================================

std::vector<ClientPendingAction> PendingActionTracker::getPendingAtPosition(GridPosition pos) const {
    std::vector<ClientPendingAction> result;
    for (const auto& [_, action] : m_pending) {
        if (action.state == PendingActionState::Pending &&
            action.targetPos == pos) {
            result.push_back(action);
        }
    }
    return result;
}

std::vector<ClientPendingAction> PendingActionTracker::getAllPending() const {
    std::vector<ClientPendingAction> result;
    result.reserve(m_pending.size());
    for (const auto& [_, action] : m_pending) {
        if (action.state == PendingActionState::Pending) {
            result.push_back(action);
        }
    }
    return result;
}

bool PendingActionTracker::hasPendingAt(GridPosition pos) const {
    for (const auto& [_, action] : m_pending) {
        if (action.state == PendingActionState::Pending &&
            action.targetPos == pos) {
            return true;
        }
    }
    return false;
}

// =============================================================================
// Rejection Feedback
// =============================================================================

std::optional<RejectionFeedback> PendingActionTracker::pollRejectionFeedback() {
    if (m_rejectionFeedback.empty()) {
        return std::nullopt;
    }

    RejectionFeedback feedback = std::move(m_rejectionFeedback.front());
    m_rejectionFeedback.pop();
    return feedback;
}

std::size_t PendingActionTracker::getUnacknowledgedRejectionCount() const {
    return m_rejectionFeedback.size();
}

void PendingActionTracker::acknowledgeAllRejections() {
    while (!m_rejectionFeedback.empty()) {
        m_rejectionFeedback.pop();
    }
}

// =============================================================================
// Utility
// =============================================================================

void PendingActionTracker::clear() {
    m_pending.clear();
    while (!m_rejectionFeedback.empty()) {
        m_rejectionFeedback.pop();
    }
}

} // namespace sims3000
