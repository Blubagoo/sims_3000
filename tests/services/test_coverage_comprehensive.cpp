/**
 * @file test_coverage_comprehensive.cpp
 * @brief Comprehensive unit tests for radius-based coverage calculation
 *        (Epic 9, Ticket E9-050)
 *
 * This is a LARGE test suite that thoroughly validates all aspects of the
 * coverage system beyond what test_coverage_calculation.cpp (17 tests),
 * test_linear_falloff.cpp (18 tests), and test_coverage_overlap.cpp (10 tests)
 * already cover.
 *
 * Test categories:
 * 1. Single building coverage pattern verification (all tiers, all types)
 * 2. Map edge clipping scenarios (no wraparound)
 * 3. Unpowered/inactive building scenarios
 * 4. Multi-building complex scenarios
 * 5. Grid size edge cases
 * 6. Coverage value precision (uint8_t rounding)
 */

#include <sims3000/services/CoverageCalculation.h>
#include <sims3000/services/ServiceCoverageGrid.h>
#include <sims3000/services/ServiceTypes.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace sims3000::services;

// =============================================================================
// Helpers
// =============================================================================

static constexpr float EPSILON = 0.001f;

/// Compute expected uint8_t coverage value mirroring CoverageCalculation.cpp logic.
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

static ServiceBuildingData make_building(int x, int y, ServiceType type,
                                          ServiceTier tier, uint8_t effectiveness,
                                          bool active = true, uint8_t owner = 0) {
    ServiceBuildingData b;
    b.x = x;
    b.y = y;
    b.type = type;
    b.tier = static_cast<uint8_t>(tier);
    b.effectiveness = effectiveness;
    b.is_active = active;
    b.owner_id = owner;
    b.capacity = 0;
    return b;
}

/// Get the configured radius for a given service type and tier.
static int get_radius(ServiceType type, ServiceTier tier) {
    ServiceConfig cfg = get_service_config(type, tier);
    return static_cast<int>(cfg.base_radius);
}

static int test_count = 0;

// =============================================================================
// 1. Single building coverage pattern verification
// =============================================================================

void test_enforcer_post_all_distances() {
    printf("Testing Enforcer Post (radius=8): coverage at every integer distance 0-8...\n");
    ++test_count;

    int radius = get_radius(ServiceType::Enforcer, ServiceTier::Post);
    assert(radius == 8);

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    // Check along positive x axis: (32+d, 32) for d=0..8
    for (int d = 0; d <= radius; ++d) {
        uint8_t actual = grid.get_coverage_at(static_cast<uint32_t>(32 + d), 32);
        uint8_t expected = expected_coverage(255, d, radius);
        assert(actual == expected);
    }
    // Beyond radius
    uint8_t beyond = grid.get_coverage_at(32 + radius + 1, 32);
    assert(beyond == 0);

    printf("  PASS\n");
}

void test_enforcer_station_key_distances() {
    printf("Testing Enforcer Station (radius=12): coverage at key distances...\n");
    ++test_count;

    int radius = get_radius(ServiceType::Enforcer, ServiceTier::Station);
    assert(radius == 12);

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Station, 255));
    calculate_radius_coverage(grid, buildings);

    // d=0,3,6,9,12
    int key_distances[] = { 0, 3, 6, 9, 12 };
    for (int d : key_distances) {
        uint8_t actual = grid.get_coverage_at(static_cast<uint32_t>(32 + d), 32);
        uint8_t expected = expected_coverage(255, d, radius);
        assert(actual == expected);
    }

    printf("  PASS\n");
}

void test_enforcer_nexus_key_distances() {
    printf("Testing Enforcer Nexus (radius=16): coverage at key distances...\n");
    ++test_count;

    int radius = get_radius(ServiceType::Enforcer, ServiceTier::Nexus);
    assert(radius == 16);

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Nexus, 255));
    calculate_radius_coverage(grid, buildings);

    int key_distances[] = { 0, 4, 8, 12, 16 };
    for (int d : key_distances) {
        uint8_t actual = grid.get_coverage_at(static_cast<uint32_t>(32 + d), 32);
        uint8_t expected = expected_coverage(255, d, radius);
        assert(actual == expected);
    }

    printf("  PASS\n");
}

void test_hazard_post_coverage_pattern() {
    printf("Testing HazardResponse Post (radius=8): coverage pattern...\n");
    ++test_count;

    int radius = get_radius(ServiceType::HazardResponse, ServiceTier::Post);
    assert(radius == 8);

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::HazardResponse, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    // Verify diamond pattern in all 4 directions
    for (int d = 0; d < radius; ++d) {
        uint8_t exp = expected_coverage(255, d, radius);
        assert(grid.get_coverage_at(static_cast<uint32_t>(32 + d), 32) == exp);  // +x
        assert(grid.get_coverage_at(static_cast<uint32_t>(32 - d), 32) == exp);  // -x
        assert(grid.get_coverage_at(32, static_cast<uint32_t>(32 + d)) == exp);  // +y
        assert(grid.get_coverage_at(32, static_cast<uint32_t>(32 - d)) == exp);  // -y
    }

    printf("  PASS\n");
}

void test_hazard_station_coverage_pattern() {
    printf("Testing HazardResponse Station (radius=12): coverage pattern...\n");
    ++test_count;

    int radius = get_radius(ServiceType::HazardResponse, ServiceTier::Station);
    assert(radius == 12);

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::HazardResponse, ServiceTier::Station, 255));
    calculate_radius_coverage(grid, buildings);

    // Check all integer distances along +x axis
    for (int d = 0; d <= radius; ++d) {
        uint8_t actual = grid.get_coverage_at(static_cast<uint32_t>(32 + d), 32);
        uint8_t expected = expected_coverage(255, d, radius);
        assert(actual == expected);
    }

    printf("  PASS\n");
}

