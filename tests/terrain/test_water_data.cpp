/**
 * @file test_water_data.cpp
 * @brief Unit tests for WaterData.h (Ticket 3-005)
 *
 * Tests cover:
 * - WaterBodyID type (uint16, 0 = no water body)
 * - FlowDirection enum (8 cardinal + diagonal directions, plus None)
 * - WaterBodyGrid construction and access
 * - FlowDirectionGrid construction and access
 * - WaterData combined struct
 * - get_water_body_id(x, y) query
 * - get_flow_direction(x, y) query
 * - Memory overhead verification (3 bytes per tile)
 * - Direction helper functions (DX, DY, opposite)
 */

#include <sims3000/terrain/WaterData.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::terrain;

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n  FAILED: %s (line %d)\n", #condition, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// WaterBodyID Type Tests
// =============================================================================

TEST(water_body_id_size) {
    // Must be exactly 2 bytes
    ASSERT_EQ(sizeof(WaterBodyID), 2);
}

TEST(water_body_id_no_body_value) {
    // NO_WATER_BODY must be 0
    ASSERT_EQ(NO_WATER_BODY, 0);
}

TEST(water_body_id_max_value) {
    // Maximum valid ID
    ASSERT_EQ(MAX_WATER_BODY_ID, 65535);
}

TEST(water_body_id_range) {
    // Valid IDs are 1 to 65535
    WaterBodyID min_valid = 1;
    WaterBodyID max_valid = MAX_WATER_BODY_ID;
    ASSERT_EQ(min_valid, 1);
    ASSERT_EQ(max_valid, 65535);
}

// =============================================================================
// FlowDirection Enum Tests
// =============================================================================

TEST(flow_direction_size) {
    // Must be exactly 1 byte
    ASSERT_EQ(sizeof(FlowDirection), 1);
}

TEST(flow_direction_values) {
    // Verify all 9 direction values
    ASSERT_EQ(static_cast<std::uint8_t>(FlowDirection::None), 0);
    ASSERT_EQ(static_cast<std::uint8_t>(FlowDirection::N), 1);
    ASSERT_EQ(static_cast<std::uint8_t>(FlowDirection::NE), 2);
    ASSERT_EQ(static_cast<std::uint8_t>(FlowDirection::E), 3);
    ASSERT_EQ(static_cast<std::uint8_t>(FlowDirection::SE), 4);
    ASSERT_EQ(static_cast<std::uint8_t>(FlowDirection::S), 5);
    ASSERT_EQ(static_cast<std::uint8_t>(FlowDirection::SW), 6);
    ASSERT_EQ(static_cast<std::uint8_t>(FlowDirection::W), 7);
    ASSERT_EQ(static_cast<std::uint8_t>(FlowDirection::NW), 8);
}

TEST(flow_direction_count) {
    ASSERT_EQ(FLOW_DIRECTION_COUNT, 9);
}

TEST(flow_direction_validation) {
    // Valid directions (0-8)
    for (std::uint8_t i = 0; i < 9; ++i) {
        ASSERT(isValidFlowDirection(i));
    }
    // Invalid directions (9+)
    ASSERT(!isValidFlowDirection(9));
    ASSERT(!isValidFlowDirection(255));
}

// =============================================================================
// FlowDirection Helper Function Tests
// =============================================================================

TEST(flow_direction_dx_values) {
    // Test X offsets for each direction
    ASSERT_EQ(getFlowDirectionDX(FlowDirection::None), 0);
    ASSERT_EQ(getFlowDirectionDX(FlowDirection::N), 0);
    ASSERT_EQ(getFlowDirectionDX(FlowDirection::NE), 1);
    ASSERT_EQ(getFlowDirectionDX(FlowDirection::E), 1);
    ASSERT_EQ(getFlowDirectionDX(FlowDirection::SE), 1);
    ASSERT_EQ(getFlowDirectionDX(FlowDirection::S), 0);
    ASSERT_EQ(getFlowDirectionDX(FlowDirection::SW), -1);
    ASSERT_EQ(getFlowDirectionDX(FlowDirection::W), -1);
    ASSERT_EQ(getFlowDirectionDX(FlowDirection::NW), -1);
}

