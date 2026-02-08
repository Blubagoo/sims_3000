/**
 * @file test_decay_config.cpp
 * @brief Unit tests for DecayConfig (Epic 7, Ticket E7-046)
 */

#include <sims3000/transport/DecayConfig.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_default_thresholds() {
    printf("Testing default decay thresholds...\n");

    DecayThresholds t{};
    assert(t.pristine_min == 200);
    assert(t.good_min == 150);
    assert(t.worn_min == 100);
    assert(t.poor_min == 50);

    printf("  PASS: Default thresholds are correct\n");
}

void test_pristine_state() {
    printf("Testing Pristine health state...\n");

    assert(get_health_state(255) == PathwayHealthState::Pristine);
    assert(get_health_state(200) == PathwayHealthState::Pristine);

    printf("  PASS: 200-255 maps to Pristine\n");
}

void test_good_state() {
    printf("Testing Good health state...\n");

    assert(get_health_state(199) == PathwayHealthState::Good);
    assert(get_health_state(150) == PathwayHealthState::Good);

    printf("  PASS: 150-199 maps to Good\n");
}

void test_worn_state() {
    printf("Testing Worn health state...\n");

    assert(get_health_state(149) == PathwayHealthState::Worn);
    assert(get_health_state(100) == PathwayHealthState::Worn);

    printf("  PASS: 100-149 maps to Worn\n");
}

void test_poor_state() {
    printf("Testing Poor health state...\n");

    assert(get_health_state(99) == PathwayHealthState::Poor);
    assert(get_health_state(50) == PathwayHealthState::Poor);

    printf("  PASS: 50-99 maps to Poor\n");
}

void test_crumbling_state() {
    printf("Testing Crumbling health state...\n");

    assert(get_health_state(49) == PathwayHealthState::Crumbling);
    assert(get_health_state(0) == PathwayHealthState::Crumbling);

    printf("  PASS: 0-49 maps to Crumbling\n");
}

void test_boundary_values() {
    printf("Testing boundary values...\n");

    // Exact boundary between Pristine and Good
    assert(get_health_state(200) == PathwayHealthState::Pristine);
    assert(get_health_state(199) == PathwayHealthState::Good);

    // Exact boundary between Good and Worn
    assert(get_health_state(150) == PathwayHealthState::Good);
    assert(get_health_state(149) == PathwayHealthState::Worn);

    // Exact boundary between Worn and Poor
    assert(get_health_state(100) == PathwayHealthState::Worn);
    assert(get_health_state(99) == PathwayHealthState::Poor);

    // Exact boundary between Poor and Crumbling
    assert(get_health_state(50) == PathwayHealthState::Poor);
    assert(get_health_state(49) == PathwayHealthState::Crumbling);

    printf("  PASS: All boundary values are correct\n");
}

void test_custom_thresholds() {
    printf("Testing custom thresholds...\n");

    DecayThresholds custom{};
    custom.pristine_min = 240;
    custom.good_min = 180;
    custom.worn_min = 120;
    custom.poor_min = 60;

    assert(get_health_state(240, custom) == PathwayHealthState::Pristine);
    assert(get_health_state(239, custom) == PathwayHealthState::Good);
    assert(get_health_state(180, custom) == PathwayHealthState::Good);
    assert(get_health_state(179, custom) == PathwayHealthState::Worn);
    assert(get_health_state(120, custom) == PathwayHealthState::Worn);
    assert(get_health_state(119, custom) == PathwayHealthState::Poor);
    assert(get_health_state(60, custom) == PathwayHealthState::Poor);
    assert(get_health_state(59, custom) == PathwayHealthState::Crumbling);

    printf("  PASS: Custom thresholds work correctly\n");
}

void test_health_state_enum_values() {
    printf("Testing PathwayHealthState enum values...\n");

    assert(static_cast<uint8_t>(PathwayHealthState::Pristine) == 0);
    assert(static_cast<uint8_t>(PathwayHealthState::Good) == 1);
    assert(static_cast<uint8_t>(PathwayHealthState::Worn) == 2);
    assert(static_cast<uint8_t>(PathwayHealthState::Poor) == 3);
    assert(static_cast<uint8_t>(PathwayHealthState::Crumbling) == 4);

    printf("  PASS: PathwayHealthState enum values are correct\n");
}

void test_decay_config_defaults() {
    printf("Testing DecayConfig default values...\n");

    DecayConfig cfg{};
    assert(cfg.base_decay_per_cycle == 1);
    assert(cfg.decay_cycle_ticks == 100);
    assert(cfg.max_traffic_multiplier == 3);

    printf("  PASS: DecayConfig defaults are correct\n");
}

void test_decay_config_custom() {
    printf("Testing DecayConfig custom values...\n");

    DecayConfig cfg{};
    cfg.base_decay_per_cycle = 5;
    cfg.decay_cycle_ticks = 200;
    cfg.max_traffic_multiplier = 10;

    assert(cfg.base_decay_per_cycle == 5);
    assert(cfg.decay_cycle_ticks == 200);
    assert(cfg.max_traffic_multiplier == 10);

    printf("  PASS: DecayConfig custom values work correctly\n");
}

int main() {
    printf("=== DecayConfig Unit Tests (Epic 7, Ticket E7-046) ===\n\n");

    test_default_thresholds();
    test_pristine_state();
    test_good_state();
    test_worn_state();
    test_poor_state();
    test_crumbling_state();
    test_boundary_values();
    test_custom_thresholds();
    test_health_state_enum_values();
    test_decay_config_defaults();
    test_decay_config_custom();

    printf("\n=== All DecayConfig Tests Passed ===\n");
    return 0;
}
