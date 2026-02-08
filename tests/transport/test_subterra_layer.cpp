/**
 * @file test_subterra_layer.cpp
 * @brief Unit tests for SubterraLayerManager (Epic 7, Ticket E7-042)
 *
 * Tests:
 * - Construction and dimensions
 * - Grid operations: set, get, has, clear
 * - Placement validation (bounds + not occupied)
 * - Out-of-bounds handling
 * - Edge cases: zero dimensions, boundary cells, negative coords
 */

#include <sims3000/transport/SubterraLayerManager.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_default_construction() {
    printf("Testing default construction...\n");

    SubterraLayerManager mgr;

    assert(mgr.width() == 0);
    assert(mgr.height() == 0);

    printf("  PASS: Default constructor gives 0x0 grid\n");
}

void test_sized_construction() {
    printf("Testing sized construction...\n");

    SubterraLayerManager mgr(64, 32);

    assert(mgr.width() == 64);
    assert(mgr.height() == 32);

    printf("  PASS: Constructor sets correct dimensions\n");
}

void test_empty_grid_has_no_subterra() {
    printf("Testing empty grid has no subterra...\n");

    SubterraLayerManager mgr(10, 10);

    for (int32_t y = 0; y < 10; ++y) {
        for (int32_t x = 0; x < 10; ++x) {
            assert(mgr.has_subterra(x, y) == false);
            assert(mgr.get_subterra_at(x, y) == 0);
        }
    }

    printf("  PASS: All cells empty after construction\n");
}

void test_set_and_get() {
    printf("Testing set and get...\n");

    SubterraLayerManager mgr(10, 10);

    mgr.set_subterra(5, 3, 42);

    assert(mgr.get_subterra_at(5, 3) == 42);
    assert(mgr.has_subterra(5, 3) == true);

    // Other cells still empty
    assert(mgr.has_subterra(4, 3) == false);
    assert(mgr.has_subterra(5, 4) == false);

    printf("  PASS: set_subterra and get_subterra_at work correctly\n");
}

void test_clear_subterra() {
    printf("Testing clear subterra...\n");

    SubterraLayerManager mgr(10, 10);

    mgr.set_subterra(2, 7, 100);
    assert(mgr.has_subterra(2, 7) == true);

    mgr.clear_subterra(2, 7);
    assert(mgr.has_subterra(2, 7) == false);
    assert(mgr.get_subterra_at(2, 7) == 0);

    printf("  PASS: clear_subterra removes entity\n");
}

void test_overwrite_subterra() {
    printf("Testing overwrite subterra...\n");

    SubterraLayerManager mgr(10, 10);

    mgr.set_subterra(1, 1, 10);
    assert(mgr.get_subterra_at(1, 1) == 10);

    mgr.set_subterra(1, 1, 20);
    assert(mgr.get_subterra_at(1, 1) == 20);

    printf("  PASS: set_subterra overwrites previous value\n");
}

void test_in_bounds() {
    printf("Testing in_bounds...\n");

    SubterraLayerManager mgr(10, 8);

    // Valid cells
    assert(mgr.in_bounds(0, 0) == true);
    assert(mgr.in_bounds(9, 7) == true);
    assert(mgr.in_bounds(5, 4) == true);

    // Out of bounds
    assert(mgr.in_bounds(-1, 0) == false);
    assert(mgr.in_bounds(0, -1) == false);
    assert(mgr.in_bounds(10, 0) == false);
    assert(mgr.in_bounds(0, 8) == false);
    assert(mgr.in_bounds(10, 8) == false);
    assert(mgr.in_bounds(-1, -1) == false);

    printf("  PASS: in_bounds validates correctly\n");
}

void test_out_of_bounds_get() {
    printf("Testing out of bounds get returns 0...\n");

    SubterraLayerManager mgr(5, 5);

    assert(mgr.get_subterra_at(-1, 0) == 0);
    assert(mgr.get_subterra_at(0, -1) == 0);
    assert(mgr.get_subterra_at(5, 0) == 0);
    assert(mgr.get_subterra_at(0, 5) == 0);
    assert(mgr.get_subterra_at(100, 100) == 0);

    printf("  PASS: Out of bounds get returns 0\n");
}

