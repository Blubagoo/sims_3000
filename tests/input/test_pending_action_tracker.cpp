/**
 * @file test_pending_action_tracker.cpp
 * @brief Unit tests for PendingActionTracker (Ticket 1-011).
 *
 * Tests:
 * - Action tracking and state queries
 * - Confirmation handling
 * - Rejection feedback generation
 * - Timeout detection
 * - Position-based queries for ghost rendering
 */

#include "sims3000/input/PendingActionTracker.h"
#include "sims3000/net/ServerMessages.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

using namespace sims3000;

// =============================================================================
// Test Utilities
// =============================================================================

static int testsPassed = 0;
static int testsFailed = 0;

#define TEST_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            std::cerr << "FAIL: " << msg << " (" << #expr << ")" << std::endl; \
            testsFailed++; \
            return; \
        } \
    } while(0)

#define TEST_PASS(name) \
    do { \
        std::cout << "PASS: " << name << std::endl; \
        testsPassed++; \
    } while(0)

// =============================================================================
// Basic Tracking Tests
// =============================================================================

void test_PendingActionTracker_InitialState() {
    PendingActionTracker tracker;

    TEST_ASSERT(tracker.getPendingCount() == 0, "Initial count is 0");
    TEST_ASSERT(tracker.getAllPending().empty(), "No pending actions initially");
    TEST_ASSERT(tracker.getUnacknowledgedRejectionCount() == 0, "No rejections initially");
    TEST_ASSERT(!tracker.pollRejectionFeedback().has_value(), "No feedback to poll");

    TEST_PASS("PendingActionTracker_InitialState");
}

void test_PendingActionTracker_TrackAction() {
    PendingActionTracker tracker;

    InputMessage input;
    input.tick = 100;
    input.playerId = 1;
    input.type = InputType::PlaceBuilding;
    input.sequenceNum = 42;
    input.targetPos = {10, 20};
    input.param1 = 5;

    tracker.trackAction(input);

    TEST_ASSERT(tracker.getPendingCount() == 1, "One pending action");
    TEST_ASSERT(tracker.hasPendingAt({10, 20}), "Has pending at target position");
    TEST_ASSERT(!tracker.hasPendingAt({0, 0}), "No pending at other position");

    auto pending = tracker.getAllPending();
    TEST_ASSERT(pending.size() == 1, "One pending in list");
    TEST_ASSERT(pending[0].sequenceNum == 42, "Sequence number matches");
    TEST_ASSERT(pending[0].type == InputType::PlaceBuilding, "Type matches");
    TEST_ASSERT(pending[0].targetPos.x == 10, "Target X matches");
    TEST_ASSERT(pending[0].targetPos.y == 20, "Target Y matches");
    TEST_ASSERT(pending[0].param1 == 5, "Param1 matches");
    TEST_ASSERT(pending[0].state == PendingActionState::Pending, "State is Pending");

    TEST_PASS("PendingActionTracker_TrackAction");
}

void test_PendingActionTracker_MultipleActions() {
    PendingActionTracker tracker;

    // Add multiple actions at different positions
    for (int i = 0; i < 5; i++) {
        InputMessage input;
        input.sequenceNum = static_cast<std::uint32_t>(i + 1);
        input.type = InputType::PlaceBuilding;
        input.targetPos = {static_cast<std::int16_t>(i * 10), static_cast<std::int16_t>(i * 10)};
        input.param1 = static_cast<std::uint32_t>(i);
        tracker.trackAction(input);
    }

    TEST_ASSERT(tracker.getPendingCount() == 5, "Five pending actions");
    TEST_ASSERT(tracker.hasPendingAt({0, 0}), "Has pending at (0,0)");
    TEST_ASSERT(tracker.hasPendingAt({40, 40}), "Has pending at (40,40)");
    TEST_ASSERT(!tracker.hasPendingAt({100, 100}), "No pending at (100,100)");

    TEST_PASS("PendingActionTracker_MultipleActions");
}

