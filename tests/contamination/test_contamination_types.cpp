/**
 * @file test_contamination_types.cpp
 * @brief Unit tests for ContaminationType enum and ContaminationComponent (E10-071, E10-080)
 */

#include "sims3000/contamination/ContaminationType.h"
#include "sims3000/contamination/ContaminationComponent.h"
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace sims3000::contamination;

void test_contamination_type_values() {
    printf("Testing ContaminationType enum values...\n");

    assert(static_cast<uint8_t>(ContaminationType::Industrial) == 0);
    assert(static_cast<uint8_t>(ContaminationType::Traffic) == 1);
    assert(static_cast<uint8_t>(ContaminationType::Energy) == 2);
    assert(static_cast<uint8_t>(ContaminationType::Terrain) == 3);

    printf("  PASS: ContaminationType values are 0-3\n");
}

void test_contamination_type_count() {
    printf("Testing CONTAMINATION_TYPE_COUNT...\n");

    assert(CONTAMINATION_TYPE_COUNT == 4);

    printf("  PASS: CONTAMINATION_TYPE_COUNT is 4\n");
}

void test_contamination_type_to_string() {
    printf("Testing contamination_type_to_string...\n");

    assert(strcmp(contamination_type_to_string(ContaminationType::Industrial), "Industrial") == 0);
    assert(strcmp(contamination_type_to_string(ContaminationType::Traffic), "Traffic") == 0);
    assert(strcmp(contamination_type_to_string(ContaminationType::Energy), "Energy") == 0);
    assert(strcmp(contamination_type_to_string(ContaminationType::Terrain), "Terrain") == 0);

    // Test invalid value returns "Unknown"
    assert(strcmp(contamination_type_to_string(static_cast<ContaminationType>(255)), "Unknown") == 0);

    printf("  PASS: String conversion works for all types\n");
}

void test_contamination_component_size() {
    printf("Testing ContaminationComponent size...\n");

    printf("  ContaminationComponent size: %zu bytes\n", sizeof(ContaminationComponent));
    assert(sizeof(ContaminationComponent) == 16);

    printf("  PASS: ContaminationComponent is exactly 16 bytes\n");
}

void test_contamination_component_defaults() {
    printf("Testing ContaminationComponent default values...\n");

    ContaminationComponent comp;

    assert(comp.base_contamination_output == 0);
    assert(comp.current_contamination_output == 0);
    assert(comp.spread_radius == 4);
    assert(comp.spread_decay_rate == 10);
    assert(comp.contamination_type == ContaminationType::Industrial);
    assert(comp.local_contamination_level == 0);
    assert(comp.is_active_source == false);
    assert(comp.padding[0] == 0);
    assert(comp.padding[1] == 0);
    assert(comp.padding[2] == 0);

    printf("  PASS: ContaminationComponent defaults are correct\n");
}

void test_contamination_component_mutation() {
    printf("Testing ContaminationComponent mutation...\n");

    ContaminationComponent comp;
    comp.base_contamination_output = 1000;
    comp.current_contamination_output = 800;
    comp.spread_radius = 8;
    comp.spread_decay_rate = 20;
    comp.contamination_type = ContaminationType::Energy;
    comp.local_contamination_level = 128;
    comp.is_active_source = true;

    assert(comp.base_contamination_output == 1000);
    assert(comp.current_contamination_output == 800);
    assert(comp.spread_radius == 8);
    assert(comp.spread_decay_rate == 20);
    assert(comp.contamination_type == ContaminationType::Energy);
    assert(comp.local_contamination_level == 128);
    assert(comp.is_active_source == true);

    printf("  PASS: ContaminationComponent mutation works correctly\n");
}

void test_contamination_component_all_types() {
    printf("Testing ContaminationComponent with all contamination types...\n");

    ContaminationComponent comp;

    comp.contamination_type = ContaminationType::Industrial;
    assert(comp.contamination_type == ContaminationType::Industrial);

    comp.contamination_type = ContaminationType::Traffic;
    assert(comp.contamination_type == ContaminationType::Traffic);

    comp.contamination_type = ContaminationType::Energy;
    assert(comp.contamination_type == ContaminationType::Energy);

    comp.contamination_type = ContaminationType::Terrain;
    assert(comp.contamination_type == ContaminationType::Terrain);

    printf("  PASS: All contamination types assignable\n");
}

int main() {
    printf("=== ContaminationType & ContaminationComponent Unit Tests (E10-071, E10-080) ===\n\n");

    test_contamination_type_values();
    test_contamination_type_count();
    test_contamination_type_to_string();
    test_contamination_component_size();
    test_contamination_component_defaults();
    test_contamination_component_mutation();
    test_contamination_component_all_types();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
