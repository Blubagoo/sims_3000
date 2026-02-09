/**
 * @file test_service_funding_integration.cpp
 * @brief Unit tests for ServiceFundingIntegration (E11-014)
 *
 * Tests: zero funding, full funding, over-funding, partial funding,
 *        base effectiveness scaling, all service types, batch calculation,
 *        custom base effectiveness, edge cases.
 */

#include "sims3000/economy/ServiceFundingIntegration.h"
#include "sims3000/economy/FundingLevel.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace sims3000::economy;

static bool approx(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

// ---------------------------------------------------------------------------
// Zero funding = inactive
// ---------------------------------------------------------------------------

void test_zero_funding_inactive() {
    printf("Testing zero funding -> services inactive...\n");

    auto r = calculate_funded_effectiveness(0, 1.0f, 0);

    assert(r.service_type == 0);
    assert(r.funding_level == 0);
    assert(approx(r.effectiveness_factor, 0.0f));
    assert(approx(r.base_effectiveness, 1.0f));
    assert(approx(r.final_effectiveness, 0.0f));

    printf("  PASS: 0%% funding = 0.0 effectiveness\n");
}

// ---------------------------------------------------------------------------
// Full funding = 100%
// ---------------------------------------------------------------------------

void test_full_funding() {
    printf("Testing 100%% funding -> full effectiveness...\n");

    auto r = calculate_funded_effectiveness(0, 1.0f, 100);

    assert(r.funding_level == 100);
    assert(approx(r.effectiveness_factor, 1.0f));
    assert(approx(r.final_effectiveness, 1.0f));

    printf("  PASS: 100%% funding = 1.0 effectiveness\n");
}

// ---------------------------------------------------------------------------
// Over-funding = ~110%
// ---------------------------------------------------------------------------

void test_over_funding() {
    printf("Testing 150%% funding -> ~1.10 effectiveness...\n");

    auto r = calculate_funded_effectiveness(0, 1.0f, 150);

    assert(r.funding_level == 150);
    assert(approx(r.effectiveness_factor, 1.10f));
    assert(approx(r.final_effectiveness, 1.10f));

    printf("  PASS: 150%% funding = ~1.10 effectiveness\n");
}

// ---------------------------------------------------------------------------
// Partial funding levels
// ---------------------------------------------------------------------------

void test_partial_funding_25() {
    printf("Testing 25%% funding...\n");

    auto r = calculate_funded_effectiveness(1, 1.0f, 25);

    assert(approx(r.effectiveness_factor, 0.40f));
    assert(approx(r.final_effectiveness, 0.40f));

    printf("  PASS: 25%% funding = 0.40 effectiveness\n");
}

void test_partial_funding_50() {
    printf("Testing 50%% funding...\n");

    auto r = calculate_funded_effectiveness(2, 1.0f, 50);

    assert(approx(r.effectiveness_factor, 0.65f));
    assert(approx(r.final_effectiveness, 0.65f));

    printf("  PASS: 50%% funding = 0.65 effectiveness\n");
}

void test_partial_funding_75() {
    printf("Testing 75%% funding...\n");

    auto r = calculate_funded_effectiveness(3, 1.0f, 75);

    assert(approx(r.effectiveness_factor, 0.85f));
    assert(approx(r.final_effectiveness, 0.85f));

    printf("  PASS: 75%% funding = 0.85 effectiveness\n");
}

// ---------------------------------------------------------------------------
// Base effectiveness scaling
// ---------------------------------------------------------------------------

void test_base_effectiveness_scaling() {
    printf("Testing base effectiveness scaling...\n");

    auto r = calculate_funded_effectiveness(0, 2.0f, 100);

    assert(approx(r.base_effectiveness, 2.0f));
    assert(approx(r.effectiveness_factor, 1.0f));
    assert(approx(r.final_effectiveness, 2.0f));

    printf("  PASS: base 2.0 * factor 1.0 = 2.0 final\n");
}

void test_base_effectiveness_with_partial_funding() {
    printf("Testing base effectiveness with partial funding...\n");

    auto r = calculate_funded_effectiveness(0, 0.5f, 50);

    assert(approx(r.base_effectiveness, 0.5f));
    assert(approx(r.effectiveness_factor, 0.65f));
    assert(approx(r.final_effectiveness, 0.5f * 0.65f));

    printf("  PASS: base 0.5 * factor 0.65 = 0.325 final\n");
}

// ---------------------------------------------------------------------------
// All 4 service types individually
// ---------------------------------------------------------------------------

void test_all_service_types() {
    printf("Testing all 4 service types...\n");

    for (uint8_t i = 0; i < 4; ++i) {
        auto r = calculate_funded_effectiveness(i, 1.0f, 100);
        assert(r.service_type == i);
        assert(approx(r.final_effectiveness, 1.0f));
    }

    printf("  PASS: All 4 service types produce correct results\n");
}

// ---------------------------------------------------------------------------
// Batch: calculate_all_funded_effectiveness
// ---------------------------------------------------------------------------

void test_batch_default_funding() {
    printf("Testing batch with default treasury (all 100%%)...\n");

    TreasuryState ts; // defaults: all funding = 100

    auto all = calculate_all_funded_effectiveness(ts);

    for (int i = 0; i < 4; ++i) {
        assert(all.services[i].service_type == static_cast<uint8_t>(i));
        assert(all.services[i].funding_level == 100);
        assert(approx(all.services[i].effectiveness_factor, 1.0f));
        assert(approx(all.services[i].final_effectiveness, 1.0f));
    }

    printf("  PASS: Batch with default funding all at 1.0\n");
}

void test_batch_varied_funding() {
    printf("Testing batch with varied funding levels...\n");

    TreasuryState ts;
    ts.funding_enforcer = 0;
    ts.funding_hazard_response = 50;
    ts.funding_medical = 100;
    ts.funding_education = 150;

    auto all = calculate_all_funded_effectiveness(ts);

    // Enforcer: 0% -> 0.0
    assert(all.services[0].funding_level == 0);
    assert(approx(all.services[0].final_effectiveness, 0.0f));

    // HazardResponse: 50% -> 0.65
    assert(all.services[1].funding_level == 50);
    assert(approx(all.services[1].final_effectiveness, 0.65f));

    // Medical: 100% -> 1.0
    assert(all.services[2].funding_level == 100);
    assert(approx(all.services[2].final_effectiveness, 1.0f));

    // Education: 150% -> 1.10
    assert(all.services[3].funding_level == 150);
    assert(approx(all.services[3].final_effectiveness, 1.10f));

    printf("  PASS: Batch with varied funding produces correct results\n");
}

void test_batch_custom_base_effectiveness() {
    printf("Testing batch with custom base effectiveness...\n");

    TreasuryState ts;
    ts.funding_enforcer = 100;
    ts.funding_hazard_response = 100;
    ts.funding_medical = 100;
    ts.funding_education = 100;

    auto all = calculate_all_funded_effectiveness(ts, 0.8f);

    for (int i = 0; i < 4; ++i) {
        assert(approx(all.services[i].base_effectiveness, 0.8f));
        assert(approx(all.services[i].final_effectiveness, 0.8f));
    }

    printf("  PASS: Batch with base 0.8 and 100%% funding = 0.8\n");
}

// ---------------------------------------------------------------------------
// Edge: zero base effectiveness
// ---------------------------------------------------------------------------

void test_zero_base_effectiveness() {
    printf("Testing zero base effectiveness...\n");

    auto r = calculate_funded_effectiveness(0, 0.0f, 100);

    assert(approx(r.base_effectiveness, 0.0f));
    assert(approx(r.final_effectiveness, 0.0f));

    printf("  PASS: Zero base effectiveness always produces 0.0\n");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    printf("=== ServiceFundingIntegration Unit Tests (E11-014) ===\n\n");

    // Zero funding
    test_zero_funding_inactive();

    // Full funding
    test_full_funding();

    // Over-funding
    test_over_funding();

    // Partial funding
    test_partial_funding_25();
    test_partial_funding_50();
    test_partial_funding_75();

    // Base effectiveness
    test_base_effectiveness_scaling();
    test_base_effectiveness_with_partial_funding();

    // All service types
    test_all_service_types();

    // Batch
    test_batch_default_funding();
    test_batch_varied_funding();
    test_batch_custom_base_effectiveness();

    // Edge cases
    test_zero_base_effectiveness();

    printf("\n=== All ServiceFundingIntegration tests passed! ===\n");
    return 0;
}