TEST(flow_direction_dy_values) {
    // Test Y offsets for each direction (positive Y is down/south)
    ASSERT_EQ(getFlowDirectionDY(FlowDirection::None), 0);
    ASSERT_EQ(getFlowDirectionDY(FlowDirection::N), -1);
    ASSERT_EQ(getFlowDirectionDY(FlowDirection::NE), -1);
    ASSERT_EQ(getFlowDirectionDY(FlowDirection::E), 0);
    ASSERT_EQ(getFlowDirectionDY(FlowDirection::SE), 1);
    ASSERT_EQ(getFlowDirectionDY(FlowDirection::S), 1);
    ASSERT_EQ(getFlowDirectionDY(FlowDirection::SW), 1);
    ASSERT_EQ(getFlowDirectionDY(FlowDirection::W), 0);
    ASSERT_EQ(getFlowDirectionDY(FlowDirection::NW), -1);
}

TEST(flow_direction_opposites) {
    // Test opposite directions
    ASSERT_EQ(getOppositeDirection(FlowDirection::None), FlowDirection::None);
    ASSERT_EQ(getOppositeDirection(FlowDirection::N), FlowDirection::S);
    ASSERT_EQ(getOppositeDirection(FlowDirection::NE), FlowDirection::SW);
    ASSERT_EQ(getOppositeDirection(FlowDirection::E), FlowDirection::W);
    ASSERT_EQ(getOppositeDirection(FlowDirection::SE), FlowDirection::NW);
    ASSERT_EQ(getOppositeDirection(FlowDirection::S), FlowDirection::N);
    ASSERT_EQ(getOppositeDirection(FlowDirection::SW), FlowDirection::NE);
    ASSERT_EQ(getOppositeDirection(FlowDirection::W), FlowDirection::E);
    ASSERT_EQ(getOppositeDirection(FlowDirection::NW), FlowDirection::SE);
}

TEST(flow_direction_opposite_symmetry) {
    // Double opposite should return original
    ASSERT_EQ(getOppositeDirection(getOppositeDirection(FlowDirection::N)), FlowDirection::N);
    ASSERT_EQ(getOppositeDirection(getOppositeDirection(FlowDirection::NE)), FlowDirection::NE);
    ASSERT_EQ(getOppositeDirection(getOppositeDirection(FlowDirection::E)), FlowDirection::E);
    ASSERT_EQ(getOppositeDirection(getOppositeDirection(FlowDirection::SE)), FlowDirection::SE);
}

// =============================================================================
// WaterBodyGrid Construction Tests
// =============================================================================

TEST(water_body_grid_default_construction) {
    WaterBodyGrid grid;
    ASSERT_EQ(grid.width, 0);
    ASSERT_EQ(grid.height, 0);
    ASSERT(grid.body_ids.empty());
    ASSERT(grid.empty());
}

TEST(water_body_grid_mapsize_small) {
    WaterBodyGrid grid(MapSize::Small);
    ASSERT_EQ(grid.width, 128);
    ASSERT_EQ(grid.height, 128);
    ASSERT_EQ(grid.body_ids.size(), 128 * 128);
    ASSERT(!grid.empty());
}

TEST(water_body_grid_mapsize_medium) {
    WaterBodyGrid grid(MapSize::Medium);
    ASSERT_EQ(grid.width, 256);
    ASSERT_EQ(grid.height, 256);
    ASSERT_EQ(grid.body_ids.size(), 256 * 256);
}

TEST(water_body_grid_mapsize_large) {
    WaterBodyGrid grid(MapSize::Large);
    ASSERT_EQ(grid.width, 512);
    ASSERT_EQ(grid.height, 512);
    ASSERT_EQ(grid.body_ids.size(), 512 * 512);
}

