/**
 * @file test_port_upgrades.cpp
 * @brief Unit tests for port infrastructure upgrades (Epic 8, Ticket E8-032)
 *
 * Tests cover:
 * - Upgrade config values (cost, multiplier, rail requirement)
 * - Upgrade validation (treasury, rail, level ordering)
 * - Trade multiplier retrieval
 * - Upgrade cost calculation
 * - Upgrade level names
 * - Edge cases: invalid levels, downgrade attempts, insufficient funds
 */

#include <sims3000/port/PortUpgrades.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <string>

using namespace sims3000::port;

// =============================================================================
// Helper: float comparison
// =============================================================================

static bool approx_eq(float a, float b, float eps = 0.01f) {
    return std::fabs(a - b) < eps;
}

// =============================================================================
// Upgrade Config Tests
// =============================================================================

void test_basic_config() {
    printf("Testing Basic upgrade config...\n");
    auto config = get_upgrade_config(PortUpgradeLevel::Basic);
    assert(config.cost == 0);
    assert(approx_eq(config.trade_multiplier, 1.0f));
    assert(config.requires_rail == false);
    printf("  PASS: Basic = {cost=0, mult=1.0, rail=false}\n");
}

void test_upgraded_terminals_config() {
    printf("Testing UpgradedTerminals config...\n");
    auto config = get_upgrade_config(PortUpgradeLevel::UpgradedTerminals);
    assert(config.cost == 50000);
    assert(approx_eq(config.trade_multiplier, 1.2f));
    assert(config.requires_rail == false);
    printf("  PASS: UpgradedTerminals = {cost=50000, mult=1.2, rail=false}\n");
}

void test_advanced_logistics_config() {
    printf("Testing AdvancedLogistics config...\n");
    auto config = get_upgrade_config(PortUpgradeLevel::AdvancedLogistics);
    assert(config.cost == 100000);
    assert(approx_eq(config.trade_multiplier, 1.4f));
    assert(config.requires_rail == true);
    printf("  PASS: AdvancedLogistics = {cost=100000, mult=1.4, rail=true}\n");
}

void test_premium_hub_config() {
    printf("Testing PremiumHub config...\n");
    auto config = get_upgrade_config(PortUpgradeLevel::PremiumHub);
    assert(config.cost == 200000);
    assert(approx_eq(config.trade_multiplier, 1.6f));
    assert(config.requires_rail == true);
    printf("  PASS: PremiumHub = {cost=200000, mult=1.6, rail=true}\n");
}

// =============================================================================
// Upgrade Validation Tests
// =============================================================================

void test_can_upgrade_basic_to_terminals() {
    printf("Testing upgrade from Basic to UpgradedTerminals...\n");
    bool result = can_upgrade_port(PortUpgradeLevel::Basic,
                                    PortUpgradeLevel::UpgradedTerminals,
                                    100000, false);
    assert(result == true);
    printf("  PASS: Basic -> UpgradedTerminals with 100k credits, no rail = allowed\n");
}

void test_can_upgrade_terminals_to_advanced() {
    printf("Testing upgrade from UpgradedTerminals to AdvancedLogistics...\n");
    bool result = can_upgrade_port(PortUpgradeLevel::UpgradedTerminals,
                                    PortUpgradeLevel::AdvancedLogistics,
                                    200000, true);
    assert(result == true);
    printf("  PASS: Terminals -> Advanced with 200k credits, rail = allowed\n");
}

void test_can_upgrade_advanced_to_premium() {
    printf("Testing upgrade from AdvancedLogistics to PremiumHub...\n");
    bool result = can_upgrade_port(PortUpgradeLevel::AdvancedLogistics,
                                    PortUpgradeLevel::PremiumHub,
                                    250000, true);
    assert(result == true);
    printf("  PASS: Advanced -> Premium with 250k credits, rail = allowed\n");
}

void test_skip_level_upgrade_allowed() {
    printf("Testing skip-level upgrade (Basic -> AdvancedLogistics)...\n");
    bool result = can_upgrade_port(PortUpgradeLevel::Basic,
                                    PortUpgradeLevel::AdvancedLogistics,
                                    100000, true);
    assert(result == true);
    printf("  PASS: Basic -> Advanced with sufficient credits+rail = allowed\n");
}

void test_cannot_downgrade() {
    printf("Testing downgrade is rejected...\n");
    bool result = can_upgrade_port(PortUpgradeLevel::PremiumHub,
                                    PortUpgradeLevel::Basic,
                                    999999, true);
    assert(result == false);
    printf("  PASS: Premium -> Basic = rejected (downgrade)\n");
}

void test_cannot_upgrade_same_level() {
    printf("Testing same-level upgrade is rejected...\n");
    bool result = can_upgrade_port(PortUpgradeLevel::UpgradedTerminals,
                                    PortUpgradeLevel::UpgradedTerminals,
                                    999999, true);
    assert(result == false);
    printf("  PASS: Terminals -> Terminals = rejected (same level)\n");
}

void test_insufficient_treasury() {
    printf("Testing insufficient treasury...\n");
    // Need 50000 for UpgradedTerminals
    bool result = can_upgrade_port(PortUpgradeLevel::Basic,
                                    PortUpgradeLevel::UpgradedTerminals,
                                    49999, false);
    assert(result == false);
    printf("  PASS: Basic -> Terminals with 49999 credits = rejected\n");
}

void test_exact_treasury() {
    printf("Testing exact treasury amount...\n");
    bool result = can_upgrade_port(PortUpgradeLevel::Basic,
                                    PortUpgradeLevel::UpgradedTerminals,
                                    50000, false);
    assert(result == true);
    printf("  PASS: Basic -> Terminals with exactly 50000 credits = allowed\n");
}