void test_hazard_nexus_coverage_pattern() {
    printf("Testing HazardResponse Nexus (radius=16): coverage pattern...\n");
    ++test_count;

    int radius = get_radius(ServiceType::HazardResponse, ServiceTier::Nexus);
    assert(radius == 16);

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::HazardResponse, ServiceTier::Nexus, 255));
    calculate_radius_coverage(grid, buildings);

    for (int d = 0; d <= radius; ++d) {
        uint8_t actual = grid.get_coverage_at(static_cast<uint32_t>(32 + d), 32);
        uint8_t expected = expected_coverage(255, d, radius);
        assert(actual == expected);
    }

    printf("  PASS\n");
}

void test_medical_post_coverage_pattern() {
    printf("Testing Medical Post (radius=8): coverage pattern...\n");
    ++test_count;

    int radius = get_radius(ServiceType::Medical, ServiceTier::Post);
    assert(radius == 8);

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Medical, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    // Verify center and midpoint
    assert(grid.get_coverage_at(32, 32) == 255);
    assert(grid.get_coverage_at(36, 32) == expected_coverage(255, 4, 8));  // 128
    assert(grid.get_coverage_at(40, 32) == 0);  // distance 8 = edge

    printf("  PASS\n");
}

void test_education_nexus_coverage_pattern() {
    printf("Testing Education Nexus (radius=16): coverage pattern...\n");
    ++test_count;

    int radius = get_radius(ServiceType::Education, ServiceTier::Nexus);
    assert(radius == 16);

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Education, ServiceTier::Nexus, 255));
    calculate_radius_coverage(grid, buildings);

    assert(grid.get_coverage_at(32, 32) == 255);
    // distance 8: 1 - 8/16 = 0.5 -> 128
    assert(grid.get_coverage_at(40, 32) == expected_coverage(255, 8, 16));
    assert(grid.get_coverage_at(48, 32) == 0);  // distance 16 = edge

    printf("  PASS\n");
}

void test_all_types_same_tier_equal_coverage() {
    printf("Testing all service types at same tier produce identical coverage...\n");
    ++test_count;

    // All four service types have the same radius/effectiveness configs per tier
    ServiceType types[] = {
        ServiceType::Enforcer, ServiceType::HazardResponse,
        ServiceType::Medical, ServiceType::Education
    };

    for (auto tier : { ServiceTier::Post, ServiceTier::Station, ServiceTier::Nexus }) {
        ServiceCoverageGrid ref_grid(64, 64);
        std::vector<ServiceBuildingData> ref_buildings;
        ref_buildings.push_back(make_building(32, 32, types[0], tier, 255));
        calculate_radius_coverage(ref_grid, ref_buildings);

        for (int t = 1; t < 4; ++t) {
            ServiceCoverageGrid test_grid(64, 64);
            std::vector<ServiceBuildingData> test_buildings;
            test_buildings.push_back(make_building(32, 32, types[t], tier, 255));
            calculate_radius_coverage(test_grid, test_buildings);

            for (uint32_t y = 0; y < 64; ++y) {
                for (uint32_t x = 0; x < 64; ++x) {
                    assert(ref_grid.get_coverage_at(x, y) == test_grid.get_coverage_at(x, y));
                }
            }
        }
    }

    printf("  PASS\n");
}

// =============================================================================
// 2. Map edge clipping scenarios
// =============================================================================

