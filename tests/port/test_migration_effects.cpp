/**
 * @file test_migration_effects.cpp
 * @brief Unit tests for population migration calculation (Epic 8, Ticket E8-024)
 *
 * Tests cover:
 * - Default input produces expected defaults
 * - Immigration formula: capacity * demand * harmony
 * - Immigration cap: 10 + (connections * 5)
 * - Emigration formula: capacity * (disorder / 100) * tribute
 * - High harmony attracts immigration
 * - High disorder causes emigration
 * - External connections amplify migration effects
 * - Input clamping for out-of-range values
 * - Zero capacity produces zero migration
 * - Net migration calculation
 * - Edge cases
 */

#include <sims3000/port/MigrationEffects.h>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000::port;

// =============================================================================
// Helper: integer comparison with tolerance for float->int truncation
// =============================================================================

static bool approx_eq_i(int32_t actual, int32_t expected, int32_t tolerance = 1) {
    return std::abs(actual - expected) <= tolerance;
}

// =============================================================================
// Default Input Tests
// =============================================================================

void test_default_input() {
    printf("Testing default MigrationInput values...\n");

    MigrationInput input;
    assert(input.total_migration_capacity == 0);
    assert(input.external_connection_count == 0);
    assert(std::fabs(input.demand_factor - 1.0f) < 0.01f);
    assert(std::fabs(input.harmony_factor - 0.5f) < 0.01f);
    assert(std::fabs(input.disorder_index) < 0.01f);
    assert(std::fabs(input.tribute_penalty - 1.0f) < 0.01f);

    printf("  PASS: Default values correct\n");
}

void test_zero_capacity() {
    printf("Testing zero migration capacity...\n");

    MigrationInput input;
    input.total_migration_capacity = 0;
    input.external_connection_count = 0;

    MigrationResult result = calculate_migration(input);
    assert(result.immigration_rate == 0);
    assert(result.emigration_rate == 0);
    assert(result.net_migration == 0);
    assert(result.max_immigration == 10);  // 10 + (0 * 5) = 10

    printf("  PASS: Zero capacity -> zero migration\n");
}

// =============================================================================
// Immigration Formula Tests
// =============================================================================

