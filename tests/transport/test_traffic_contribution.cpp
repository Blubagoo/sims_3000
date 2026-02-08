/**
 * @file test_traffic_contribution.cpp
 * @brief Unit tests for TrafficContribution (Epic 7, Ticket E7-012)
 */

#include <sims3000/transport/TrafficContribution.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_default_config_values() {
    printf("Testing default config values...\n");

    TrafficContributionConfig config{};
    assert(config.habitation_base == 2);
    assert(config.exchange_base == 5);
    assert(config.fabrication_base == 3);

    printf("  PASS: Default config values are correct\n");
}

void test_habitation_contribution() {
    printf("Testing habitation traffic contribution...\n");

    // zone_type 0 = habitation, base = 2
    // Full occupancy (255), level 1: (2 * 255 * 1) / 255 = 2
    uint32_t result = calculate_traffic_contribution(0, 255, 1);
    assert(result == 2);

    // Half occupancy (128), level 1: (2 * 128 * 1) / 255 = 1 (integer division)
    result = calculate_traffic_contribution(0, 128, 1);
    assert(result == 1);

    // Empty building (0), level 1: (2 * 0 * 1) / 255 = 0
    result = calculate_traffic_contribution(0, 0, 1);
    assert(result == 0);

    printf("  PASS: Habitation contribution calculated correctly\n");
}

void test_exchange_contribution() {
    printf("Testing exchange traffic contribution...\n");

    // zone_type 1 = exchange, base = 5
    // Full occupancy (255), level 1: (5 * 255 * 1) / 255 = 5
    uint32_t result = calculate_traffic_contribution(1, 255, 1);
    assert(result == 5);

    // Half occupancy (128), level 1: (5 * 128 * 1) / 255 = 2 (integer division)
    result = calculate_traffic_contribution(1, 128, 1);
    assert(result == 2);

    printf("  PASS: Exchange contribution calculated correctly\n");
}

void test_fabrication_contribution() {
    printf("Testing fabrication traffic contribution...\n");

    // zone_type 2 = fabrication, base = 3
    // Full occupancy (255), level 1: (3 * 255 * 1) / 255 = 3
    uint32_t result = calculate_traffic_contribution(2, 255, 1);
    assert(result == 3);

    printf("  PASS: Fabrication contribution calculated correctly\n");
}

void test_level_scaling() {
    printf("Testing level scaling...\n");

    // Level 1: (2 * 255 * 1) / 255 = 2
    uint32_t level1 = calculate_traffic_contribution(0, 255, 1);
    assert(level1 == 2);

    // Level 2: (2 * 255 * 2) / 255 = 4  (2x level 1)
    uint32_t level2 = calculate_traffic_contribution(0, 255, 2);
    assert(level2 == 4);

    // Level 3: (2 * 255 * 3) / 255 = 6  (3x level 1)
    uint32_t level3 = calculate_traffic_contribution(0, 255, 3);
    assert(level3 == 6);

    // Verify linear scaling relationship
    assert(level2 == level1 * 2);
    assert(level3 == level1 * 3);

    printf("  PASS: Level scaling applied correctly\n");
}

void test_level_scaling_exchange() {
    printf("Testing level scaling for exchange zone...\n");

    // Exchange base = 5, full occupancy
    // Level 1: (5 * 255 * 1) / 255 = 5
    uint32_t level1 = calculate_traffic_contribution(1, 255, 1);
    assert(level1 == 5);

    // Level 2: (5 * 255 * 2) / 255 = 10
    uint32_t level2 = calculate_traffic_contribution(1, 255, 2);
    assert(level2 == 10);

    // Level 3: (5 * 255 * 3) / 255 = 15
    uint32_t level3 = calculate_traffic_contribution(1, 255, 3);
    assert(level3 == 15);

    printf("  PASS: Level scaling for exchange zone correct\n");
}

void test_invalid_zone_type() {
    printf("Testing invalid zone type returns 0...\n");

    // zone_type 3 is invalid
    uint32_t result = calculate_traffic_contribution(3, 255, 3);
    assert(result == 0);

    // zone_type 255 is invalid
    result = calculate_traffic_contribution(255, 255, 3);
    assert(result == 0);

    printf("  PASS: Invalid zone type returns 0\n");
}

void test_zero_level() {
    printf("Testing zero level returns 0...\n");

    // level 0 should produce 0 regardless of other parameters
    uint32_t result = calculate_traffic_contribution(0, 255, 0);
    assert(result == 0);

    result = calculate_traffic_contribution(1, 255, 0);
    assert(result == 0);

    result = calculate_traffic_contribution(2, 255, 0);
    assert(result == 0);

    printf("  PASS: Zero level returns 0\n");
}

void test_custom_config() {
    printf("Testing custom config values...\n");

    TrafficContributionConfig config{};
    config.habitation_base = 10;
    config.exchange_base = 20;
    config.fabrication_base = 15;

    // Habitation with custom config: (10 * 255 * 1) / 255 = 10
    uint32_t result = calculate_traffic_contribution(0, 255, 1, config);
    assert(result == 10);

    // Exchange with custom config: (20 * 255 * 1) / 255 = 20
    result = calculate_traffic_contribution(1, 255, 1, config);
    assert(result == 20);

    // Fabrication with custom config: (15 * 255 * 1) / 255 = 15
    result = calculate_traffic_contribution(2, 255, 1, config);
    assert(result == 15);

    printf("  PASS: Custom config values applied correctly\n");
}

void test_partial_occupancy_with_levels() {
    printf("Testing partial occupancy combined with levels...\n");

    // Habitation, half occupancy (128), level 2: (2 * 128 * 2) / 255 = 2
    uint32_t result = calculate_traffic_contribution(0, 128, 2);
    assert(result == 2);

    // Exchange, quarter occupancy (64), level 3: (5 * 64 * 3) / 255 = 3
    result = calculate_traffic_contribution(1, 64, 3);
    assert(result == 3);

    // Fabrication, 3/4 occupancy (192), level 2: (3 * 192 * 2) / 255 = 4
    result = calculate_traffic_contribution(2, 192, 2);
    assert(result == 4);

    printf("  PASS: Partial occupancy with levels calculated correctly\n");
}

void test_boundary_values() {
    printf("Testing boundary values...\n");

    // Minimum non-zero: base=2, occupancy=1, level=1: (2 * 1 * 1) / 255 = 0
    uint32_t result = calculate_traffic_contribution(0, 1, 1);
    assert(result == 0);

    // Max occupancy, max level for exchange: (5 * 255 * 3) / 255 = 15
    result = calculate_traffic_contribution(1, 255, 3);
    assert(result == 15);

    printf("  PASS: Boundary values handled correctly\n");
}

int main() {
    printf("=== TrafficContribution Unit Tests (Epic 7, Ticket E7-012) ===\n\n");

    test_default_config_values();
    test_habitation_contribution();
    test_exchange_contribution();
    test_fabrication_contribution();
    test_level_scaling();
    test_level_scaling_exchange();
    test_invalid_zone_type();
    test_zero_level();
    test_custom_config();
    test_partial_occupancy_with_levels();
    test_boundary_values();

    printf("\n=== All TrafficContribution Tests Passed ===\n");
    return 0;
}
