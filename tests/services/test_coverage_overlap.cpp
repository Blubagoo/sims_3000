/**
 * @file test_coverage_overlap.cpp
 * @brief Exhaustive unit tests for coverage overlap handling (Epic 9, Ticket E9-022)
 *
 * Validates that calculate_radius_coverage() uses max-value semantics
 * when multiple buildings' coverage areas overlap.
 *
 * Key invariant: grid[x,y] = max(building_i coverage) for all buildings i
 * No stacking/accumulation -- prevents "pile enforcers" exploit.
 *
 * Tests cover:
 * - Two buildings with overlapping coverage: max-value at overlap point
 * - Three buildings with overlapping coverage: max-value
 * - Higher effectiveness replaces lower
 * - Lower effectiveness does NOT replace higher
 * - Non-overlapping buildings: each tile has only its building's value
 * - Full overlap (same position): stronger building wins
 * - Edge overlap: only boundary tiles overlap
 */

#include <sims3000/services/CoverageCalculation.h>
#include <sims3000/services/ServiceCoverageGrid.h>
#include <sims3000/services/ServiceTypes.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace sims3000::services;

// Helper: compute expected uint8_t coverage value for a building at a given tile.
// Uses the same math as CoverageCalculation.cpp.
static uint8_t expected_coverage(uint8_t effectiveness_u8, int distance, int radius) {
    if (radius <= 0 || distance >= radius || distance < 0) {
        return 0;
    }
    float eff = static_cast<float>(effectiveness_u8) / 255.0f;
    float falloff = 1.0f - static_cast<float>(distance) / static_cast<float>(radius);
    float strength = eff * falloff;
    return static_cast<uint8_t>(std::min(255.0f, strength * 255.0f + 0.5f));
}

static int manhattan(int x1, int y1, int x2, int y2) {
    return std::abs(x1 - x2) + std::abs(y1 - y2);
}

static ServiceBuildingData make_enforcer_post(int x, int y, uint8_t effectiveness) {
    ServiceBuildingData b;
    b.x = x;
    b.y = y;
    b.type = ServiceType::Enforcer;
    b.tier = static_cast<uint8_t>(ServiceTier::Post);  // radius = 8
    b.effectiveness = effectiveness;
    b.is_active = true;
    b.owner_id = 0;
    b.capacity = 0;
    return b;
}

static ServiceBuildingData make_enforcer_station(int x, int y, uint8_t effectiveness) {
    ServiceBuildingData b;
    b.x = x;
    b.y = y;
    b.type = ServiceType::Enforcer;
    b.tier = static_cast<uint8_t>(ServiceTier::Station);  // radius = 12
    b.effectiveness = effectiveness;
    b.is_active = true;
    b.owner_id = 0;
    b.capacity = 0;
    return b;
}

// =============================================================================
// Two buildings with overlapping coverage: verify max-value at overlap point
// =============================================================================

void test_two_buildings_overlap() {
    printf("Testing two buildings with overlapping coverage...\n");

    // Enforcer Post: radius = 8
    // Place buildings at (10, 16) and (14, 16) -- 4 tiles apart
    // Overlap region is between them
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_enforcer_post(10, 16, 255));
    buildings.push_back(make_enforcer_post(14, 16, 255));

    calculate_radius_coverage(grid, buildings);

    // Check tile at (12, 16): equidistant from both (distance=2)
    int d1 = manhattan(12, 16, 10, 16);  // 2
    int d2 = manhattan(12, 16, 14, 16);  // 2
    uint8_t exp1 = expected_coverage(255, d1, 8);
    uint8_t exp2 = expected_coverage(255, d2, 8);
    uint8_t expected_val = (exp1 > exp2) ? exp1 : exp2;

    uint8_t actual = grid.get_coverage_at(12, 16);
    assert(actual == expected_val);

    // Check tile at (11, 16): closer to building 1
    d1 = manhattan(11, 16, 10, 16);  // 1
    d2 = manhattan(11, 16, 14, 16);  // 3
    exp1 = expected_coverage(255, d1, 8);  // higher
    exp2 = expected_coverage(255, d2, 8);  // lower
    expected_val = (exp1 > exp2) ? exp1 : exp2;

    actual = grid.get_coverage_at(11, 16);
    assert(actual == expected_val);
    assert(actual == exp1);  // Building 1 is closer, so it provides max

    printf("  PASS: Two building overlap uses max-value correctly\n");
}

// =============================================================================
// Three buildings with overlapping coverage: verify max-value
// =============================================================================

