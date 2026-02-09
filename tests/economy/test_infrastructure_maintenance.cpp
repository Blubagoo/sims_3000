/**
 * @file test_infrastructure_maintenance.cpp
 * @brief Unit tests for InfrastructureMaintenance (E11-009)
 */

#include "sims3000/economy/InfrastructureMaintenance.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace sims3000::economy;

// ============================================================================
// Maintenance Rate Tests
// ============================================================================

void test_pathway_rate() {
    printf("Testing pathway maintenance rate...\n");
    assert(get_infrastructure_maintenance_rate(InfrastructureType::Pathway) == 5);
    printf("  PASS: Pathway rate is 5\n");
}

void test_energy_conduit_rate() {
    printf("Testing energy conduit maintenance rate...\n");
    assert(get_infrastructure_maintenance_rate(InfrastructureType::EnergyConduit) == 2);
    printf("  PASS: EnergyConduit rate is 2\n");
}

void test_fluid_conduit_rate() {
    printf("Testing fluid conduit maintenance rate...\n");
    assert(get_infrastructure_maintenance_rate(InfrastructureType::FluidConduit) == 3);
    printf("  PASS: FluidConduit rate is 3\n");
}

void test_rail_track_rate() {
    printf("Testing rail track maintenance rate...\n");
    assert(get_infrastructure_maintenance_rate(InfrastructureType::RailTrack) == 8);
    printf("  PASS: RailTrack rate is 8\n");
}

void test_rate_constants_match() {
    printf("Testing rate constants match get_infrastructure_maintenance_rate...\n");
    assert(get_infrastructure_maintenance_rate(InfrastructureType::Pathway) == MAINTENANCE_PATHWAY);
    assert(get_infrastructure_maintenance_rate(InfrastructureType::EnergyConduit) == MAINTENANCE_ENERGY_CONDUIT);
    assert(get_infrastructure_maintenance_rate(InfrastructureType::FluidConduit) == MAINTENANCE_FLUID_CONDUIT);
    assert(get_infrastructure_maintenance_rate(InfrastructureType::RailTrack) == MAINTENANCE_RAIL_TRACK);
    printf("  PASS: All constants match\n");
}

// ============================================================================
// Per-Entity Cost Calculation Tests
// ============================================================================

void test_basic_cost_calculation() {
    printf("Testing basic infrastructure cost calculation...\n");

    InfrastructureMaintenanceInput input;
    input.base_cost = 100;
    input.cost_multiplier = 1.0f;

    int64_t cost = calculate_infrastructure_cost(input);
    assert(cost == 100);

    printf("  PASS: 100 * 1.0 = 100\n");
}

void test_cost_multiplier_effect() {
    printf("Testing cost_multiplier effect...\n");

    InfrastructureMaintenanceInput input;
    input.base_cost = 100;
    input.cost_multiplier = 1.5f;

    int64_t cost = calculate_infrastructure_cost(input);
    assert(cost == 150);

    printf("  PASS: 100 * 1.5 = 150\n");
}

void test_cost_multiplier_less_than_one() {
    printf("Testing cost_multiplier < 1.0...\n");

    InfrastructureMaintenanceInput input;
    input.base_cost = 100;
    input.cost_multiplier = 0.5f;

    int64_t cost = calculate_infrastructure_cost(input);
    assert(cost == 50);

    printf("  PASS: 100 * 0.5 = 50\n");
}

void test_cost_multiplier_fractional() {
    printf("Testing fractional cost_multiplier rounding...\n");

    InfrastructureMaintenanceInput input;
    input.base_cost = 10;
    input.cost_multiplier = 1.25f;

    int64_t cost = calculate_infrastructure_cost(input);
    // 10 * 1.25 = 12.5 -> rounds to 13
    assert(cost == 13);

    printf("  PASS: 10 * 1.25 = 13 (rounded)\n");
}

void test_zero_base_cost() {
    printf("Testing zero base cost...\n");

    InfrastructureMaintenanceInput input;
    input.base_cost = 0;
    input.cost_multiplier = 2.0f;

    int64_t cost = calculate_infrastructure_cost(input);
    assert(cost == 0);

    printf("  PASS: 0 * 2.0 = 0\n");
}

void test_zero_multiplier() {
    printf("Testing zero cost_multiplier...\n");

    InfrastructureMaintenanceInput input;
    input.base_cost = 100;
    input.cost_multiplier = 0.0f;

    int64_t cost = calculate_infrastructure_cost(input);
    assert(cost == 0);

    printf("  PASS: 100 * 0.0 = 0\n");
}

void test_large_base_cost() {
    printf("Testing large base cost...\n");

    InfrastructureMaintenanceInput input;
    input.base_cost = 1000000;
    input.cost_multiplier = 1.0f;

    int64_t cost = calculate_infrastructure_cost(input);
    assert(cost == 1000000);

    printf("  PASS: Large base cost handled correctly\n");
}

void test_negative_base_cost() {
    printf("Testing negative base cost (edge case)...\n");

    InfrastructureMaintenanceInput input;
    input.base_cost = -50;
    input.cost_multiplier = 1.0f;

    int64_t cost = calculate_infrastructure_cost(input);
    assert(cost == -50);

    printf("  PASS: Negative base cost passes through\n");
}

