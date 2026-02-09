/**
 * @file test_migration_in.cpp
 * @brief Tests for migration in calculation (Ticket E10-025)
 *
 * Validates:
 * - Positive attraction: migration in > 0
 * - Negative attraction (<-50): migration in = 0
 * - Capped by available housing
 * - Colony size bonus: larger colonies attract more
 * - Neutral attraction: ~BASE_MIGRATION
 */

#include <cassert>
#include <cmath>
#include <cstdio>

#include "sims3000/population/MigrationIn.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Helper: float approximate equality
// --------------------------------------------------------------------------
static bool approx(float a, float b, float epsilon = 0.01f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Test: Positive attraction -> migration in > 0
// --------------------------------------------------------------------------
static void test_positive_attraction() {
    // net_attraction = +50, moderate city, plenty of housing
    MigrationInResult result = calculate_migration_in(50, 1000, 10000);

    assert(result.migrants_in > 0 && "Positive attraction should produce migrants");
    assert(result.attraction_multiplier > 1.0f && "Positive attraction should have mult > 1");

    // attraction_normalized = (50+100)/200 = 0.75
    // mult = 0 + 0.75 * 2 = 1.5
    assert(approx(result.attraction_multiplier, 1.5f) && "Multiplier should be 1.5");

    // colony_size_bonus = 1000 * 0.001 = 1.0
    assert(approx(result.colony_size_bonus, 1.0f) && "Colony bonus should be 1.0");

    // raw = (50 + 1.0) * 1.5 = 76.5 -> round = 77 (well under 10000 housing)
    assert(result.migrants_in == 77 && "Should have ~77 migrants");

    std::printf("  PASS: Positive attraction -> migration in > 0\n");
}

// --------------------------------------------------------------------------
// Test: Negative attraction (<-50) -> migration in = 0
// --------------------------------------------------------------------------
static void test_very_negative_attraction() {
    MigrationInResult result = calculate_migration_in(-51, 5000, 10000);

    assert(result.migrants_in == 0 && "Very negative attraction should block migration");
    assert(approx(result.attraction_multiplier, 0.0f) && "Multiplier should be 0");

    std::printf("  PASS: Negative attraction (<-50) -> migration in = 0\n");
}

// --------------------------------------------------------------------------
// Test: Exactly -50 should still allow migration
// --------------------------------------------------------------------------
static void test_boundary_negative_attraction() {
    MigrationInResult result = calculate_migration_in(-50, 1000, 10000);

    // net_attraction = -50 is NOT < -50, so migration should proceed
    // attraction_normalized = (-50+100)/200 = 0.25
    // mult = 0 + 0.25 * 2 = 0.5
    assert(result.migrants_in > 0 && "Exactly -50 should still allow some migration");
    assert(approx(result.attraction_multiplier, 0.5f) && "Multiplier at -50 should be 0.5");

    std::printf("  PASS: Exactly -50 boundary -> still allows migration\n");
}

// --------------------------------------------------------------------------
// Test: Capped by available housing
// --------------------------------------------------------------------------
static void test_capped_by_housing() {
    // Very attractive, big colony, but only 5 housing units available
    MigrationInResult result = calculate_migration_in(100, 100000, 5);

    assert(result.migrants_in <= 5 && "Migration should be capped by available housing");
    assert(result.migrants_in == 5 && "Should fill all available housing");

    std::printf("  PASS: Capped by available housing\n");
}

// --------------------------------------------------------------------------
// Test: Colony size bonus -> larger colonies attract more
// --------------------------------------------------------------------------
static void test_colony_size_bonus() {
    // Small colony
    MigrationInResult result_small = calculate_migration_in(50, 100, 10000);
    // Large colony
    MigrationInResult result_large = calculate_migration_in(50, 100000, 10000);

    assert(result_large.colony_size_bonus > result_small.colony_size_bonus &&
           "Larger colony should have bigger bonus");
    assert(result_large.migrants_in > result_small.migrants_in &&
           "Larger colony should attract more migrants");

    // Small: bonus = 100 * 0.001 = 0.1, raw = (50+0.1)*1.5 = 75.15 -> 75
    assert(approx(result_small.colony_size_bonus, 0.1f) && "Small colony bonus should be 0.1");

    // Large: bonus = 100000 * 0.001 = 100, raw = (50+100)*1.5 = 225 -> 225
    assert(approx(result_large.colony_size_bonus, 100.0f) && "Large colony bonus should be 100");

    std::printf("  PASS: Colony size bonus -> larger colonies attract more\n");
}

// --------------------------------------------------------------------------
// Test: Neutral attraction -> approximately BASE_MIGRATION
// --------------------------------------------------------------------------
static void test_neutral_attraction() {
    // net_attraction = 0 (neutral), small colony to minimize bonus
    MigrationInResult result = calculate_migration_in(0, 0, 10000);

    // attraction_normalized = (0+100)/200 = 0.5
    // mult = 0 + 0.5 * 2 = 1.0
    assert(approx(result.attraction_multiplier, 1.0f) && "Neutral should have multiplier 1.0");

    // colony_size_bonus = 0 (no beings)
    // raw = (50 + 0) * 1.0 = 50
    assert(result.migrants_in == constants::BASE_MIGRATION &&
           "Neutral attraction with no colony should give BASE_MIGRATION");

    std::printf("  PASS: Neutral attraction -> ~BASE_MIGRATION\n");
}

// --------------------------------------------------------------------------
// Test: Zero housing -> no migration
// --------------------------------------------------------------------------
static void test_zero_housing() {
    MigrationInResult result = calculate_migration_in(100, 10000, 0);

    assert(result.migrants_in == 0 && "Zero housing should block all migration");

    std::printf("  PASS: Zero housing -> no migration\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Migration In Tests (E10-025) ===\n");

    test_positive_attraction();
    test_very_negative_attraction();
    test_boundary_negative_attraction();
    test_capped_by_housing();
    test_colony_size_bonus();
    test_neutral_attraction();
    test_zero_housing();

    std::printf("All migration in tests passed.\n");
    return 0;
}
