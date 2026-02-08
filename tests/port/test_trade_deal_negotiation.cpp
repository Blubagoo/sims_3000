/**
 * @file test_trade_deal_negotiation.cpp
 * @brief Unit tests for trade deal negotiation (Epic 8, Ticket E8-022)
 *
 * Tests cover:
 * - TradeDealConfig lookup for all tiers
 * - Deal initiation success with valid parameters
 * - Deal initiation failures (invalid player, neighbor, type, treasury)
 * - Agreement component population on success
 * - Deal downgrade through all tiers
 * - Tick-based expiration
 * - Deal reverts to lower tier if payment fails (downgrade)
 */

#include <sims3000/port/TradeDealNegotiation.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <sims3000/port/PortTypes.h>
#include <sims3000/ecs/Components.h>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000::port;
using namespace sims3000;

// =============================================================================
// Helper: float comparison with tolerance
// =============================================================================

static bool float_eq(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

// =============================================================================
// Test: Config lookup for None tier
// =============================================================================

void test_config_none() {
    printf("Testing TradeDealConfig for None tier...\n");

    TradeDealConfig config = get_trade_deal_config(TradeAgreementType::None);
    assert(config.cost_per_cycle == 0);
    assert(float_eq(config.income_multiplier, 0.5f));
    assert(config.demand_bonus == 0);
    assert(config.default_duration == 0);

    printf("  PASS: None tier config correct\n");
}

// =============================================================================
// Test: Config lookup for Basic tier
// =============================================================================

void test_config_basic() {
    printf("Testing TradeDealConfig for Basic tier...\n");

    TradeDealConfig config = get_trade_deal_config(TradeAgreementType::Basic);
    assert(config.cost_per_cycle == 1000);
    assert(float_eq(config.income_multiplier, 0.8f));
    assert(config.demand_bonus == 5);
    assert(config.default_duration == 500);

    printf("  PASS: Basic tier config correct\n");
}

// =============================================================================
// Test: Config lookup for Enhanced tier
// =============================================================================

void test_config_enhanced() {
    printf("Testing TradeDealConfig for Enhanced tier...\n");

    TradeDealConfig config = get_trade_deal_config(TradeAgreementType::Enhanced);
    assert(config.cost_per_cycle == 2500);
    assert(float_eq(config.income_multiplier, 1.0f));
    assert(config.demand_bonus == 10);
    assert(config.default_duration == 1000);

    printf("  PASS: Enhanced tier config correct\n");
}

// =============================================================================
// Test: Config lookup for Premium tier
// =============================================================================

void test_config_premium() {
    printf("Testing TradeDealConfig for Premium tier...\n");

    TradeDealConfig config = get_trade_deal_config(TradeAgreementType::Premium);
    assert(config.cost_per_cycle == 5000);
    assert(float_eq(config.income_multiplier, 1.2f));
    assert(config.demand_bonus == 15);
    assert(config.default_duration == 1500);

    printf("  PASS: Premium tier config correct\n");
}

// =============================================================================
// Test: Successful deal initiation (Basic)
// =============================================================================

void test_initiate_basic_deal() {
    printf("Testing successful Basic deal initiation...\n");

    TradeAgreementComponent agreement{};
    bool result = initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);

    assert(result == true);
    assert(agreement.party_a == GAME_MASTER);
    assert(agreement.party_b == 1);
    assert(agreement.agreement_type == TradeAgreementType::Basic);
    assert(agreement.neighbor_id == 2);
    assert(agreement.cycles_remaining == 500);
    assert(agreement.demand_bonus_a == 0);
    assert(agreement.demand_bonus_b == 5);
    assert(agreement.income_bonus_percent == 80);  // 0.8 * 100
    assert(agreement.cost_per_cycle_a == 0);
    assert(agreement.cost_per_cycle_b == 1000);

    printf("  PASS: Basic deal initiated correctly\n");
}

// =============================================================================
// Test: Successful deal initiation (Premium)
// =============================================================================

void test_initiate_premium_deal() {
    printf("Testing successful Premium deal initiation...\n");

    TradeAgreementComponent agreement{};
    bool result = initiate_trade_deal(agreement, 3, 4, TradeAgreementType::Premium, 100000);

    assert(result == true);
    assert(agreement.party_a == GAME_MASTER);
    assert(agreement.party_b == 3);
    assert(agreement.agreement_type == TradeAgreementType::Premium);
    assert(agreement.neighbor_id == 4);
    assert(agreement.cycles_remaining == 1500);
    assert(agreement.demand_bonus_b == 15);
    assert(agreement.income_bonus_percent == 120);  // 1.2 * 100
    assert(agreement.cost_per_cycle_b == 5000);

    printf("  PASS: Premium deal initiated correctly\n");
}

// =============================================================================
// Test: Fail - proposed type is None
// =============================================================================

void test_initiate_fail_none_type() {
    printf("Testing deal initiation fails with None type...\n");

    TradeAgreementComponent agreement{};
    bool result = initiate_trade_deal(agreement, 1, 2, TradeAgreementType::None, 50000);

    assert(result == false);

    printf("  PASS: None type correctly rejected\n");
}