void test_basic_immigration() {
    printf("Testing basic immigration formula...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 4;
    input.demand_factor = 1.0f;
    input.harmony_factor = 1.0f;
    input.disorder_index = 0.0f;

    // immigration = 100 * 1.0 * 1.0 = 100
    // max_immigration = 10 + (4 * 5) = 30
    // capped at 30
    MigrationResult result = calculate_migration(input);
    assert(result.immigration_rate == 30);
    assert(result.max_immigration == 30);

    printf("  PASS: immigration 100 capped at max 30\n");
}

void test_immigration_under_cap() {
    printf("Testing immigration under cap...\n");

    MigrationInput input;
    input.total_migration_capacity = 20;
    input.external_connection_count = 10;
    input.demand_factor = 1.0f;
    input.harmony_factor = 0.5f;
    input.disorder_index = 0.0f;

    // immigration = 20 * 1.0 * 0.5 = 10
    // max_immigration = 10 + (10 * 5) = 60
    // 10 < 60, so not capped
    MigrationResult result = calculate_migration(input);
    assert(result.immigration_rate == 10);
    assert(result.max_immigration == 60);

    printf("  PASS: immigration 10 (under cap of 60)\n");
}

void test_demand_factor_high() {
    printf("Testing high demand factor increases immigration...\n");

    MigrationInput input;
    input.total_migration_capacity = 40;
    input.external_connection_count = 20;
    input.demand_factor = 1.5f;
    input.harmony_factor = 1.0f;
    input.disorder_index = 0.0f;

    // immigration = 40 * 1.5 * 1.0 = 60
    // max_immigration = 10 + (20 * 5) = 110
    // 60 < 110, not capped
    MigrationResult result = calculate_migration(input);
    assert(result.immigration_rate == 60);

    printf("  PASS: High demand (1.5) -> immigration = 60\n");
}

void test_demand_factor_low() {
    printf("Testing low demand factor reduces immigration...\n");

    MigrationInput input;
    input.total_migration_capacity = 40;
    input.external_connection_count = 20;
    input.demand_factor = 0.5f;
    input.harmony_factor = 1.0f;
    input.disorder_index = 0.0f;

    // immigration = 40 * 0.5 * 1.0 = 20
    MigrationResult result = calculate_migration(input);
    assert(result.immigration_rate == 20);

    printf("  PASS: Low demand (0.5) -> immigration = 20\n");
}

// =============================================================================
// High Harmony Attracts Immigration
// =============================================================================

void test_high_harmony_attracts() {
    printf("Testing high harmony attracts immigration...\n");

    MigrationInput base;
    base.total_migration_capacity = 100;
    base.external_connection_count = 50;
    base.demand_factor = 1.0f;
    base.disorder_index = 0.0f;

    // Low harmony
    base.harmony_factor = 0.2f;
    MigrationResult low = calculate_migration(base);

    // High harmony
    base.harmony_factor = 0.9f;
    MigrationResult high = calculate_migration(base);

    assert(high.immigration_rate > low.immigration_rate);

    printf("  PASS: harmony 0.9 -> %d > harmony 0.2 -> %d\n",
           high.immigration_rate, low.immigration_rate);
}

void test_zero_harmony_zero_immigration() {
    printf("Testing zero harmony produces zero immigration...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 5;
    input.demand_factor = 1.0f;
    input.harmony_factor = 0.0f;
    input.disorder_index = 0.0f;

    // immigration = 100 * 1.0 * 0.0 = 0
    MigrationResult result = calculate_migration(input);
    assert(result.immigration_rate == 0);

    printf("  PASS: Zero harmony -> zero immigration\n");
}

// =============================================================================
// High Disorder Causes Emigration
// =============================================================================

void test_high_disorder_emigration() {
    printf("Testing high disorder causes emigration...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 5;
    input.demand_factor = 1.0f;
    input.harmony_factor = 0.5f;
    input.disorder_index = 80.0f;
    input.tribute_penalty = 1.0f;

    // emigration = 100 * (80 / 100) * 1.0 = 80
    MigrationResult result = calculate_migration(input);
    assert(result.emigration_rate == 80);

    printf("  PASS: disorder 80 -> emigration = %d\n", result.emigration_rate);
}

void test_zero_disorder_no_emigration() {
    printf("Testing zero disorder produces no emigration...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 5;
    input.demand_factor = 1.0f;
    input.harmony_factor = 0.5f;
    input.disorder_index = 0.0f;
    input.tribute_penalty = 1.0f;

    // emigration = 100 * (0 / 100) * 1.0 = 0
    MigrationResult result = calculate_migration(input);
    assert(result.emigration_rate == 0);

    printf("  PASS: Zero disorder -> zero emigration\n");
}

void test_max_disorder_emigration() {
    printf("Testing max disorder (100) emigration...\n");

    MigrationInput input;
    input.total_migration_capacity = 200;
    input.external_connection_count = 5;
    input.demand_factor = 1.0f;
    input.harmony_factor = 0.5f;
    input.disorder_index = 100.0f;
    input.tribute_penalty = 1.0f;

    // emigration = 200 * (100 / 100) * 1.0 = 200
    MigrationResult result = calculate_migration(input);
    assert(result.emigration_rate == 200);

    printf("  PASS: Max disorder -> emigration = %d\n", result.emigration_rate);
}

// =============================================================================
// Tribute Penalty Tests
// =============================================================================

void test_tribute_penalty_amplifies_emigration() {
    printf("Testing tribute penalty amplifies emigration...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 5;
    input.demand_factor = 1.0f;
    input.harmony_factor = 0.5f;
    input.disorder_index = 50.0f;

    // No penalty
    input.tribute_penalty = 1.0f;
    MigrationResult no_penalty = calculate_migration(input);

    // With penalty
    input.tribute_penalty = 2.0f;
    MigrationResult with_penalty = calculate_migration(input);

    // no_penalty: 100 * (50/100) * 1.0 = 50
    // with_penalty: 100 * (50/100) * 2.0 = 100
    assert(no_penalty.emigration_rate == 50);
    assert(with_penalty.emigration_rate == 100);

    printf("  PASS: tribute 1.0 -> %d, tribute 2.0 -> %d\n",
           no_penalty.emigration_rate, with_penalty.emigration_rate);
}

// =============================================================================
// External Connections Amplify Migration
// =============================================================================

void test_connections_amplify_immigration_cap() {
    printf("Testing external connections amplify immigration cap...\n");

    MigrationInput input;
    input.total_migration_capacity = 1000;  // High capacity
    input.demand_factor = 1.0f;
    input.harmony_factor = 1.0f;
    input.disorder_index = 0.0f;

    // Few connections
    input.external_connection_count = 2;
    MigrationResult few = calculate_migration(input);
    // max = 10 + (2 * 5) = 20

    // Many connections
    input.external_connection_count = 20;
    MigrationResult many = calculate_migration(input);
    // max = 10 + (20 * 5) = 110

    assert(few.max_immigration == 20);
    assert(many.max_immigration == 110);
    assert(many.immigration_rate > few.immigration_rate);

    printf("  PASS: 2 connections -> cap %d, 20 connections -> cap %d\n",
           few.max_immigration, many.max_immigration);
}

void test_more_capacity_more_emigration() {
    printf("Testing more capacity means more emigration...\n");

    MigrationInput base;
    base.external_connection_count = 5;
    base.demand_factor = 1.0f;
    base.harmony_factor = 0.5f;
    base.disorder_index = 50.0f;
    base.tribute_penalty = 1.0f;

    base.total_migration_capacity = 50;
    MigrationResult low_cap = calculate_migration(base);

    base.total_migration_capacity = 200;
    MigrationResult high_cap = calculate_migration(base);

    // low: 50 * (50/100) * 1.0 = 25
    // high: 200 * (50/100) * 1.0 = 100
    assert(low_cap.emigration_rate == 25);
    assert(high_cap.emigration_rate == 100);

    printf("  PASS: capacity 50 -> emigration %d, capacity 200 -> emigration %d\n",
           low_cap.emigration_rate, high_cap.emigration_rate);
}

// =============================================================================
// Net Migration Tests
// =============================================================================

void test_positive_net_migration() {
    printf("Testing positive net migration (more immigration)...\n");

    MigrationInput input;
    input.total_migration_capacity = 50;
    input.external_connection_count = 20;
    input.demand_factor = 1.0f;
    input.harmony_factor = 1.0f;
    input.disorder_index = 10.0f;
    input.tribute_penalty = 1.0f;

    // immigration = 50 * 1.0 * 1.0 = 50
    // max = 10 + (20 * 5) = 110, so not capped -> 50
    // emigration = 50 * (10/100) * 1.0 = 5
    // net = 50 - 5 = 45
    MigrationResult result = calculate_migration(input);
    assert(result.immigration_rate == 50);
    assert(result.emigration_rate == 5);
    assert(result.net_migration == 45);

    printf("  PASS: net migration = %d (immigration %d - emigration %d)\n",
           result.net_migration, result.immigration_rate, result.emigration_rate);
}

void test_negative_net_migration() {
    printf("Testing negative net migration (more emigration)...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 2;
    input.demand_factor = 0.5f;
    input.harmony_factor = 0.2f;
    input.disorder_index = 90.0f;
    input.tribute_penalty = 1.5f;

    // immigration = 100 * 0.5 * 0.2 = 10
    // max = 10 + (2 * 5) = 20, so not capped -> 10
    // emigration = 100 * (90/100) * 1.5 = 135
    // net = 10 - 135 = -125
    MigrationResult result = calculate_migration(input);
    assert(result.immigration_rate == 10);
    assert(result.emigration_rate == 135);
    assert(result.net_migration == -125);

    printf("  PASS: net migration = %d (deeply negative)\n", result.net_migration);
}

// =============================================================================
// Input Clamping Tests
// =============================================================================

void test_demand_factor_clamped_below() {
    printf("Testing demand factor clamped to 0.5 minimum...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 50;
    input.demand_factor = 0.0f;  // Should clamp to 0.5
    input.harmony_factor = 1.0f;
    input.disorder_index = 0.0f;

    // immigration = 100 * 0.5 * 1.0 = 50 (clamped demand)
    MigrationResult result = calculate_migration(input);
    assert(result.immigration_rate == 50);

    printf("  PASS: demand 0.0 clamped to 0.5 -> immigration = %d\n",
           result.immigration_rate);
}

void test_demand_factor_clamped_above() {
    printf("Testing demand factor clamped to 1.5 maximum...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 50;
    input.demand_factor = 3.0f;  // Should clamp to 1.5
    input.harmony_factor = 1.0f;
    input.disorder_index = 0.0f;

    // immigration = 100 * 1.5 * 1.0 = 150
    // max = 10 + (50 * 5) = 260, not capped
    MigrationResult result = calculate_migration(input);
    assert(result.immigration_rate == 150);

    printf("  PASS: demand 3.0 clamped to 1.5 -> immigration = %d\n",
           result.immigration_rate);
}

void test_harmony_clamped() {
    printf("Testing harmony factor clamped to [0.0, 1.0]...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 50;
    input.demand_factor = 1.0f;
    input.disorder_index = 0.0f;

    // Below: clamp to 0.0
    input.harmony_factor = -0.5f;
    MigrationResult low = calculate_migration(input);
    assert(low.immigration_rate == 0);

    // Above: clamp to 1.0
    input.harmony_factor = 2.0f;
    MigrationResult high = calculate_migration(input);
    assert(high.immigration_rate == 100);

    printf("  PASS: harmony -0.5 -> %d, harmony 2.0 -> %d\n",
           low.immigration_rate, high.immigration_rate);
}

void test_disorder_clamped() {
    printf("Testing disorder index clamped to [0, 100]...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 5;
    input.demand_factor = 1.0f;
    input.harmony_factor = 0.5f;
    input.tribute_penalty = 1.0f;

    // Below: clamp to 0
    input.disorder_index = -10.0f;
    MigrationResult low = calculate_migration(input);
    assert(low.emigration_rate == 0);

    // Above: clamp to 100
    input.disorder_index = 200.0f;
    MigrationResult high = calculate_migration(input);
    assert(high.emigration_rate == 100);

    printf("  PASS: disorder -10 -> emigration %d, disorder 200 -> emigration %d\n",
           low.emigration_rate, high.emigration_rate);
}

void test_tribute_clamped_minimum() {
    printf("Testing tribute penalty clamped to minimum 1.0...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 5;
    input.demand_factor = 1.0f;
    input.harmony_factor = 0.5f;
    input.disorder_index = 50.0f;
    input.tribute_penalty = 0.5f;  // Should clamp to 1.0

    // emigration = 100 * (50/100) * 1.0 = 50 (clamped tribute)
    MigrationResult result = calculate_migration(input);
    assert(result.emigration_rate == 50);

    printf("  PASS: tribute 0.5 clamped to 1.0 -> emigration = %d\n",
           result.emigration_rate);
}

// =============================================================================
// Immigration Cap Tests
// =============================================================================

void test_immigration_cap_zero_connections() {
    printf("Testing immigration cap with zero connections...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 0;
    input.demand_factor = 1.0f;
    input.harmony_factor = 1.0f;
    input.disorder_index = 0.0f;

    // max = 10 + (0 * 5) = 10
    // immigration = 100 * 1.0 * 1.0 = 100, capped at 10
    MigrationResult result = calculate_migration(input);
    assert(result.max_immigration == 10);
    assert(result.immigration_rate == 10);

    printf("  PASS: 0 connections -> cap = %d, immigration = %d\n",
           result.max_immigration, result.immigration_rate);
}

void test_immigration_cap_many_connections() {
    printf("Testing immigration cap with many connections...\n");

    MigrationInput input;
    input.total_migration_capacity = 500;
    input.external_connection_count = 100;
    input.demand_factor = 1.0f;
    input.harmony_factor = 1.0f;
    input.disorder_index = 0.0f;

    // max = 10 + (100 * 5) = 510
    // immigration = 500 * 1.0 * 1.0 = 500, under cap
    MigrationResult result = calculate_migration(input);
    assert(result.max_immigration == 510);
    assert(result.immigration_rate == 500);

    printf("  PASS: 100 connections -> cap = %d, immigration = %d\n",
           result.max_immigration, result.immigration_rate);
}

// =============================================================================
// Combined Scenario Tests
// =============================================================================

void test_balanced_city() {
    printf("Testing balanced city scenario...\n");

    MigrationInput input;
    input.total_migration_capacity = 75;
    input.external_connection_count = 6;
    input.demand_factor = 1.0f;
    input.harmony_factor = 0.5f;
    input.disorder_index = 25.0f;
    input.tribute_penalty = 1.0f;

    // immigration = 75 * 1.0 * 0.5 = 37
    // max = 10 + (6 * 5) = 40, so 37 not capped
    // emigration = 75 * (25/100) * 1.0 = 18
    // net = 37 - 18 = 19
    MigrationResult result = calculate_migration(input);
    assert(approx_eq_i(result.immigration_rate, 37));
    assert(approx_eq_i(result.emigration_rate, 18));
    assert(approx_eq_i(result.net_migration, 19));

    printf("  PASS: balanced city -> net migration = %d\n", result.net_migration);
}

void test_thriving_city() {
    printf("Testing thriving city scenario (high harmony, low disorder)...\n");

    MigrationInput input;
    input.total_migration_capacity = 150;
    input.external_connection_count = 10;
    input.demand_factor = 1.5f;
    input.harmony_factor = 0.9f;
    input.disorder_index = 5.0f;
    input.tribute_penalty = 1.0f;

    // immigration = 150 * 1.5 * 0.9 = 202
    // max = 10 + (10 * 5) = 60, capped at 60
    // emigration = 150 * (5/100) * 1.0 = 7
    // net = 60 - 7 = 53
    MigrationResult result = calculate_migration(input);
    assert(result.immigration_rate == 60);
    assert(approx_eq_i(result.emigration_rate, 7));
    assert(approx_eq_i(result.net_migration, 53));

    printf("  PASS: thriving city -> net migration = %d\n", result.net_migration);
}

void test_struggling_city() {
    printf("Testing struggling city scenario (low harmony, high disorder, tribute)...\n");

    MigrationInput input;
    input.total_migration_capacity = 100;
    input.external_connection_count = 3;
    input.demand_factor = 0.5f;
    input.harmony_factor = 0.1f;
    input.disorder_index = 80.0f;
    input.tribute_penalty = 1.5f;

    // immigration = 100 * 0.5 * 0.1 = 5
    // max = 10 + (3 * 5) = 25, 5 not capped
    // emigration = 100 * (80/100) * 1.5 = 120
    // net = 5 - 120 = -115
    MigrationResult result = calculate_migration(input);
    assert(result.immigration_rate == 5);
    assert(result.emigration_rate == 120);
    assert(result.net_migration == -115);

    printf("  PASS: struggling city -> net migration = %d\n", result.net_migration);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Migration Effects Tests (Epic 8, Ticket E8-024) ===\n\n");

    // Default / zero
    test_default_input();
    test_zero_capacity();

    // Immigration formula
    test_basic_immigration();
    test_immigration_under_cap();
    test_demand_factor_high();
    test_demand_factor_low();

    // High harmony attracts immigration
    test_high_harmony_attracts();
    test_zero_harmony_zero_immigration();

    // High disorder causes emigration
    test_high_disorder_emigration();
    test_zero_disorder_no_emigration();
    test_max_disorder_emigration();

    // Tribute penalty
    test_tribute_penalty_amplifies_emigration();

    // External connections amplify migration
    test_connections_amplify_immigration_cap();
    test_more_capacity_more_emigration();

    // Net migration
    test_positive_net_migration();
    test_negative_net_migration();

    // Input clamping
    test_demand_factor_clamped_below();
    test_demand_factor_clamped_above();
    test_harmony_clamped();
    test_disorder_clamped();
    test_tribute_clamped_minimum();

    // Immigration cap
    test_immigration_cap_zero_connections();
    test_immigration_cap_many_connections();

    // Combined scenarios
    test_balanced_city();
    test_thriving_city();
    test_struggling_city();

    printf("\n=== All Migration Effects Tests Passed ===\n");
    return 0;
}
