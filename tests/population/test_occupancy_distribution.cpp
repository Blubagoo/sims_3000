/**
 * @file test_occupancy_distribution.cpp
 * @brief Tests for occupancy distribution (Ticket E10-022)
 *
 * Validates:
 * - Only habitation buildings receive occupancy
 * - Proportional distribution by capacity
 * - Correct OccupancyState classification
 * - Full capacity scenario
 * - Empty buildings scenario
 */

#include <cassert>
#include <cstdio>
#include <vector>

#include "sims3000/population/OccupancyDistribution.h"
#include "sims3000/population/BuildingOccupancyComponent.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Test: Only habitation buildings receive occupancy
// --------------------------------------------------------------------------
static void test_habitation_only() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 100, 0},  // Habitation
        {2, 200, 1},  // Exchange (should be filtered out)
        {3, 150, 0},  // Habitation
        {4, 300, 2},  // Fabrication (should be filtered out)
    };

    auto results = distribute_occupancy(250, buildings);

    // Should only have 2 results (habitation buildings)
    assert(results.size() == 2 && "Only habitation buildings should receive occupancy");

    // Check that results are for buildings 1 and 3
    bool found_1 = false, found_3 = false;
    for (const auto& result : results) {
        if (result.building_id == 1) found_1 = true;
        if (result.building_id == 3) found_3 = true;
        assert((result.building_id == 1 || result.building_id == 3) &&
               "Only habitation buildings should be in results");
    }
    assert(found_1 && found_3 && "Both habitation buildings should be in results");

    std::printf("  PASS: Only habitation buildings receive occupancy\n");
}

// --------------------------------------------------------------------------
// Test: Proportional distribution by capacity
// --------------------------------------------------------------------------
static void test_proportional_distribution() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 100, 0},  // 40% of capacity
        {2, 150, 0},  // 60% of capacity
    };
    // Total capacity = 250

    uint32_t total_beings = 100;
    auto results = distribute_occupancy(total_beings, buildings);

    assert(results.size() == 2 && "Should have 2 results");

    // Find each building's result
    uint32_t occ_1 = 0, occ_2 = 0;
    for (const auto& result : results) {
        if (result.building_id == 1) occ_1 = result.occupancy;
        if (result.building_id == 2) occ_2 = result.occupancy;
    }

    // Should be roughly proportional: 40 and 60, allowing for rounding
    assert(occ_1 + occ_2 == total_beings && "Total occupancy should equal total beings");
    assert(occ_1 >= 35 && occ_1 <= 45 && "Building 1 should get ~40%");
    assert(occ_2 >= 55 && occ_2 <= 65 && "Building 2 should get ~60%");

    std::printf("  PASS: Proportional distribution by capacity\n");
}

// --------------------------------------------------------------------------
// Test: OccupancyState classification
// --------------------------------------------------------------------------
static void test_occupancy_state_classification() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 100, 0},
        {2, 100, 0},
        {3, 100, 0},
        {4, 100, 0},
        {5, 100, 0},
    };
    // Total capacity = 500

    // Distribute different amounts to test state thresholds
    auto results1 = distribute_occupancy(0, buildings);     // Empty (0%)
    auto results2 = distribute_occupancy(100, buildings);   // UnderOccupied (<50%)
    auto results3 = distribute_occupancy(350, buildings);   // NormalOccupied (50-90%)
    auto results4 = distribute_occupancy(480, buildings);   // FullyOccupied (>90%)
    auto results5 = distribute_occupancy(600, buildings);   // FullyOccupied (at capacity, >= total)

    // Check Empty state
    for (const auto& result : results1) {
        assert(result.state == static_cast<uint8_t>(OccupancyState::Empty) &&
               "Zero occupancy should be Empty");
    }

    // Check UnderOccupied state (20% occupancy)
    for (const auto& result : results2) {
        assert(result.state == static_cast<uint8_t>(OccupancyState::UnderOccupied) &&
               "Low occupancy should be UnderOccupied");
    }

    // Check NormalOccupied state (70% occupancy)
    for (const auto& result : results3) {
        assert(result.state == static_cast<uint8_t>(OccupancyState::NormalOccupied) &&
               "Medium occupancy should be NormalOccupied");
    }

    // Check FullyOccupied state (96% occupancy)
    for (const auto& result : results4) {
        assert(result.state == static_cast<uint8_t>(OccupancyState::FullyOccupied) &&
               "High occupancy should be FullyOccupied");
    }

    // Check at-capacity state (100%)
    for (const auto& result : results5) {
        assert(result.state == static_cast<uint8_t>(OccupancyState::FullyOccupied) &&
               "At capacity should be FullyOccupied");
        assert(result.occupancy == 100 && "Should be filled to capacity");
    }

    std::printf("  PASS: OccupancyState classification\n");
}