// =============================================================================
// Test: Fail - player_id is GAME_MASTER
// =============================================================================

void test_initiate_fail_game_master_player() {
    printf("Testing deal initiation fails with GAME_MASTER player...\n");

    TradeAgreementComponent agreement{};
    bool result = initiate_trade_deal(agreement, GAME_MASTER, 2, TradeAgreementType::Basic, 50000);

    assert(result == false);

    printf("  PASS: GAME_MASTER player correctly rejected\n");
}

// =============================================================================
// Test: Fail - neighbor_id out of range (0)
// =============================================================================

void test_initiate_fail_neighbor_zero() {
    printf("Testing deal initiation fails with neighbor_id = 0...\n");

    TradeAgreementComponent agreement{};
    bool result = initiate_trade_deal(agreement, 1, 0, TradeAgreementType::Basic, 50000);

    assert(result == false);

    printf("  PASS: neighbor_id 0 correctly rejected\n");
}

// =============================================================================
// Test: Fail - neighbor_id out of range (5)
// =============================================================================

void test_initiate_fail_neighbor_too_high() {
    printf("Testing deal initiation fails with neighbor_id = 5...\n");

    TradeAgreementComponent agreement{};
    bool result = initiate_trade_deal(agreement, 1, 5, TradeAgreementType::Basic, 50000);

    assert(result == false);

    printf("  PASS: neighbor_id 5 correctly rejected\n");
}

// =============================================================================
// Test: Fail - insufficient treasury
// =============================================================================

void test_initiate_fail_insufficient_treasury() {
    printf("Testing deal initiation fails with insufficient treasury...\n");

    TradeAgreementComponent agreement{};

    // Basic costs 1000, player has 999
    bool result = initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 999);
    assert(result == false);

    // Premium costs 5000, player has 4999
    result = initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Premium, 4999);
    assert(result == false);

    printf("  PASS: Insufficient treasury correctly rejected\n");
}

// =============================================================================
// Test: Treasury exactly at cost succeeds
// =============================================================================

void test_initiate_exact_treasury() {
    printf("Testing deal initiation succeeds with exact treasury...\n");

    TradeAgreementComponent agreement{};
    bool result = initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 1000);

    assert(result == true);
    assert(agreement.agreement_type == TradeAgreementType::Basic);

    printf("  PASS: Exact treasury amount accepted\n");
}

// =============================================================================
// Test: All valid neighbor IDs (1-4)
// =============================================================================

void test_initiate_all_valid_neighbors() {
    printf("Testing deal initiation with all valid neighbor IDs (1-4)...\n");

    for (uint8_t nid = 1; nid <= 4; ++nid) {
        TradeAgreementComponent agreement{};
        bool result = initiate_trade_deal(agreement, 1, nid, TradeAgreementType::Basic, 50000);
        assert(result == true);
        assert(agreement.neighbor_id == nid);
    }

    printf("  PASS: All neighbor IDs 1-4 accepted\n");
}

// =============================================================================
// Test: Downgrade Premium -> Enhanced
// =============================================================================

