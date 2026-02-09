/**
 * @file test_population_stats.cpp
 * @brief Tests for population stat query interface (Ticket E10-030)
 *
 * Validates:
 * - All stat IDs return correct values
 * - Stat names are correct
 * - Stat ID validation
 * - Invalid stat IDs return 0.0f
 * - Life expectancy calculation integration
 */

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "sims3000/population/PopulationStats.h"
#include "sims3000/population/PopulationData.h"
#include "sims3000/population/EmploymentData.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Helper: float approximate equality
// --------------------------------------------------------------------------
static bool approx(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Test: STAT_TOTAL_BEINGS
// --------------------------------------------------------------------------
static void test_stat_total_beings() {
    PopulationData pop{};
    EmploymentData emp{};
    pop.total_beings = 5000;

    float value = get_population_stat(pop, emp, STAT_TOTAL_BEINGS);
    assert(approx(value, 5000.0f) && "STAT_TOTAL_BEINGS should return total_beings");

    const char* name = get_population_stat_name(STAT_TOTAL_BEINGS);
    assert(name != nullptr && "Name should not be null");
    assert(std::strcmp(name, "Total Population") == 0 && "Name should be 'Total Population'");

    std::printf("  PASS: STAT_TOTAL_BEINGS\n");
}

// --------------------------------------------------------------------------
// Test: STAT_BIRTH_RATE
// --------------------------------------------------------------------------
static void test_stat_birth_rate() {
    PopulationData pop{};
    EmploymentData emp{};
    pop.birth_rate_per_1000 = 15;

    float value = get_population_stat(pop, emp, STAT_BIRTH_RATE);
    assert(approx(value, 15.0f) && "STAT_BIRTH_RATE should return birth_rate_per_1000");

    const char* name = get_population_stat_name(STAT_BIRTH_RATE);
    assert(std::strcmp(name, "Birth Rate") == 0 && "Name should be 'Birth Rate'");

    std::printf("  PASS: STAT_BIRTH_RATE\n");
}

// --------------------------------------------------------------------------
// Test: STAT_DEATH_RATE
// --------------------------------------------------------------------------
static void test_stat_death_rate() {
    PopulationData pop{};
    EmploymentData emp{};
    pop.death_rate_per_1000 = 8;

    float value = get_population_stat(pop, emp, STAT_DEATH_RATE);
    assert(approx(value, 8.0f) && "STAT_DEATH_RATE should return death_rate_per_1000");

    const char* name = get_population_stat_name(STAT_DEATH_RATE);
    assert(std::strcmp(name, "Death Rate") == 0 && "Name should be 'Death Rate'");

    std::printf("  PASS: STAT_DEATH_RATE\n");
}

// --------------------------------------------------------------------------
// Test: STAT_GROWTH_RATE
// --------------------------------------------------------------------------
static void test_stat_growth_rate() {
    PopulationData pop{};
    EmploymentData emp{};
    pop.growth_rate = 0.025f;  // 2.5% growth

    float value = get_population_stat(pop, emp, STAT_GROWTH_RATE);
    assert(approx(value, 2.5f) && "STAT_GROWTH_RATE should return growth_rate * 100");

    const char* name = get_population_stat_name(STAT_GROWTH_RATE);
    assert(std::strcmp(name, "Growth Rate") == 0 && "Name should be 'Growth Rate'");

    std::printf("  PASS: STAT_GROWTH_RATE\n");
}

// --------------------------------------------------------------------------
// Test: STAT_HARMONY
// --------------------------------------------------------------------------
static void test_stat_harmony() {
    PopulationData pop{};
    EmploymentData emp{};
    pop.harmony_index = 75;

    float value = get_population_stat(pop, emp, STAT_HARMONY);
    assert(approx(value, 75.0f) && "STAT_HARMONY should return harmony_index");

    const char* name = get_population_stat_name(STAT_HARMONY);
    assert(std::strcmp(name, "Harmony") == 0 && "Name should be 'Harmony'");

    std::printf("  PASS: STAT_HARMONY\n");
}

// --------------------------------------------------------------------------
// Test: STAT_HEALTH
// --------------------------------------------------------------------------
static void test_stat_health() {
    PopulationData pop{};
    EmploymentData emp{};
    pop.health_index = 60;

    float value = get_population_stat(pop, emp, STAT_HEALTH);
    assert(approx(value, 60.0f) && "STAT_HEALTH should return health_index");

    const char* name = get_population_stat_name(STAT_HEALTH);
    assert(std::strcmp(name, "Health") == 0 && "Name should be 'Health'");

    std::printf("  PASS: STAT_HEALTH\n");
}

// --------------------------------------------------------------------------
// Test: STAT_EDUCATION
// --------------------------------------------------------------------------
static void test_stat_education() {
    PopulationData pop{};
    EmploymentData emp{};
    pop.education_index = 80;

    float value = get_population_stat(pop, emp, STAT_EDUCATION);
    assert(approx(value, 80.0f) && "STAT_EDUCATION should return education_index");

    const char* name = get_population_stat_name(STAT_EDUCATION);
    assert(std::strcmp(name, "Education") == 0 && "Name should be 'Education'");

    std::printf("  PASS: STAT_EDUCATION\n");
}

// --------------------------------------------------------------------------
// Test: STAT_UNEMPLOYMENT
// --------------------------------------------------------------------------
static void test_stat_unemployment() {
    PopulationData pop{};
    EmploymentData emp{};
    emp.unemployment_rate = 12;  // 12% unemployment

    float value = get_population_stat(pop, emp, STAT_UNEMPLOYMENT);
    assert(approx(value, 12.0f) && "STAT_UNEMPLOYMENT should return unemployment_rate");

    const char* name = get_population_stat_name(STAT_UNEMPLOYMENT);
    assert(std::strcmp(name, "Unemployment Rate") == 0 && "Name should be 'Unemployment Rate'");

    std::printf("  PASS: STAT_UNEMPLOYMENT\n");
}

// --------------------------------------------------------------------------
// Test: STAT_LIFE_EXPECTANCY
// --------------------------------------------------------------------------
static void test_stat_life_expectancy() {
    PopulationData pop{};
    EmploymentData emp{};
    pop.health_index = 50;
    pop.education_index = 50;
    pop.harmony_index = 50;

    float value = get_population_stat(pop, emp, STAT_LIFE_EXPECTANCY);

    // Life expectancy should be calculated (using default contamination/disorder = 50)
    // This should give approximately 58.4 cycles based on the formula
    assert(value >= 30.0f && value <= 120.0f && "Life expectancy should be in valid range");
    assert(value > 0.0f && "Life expectancy should be positive");

    const char* name = get_population_stat_name(STAT_LIFE_EXPECTANCY);
    assert(std::strcmp(name, "Life Expectancy") == 0 && "Name should be 'Life Expectancy'");

    std::printf("  PASS: STAT_LIFE_EXPECTANCY\n");
}

// --------------------------------------------------------------------------
// Test: Invalid stat ID returns 0.0f
// --------------------------------------------------------------------------
static void test_invalid_stat_id() {
    PopulationData pop{};
    EmploymentData emp{};
    pop.total_beings = 1000;

    // Test invalid stat IDs
    float value1 = get_population_stat(pop, emp, 0);
    assert(approx(value1, 0.0f) && "Invalid stat ID 0 should return 0.0f");

    float value2 = get_population_stat(pop, emp, 999);
    assert(approx(value2, 0.0f) && "Invalid stat ID 999 should return 0.0f");

    const char* name = get_population_stat_name(999);
    assert(name == nullptr && "Invalid stat ID should return nullptr for name");

    std::printf("  PASS: Invalid stat ID returns 0.0f\n");
}

// --------------------------------------------------------------------------
// Test: Stat ID validation
// --------------------------------------------------------------------------
static void test_stat_id_validation() {
    // Valid population stat IDs (200-299)
    assert(is_valid_population_stat(200) && "200 should be valid");
    assert(is_valid_population_stat(208) && "208 should be valid");
    assert(is_valid_population_stat(299) && "299 should be valid");

    // Invalid stat IDs
    assert(!is_valid_population_stat(0) && "0 should be invalid");
    assert(!is_valid_population_stat(199) && "199 should be invalid");
    assert(!is_valid_population_stat(300) && "300 should be invalid");
    assert(!is_valid_population_stat(1000) && "1000 should be invalid");

    std::printf("  PASS: Stat ID validation\n");
}

// --------------------------------------------------------------------------
// Test: All defined stat IDs are valid
// --------------------------------------------------------------------------
static void test_all_stat_ids_valid() {
    assert(is_valid_population_stat(STAT_TOTAL_BEINGS) && "STAT_TOTAL_BEINGS should be valid");
    assert(is_valid_population_stat(STAT_BIRTH_RATE) && "STAT_BIRTH_RATE should be valid");
    assert(is_valid_population_stat(STAT_DEATH_RATE) && "STAT_DEATH_RATE should be valid");
    assert(is_valid_population_stat(STAT_GROWTH_RATE) && "STAT_GROWTH_RATE should be valid");
    assert(is_valid_population_stat(STAT_HARMONY) && "STAT_HARMONY should be valid");
    assert(is_valid_population_stat(STAT_HEALTH) && "STAT_HEALTH should be valid");
    assert(is_valid_population_stat(STAT_EDUCATION) && "STAT_EDUCATION should be valid");
    assert(is_valid_population_stat(STAT_UNEMPLOYMENT) && "STAT_UNEMPLOYMENT should be valid");
    assert(is_valid_population_stat(STAT_LIFE_EXPECTANCY) && "STAT_LIFE_EXPECTANCY should be valid");

    std::printf("  PASS: All defined stat IDs are valid\n");
}

// --------------------------------------------------------------------------
// Test: Negative growth rate
// --------------------------------------------------------------------------
static void test_negative_growth_rate() {
    PopulationData pop{};
    EmploymentData emp{};
    pop.growth_rate = -0.015f;  // -1.5% growth (shrinking)

    float value = get_population_stat(pop, emp, STAT_GROWTH_RATE);
    assert(approx(value, -1.5f) && "Negative growth rate should be handled correctly");

    std::printf("  PASS: Negative growth rate\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Population Stats Query Tests (E10-030) ===\n");

    test_stat_total_beings();
    test_stat_birth_rate();
    test_stat_death_rate();
    test_stat_growth_rate();
    test_stat_harmony();
    test_stat_health();
    test_stat_education();
    test_stat_unemployment();
    test_stat_life_expectancy();
    test_invalid_stat_id();
    test_stat_id_validation();
    test_all_stat_ids_valid();
    test_negative_growth_rate();

    std::printf("All population stats query tests passed.\n");
    return 0;
}
