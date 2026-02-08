/**
 * @file test_trade_agreement.cpp
 * @brief Unit tests for TradeAgreementComponent (Epic 8, Ticket E8-005)
 *
 * Tests cover:
 * - Component size (16 bytes)
 * - Trivially copyable requirement
 * - Default initialization values
 * - Custom value assignment
 * - NPC neighbor support (party_a = GAME_MASTER)
 * - Inter-player trade support
 * - Duration tracking for deal expiration
 * - Demand bonus ranges
 * - Income bonus percentage
 * - Cost per cycle values
 * - Copy semantics
 */

#include <sims3000/port/TradeAgreementComponent.h>
#include <sims3000/ecs/Components.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::port;
using namespace sims3000;

void test_trade_agreement_size() {
    printf("Testing TradeAgreementComponent size...\n");

    assert(sizeof(TradeAgreementComponent) == 16);

    printf("  PASS: TradeAgreementComponent is 16 bytes\n");
}

void test_trade_agreement_trivially_copyable() {
    printf("Testing TradeAgreementComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<TradeAgreementComponent>::value);

    printf("  PASS: TradeAgreementComponent is trivially copyable\n");
}

void test_trade_agreement_default_initialization() {
    printf("Testing default initialization...\n");

    TradeAgreementComponent trade{};
    assert(trade.party_a == 0);
    assert(trade.party_b == 0);
    assert(trade.agreement_type == TradeAgreementType::None);
    assert(trade.neighbor_id == 0);
    assert(trade.cycles_remaining == 0);
    assert(trade.demand_bonus_a == 0);
    assert(trade.demand_bonus_b == 0);
    assert(trade.income_bonus_percent == 100);
    assert(trade.padding == 0);
    assert(trade.cost_per_cycle_a == 0);
    assert(trade.cost_per_cycle_b == 0);

    printf("  PASS: Default initialization works correctly\n");
}

void test_trade_agreement_custom_values() {
    printf("Testing custom value assignment...\n");

    TradeAgreementComponent trade{};
    trade.party_a = 1;
    trade.party_b = 2;
    trade.agreement_type = TradeAgreementType::Enhanced;
    trade.neighbor_id = 5;
    trade.cycles_remaining = 1000;
    trade.demand_bonus_a = 10;
    trade.demand_bonus_b = -5;
    trade.income_bonus_percent = 150;
    trade.cost_per_cycle_a = -500;
    trade.cost_per_cycle_b = -250;

    assert(trade.party_a == 1);
    assert(trade.party_b == 2);
    assert(trade.agreement_type == TradeAgreementType::Enhanced);
    assert(trade.neighbor_id == 5);
    assert(trade.cycles_remaining == 1000);
    assert(trade.demand_bonus_a == 10);
    assert(trade.demand_bonus_b == -5);
    assert(trade.income_bonus_percent == 150);
    assert(trade.cost_per_cycle_a == -500);
    assert(trade.cost_per_cycle_b == -250);

    printf("  PASS: Custom value assignment works correctly\n");
}

void test_trade_agreement_npc_neighbor() {
    printf("Testing NPC neighbor support (party_a = GAME_MASTER)...\n");

    TradeAgreementComponent trade{};
    trade.party_a = GAME_MASTER;  // 0 = NPC/game-controlled
    trade.party_b = 1;            // Player 1
    trade.agreement_type = TradeAgreementType::Basic;
    trade.neighbor_id = 3;
    trade.cycles_remaining = 500;

    assert(trade.party_a == GAME_MASTER);
    assert(trade.party_a == 0);
    assert(trade.party_b == 1);
    assert(trade.neighbor_id == 3);

    printf("  PASS: NPC neighbor support works correctly\n");
}

void test_trade_agreement_inter_player() {
    printf("Testing inter-player trade support...\n");

    TradeAgreementComponent trade{};
    trade.party_a = 1;  // Player 1
    trade.party_b = 3;  // Player 3
    trade.agreement_type = TradeAgreementType::Premium;
    trade.cycles_remaining = 2000;
    trade.demand_bonus_a = 15;
    trade.demand_bonus_b = 15;
    trade.income_bonus_percent = 120;

    assert(trade.party_a == 1);
    assert(trade.party_b == 3);
    assert(trade.agreement_type == TradeAgreementType::Premium);

    printf("  PASS: Inter-player trade support works correctly\n");
}

void test_trade_agreement_duration_tracking() {
    printf("Testing duration tracking for deal expiration...\n");

    TradeAgreementComponent trade{};
    trade.cycles_remaining = 1000;

    // Simulate passage of time
    assert(trade.cycles_remaining == 1000);
    trade.cycles_remaining = 999;
    assert(trade.cycles_remaining == 999);

    // Deal expires at 0
    trade.cycles_remaining = 0;
    assert(trade.cycles_remaining == 0);

    // Max duration
    trade.cycles_remaining = 65535;
    assert(trade.cycles_remaining == 65535);

    printf("  PASS: Duration tracking works correctly\n");
}

void test_trade_agreement_demand_bonus_ranges() {
    printf("Testing demand bonus ranges...\n");

    TradeAgreementComponent trade{};

    // Positive bonuses
    trade.demand_bonus_a = 127;  // max int8_t
    trade.demand_bonus_b = 127;
    assert(trade.demand_bonus_a == 127);
    assert(trade.demand_bonus_b == 127);

    // Negative bonuses (penalties)
    trade.demand_bonus_a = -128;  // min int8_t
    trade.demand_bonus_b = -128;
    assert(trade.demand_bonus_a == -128);
    assert(trade.demand_bonus_b == -128);

    // Zero (neutral)
    trade.demand_bonus_a = 0;
    trade.demand_bonus_b = 0;
    assert(trade.demand_bonus_a == 0);
    assert(trade.demand_bonus_b == 0);

    printf("  PASS: Demand bonus ranges work correctly\n");
}

void test_trade_agreement_income_bonus() {
    printf("Testing income bonus percentage...\n");

    TradeAgreementComponent trade{};

    // Default is 100 (1.0x multiplier)
    assert(trade.income_bonus_percent == 100);

    // 1.5x multiplier
    trade.income_bonus_percent = 150;
    assert(trade.income_bonus_percent == 150);

    // 0.5x multiplier (penalty)
    trade.income_bonus_percent = 50;
    assert(trade.income_bonus_percent == 50);

    // Max (2.55x)
    trade.income_bonus_percent = 255;
    assert(trade.income_bonus_percent == 255);

    // Zero (no income)
    trade.income_bonus_percent = 0;
    assert(trade.income_bonus_percent == 0);

    printf("  PASS: Income bonus percentage works correctly\n");
}

void test_trade_agreement_cost_per_cycle() {
    printf("Testing cost per cycle values...\n");

    TradeAgreementComponent trade{};

    // Positive costs (income)
    trade.cost_per_cycle_a = 1000;
    trade.cost_per_cycle_b = 500;
    assert(trade.cost_per_cycle_a == 1000);
    assert(trade.cost_per_cycle_b == 500);

    // Negative costs (expenses)
    trade.cost_per_cycle_a = -1000;
    trade.cost_per_cycle_b = -500;
    assert(trade.cost_per_cycle_a == -1000);
    assert(trade.cost_per_cycle_b == -500);

    // Large values for party A (int32_t range)
    trade.cost_per_cycle_a = 2000000;
    assert(trade.cost_per_cycle_a == 2000000);

    trade.cost_per_cycle_a = -2000000;
    assert(trade.cost_per_cycle_a == -2000000);

    // Party B range (int16_t)
    trade.cost_per_cycle_b = 32767;
    assert(trade.cost_per_cycle_b == 32767);

    trade.cost_per_cycle_b = -32768;
    assert(trade.cost_per_cycle_b == -32768);

    printf("  PASS: Cost per cycle values work correctly\n");
}

void test_trade_agreement_all_types() {
    printf("Testing all agreement types can be assigned...\n");

    TradeAgreementComponent trade{};

    trade.agreement_type = TradeAgreementType::None;
    assert(trade.agreement_type == TradeAgreementType::None);

    trade.agreement_type = TradeAgreementType::Basic;
    assert(trade.agreement_type == TradeAgreementType::Basic);

    trade.agreement_type = TradeAgreementType::Enhanced;
    assert(trade.agreement_type == TradeAgreementType::Enhanced);

    trade.agreement_type = TradeAgreementType::Premium;
    assert(trade.agreement_type == TradeAgreementType::Premium);

    printf("  PASS: All agreement types can be assigned\n");
}

void test_trade_agreement_copy() {
    printf("Testing copy semantics...\n");

    TradeAgreementComponent original{};
    original.party_a = GAME_MASTER;
    original.party_b = 2;
    original.agreement_type = TradeAgreementType::Premium;
    original.neighbor_id = 7;
    original.cycles_remaining = 3000;
    original.demand_bonus_a = 20;
    original.demand_bonus_b = -10;
    original.income_bonus_percent = 175;
    original.cost_per_cycle_a = -1500;
    original.cost_per_cycle_b = 800;

    TradeAgreementComponent copy = original;
    assert(copy.party_a == GAME_MASTER);
    assert(copy.party_b == 2);
    assert(copy.agreement_type == TradeAgreementType::Premium);
    assert(copy.neighbor_id == 7);
    assert(copy.cycles_remaining == 3000);
    assert(copy.demand_bonus_a == 20);
    assert(copy.demand_bonus_b == -10);
    assert(copy.income_bonus_percent == 175);
    assert(copy.cost_per_cycle_a == -1500);
    assert(copy.cost_per_cycle_b == 800);

    printf("  PASS: Copy semantics work correctly\n");
}

int main() {
    printf("=== TradeAgreementComponent Unit Tests (Epic 8, Ticket E8-005) ===\n\n");

    test_trade_agreement_size();
    test_trade_agreement_trivially_copyable();
    test_trade_agreement_default_initialization();
    test_trade_agreement_custom_values();
    test_trade_agreement_npc_neighbor();
    test_trade_agreement_inter_player();
    test_trade_agreement_duration_tracking();
    test_trade_agreement_demand_bonus_ranges();
    test_trade_agreement_income_bonus();
    test_trade_agreement_cost_per_cycle();
    test_trade_agreement_all_types();
    test_trade_agreement_copy();

    printf("\n=== All TradeAgreementComponent Tests Passed ===\n");
    return 0;
}