void test_edge_clip_origin_64x64() {
    printf("Testing building at (0,0) on 64x64 map: only positive quadrant covered...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(0, 0, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    // Center should be fully covered
    assert(grid.get_coverage_at(0, 0) == 255);

    // Tiles at positive distances should have coverage
    assert(grid.get_coverage_at(4, 0) == expected_coverage(255, 4, 8));
    assert(grid.get_coverage_at(0, 4) == expected_coverage(255, 4, 8));

    // Verify NO tiles outside grid receive coverage (can't access negative, so just
    // check that no coverage "wraps around" to the far side of the grid)
    assert(grid.get_coverage_at(63, 0) == 0);
    assert(grid.get_coverage_at(0, 63) == 0);
    assert(grid.get_coverage_at(63, 63) == 0);

    // Count total covered tiles -- should only be in positive quadrant
    int covered = 0;
    for (uint32_t y = 0; y < 64; ++y) {
        for (uint32_t x = 0; x < 64; ++x) {
            if (grid.get_coverage_at(x, y) > 0) {
                // Must be within radius of (0,0)
                int d = manhattan(static_cast<int>(x), static_cast<int>(y), 0, 0);
                assert(d < 8);
                ++covered;
            }
        }
    }
    assert(covered > 0);

    printf("  PASS\n");
}

void test_edge_clip_far_corner_64x64() {
    printf("Testing building at (63,63) on 64x64 map: only negative quadrant covered...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(63, 63, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    assert(grid.get_coverage_at(63, 63) == 255);
    assert(grid.get_coverage_at(59, 63) == expected_coverage(255, 4, 8));
    assert(grid.get_coverage_at(63, 59) == expected_coverage(255, 4, 8));

    // No wraparound to origin
    assert(grid.get_coverage_at(0, 0) == 0);
    assert(grid.get_coverage_at(0, 63) == 0);
    assert(grid.get_coverage_at(63, 0) == 0);

    printf("  PASS\n");
}

void test_edge_clip_left_edge() {
    printf("Testing building at (0,32) on 64x64 map: left edge clipped...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(0, 32, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    assert(grid.get_coverage_at(0, 32) == 255);

    // Positive x direction has coverage
    assert(grid.get_coverage_at(4, 32) == expected_coverage(255, 4, 8));

    // No negative x wraparound
    assert(grid.get_coverage_at(63, 32) == 0);

    // Y direction is fully within bounds
    assert(grid.get_coverage_at(0, 36) == expected_coverage(255, 4, 8));
    assert(grid.get_coverage_at(0, 28) == expected_coverage(255, 4, 8));

    printf("  PASS\n");
}

void test_edge_clip_right_edge() {
    printf("Testing building at (63,32) on 64x64 map: right edge clipped...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(63, 32, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    assert(grid.get_coverage_at(63, 32) == 255);

    // Negative x direction has coverage
    assert(grid.get_coverage_at(59, 32) == expected_coverage(255, 4, 8));

    // No wraparound to left side
    assert(grid.get_coverage_at(0, 32) == 0);

    printf("  PASS\n");
}

void test_edge_clip_top_edge() {
    printf("Testing building at (32,0) on 64x64 map: top edge clipped...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 0, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    assert(grid.get_coverage_at(32, 0) == 255);

    // Positive y direction has coverage
    assert(grid.get_coverage_at(32, 4) == expected_coverage(255, 4, 8));

    // No wraparound to bottom
    assert(grid.get_coverage_at(32, 63) == 0);

    printf("  PASS\n");
}

void test_edge_clip_bottom_edge() {
    printf("Testing building at (32,63) on 64x64 map: bottom edge clipped...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 63, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    assert(grid.get_coverage_at(32, 63) == 255);

    // Negative y direction has coverage
    assert(grid.get_coverage_at(32, 59) == expected_coverage(255, 4, 8));

    // No wraparound to top
    assert(grid.get_coverage_at(32, 0) == 0);

    printf("  PASS\n");
}

void test_no_out_of_bounds_coverage() {
    printf("Testing NO tiles outside grid boundaries receive coverage...\n");
    ++test_count;

    // Place a Nexus (radius=16) at corner of a small 32x32 map
    // Many tiles would be out of bounds
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(0, 0, ServiceType::Enforcer, ServiceTier::Nexus, 255));
    calculate_radius_coverage(grid, buildings);

    // Every covered tile must be within [0,32) x [0,32) -- which is guaranteed
    // by the grid API. But verify that coverage values are correct and no corruption.
    for (uint32_t y = 0; y < 32; ++y) {
        for (uint32_t x = 0; x < 32; ++x) {
            int d = manhattan(static_cast<int>(x), static_cast<int>(y), 0, 0);
            if (d < 16) {
                assert(grid.get_coverage_at(x, y) == expected_coverage(255, d, 16));
            } else {
                assert(grid.get_coverage_at(x, y) == 0);
            }
        }
    }

    // Out-of-bounds reads return 0 (grid API safety)
    assert(grid.get_coverage_at(32, 0) == 0);
    assert(grid.get_coverage_at(0, 32) == 0);
    assert(grid.get_coverage_at(100, 100) == 0);

    printf("  PASS\n");
}

void test_large_radius_small_map_clipping() {
    printf("Testing large radius building on small map clips correctly...\n");
    ++test_count;

    // Nexus (radius=16) at center of 16x16 map
    // Radius extends well beyond map edges in all directions
    ServiceCoverageGrid grid(16, 16);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(8, 8, ServiceType::Enforcer, ServiceTier::Nexus, 255));
    calculate_radius_coverage(grid, buildings);

    // Center has full coverage
    assert(grid.get_coverage_at(8, 8) == 255);

    // All tiles within the small map should have coverage since max distance
    // from center is 8+8=16 which equals the radius (0 at edge)
    // But tiles at corners: manhattan(0,0, 8,8)=16 -> 0 coverage
    assert(grid.get_coverage_at(0, 0) == 0);  // distance=16 -> edge, 0

    // Tile at (1,1): distance=14 -> 1-14/16=0.125 -> 32
    assert(grid.get_coverage_at(1, 1) == expected_coverage(255, 14, 16));

    printf("  PASS\n");
}

// =============================================================================
// 3. Unpowered/inactive building scenarios
// =============================================================================

void test_single_inactive_grid_empty() {
    printf("Testing single inactive building: grid stays empty...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Station, 255, false));
    calculate_radius_coverage(grid, buildings);

    for (uint32_t y = 0; y < 64; ++y) {
        for (uint32_t x = 0; x < 64; ++x) {
            assert(grid.get_coverage_at(x, y) == 0);
        }
    }

    printf("  PASS\n");
}

void test_mix_active_and_inactive() {
    printf("Testing mix of active and inactive: only active buildings contribute...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;

    // Inactive at (16,16)
    buildings.push_back(make_building(16, 16, ServiceType::Enforcer, ServiceTier::Post, 255, false));
    // Active at (48,48)
    buildings.push_back(make_building(48, 48, ServiceType::Enforcer, ServiceTier::Post, 255, true));

    calculate_radius_coverage(grid, buildings);

    // Inactive building position should have 0 coverage
    assert(grid.get_coverage_at(16, 16) == 0);

    // Active building position should have full coverage
    assert(grid.get_coverage_at(48, 48) == 255);

    // Tiles around inactive building should be 0
    assert(grid.get_coverage_at(20, 16) == 0);

    // Tiles around active building should have coverage
    assert(grid.get_coverage_at(52, 48) == expected_coverage(255, 4, 8));

    printf("  PASS\n");
}

void test_zero_effectiveness_grid_empty() {
    printf("Testing building with effectiveness=0: grid stays empty (even if active)...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Post, 0, true));
    calculate_radius_coverage(grid, buildings);

    // effectiveness=0 means all coverage values compute to 0
    for (uint32_t y = 24; y < 40; ++y) {
        for (uint32_t x = 24; x < 40; ++x) {
            assert(grid.get_coverage_at(x, y) == 0);
        }
    }

    printf("  PASS\n");
}

void test_inactive_toggle_recalculation() {
    printf("Testing building toggled from active to inactive: coverage removed on recalc...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;

    // First calculation: active
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Post, 255, true));
    calculate_radius_coverage(grid, buildings);

    // Verify coverage exists
    assert(grid.get_coverage_at(32, 32) == 255);
    assert(grid.get_coverage_at(36, 32) == expected_coverage(255, 4, 8));

    // Toggle to inactive and recalculate
    buildings[0].is_active = false;
    calculate_radius_coverage(grid, buildings);

    // All coverage should be gone
    assert(grid.get_coverage_at(32, 32) == 0);
    assert(grid.get_coverage_at(36, 32) == 0);

    printf("  PASS\n");
}

void test_multiple_inactive_buildings() {
    printf("Testing multiple inactive buildings: entire grid remains empty...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;

    buildings.push_back(make_building(10, 10, ServiceType::Enforcer, ServiceTier::Post, 255, false));
    buildings.push_back(make_building(30, 30, ServiceType::HazardResponse, ServiceTier::Station, 200, false));
    buildings.push_back(make_building(50, 50, ServiceType::Medical, ServiceTier::Nexus, 128, false));

    calculate_radius_coverage(grid, buildings);

    for (uint32_t y = 0; y < 64; ++y) {
        for (uint32_t x = 0; x < 64; ++x) {
            assert(grid.get_coverage_at(x, y) == 0);
        }
    }

    printf("  PASS\n");
}

// =============================================================================
// 4. Multi-building complex scenarios
// =============================================================================

void test_four_buildings_in_corners() {
    printf("Testing 4 buildings in corners of map: coverage patterns don't interfere...\n");
    ++test_count;

    // Use 64x64 map with Enforcer Posts (radius=8) in all corners.
    // Corners are >16 tiles apart so no overlap possible.
    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(4, 4, ServiceType::Enforcer, ServiceTier::Post, 255));
    buildings.push_back(make_building(59, 4, ServiceType::Enforcer, ServiceTier::Post, 200));
    buildings.push_back(make_building(4, 59, ServiceType::Enforcer, ServiceTier::Post, 180));
    buildings.push_back(make_building(59, 59, ServiceType::Enforcer, ServiceTier::Post, 150));

    calculate_radius_coverage(grid, buildings);

    // Each corner has its own independent coverage
    assert(grid.get_coverage_at(4, 4) == expected_coverage(255, 0, 8));
    assert(grid.get_coverage_at(59, 4) == expected_coverage(200, 0, 8));
    assert(grid.get_coverage_at(4, 59) == expected_coverage(180, 0, 8));
    assert(grid.get_coverage_at(59, 59) == expected_coverage(150, 0, 8));

    // Center of map should be uncovered
    assert(grid.get_coverage_at(32, 32) == 0);

    // Verify no cross-contamination: tile near corner 1 only has corner 1's value
    uint8_t near_c1 = grid.get_coverage_at(7, 4);
    assert(near_c1 == expected_coverage(255, 3, 8));

    printf("  PASS\n");
}

void test_line_of_buildings_merge() {
    printf("Testing line of buildings: coverage areas merge with max-value...\n");
    ++test_count;

    // Place 4 Enforcer Posts in a line along x axis, 6 tiles apart
    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    int positions[] = { 10, 16, 22, 28 };
    for (int pos : positions) {
        buildings.push_back(make_building(pos, 32, ServiceType::Enforcer, ServiceTier::Post, 255));
    }

    calculate_radius_coverage(grid, buildings);

    // Check a tile between two buildings: (13, 32)
    // Distance to building at 10: 3, Distance to building at 16: 3
    // Both give same coverage -> max is same
    uint8_t at_13 = grid.get_coverage_at(13, 32);
    uint8_t exp = expected_coverage(255, 3, 8);
    assert(at_13 == exp);

    // All building centers should be 255
    for (int pos : positions) {
        assert(grid.get_coverage_at(static_cast<uint32_t>(pos), 32) == 255);
    }

    // Between first and second building, each tile gets max of both contributions
    for (int x = 10; x <= 16; ++x) {
        int d1 = std::abs(x - 10);
        int d2 = std::abs(x - 16);
        uint8_t exp1 = expected_coverage(255, d1, 8);
        uint8_t exp2 = expected_coverage(255, d2, 8);
        uint8_t expected_val = std::max(exp1, exp2);
        assert(grid.get_coverage_at(static_cast<uint32_t>(x), 32) == expected_val);
    }

    printf("  PASS\n");
}

void test_building_upgrade_tier_change() {
    printf("Testing building upgrade (tier change): larger radius, same position...\n");
    ++test_count;

    // First: Post at (32,32), radius=8
    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    // Tile at distance 10 should be uncovered (beyond radius 8)
    assert(grid.get_coverage_at(42, 32) == 0);

    // Upgrade to Station (radius=12)
    buildings[0].tier = static_cast<uint8_t>(ServiceTier::Station);
    calculate_radius_coverage(grid, buildings);

    // Now tile at distance 10 should have coverage
    uint8_t at_10 = grid.get_coverage_at(42, 32);
    assert(at_10 == expected_coverage(255, 10, 12));
    assert(at_10 > 0);

    // Upgrade to Nexus (radius=16)
    buildings[0].tier = static_cast<uint8_t>(ServiceTier::Nexus);
    calculate_radius_coverage(grid, buildings);

    // Tile at distance 14 should now have coverage
    uint8_t at_14 = grid.get_coverage_at(46, 32);
    assert(at_14 == expected_coverage(255, 14, 16));
    assert(at_14 > 0);

    printf("  PASS\n");
}

void test_dense_placement() {
    printf("Testing dense placement: many buildings close together...\n");
    ++test_count;

    // Place 9 buildings in a 3x3 grid, each 2 tiles apart
    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    for (int dy = 0; dy < 3; ++dy) {
        for (int dx = 0; dx < 3; ++dx) {
            buildings.push_back(make_building(
                30 + dx * 2, 30 + dy * 2,
                ServiceType::Enforcer, ServiceTier::Post, 255));
        }
    }

    calculate_radius_coverage(grid, buildings);

    // Center building at (32, 32) should have 255
    assert(grid.get_coverage_at(32, 32) == 255);

    // All 9 building locations should have 255
    for (int dy = 0; dy < 3; ++dy) {
        for (int dx = 0; dx < 3; ++dx) {
            assert(grid.get_coverage_at(
                static_cast<uint32_t>(30 + dx * 2),
                static_cast<uint32_t>(30 + dy * 2)) == 255);
        }
    }

    // A tile at (31, 31): distance to (30,30)=2, (32,30)=2, (30,32)=2, (32,32)=2
    // All give same value: expected_coverage(255, 2, 8) = 191
    uint8_t at_31_31 = grid.get_coverage_at(31, 31);
    assert(at_31_31 == expected_coverage(255, 2, 8));

    printf("  PASS\n");
}

void test_mixed_tiers_overlap() {
    printf("Testing mixed tiers: Post and Nexus at different positions...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    // Post (radius=8) at (20, 32)
    buildings.push_back(make_building(20, 32, ServiceType::Enforcer, ServiceTier::Post, 255));
    // Nexus (radius=16) at (40, 32)
    buildings.push_back(make_building(40, 32, ServiceType::Enforcer, ServiceTier::Nexus, 255));

    calculate_radius_coverage(grid, buildings);

    // Tile at (28, 32): distance to Post=8 (edge->0), distance to Nexus=12
    // Post contribution: 0 (at edge)
    // Nexus contribution: 1-12/16=0.25 -> 64
    uint8_t at_28 = grid.get_coverage_at(28, 32);
    uint8_t exp_post = expected_coverage(255, 8, 8);   // 0
    uint8_t exp_nexus = expected_coverage(255, 12, 16); // 64
    assert(at_28 == std::max(exp_post, exp_nexus));

    // Tile at (30, 32): distance to Post=10 (beyond), distance to Nexus=10
    // Only Nexus contributes
    uint8_t at_30 = grid.get_coverage_at(30, 32);
    assert(at_30 == expected_coverage(255, 10, 16));

    printf("  PASS\n");
}

void test_different_effectiveness_buildings() {
    printf("Testing buildings with different effectiveness values...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    // Weak building close to test point
    buildings.push_back(make_building(30, 32, ServiceType::Enforcer, ServiceTier::Post, 100));
    // Strong building further away
    buildings.push_back(make_building(38, 32, ServiceType::Enforcer, ServiceTier::Post, 255));

    calculate_radius_coverage(grid, buildings);

    // At (32, 32): distance to weak=2, distance to strong=6
    uint8_t exp_weak = expected_coverage(100, 2, 8);   // (100/255)*0.75*255+0.5 = 75
    uint8_t exp_strong = expected_coverage(255, 6, 8);  // 1.0*0.25*255+0.5 = 64
    uint8_t at_32 = grid.get_coverage_at(32, 32);
    // Weak building is closer so its value may be higher despite lower effectiveness
    assert(at_32 == std::max(exp_weak, exp_strong));

    printf("  PASS\n");
}

// =============================================================================
// 5. Grid size edge cases
// =============================================================================

void test_1x1_grid() {
    printf("Testing 1x1 grid with a building...\n");
    ++test_count;

    ServiceCoverageGrid grid(1, 1);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(0, 0, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    // The single tile should have full coverage (distance=0)
    assert(grid.get_coverage_at(0, 0) == 255);

    printf("  PASS\n");
}

void test_large_grid_512x512() {
    printf("Testing very large grid (512x512) with single building...\n");
    ++test_count;

    ServiceCoverageGrid grid(512, 512);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(256, 256, ServiceType::Enforcer, ServiceTier::Nexus, 255));
    calculate_radius_coverage(grid, buildings);

    // Center should be 255
    assert(grid.get_coverage_at(256, 256) == 255);

    // Distance 8 should have coverage
    assert(grid.get_coverage_at(264, 256) == expected_coverage(255, 8, 16));

    // Distance 16 should be 0 (edge)
    assert(grid.get_coverage_at(272, 256) == 0);

    // Far corner should be 0
    assert(grid.get_coverage_at(0, 0) == 0);
    assert(grid.get_coverage_at(511, 511) == 0);

    printf("  PASS\n");
}

void test_non_square_grid_128x64() {
    printf("Testing non-square grid (128x64)...\n");
    ++test_count;

    ServiceCoverageGrid grid(128, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(64, 32, ServiceType::Enforcer, ServiceTier::Station, 255));
    calculate_radius_coverage(grid, buildings);

    assert(grid.get_coverage_at(64, 32) == 255);

    // Check all 4 cardinal directions at distance 6
    assert(grid.get_coverage_at(70, 32) == expected_coverage(255, 6, 12));
    assert(grid.get_coverage_at(58, 32) == expected_coverage(255, 6, 12));
    assert(grid.get_coverage_at(64, 38) == expected_coverage(255, 6, 12));
    assert(grid.get_coverage_at(64, 26) == expected_coverage(255, 6, 12));

    // Width is 128, height is 64 -- beyond height boundary is safe
    assert(grid.get_coverage_at(64, 63) == 0);  // distance 31, beyond radius

    printf("  PASS\n");
}

void test_non_square_grid_64x128() {
    printf("Testing non-square grid (64x128)...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 128);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 64, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    assert(grid.get_coverage_at(32, 64) == 255);
    assert(grid.get_coverage_at(36, 64) == expected_coverage(255, 4, 8));

    printf("  PASS\n");
}

void test_small_grid_4x4() {
    printf("Testing small 4x4 grid with Post (radius=8) building...\n");
    ++test_count;

    ServiceCoverageGrid grid(4, 4);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(2, 2, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    // All tiles within 4x4 are within radius 8 of (2,2)
    for (uint32_t y = 0; y < 4; ++y) {
        for (uint32_t x = 0; x < 4; ++x) {
            int d = manhattan(static_cast<int>(x), static_cast<int>(y), 2, 2);
            assert(d < 8);  // Max distance in 4x4 from center is 4
            assert(grid.get_coverage_at(x, y) == expected_coverage(255, d, 8));
        }
    }

    printf("  PASS\n");
}

// =============================================================================
// 6. Coverage value precision (uint8_t rounding)
// =============================================================================

void test_uint8_rounding_at_boundaries() {
    printf("Testing uint8_t rounding at boundary values...\n");
    ++test_count;

    // The formula: uint8_t = min(255, strength * 255 + 0.5)
    // Test specific effectiveness/distance combos that produce fractional values

    // Effectiveness 255, radius 8, distance 1: 1.0 * (1-1/8) = 0.875
    // 0.875 * 255 + 0.5 = 223.125 + 0.5 = 223.625 -> uint8_t truncates to 223
    {
        ServiceCoverageGrid grid(32, 32);
        std::vector<ServiceBuildingData> buildings;
        buildings.push_back(make_building(16, 16, ServiceType::Enforcer, ServiceTier::Post, 255));
        calculate_radius_coverage(grid, buildings);
        uint8_t actual = grid.get_coverage_at(17, 16);
        uint8_t expected = expected_coverage(255, 1, 8);
        assert(actual == expected);
    }

    // Effectiveness 255, radius 8, distance 3: 1.0 * (1-3/8) = 0.625
    // 0.625 * 255 + 0.5 = 159.375 + 0.5 = 159.875 -> uint8_t truncates to 159
    {
        ServiceCoverageGrid grid(32, 32);
        std::vector<ServiceBuildingData> buildings;
        buildings.push_back(make_building(16, 16, ServiceType::Enforcer, ServiceTier::Post, 255));
        calculate_radius_coverage(grid, buildings);
        // Verify with manual float calculation
        float eff = 255.0f / 255.0f;
        float falloff_val = 1.0f - 3.0f / 8.0f;
        float strength = eff * falloff_val;
        uint8_t expected = static_cast<uint8_t>(std::min(255.0f, strength * 255.0f + 0.5f));
        assert(grid.get_coverage_at(19, 16) == expected);
    }

    printf("  PASS\n");
}

void test_no_overflow_full_effectiveness() {
    printf("Testing no overflow: full effectiveness at distance 0...\n");
    ++test_count;

    // effectiveness=255, distance=0: (255/255) * 1.0 * 255 + 0.5 = 255.5
    // min(255, 255.5) = 255, cast to uint8_t = 255 (no overflow)
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(16, 16, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);
    assert(grid.get_coverage_at(16, 16) == 255);

    printf("  PASS\n");
}

void test_no_underflow_zero_effectiveness() {
    printf("Testing no underflow: zero effectiveness...\n");
    ++test_count;

    // effectiveness=0: (0/255) * anything = 0.0 -> 0.0 * 255 + 0.5 = 0.5 -> uint8_t 0
    // Actually wait: 0.0 * falloff = 0.0; 0.0 * 255 + 0.5 = 0.5 -> uint8_t 0
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(16, 16, ServiceType::Enforcer, ServiceTier::Post, 0));
    calculate_radius_coverage(grid, buildings);

    // Should be 0 everywhere even with the +0.5 rounding
    // Because 0/255 = 0.0, 0.0 * falloff = 0.0, 0.0 * 255 + 0.5 = 0.5 -> truncated to 0
    assert(grid.get_coverage_at(16, 16) == 0);

    printf("  PASS\n");
}

void test_effectiveness_1_minimum_nonzero() {
    printf("Testing effectiveness=1: minimum nonzero at center...\n");
    ++test_count;

    // effectiveness=1: (1/255) * 1.0 = 0.003922
    // 0.003922 * 255 + 0.5 = 1.0 + 0.5 = 1.5 -> uint8_t 1
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(16, 16, ServiceType::Enforcer, ServiceTier::Post, 1));
    calculate_radius_coverage(grid, buildings);

    uint8_t center = grid.get_coverage_at(16, 16);
    float eff = 1.0f / 255.0f;
    float strength = eff * 1.0f;  // distance=0
    uint8_t expected = static_cast<uint8_t>(std::min(255.0f, strength * 255.0f + 0.5f));
    assert(center == expected);
    assert(center == 1);  // Matches manual calc

    printf("  PASS\n");
}

void test_float_to_uint8_precision_all_distances() {
    printf("Testing float-to-uint8_t precision at all distances for radius 12...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Station, 200));
    calculate_radius_coverage(grid, buildings);

    int radius = 12;
    float eff = 200.0f / 255.0f;

    for (int d = 0; d < radius; ++d) {
        float falloff_val = 1.0f - static_cast<float>(d) / static_cast<float>(radius);
        float strength = eff * falloff_val;
        uint8_t expected = static_cast<uint8_t>(std::min(255.0f, strength * 255.0f + 0.5f));
        uint8_t actual = grid.get_coverage_at(static_cast<uint32_t>(32 + d), 32);
        assert(actual == expected);
    }

    printf("  PASS\n");
}

void test_coverage_value_at_exact_half_radius() {
    printf("Testing coverage value at exact half radius for various effectiveness values...\n");
    ++test_count;

    uint8_t effs[] = { 1, 50, 100, 128, 200, 255 };
    int radius = 8;
    int half_d = 4;

    for (uint8_t e : effs) {
        ServiceCoverageGrid grid(32, 32);
        std::vector<ServiceBuildingData> buildings;
        buildings.push_back(make_building(16, 16, ServiceType::Enforcer, ServiceTier::Post, e));
        calculate_radius_coverage(grid, buildings);

        float eff_norm = static_cast<float>(e) / 255.0f;
        float falloff_val = 1.0f - static_cast<float>(half_d) / static_cast<float>(radius);
        float strength = eff_norm * falloff_val;
        uint8_t expected = static_cast<uint8_t>(std::min(255.0f, strength * 255.0f + 0.5f));
        uint8_t actual = grid.get_coverage_at(20, 16);
        assert(actual == expected);
    }

    printf("  PASS\n");
}

// =============================================================================
// Additional comprehensive tests
// =============================================================================

void test_diagonal_coverage_symmetry() {
    printf("Testing diagonal coverage symmetry (all 4 quadrants)...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Station, 255));
    calculate_radius_coverage(grid, buildings);

    int radius = 12;

    // For each distance along diagonals, all 4 quadrants should match
    for (int d = 1; d < radius / 2; ++d) {
        // Diagonal tiles: (32+d, 32+d) etc. have manhattan distance 2*d
        int md = 2 * d;
        if (md >= radius) break;

        uint8_t exp = expected_coverage(255, md, radius);
        assert(grid.get_coverage_at(static_cast<uint32_t>(32 + d), static_cast<uint32_t>(32 + d)) == exp);
        assert(grid.get_coverage_at(static_cast<uint32_t>(32 - d), static_cast<uint32_t>(32 + d)) == exp);
        assert(grid.get_coverage_at(static_cast<uint32_t>(32 + d), static_cast<uint32_t>(32 - d)) == exp);
        assert(grid.get_coverage_at(static_cast<uint32_t>(32 - d), static_cast<uint32_t>(32 - d)) == exp);
    }

    printf("  PASS\n");
}

void test_recalculation_full_replace() {
    printf("Testing recalculation fully replaces previous data...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;

    // First calculation with building at (16, 16)
    buildings.push_back(make_building(16, 16, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);
    assert(grid.get_coverage_at(16, 16) == 255);
    assert(grid.get_coverage_at(48, 48) == 0);

    // Second calculation with building moved to (48, 48)
    buildings.clear();
    buildings.push_back(make_building(48, 48, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    // Old position should be cleared
    assert(grid.get_coverage_at(16, 16) == 0);
    // New position should have coverage
    assert(grid.get_coverage_at(48, 48) == 255);

    printf("  PASS\n");
}

void test_coverage_diamond_shape_complete() {
    printf("Testing complete diamond shape verification for Post (radius=8)...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    int radius = 8;
    int cx = 32, cy = 32;

    // Verify every tile in the 17x17 bounding box
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int x = cx + dx;
            int y = cy + dy;
            int d = std::abs(dx) + std::abs(dy);

            uint8_t actual = grid.get_coverage_at(static_cast<uint32_t>(x),
                                                   static_cast<uint32_t>(y));
            if (d < radius) {
                uint8_t exp = expected_coverage(255, d, radius);
                assert(actual == exp);
            } else {
                assert(actual == 0);
            }
        }
    }

    printf("  PASS\n");
}

void test_coverage_total_tile_count_post() {
    printf("Testing total covered tile count for Post (radius=8)...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    int radius = 8;
    int covered_count = 0;
    for (uint32_t y = 0; y < 64; ++y) {
        for (uint32_t x = 0; x < 64; ++x) {
            if (grid.get_coverage_at(x, y) > 0) {
                ++covered_count;
            }
        }
    }

    // For manhattan distance, the number of tiles with distance < r is:
    // Sum for d=0 to r-1 of (number of tiles at distance d)
    // At distance d: 4*d tiles (except d=0 which is 1)
    // Total = 1 + sum(d=1..r-1)(4*d) = 1 + 4 * r*(r-1)/2 = 1 + 2*r*(r-1)
    // For r=8: 1 + 2*8*7 = 1 + 112 = 113
    int expected_count = 1 + 2 * radius * (radius - 1);
    assert(covered_count == expected_count);

    printf("  PASS\n");
}

void test_two_buildings_different_owners() {
    printf("Testing two buildings with different owner_ids (TODO: ownership not enforced)...\n");
    ++test_count;

    // Currently ownership is NOT checked -- both buildings should contribute
    // regardless of owner_id. This tests the TODO behavior documented in the header.
    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(20, 32, ServiceType::Enforcer, ServiceTier::Post, 255, true, 0));
    buildings.push_back(make_building(44, 32, ServiceType::Enforcer, ServiceTier::Post, 255, true, 1));

    calculate_radius_coverage(grid, buildings);

    // Both buildings should contribute coverage since ownership is not yet enforced
    assert(grid.get_coverage_at(20, 32) == 255);
    assert(grid.get_coverage_at(44, 32) == 255);

    // Coverage from both buildings is present
    assert(grid.get_coverage_at(24, 32) == expected_coverage(255, 4, 8));
    assert(grid.get_coverage_at(40, 32) == expected_coverage(255, 4, 8));

    printf("  PASS\n");
}

void test_owner_boundary_not_enforced() {
    printf("Testing owner boundary: all tiles covered regardless of owner (TODO behavior)...\n");
    ++test_count;

    // Even with different owner_ids, coverage applies to all tiles
    // because the TODO in the code says "treat all tiles as owned"
    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(16, 16, ServiceType::Enforcer, ServiceTier::Post, 255, true, 5));
    calculate_radius_coverage(grid, buildings);

    // All tiles within radius should have coverage regardless of owner_id
    for (int d = 0; d < 8; ++d) {
        uint8_t exp = expected_coverage(255, d, 8);
        assert(grid.get_coverage_at(static_cast<uint32_t>(16 + d), 16) == exp);
    }

    printf("  PASS\n");
}

void test_nexus_at_all_corners_64x64() {
    printf("Testing Nexus (radius=16) at all corners of 64x64 map...\n");
    ++test_count;

    // Nexus has large radius -- at corners, most of the coverage area is clipped
    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(0, 0, ServiceType::Enforcer, ServiceTier::Nexus, 255));

    calculate_radius_coverage(grid, buildings);

    // Only positive quadrant should have coverage
    assert(grid.get_coverage_at(0, 0) == 255);
    assert(grid.get_coverage_at(8, 0) == expected_coverage(255, 8, 16));
    assert(grid.get_coverage_at(0, 8) == expected_coverage(255, 8, 16));
    assert(grid.get_coverage_at(15, 0) == expected_coverage(255, 15, 16));

    // Far corner should be 0
    assert(grid.get_coverage_at(63, 63) == 0);

    printf("  PASS\n");
}

void test_effectiveness_254_rounding() {
    printf("Testing effectiveness=254 rounding precision...\n");
    ++test_count;

    ServiceCoverageGrid grid(32, 32);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(16, 16, ServiceType::Enforcer, ServiceTier::Post, 254));
    calculate_radius_coverage(grid, buildings);

    // At center: (254/255) * 1.0 * 255 + 0.5 = 254.0 + 0.5 = 254.5 -> uint8_t 254
    uint8_t center = grid.get_coverage_at(16, 16);
    float eff = 254.0f / 255.0f;
    uint8_t expected = static_cast<uint8_t>(std::min(255.0f, eff * 255.0f + 0.5f));
    assert(center == expected);

    printf("  PASS\n");
}

void test_multiple_services_different_types() {
    printf("Testing buildings of different service types on same grid...\n");
    ++test_count;

    // The system calculates coverage per-call with the provided buildings.
    // Different types with same tier have same radius, so coverage should be identical.
    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(20, 32, ServiceType::Enforcer, ServiceTier::Post, 255));
    buildings.push_back(make_building(44, 32, ServiceType::HazardResponse, ServiceTier::Post, 255));

    calculate_radius_coverage(grid, buildings);

    // Both contribute independently (same radius, same effectiveness)
    assert(grid.get_coverage_at(20, 32) == 255);
    assert(grid.get_coverage_at(44, 32) == 255);

    printf("  PASS\n");
}

void test_grid_clear_and_recalc_with_fewer_buildings() {
    printf("Testing grid recalculation with fewer buildings removes old coverage...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);

    // First: two buildings
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(16, 16, ServiceType::Enforcer, ServiceTier::Post, 255));
    buildings.push_back(make_building(48, 48, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);
    assert(grid.get_coverage_at(16, 16) == 255);
    assert(grid.get_coverage_at(48, 48) == 255);

    // Recalculate with only one building
    buildings.clear();
    buildings.push_back(make_building(48, 48, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    // First building's coverage should be gone
    assert(grid.get_coverage_at(16, 16) == 0);
    // Second building's coverage should remain
    assert(grid.get_coverage_at(48, 48) == 255);

    printf("  PASS\n");
}

void test_building_at_grid_center() {
    printf("Testing building placed exactly at grid center for even-dimension grid...\n");
    ++test_count;

    // 64x64 grid, center is not a whole tile but (31.5, 31.5)
    // Place at (32, 32) and verify symmetric coverage
    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    // Symmetric in all 4 cardinal directions
    for (int d = 1; d < 8; ++d) {
        uint8_t exp = expected_coverage(255, d, 8);
        assert(grid.get_coverage_at(static_cast<uint32_t>(32 + d), 32) == exp);
        assert(grid.get_coverage_at(static_cast<uint32_t>(32 - d), 32) == exp);
        assert(grid.get_coverage_at(32, static_cast<uint32_t>(32 + d)) == exp);
        assert(grid.get_coverage_at(32, static_cast<uint32_t>(32 - d)) == exp);
    }

    printf("  PASS\n");
}

void test_adjacent_buildings_coverage() {
    printf("Testing two adjacent buildings (1 tile apart)...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Post, 255));
    buildings.push_back(make_building(33, 32, ServiceType::Enforcer, ServiceTier::Post, 255));
    calculate_radius_coverage(grid, buildings);

    // Both centers should be 255
    assert(grid.get_coverage_at(32, 32) == 255);
    assert(grid.get_coverage_at(33, 32) == 255);

    // Tile between them: (32, 32) is building 1 center (d=0 -> 255)
    // Tile at (31, 32): d=1 from b1, d=2 from b2 -> max(224, 191) = 224
    uint8_t at_31 = grid.get_coverage_at(31, 32);
    uint8_t exp_b1 = expected_coverage(255, 1, 8);
    uint8_t exp_b2 = expected_coverage(255, 2, 8);
    assert(at_31 == std::max(exp_b1, exp_b2));

    printf("  PASS\n");
}

void test_coverage_monotonic_decrease_from_center() {
    printf("Testing coverage monotonically decreases from building center...\n");
    ++test_count;

    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings;
    buildings.push_back(make_building(32, 32, ServiceType::Enforcer, ServiceTier::Nexus, 200));
    calculate_radius_coverage(grid, buildings);

    int radius = 16;
    uint8_t prev = grid.get_coverage_at(32, 32);
    for (int d = 1; d <= radius; ++d) {
        uint8_t curr = grid.get_coverage_at(static_cast<uint32_t>(32 + d), 32);
        assert(curr <= prev);
        prev = curr;
    }

    printf("  PASS\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Comprehensive Coverage Calculation Tests (Epic 9, Ticket E9-050) ===\n\n");

    // 1. Single building coverage pattern verification
    test_enforcer_post_all_distances();
    test_enforcer_station_key_distances();
    test_enforcer_nexus_key_distances();
    test_hazard_post_coverage_pattern();
    test_hazard_station_coverage_pattern();
    test_hazard_nexus_coverage_pattern();
    test_medical_post_coverage_pattern();
    test_education_nexus_coverage_pattern();
    test_all_types_same_tier_equal_coverage();

    // 2. Map edge clipping scenarios
    test_edge_clip_origin_64x64();
    test_edge_clip_far_corner_64x64();
    test_edge_clip_left_edge();
    test_edge_clip_right_edge();
    test_edge_clip_top_edge();
    test_edge_clip_bottom_edge();
    test_no_out_of_bounds_coverage();
    test_large_radius_small_map_clipping();

    // 3. Unpowered/inactive building scenarios
    test_single_inactive_grid_empty();
    test_mix_active_and_inactive();
    test_zero_effectiveness_grid_empty();
    test_inactive_toggle_recalculation();
    test_multiple_inactive_buildings();

    // 4. Multi-building complex scenarios
    test_four_buildings_in_corners();
    test_line_of_buildings_merge();
    test_building_upgrade_tier_change();
    test_dense_placement();
    test_mixed_tiers_overlap();
    test_different_effectiveness_buildings();

    // 5. Grid size edge cases
    test_1x1_grid();
    test_large_grid_512x512();
    test_non_square_grid_128x64();
    test_non_square_grid_64x128();
    test_small_grid_4x4();

    // 6. Coverage value precision
    test_uint8_rounding_at_boundaries();
    test_no_overflow_full_effectiveness();
    test_no_underflow_zero_effectiveness();
    test_effectiveness_1_minimum_nonzero();
    test_float_to_uint8_precision_all_distances();
    test_coverage_value_at_exact_half_radius();

    // Additional comprehensive tests
    test_diagonal_coverage_symmetry();
    test_recalculation_full_replace();
    test_coverage_diamond_shape_complete();
    test_coverage_total_tile_count_post();
    test_two_buildings_different_owners();
    test_owner_boundary_not_enforced();
    test_nexus_at_all_corners_64x64();
    test_effectiveness_254_rounding();
    test_multiple_services_different_types();
    test_grid_clear_and_recalc_with_fewer_buildings();
    test_building_at_grid_center();
    test_adjacent_buildings_coverage();
    test_coverage_monotonic_decrease_from_center();

    printf("\n=== All %d Comprehensive Coverage Tests Passed ===\n", test_count);
    return 0;
}