void test_three_buildings_overlap() {
    printf("Testing three buildings with overlapping coverage...\n");

    // Place three buildings in a triangle: (16,10), (12,18), (20,18)
    // All Enforcer Post (radius=8)
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_enforcer_post(16, 10, 255));
    buildings.push_back(make_enforcer_post(12, 18, 255));
    buildings.push_back(make_enforcer_post(20, 18, 255));

    calculate_radius_coverage(grid, buildings);

    // Check a central-ish tile (16, 14) which is within range of building 1
    int d1 = manhattan(16, 14, 16, 10);  // 4
    int d2 = manhattan(16, 14, 12, 18);  // 8
    int d3 = manhattan(16, 14, 20, 18);  // 8

    uint8_t exp1 = expected_coverage(255, d1, 8);
    uint8_t exp2 = expected_coverage(255, d2, 8);  // at edge -> 0
    uint8_t exp3 = expected_coverage(255, d3, 8);  // at edge -> 0

    uint8_t expected_val = exp1;
    if (exp2 > expected_val) expected_val = exp2;
    if (exp3 > expected_val) expected_val = exp3;

    uint8_t actual = grid.get_coverage_at(16, 14);
    assert(actual == expected_val);
    assert(actual == exp1);  // Building 1 is closest

    // Check tile (14, 16): within range of buildings 1 and 2
    d1 = manhattan(14, 16, 16, 10);  // 8 -> edge, 0
    d2 = manhattan(14, 16, 12, 18);  // 4
    d3 = manhattan(14, 16, 20, 18);  // 8 -> edge, 0

    exp1 = expected_coverage(255, d1, 8);
    exp2 = expected_coverage(255, d2, 8);
    exp3 = expected_coverage(255, d3, 8);

    expected_val = exp1;
    if (exp2 > expected_val) expected_val = exp2;
    if (exp3 > expected_val) expected_val = exp3;

    actual = grid.get_coverage_at(14, 16);
    assert(actual == expected_val);
    assert(actual == exp2);  // Building 2 is closest

    printf("  PASS: Three building overlap uses max-value correctly\n");
}

// =============================================================================
// Higher effectiveness replaces lower
// =============================================================================

void test_higher_replaces_lower() {
    printf("Testing higher effectiveness building replaces lower...\n");

    // Two buildings at same position, different effectiveness
    // Building 1: low effectiveness (100), Building 2: high effectiveness (255)
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_enforcer_post(16, 16, 100));  // weak
    buildings.push_back(make_enforcer_post(16, 16, 255));  // strong

    calculate_radius_coverage(grid, buildings);

    // Center should have the stronger building's value
    uint8_t center = grid.get_coverage_at(16, 16);
    uint8_t exp_strong = expected_coverage(255, 0, 8);
    assert(center == exp_strong);
    assert(center == 255);

    // At distance 4: strong building should dominate
    uint8_t at_4 = grid.get_coverage_at(20, 16);
    uint8_t exp_weak_4 = expected_coverage(100, 4, 8);
    uint8_t exp_strong_4 = expected_coverage(255, 4, 8);
    assert(at_4 == exp_strong_4);
    assert(exp_strong_4 > exp_weak_4);

    printf("  PASS: Higher effectiveness replaces lower\n");
}

// =============================================================================
// Lower effectiveness does NOT replace higher
// =============================================================================

void test_lower_does_not_replace_higher() {
    printf("Testing lower effectiveness does NOT replace higher...\n");

    // Order matters in the vector: put strong first, then weak
    // Max-value should still keep the strong value
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_enforcer_post(16, 16, 255));  // strong first
    buildings.push_back(make_enforcer_post(16, 16, 100));  // weak second

    calculate_radius_coverage(grid, buildings);

    // Center should still have the stronger building's value
    uint8_t center = grid.get_coverage_at(16, 16);
    assert(center == 255);

    // At distance 4: strong building should still dominate
    uint8_t at_4 = grid.get_coverage_at(20, 16);
    uint8_t exp_strong_4 = expected_coverage(255, 4, 8);
    assert(at_4 == exp_strong_4);

    printf("  PASS: Lower effectiveness does NOT replace higher\n");
}

// =============================================================================
// Non-overlapping buildings: each tile has only its building's value
// =============================================================================

void test_non_overlapping() {
    printf("Testing non-overlapping buildings have independent coverage...\n");

    // Place buildings far apart so their radii don't overlap
    // Enforcer Post: radius = 8
    // Building 1 at (4, 4), Building 2 at (28, 28)
    // Manhattan distance between them = 24+24 = 48, far beyond 2*8=16
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_enforcer_post(4, 4, 200));
    buildings.push_back(make_enforcer_post(28, 28, 150));

    calculate_radius_coverage(grid, buildings);

    // Building 1 center should have only building 1's value
    uint8_t b1_center = grid.get_coverage_at(4, 4);
    assert(b1_center == expected_coverage(200, 0, 8));

    // Building 2 center should have only building 2's value
    uint8_t b2_center = grid.get_coverage_at(28, 28);
    assert(b2_center == expected_coverage(150, 0, 8));

    // Tile near building 1 (distance 3) should only have building 1's contribution
    uint8_t near_b1 = grid.get_coverage_at(7, 4);
    assert(near_b1 == expected_coverage(200, 3, 8));

    // Tile near building 2 (distance 3) should only have building 2's contribution
    uint8_t near_b2 = grid.get_coverage_at(25, 28);
    assert(near_b2 == expected_coverage(150, 3, 8));

    // Midpoint tile (16, 16) should be 0 -- beyond both radii
    uint8_t mid = grid.get_coverage_at(16, 16);
    assert(mid == 0);

    printf("  PASS: Non-overlapping buildings have independent coverage\n");
}

