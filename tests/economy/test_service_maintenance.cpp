/**
 * @file test_service_maintenance.cpp
 * @brief Unit tests for ServiceMaintenance (E11-010)
 */

#include "sims3000/economy/ServiceMaintenance.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace sims3000::economy;

// ============================================================================
// Base Cost Tests
// ============================================================================

void test_enforcer_base_cost() {
    printf("Testing enforcer base cost...\n");
    assert(get_service_base_cost(0) == 100);
    assert(get_service_base_cost(0) == SERVICE_COST_ENFORCER);
    printf("  PASS: Enforcer base cost is 100\n");
}

void test_hazard_response_base_cost() {
    printf("Testing hazard response base cost...\n");
    assert(get_service_base_cost(1) == 120);
    assert(get_service_base_cost(1) == SERVICE_COST_HAZARD_RESPONSE);
    printf("  PASS: HazardResponse base cost is 120\n");
}

void test_medical_base_cost() {
    printf("Testing medical base cost...\n");
    assert(get_service_base_cost(2) == 300);
    assert(get_service_base_cost(2) == SERVICE_COST_MEDICAL);
    printf("  PASS: Medical base cost is 300\n");
}

void test_education_base_cost() {
    printf("Testing education base cost...\n");
    assert(get_service_base_cost(3) == 200);
    assert(get_service_base_cost(3) == SERVICE_COST_EDUCATION);
    printf("  PASS: Education base cost is 200\n");
}

void test_unknown_service_type_base_cost() {
    printf("Testing unknown service type base cost...\n");
    assert(get_service_base_cost(4) == 0);
    assert(get_service_base_cost(255) == 0);
    printf("  PASS: Unknown service types return 0\n");
}

// ============================================================================
// Funding Scaling Tests
// ============================================================================

void test_funding_100_percent() {
    printf("Testing 100%% funding (default)...\n");

    ServiceMaintenanceInput input;
    input.service_type = 0; // Enforcer
    input.base_cost = 100;
    input.funding_level = 100;

    auto result = calculate_service_maintenance(input);

    assert(result.actual_cost == 100);
    assert(std::fabs(result.funding_factor - 1.0f) < 0.001f);

    printf("  PASS: 100 * 100%% = 100\n");
}

void test_funding_50_percent() {
    printf("Testing 50%% funding...\n");

    ServiceMaintenanceInput input;
    input.service_type = 0;
    input.base_cost = 100;
    input.funding_level = 50;

    auto result = calculate_service_maintenance(input);

    assert(result.actual_cost == 50);
    assert(std::fabs(result.funding_factor - 0.5f) < 0.001f);

    printf("  PASS: 100 * 50%% = 50\n");
}

void test_funding_150_percent() {
    printf("Testing 150%% funding...\n");

    ServiceMaintenanceInput input;
    input.service_type = 0;
    input.base_cost = 100;
    input.funding_level = 150;

    auto result = calculate_service_maintenance(input);

    assert(result.actual_cost == 150);
    assert(std::fabs(result.funding_factor - 1.5f) < 0.001f);

    printf("  PASS: 100 * 150%% = 150\n");
}

void test_funding_0_percent() {
    printf("Testing 0%% funding...\n");

    ServiceMaintenanceInput input;
    input.service_type = 0;
    input.base_cost = 100;
    input.funding_level = 0;

    auto result = calculate_service_maintenance(input);

    assert(result.actual_cost == 0);
    assert(std::fabs(result.funding_factor - 0.0f) < 0.001f);

    printf("  PASS: 100 * 0%% = 0\n");
}

void test_funding_125_percent() {
    printf("Testing 125%% funding...\n");

    ServiceMaintenanceInput input;
    input.service_type = 2; // Medical
    input.base_cost = 300;
    input.funding_level = 125;

    auto result = calculate_service_maintenance(input);

    assert(result.actual_cost == 375);
    assert(std::fabs(result.funding_factor - 1.25f) < 0.001f);

    printf("  PASS: 300 * 125%% = 375\n");
}

void test_funding_with_odd_percentage() {
    printf("Testing odd funding percentage (73%%)...\n");

    ServiceMaintenanceInput input;
    input.service_type = 1; // HazardResponse
    input.base_cost = 120;
    input.funding_level = 73;

    auto result = calculate_service_maintenance(input);

    // 120 * 0.73 = 87.6 -> rounds to 88
    assert(result.actual_cost == 88);
    assert(std::fabs(result.funding_factor - 0.73f) < 0.001f);

    printf("  PASS: 120 * 73%% = 88 (rounded)\n");
}

void test_zero_base_cost_with_funding() {
    printf("Testing zero base cost with funding...\n");

    ServiceMaintenanceInput input;
    input.service_type = 0;
    input.base_cost = 0;
    input.funding_level = 100;

    auto result = calculate_service_maintenance(input);

    assert(result.actual_cost == 0);

    printf("  PASS: 0 * 100%% = 0\n");
}

// ============================================================================
// Funding Scaling With Real Service Costs
// ============================================================================

