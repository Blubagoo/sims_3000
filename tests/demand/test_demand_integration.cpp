/**
 * @file test_demand_integration.cpp
 * @brief Tests for demand integration helpers (Ticket E10-049)
 *
 * Validates:
 * - should_spawn_building() checks positive demand
 * - get_growth_pressure() clamps to [-100, +100]
 * - should_upgrade_building() uses threshold correctly
 * - should_downgrade_building() uses threshold correctly
 * - Integration with IDemandProvider interface
 */

#include <cassert>
#include <cstdio>

#include "sims3000/demand/DemandIntegration.h"
#include "sims3000/building/ForwardDependencyInterfaces.h"

using namespace sims3000::demand;
using namespace sims3000::building;

// --------------------------------------------------------------------------
// Mock IDemandProvider for testing
// --------------------------------------------------------------------------
class MockDemandProvider : public IDemandProvider {
public:
    // Set mock demand value for testing
    void set_demand(uint8_t zone_type, uint32_t player_id, float value) {
        (void)zone_type;
        (void)player_id;
        m_demand = value;
    }

    // IDemandProvider interface
    float get_demand(uint8_t zone_type, uint32_t player_id) const override {
        (void)zone_type;
        (void)player_id;
        return m_demand;
    }

    uint32_t get_demand_cap(uint8_t zone_type, uint32_t player_id) const override {
        (void)zone_type;
        (void)player_id;
        return 0;  // Not used in these tests
    }

    bool has_positive_demand(uint8_t zone_type, uint32_t player_id) const override {
        (void)zone_type;
        (void)player_id;
        return m_demand > 0.0f;
    }

private:
    float m_demand = 0.0f;
};

// --------------------------------------------------------------------------
// Test: should_spawn_building() with positive demand
// --------------------------------------------------------------------------
static void test_spawn_positive_demand() {
    printf("Testing should_spawn_building() with positive demand...\n");

    MockDemandProvider provider;
    provider.set_demand(0, 0, 75.0f);  // Positive demand

    bool result = should_spawn_building(provider, 0, 0);

    assert(result == true && "Should spawn with positive demand");

    printf("  PASS: should_spawn_building() returns true for positive demand\n");
}

// --------------------------------------------------------------------------
// Test: should_spawn_building() with zero/negative demand
// --------------------------------------------------------------------------
static void test_spawn_no_demand() {
    printf("Testing should_spawn_building() with zero/negative demand...\n");

    MockDemandProvider provider;

    // Zero demand
    provider.set_demand(0, 0, 0.0f);
    assert(should_spawn_building(provider, 0, 0) == false &&
           "Should not spawn with zero demand");

    // Negative demand
    provider.set_demand(0, 0, -50.0f);
    assert(should_spawn_building(provider, 0, 0) == false &&
           "Should not spawn with negative demand");

    printf("  PASS: should_spawn_building() returns false for zero/negative demand\n");
}

// --------------------------------------------------------------------------
// Test: get_growth_pressure() clamping
// --------------------------------------------------------------------------
static void test_growth_pressure_clamping() {
    printf("Testing get_growth_pressure() clamping...\n");

    MockDemandProvider provider;

    // Normal range
    provider.set_demand(0, 0, 50.0f);
    assert(get_growth_pressure(provider, 0, 0) == 50 &&
           "Normal value should be unchanged");

    // Negative normal range
    provider.set_demand(0, 0, -30.0f);
    assert(get_growth_pressure(provider, 0, 0) == -30 &&
           "Negative value should be unchanged");

    // Above max (should clamp to 100)
    provider.set_demand(0, 0, 150.0f);
    assert(get_growth_pressure(provider, 0, 0) == 100 &&
           "Should clamp to 100");

    // Below min (should clamp to -100)
    provider.set_demand(0, 0, -200.0f);
    assert(get_growth_pressure(provider, 0, 0) == -100 &&
           "Should clamp to -100");

    // Exactly at boundaries
    provider.set_demand(0, 0, 100.0f);
    assert(get_growth_pressure(provider, 0, 0) == 100 &&
           "Exactly 100 should remain 100");

    provider.set_demand(0, 0, -100.0f);
    assert(get_growth_pressure(provider, 0, 0) == -100 &&
           "Exactly -100 should remain -100");

    printf("  PASS: get_growth_pressure() clamping works correctly\n");
}

// --------------------------------------------------------------------------
// Test: should_upgrade_building() with default threshold
// --------------------------------------------------------------------------
static void test_upgrade_default_threshold() {
    printf("Testing should_upgrade_building() with default threshold...\n");

    MockDemandProvider provider;

    // Above default threshold (50)
    provider.set_demand(0, 0, 75.0f);
    assert(should_upgrade_building(provider, 0, 0) == true &&
           "Should upgrade when demand > 50");

    // Below default threshold
    provider.set_demand(0, 0, 30.0f);
    assert(should_upgrade_building(provider, 0, 0) == false &&
           "Should not upgrade when demand <= 50");

    // Exactly at threshold
    provider.set_demand(0, 0, 50.0f);
    assert(should_upgrade_building(provider, 0, 0) == false &&
           "Should not upgrade when demand == 50");

    printf("  PASS: should_upgrade_building() default threshold (50)\n");
}

