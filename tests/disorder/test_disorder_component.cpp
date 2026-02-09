/**
 * @file test_disorder_component.cpp
 * @brief Unit tests for DisorderComponent (E10-070)
 */

#include "sims3000/disorder/DisorderComponent.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::disorder;

void test_disorder_component_size() {
    printf("Testing DisorderComponent size...\n");

    printf("  DisorderComponent size: %zu bytes\n", sizeof(DisorderComponent));
    assert(sizeof(DisorderComponent) == 12);

    printf("  PASS: DisorderComponent is exactly 12 bytes\n");
}

void test_disorder_component_defaults() {
    printf("Testing DisorderComponent default values...\n");

    DisorderComponent comp;

    assert(comp.base_disorder_generation == 0);
    assert(comp.current_disorder_generation == 0);
    assert(comp.suppression_power == 0);
    assert(comp.suppression_radius == 0);
    assert(comp.local_disorder_level == 0);
    assert(comp.is_disorder_source == false);
    assert(comp.is_enforcer == false);
    assert(comp.padding[0] == 0);
    assert(comp.padding[1] == 0);

    printf("  PASS: DisorderComponent defaults are all zero/false\n");
}

void test_disorder_component_source() {
    printf("Testing DisorderComponent as disorder source...\n");

    DisorderComponent comp;
    comp.is_disorder_source = true;
    comp.base_disorder_generation = 100;
    comp.current_disorder_generation = 80;  // reduced by nearby enforcer
    comp.local_disorder_level = 45;

    assert(comp.is_disorder_source == true);
    assert(comp.is_enforcer == false);
    assert(comp.base_disorder_generation == 100);
    assert(comp.current_disorder_generation == 80);
    assert(comp.local_disorder_level == 45);

    printf("  PASS: DisorderComponent works as disorder source\n");
}

void test_disorder_component_enforcer() {
    printf("Testing DisorderComponent as enforcer...\n");

    DisorderComponent comp;
    comp.is_enforcer = true;
    comp.suppression_power = 500;
    comp.suppression_radius = 8;

    assert(comp.is_enforcer == true);
    assert(comp.is_disorder_source == false);
    assert(comp.suppression_power == 500);
    assert(comp.suppression_radius == 8);

    printf("  PASS: DisorderComponent works as enforcer\n");
}

void test_disorder_component_uint16_range() {
    printf("Testing DisorderComponent uint16_t ranges...\n");

    DisorderComponent comp;
    comp.base_disorder_generation = 65535;
    comp.current_disorder_generation = 65535;
    comp.suppression_power = 65535;

    assert(comp.base_disorder_generation == 65535);
    assert(comp.current_disorder_generation == 65535);
    assert(comp.suppression_power == 65535);

    printf("  PASS: uint16_t fields handle full range\n");
}

int main() {
    printf("=== DisorderComponent Unit Tests (E10-070) ===\n\n");

    test_disorder_component_size();
    test_disorder_component_defaults();
    test_disorder_component_source();
    test_disorder_component_enforcer();
    test_disorder_component_uint16_range();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
