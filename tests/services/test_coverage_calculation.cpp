/**
 * @file test_coverage_calculation.cpp
 * @brief Unit tests for radius-based coverage calculation (Epic 9, Ticket E9-020)
 *
 * Tests cover:
 * - Single building coverage pattern
 * - Linear falloff values
 * - Bounds clipping (no wraparound)
 * - Inactive building skip
 * - Max-value overlap from multiple buildings
 * - Edge cases: zero radius, out-of-bounds position, empty buildings
 */

#include <sims3000/services/CoverageCalculation.h>
#include <sims3000/services/ServiceCoverageGrid.h>
#include <sims3000/services/ServiceTypes.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace sims3000::services;

// =============================================================================
// calculate_falloff tests
// =============================================================================

void test_falloff_at_center() {
    printf("Testing falloff at distance 0 (center)...\n");

    float result = calculate_falloff(1.0f, 0, 8);
    assert(std::fabs(result - 1.0f) < 0.001f);

    printf("  PASS: Falloff at center is full effectiveness\n");
}

void test_falloff_at_edge() {
    printf("Testing falloff at edge of radius...\n");

    // At distance == radius, falloff should be 0
    float result = calculate_falloff(1.0f, 8, 8);
    assert(std::fabs(result - 0.0f) < 0.001f);

    printf("  PASS: Falloff at edge is 0\n");
}

void test_falloff_beyond_radius() {
    printf("Testing falloff beyond radius...\n");

    float result = calculate_falloff(1.0f, 10, 8);
    assert(result == 0.0f);

    printf("  PASS: Falloff beyond radius is 0\n");
}

void test_falloff_midpoint() {
    printf("Testing falloff at midpoint...\n");

    // At distance = radius/2, falloff = effectiveness * 0.5
    float result = calculate_falloff(1.0f, 4, 8);
    assert(std::fabs(result - 0.5f) < 0.001f);

    printf("  PASS: Falloff at midpoint is 0.5\n");
}

void test_falloff_with_partial_effectiveness() {
    printf("Testing falloff with partial effectiveness...\n");

    // effectiveness = 0.5, distance = 0 -> 0.5 * 1.0 = 0.5
    float result = calculate_falloff(0.5f, 0, 8);
    assert(std::fabs(result - 0.5f) < 0.001f);

    // effectiveness = 0.5, distance = 4, radius = 8 -> 0.5 * 0.5 = 0.25
    result = calculate_falloff(0.5f, 4, 8);
    assert(std::fabs(result - 0.25f) < 0.001f);

    printf("  PASS: Partial effectiveness scales correctly\n");
}

void test_falloff_zero_radius() {
    printf("Testing falloff with zero radius...\n");

    float result = calculate_falloff(1.0f, 0, 0);
    assert(result == 0.0f);

    printf("  PASS: Zero radius returns 0\n");
}

void test_falloff_negative_distance() {
    printf("Testing falloff with negative distance (absolute value used)...\n");

    float result = calculate_falloff(1.0f, -4, 8);
    assert(std::fabs(result - 0.5f) < 0.001f);

    printf("  PASS: Negative distance treated as positive\n");
}

// =============================================================================
// Single building coverage tests
// =============================================================================

void test_single_building_coverage() {
    printf("Testing single building coverage pattern...\n");

    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;

    // Place an Enforcer Post (radius=8) at center of map
    ServiceBuildingData b;
    b.x = 16;
    b.y = 16;
    b.type = ServiceType::Enforcer;
    b.tier = static_cast<uint8_t>(ServiceTier::Post);
    b.effectiveness = 255;  // Full effectiveness
    b.is_active = true;
    b.owner_id = 0;
    buildings.push_back(b);

    calculate_radius_coverage(grid, buildings);

    // Center tile should have maximum coverage (distance=0)
    uint8_t center = grid.get_coverage_at(16, 16);
    assert(center == 255);

    // Tile at manhattan distance 4 from center: 1.0 * (1 - 4/8) = 0.5 -> 128
    uint8_t mid = grid.get_coverage_at(20, 16);
    assert(mid == 128);

    // Tile at manhattan distance 7: 1.0 * (1 - 7/8) = 0.125 -> 32
    uint8_t near_edge = grid.get_coverage_at(23, 16);
    assert(near_edge == 32);

    // Tile at manhattan distance 8 (edge): should be 0
    uint8_t edge = grid.get_coverage_at(24, 16);
    assert(edge == 0);

    // Tile well beyond radius
    uint8_t far = grid.get_coverage_at(30, 16);
    assert(far == 0);

    printf("  PASS: Single building coverage pattern correct\n");
}

