/**
 * @file test_ordinance.cpp
 * @brief Unit tests for Ordinance framework (E11-021)
 *
 * Tests: configs, enable/disable, total cost, all active, none active,
 * event struct, each ordinance's values, get_ordinance_config.
 */

#include "sims3000/economy/Ordinance.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace sims3000::economy;

// ---------------------------------------------------------------------------
// Helper: float comparison
// ---------------------------------------------------------------------------

static bool float_eq(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

// ---------------------------------------------------------------------------
// Test: Enhanced Patrol config values
// ---------------------------------------------------------------------------

void test_enhanced_patrol_config() {
    printf("Testing Enhanced Patrol config...\n");

    const auto& cfg = get_ordinance_config(OrdinanceType::EnhancedPatrol);
    assert(cfg.type == OrdinanceType::EnhancedPatrol);
    assert(std::strcmp(cfg.name, "Enhanced Patrol") == 0);
    assert(cfg.cost_per_phase == 1000);
    assert(float_eq(cfg.effect_multiplier, 0.10f));

    printf("  PASS: Enhanced Patrol: 1000/phase, 0.10 effect\n");
}

// ---------------------------------------------------------------------------
// Test: Industrial Scrubbers config values
// ---------------------------------------------------------------------------

void test_industrial_scrubbers_config() {
    printf("Testing Industrial Scrubbers config...\n");

    const auto& cfg = get_ordinance_config(OrdinanceType::IndustrialScrubbers);
    assert(cfg.type == OrdinanceType::IndustrialScrubbers);
    assert(std::strcmp(cfg.name, "Industrial Scrubbers") == 0);
    assert(cfg.cost_per_phase == 2000);
    assert(float_eq(cfg.effect_multiplier, 0.15f));

    printf("  PASS: Industrial Scrubbers: 2000/phase, 0.15 effect\n");
}

// ---------------------------------------------------------------------------
// Test: Free Transit config values
// ---------------------------------------------------------------------------

void test_free_transit_config() {
    printf("Testing Free Transit config...\n");

    const auto& cfg = get_ordinance_config(OrdinanceType::FreeTransit);
    assert(cfg.type == OrdinanceType::FreeTransit);
    assert(std::strcmp(cfg.name, "Free Transit") == 0);
    assert(cfg.cost_per_phase == 5000);
    assert(float_eq(cfg.effect_multiplier, 10.0f));

    printf("  PASS: Free Transit: 5000/phase, 10.0 effect\n");
}

// ---------------------------------------------------------------------------
// Test: default state is all inactive
// ---------------------------------------------------------------------------

void test_default_all_inactive() {
    printf("Testing default OrdinanceState has all inactive...\n");

    OrdinanceState state;
    assert(state.is_active(OrdinanceType::EnhancedPatrol) == false);
    assert(state.is_active(OrdinanceType::IndustrialScrubbers) == false);
    assert(state.is_active(OrdinanceType::FreeTransit) == false);

    printf("  PASS: All ordinances inactive by default\n");
}

// ---------------------------------------------------------------------------
// Test: enable single ordinance
// ---------------------------------------------------------------------------

void test_enable_single() {
    printf("Testing enable single ordinance...\n");

    OrdinanceState state;
    state.enable(OrdinanceType::EnhancedPatrol);

    assert(state.is_active(OrdinanceType::EnhancedPatrol) == true);
    assert(state.is_active(OrdinanceType::IndustrialScrubbers) == false);
    assert(state.is_active(OrdinanceType::FreeTransit) == false);

    printf("  PASS: Only Enhanced Patrol active\n");
}

// ---------------------------------------------------------------------------
// Test: disable ordinance
// ---------------------------------------------------------------------------

void test_disable() {
    printf("Testing disable ordinance...\n");

    OrdinanceState state;
    state.enable(OrdinanceType::FreeTransit);
    assert(state.is_active(OrdinanceType::FreeTransit) == true);

    state.disable(OrdinanceType::FreeTransit);
    assert(state.is_active(OrdinanceType::FreeTransit) == false);

    printf("  PASS: Free Transit enabled then disabled\n");
}

// ---------------------------------------------------------------------------
// Test: enable multiple ordinances
// ---------------------------------------------------------------------------

void test_enable_multiple() {
    printf("Testing enable multiple ordinances...\n");

    OrdinanceState state;
    state.enable(OrdinanceType::EnhancedPatrol);
    state.enable(OrdinanceType::IndustrialScrubbers);

    assert(state.is_active(OrdinanceType::EnhancedPatrol) == true);
    assert(state.is_active(OrdinanceType::IndustrialScrubbers) == true);
    assert(state.is_active(OrdinanceType::FreeTransit) == false);

    printf("  PASS: Two ordinances active\n");
}

// ---------------------------------------------------------------------------
// Test: total cost with none active
// ---------------------------------------------------------------------------

void test_total_cost_none_active() {
    printf("Testing total cost with none active...\n");

    OrdinanceState state;
    assert(state.get_total_cost() == 0);

    printf("  PASS: Total cost is 0 with no ordinances\n");
}

// ---------------------------------------------------------------------------
// Test: total cost with single active
// ---------------------------------------------------------------------------

void test_total_cost_single() {
    printf("Testing total cost with single ordinance...\n");

    OrdinanceState state;
    state.enable(OrdinanceType::IndustrialScrubbers);
    assert(state.get_total_cost() == 2000);

    printf("  PASS: Total cost = 2000 (Industrial Scrubbers)\n");
}

// ---------------------------------------------------------------------------
// Test: total cost with all active
// ---------------------------------------------------------------------------

void test_total_cost_all_active() {
    printf("Testing total cost with all ordinances active...\n");

    OrdinanceState state;
    state.enable(OrdinanceType::EnhancedPatrol);
    state.enable(OrdinanceType::IndustrialScrubbers);
    state.enable(OrdinanceType::FreeTransit);

    // 1000 + 2000 + 5000 = 8000
    assert(state.get_total_cost() == 8000);

    printf("  PASS: Total cost = 8000 (all active)\n");
}

// ---------------------------------------------------------------------------
// Test: OrdinanceChangedEvent struct
// ---------------------------------------------------------------------------

void test_event_struct() {
    printf("Testing OrdinanceChangedEvent struct...\n");

    OrdinanceChangedEvent evt;
    evt.player_id = 2;
    evt.type = OrdinanceType::FreeTransit;
    evt.enabled = true;

    assert(evt.player_id == 2);
    assert(evt.type == OrdinanceType::FreeTransit);
    assert(evt.enabled == true);

    printf("  PASS: OrdinanceChangedEvent fields correct\n");
}

// ---------------------------------------------------------------------------
// Test: enable already enabled (idempotent)
// ---------------------------------------------------------------------------

void test_enable_idempotent() {
    printf("Testing enable is idempotent...\n");

    OrdinanceState state;
    state.enable(OrdinanceType::EnhancedPatrol);
    state.enable(OrdinanceType::EnhancedPatrol);

    assert(state.is_active(OrdinanceType::EnhancedPatrol) == true);
    assert(state.get_total_cost() == 1000); // Only counted once

    printf("  PASS: Double-enable does not double cost\n");
}

// ---------------------------------------------------------------------------
// Test: disable already disabled (idempotent)
// ---------------------------------------------------------------------------

void test_disable_idempotent() {
    printf("Testing disable is idempotent...\n");

    OrdinanceState state;
    state.disable(OrdinanceType::FreeTransit);

    assert(state.is_active(OrdinanceType::FreeTransit) == false);
    assert(state.get_total_cost() == 0);

    printf("  PASS: Disable on already-disabled is safe\n");
}

// ---------------------------------------------------------------------------
// Test: ORDINANCE_TYPE_COUNT matches enum
// ---------------------------------------------------------------------------

void test_type_count() {
    printf("Testing ORDINANCE_TYPE_COUNT...\n");

    assert(ORDINANCE_TYPE_COUNT == 3);

    printf("  PASS: ORDINANCE_TYPE_COUNT = 3\n");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    printf("=== Ordinance Unit Tests (E11-021) ===\n\n");

    // Config values
    test_enhanced_patrol_config();
    test_industrial_scrubbers_config();
    test_free_transit_config();

    // State management
    test_default_all_inactive();
    test_enable_single();
    test_disable();
    test_enable_multiple();
    test_enable_idempotent();
    test_disable_idempotent();

    // Total cost
    test_total_cost_none_active();
    test_total_cost_single();
    test_total_cost_all_active();

    // Events and enum
    test_event_struct();
    test_type_count();

    printf("\n=== All Ordinance tests passed! (14 tests) ===\n");
    return 0;
}
