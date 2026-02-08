/**
 * @file test_trade_agreement_benefits.cpp
 * @brief Unit tests for trade agreement benefits (Epic 8, Ticket E8-027)
 *
 * Tests cover:
 * - Benefits lookup for each tier (None, Basic, Enhanced, Premium)
 * - Demand bonus calculation from multiple agreements
 * - Income bonus calculation from multiple agreements
 * - Symmetric benefits (both players receive same bonuses)
 * - Only active agreements contribute to bonuses
 * - Player must be a party to the agreement to receive benefits
 */

#include <sims3000/port/TradeAgreementBenefits.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <sims3000/port/PortTypes.h>
#include <cassert>
#include <cstdio>
#include <vector>

using namespace sims3000::port;

// =============================================================================
// Helper: Create a simple inter-player trade agreement
// =============================================================================

static TradeAgreementComponent make_agreement(
    uint8_t a, uint8_t b, TradeAgreementType type, uint16_t cycles = 100) {
    TradeAgreementComponent agreement{};
    agreement.party_a = a;
    agreement.party_b = b;
    agreement.agreement_type = type;
    agreement.cycles_remaining = cycles;
    return agreement;
}

// =============================================================================
// Test: Benefits for None tier
// =============================================================================

void test_benefits_none() {
    printf("Testing get_agreement_benefits for None tier...\n");

    TradeAgreementBenefits benefits = get_agreement_benefits(TradeAgreementType::None);
    assert(benefits.demand_bonus == 0);
    assert(benefits.income_bonus_percent == 0);

    printf("  PASS: None tier: +0 demand, +0%% income\n");
}

// =============================================================================
// Test: Benefits for Basic tier (Basic Trade)
// =============================================================================

void test_benefits_basic() {
    printf("Testing get_agreement_benefits for Basic tier...\n");

    TradeAgreementBenefits benefits = get_agreement_benefits(TradeAgreementType::Basic);
    assert(benefits.demand_bonus == 3);
    assert(benefits.income_bonus_percent == 5);

    printf("  PASS: Basic tier: +3 demand, +5%% income\n");
}

// =============================================================================
// Test: Benefits for Enhanced tier (Advanced Trade)
// =============================================================================

void test_benefits_enhanced() {
    printf("Testing get_agreement_benefits for Enhanced tier...\n");

    TradeAgreementBenefits benefits = get_agreement_benefits(TradeAgreementType::Enhanced);
    assert(benefits.demand_bonus == 6);
    assert(benefits.income_bonus_percent == 10);

    printf("  PASS: Enhanced tier: +6 demand, +10%% income\n");
}

// =============================================================================
// Test: Benefits for Premium tier (Strategic Partnership)
// =============================================================================

void test_benefits_premium() {
    printf("Testing get_agreement_benefits for Premium tier...\n");

    TradeAgreementBenefits benefits = get_agreement_benefits(TradeAgreementType::Premium);
    assert(benefits.demand_bonus == 10);
    assert(benefits.income_bonus_percent == 15);

    printf("  PASS: Premium tier: +10 demand, +15%% income\n");
}

// =============================================================================
// Test: Demand bonus - single agreement, player is party_a
// =============================================================================