TEST(water_body_grid_initialized_to_no_body) {
    WaterBodyGrid grid(MapSize::Small);

    // All tiles should be NO_WATER_BODY
    for (std::uint16_t y = 0; y < 128; y += 17) {
        for (std::uint16_t x = 0; x < 128; x += 17) {
            ASSERT_EQ(grid.get(x, y), NO_WATER_BODY);
        }
    }
}

// =============================================================================
// WaterBodyGrid Access Tests
// =============================================================================

TEST(water_body_grid_get_set) {
    WaterBodyGrid grid(MapSize::Small);

    grid.set(10, 20, 42);
    ASSERT_EQ(grid.get(10, 20), 42);

    grid.set(100, 100, MAX_WATER_BODY_ID);
    ASSERT_EQ(grid.get(100, 100), MAX_WATER_BODY_ID);
}

TEST(water_body_grid_signed_coordinates) {
    WaterBodyGrid grid(MapSize::Small);

    std::int32_t x = 50;
    std::int32_t y = 60;

    grid.set(x, y, 123);
    ASSERT_EQ(grid.get(x, y), 123);
}

TEST(water_body_grid_in_bounds) {
    WaterBodyGrid grid(MapSize::Small);  // 128x128

    ASSERT(grid.in_bounds(0, 0));
    ASSERT(grid.in_bounds(127, 127));
    ASSERT(grid.in_bounds(64, 64));

    ASSERT(!grid.in_bounds(-1, 0));
    ASSERT(!grid.in_bounds(0, -1));
    ASSERT(!grid.in_bounds(128, 0));
    ASSERT(!grid.in_bounds(0, 128));
}

TEST(water_body_grid_index_of) {
    WaterBodyGrid grid(MapSize::Small);

    ASSERT_EQ(grid.index_of(0, 0), 0);
    ASSERT_EQ(grid.index_of(127, 0), 127);
    ASSERT_EQ(grid.index_of(0, 1), 128);
    ASSERT_EQ(grid.index_of(5, 3), 389);
}

TEST(water_body_grid_clear) {
    WaterBodyGrid grid(MapSize::Small);

    // Set some values
    grid.set(10, 10, 100);
    grid.set(50, 50, 200);

    // Clear
    grid.clear();

    // All should be NO_WATER_BODY
    ASSERT_EQ(grid.get(10, 10), NO_WATER_BODY);
    ASSERT_EQ(grid.get(50, 50), NO_WATER_BODY);
}

TEST(water_body_grid_initialize) {
    WaterBodyGrid grid(MapSize::Small);
    grid.set(10, 10, 100);

    grid.initialize(MapSize::Medium);

    ASSERT_EQ(grid.width, 256);
    ASSERT_EQ(grid.height, 256);
    ASSERT_EQ(grid.tile_count(), 256 * 256);
}

// =============================================================================
// FlowDirectionGrid Construction Tests
// =============================================================================

TEST(flow_direction_grid_default_construction) {
    FlowDirectionGrid grid;
    ASSERT_EQ(grid.width, 0);
    ASSERT_EQ(grid.height, 0);
    ASSERT(grid.directions.empty());
    ASSERT(grid.empty());
}

TEST(flow_direction_grid_mapsize_small) {
    FlowDirectionGrid grid(MapSize::Small);
    ASSERT_EQ(grid.width, 128);
    ASSERT_EQ(grid.height, 128);
    ASSERT_EQ(grid.directions.size(), 128 * 128);
    ASSERT(!grid.empty());
}

TEST(flow_direction_grid_mapsize_large) {
    FlowDirectionGrid grid(MapSize::Large);
    ASSERT_EQ(grid.width, 512);
    ASSERT_EQ(grid.height, 512);
    ASSERT_EQ(grid.directions.size(), 512 * 512);
}

