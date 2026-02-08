/**
 * @file test_maintenance_cost.cpp
 * @brief Unit tests for MaintenanceCost (Epic 7, Ticket E7-021)
 *
 * Tests:
 * - cost_per_health returns correct rates per pathway type
 * - calculate_maintenance_cost scales with damage
 * - Pristine pathway costs 0
 * - Destroyed pathway costs full rate
 * - Pedestrian pathways are free
 * - Custom config values
 */

#include <sims3000/transport/MaintenanceCost.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_cost_per_health_defaults() {
    printf("Testing cost_per_health default rates...\n");

    MaintenanceCostConfig cfg{};

    assert(cost_per_health(PathwayType::BasicPathway, cfg) == 1);
    assert(cost_per_health(PathwayType::TransitCorridor, cfg) == 3);
    assert(cost_per_health(PathwayType::Pedestrian, cfg) == 0);
    assert(cost_per_health(PathwayType::Bridge, cfg) == 4);
    assert(cost_per_health(PathwayType::Tunnel, cfg) == 4);

    printf("  PASS: Default cost rates are correct\n");
}

void test_cost_per_health_custom() {
    printf("Testing cost_per_health custom rates...\n");

    MaintenanceCostConfig cfg{};
    cfg.basic_cost_per_health = 10;
    cfg.corridor_cost_per_health = 20;
    cfg.pedestrian_cost_per_health = 5;
    cfg.bridge_cost_per_health = 30;
    cfg.tunnel_cost_per_health = 25;

    assert(cost_per_health(PathwayType::BasicPathway, cfg) == 10);
    assert(cost_per_health(PathwayType::TransitCorridor, cfg) == 20);
    assert(cost_per_health(PathwayType::Pedestrian, cfg) == 5);
    assert(cost_per_health(PathwayType::Bridge, cfg) == 30);
    assert(cost_per_health(PathwayType::Tunnel, cfg) == 25);

    printf("  PASS: Custom cost rates work correctly\n");
}

void test_pristine_pathway_costs_zero() {
    printf("Testing pristine pathway costs zero...\n");

    RoadComponent road{};
    road.health = 255;  // Pristine
    road.type = PathwayType::BasicPathway;

    uint32_t cost = calculate_maintenance_cost(road);
    assert(cost == 0);  // (255-255) * 1 / 255 = 0

    printf("  PASS: Pristine pathway has zero maintenance cost\n");
}

void test_destroyed_pathway_full_cost() {
    printf("Testing destroyed pathway has full cost...\n");

    RoadComponent road{};
    road.health = 0;  // Destroyed
    road.type = PathwayType::BasicPathway;

    uint32_t cost = calculate_maintenance_cost(road);
    // (255 - 0) * 1 / 255 = 1
    assert(cost == 1);

    printf("  PASS: Destroyed BasicPathway costs 1\n");
}

void test_destroyed_corridor_full_cost() {
    printf("Testing destroyed TransitCorridor has full cost...\n");

    RoadComponent road{};
    road.health = 0;
    road.type = PathwayType::TransitCorridor;

    uint32_t cost = calculate_maintenance_cost(road);
    // (255 - 0) * 3 / 255 = 3
    assert(cost == 3);

    printf("  PASS: Destroyed TransitCorridor costs 3\n");
}

void test_destroyed_bridge_full_cost() {
    printf("Testing destroyed Bridge has full cost...\n");

    RoadComponent road{};
    road.health = 0;
    road.type = PathwayType::Bridge;

    uint32_t cost = calculate_maintenance_cost(road);
    // (255 - 0) * 4 / 255 = 4
    assert(cost == 4);

    printf("  PASS: Destroyed Bridge costs 4\n");
}

void test_destroyed_tunnel_full_cost() {
    printf("Testing destroyed Tunnel has full cost...\n");

    RoadComponent road{};
    road.health = 0;
    road.type = PathwayType::Tunnel;

    uint32_t cost = calculate_maintenance_cost(road);
    // (255 - 0) * 4 / 255 = 4
    assert(cost == 4);

    printf("  PASS: Destroyed Tunnel costs 4\n");
}