void test_demand_bonus_single_as_party_a() {
    printf("Testing demand bonus with single agreement (player is party_a)...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Basic));

    int16_t bonus = calculate_total_demand_bonus(agreements, 1);
    assert(bonus == 3);

    printf("  PASS: Player as party_a gets +3 demand bonus\n");
}

// =============================================================================
// Test: Demand bonus - single agreement, player is party_b
// =============================================================================

void test_demand_bonus_single_as_party_b() {
    printf("Testing demand bonus with single agreement (player is party_b)...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Basic));

    int16_t bonus = calculate_total_demand_bonus(agreements, 2);
    assert(bonus == 3);

    printf("  PASS: Player as party_b gets +3 demand bonus\n");
}

// =============================================================================
// Test: Demand bonus - symmetric (both players get same)
// =============================================================================

void test_demand_bonus_symmetric() {
    printf("Testing demand bonus is symmetric for both players...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Enhanced));

    int16_t bonus_a = calculate_total_demand_bonus(agreements, 1);
    int16_t bonus_b = calculate_total_demand_bonus(agreements, 2);
    assert(bonus_a == bonus_b);
    assert(bonus_a == 6);

    printf("  PASS: Both players receive +6 demand bonus\n");
}

// =============================================================================
// Test: Demand bonus - multiple agreements stack
// =============================================================================

void test_demand_bonus_multiple() {
    printf("Testing demand bonus stacks from multiple agreements...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Basic));     // +3
    agreements.push_back(make_agreement(1, 3, TradeAgreementType::Enhanced));  // +6

    int16_t bonus = calculate_total_demand_bonus(agreements, 1);
    assert(bonus == 9); // 3 + 6

    printf("  PASS: Player 1 gets +9 from two agreements\n");
}

// =============================================================================
// Test: Demand bonus - only counts agreements player is part of
// =============================================================================

void test_demand_bonus_only_own() {
    printf("Testing demand bonus only counts player's own agreements...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Basic));     // +3 for player 1
    agreements.push_back(make_agreement(3, 4, TradeAgreementType::Premium));   // +10 for players 3,4

    int16_t bonus_1 = calculate_total_demand_bonus(agreements, 1);
    assert(bonus_1 == 3);

    int16_t bonus_3 = calculate_total_demand_bonus(agreements, 3);
    assert(bonus_3 == 10);

    int16_t bonus_5 = calculate_total_demand_bonus(agreements, 5);
    assert(bonus_5 == 0); // Not in any agreement

    printf("  PASS: Players only get bonuses from their own agreements\n");
}

// =============================================================================
// Test: Demand bonus - inactive (None) agreements excluded
// =============================================================================

void test_demand_bonus_excludes_none() {
    printf("Testing demand bonus excludes None-type agreements...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::None));
    agreements.push_back(make_agreement(1, 3, TradeAgreementType::Basic));

    int16_t bonus = calculate_total_demand_bonus(agreements, 1);
    assert(bonus == 3); // Only the Basic agreement counts

    printf("  PASS: None-type agreements excluded from bonus\n");
}

// =============================================================================
// Test: Demand bonus - empty agreements vector
// =============================================================================

void test_demand_bonus_empty() {
    printf("Testing demand bonus with no agreements...\n");

    std::vector<TradeAgreementComponent> agreements;

    int16_t bonus = calculate_total_demand_bonus(agreements, 1);
    assert(bonus == 0);

    printf("  PASS: No agreements gives zero bonus\n");
}

// =============================================================================
// Test: Income bonus - no agreements
// =============================================================================

void test_income_no_agreements() {
    printf("Testing income with no agreements...\n");

    std::vector<TradeAgreementComponent> agreements;

    int64_t income = apply_trade_agreement_income_bonus(10000, agreements, 1);
    assert(income == 10000); // 10000 * 100 / 100

    printf("  PASS: No agreements: income unchanged at 10000\n");
}

// =============================================================================
// Test: Income bonus - single Basic agreement (+5%)
// =============================================================================

void test_income_basic() {
    printf("Testing income with single Basic agreement...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Basic));

    int64_t income = apply_trade_agreement_income_bonus(10000, agreements, 1);
    assert(income == 10500); // 10000 * 105 / 100

    printf("  PASS: Basic agreement: 10000 -> 10500 (+5%%)\n");
}

// =============================================================================
// Test: Income bonus - single Enhanced agreement (+10%)
// =============================================================================

void test_income_enhanced() {
    printf("Testing income with single Enhanced agreement...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Enhanced));

    int64_t income = apply_trade_agreement_income_bonus(10000, agreements, 1);
    assert(income == 11000); // 10000 * 110 / 100

    printf("  PASS: Enhanced agreement: 10000 -> 11000 (+10%%)\n");
}

// =============================================================================
// Test: Income bonus - single Premium agreement (+15%)
// =============================================================================

void test_income_premium() {
    printf("Testing income with single Premium agreement...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Premium));

    int64_t income = apply_trade_agreement_income_bonus(10000, agreements, 1);
    assert(income == 11500); // 10000 * 115 / 100

    printf("  PASS: Premium agreement: 10000 -> 11500 (+15%%)\n");
}

// =============================================================================
// Test: Income bonus - multiple agreements stack
// =============================================================================

void test_income_multiple() {
    printf("Testing income with multiple stacking agreements...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Basic));     // +5%
    agreements.push_back(make_agreement(1, 3, TradeAgreementType::Enhanced));  // +10%

    int64_t income = apply_trade_agreement_income_bonus(10000, agreements, 1);
    assert(income == 11500); // 10000 * (100 + 5 + 10) / 100 = 10000 * 115 / 100

    printf("  PASS: Stacked agreements: 10000 -> 11500 (+15%%)\n");
}

// =============================================================================
// Test: Income bonus - symmetric for both players
// =============================================================================

void test_income_symmetric() {
    printf("Testing income bonus is symmetric for both players...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Premium));

    int64_t income_a = apply_trade_agreement_income_bonus(10000, agreements, 1);
    int64_t income_b = apply_trade_agreement_income_bonus(10000, agreements, 2);
    assert(income_a == income_b);
    assert(income_a == 11500);

    printf("  PASS: Both players receive same income bonus\n");
}

// =============================================================================
// Test: Income bonus - only counts player's own agreements
// =============================================================================

void test_income_only_own() {
    printf("Testing income only counts player's own agreements...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Premium));   // +15% for 1,2
    agreements.push_back(make_agreement(3, 4, TradeAgreementType::Basic));     // +5% for 3,4

    int64_t income_1 = apply_trade_agreement_income_bonus(10000, agreements, 1);
    assert(income_1 == 11500); // Only Premium applies

    int64_t income_3 = apply_trade_agreement_income_bonus(10000, agreements, 3);
    assert(income_3 == 10500); // Only Basic applies

    int64_t income_5 = apply_trade_agreement_income_bonus(10000, agreements, 5);
    assert(income_5 == 10000); // No agreements

    printf("  PASS: Income bonus only applies to player's own agreements\n");
}

// =============================================================================
// Test: Income bonus - excludes None-type agreements
// =============================================================================

void test_income_excludes_none() {
    printf("Testing income excludes None-type agreements...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::None));
    agreements.push_back(make_agreement(1, 3, TradeAgreementType::Basic));

    int64_t income = apply_trade_agreement_income_bonus(10000, agreements, 1);
    assert(income == 10500); // Only Basic applies

    printf("  PASS: None-type agreements excluded from income bonus\n");
}

// =============================================================================
// Test: Income bonus - zero base income
// =============================================================================

void test_income_zero_base() {
    printf("Testing income with zero base...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Premium));

    int64_t income = apply_trade_agreement_income_bonus(0, agreements, 1);
    assert(income == 0); // 0 * anything = 0

    printf("  PASS: Zero base income remains zero\n");
}

// =============================================================================
// Test: Income bonus - large base income
// =============================================================================

void test_income_large_base() {
    printf("Testing income with large base value...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Premium));

    int64_t base = 1000000;
    int64_t income = apply_trade_agreement_income_bonus(base, agreements, 1);
    assert(income == 1150000); // 1000000 * 115 / 100

    printf("  PASS: Large base income: 1000000 -> 1150000\n");
}

// =============================================================================
// Test: All three tiers stacked
// =============================================================================

void test_all_tiers_stacked() {
    printf("Testing all three tiers stacked for one player...\n");

    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(1, 2, TradeAgreementType::Basic));     // +3 demand, +5%
    agreements.push_back(make_agreement(1, 3, TradeAgreementType::Enhanced));  // +6 demand, +10%
    agreements.push_back(make_agreement(1, 4, TradeAgreementType::Premium));   // +10 demand, +15%

    int16_t demand = calculate_total_demand_bonus(agreements, 1);
    assert(demand == 19); // 3 + 6 + 10

    int64_t income = apply_trade_agreement_income_bonus(10000, agreements, 1);
    assert(income == 13000); // 10000 * (100 + 5 + 10 + 15) / 100 = 10000 * 130 / 100

    printf("  PASS: All tiers stacked: +19 demand, 10000 -> 13000\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Trade Agreement Benefits Tests (Epic 8, Ticket E8-027) ===\n\n");

    // Benefits lookup tests
    test_benefits_none();
    test_benefits_basic();
    test_benefits_enhanced();
    test_benefits_premium();

    // Demand bonus tests
    test_demand_bonus_single_as_party_a();
    test_demand_bonus_single_as_party_b();
    test_demand_bonus_symmetric();
    test_demand_bonus_multiple();
    test_demand_bonus_only_own();
    test_demand_bonus_excludes_none();
    test_demand_bonus_empty();

    // Income bonus tests
    test_income_no_agreements();
    test_income_basic();
    test_income_enhanced();
    test_income_premium();
    test_income_multiple();
    test_income_symmetric();
    test_income_only_own();
    test_income_excludes_none();
    test_income_zero_base();
    test_income_large_base();

    // Combined test
    test_all_tiers_stacked();

    printf("\n=== All Trade Agreement Benefits Tests Passed ===\n");
    return 0;
}
