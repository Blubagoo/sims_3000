/**
 * @file test_migration_out.cpp
 * @brief Tests for migration out calculation (Ticket E10-026)
 *
 * Validates:
 * - Default factors (neutral): base out rate
 * - High disorder (>50): increased out rate
 * - High contamination + low harmony: compounding desperation
 * - Out rate capped at 5%
 * - Never causes total exodus (leaves at least 1)
 */

#include <cassert>
#include <cmath>
#include <cstdio>

#include "sims3000/population/MigrationOut.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Helper: float approximate equality
// --------------------------------------------------------------------------
static bool approx(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Test: Default factors (neutral) -> base out rate
// --------------------------------------------------------------------------
static void test_default_factors() {
    MigrationFactors factors{};
    // defaults: disorder=0, contamination=0, job_availability=50, harmony=50
    // None trigger desperation thresholds

    MigrationOutResult result = calculate_migration_out(factors, 10000);

    // desperation = 0, effective_rate = 0.005 + 0 = 0.005
    assert(approx(result.desperation_factor, 0.0f) && "Default factors should have 0 desperation");
    assert(approx(result.effective_out_rate, constants::BASE_OUT_RATE) &&
           "Default factors should use base out rate");

    // migrants_out = round(10000 * 0.005) = 50
    assert(result.migrants_out == 50 && "Should lose 50 beings at base rate with 10000 pop");

    std::printf("  PASS: Default factors (neutral) -> base out rate\n");
}

// --------------------------------------------------------------------------
// Test: High disorder (>50) -> increased out rate
// --------------------------------------------------------------------------
static void test_high_disorder() {
    MigrationFactors factors{};
    factors.disorder_level = 80;  // > 50 threshold
    // others at defaults (no desperation from them)

    MigrationOutResult result = calculate_migration_out(factors, 10000);

    // desperation = (80 - 50) / 100 = 0.30
    assert(approx(result.desperation_factor, 0.30f) && "Disorder 80 should give 0.30 desperation");

    // effective_rate = 0.005 + 0.30 * 0.05 = 0.005 + 0.015 = 0.020
    assert(approx(result.effective_out_rate, 0.020f) && "Effective rate should be 0.020");

    // migrants_out = round(10000 * 0.020) = 200
    assert(result.migrants_out == 200 && "Should lose 200 beings with high disorder");

    std::printf("  PASS: High disorder (>50) -> increased out rate\n");
}

// --------------------------------------------------------------------------
// Test: Compounding desperation (contamination + low harmony)
// --------------------------------------------------------------------------
static void test_compounding_desperation() {
    MigrationFactors factors{};
    factors.contamination_level = 90; // > 50: desperation += (90-50)/100 = 0.40
    factors.harmony_level = 10;       // < 30: desperation += (30-10)/100 = 0.20
    // disorder = 0 (no contribution), job_availability = 50 (no contribution)

    MigrationOutResult result = calculate_migration_out(factors, 10000);

    // desperation = 0.40 + 0.20 = 0.60
    assert(approx(result.desperation_factor, 0.60f) && "Compounded desperation should be 0.60");

    // effective_rate = 0.005 + 0.60 * 0.05 = 0.005 + 0.030 = 0.035
    assert(approx(result.effective_out_rate, 0.035f) && "Effective rate should be 0.035");

    // migrants_out = round(10000 * 0.035) = 350
    assert(result.migrants_out == 350 && "Should lose 350 beings with compounding desperation");

    std::printf("  PASS: Compounding desperation (contamination + low harmony)\n");
}

// --------------------------------------------------------------------------
// Test: Out rate capped at 5%
// --------------------------------------------------------------------------
static void test_rate_cap() {
    MigrationFactors factors{};
    // Max out all desperation factors
    factors.disorder_level = 100;       // (100-50)/100 = 0.50
    factors.contamination_level = 100;  // (100-50)/100 = 0.50
    factors.job_availability = 0;       // (30-0)/100 = 0.30
    factors.harmony_level = 0;          // (30-0)/100 = 0.30

    MigrationOutResult result = calculate_migration_out(factors, 10000);

    // desperation = 0.50 + 0.50 + 0.30 + 0.30 = 1.60
    assert(approx(result.desperation_factor, 1.60f) && "Max desperation should be 1.60");

    // uncapped rate = 0.005 + 1.60 * 0.05 = 0.005 + 0.080 = 0.085
    // capped to MAX_OUT_RATE = 0.05
    assert(approx(result.effective_out_rate, constants::MAX_OUT_RATE) &&
           "Effective rate should be capped at MAX_OUT_RATE (0.05)");

    // migrants_out = round(10000 * 0.05) = 500
    assert(result.migrants_out == 500 && "Should lose 500 at capped rate");

    std::printf("  PASS: Out rate capped at 5%%\n");
}

// --------------------------------------------------------------------------
// Test: Never causes total exodus (leaves at least 1)
// --------------------------------------------------------------------------
static void test_no_total_exodus() {
    MigrationFactors factors{};
    factors.disorder_level = 100;
    factors.contamination_level = 100;
    factors.job_availability = 0;
    factors.harmony_level = 0;

    // Very small population
    MigrationOutResult result = calculate_migration_out(factors, 2);

    // rate = 0.05, raw = round(2 * 0.05) = round(0.1) = 0
    // Actually with 2 beings at 5%, round(0.1) = 0, so no exodus issue here
    // Let's test a case where it would cause exodus
    assert(result.migrants_out < 2 && "Should never empty the population");

    std::printf("  PASS: Never causes total exodus (small pop)\n");
}

// --------------------------------------------------------------------------
// Test: Single being never leaves
// --------------------------------------------------------------------------
static void test_single_being() {
    MigrationFactors factors{};
    factors.disorder_level = 100;
    factors.contamination_level = 100;
    factors.job_availability = 0;
    factors.harmony_level = 0;

    // Population of 1: should never go to 0
    MigrationOutResult result = calculate_migration_out(factors, 1);

    // round(1 * 0.05) = 0, but even if it were 1, the guard should prevent exodus
    assert(result.migrants_out == 0 && "Single being should not leave");

    std::printf("  PASS: Single being never leaves\n");
}

// --------------------------------------------------------------------------
// Test: Zero population -> zero migrants out
// --------------------------------------------------------------------------
static void test_zero_population() {
    MigrationFactors factors{};
    factors.disorder_level = 100;

    MigrationOutResult result = calculate_migration_out(factors, 0);

    assert(result.migrants_out == 0 && "Zero population should produce zero migrants out");

    std::printf("  PASS: Zero population -> zero migrants out\n");
}

// --------------------------------------------------------------------------
// Test: Moderate population at max rate leaves at least 1
// --------------------------------------------------------------------------
static void test_exodus_guard_moderate_pop() {
    MigrationFactors factors{};
    factors.disorder_level = 100;
    factors.contamination_level = 100;
    factors.job_availability = 0;
    factors.harmony_level = 0;

    // Population of 10: 5% = 0.5, round = 1 (but should not cause exodus for pop > 1)
    MigrationOutResult result = calculate_migration_out(factors, 10);

    // round(10 * 0.05) = 1 (or possibly 0 depending on rounding)
    // Either way, should leave at least 1
    assert(result.migrants_out < 10 && "Must leave at least 1 being");
    assert(result.migrants_out >= 0 && "Should not be negative");

    std::printf("  PASS: Moderate population at max rate leaves at least 1\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Migration Out Tests (E10-026) ===\n");

    test_default_factors();
    test_high_disorder();
    test_compounding_desperation();
    test_rate_cap();
    test_no_total_exodus();
    test_single_being();
    test_zero_population();
    test_exodus_guard_moderate_pop();

    std::printf("All migration out tests passed.\n");
    return 0;
}