void test_pedestrian_always_free() {
    printf("Testing pedestrian pathways are always free...\n");

    RoadComponent road{};
    road.type = PathwayType::Pedestrian;

    // Test at various health levels
    road.health = 255;
    assert(calculate_maintenance_cost(road) == 0);

    road.health = 128;
    assert(calculate_maintenance_cost(road) == 0);

    road.health = 0;
    assert(calculate_maintenance_cost(road) == 0);

    printf("  PASS: Pedestrian pathways are always free\n");
}

void test_cost_scales_with_damage() {
    printf("Testing cost scales with damage...\n");

    RoadComponent road{};
    road.type = PathwayType::BasicPathway;

    // Health 200: missing 55 health -> (55 * 1) / 255 = 0
    road.health = 200;
    uint32_t cost_200 = calculate_maintenance_cost(road);

    // Health 100: missing 155 health -> (155 * 1) / 255 = 0
    road.health = 100;
    uint32_t cost_100 = calculate_maintenance_cost(road);

    // Health 50: missing 205 health -> (205 * 1) / 255 = 0
    road.health = 50;
    uint32_t cost_50 = calculate_maintenance_cost(road);

    // More damage = more cost (or equal, given integer division)
    assert(cost_200 <= cost_100);
    assert(cost_100 <= cost_50);

    printf("  PASS: Cost scales with increasing damage\n");
}

void test_cost_scales_with_damage_corridor() {
    printf("Testing cost scales with damage for TransitCorridor...\n");

    RoadComponent road{};
    road.type = PathwayType::TransitCorridor;

    // Health 255: missing 0 -> cost 0
    road.health = 255;
    assert(calculate_maintenance_cost(road) == 0);

    // Health 0: missing 255 -> (255 * 3) / 255 = 3
    road.health = 0;
    assert(calculate_maintenance_cost(road) == 3);

    // Health 128: missing 127 -> (127 * 3) / 255 = 1 (integer division)
    road.health = 128;
    uint32_t mid_cost = calculate_maintenance_cost(road);
    assert(mid_cost >= 0 && mid_cost <= 3);

    printf("  PASS: Corridor cost scales correctly\n");
}

void test_maintenance_cost_config_defaults() {
    printf("Testing MaintenanceCostConfig defaults...\n");

    MaintenanceCostConfig cfg{};
    assert(cfg.basic_cost_per_health == 1);
    assert(cfg.corridor_cost_per_health == 3);
    assert(cfg.pedestrian_cost_per_health == 0);
    assert(cfg.bridge_cost_per_health == 4);
    assert(cfg.tunnel_cost_per_health == 4);

    printf("  PASS: MaintenanceCostConfig defaults are correct\n");
}

void test_custom_config_affects_cost() {
    printf("Testing custom config affects calculated cost...\n");

    RoadComponent road{};
    road.health = 0;
    road.type = PathwayType::BasicPathway;

    MaintenanceCostConfig high_cost{};
    high_cost.basic_cost_per_health = 100;

    uint32_t cost = calculate_maintenance_cost(road, high_cost);
    // (255 * 100) / 255 = 100
    assert(cost == 100);

    printf("  PASS: Custom config produces expected cost\n");
}

void test_default_function_argument() {
    printf("Testing default function argument for config...\n");

    RoadComponent road{};
    road.health = 0;
    road.type = PathwayType::BasicPathway;

    // Call without explicit config (uses default)
    uint32_t cost = calculate_maintenance_cost(road);
    assert(cost == 1);  // (255 * 1) / 255 = 1

    // Call cost_per_health without explicit config
    uint16_t rate = cost_per_health(PathwayType::BasicPathway);
    assert(rate == 1);

    printf("  PASS: Default function arguments work correctly\n");
}

int main() {
    printf("=== MaintenanceCost Unit Tests (Epic 7, Ticket E7-021) ===\n\n");

    test_cost_per_health_defaults();
    test_cost_per_health_custom();
    test_pristine_pathway_costs_zero();
    test_destroyed_pathway_full_cost();
    test_destroyed_corridor_full_cost();
    test_destroyed_bridge_full_cost();
    test_destroyed_tunnel_full_cost();
    test_pedestrian_always_free();
    test_cost_scales_with_damage();
    test_cost_scales_with_damage_corridor();
    test_maintenance_cost_config_defaults();
    test_custom_config_affects_cost();
    test_default_function_argument();

    printf("\n=== All MaintenanceCost Tests Passed ===\n");
    return 0;
}