void test_building_at_origin() {
    printf("Testing building at map origin (0,0)...\n");

    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;

    ServiceBuildingData b;
    b.x = 0;
    b.y = 0;
    b.type = ServiceType::Enforcer;
    b.tier = static_cast<uint8_t>(ServiceTier::Post);
    b.effectiveness = 255;
    b.is_active = true;
    b.owner_id = 0;
    buildings.push_back(b);

    calculate_radius_coverage(grid, buildings);

    // Origin should have full coverage
    assert(grid.get_coverage_at(0, 0) == 255);

    // Should clip at map bounds (no negative index / no wraparound)
    // Tile at (7, 0) should have coverage (distance=7, 1-7/8 = 0.125 -> 32)
    assert(grid.get_coverage_at(7, 0) == 32);

    printf("  PASS: Building at origin with bounds clipping correct\n");
}

void test_building_at_corner() {
    printf("Testing building at map corner...\n");

    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;

    ServiceBuildingData b;
    b.x = 31;
    b.y = 31;
    b.type = ServiceType::Enforcer;
    b.tier = static_cast<uint8_t>(ServiceTier::Post);
    b.effectiveness = 255;
    b.is_active = true;
    b.owner_id = 0;
    buildings.push_back(b);

    calculate_radius_coverage(grid, buildings);

    // Corner should have full coverage
    assert(grid.get_coverage_at(31, 31) == 255);

    // No wraparound - tiles on opposite side should be 0
    assert(grid.get_coverage_at(0, 0) == 0);
    assert(grid.get_coverage_at(0, 31) == 0);

    printf("  PASS: Building at corner with no wraparound correct\n");
}

// =============================================================================
// Inactive building tests
// =============================================================================

void test_inactive_building_skipped() {
    printf("Testing inactive building is skipped...\n");

    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;

    ServiceBuildingData b;
    b.x = 16;
    b.y = 16;
    b.type = ServiceType::Enforcer;
    b.tier = static_cast<uint8_t>(ServiceTier::Post);
    b.effectiveness = 255;
    b.is_active = false;  // Inactive!
    b.owner_id = 0;
    buildings.push_back(b);

    calculate_radius_coverage(grid, buildings);

    // All tiles should be 0
    for (uint32_t y = 0; y < 32; ++y) {
        for (uint32_t x = 0; x < 32; ++x) {
            assert(grid.get_coverage_at(x, y) == 0);
        }
    }

    printf("  PASS: Inactive building produces no coverage\n");
}

// =============================================================================
// Max-value overlap tests
// =============================================================================

void test_max_value_overlap() {
    printf("Testing max-value overlap from multiple buildings...\n");

    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;

    // Building 1: at (10, 16) with full effectiveness
    ServiceBuildingData b1;
    b1.x = 10;
    b1.y = 16;
    b1.type = ServiceType::Enforcer;
    b1.tier = static_cast<uint8_t>(ServiceTier::Post);
    b1.effectiveness = 255;
    b1.is_active = true;
    b1.owner_id = 0;
    buildings.push_back(b1);

    // Building 2: at (14, 16) with full effectiveness
    ServiceBuildingData b2;
    b2.x = 14;
    b2.y = 16;
    b2.type = ServiceType::Enforcer;
    b2.tier = static_cast<uint8_t>(ServiceTier::Post);
    b2.effectiveness = 255;
    b2.is_active = true;
    b2.owner_id = 0;
    buildings.push_back(b2);

    calculate_radius_coverage(grid, buildings);

    // Tile at (12, 16):
    //   From b1: distance=2, 1-2/8=0.75 -> 191
    //   From b2: distance=2, 1-2/8=0.75 -> 191
    //   Max overlap -> 191
    uint8_t overlap_tile = grid.get_coverage_at(12, 16);
    assert(overlap_tile == 191);

    // Tile at (10, 16):
    //   From b1: distance=0 -> 255
    //   From b2: distance=4, 1-4/8=0.5 -> 128
    //   Max overlap -> 255
    uint8_t b1_center = grid.get_coverage_at(10, 16);
    assert(b1_center == 255);

    printf("  PASS: Max-value overlap works correctly\n");
}

// =============================================================================
// Empty buildings vector
// =============================================================================

void test_empty_buildings() {
    printf("Testing empty buildings vector clears grid...\n");

    ServiceCoverageGrid grid(16, 16);

    // Pre-populate grid with some values
    grid.set_coverage_at(5, 5, 100);
    grid.set_coverage_at(10, 10, 200);

    std::vector<ServiceBuildingData> buildings;
    calculate_radius_coverage(grid, buildings);

    // Grid should be cleared
    assert(grid.get_coverage_at(5, 5) == 0);
    assert(grid.get_coverage_at(10, 10) == 0);

    printf("  PASS: Empty buildings vector clears grid\n");
}

// =============================================================================
// Partial effectiveness
// =============================================================================

