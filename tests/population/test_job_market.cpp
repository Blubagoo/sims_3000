/**
 * @file test_job_market.cpp
 * @brief Tests for job market aggregation (Ticket E10-020)
 *
 * Validates:
 * - Simple aggregation works correctly
 * - Zero capacities -> zero jobs
 * - Large values work correctly
 */

#include <cassert>
#include <cstdio>

#include "sims3000/population/JobMarketAggregation.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Test: Simple aggregation works
// --------------------------------------------------------------------------
static void test_simple_aggregation() {
    JobMarketResult result = aggregate_job_market(500, 300);

    assert(result.exchange_jobs == 500 && "Exchange jobs should equal exchange capacity");
    assert(result.fabrication_jobs == 300 && "Fabrication jobs should equal fabrication capacity");
    assert(result.total_jobs == 800 && "Total jobs should be sum of sectors");

    std::printf("  PASS: Simple aggregation\n");
}

// --------------------------------------------------------------------------
// Test: Zero capacities -> zero jobs
// --------------------------------------------------------------------------
static void test_zero_capacities() {
    JobMarketResult result = aggregate_job_market(0, 0);

    assert(result.exchange_jobs == 0 && "Zero exchange capacity -> zero exchange jobs");
    assert(result.fabrication_jobs == 0 && "Zero fabrication capacity -> zero fabrication jobs");
    assert(result.total_jobs == 0 && "Zero total capacity -> zero total jobs");

    std::printf("  PASS: Zero capacities -> zero jobs\n");
}

// --------------------------------------------------------------------------
// Test: Large values work correctly
// --------------------------------------------------------------------------
static void test_large_values() {
    uint32_t large_exchange = 1000000;
    uint32_t large_fabrication = 2000000;

    JobMarketResult result = aggregate_job_market(large_exchange, large_fabrication);

    assert(result.exchange_jobs == 1000000 && "Large exchange capacity should pass through");
    assert(result.fabrication_jobs == 2000000 && "Large fabrication capacity should pass through");
    assert(result.total_jobs == 3000000 && "Total should be sum of large values");

    std::printf("  PASS: Large values work correctly\n");
}

// --------------------------------------------------------------------------
// Test: Only exchange sector has jobs
// --------------------------------------------------------------------------
static void test_exchange_only() {
    JobMarketResult result = aggregate_job_market(1000, 0);

    assert(result.exchange_jobs == 1000 && "Exchange jobs should be 1000");
    assert(result.fabrication_jobs == 0 && "Fabrication jobs should be 0");
    assert(result.total_jobs == 1000 && "Total should equal exchange only");

    std::printf("  PASS: Exchange-only jobs\n");
}

// --------------------------------------------------------------------------
// Test: Only fabrication sector has jobs
// --------------------------------------------------------------------------
static void test_fabrication_only() {
    JobMarketResult result = aggregate_job_market(0, 750);

    assert(result.exchange_jobs == 0 && "Exchange jobs should be 0");
    assert(result.fabrication_jobs == 750 && "Fabrication jobs should be 750");
    assert(result.total_jobs == 750 && "Total should equal fabrication only");

    std::printf("  PASS: Fabrication-only jobs\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Job Market Aggregation Tests (E10-020) ===\n");

    test_simple_aggregation();
    test_zero_capacities();
    test_large_values();
    test_exchange_only();
    test_fabrication_only();

    std::printf("All job market aggregation tests passed.\n");
    return 0;
}