void test_out_of_bounds_set_ignored() {
    printf("Testing out of bounds set is ignored...\n");

    SubterraLayerManager mgr(5, 5);

    // These should silently do nothing
    mgr.set_subterra(-1, 0, 99);
    mgr.set_subterra(0, -1, 99);
    mgr.set_subterra(5, 0, 99);
    mgr.set_subterra(0, 5, 99);

    // Grid should still be empty
    for (int32_t y = 0; y < 5; ++y) {
        for (int32_t x = 0; x < 5; ++x) {
            assert(mgr.has_subterra(x, y) == false);
        }
    }

    printf("  PASS: Out of bounds set is silently ignored\n");
}

void test_can_build_empty_cell() {
    printf("Testing can_build on empty cell...\n");

    SubterraLayerManager mgr(10, 10);

    assert(mgr.can_build_subterra_at(5, 5) == true);

    printf("  PASS: Can build on empty cell\n");
}

void test_can_build_occupied_cell() {
    printf("Testing can_build on occupied cell...\n");

    SubterraLayerManager mgr(10, 10);

    mgr.set_subterra(5, 5, 42);

    assert(mgr.can_build_subterra_at(5, 5) == false);

    printf("  PASS: Cannot build on occupied cell\n");
}

void test_can_build_out_of_bounds() {
    printf("Testing can_build out of bounds...\n");

    SubterraLayerManager mgr(10, 10);

    assert(mgr.can_build_subterra_at(-1, 0) == false);
    assert(mgr.can_build_subterra_at(0, -1) == false);
    assert(mgr.can_build_subterra_at(10, 0) == false);
    assert(mgr.can_build_subterra_at(0, 10) == false);

    printf("  PASS: Cannot build out of bounds\n");
}

void test_can_build_after_clear() {
    printf("Testing can_build after clearing a cell...\n");

    SubterraLayerManager mgr(10, 10);

    mgr.set_subterra(3, 3, 77);
    assert(mgr.can_build_subterra_at(3, 3) == false);

    mgr.clear_subterra(3, 3);
    assert(mgr.can_build_subterra_at(3, 3) == true);

    printf("  PASS: Can build after clearing cell\n");
}

void test_boundary_cells() {
    printf("Testing boundary cells...\n");

    SubterraLayerManager mgr(10, 10);

    // Corners
    mgr.set_subterra(0, 0, 1);
    mgr.set_subterra(9, 0, 2);
    mgr.set_subterra(0, 9, 3);
    mgr.set_subterra(9, 9, 4);

    assert(mgr.get_subterra_at(0, 0) == 1);
    assert(mgr.get_subterra_at(9, 0) == 2);
    assert(mgr.get_subterra_at(0, 9) == 3);
    assert(mgr.get_subterra_at(9, 9) == 4);

    printf("  PASS: Boundary/corner cells work correctly\n");
}

void test_multiple_entities() {
    printf("Testing multiple entities in grid...\n");

    SubterraLayerManager mgr(10, 10);

    mgr.set_subterra(0, 0, 100);
    mgr.set_subterra(5, 5, 200);
    mgr.set_subterra(9, 9, 300);

    assert(mgr.get_subterra_at(0, 0) == 100);
    assert(mgr.get_subterra_at(5, 5) == 200);
    assert(mgr.get_subterra_at(9, 9) == 300);
    assert(mgr.has_subterra(3, 3) == false);

    printf("  PASS: Multiple entities stored independently\n");
}

void test_zero_dimension_grid() {
    printf("Testing zero-dimension grid...\n");

    SubterraLayerManager mgr(0, 0);

    assert(mgr.width() == 0);
    assert(mgr.height() == 0);
    assert(mgr.in_bounds(0, 0) == false);
    assert(mgr.can_build_subterra_at(0, 0) == false);
    assert(mgr.get_subterra_at(0, 0) == 0);

    printf("  PASS: Zero-dimension grid handles all operations safely\n");
}

int main() {
    printf("=== SubterraLayerManager Unit Tests (Epic 7, Ticket E7-042) ===\n\n");

    test_default_construction();
    test_sized_construction();
    test_empty_grid_has_no_subterra();
    test_set_and_get();
    test_clear_subterra();
    test_overwrite_subterra();
    test_in_bounds();
    test_out_of_bounds_get();
    test_out_of_bounds_set_ignored();
    test_can_build_empty_cell();
    test_can_build_occupied_cell();
    test_can_build_out_of_bounds();
    test_can_build_after_clear();
    test_boundary_cells();
    test_multiple_entities();
    test_zero_dimension_grid();

    printf("\n=== All SubterraLayerManager Tests Passed ===\n");
    return 0;
}