void test_partial_effectiveness() {
    printf("Testing partial effectiveness building...\n");

    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;

    ServiceBuildingData b;
    b.x = 16;
    b.y = 16;
    b.type = ServiceType::Enforcer;
    b.tier = static_cast<uint8_t>(ServiceTier::Post);
    b.effectiveness = 128;  // ~50% effectiveness
    b.is_active = true;
    b.owner_id = 0;
    buildings.push_back(b);

    calculate_radius_coverage(grid, buildings);

    // Center: effectiveness = 128/255 = ~0.502, falloff = 1.0
    // strength = 0.502 * 1.0 = 0.502 -> uint8 = round(0.502 * 255) = 128
    uint8_t center = grid.get_coverage_at(16, 16);
    assert(center == 128);

    printf("  PASS: Partial effectiveness scales correctly\n");
}

// =============================================================================
// Different tiers
// =============================================================================

void test_different_tiers() {
    printf("Testing different building tiers have different radii...\n");

    // Test with Station tier (radius=12)
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;

    ServiceBuildingData b;
    b.x = 16;
    b.y = 16;
    b.type = ServiceType::Enforcer;
    b.tier = static_cast<uint8_t>(ServiceTier::Station);  // radius=12
    b.effectiveness = 255;
    b.is_active = true;
    b.owner_id = 0;
    buildings.push_back(b);

    calculate_radius_coverage(grid, buildings);

    // Tile at distance 10 should have coverage (within radius 12)
    // 1 - 10/12 = 0.1667 -> round(0.1667 * 255) = 43
    uint8_t at_10 = grid.get_coverage_at(26, 16);
    assert(at_10 == 43);

    // Tile at distance 12 should be 0
    uint8_t at_12 = grid.get_coverage_at(28, 16);
    assert(at_12 == 0);

    printf("  PASS: Different tiers produce different radii\n");
}

// =============================================================================
// Manhattan distance verification
// =============================================================================

void test_manhattan_distance_pattern() {
    printf("Testing manhattan distance produces diamond pattern...\n");

    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;

    ServiceBuildingData b;
    b.x = 16;
    b.y = 16;
    b.type = ServiceType::Enforcer;
    b.tier = static_cast<uint8_t>(ServiceTier::Post);  // radius=8
    b.effectiveness = 255;
    b.is_active = true;
    b.owner_id = 0;
    buildings.push_back(b);

    calculate_radius_coverage(grid, buildings);

    // All tiles at same manhattan distance should have same coverage
    // Distance 4 from (16,16): (20,16), (16,20), (12,16), (16,12), (18,18), etc.
    uint8_t expected_d4 = grid.get_coverage_at(20, 16);
    assert(grid.get_coverage_at(16, 20) == expected_d4);
    assert(grid.get_coverage_at(12, 16) == expected_d4);
    assert(grid.get_coverage_at(16, 12) == expected_d4);
    assert(grid.get_coverage_at(18, 18) == expected_d4);  // dx=2, dy=2 -> d=4
    assert(grid.get_coverage_at(14, 18) == expected_d4);  // dx=-2, dy=2 -> d=4

    printf("  PASS: Manhattan distance produces correct diamond pattern\n");
}

// =============================================================================
// Grid clearing before calculation
// =============================================================================

void test_grid_cleared_before_calculation() {
    printf("Testing grid is cleared before calculation...\n");

    ServiceCoverageGrid grid(32, 32);

    // Pre-populate with data that should be overwritten
    for (uint32_t y = 0; y < 32; ++y) {
        for (uint32_t x = 0; x < 32; ++x) {
            grid.set_coverage_at(x, y, 200);
        }
    }

    std::vector<ServiceBuildingData> buildings;

    // Place building at one corner
    ServiceBuildingData b;
    b.x = 0;
    b.y = 0;
    b.type = ServiceType::Enforcer;
    b.tier = static_cast<uint8_t>(ServiceTier::Post);
    b.effectiveness = 255;
    b.is_active = true;
    b.owner_id = 0;
    buildings.push_back(b);

    calculate_radius_coverage(grid, buildings);

    // Tile far from building should be 0 (was 200 before calculation)
    assert(grid.get_coverage_at(31, 31) == 0);

    printf("  PASS: Grid is cleared before calculation\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Coverage Calculation Unit Tests (Epic 9, Ticket E9-020) ===\n\n");

    // Falloff tests
    test_falloff_at_center();
    test_falloff_at_edge();
    test_falloff_beyond_radius();
    test_falloff_midpoint();
    test_falloff_with_partial_effectiveness();
    test_falloff_zero_radius();
    test_falloff_negative_distance();

    // Coverage calculation tests
    test_single_building_coverage();
    test_building_at_origin();
    test_building_at_corner();
    test_inactive_building_skipped();
    test_max_value_overlap();
    test_empty_buildings();
    test_partial_effectiveness();
    test_different_tiers();
    test_manhattan_distance_pattern();
    test_grid_cleared_before_calculation();

    printf("\n=== All Coverage Calculation Tests Passed ===\n");
    return 0;
}