// --------------------------------------------------------------------------
// Test: Full capacity scenario (beings >= capacity)
// --------------------------------------------------------------------------
static void test_full_capacity() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 100, 0},
        {2, 200, 0},
        {3, 150, 0},
    };
    // Total capacity = 450

    uint32_t total_beings = 500;  // Exceeds capacity
    auto results = distribute_occupancy(total_beings, buildings);

    assert(results.size() == 3 && "Should have 3 results");

    // All buildings should be at capacity
    for (const auto& result : results) {
        if (result.building_id == 1) {
            assert(result.occupancy == 100 && "Building 1 should be at capacity");
        } else if (result.building_id == 2) {
            assert(result.occupancy == 200 && "Building 2 should be at capacity");
        } else if (result.building_id == 3) {
            assert(result.occupancy == 150 && "Building 3 should be at capacity");
        }
        assert(result.state == static_cast<uint8_t>(OccupancyState::FullyOccupied) &&
               "All buildings should be FullyOccupied");
    }

    std::printf("  PASS: Full capacity scenario\n");
}

// --------------------------------------------------------------------------
// Test: Empty buildings (zero population)
// --------------------------------------------------------------------------
static void test_zero_population() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 100, 0},
        {2, 200, 0},
    };

    auto results = distribute_occupancy(0, buildings);

    assert(results.size() == 2 && "Should have 2 results");

    for (const auto& result : results) {
        assert(result.occupancy == 0 && "Occupancy should be zero");
        assert(result.state == static_cast<uint8_t>(OccupancyState::Empty) &&
               "State should be Empty");
    }

    std::printf("  PASS: Zero population\n");
}

// --------------------------------------------------------------------------
// Test: No habitation buildings
// --------------------------------------------------------------------------
static void test_no_habitation_buildings() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 100, 1},  // Exchange
        {2, 200, 2},  // Fabrication
    };

    auto results = distribute_occupancy(100, buildings);

    assert(results.empty() && "Should have no results with no habitation buildings");

    std::printf("  PASS: No habitation buildings\n");
}

// --------------------------------------------------------------------------
// Test: Zero capacity buildings
// --------------------------------------------------------------------------
static void test_zero_capacity() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 0, 0},
        {2, 0, 0},
    };

    auto results = distribute_occupancy(100, buildings);

    assert(results.size() == 2 && "Should have results for zero-capacity buildings");

    for (const auto& result : results) {
        assert(result.occupancy == 0 && "Zero-capacity buildings should have zero occupancy");
        assert(result.state == static_cast<uint8_t>(OccupancyState::Empty) &&
               "Zero-capacity buildings should be Empty");
    }

    std::printf("  PASS: Zero capacity buildings\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Occupancy Distribution Tests (E10-022) ===\n");

    test_habitation_only();
    test_proportional_distribution();
    test_occupancy_state_classification();
    test_full_capacity();
    test_zero_population();
    test_no_habitation_buildings();
    test_zero_capacity();

    std::printf("All occupancy distribution tests passed.\n");
    return 0;
}
