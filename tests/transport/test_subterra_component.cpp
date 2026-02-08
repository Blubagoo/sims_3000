/**
 * @file test_subterra_component.cpp
 * @brief Unit tests for SubterraComponent (Epic 7, Ticket E7-043)
 */

#include <sims3000/transport/SubterraComponent.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_subterra_component_size() {
    printf("Testing SubterraComponent size...\n");

    assert(sizeof(SubterraComponent) == 8);

    printf("  PASS: SubterraComponent is 8 bytes\n");
}

void test_subterra_trivially_copyable() {
    printf("Testing SubterraComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<SubterraComponent>::value);

    printf("  PASS: SubterraComponent is trivially copyable\n");
}

void test_subterra_default_initialization() {
    printf("Testing default initialization...\n");

    SubterraComponent subterra{};
    assert(subterra.depth_level == 1);
    assert(subterra.is_excavated == true);
    assert(subterra.ventilation_radius == 2);
    assert(subterra.has_surface_access == false);

    printf("  PASS: Default initialization works correctly\n");
}

void test_subterra_custom_values() {
    printf("Testing custom value assignment...\n");

    SubterraComponent subterra{};
    subterra.depth_level = 3;
    subterra.is_excavated = false;
    subterra.ventilation_radius = 5;
    subterra.has_surface_access = true;

    assert(subterra.depth_level == 3);
    assert(subterra.is_excavated == false);
    assert(subterra.ventilation_radius == 5);
    assert(subterra.has_surface_access == true);

    printf("  PASS: Custom value assignment works correctly\n");
}

void test_subterra_copy() {
    printf("Testing copy semantics...\n");

    SubterraComponent original{};
    original.depth_level = 2;
    original.is_excavated = true;
    original.ventilation_radius = 4;
    original.has_surface_access = true;

    SubterraComponent copy = original;
    assert(copy.depth_level == 2);
    assert(copy.is_excavated == true);
    assert(copy.ventilation_radius == 4);
    assert(copy.has_surface_access == true);

    printf("  PASS: Copy semantics work correctly\n");
}

void test_subterra_mvp_depth() {
    printf("Testing MVP depth level constraint...\n");

    SubterraComponent subterra{};
    // MVP default is depth level 1
    assert(subterra.depth_level == 1);

    // Can be set to other levels for future expansion
    subterra.depth_level = 2;
    assert(subterra.depth_level == 2);

    printf("  PASS: MVP depth level constraint verified\n");
}

int main() {
    printf("=== SubterraComponent Unit Tests (Epic 7, Ticket E7-043) ===\n\n");

    test_subterra_component_size();
    test_subterra_trivially_copyable();
    test_subterra_default_initialization();
    test_subterra_custom_values();
    test_subterra_copy();
    test_subterra_mvp_depth();

    printf("\n=== All SubterraComponent Tests Passed ===\n");
    return 0;
}
