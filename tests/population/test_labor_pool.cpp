/**
 * @file test_labor_pool.cpp
 * @brief Tests for labor pool calculation (Ticket E10-019)
 *
 * Validates:
 * - Default population: verify base labor participation
 * - All adults: maximum working_age_beings
 * - High harmony/education: increased participation
 * - Zero population: zero labor force
 */

#include <cassert>
#include <cmath>
#include <cstdio>

#include "sims3000/population/LaborPoolCalculation.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Helper: float approximate equality
// --------------------------------------------------------------------------
static bool approx(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Test: Default population produces expected base labor participation
// --------------------------------------------------------------------------
static void test_default_labor_participation() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.adult_percent = 34;     // Default
    pop.harmony_index = 50;
    pop.education_index = 50;

    EmploymentData emp{};

    LaborPoolResult result = calculate_labor_pool(pop, emp);

    // working_age_beings = round(1000 * 34 / 100) = 340
    assert(result.working_age_beings == 340 && "Default working age beings should be 340");

    // participation = 0.65 + (50/100)*0.10 + (50/100)*0.10 = 0.65 + 0.05 + 0.05 = 0.75
    assert(approx(result.labor_participation_rate, 0.75f) &&
           "Default participation rate should be 0.75");

    // labor_force = round(340 * 0.75) = 255
    assert(result.labor_force == 255 && "Default labor force should be 255");

    std::printf("  PASS: Default labor participation\n");
}

// --------------------------------------------------------------------------
// Test: All adults maximizes working age beings
// --------------------------------------------------------------------------
static void test_all_adults() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.adult_percent = 100;    // Everyone is working age
    pop.harmony_index = 50;
    pop.education_index = 50;

    EmploymentData emp{};

    LaborPoolResult result = calculate_labor_pool(pop, emp);

    // working_age_beings = round(1000 * 100 / 100) = 1000
    assert(result.working_age_beings == 1000 && "All-adult working age beings should be 1000");

    // labor_force = round(1000 * 0.75) = 750
    assert(result.labor_force == 750 && "All-adult labor force should be 750");

    std::printf("  PASS: All adults maximizes working age beings\n");
}

// --------------------------------------------------------------------------
// Test: High harmony and education increases participation
// --------------------------------------------------------------------------
static void test_high_harmony_education() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.adult_percent = 50;
    pop.harmony_index = 100;    // Max harmony
    pop.education_index = 100;  // Max education

    EmploymentData emp{};

    LaborPoolResult result = calculate_labor_pool(pop, emp);

    // working_age_beings = round(1000 * 50 / 100) = 500
    assert(result.working_age_beings == 500 && "Working age beings should be 500");

    // participation = 0.65 + (100/100)*0.10 + (100/100)*0.10 = 0.65 + 0.10 + 0.10 = 0.85
    assert(approx(result.labor_participation_rate, 0.85f) &&
           "Max harmony/education participation rate should be 0.85");

    // labor_force = round(500 * 0.85) = 425
    assert(result.labor_force == 425 && "High modifier labor force should be 425");

    std::printf("  PASS: High harmony/education increases participation\n");
}

// --------------------------------------------------------------------------
// Test: Zero population produces zero labor force
// --------------------------------------------------------------------------
static void test_zero_population() {
    PopulationData pop{};
    pop.total_beings = 0;

    EmploymentData emp{};

    LaborPoolResult result = calculate_labor_pool(pop, emp);

    assert(result.working_age_beings == 0 && "Zero population should have zero working age");
    assert(result.labor_force == 0 && "Zero population should have zero labor force");
    assert(result.labor_participation_rate == 0.0f && "Zero population participation rate should be 0");

    std::printf("  PASS: Zero population produces zero labor force\n");
}

// --------------------------------------------------------------------------
// Test: Low harmony and education results in base participation
// --------------------------------------------------------------------------
static void test_low_harmony_education() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.adult_percent = 50;
    pop.harmony_index = 0;      // Min harmony
    pop.education_index = 0;    // Min education

    EmploymentData emp{};

    LaborPoolResult result = calculate_labor_pool(pop, emp);

    // participation = 0.65 + 0 + 0 = 0.65
    assert(approx(result.labor_participation_rate, 0.65f) &&
           "Zero indices should give base participation rate of 0.65");

    // working_age = round(1000 * 0.5) = 500
    // labor_force = round(500 * 0.65) = 325
    assert(result.labor_force == 325 && "Low modifier labor force should be 325");

    std::printf("  PASS: Low harmony/education results in base participation\n");
}

// --------------------------------------------------------------------------
// Test: Participation rate clamped to [0, 1]
// --------------------------------------------------------------------------
static void test_participation_clamp() {
    // Even with hypothetically extreme values, rate should not exceed 1.0
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.adult_percent = 50;
    pop.harmony_index = 100;
    pop.education_index = 100;

    EmploymentData emp{};

    LaborPoolResult result = calculate_labor_pool(pop, emp);

    // Max participation = 0.65 + 0.10 + 0.10 = 0.85, well under 1.0
    assert(result.labor_participation_rate <= 1.0f &&
           "Participation rate should never exceed 1.0");
    assert(result.labor_participation_rate >= 0.0f &&
           "Participation rate should never be negative");

    std::printf("  PASS: Participation rate clamped to [0, 1]\n");
}

// --------------------------------------------------------------------------
// Test: No adults means zero working age (but population exists)
// --------------------------------------------------------------------------
static void test_no_adults() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.adult_percent = 0;      // No working-age population
    pop.harmony_index = 50;
    pop.education_index = 50;

    EmploymentData emp{};

    LaborPoolResult result = calculate_labor_pool(pop, emp);

    assert(result.working_age_beings == 0 && "No adults should mean zero working age beings");
    assert(result.labor_force == 0 && "No adults should mean zero labor force");

    std::printf("  PASS: No adults means zero working age\n");
}

// --------------------------------------------------------------------------
// Test: Large population scaling
// --------------------------------------------------------------------------
static void test_large_population() {
    PopulationData pop{};
    pop.total_beings = 100000;
    pop.adult_percent = 60;
    pop.harmony_index = 50;
    pop.education_index = 50;

    EmploymentData emp{};

    LaborPoolResult result = calculate_labor_pool(pop, emp);

    // working_age = round(100000 * 60 / 100) = 60000
    assert(result.working_age_beings == 60000 && "Large pop working age should be 60000");

    // participation = 0.75
    // labor_force = round(60000 * 0.75) = 45000
    assert(result.labor_force == 45000 && "Large pop labor force should be 45000");

    std::printf("  PASS: Large population scaling\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Labor Pool Calculation Tests (E10-019) ===\n");

    test_default_labor_participation();
    test_all_adults();
    test_high_harmony_education();
    test_zero_population();
    test_low_harmony_education();
    test_participation_clamp();
    test_no_adults();
    test_large_population();

    std::printf("All labor pool calculation tests passed.\n");
    return 0;
}
