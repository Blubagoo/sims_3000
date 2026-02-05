/**
 * @file test_types.cpp
 * @brief Unit tests for core types.
 */

#include "sims3000/core/types.h"
#include <cassert>
#include <cstdio>
#include <unordered_set>

using namespace sims3000;

void test_type_sizes() {
    printf("Testing type sizes...\n");

    assert(sizeof(EntityID) == 4);
    assert(sizeof(PlayerID) == 1);
    assert(sizeof(Credits) == 8);
    assert(sizeof(SimulationTick) == 8);
    assert(sizeof(GridPosition) == 4);

    printf("  PASS: All type sizes correct\n");
}

void test_grid_position_operators() {
    printf("Testing GridPosition operators...\n");

    GridPosition a{10, 20};
    GridPosition b{5, 15};

    // Addition
    GridPosition sum = a + b;
    assert(sum.x == 15);
    assert(sum.y == 35);

    // Subtraction
    GridPosition diff = a - b;
    assert(diff.x == 5);
    assert(diff.y == 5);

    // Equality
    assert(a == a);
    assert(!(a == b));
    assert(a != b);

    printf("  PASS: GridPosition operators work correctly\n");
}

void test_grid_position_hash() {
    printf("Testing GridPosition hash...\n");

    std::unordered_set<GridPosition> positions;

    positions.insert({0, 0});
    positions.insert({1, 0});
    positions.insert({0, 1});
    positions.insert({1, 1});
    positions.insert({-1, -1});

    assert(positions.size() == 5);
    assert(positions.count({0, 0}) == 1);
    assert(positions.count({1, 1}) == 1);
    assert(positions.count({-1, -1}) == 1);
    assert(positions.count({2, 2}) == 0);

    printf("  PASS: GridPosition hash works correctly\n");
}

void test_type_ranges() {
    printf("Testing type ranges...\n");

    // EntityID - 32-bit unsigned
    EntityID maxEntity = 0xFFFFFFFF;
    assert(maxEntity == 4294967295);

    // PlayerID - 8-bit unsigned
    PlayerID maxPlayer = 255;
    assert(maxPlayer == 255);

    // Credits - 64-bit signed (can be negative for debt)
    Credits debt = -1000000;
    Credits wealth = 1000000000000LL;
    assert(debt < 0);
    assert(wealth > 0);

    // SimulationTick - 64-bit unsigned
    SimulationTick maxTick = 0xFFFFFFFFFFFFFFFFULL;
    assert(maxTick > 0);

    // GridPosition - 16-bit signed per axis
    GridPosition maxPos{32767, 32767};
    GridPosition minPos{-32768, -32768};
    assert(maxPos.x == 32767);
    assert(minPos.x == -32768);

    printf("  PASS: Type ranges correct\n");
}

int main() {
    printf("=== Core Types Unit Tests ===\n\n");

    test_type_sizes();
    test_grid_position_operators();
    test_grid_position_hash();
    test_type_ranges();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
