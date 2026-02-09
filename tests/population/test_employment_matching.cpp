/**
 * @file test_employment_matching.cpp
 * @brief Tests for employment matching algorithm (Ticket E10-021)
 *
 * Validates:
 * - More labor than jobs: full employment of available jobs, unemployment > 0
 * - More jobs than labor: all labor employed, unemployment = 0
 * - Equal labor and jobs: full employment
 * - Proportional distribution between exchange/fabrication
 * - Zero jobs: all unemployed (100%)
 * - Zero labor: no one employed
 */

#include <cassert>
#include <cstdio>

#include "sims3000/population/EmploymentMatching.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Test: More labor than jobs -> partial employment, unemployment > 0
// --------------------------------------------------------------------------
static void test_more_labor_than_jobs() {
    EmploymentMatchingResult result = match_employment(1000, 300, 200);

    // total_jobs = 500, labor = 1000
    // max_employment = min(1000, 500) = 500
    assert(result.employed_laborers == 500 && "Should employ 500 of 1000 laborers");
    assert(result.unemployed == 500 && "Should have 500 unemployed");
    assert(result.unemployment_rate == 50 && "Unemployment rate should be 50%");

    // Proportional: exchange = 500 * 300/500 = 300, fabrication = 200
    assert(result.exchange_employed == 300 && "Exchange employed should be 300");
    assert(result.fabrication_employed == 200 && "Fabrication employed should be 200");

    std::printf("  PASS: More labor than jobs -> partial employment\n");
}

// --------------------------------------------------------------------------
// Test: More jobs than labor -> all labor employed, unemployment = 0
// --------------------------------------------------------------------------
static void test_more_jobs_than_labor() {
    EmploymentMatchingResult result = match_employment(200, 500, 300);

    // total_jobs = 800, labor = 200
    // max_employment = min(200, 800) = 200
    assert(result.employed_laborers == 200 && "Should employ all 200 laborers");
    assert(result.unemployed == 0 && "Should have 0 unemployed");
    assert(result.unemployment_rate == 0 && "Unemployment rate should be 0%");

    // Proportional: exchange = 200 * 500/800 = 125, fabrication = 75
    assert(result.exchange_employed == 125 && "Exchange employed should be 125");
    assert(result.fabrication_employed == 75 && "Fabrication employed should be 75");

    std::printf("  PASS: More jobs than labor -> full employment\n");
}

// --------------------------------------------------------------------------
// Test: Equal labor and jobs -> full employment
// --------------------------------------------------------------------------
static void test_equal_labor_and_jobs() {
    EmploymentMatchingResult result = match_employment(500, 300, 200);

    // total_jobs = 500, labor = 500
    // max_employment = 500
    assert(result.employed_laborers == 500 && "Should employ all 500 laborers");
    assert(result.unemployed == 0 && "Should have 0 unemployed");
    assert(result.unemployment_rate == 0 && "Unemployment rate should be 0%");

    // Proportional: exchange = 500 * 300/500 = 300, fabrication = 200
    assert(result.exchange_employed == 300 && "Exchange employed should be 300");
    assert(result.fabrication_employed == 200 && "Fabrication employed should be 200");

    std::printf("  PASS: Equal labor and jobs -> full employment\n");
}

// --------------------------------------------------------------------------
// Test: Proportional distribution between exchange/fabrication
// --------------------------------------------------------------------------
static void test_proportional_distribution() {
    // 80% exchange, 20% fabrication
    EmploymentMatchingResult result = match_employment(100, 800, 200);

    // total_jobs = 1000, labor = 100
    // max_employment = 100
    // exchange = 100 * 800/1000 = 80
    // fabrication = 100 - 80 = 20
    assert(result.exchange_employed == 80 && "Exchange should get 80% of employment");
    assert(result.fabrication_employed == 20 && "Fabrication should get 20% of employment");
    assert(result.employed_laborers == 100 && "All laborers should be employed");

    std::printf("  PASS: Proportional distribution between sectors\n");
}

// --------------------------------------------------------------------------
// Test: Zero jobs -> all unemployed (100%)
// --------------------------------------------------------------------------
static void test_zero_jobs() {
    EmploymentMatchingResult result = match_employment(500, 0, 0);

    assert(result.employed_laborers == 0 && "No one should be employed with 0 jobs");
    assert(result.unemployed == 500 && "All 500 should be unemployed");
    assert(result.exchange_employed == 0 && "No exchange employment");
    assert(result.fabrication_employed == 0 && "No fabrication employment");
    assert(result.unemployment_rate == 100 && "Unemployment rate should be 100%");

    std::printf("  PASS: Zero jobs -> all unemployed (100%%)\n");
}

// --------------------------------------------------------------------------
// Test: Zero labor -> no one employed
// --------------------------------------------------------------------------
static void test_zero_labor() {
    EmploymentMatchingResult result = match_employment(0, 100, 200);

    assert(result.employed_laborers == 0 && "No one to employ with 0 labor");
    assert(result.unemployed == 0 && "No one unemployed either");
    assert(result.exchange_employed == 0 && "No exchange employment");
    assert(result.fabrication_employed == 0 && "No fabrication employment");
    assert(result.unemployment_rate == 0 && "Unemployment rate should be 0% with no labor");

    std::printf("  PASS: Zero labor -> no one employed\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Employment Matching Tests (E10-021) ===\n");

    test_more_labor_than_jobs();
    test_more_jobs_than_labor();
    test_equal_labor_and_jobs();
    test_proportional_distribution();
    test_zero_jobs();
    test_zero_labor();

    std::printf("All employment matching tests passed.\n");
    return 0;
}