void test_missing_rail_for_advanced() {
    printf("Testing missing rail for AdvancedLogistics...\n");
    bool result = can_upgrade_port(PortUpgradeLevel::Basic,
                                    PortUpgradeLevel::AdvancedLogistics,
                                    200000, false);  // No rail!
    assert(result == false);
    printf("  PASS: Basic -> Advanced without rail = rejected\n");
}

void test_missing_rail_for_premium() {
    printf("Testing missing rail for PremiumHub...\n");
    bool result = can_upgrade_port(PortUpgradeLevel::AdvancedLogistics,
                                    PortUpgradeLevel::PremiumHub,
                                    300000, false);  // No rail!
    assert(result == false);
    printf("  PASS: Advanced -> Premium without rail = rejected\n");
}

// =============================================================================
// Trade Multiplier Tests
// =============================================================================

void test_trade_multiplier_basic() {
    printf("Testing trade multiplier for Basic...\n");
    assert(approx_eq(get_trade_multiplier(PortUpgradeLevel::Basic), 1.0f));
    printf("  PASS: Basic = 1.0x\n");
}

void test_trade_multiplier_terminals() {
    printf("Testing trade multiplier for UpgradedTerminals...\n");
    assert(approx_eq(get_trade_multiplier(PortUpgradeLevel::UpgradedTerminals), 1.2f));
    printf("  PASS: UpgradedTerminals = 1.2x\n");
}

void test_trade_multiplier_advanced() {
    printf("Testing trade multiplier for AdvancedLogistics...\n");
    assert(approx_eq(get_trade_multiplier(PortUpgradeLevel::AdvancedLogistics), 1.4f));
    printf("  PASS: AdvancedLogistics = 1.4x\n");
}

void test_trade_multiplier_premium() {
    printf("Testing trade multiplier for PremiumHub...\n");
    assert(approx_eq(get_trade_multiplier(PortUpgradeLevel::PremiumHub), 1.6f));
    printf("  PASS: PremiumHub = 1.6x\n");
}

// =============================================================================
// Upgrade Cost Tests
// =============================================================================

void test_upgrade_cost_basic_to_terminals() {
    printf("Testing upgrade cost Basic -> Terminals...\n");
    int64_t cost = get_upgrade_cost(PortUpgradeLevel::Basic,
                                      PortUpgradeLevel::UpgradedTerminals);
    assert(cost == 50000);
    printf("  PASS: cost = %lld\n", (long long)cost);
}

void test_upgrade_cost_terminals_to_advanced() {
    printf("Testing upgrade cost Terminals -> Advanced...\n");
    int64_t cost = get_upgrade_cost(PortUpgradeLevel::UpgradedTerminals,
                                      PortUpgradeLevel::AdvancedLogistics);
    assert(cost == 100000);
    printf("  PASS: cost = %lld\n", (long long)cost);
}

void test_upgrade_cost_downgrade_returns_zero() {
    printf("Testing upgrade cost for downgrade returns 0...\n");
    int64_t cost = get_upgrade_cost(PortUpgradeLevel::PremiumHub,
                                      PortUpgradeLevel::Basic);
    assert(cost == 0);
    printf("  PASS: downgrade cost = 0\n");
}

void test_upgrade_cost_same_level_returns_zero() {
    printf("Testing upgrade cost for same level returns 0...\n");
    int64_t cost = get_upgrade_cost(PortUpgradeLevel::AdvancedLogistics,
                                      PortUpgradeLevel::AdvancedLogistics);
    assert(cost == 0);
    printf("  PASS: same-level cost = 0\n");
}

// =============================================================================
// Level Name Tests
// =============================================================================

void test_upgrade_level_names() {
    printf("Testing upgrade level names...\n");
    assert(std::string(upgrade_level_name(PortUpgradeLevel::Basic)) == "Basic");
    assert(std::string(upgrade_level_name(PortUpgradeLevel::UpgradedTerminals)) == "Upgraded Terminals");
    assert(std::string(upgrade_level_name(PortUpgradeLevel::AdvancedLogistics)) == "Advanced Logistics");
    assert(std::string(upgrade_level_name(PortUpgradeLevel::PremiumHub)) == "Premium Hub");
    printf("  PASS: All level names correct\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Port Infrastructure Upgrade Tests (Epic 8, Ticket E8-032) ===\n\n");

    // Config values
    test_basic_config();
    test_upgraded_terminals_config();
    test_advanced_logistics_config();
    test_premium_hub_config();

    // Upgrade validation
    test_can_upgrade_basic_to_terminals();
    test_can_upgrade_terminals_to_advanced();
    test_can_upgrade_advanced_to_premium();
    test_skip_level_upgrade_allowed();
    test_cannot_downgrade();
    test_cannot_upgrade_same_level();
    test_insufficient_treasury();
    test_exact_treasury();
    test_missing_rail_for_advanced();
    test_missing_rail_for_premium();

    // Trade multipliers
    test_trade_multiplier_basic();
    test_trade_multiplier_terminals();
    test_trade_multiplier_advanced();
    test_trade_multiplier_premium();

    // Upgrade costs
    test_upgrade_cost_basic_to_terminals();
    test_upgrade_cost_terminals_to_advanced();
    test_upgrade_cost_downgrade_returns_zero();
    test_upgrade_cost_same_level_returns_zero();

    // Level names
    test_upgrade_level_names();

    printf("\n=== All Port Infrastructure Upgrade Tests Passed ===\n");
    return 0;
}
