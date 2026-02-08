/**
 * @file test_trade_deal_expiration.cpp
 * @brief Unit tests for trade deal expiration (Epic 8, Ticket E8-023)
 *
 * Tests cover:
 * - Expiration check: Active, Warning, Expired states
 * - Warning event at 5 cycles remaining
 * - Expiration event when cycles reach 0
 * - Combined tick-with-expiration function
 * - Renewal success with valid parameters
 * - Renewal failure: expired deal, None type, insufficient treasury
 * - Warning constant is 5
 */

#include <sims3000/port/TradeDealExpiration.h>
#include <sims3000/port/TradeDealNegotiation.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <sims3000/port/TradeEvents.h>
#include <sims3000/port/PortTypes.h>
#include <sims3000/ecs/Components.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::port;
using namespace sims3000;

// =============================================================================
// Test: Warning constant is 5
// =============================================================================

void test_warning_constant() {
    printf("Testing TRADE_DEAL_WARNING_CYCLES is 5...\n");

    assert(TRADE_DEAL_WARNING_CYCLES == 5);

    printf("  PASS: TRADE_DEAL_WARNING_CYCLES == 5\n");
}

// =============================================================================
// Test: Check expiration - Active state
// =============================================================================

void test_check_active() {
    printf("Testing check_trade_deal_expiration returns Active...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);
    assert(agreement.cycles_remaining == 500);

    ExpirationCheckResult result = check_trade_deal_expiration(agreement);
    assert(result == ExpirationCheckResult::Active);

    printf("  PASS: Active deal returns Active\n");
}

// =============================================================================
// Test: Check expiration - Warning state at exactly 5 cycles
// =============================================================================

void test_check_warning_at_5() {
    printf("Testing check_trade_deal_expiration returns Warning at 5 cycles...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);
    agreement.cycles_remaining = 5;

    ExpirationCheckResult result = check_trade_deal_expiration(agreement);
    assert(result == ExpirationCheckResult::Warning);

    printf("  PASS: 5 cycles returns Warning\n");
}

// =============================================================================
// Test: Check expiration - Warning state at 1 cycle
// =============================================================================

void test_check_warning_at_1() {
    printf("Testing check_trade_deal_expiration returns Warning at 1 cycle...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);
    agreement.cycles_remaining = 1;

    ExpirationCheckResult result = check_trade_deal_expiration(agreement);
    assert(result == ExpirationCheckResult::Warning);

    printf("  PASS: 1 cycle returns Warning\n");
}

// =============================================================================
// Test: Check expiration - Active at 6 cycles (just above threshold)
// =============================================================================

void test_check_active_at_6() {
    printf("Testing check_trade_deal_expiration returns Active at 6 cycles...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);
    agreement.cycles_remaining = 6;

    ExpirationCheckResult result = check_trade_deal_expiration(agreement);
    assert(result == ExpirationCheckResult::Active);

    printf("  PASS: 6 cycles returns Active\n");
}

// =============================================================================
// Test: Check expiration - Expired (0 cycles)
// =============================================================================

void test_check_expired_zero_cycles() {
    printf("Testing check_trade_deal_expiration returns Expired at 0 cycles...\n");

    TradeAgreementComponent agreement{};
    agreement.agreement_type = TradeAgreementType::Basic;
    agreement.cycles_remaining = 0;

    ExpirationCheckResult result = check_trade_deal_expiration(agreement);
    assert(result == ExpirationCheckResult::Expired);

    printf("  PASS: 0 cycles returns Expired\n");
}

// =============================================================================
// Test: Check expiration - Expired (None type)
// =============================================================================

void test_check_expired_none_type() {
    printf("Testing check_trade_deal_expiration returns Expired for None type...\n");

    TradeAgreementComponent agreement{};
    agreement.agreement_type = TradeAgreementType::None;
    agreement.cycles_remaining = 100;

    ExpirationCheckResult result = check_trade_deal_expiration(agreement);
    assert(result == ExpirationCheckResult::Expired);

    printf("  PASS: None type returns Expired\n");
}

// =============================================================================
// Test: tick_trade_deal_with_expiration - normal active tick
// =============================================================================

void test_tick_with_expiration_active() {
    printf("Testing tick_trade_deal_with_expiration during active phase...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);
    assert(agreement.cycles_remaining == 500);

    TradeDealExpirationWarningEvent warning{};
    TradeAgreementExpiredEvent expired{};

    ExpirationCheckResult result = tick_trade_deal_with_expiration(
        agreement, 42, warning, expired);

    assert(result == ExpirationCheckResult::Active);
    assert(agreement.cycles_remaining == 499);
    // Warning event should not be populated (agreement fields stay default)
    assert(warning.agreement == 0);
    // Expired event should not be populated
    assert(expired.agreement == 0);

    printf("  PASS: Active tick returns Active, no events\n");
}

// =============================================================================
// Test: tick_trade_deal_with_expiration - enters warning at 5 cycles
// =============================================================================

void test_tick_with_expiration_warning() {
    printf("Testing tick_trade_deal_with_expiration emits warning at 5 cycles...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Enhanced, 100000);
    agreement.cycles_remaining = 6; // After tick, will be 5

    TradeDealExpirationWarningEvent warning{};
    TradeAgreementExpiredEvent expired{};

    ExpirationCheckResult result = tick_trade_deal_with_expiration(
        agreement, 99, warning, expired);

    assert(result == ExpirationCheckResult::Warning);
    assert(agreement.cycles_remaining == 5);
    assert(warning.agreement == 99);
    assert(warning.party_a == GAME_MASTER);
    assert(warning.party_b == 1);
    assert(warning.cycles_remaining == 5);
    assert(warning.type == TradeAgreementType::Enhanced);

    printf("  PASS: Warning emitted at 5 cycles remaining\n");
}

// =============================================================================
// Test: tick_trade_deal_with_expiration - warning at each cycle <= 5
// =============================================================================

void test_tick_with_expiration_warning_each_cycle() {
    printf("Testing tick_trade_deal_with_expiration emits warning for cycles 5..1...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);
    agreement.cycles_remaining = 6;

    for (uint16_t expected = 5; expected >= 1; --expected) {
        TradeDealExpirationWarningEvent warning{};
        TradeAgreementExpiredEvent expired{};

        ExpirationCheckResult result = tick_trade_deal_with_expiration(
            agreement, 42, warning, expired);

        assert(result == ExpirationCheckResult::Warning);
        assert(agreement.cycles_remaining == expected);
        assert(warning.cycles_remaining == expected);
    }

    printf("  PASS: Warning emitted for each cycle 5..1\n");
}

// =============================================================================
// Test: tick_trade_deal_with_expiration - expires at 0
// =============================================================================

void test_tick_with_expiration_expired() {
    printf("Testing tick_trade_deal_with_expiration emits expired at 0...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 2, 3, TradeAgreementType::Premium, 100000);
    agreement.cycles_remaining = 1;

    TradeDealExpirationWarningEvent warning{};
    TradeAgreementExpiredEvent expired{};

    ExpirationCheckResult result = tick_trade_deal_with_expiration(
        agreement, 77, warning, expired);

    assert(result == ExpirationCheckResult::Expired);
    assert(agreement.cycles_remaining == 0);
    assert(agreement.agreement_type == TradeAgreementType::None);
    assert(expired.agreement == 77);
    // After expiration, party_a/party_b are the original values before zeroing
    // tick_trade_deal zeros the bonuses but leaves party fields

    printf("  PASS: Expired event emitted at 0 cycles\n");
}

// =============================================================================
// Test: tick_trade_deal_with_expiration - already expired stays expired
// =============================================================================

void test_tick_with_expiration_already_expired() {
    printf("Testing tick_trade_deal_with_expiration on already expired deal...\n");

    TradeAgreementComponent agreement{};
    agreement.agreement_type = TradeAgreementType::None;
    agreement.cycles_remaining = 0;

    TradeDealExpirationWarningEvent warning{};
    TradeAgreementExpiredEvent expired{};

    ExpirationCheckResult result = tick_trade_deal_with_expiration(
        agreement, 10, warning, expired);

    assert(result == ExpirationCheckResult::Expired);
    assert(expired.agreement == 10);

    printf("  PASS: Already expired deal stays expired\n");
}

// =============================================================================
// Test: renew_trade_deal - success
// =============================================================================

void test_renew_success() {
    printf("Testing renew_trade_deal succeeds...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);

    // Tick down to 10 remaining
    agreement.cycles_remaining = 10;

    bool renewed = renew_trade_deal(agreement, 50000);
    assert(renewed == true);
    assert(agreement.cycles_remaining == 500);  // Reset to Basic default
    assert(agreement.agreement_type == TradeAgreementType::Basic);

    printf("  PASS: Deal renewed with reset duration\n");
}

// =============================================================================
// Test: renew_trade_deal - success for Enhanced tier
// =============================================================================

void test_renew_enhanced() {
    printf("Testing renew_trade_deal succeeds for Enhanced tier...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Enhanced, 100000);

    agreement.cycles_remaining = 3;

    bool renewed = renew_trade_deal(agreement, 100000);
    assert(renewed == true);
    assert(agreement.cycles_remaining == 1000);  // Enhanced default

    printf("  PASS: Enhanced deal renewed to 1000 cycles\n");
}

// =============================================================================
// Test: renew_trade_deal - success for Premium tier
// =============================================================================

void test_renew_premium() {
    printf("Testing renew_trade_deal succeeds for Premium tier...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Premium, 100000);

    agreement.cycles_remaining = 1;

    bool renewed = renew_trade_deal(agreement, 100000);
    assert(renewed == true);
    assert(agreement.cycles_remaining == 1500);  // Premium default

    printf("  PASS: Premium deal renewed to 1500 cycles\n");
}

// =============================================================================
// Test: renew_trade_deal - fail on None type
// =============================================================================

void test_renew_fail_none() {
    printf("Testing renew_trade_deal fails on None type...\n");

    TradeAgreementComponent agreement{};
    agreement.agreement_type = TradeAgreementType::None;
    agreement.cycles_remaining = 10;

    bool renewed = renew_trade_deal(agreement, 100000);
    assert(renewed == false);

    printf("  PASS: Cannot renew None agreement\n");
}

// =============================================================================
// Test: renew_trade_deal - fail on zero cycles (expired)
// =============================================================================

void test_renew_fail_expired() {
    printf("Testing renew_trade_deal fails on expired deal (0 cycles)...\n");

    TradeAgreementComponent agreement{};
    agreement.agreement_type = TradeAgreementType::Basic;
    agreement.cycles_remaining = 0;

    bool renewed = renew_trade_deal(agreement, 100000);
    assert(renewed == false);

    printf("  PASS: Cannot renew expired deal\n");
}

// =============================================================================
// Test: renew_trade_deal - fail on insufficient treasury
// =============================================================================

void test_renew_fail_insufficient_treasury() {
    printf("Testing renew_trade_deal fails with insufficient treasury...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);
    agreement.cycles_remaining = 10;

    // Basic costs 1000 per cycle, player has only 999
    bool renewed = renew_trade_deal(agreement, 999);
    assert(renewed == false);
    assert(agreement.cycles_remaining == 10);  // Unchanged

    printf("  PASS: Insufficient treasury correctly rejected\n");
}

// =============================================================================
// Test: renew_trade_deal - exact treasury succeeds
// =============================================================================

void test_renew_exact_treasury() {
    printf("Testing renew_trade_deal succeeds with exact treasury...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);
    agreement.cycles_remaining = 10;

    bool renewed = renew_trade_deal(agreement, 1000);  // Exactly one cycle cost
    assert(renewed == true);
    assert(agreement.cycles_remaining == 500);

    printf("  PASS: Exact treasury accepted for renewal\n");
}

// =============================================================================
// Test: Full lifecycle - initiate, tick to warning, renew, tick again
// =============================================================================

void test_full_lifecycle() {
    printf("Testing full lifecycle: initiate -> warning -> renew -> active...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);

    // Fast-forward to just before warning threshold
    agreement.cycles_remaining = 7;

    // Tick to 6 - still active
    TradeDealExpirationWarningEvent warning{};
    TradeAgreementExpiredEvent expired{};
    ExpirationCheckResult result = tick_trade_deal_with_expiration(
        agreement, 1, warning, expired);
    assert(result == ExpirationCheckResult::Active);
    assert(agreement.cycles_remaining == 6);

    // Tick to 5 - warning
    result = tick_trade_deal_with_expiration(agreement, 1, warning, expired);
    assert(result == ExpirationCheckResult::Warning);
    assert(agreement.cycles_remaining == 5);

    // Renew
    bool renewed = renew_trade_deal(agreement, 50000);
    assert(renewed == true);
    assert(agreement.cycles_remaining == 500);

    // Verify it's back to active
    result = check_trade_deal_expiration(agreement);
    assert(result == ExpirationCheckResult::Active);

    printf("  PASS: Full lifecycle works correctly\n");
}

// =============================================================================
// Test: ExpirationWarningEvent default constructor
// =============================================================================

void test_warning_event_default() {
    printf("Testing TradeDealExpirationWarningEvent default constructor...\n");

    TradeDealExpirationWarningEvent event;
    assert(event.agreement == 0);
    assert(event.party_a == 0);
    assert(event.party_b == 0);
    assert(event.cycles_remaining == 0);
    assert(event.type == TradeAgreementType::None);

    printf("  PASS: Default warning event has expected values\n");
}

// =============================================================================
// Test: ExpirationWarningEvent parameterized constructor
// =============================================================================

void test_warning_event_param() {
    printf("Testing TradeDealExpirationWarningEvent parameterized constructor...\n");

    TradeDealExpirationWarningEvent event(42, 1, 2, 5, TradeAgreementType::Enhanced);
    assert(event.agreement == 42);
    assert(event.party_a == 1);
    assert(event.party_b == 2);
    assert(event.cycles_remaining == 5);
    assert(event.type == TradeAgreementType::Enhanced);

    printf("  PASS: Parameterized warning event has expected values\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Trade Deal Expiration Tests (Epic 8, Ticket E8-023) ===\n\n");

    // Constant test
    test_warning_constant();

    // Check expiration status tests
    test_check_active();
    test_check_warning_at_5();
    test_check_warning_at_1();
    test_check_active_at_6();
    test_check_expired_zero_cycles();
    test_check_expired_none_type();

    // Tick with expiration tests
    test_tick_with_expiration_active();
    test_tick_with_expiration_warning();
    test_tick_with_expiration_warning_each_cycle();
    test_tick_with_expiration_expired();
    test_tick_with_expiration_already_expired();

    // Renewal tests
    test_renew_success();
    test_renew_enhanced();
    test_renew_premium();
    test_renew_fail_none();
    test_renew_fail_expired();
    test_renew_fail_insufficient_treasury();
    test_renew_exact_treasury();

    // Lifecycle test
    test_full_lifecycle();

    // Event constructor tests
    test_warning_event_default();
    test_warning_event_param();

    printf("\n=== All Trade Deal Expiration Tests Passed ===\n");
    return 0;
}
