/**
 * @file test_demand_stats.cpp
 * @brief Tests for demand statistics interface (Ticket E10-048)
 *
 * Validates:
 * - get_demand_stat() returns correct values for all stat IDs
 * - get_demand_stat_name() returns correct names
 * - is_valid_demand_stat() validates stat IDs correctly
 * - Invalid stat IDs return 0.0f and "Unknown"
 */

#include <cassert>
#include <cstdio>
#include <cstring>

#include "sims3000/demand/DemandStats.h"

using namespace sims3000::demand;

// --------------------------------------------------------------------------
// Test: get_demand_stat() returns correct demand values
// --------------------------------------------------------------------------
static void test_get_demand_values() {
    DemandData data{};
    data.habitation_demand = 50;
    data.exchange_demand = -30;
    data.fabrication_demand = 75;
    data.habitation_cap = 1000;
    data.exchange_cap = 500;
    data.fabrication_cap = 750;

    // Test demand values
    assert(get_demand_stat(data, STAT_HABITATION_DEMAND) == 50.0f);
    assert(get_demand_stat(data, STAT_EXCHANGE_DEMAND) == -30.0f);
    assert(get_demand_stat(data, STAT_FABRICATION_DEMAND) == 75.0f);

    // Test cap values
    assert(get_demand_stat(data, STAT_HABITATION_CAP) == 1000.0f);
    assert(get_demand_stat(data, STAT_EXCHANGE_CAP) == 500.0f);
    assert(get_demand_stat(data, STAT_FABRICATION_CAP) == 750.0f);

    std::printf("  PASS: get_demand_stat() returns correct values\n");
}

// --------------------------------------------------------------------------
// Test: get_demand_stat() with zero/negative values
// --------------------------------------------------------------------------
static void test_zero_and_negative_demands() {
    DemandData data{};
    data.habitation_demand = 0;
    data.exchange_demand = -100;  // Minimum demand
    data.fabrication_demand = 100; // Maximum demand

    assert(get_demand_stat(data, STAT_HABITATION_DEMAND) == 0.0f);
    assert(get_demand_stat(data, STAT_EXCHANGE_DEMAND) == -100.0f);
    assert(get_demand_stat(data, STAT_FABRICATION_DEMAND) == 100.0f);

    std::printf("  PASS: Zero and negative demand values work correctly\n");
}

// --------------------------------------------------------------------------
// Test: get_demand_stat() with invalid stat ID
// --------------------------------------------------------------------------
static void test_invalid_stat_id() {
    DemandData data{};
    data.habitation_demand = 50;

    // Invalid stat IDs should return 0.0f
    assert(get_demand_stat(data, 0) == 0.0f);
    assert(get_demand_stat(data, 999) == 0.0f);
    assert(get_demand_stat(data, 299) == 0.0f);  // One below valid range
    assert(get_demand_stat(data, 306) == 0.0f);  // One above valid range

    std::printf("  PASS: Invalid stat IDs return 0.0f\n");
}

// --------------------------------------------------------------------------
// Test: get_demand_stat_name() returns correct names
// --------------------------------------------------------------------------
static void test_stat_names() {
    assert(std::strcmp(get_demand_stat_name(STAT_HABITATION_DEMAND), "Habitation Demand") == 0);
    assert(std::strcmp(get_demand_stat_name(STAT_EXCHANGE_DEMAND), "Exchange Demand") == 0);
    assert(std::strcmp(get_demand_stat_name(STAT_FABRICATION_DEMAND), "Fabrication Demand") == 0);
    assert(std::strcmp(get_demand_stat_name(STAT_HABITATION_CAP), "Habitation Cap") == 0);
    assert(std::strcmp(get_demand_stat_name(STAT_EXCHANGE_CAP), "Exchange Cap") == 0);
    assert(std::strcmp(get_demand_stat_name(STAT_FABRICATION_CAP), "Fabrication Cap") == 0);

    std::printf("  PASS: Stat names are correct\n");
}