// =============================================================================
// Full overlap (same position): stronger building wins
// =============================================================================

void test_full_overlap_same_position() {
    printf("Testing full overlap at same position...\n");

    // Three buildings at exact same position, different effectiveness
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_enforcer_post(16, 16, 100));
    buildings.push_back(make_enforcer_post(16, 16, 200));
    buildings.push_back(make_enforcer_post(16, 16, 150));

    calculate_radius_coverage(grid, buildings);

    // At every tile within radius, the building with effectiveness=200 should win
    for (int d = 0; d < 8; ++d) {
        uint8_t expected_val = expected_coverage(200, d, 8);
        uint8_t actual = grid.get_coverage_at(static_cast<uint32_t>(16 + d), 16);
        assert(actual == expected_val);
    }

    // Verify that the center is 200 (not 100, not 150, not accumulated)
    uint8_t center = grid.get_coverage_at(16, 16);
    assert(center == expected_coverage(200, 0, 8));

    // NOT accumulated: center should not be 100+200+150 clamped
    // It should be exactly what the 200-effectiveness building produces
    assert(center != 255);  // Would be 255 if stacked (100+200+150 > 255)

    printf("  PASS: Full overlap uses max-value, not accumulation\n");
}

// =============================================================================
// Edge overlap: only boundary tiles overlap
// =============================================================================

void test_edge_overlap() {
    printf("Testing edge overlap: only boundary tiles share coverage...\n");

    // Place two Enforcer Posts (radius=8) exactly 15 apart on x-axis
    // Building 1 at (4, 16), Building 2 at (19, 16)
    // B1 covers x=[0..12], B2 covers x=[11..27] (clamped to manhattan distance)
    // The overlap region is narrow around x=12 where both have low coverage
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_enforcer_post(4, 16, 255));
    buildings.push_back(make_enforcer_post(19, 16, 255));

    calculate_radius_coverage(grid, buildings);

    // Tile at (4, 16): only building 1 (distance to b2 = 15, beyond radius 8)
    int d_b1 = manhattan(4, 16, 4, 16);   // 0
    int d_b2 = manhattan(4, 16, 19, 16);  // 15
    assert(d_b2 > 8);  // Beyond b2's radius
    uint8_t val = grid.get_coverage_at(4, 16);
    assert(val == expected_coverage(255, d_b1, 8));

    // Tile at (19, 16): only building 2 (distance to b1 = 15, beyond radius 8)
    d_b1 = manhattan(19, 16, 4, 16);   // 15
    d_b2 = manhattan(19, 16, 19, 16);  // 0
    assert(d_b1 > 8);
    val = grid.get_coverage_at(19, 16);
    assert(val == expected_coverage(255, d_b2, 8));

    // Tile at (12, 16): within range of b1 (d=8 -> edge -> 0), b2 has d=7 -> coverage
    d_b1 = manhattan(12, 16, 4, 16);   // 8 -> at edge, 0
    d_b2 = manhattan(12, 16, 19, 16);  // 7
    uint8_t exp1 = expected_coverage(255, d_b1, 8);  // 0
    uint8_t exp2 = expected_coverage(255, d_b2, 8);  // small
    uint8_t expected_val = (exp1 > exp2) ? exp1 : exp2;
    val = grid.get_coverage_at(12, 16);
    assert(val == expected_val);

    // Tile at (11, 16): within range of b1 (d=7), b2 has d=8 -> edge -> 0
    d_b1 = manhattan(11, 16, 4, 16);   // 7
    d_b2 = manhattan(11, 16, 19, 16);  // 8 -> edge, 0
    exp1 = expected_coverage(255, d_b1, 8);
    exp2 = expected_coverage(255, d_b2, 8);  // 0
    expected_val = (exp1 > exp2) ? exp1 : exp2;
    val = grid.get_coverage_at(11, 16);
    assert(val == expected_val);
    assert(val == exp1);  // Only b1 contributes

    printf("  PASS: Edge overlap: only boundary tiles share coverage\n");
}

// =============================================================================
// No stacking verification: accumulated would exceed max single building
// =============================================================================