void test_PendingActionTracker_MultipleAtSamePosition() {
    PendingActionTracker tracker;

    // Add multiple actions at same position
    for (int i = 0; i < 3; i++) {
        InputMessage input;
        input.sequenceNum = static_cast<std::uint32_t>(i + 1);
        input.type = InputType::PlaceBuilding;
        input.targetPos = {10, 20};  // Same position
        input.param1 = static_cast<std::uint32_t>(i);
        tracker.trackAction(input);
    }

    TEST_ASSERT(tracker.getPendingCount() == 3, "Three pending actions");

    auto atPos = tracker.getPendingAtPosition({10, 20});
    TEST_ASSERT(atPos.size() == 3, "Three actions at (10, 20)");

    TEST_PASS("PendingActionTracker_MultipleAtSamePosition");
}

// =============================================================================
// Confirmation Tests
// =============================================================================

void test_PendingActionTracker_ConfirmAction() {
    PendingActionTracker tracker;

    InputMessage input;
    input.sequenceNum = 42;
    input.type = InputType::PlaceBuilding;
    input.targetPos = {10, 20};
    tracker.trackAction(input);

    TEST_ASSERT(tracker.getPendingCount() == 1, "One pending before confirm");

    tracker.confirmAction(42);

    TEST_ASSERT(tracker.getPendingCount() == 0, "None pending after confirm");
    TEST_ASSERT(!tracker.hasPendingAt({10, 20}), "No longer pending at position");

    TEST_PASS("PendingActionTracker_ConfirmAction");
}

void test_PendingActionTracker_ConfirmNonexistent() {
    PendingActionTracker tracker;

    InputMessage input;
    input.sequenceNum = 42;
    input.type = InputType::PlaceBuilding;
    input.targetPos = {10, 20};
    tracker.trackAction(input);

    // Confirm wrong sequence number - should be no-op
    tracker.confirmAction(999);

    TEST_ASSERT(tracker.getPendingCount() == 1, "Still one pending");
    TEST_ASSERT(tracker.hasPendingAt({10, 20}), "Still pending at position");

    TEST_PASS("PendingActionTracker_ConfirmNonexistent");
}

// =============================================================================
// Rejection Tests
// =============================================================================

void test_PendingActionTracker_OnRejection() {
    PendingActionTracker tracker;

    InputMessage input;
    input.sequenceNum = 42;
    input.type = InputType::PlaceBuilding;
    input.targetPos = {10, 20};
    tracker.trackAction(input);

    RejectionMessage rejection;
    rejection.inputSequenceNum = 42;
    rejection.reason = RejectionReason::InsufficientFunds;
    rejection.message = "Not enough credits";

    tracker.onRejection(rejection);

    TEST_ASSERT(tracker.getPendingCount() == 0, "None pending after rejection");
    TEST_ASSERT(tracker.getUnacknowledgedRejectionCount() == 1, "One rejection queued");

    auto feedback = tracker.pollRejectionFeedback();
    TEST_ASSERT(feedback.has_value(), "Feedback available");
    TEST_ASSERT(feedback->position.x == 10, "Feedback position X matches");
    TEST_ASSERT(feedback->position.y == 20, "Feedback position Y matches");
    TEST_ASSERT(feedback->reason == RejectionReason::InsufficientFunds, "Feedback reason matches");
    TEST_ASSERT(feedback->message == "Not enough credits", "Feedback message matches");

    TEST_ASSERT(tracker.getUnacknowledgedRejectionCount() == 0, "Queue emptied after poll");

    TEST_PASS("PendingActionTracker_OnRejection");
}