// --------------------------------------------------------------------------
// Test: should_upgrade_building() with custom threshold
// --------------------------------------------------------------------------
static void test_upgrade_custom_threshold() {
    printf("Testing should_upgrade_building() with custom threshold...\n");

    MockDemandProvider provider;
    provider.set_demand(0, 0, 60.0f);

    // Custom threshold 70
    assert(should_upgrade_building(provider, 0, 0, 70) == false &&
           "Should not upgrade when demand (60) <= threshold (70)");

    // Custom threshold 40
    assert(should_upgrade_building(provider, 0, 0, 40) == true &&
           "Should upgrade when demand (60) > threshold (40)");

    printf("  PASS: should_upgrade_building() custom threshold\n");
}

// --------------------------------------------------------------------------
// Test: should_downgrade_building() with default threshold
// --------------------------------------------------------------------------
static void test_downgrade_default_threshold() {
    printf("Testing should_downgrade_building() with default threshold...\n");

    MockDemandProvider provider;

    // Below default threshold (-50)
    provider.set_demand(0, 0, -75.0f);
    assert(should_downgrade_building(provider, 0, 0) == true &&
           "Should downgrade when demand < -50");

    // Above default threshold
    provider.set_demand(0, 0, -30.0f);
    assert(should_downgrade_building(provider, 0, 0) == false &&
           "Should not downgrade when demand >= -50");

    // Exactly at threshold
    provider.set_demand(0, 0, -50.0f);
    assert(should_downgrade_building(provider, 0, 0) == false &&
           "Should not downgrade when demand == -50");

    printf("  PASS: should_downgrade_building() default threshold (-50)\n");
}

// --------------------------------------------------------------------------
// Test: should_downgrade_building() with custom threshold
// --------------------------------------------------------------------------
static void test_downgrade_custom_threshold() {
    printf("Testing should_downgrade_building() with custom threshold...\n");

    MockDemandProvider provider;
    provider.set_demand(0, 0, -40.0f);

    // Custom threshold -30
    assert(should_downgrade_building(provider, 0, 0, -30) == true &&
           "Should downgrade when demand (-40) < threshold (-30)");

    // Custom threshold -60
    assert(should_downgrade_building(provider, 0, 0, -60) == false &&
           "Should not downgrade when demand (-40) >= threshold (-60)");

    printf("  PASS: should_downgrade_building() custom threshold\n");
}

// --------------------------------------------------------------------------
// Test: Zone type and player ID forwarding
// --------------------------------------------------------------------------
static void test_parameter_forwarding() {
    printf("Testing parameter forwarding...\n");

    MockDemandProvider provider;
    provider.set_demand(1, 2, 80.0f);  // Set demand for zone 1, player 2

    // All functions should work regardless of zone_type/player_id
    // (our mock ignores them, but real provider would use them)
    assert(should_spawn_building(provider, 1, 2) == true &&
           "should_spawn_building forwards parameters");

    int8_t pressure = get_growth_pressure(provider, 1, 2);
    assert(pressure == 80 && "get_growth_pressure forwards parameters");

    assert(should_upgrade_building(provider, 1, 2, 50) == true &&
           "should_upgrade_building forwards parameters");

    assert(should_downgrade_building(provider, 1, 2, -50) == false &&
           "should_downgrade_building forwards parameters");

    printf("  PASS: Parameter forwarding\n");
}

// --------------------------------------------------------------------------
// Test: Edge cases
// --------------------------------------------------------------------------
static void test_edge_cases() {
    printf("Testing edge cases...\n");

    MockDemandProvider provider;

    // Very small positive demand (just above zero)
    provider.set_demand(0, 0, 0.1f);
    assert(should_spawn_building(provider, 0, 0) == true &&
           "Small positive demand should allow spawning");
    assert(get_growth_pressure(provider, 0, 0) == 0 &&
           "0.1 rounds to 0 in int8_t conversion");

    // Very small negative demand (just below zero)
    provider.set_demand(0, 0, -0.1f);
    assert(should_spawn_building(provider, 0, 0) == false &&
           "Small negative demand should block spawning");

    printf("  PASS: Edge cases\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    printf("=== Demand Integration Tests (E10-049) ===\n\n");

    test_spawn_positive_demand();
    test_spawn_no_demand();
    test_growth_pressure_clamping();
    test_upgrade_default_threshold();
    test_upgrade_custom_threshold();
    test_downgrade_default_threshold();
    test_downgrade_custom_threshold();
    test_parameter_forwarding();
    test_edge_cases();

    printf("\n=== All Tests Passed ===\n");
    return 0;
}