void test_all_services_at_default_funding() {
    printf("Testing all service types at 100%% funding...\n");

    for (uint8_t type = 0; type < 4; type++) {
        ServiceMaintenanceInput input;
        input.service_type = type;
        input.base_cost = get_service_base_cost(type);
        input.funding_level = 100;

        auto result = calculate_service_maintenance(input);
        assert(result.actual_cost == static_cast<int64_t>(get_service_base_cost(type)));
    }

    printf("  PASS: All service types at 100%% funding match base costs\n");
}

void test_all_services_at_half_funding() {
    printf("Testing all service types at 50%% funding...\n");

    // Enforcer: 100 * 0.5 = 50
    ServiceMaintenanceInput e;
    e.service_type = 0; e.base_cost = 100; e.funding_level = 50;
    assert(calculate_service_maintenance(e).actual_cost == 50);

    // HazardResponse: 120 * 0.5 = 60
    ServiceMaintenanceInput h;
    h.service_type = 1; h.base_cost = 120; h.funding_level = 50;
    assert(calculate_service_maintenance(h).actual_cost == 60);

    // Medical: 300 * 0.5 = 150
    ServiceMaintenanceInput m;
    m.service_type = 2; m.base_cost = 300; m.funding_level = 50;
    assert(calculate_service_maintenance(m).actual_cost == 150);

    // Education: 200 * 0.5 = 100
    ServiceMaintenanceInput ed;
    ed.service_type = 3; ed.base_cost = 200; ed.funding_level = 50;
    assert(calculate_service_maintenance(ed).actual_cost == 100);

    printf("  PASS: All service types at 50%% funding are correct\n");
}

// ============================================================================
// Aggregate Function Tests
// ============================================================================

void test_aggregate_empty() {
    printf("Testing aggregate with empty vector...\n");

    std::vector<std::pair<uint8_t, int64_t>> costs;
    auto summary = aggregate_service_maintenance(costs);

    assert(summary.enforcer_cost == 0);
    assert(summary.hazard_response_cost == 0);
    assert(summary.medical_cost == 0);
    assert(summary.education_cost == 0);
    assert(summary.total == 0);

    printf("  PASS: Empty aggregate returns all zeros\n");
}

void test_aggregate_single_type() {
    printf("Testing aggregate with single type...\n");

    std::vector<std::pair<uint8_t, int64_t>> costs = {
        {0, 100},  // Enforcer
        {0, 100},  // Enforcer
        {0, 100}   // Enforcer
    };

    auto summary = aggregate_service_maintenance(costs);

    assert(summary.enforcer_cost == 300);
    assert(summary.hazard_response_cost == 0);
    assert(summary.medical_cost == 0);
    assert(summary.education_cost == 0);
    assert(summary.total == 300);

    printf("  PASS: Single type aggregation correct\n");
}

void test_aggregate_all_types() {
    printf("Testing aggregate with all types...\n");

    std::vector<std::pair<uint8_t, int64_t>> costs = {
        {0, 100},  // Enforcer
        {1, 120},  // HazardResponse
        {2, 300},  // Medical
        {3, 200},  // Education
        {0, 100},  // Enforcer (second building)
        {2, 300}   // Medical (second building)
    };

    auto summary = aggregate_service_maintenance(costs);

    assert(summary.enforcer_cost == 200);
    assert(summary.hazard_response_cost == 120);
    assert(summary.medical_cost == 600);
    assert(summary.education_cost == 200);
    assert(summary.total == 1120);

    printf("  PASS: All-types aggregation correct (total=1120)\n");
}

void test_aggregate_unknown_type_ignored() {
    printf("Testing aggregate ignores unknown types...\n");

    std::vector<std::pair<uint8_t, int64_t>> costs = {
        {0, 100},  // Enforcer
        {99, 999}, // Unknown (should be ignored)
        {3, 200}   // Education
    };

    auto summary = aggregate_service_maintenance(costs);

    assert(summary.enforcer_cost == 100);
    assert(summary.education_cost == 200);
    assert(summary.total == 300);

    printf("  PASS: Unknown types are ignored in aggregation\n");
}

void test_aggregate_zero_costs() {
    printf("Testing aggregate with zero-cost entries...\n");

    std::vector<std::pair<uint8_t, int64_t>> costs = {
        {0, 0},
        {1, 0},
        {2, 0},
        {3, 0}
    };

    auto summary = aggregate_service_maintenance(costs);

    assert(summary.enforcer_cost == 0);
    assert(summary.hazard_response_cost == 0);
    assert(summary.medical_cost == 0);
    assert(summary.education_cost == 0);
    assert(summary.total == 0);

    printf("  PASS: Zero-cost entries handled correctly\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Service Maintenance Unit Tests (E11-010) ===\n\n");

    // Base cost tests
    test_enforcer_base_cost();
    test_hazard_response_base_cost();
    test_medical_base_cost();
    test_education_base_cost();
    test_unknown_service_type_base_cost();

    // Funding scaling tests
    test_funding_100_percent();
    test_funding_50_percent();
    test_funding_150_percent();
    test_funding_0_percent();
    test_funding_125_percent();
    test_funding_with_odd_percentage();
    test_zero_base_cost_with_funding();

    // Real service cost scaling
    test_all_services_at_default_funding();
    test_all_services_at_half_funding();

    // Aggregate tests
    test_aggregate_empty();
    test_aggregate_single_type();
    test_aggregate_all_types();
    test_aggregate_unknown_type_ignored();
    test_aggregate_zero_costs();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