TEST(flow_direction_grid_initialized_to_none) {
    FlowDirectionGrid grid(MapSize::Small);

    // All tiles should be FlowDirection::None
    for (std::uint16_t y = 0; y < 128; y += 17) {
        for (std::uint16_t x = 0; x < 128; x += 17) {
            ASSERT_EQ(grid.get(x, y), FlowDirection::None);
        }
    }
}

// =============================================================================
// FlowDirectionGrid Access Tests
// =============================================================================

TEST(flow_direction_grid_get_set) {
    FlowDirectionGrid grid(MapSize::Small);

    grid.set(10, 20, FlowDirection::N);
    ASSERT_EQ(grid.get(10, 20), FlowDirection::N);

    grid.set(30, 40, FlowDirection::SE);
    ASSERT_EQ(grid.get(30, 40), FlowDirection::SE);
}

TEST(flow_direction_grid_all_directions) {
    FlowDirectionGrid grid(MapSize::Small);

    // Set and verify each direction
    grid.set(0, 0, FlowDirection::None);
    grid.set(1, 0, FlowDirection::N);
    grid.set(2, 0, FlowDirection::NE);
    grid.set(3, 0, FlowDirection::E);
    grid.set(4, 0, FlowDirection::SE);
    grid.set(5, 0, FlowDirection::S);
    grid.set(6, 0, FlowDirection::SW);
    grid.set(7, 0, FlowDirection::W);
    grid.set(8, 0, FlowDirection::NW);

    ASSERT_EQ(grid.get(0, 0), FlowDirection::None);
    ASSERT_EQ(grid.get(1, 0), FlowDirection::N);
    ASSERT_EQ(grid.get(2, 0), FlowDirection::NE);
    ASSERT_EQ(grid.get(3, 0), FlowDirection::E);
    ASSERT_EQ(grid.get(4, 0), FlowDirection::SE);
    ASSERT_EQ(grid.get(5, 0), FlowDirection::S);
    ASSERT_EQ(grid.get(6, 0), FlowDirection::SW);
    ASSERT_EQ(grid.get(7, 0), FlowDirection::W);
    ASSERT_EQ(grid.get(8, 0), FlowDirection::NW);
}

TEST(flow_direction_grid_in_bounds) {
    FlowDirectionGrid grid(MapSize::Medium);  // 256x256

    ASSERT(grid.in_bounds(0, 0));
    ASSERT(grid.in_bounds(255, 255));

    ASSERT(!grid.in_bounds(-1, 0));
    ASSERT(!grid.in_bounds(256, 0));
}

TEST(flow_direction_grid_clear) {
    FlowDirectionGrid grid(MapSize::Small);

    // Set some values
    grid.set(10, 10, FlowDirection::E);
    grid.set(50, 50, FlowDirection::W);

    // Clear
    grid.clear();

    // All should be None
    ASSERT_EQ(grid.get(10, 10), FlowDirection::None);
    ASSERT_EQ(grid.get(50, 50), FlowDirection::None);
}

// =============================================================================
// Memory Budget Tests
// =============================================================================

TEST(water_body_grid_memory_small) {
    WaterBodyGrid grid(MapSize::Small);  // 128x128

    // 128 * 128 = 16,384 tiles * 2 bytes = 32,768 bytes = 32KB
    ASSERT_EQ(grid.tile_count(), 16384);
    ASSERT_EQ(grid.memory_bytes(), 32768);
}

TEST(water_body_grid_memory_medium) {
    WaterBodyGrid grid(MapSize::Medium);  // 256x256

    // 256 * 256 = 65,536 tiles * 2 bytes = 131,072 bytes = 128KB
    ASSERT_EQ(grid.tile_count(), 65536);
    ASSERT_EQ(grid.memory_bytes(), 131072);
}