void test_PendingActionTracker_RejectionCallback() {
    PendingActionTracker tracker;

    bool callbackCalled = false;
    ClientPendingAction callbackAction;

    tracker.setRejectionCallback([&](const ClientPendingAction& action) {
        callbackCalled = true;
        callbackAction = action;
    });

    InputMessage input;
    input.sequenceNum = 42;
    input.type = InputType::PlaceBuilding;
    input.targetPos = {10, 20};
    tracker.trackAction(input);

    RejectionMessage rejection;
    rejection.inputSequenceNum = 42;
    rejection.reason = RejectionReason::AreaOccupied;
    rejection.message = "Already occupied";

    tracker.onRejection(rejection);

    TEST_ASSERT(callbackCalled, "Callback was called");
    TEST_ASSERT(callbackAction.sequenceNum == 42, "Callback received correct action");
    TEST_ASSERT(callbackAction.rejectionReason == RejectionReason::AreaOccupied, "Reason correct");

    TEST_PASS("PendingActionTracker_RejectionCallback");
}

void test_PendingActionTracker_RejectNonexistent() {
    PendingActionTracker tracker;

    InputMessage input;
    input.sequenceNum = 42;
    input.type = InputType::PlaceBuilding;
    input.targetPos = {10, 20};
    tracker.trackAction(input);

    RejectionMessage rejection;
    rejection.inputSequenceNum = 999;  // Wrong sequence
    rejection.reason = RejectionReason::InsufficientFunds;
    rejection.message = "Test";

    tracker.onRejection(rejection);

    // Original action should still be pending
    TEST_ASSERT(tracker.getPendingCount() == 1, "Still one pending");
    TEST_ASSERT(tracker.getUnacknowledgedRejectionCount() == 0, "No rejection feedback");

    TEST_PASS("PendingActionTracker_RejectNonexistent");
}

// =============================================================================
// Timeout Tests
// =============================================================================

void test_PendingActionTracker_NoTimeoutBeforeThreshold() {
    PendingActionTracker tracker;
    tracker.setTimeout(std::chrono::milliseconds(100));  // Short timeout for testing

    InputMessage input;
    input.sequenceNum = 42;
    input.type = InputType::PlaceBuilding;
    input.targetPos = {10, 20};
    tracker.trackAction(input);

    // Immediately update - should not timeout
    tracker.update();

    TEST_ASSERT(tracker.getPendingCount() == 1, "Still pending after immediate update");

    TEST_PASS("PendingActionTracker_NoTimeoutBeforeThreshold");
}

void test_PendingActionTracker_TimeoutAfterThreshold() {
    PendingActionTracker tracker;
    tracker.setTimeout(std::chrono::milliseconds(50));  // Very short timeout

    InputMessage input;
    input.sequenceNum = 42;
    input.type = InputType::PlaceBuilding;
    input.targetPos = {10, 20};
    tracker.trackAction(input);

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    tracker.update();

    TEST_ASSERT(tracker.getPendingCount() == 0, "None pending after timeout");
    TEST_ASSERT(tracker.getUnacknowledgedRejectionCount() == 1, "Timeout generated feedback");

    auto feedback = tracker.pollRejectionFeedback();
    TEST_ASSERT(feedback.has_value(), "Feedback available");
    TEST_ASSERT(feedback->reason == RejectionReason::ServerBusy, "Timeout reason is ServerBusy");

    TEST_PASS("PendingActionTracker_TimeoutAfterThreshold");
}

// =============================================================================
// Clear and Acknowledge Tests
// =============================================================================

void test_PendingActionTracker_Clear() {
    PendingActionTracker tracker;

    for (int i = 0; i < 5; i++) {
        InputMessage input;
        input.sequenceNum = static_cast<std::uint32_t>(i + 1);
        input.type = InputType::PlaceBuilding;
        input.targetPos = {static_cast<std::int16_t>(i), static_cast<std::int16_t>(i)};
        tracker.trackAction(input);
    }

    RejectionMessage rejection;
    rejection.inputSequenceNum = 1;
    rejection.reason = RejectionReason::InvalidLocation;
    rejection.message = "Test";
    tracker.onRejection(rejection);

    TEST_ASSERT(tracker.getPendingCount() == 4, "Four pending before clear");
    TEST_ASSERT(tracker.getUnacknowledgedRejectionCount() == 1, "One rejection before clear");

    tracker.clear();

    TEST_ASSERT(tracker.getPendingCount() == 0, "None pending after clear");
    TEST_ASSERT(tracker.getUnacknowledgedRejectionCount() == 0, "No rejections after clear");

    TEST_PASS("PendingActionTracker_Clear");
}

