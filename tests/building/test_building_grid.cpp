/**
 * @file test_building_grid.cpp
 * @brief Unit tests for BuildingGrid (Epic 4, Ticket 4-007)
 */

#include <sims3000/building/BuildingGrid.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::building;

void test_initialization() {
    printf("Testing BuildingGrid initialization...\n");

    BuildingGrid grid;
    assert(grid.empty());
    assert(grid.getWidth() == 0);
    assert(grid.getHeight() == 0);

    grid.initialize(128, 128);
    assert(!grid.empty());
    assert(grid.getWidth() == 128);
    assert(grid.getHeight() == 128);
    assert(grid.cell_count() == 128 * 128);

    printf("  PASS: Initialization works correctly\n");
}

void test_memory_size() {
    printf("Testing memory size...\n");

    BuildingGrid grid128(128, 128);
    assert(grid128.memory_bytes() == 65536); // 64KB

    BuildingGrid grid256(256, 256);
    assert(grid256.memory_bytes() == 262144); // 256KB

    BuildingGrid grid512(512, 512);
    assert(grid512.memory_bytes() == 1048576); // 1MB

    printf("  PASS: Memory sizes correct\n");
}

void test_bounds_checking() {
    printf("Testing bounds checking...\n");

    BuildingGrid grid(128, 128);
    assert(grid.in_bounds(0, 0));
    assert(grid.in_bounds(127, 127));
    assert(!grid.in_bounds(-1, 0));
    assert(!grid.in_bounds(128, 0));

    printf("  PASS: Bounds checking works correctly\n");
}

void test_single_tile_operations() {
    printf("Testing single-tile operations...\n");

    BuildingGrid grid(128, 128);

    // Initially invalid
    assert(grid.get_building_at(0, 0) == INVALID_ENTITY);

    // Set and get
    grid.set_building_at(10, 20, 12345);
    assert(grid.get_building_at(10, 20) == 12345);

    // Is occupied
    assert(grid.is_tile_occupied(10, 20));
    assert(!grid.is_tile_occupied(11, 20));

    // Clear
    grid.clear_building_at(10, 20);
    assert(grid.get_building_at(10, 20) == INVALID_ENTITY);
    assert(!grid.is_tile_occupied(10, 20));

    printf("  PASS: Single-tile operations work correctly\n");
}

void test_footprint_operations() {
    printf("Testing footprint operations...\n");

    BuildingGrid grid(128, 128);

    // Check footprint available
    assert(grid.is_footprint_available(10, 10, 2, 2));

    // Set 2x2 footprint
    grid.set_footprint(10, 10, 2, 2, 555);
    assert(grid.get_building_at(10, 10) == 555);
    assert(grid.get_building_at(11, 10) == 555);
    assert(grid.get_building_at(10, 11) == 555);
    assert(grid.get_building_at(11, 11) == 555);

    // Adjacent cells should be unaffected
    assert(grid.get_building_at(9, 10) == INVALID_ENTITY);
    assert(grid.get_building_at(12, 10) == INVALID_ENTITY);

    // Footprint not available where building exists
    assert(!grid.is_footprint_available(10, 10, 2, 2));
    assert(!grid.is_footprint_available(9, 9, 2, 2)); // Overlaps

    // Clear footprint
    grid.clear_footprint(10, 10, 2, 2);
    assert(grid.get_building_at(10, 10) == INVALID_ENTITY);
    assert(grid.get_building_at(11, 11) == INVALID_ENTITY);

    printf("  PASS: Footprint operations work correctly\n");
}

void test_out_of_bounds() {
    printf("Testing out-of-bounds handling...\n");

    BuildingGrid grid(128, 128);

    // Out-of-bounds get returns INVALID_ENTITY
    assert(grid.get_building_at(-1, 0) == INVALID_ENTITY);
    assert(grid.get_building_at(128, 0) == INVALID_ENTITY);

    // Out-of-bounds set does nothing (no crash)
    grid.set_building_at(-1, 0, 999);
    grid.set_building_at(128, 0, 999);

    printf("  PASS: Out-of-bounds handling works correctly\n");
}

void test_clear_all() {
    printf("Testing clear_all...\n");

    BuildingGrid grid(128, 128);
    grid.set_building_at(10, 10, 111);
    grid.set_building_at(20, 20, 222);

    assert(grid.get_building_at(10, 10) == 111);
    assert(grid.get_building_at(20, 20) == 222);

    grid.clear_all();

    assert(grid.get_building_at(10, 10) == INVALID_ENTITY);
    assert(grid.get_building_at(20, 20) == INVALID_ENTITY);

    printf("  PASS: clear_all works correctly\n");
}

int main() {
    printf("=== BuildingGrid Unit Tests (Epic 4, Ticket 4-007) ===\n\n");

    test_initialization();
    test_memory_size();
    test_bounds_checking();
    test_single_tile_operations();
    test_footprint_operations();
    test_out_of_bounds();
    test_clear_all();

    printf("\n=== All BuildingGrid Tests Passed ===\n");
    return 0;
}