TEST(water_body_grid_memory_large) {
    WaterBodyGrid grid(MapSize::Large);  // 512x512

    // 512 * 512 = 262,144 tiles * 2 bytes = 524,288 bytes = 512KB
    ASSERT_EQ(grid.tile_count(), 262144);
    ASSERT_EQ(grid.memory_bytes(), 524288);
}

TEST(flow_direction_grid_memory_small) {
    FlowDirectionGrid grid(MapSize::Small);  // 128x128

    // 128 * 128 = 16,384 tiles * 1 byte = 16,384 bytes = 16KB
    ASSERT_EQ(grid.tile_count(), 16384);
    ASSERT_EQ(grid.memory_bytes(), 16384);
}

TEST(flow_direction_grid_memory_medium) {
    FlowDirectionGrid grid(MapSize::Medium);  // 256x256

    // 256 * 256 = 65,536 tiles * 1 byte = 65,536 bytes = 64KB
    ASSERT_EQ(grid.tile_count(), 65536);
    ASSERT_EQ(grid.memory_bytes(), 65536);
}

TEST(flow_direction_grid_memory_large) {
    FlowDirectionGrid grid(MapSize::Large);  // 512x512

    // 512 * 512 = 262,144 tiles * 1 byte = 262,144 bytes = 256KB
    ASSERT_EQ(grid.tile_count(), 262144);
    ASSERT_EQ(grid.memory_bytes(), 262144);
}

TEST(combined_memory_budget_large) {
    // 512x512 combined: 512KB + 256KB = 768KB
    WaterBodyGrid body_grid(MapSize::Large);
    FlowDirectionGrid flow_grid(MapSize::Large);

    std::size_t combined = body_grid.memory_bytes() + flow_grid.memory_bytes();
    ASSERT_EQ(combined, 786432);  // 768KB
}

// =============================================================================
// WaterData Combined Struct Tests
// =============================================================================

TEST(water_data_default_construction) {
    WaterData data;
    ASSERT(data.empty());
}

TEST(water_data_mapsize_construction) {
    WaterData data(MapSize::Medium);
    ASSERT(!data.empty());
    ASSERT_EQ(data.water_body_ids.width, 256);
    ASSERT_EQ(data.flow_directions.width, 256);
}

TEST(water_data_initialize) {
    WaterData data;
    data.initialize(MapSize::Large);

    ASSERT(!data.empty());
    ASSERT_EQ(data.water_body_ids.width, 512);
    ASSERT_EQ(data.flow_directions.width, 512);
}

TEST(water_data_get_water_body_id) {
    WaterData data(MapSize::Small);

    // Initially NO_WATER_BODY
    ASSERT_EQ(data.get_water_body_id(50, 50), NO_WATER_BODY);

    // Set via underlying grid
    data.water_body_ids.set(50, 50, 123);

    // Get via convenience method
    ASSERT_EQ(data.get_water_body_id(50, 50), 123);
}

TEST(water_data_get_flow_direction) {
    WaterData data(MapSize::Small);

    // Initially None
    ASSERT_EQ(data.get_flow_direction(50, 50), FlowDirection::None);

    // Set via underlying grid
    data.flow_directions.set(50, 50, FlowDirection::NE);

    // Get via convenience method
    ASSERT_EQ(data.get_flow_direction(50, 50), FlowDirection::NE);
}

TEST(water_data_set_water_body_id) {
    WaterData data(MapSize::Small);

    data.set_water_body_id(30, 40, 999);
    ASSERT_EQ(data.get_water_body_id(30, 40), 999);
}

TEST(water_data_set_flow_direction) {
    WaterData data(MapSize::Small);

    data.set_flow_direction(30, 40, FlowDirection::SW);
    ASSERT_EQ(data.get_flow_direction(30, 40), FlowDirection::SW);
}

TEST(water_data_in_bounds) {
    WaterData data(MapSize::Small);  // 128x128

    ASSERT(data.in_bounds(0, 0));
    ASSERT(data.in_bounds(127, 127));
    ASSERT(!data.in_bounds(-1, 0));
    ASSERT(!data.in_bounds(128, 0));
}