// --------------------------------------------------------------------------
// Test: get_demand_stat_name() with invalid ID
// --------------------------------------------------------------------------
static void test_invalid_stat_name() {
    assert(std::strcmp(get_demand_stat_name(0), "Unknown") == 0);
    assert(std::strcmp(get_demand_stat_name(999), "Unknown") == 0);

    std::printf("  PASS: Invalid stat IDs return 'Unknown'\n");
}

// --------------------------------------------------------------------------
// Test: is_valid_demand_stat() validates correctly
// --------------------------------------------------------------------------
static void test_is_valid_stat() {
    // Valid range: 300-305
    assert(is_valid_demand_stat(STAT_HABITATION_DEMAND) == true);
    assert(is_valid_demand_stat(STAT_EXCHANGE_DEMAND) == true);
    assert(is_valid_demand_stat(STAT_FABRICATION_DEMAND) == true);
    assert(is_valid_demand_stat(STAT_HABITATION_CAP) == true);
    assert(is_valid_demand_stat(STAT_EXCHANGE_CAP) == true);
    assert(is_valid_demand_stat(STAT_FABRICATION_CAP) == true);

    // Invalid IDs
    assert(is_valid_demand_stat(0) == false);
    assert(is_valid_demand_stat(299) == false);
    assert(is_valid_demand_stat(306) == false);
    assert(is_valid_demand_stat(999) == false);

    std::printf("  PASS: is_valid_demand_stat() validates correctly\n");
}

// --------------------------------------------------------------------------
// Test: All stat ID constants are unique
// --------------------------------------------------------------------------
static void test_stat_id_uniqueness() {
    // Ensure no duplicate stat IDs
    uint16_t ids[] = {
        STAT_HABITATION_DEMAND,
        STAT_EXCHANGE_DEMAND,
        STAT_FABRICATION_DEMAND,
        STAT_HABITATION_CAP,
        STAT_EXCHANGE_CAP,
        STAT_FABRICATION_CAP
    };

    constexpr size_t count = sizeof(ids) / sizeof(ids[0]);
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            assert(ids[i] != ids[j] && "Stat IDs must be unique");
        }
    }

    std::printf("  PASS: All stat IDs are unique\n");
}

// --------------------------------------------------------------------------
// Test: Stat ID range is sequential
// --------------------------------------------------------------------------
static void test_stat_id_range() {
    // Verify stat IDs are sequential 300-305
    assert(STAT_HABITATION_DEMAND == 300);
    assert(STAT_EXCHANGE_DEMAND == 301);
    assert(STAT_FABRICATION_DEMAND == 302);
    assert(STAT_HABITATION_CAP == 303);
    assert(STAT_EXCHANGE_CAP == 304);
    assert(STAT_FABRICATION_CAP == 305);

    std::printf("  PASS: Stat IDs are in expected sequential range\n");
}

// --------------------------------------------------------------------------
// Test: Edge case with maximum cap values
// --------------------------------------------------------------------------
static void test_large_cap_values() {
    DemandData data{};
    data.habitation_cap = 999999;
    data.exchange_cap = 888888;
    data.fabrication_cap = 777777;

    assert(get_demand_stat(data, STAT_HABITATION_CAP) == 999999.0f);
    assert(get_demand_stat(data, STAT_EXCHANGE_CAP) == 888888.0f);
    assert(get_demand_stat(data, STAT_FABRICATION_CAP) == 777777.0f);

    std::printf("  PASS: Large cap values work correctly\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Demand Statistics Tests (E10-048) ===\n");

    test_get_demand_values();
    test_zero_and_negative_demands();
    test_invalid_stat_id();
    test_stat_names();
    test_invalid_stat_name();
    test_is_valid_stat();
    test_stat_id_uniqueness();
    test_stat_id_range();
    test_large_cap_values();

    std::printf("All demand statistics tests passed.\n");
    return 0;
}