void test_PendingActionTracker_AcknowledgeAll() {
    PendingActionTracker tracker;

    // Generate multiple rejections
    for (int i = 0; i < 3; i++) {
        InputMessage input;
        input.sequenceNum = static_cast<std::uint32_t>(i + 1);
        input.type = InputType::PlaceBuilding;
        input.targetPos = {0, 0};
        tracker.trackAction(input);

        RejectionMessage rejection;
        rejection.inputSequenceNum = static_cast<std::uint32_t>(i + 1);
        rejection.reason = RejectionReason::InvalidLocation;
        rejection.message = "Test";
        tracker.onRejection(rejection);
    }

    TEST_ASSERT(tracker.getUnacknowledgedRejectionCount() == 3, "Three rejections queued");

    tracker.acknowledgeAllRejections();

    TEST_ASSERT(tracker.getUnacknowledgedRejectionCount() == 0, "All rejections acknowledged");
    TEST_ASSERT(!tracker.pollRejectionFeedback().has_value(), "No feedback to poll");

    TEST_PASS("PendingActionTracker_AcknowledgeAll");
}

// =============================================================================
// Position Query Tests
// =============================================================================

void test_PendingActionTracker_GetPendingAtPosition() {
    PendingActionTracker tracker;

    // Add actions at different positions
    InputMessage input1;
    input1.sequenceNum = 1;
    input1.type = InputType::PlaceBuilding;
    input1.targetPos = {10, 20};
    input1.param1 = 100;
    tracker.trackAction(input1);

    InputMessage input2;
    input2.sequenceNum = 2;
    input2.type = InputType::PlaceRoad;
    input2.targetPos = {10, 20};  // Same position
    input2.param1 = 200;
    tracker.trackAction(input2);

    InputMessage input3;
    input3.sequenceNum = 3;
    input3.type = InputType::SetZone;
    input3.targetPos = {30, 40};  // Different position
    input3.param1 = 300;
    tracker.trackAction(input3);

    auto at1020 = tracker.getPendingAtPosition({10, 20});
    TEST_ASSERT(at1020.size() == 2, "Two actions at (10, 20)");

    auto at3040 = tracker.getPendingAtPosition({30, 40});
    TEST_ASSERT(at3040.size() == 1, "One action at (30, 40)");

    auto atEmpty = tracker.getPendingAtPosition({0, 0});
    TEST_ASSERT(atEmpty.empty(), "No actions at (0, 0)");

    TEST_PASS("PendingActionTracker_GetPendingAtPosition");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== Pending Action Tracker Tests (Ticket 1-011) ===" << std::endl << std::endl;

    // Basic tracking tests
    test_PendingActionTracker_InitialState();
    test_PendingActionTracker_TrackAction();
    test_PendingActionTracker_MultipleActions();
    test_PendingActionTracker_MultipleAtSamePosition();

    // Confirmation tests
    test_PendingActionTracker_ConfirmAction();
    test_PendingActionTracker_ConfirmNonexistent();

    // Rejection tests
    test_PendingActionTracker_OnRejection();
    test_PendingActionTracker_RejectionCallback();
    test_PendingActionTracker_RejectNonexistent();

    // Timeout tests
    test_PendingActionTracker_NoTimeoutBeforeThreshold();
    test_PendingActionTracker_TimeoutAfterThreshold();

    // Clear and acknowledge tests
    test_PendingActionTracker_Clear();
    test_PendingActionTracker_AcknowledgeAll();

    // Position query tests
    test_PendingActionTracker_GetPendingAtPosition();

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << testsPassed << std::endl;
    std::cout << "Failed: " << testsFailed << std::endl;

    return testsFailed == 0 ? 0 : 1;
}