TEST(water_data_memory_bytes) {
    WaterData data(MapSize::Large);

    // 512KB + 256KB = 768KB = 786,432 bytes
    ASSERT_EQ(data.memory_bytes(), 786432);
}

TEST(water_data_clear) {
    WaterData data(MapSize::Small);

    data.set_water_body_id(10, 10, 100);
    data.set_flow_direction(10, 10, FlowDirection::E);

    data.clear();

    ASSERT_EQ(data.get_water_body_id(10, 10), NO_WATER_BODY);
    ASSERT_EQ(data.get_flow_direction(10, 10), FlowDirection::None);
}

// =============================================================================
// Typical Usage Pattern Tests
// =============================================================================

TEST(river_flow_pattern) {
    // Simulate a river flowing from north to south
    WaterData data(MapSize::Small);

    WaterBodyID river_id = 1;

    // Create a river from (50, 10) to (50, 50)
    for (int y = 10; y <= 50; ++y) {
        data.set_water_body_id(50, y, river_id);
        data.set_flow_direction(50, y, FlowDirection::S);
    }

    // Verify river tiles
    ASSERT_EQ(data.get_water_body_id(50, 20), river_id);
    ASSERT_EQ(data.get_flow_direction(50, 20), FlowDirection::S);

    // Non-river tiles
    ASSERT_EQ(data.get_water_body_id(49, 20), NO_WATER_BODY);
    ASSERT_EQ(data.get_flow_direction(49, 20), FlowDirection::None);
}

TEST(multiple_water_bodies) {
    WaterData data(MapSize::Small);

    WaterBodyID ocean_id = 1;
    WaterBodyID lake_id = 2;
    WaterBodyID river_id = 3;

    // Ocean in corner
    data.set_water_body_id(0, 0, ocean_id);
    data.set_water_body_id(1, 0, ocean_id);
    data.set_water_body_id(0, 1, ocean_id);

    // Lake in center
    data.set_water_body_id(64, 64, lake_id);
    data.set_water_body_id(65, 64, lake_id);
    data.set_water_body_id(64, 65, lake_id);

    // River connecting them
    data.set_water_body_id(32, 32, river_id);
    data.set_flow_direction(32, 32, FlowDirection::NW);

    // Verify different bodies
    ASSERT_EQ(data.get_water_body_id(0, 0), ocean_id);
    ASSERT_EQ(data.get_water_body_id(64, 64), lake_id);
    ASSERT_EQ(data.get_water_body_id(32, 32), river_id);

    // Ocean and lake have no flow, river has flow
    ASSERT_EQ(data.get_flow_direction(0, 0), FlowDirection::None);
    ASSERT_EQ(data.get_flow_direction(64, 64), FlowDirection::None);
    ASSERT_EQ(data.get_flow_direction(32, 32), FlowDirection::NW);
}

TEST(diagonal_river_pattern) {
    // Simulate a river flowing diagonally SE
    WaterData data(MapSize::Small);

    WaterBodyID river_id = 1;

    for (int i = 0; i < 20; ++i) {
        int x = 30 + i;
        int y = 30 + i;
        data.set_water_body_id(x, y, river_id);
        data.set_flow_direction(x, y, FlowDirection::SE);
    }

    // Verify diagonal pattern
    ASSERT_EQ(data.get_water_body_id(35, 35), river_id);
    ASSERT_EQ(data.get_flow_direction(35, 35), FlowDirection::SE);

    // Using helper functions to trace flow
    int x = 35, y = 35;
    FlowDirection dir = data.get_flow_direction(x, y);
    int next_x = x + getFlowDirectionDX(dir);
    int next_y = y + getFlowDirectionDY(dir);

    ASSERT_EQ(next_x, 36);
    ASSERT_EQ(next_y, 36);
    ASSERT_EQ(data.get_water_body_id(next_x, next_y), river_id);
}

