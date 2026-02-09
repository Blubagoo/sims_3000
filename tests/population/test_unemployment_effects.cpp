/**
 * @file test_unemployment_effects.cpp
 * @brief Tests for unemployment effects on harmony (Ticket E10-023)
 *
 * Validates:
 * - Full employment bonus (unemployment <= 2%)
 * - Linear penalty for unemployment > 2%
 * - Maximum penalty cap at -30
 * - Harmony clamped to [0, 100]
 * - PopulationData modification
 */

#include <cassert>
#include <cstdio>
#include <cmath>

#include "sims3000/population/UnemploymentEffects.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Test: Full employment bonus (unemployment <= 2%)
// --------------------------------------------------------------------------
static void test_full_employment_bonus() {
    // Test 0% unemployment
    auto result1 = calculate_unemployment_effect(0.0f);
    assert(result1.is_full_employment && "0% should be full employment");
    assert(std::abs(result1.harmony_modifier - FULL_EMPLOYMENT_BONUS) < 0.01f &&
           "Should apply full employment bonus");

    // Test 2% unemployment (threshold)
    auto result2 = calculate_unemployment_effect(2.0f);
    assert(result2.is_full_employment && "2% should be full employment");
    assert(std::abs(result2.harmony_modifier - FULL_EMPLOYMENT_BONUS) < 0.01f &&
           "Should apply full employment bonus at 2%");

    // Test 1% unemployment
    auto result3 = calculate_unemployment_effect(1.0f);
    assert(result3.is_full_employment && "1% should be full employment");
    assert(std::abs(result3.harmony_modifier - FULL_EMPLOYMENT_BONUS) < 0.01f &&
           "Should apply full employment bonus at 1%");

    std::printf("  PASS: Full employment bonus\n");
}

// --------------------------------------------------------------------------
// Test: Linear penalty for unemployment > 2%
// --------------------------------------------------------------------------
static void test_linear_penalty() {
    // Test 10% unemployment
    auto result1 = calculate_unemployment_effect(10.0f);
    assert(!result1.is_full_employment && "10% should not be full employment");
    float expected1 = -(10.0f * UNEMPLOYMENT_HARMONY_PENALTY_RATE);
    assert(std::abs(result1.harmony_modifier - expected1) < 0.01f &&
           "Should apply linear penalty");

    // Test 20% unemployment
    auto result2 = calculate_unemployment_effect(20.0f);
    assert(!result2.is_full_employment && "20% should not be full employment");
    float expected2 = -(20.0f * UNEMPLOYMENT_HARMONY_PENALTY_RATE);
    assert(std::abs(result2.harmony_modifier - expected2) < 0.01f &&
           "Should apply linear penalty");

    // Test 5% unemployment
    auto result3 = calculate_unemployment_effect(5.0f);
    assert(!result3.is_full_employment && "5% should not be full employment");
    float expected3 = -(5.0f * UNEMPLOYMENT_HARMONY_PENALTY_RATE);
    assert(std::abs(result3.harmony_modifier - expected3) < 0.01f &&
           "Should apply linear penalty");

    std::printf("  PASS: Linear penalty\n");
}

// --------------------------------------------------------------------------
// Test: Maximum penalty cap at -30
// --------------------------------------------------------------------------
static void test_maximum_penalty_cap() {
    // Test very high unemployment (should cap at -30)
    auto result1 = calculate_unemployment_effect(100.0f);
    assert(!result1.is_full_employment && "100% should not be full employment");
    assert(std::abs(result1.harmony_modifier - (-MAX_UNEMPLOYMENT_PENALTY)) < 0.01f &&
           "Should cap at maximum penalty");

    // Test unemployment that exceeds cap threshold
    float cap_threshold = MAX_UNEMPLOYMENT_PENALTY / UNEMPLOYMENT_HARMONY_PENALTY_RATE;  // 60%
    auto result2 = calculate_unemployment_effect(cap_threshold + 10.0f);
    assert(std::abs(result2.harmony_modifier - (-MAX_UNEMPLOYMENT_PENALTY)) < 0.01f &&
           "Should cap at maximum penalty");

    // Test at exact cap threshold
    auto result3 = calculate_unemployment_effect(cap_threshold);
    assert(std::abs(result3.harmony_modifier - (-MAX_UNEMPLOYMENT_PENALTY)) < 0.01f &&
           "Should cap at maximum penalty at threshold");

    std::printf("  PASS: Maximum penalty cap\n");
}

// --------------------------------------------------------------------------
// Test: Harmony clamped to [0, 100]
// --------------------------------------------------------------------------
static void test_harmony_clamping() {
    // Test clamping to 100
    PopulationData pop1{};
    pop1.harmony_index = 98;
    apply_unemployment_effect(pop1, 0.0f);  // +5 bonus -> 103, should clamp to 100
    assert(pop1.harmony_index == 100 && "Harmony should clamp to 100");

    // Test clamping to 0
    PopulationData pop2{};
    pop2.harmony_index = 20;
    apply_unemployment_effect(pop2, 100.0f);  // -30 penalty -> -10, should clamp to 0
    assert(pop2.harmony_index == 0 && "Harmony should clamp to 0");

    // Test no clamping needed
    PopulationData pop3{};
    pop3.harmony_index = 50;
    apply_unemployment_effect(pop3, 10.0f);  // -5 penalty -> 45, no clamping
    assert(pop3.harmony_index == 45 && "Harmony should be 45");

    std::printf("  PASS: Harmony clamping\n");
}

// --------------------------------------------------------------------------
// Test: Apply effect modifies PopulationData
// --------------------------------------------------------------------------
static void test_apply_effect_modifies_population() {
    // Test positive modifier (full employment)
    PopulationData pop1{};
    pop1.harmony_index = 50;
    apply_unemployment_effect(pop1, 0.0f);
    assert(pop1.harmony_index == 55 && "Harmony should increase by 5");

    // Test negative modifier
    PopulationData pop2{};
    pop2.harmony_index = 50;
    apply_unemployment_effect(pop2, 10.0f);
    assert(pop2.harmony_index == 45 && "Harmony should decrease by 5");

    // Test larger negative modifier
    PopulationData pop3{};
    pop3.harmony_index = 60;
    apply_unemployment_effect(pop3, 40.0f);  // -20 penalty
    assert(pop3.harmony_index == 40 && "Harmony should decrease by 20");

    std::printf("  PASS: Apply effect modifies PopulationData\n");
}

// --------------------------------------------------------------------------
// Test: Edge case at 2.01% unemployment
// --------------------------------------------------------------------------
static void test_edge_case_just_above_threshold() {
    auto result = calculate_unemployment_effect(2.01f);
    assert(!result.is_full_employment && "2.01% should not be full employment");
    assert(result.harmony_modifier < 0.0f && "Should apply penalty, not bonus");

    std::printf("  PASS: Edge case just above threshold\n");
}

// --------------------------------------------------------------------------
// Test: Zero unemployment
// --------------------------------------------------------------------------
static void test_zero_unemployment() {
    PopulationData pop{};
    pop.harmony_index = 50;

    apply_unemployment_effect(pop, 0.0f);

    assert(pop.harmony_index == 55 && "Zero unemployment should give +5 bonus");

    std::printf("  PASS: Zero unemployment\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Unemployment Effects Tests (E10-023) ===\n");

    test_full_employment_bonus();
    test_linear_penalty();
    test_maximum_penalty_cap();
    test_harmony_clamping();
    test_apply_effect_modifies_population();
    test_edge_case_just_above_threshold();
    test_zero_unemployment();

    std::printf("All unemployment effects tests passed.\n");
    return 0;
}