void test_per_tile_cost_with_multiplier() {
    printf("Testing per-tile cost with multiplier (simulated aged pathway)...\n");

    // Simulate: a pathway entity with base_cost = MAINTENANCE_PATHWAY, aged (1.3x)
    InfrastructureMaintenanceInput input;
    input.base_cost = MAINTENANCE_PATHWAY;  // 5
    input.cost_multiplier = 1.3f;

    int64_t cost = calculate_infrastructure_cost(input);
    // 5 * 1.3 = 6.5 -> rounds to 7
    assert(cost == 7);

    printf("  PASS: Pathway 5 * 1.3 = 7 (rounded)\n");
}

// ============================================================================
// Aggregate Function Tests
// ============================================================================

void test_aggregate_empty() {
    printf("Testing aggregate with empty vector...\n");

    std::vector<std::pair<InfrastructureType, int64_t>> costs;
    auto result = aggregate_infrastructure_maintenance(costs);

    assert(result.pathway_cost == 0);
    assert(result.energy_conduit_cost == 0);
    assert(result.fluid_conduit_cost == 0);
    assert(result.rail_track_cost == 0);
    assert(result.total == 0);

    printf("  PASS: Empty aggregate returns all zeros\n");
}

void test_aggregate_single_type() {
    printf("Testing aggregate with single type...\n");

    std::vector<std::pair<InfrastructureType, int64_t>> costs = {
        {InfrastructureType::Pathway, 10},
        {InfrastructureType::Pathway, 20},
        {InfrastructureType::Pathway, 30}
    };

    auto result = aggregate_infrastructure_maintenance(costs);

    assert(result.pathway_cost == 60);
    assert(result.energy_conduit_cost == 0);
    assert(result.fluid_conduit_cost == 0);
    assert(result.rail_track_cost == 0);
    assert(result.total == 60);

    printf("  PASS: Single type aggregation correct\n");
}

void test_aggregate_all_types() {
    printf("Testing aggregate with all types...\n");

    std::vector<std::pair<InfrastructureType, int64_t>> costs = {
        {InfrastructureType::Pathway, 50},
        {InfrastructureType::EnergyConduit, 20},
        {InfrastructureType::FluidConduit, 30},
        {InfrastructureType::RailTrack, 80},
        {InfrastructureType::Pathway, 50},
        {InfrastructureType::EnergyConduit, 20}
    };

    auto result = aggregate_infrastructure_maintenance(costs);

    assert(result.pathway_cost == 100);
    assert(result.energy_conduit_cost == 40);
    assert(result.fluid_conduit_cost == 30);
    assert(result.rail_track_cost == 80);
    assert(result.total == 250);

    printf("  PASS: All-types aggregation correct (total=250)\n");
}

void test_aggregate_single_entry() {
    printf("Testing aggregate with single entry...\n");

    std::vector<std::pair<InfrastructureType, int64_t>> costs = {
        {InfrastructureType::RailTrack, 42}
    };

    auto result = aggregate_infrastructure_maintenance(costs);

    assert(result.pathway_cost == 0);
    assert(result.energy_conduit_cost == 0);
    assert(result.fluid_conduit_cost == 0);
    assert(result.rail_track_cost == 42);
    assert(result.total == 42);

    printf("  PASS: Single entry aggregation correct\n");
}

void test_aggregate_zero_costs() {
    printf("Testing aggregate with zero-cost entries...\n");

    std::vector<std::pair<InfrastructureType, int64_t>> costs = {
        {InfrastructureType::Pathway, 0},
        {InfrastructureType::EnergyConduit, 0}
    };

    auto result = aggregate_infrastructure_maintenance(costs);

    assert(result.pathway_cost == 0);
    assert(result.energy_conduit_cost == 0);
    assert(result.total == 0);

    printf("  PASS: Zero-cost entries handled correctly\n");
}

// ============================================================================
// Infrastructure Type Enum Tests
// ============================================================================

void test_infrastructure_type_values() {
    printf("Testing InfrastructureType enum values...\n");

    assert(static_cast<uint8_t>(InfrastructureType::Pathway) == 0);
    assert(static_cast<uint8_t>(InfrastructureType::EnergyConduit) == 1);
    assert(static_cast<uint8_t>(InfrastructureType::FluidConduit) == 2);
    assert(static_cast<uint8_t>(InfrastructureType::RailTrack) == 3);

    printf("  PASS: InfrastructureType enum values are correct\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Infrastructure Maintenance Unit Tests (E11-009) ===\n\n");

    // Rate tests
    test_pathway_rate();
    test_energy_conduit_rate();
    test_fluid_conduit_rate();
    test_rail_track_rate();
    test_rate_constants_match();

    // Per-entity cost calculation tests
    test_basic_cost_calculation();
    test_cost_multiplier_effect();
    test_cost_multiplier_less_than_one();
    test_cost_multiplier_fractional();
    test_zero_base_cost();
    test_zero_multiplier();
    test_large_base_cost();
    test_negative_base_cost();
    test_per_tile_cost_with_multiplier();

    // Aggregate tests
    test_aggregate_empty();
    test_aggregate_single_type();
    test_aggregate_all_types();
    test_aggregate_single_entry();
    test_aggregate_zero_costs();

    // Enum tests
    test_infrastructure_type_values();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