TEST(edge_tile_water_body) {
    WaterData data(MapSize::Small);

    // Set water bodies at corners and edges
    data.set_water_body_id(0, 0, 1);
    data.set_water_body_id(127, 0, 2);
    data.set_water_body_id(0, 127, 3);
    data.set_water_body_id(127, 127, 4);

    ASSERT_EQ(data.get_water_body_id(0, 0), 1);
    ASSERT_EQ(data.get_water_body_id(127, 0), 2);
    ASSERT_EQ(data.get_water_body_id(0, 127), 3);
    ASSERT_EQ(data.get_water_body_id(127, 127), 4);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== WaterData Unit Tests (Ticket 3-005) ===\n\n");

    // WaterBodyID type tests
    RUN_TEST(water_body_id_size);
    RUN_TEST(water_body_id_no_body_value);
    RUN_TEST(water_body_id_max_value);
    RUN_TEST(water_body_id_range);

    // FlowDirection enum tests
    RUN_TEST(flow_direction_size);
    RUN_TEST(flow_direction_values);
    RUN_TEST(flow_direction_count);
    RUN_TEST(flow_direction_validation);

    // FlowDirection helper function tests
    RUN_TEST(flow_direction_dx_values);
    RUN_TEST(flow_direction_dy_values);
    RUN_TEST(flow_direction_opposites);
    RUN_TEST(flow_direction_opposite_symmetry);

    // WaterBodyGrid construction tests
    RUN_TEST(water_body_grid_default_construction);
    RUN_TEST(water_body_grid_mapsize_small);
    RUN_TEST(water_body_grid_mapsize_medium);
    RUN_TEST(water_body_grid_mapsize_large);
    RUN_TEST(water_body_grid_initialized_to_no_body);

    // WaterBodyGrid access tests
    RUN_TEST(water_body_grid_get_set);
    RUN_TEST(water_body_grid_signed_coordinates);
    RUN_TEST(water_body_grid_in_bounds);
    RUN_TEST(water_body_grid_index_of);
    RUN_TEST(water_body_grid_clear);
    RUN_TEST(water_body_grid_initialize);

    // FlowDirectionGrid construction tests
    RUN_TEST(flow_direction_grid_default_construction);
    RUN_TEST(flow_direction_grid_mapsize_small);
    RUN_TEST(flow_direction_grid_mapsize_large);
    RUN_TEST(flow_direction_grid_initialized_to_none);

    // FlowDirectionGrid access tests
    RUN_TEST(flow_direction_grid_get_set);
    RUN_TEST(flow_direction_grid_all_directions);
    RUN_TEST(flow_direction_grid_in_bounds);
    RUN_TEST(flow_direction_grid_clear);

    // Memory budget tests
    RUN_TEST(water_body_grid_memory_small);
    RUN_TEST(water_body_grid_memory_medium);
    RUN_TEST(water_body_grid_memory_large);
    RUN_TEST(flow_direction_grid_memory_small);
    RUN_TEST(flow_direction_grid_memory_medium);
    RUN_TEST(flow_direction_grid_memory_large);
    RUN_TEST(combined_memory_budget_large);

    // WaterData combined struct tests
    RUN_TEST(water_data_default_construction);
    RUN_TEST(water_data_mapsize_construction);
    RUN_TEST(water_data_initialize);
    RUN_TEST(water_data_get_water_body_id);
    RUN_TEST(water_data_get_flow_direction);
    RUN_TEST(water_data_set_water_body_id);
    RUN_TEST(water_data_set_flow_direction);
    RUN_TEST(water_data_in_bounds);
    RUN_TEST(water_data_memory_bytes);
    RUN_TEST(water_data_clear);

    // Typical usage pattern tests
    RUN_TEST(river_flow_pattern);
    RUN_TEST(multiple_water_bodies);
    RUN_TEST(diagonal_river_pattern);
    RUN_TEST(edge_tile_water_body);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