void test_downgrade_premium_to_enhanced() {
    printf("Testing downgrade Premium -> Enhanced...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Premium, 100000);

    TradeAgreementType new_type = downgrade_trade_deal(agreement);

    assert(new_type == TradeAgreementType::Enhanced);
    assert(agreement.agreement_type == TradeAgreementType::Enhanced);
    assert(agreement.demand_bonus_b == 10);
    assert(agreement.income_bonus_percent == 100);  // 1.0 * 100
    assert(agreement.cost_per_cycle_b == 2500);
    assert(agreement.cycles_remaining == 1000);

    printf("  PASS: Premium correctly downgraded to Enhanced\n");
}

// =============================================================================
// Test: Downgrade Enhanced -> Basic
// =============================================================================

void test_downgrade_enhanced_to_basic() {
    printf("Testing downgrade Enhanced -> Basic...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Enhanced, 100000);

    TradeAgreementType new_type = downgrade_trade_deal(agreement);

    assert(new_type == TradeAgreementType::Basic);
    assert(agreement.agreement_type == TradeAgreementType::Basic);
    assert(agreement.demand_bonus_b == 5);
    assert(agreement.income_bonus_percent == 80);
    assert(agreement.cost_per_cycle_b == 1000);
    assert(agreement.cycles_remaining == 500);

    printf("  PASS: Enhanced correctly downgraded to Basic\n");
}

// =============================================================================
// Test: Downgrade Basic -> None
// =============================================================================

void test_downgrade_basic_to_none() {
    printf("Testing downgrade Basic -> None...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 100000);

    TradeAgreementType new_type = downgrade_trade_deal(agreement);

    assert(new_type == TradeAgreementType::None);
    assert(agreement.agreement_type == TradeAgreementType::None);
    assert(agreement.demand_bonus_b == 0);
    assert(agreement.income_bonus_percent == 50);  // 0.5 * 100
    assert(agreement.cost_per_cycle_b == 0);
    assert(agreement.cycles_remaining == 0);

    printf("  PASS: Basic correctly downgraded to None\n");
}

// =============================================================================
// Test: Downgrade None stays None
// =============================================================================

void test_downgrade_none_stays_none() {
    printf("Testing downgrade None stays None...\n");

    TradeAgreementComponent agreement{};
    agreement.agreement_type = TradeAgreementType::None;

    TradeAgreementType new_type = downgrade_trade_deal(agreement);

    assert(new_type == TradeAgreementType::None);
    assert(agreement.agreement_type == TradeAgreementType::None);

    printf("  PASS: None stays None on downgrade\n");
}

// =============================================================================
// Test: Full downgrade chain Premium -> Enhanced -> Basic -> None
// =============================================================================

void test_full_downgrade_chain() {
    printf("Testing full downgrade chain Premium -> Enhanced -> Basic -> None...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Premium, 100000);

    assert(downgrade_trade_deal(agreement) == TradeAgreementType::Enhanced);
    assert(downgrade_trade_deal(agreement) == TradeAgreementType::Basic);
    assert(downgrade_trade_deal(agreement) == TradeAgreementType::None);
    assert(downgrade_trade_deal(agreement) == TradeAgreementType::None);

    printf("  PASS: Full downgrade chain works correctly\n");
}

// =============================================================================
// Test: Tick decrements cycles_remaining
// =============================================================================

void test_tick_decrements() {
    printf("Testing tick decrements cycles_remaining...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);

    assert(agreement.cycles_remaining == 500);

    bool active = tick_trade_deal(agreement);
    assert(active == true);
    assert(agreement.cycles_remaining == 499);

    active = tick_trade_deal(agreement);
    assert(active == true);
    assert(agreement.cycles_remaining == 498);

    printf("  PASS: Tick correctly decrements cycles\n");
}

// =============================================================================
// Test: Deal expires when cycles reach 0
// =============================================================================

void test_tick_expiration() {
    printf("Testing deal expires when cycles reach 0...\n");

    TradeAgreementComponent agreement{};
    initiate_trade_deal(agreement, 1, 2, TradeAgreementType::Basic, 50000);

    // Set to 1 cycle remaining so next tick expires it
    agreement.cycles_remaining = 1;

    bool active = tick_trade_deal(agreement);
    assert(active == false);
    assert(agreement.cycles_remaining == 0);
    assert(agreement.agreement_type == TradeAgreementType::None);
    assert(agreement.demand_bonus_b == 0);
    assert(agreement.cost_per_cycle_b == 0);

    printf("  PASS: Deal expires correctly at 0 cycles\n");
}

// =============================================================================
// Test: Tick on already-expired deal returns false
// =============================================================================

void test_tick_already_expired() {
    printf("Testing tick on already-expired deal returns false...\n");

    TradeAgreementComponent agreement{};
    agreement.agreement_type = TradeAgreementType::None;
    agreement.cycles_remaining = 0;

    bool active = tick_trade_deal(agreement);
    assert(active == false);

    printf("  PASS: Tick on expired deal returns false\n");
}

// =============================================================================
// Test: Full lifecycle - initiate, tick through, expire
// =============================================================================

void test_full_lifecycle() {
    printf("Testing full deal lifecycle...\n");

    TradeAgreementComponent agreement{};
    bool result = initiate_trade_deal(agreement, 2, 3, TradeAgreementType::Enhanced, 100000);
    assert(result == true);

    uint16_t initial_cycles = agreement.cycles_remaining;
    assert(initial_cycles == 1000);

    // Tick through most of the duration
    for (uint16_t i = 0; i < 999; ++i) {
        bool active = tick_trade_deal(agreement);
        assert(active == true);
        assert(agreement.agreement_type == TradeAgreementType::Enhanced);
    }

    assert(agreement.cycles_remaining == 1);

    // Final tick - expires
    bool active = tick_trade_deal(agreement);
    assert(active == false);
    assert(agreement.agreement_type == TradeAgreementType::None);
    assert(agreement.cycles_remaining == 0);

    printf("  PASS: Full lifecycle completes correctly (%u cycles)\n", initial_cycles);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Trade Deal Negotiation Tests (Epic 8, Ticket E8-022) ===\n\n");

    // Config lookup tests
    test_config_none();
    test_config_basic();
    test_config_enhanced();
    test_config_premium();

    // Deal initiation tests
    test_initiate_basic_deal();
    test_initiate_premium_deal();
    test_initiate_fail_none_type();
    test_initiate_fail_game_master_player();
    test_initiate_fail_neighbor_zero();
    test_initiate_fail_neighbor_too_high();
    test_initiate_fail_insufficient_treasury();
    test_initiate_exact_treasury();
    test_initiate_all_valid_neighbors();

    // Downgrade tests
    test_downgrade_premium_to_enhanced();
    test_downgrade_enhanced_to_basic();
    test_downgrade_basic_to_none();
    test_downgrade_none_stays_none();
    test_full_downgrade_chain();

    // Tick/expiration tests
    test_tick_decrements();
    test_tick_expiration();
    test_tick_already_expired();
    test_full_lifecycle();

    printf("\n=== All Trade Deal Negotiation Tests Passed ===\n");
    return 0;
}