void test_no_stacking() {
    printf("Testing no stacking: coverage never exceeds single building max...\n");

    // Place 5 buildings all at the same spot with effectiveness 200
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    for (int i = 0; i < 5; ++i) {
        buildings.push_back(make_enforcer_post(16, 16, 200));
    }

    calculate_radius_coverage(grid, buildings);

    // If stacking: center would be 200*5 -> clamped to 255
    // With max-value: center should be exactly what one building at eff=200 gives
    uint8_t center = grid.get_coverage_at(16, 16);
    uint8_t single_building = expected_coverage(200, 0, 8);
    assert(center == single_building);

    // Check at various distances that values match single-building expectation
    for (int d = 0; d < 8; ++d) {
        uint8_t actual = grid.get_coverage_at(static_cast<uint32_t>(16 + d), 16);
        uint8_t expected_val = expected_coverage(200, d, 8);
        assert(actual == expected_val);
    }

    printf("  PASS: No stacking: coverage equals single building max\n");
}

// =============================================================================
// Different tiers overlapping: larger radius wins at edge
// =============================================================================

void test_different_tier_overlap() {
    printf("Testing overlap between different tiers...\n");

    // Post (radius=8) at (16,16) and Station (radius=12) at (16,16)
    // Both full effectiveness -- Station has larger radius
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_enforcer_post(16, 16, 255));     // radius=8
    buildings.push_back(make_enforcer_station(16, 16, 255));  // radius=12

    calculate_radius_coverage(grid, buildings);

    // At center: both provide 255 -> max is 255
    assert(grid.get_coverage_at(16, 16) == 255);

    // At distance 4: Post gives 1-4/8=0.5*255=128, Station gives 1-4/12=0.667*255=170
    // Max should be 170 (station wins)
    uint8_t at_4 = grid.get_coverage_at(20, 16);
    uint8_t exp_post_4 = expected_coverage(255, 4, 8);
    uint8_t exp_station_4 = expected_coverage(255, 4, 12);
    uint8_t expected_val = (exp_post_4 > exp_station_4) ? exp_post_4 : exp_station_4;
    assert(at_4 == expected_val);
    assert(at_4 == exp_station_4);  // Station wins at this distance

    // At distance 9: Post gives 0 (beyond radius 8), Station gives 1-9/12=0.25*255=64
    uint8_t at_9 = grid.get_coverage_at(25, 16);
    uint8_t exp_post_9 = expected_coverage(255, 9, 8);    // 0
    uint8_t exp_station_9 = expected_coverage(255, 9, 12); // ~64
    assert(exp_post_9 == 0);
    assert(at_9 == exp_station_9);

    printf("  PASS: Different tier overlap uses max-value correctly\n");
}

// =============================================================================
// Overlap with order independence: result same regardless of building order
// =============================================================================

void test_order_independence() {
    printf("Testing overlap result is independent of building order...\n");

    // Run coverage calculation with buildings in two different orders
    // and verify grids are identical
    ServiceCoverageGrid grid1(32, 32);
    ServiceCoverageGrid grid2(32, 32);

    std::vector<ServiceBuildingData> buildings_order1;
    buildings_order1.push_back(make_enforcer_post(10, 16, 255));
    buildings_order1.push_back(make_enforcer_post(14, 16, 180));
    buildings_order1.push_back(make_enforcer_post(12, 12, 200));

    std::vector<ServiceBuildingData> buildings_order2;
    buildings_order2.push_back(make_enforcer_post(12, 12, 200));
    buildings_order2.push_back(make_enforcer_post(10, 16, 255));
    buildings_order2.push_back(make_enforcer_post(14, 16, 180));

    calculate_radius_coverage(grid1, buildings_order1);
    calculate_radius_coverage(grid2, buildings_order2);

    // Every tile must match
    for (uint32_t y = 0; y < 32; ++y) {
        for (uint32_t x = 0; x < 32; ++x) {
            assert(grid1.get_coverage_at(x, y) == grid2.get_coverage_at(x, y));
        }
    }

    printf("  PASS: Coverage is order-independent\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Coverage Overlap Unit Tests (Epic 9, Ticket E9-022) ===\n\n");

    // Core overlap tests
    test_two_buildings_overlap();
    test_three_buildings_overlap();

    // Effectiveness priority tests
    test_higher_replaces_lower();
    test_lower_does_not_replace_higher();

    // Isolation test
    test_non_overlapping();

    // Full overlap
    test_full_overlap_same_position();

    // Edge overlap
    test_edge_overlap();

    // Anti-stacking
    test_no_stacking();

    // Different tiers
    test_different_tier_overlap();

    // Order independence
    test_order_independence();

    printf("\n=== All Coverage Overlap Tests Passed ===\n");
    return 0;
}
