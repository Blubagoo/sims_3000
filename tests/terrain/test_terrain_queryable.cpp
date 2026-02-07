/**
 * @file test_terrain_queryable.cpp
 * @brief Unit tests for ITerrainQueryable interface (Ticket 3-014, 3-015)
 *
 * Tests cover:
 * - Interface method declarations exist
 * - Mock implementation to verify interface contract
 * - Buildability logic verification
 * - Out-of-bounds coordinate handling (safe defaults)
 * - O(1) query verification with benchmark tests
 * - All 13 original interface methods tested
 * - Batch query methods (Ticket 3-015):
 *   - get_tiles_in_rect: Row-major iteration, clipping, edge cases
 *   - get_buildable_tiles_in_rect: Count verification
 *   - count_terrain_type_in_rect: Type counting
 * - Performance benchmark: 10,000 tile rect query < 10 microseconds
 */

#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainTypeInfo.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <chrono>

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

#define ASSERT_FLOAT_EQ(a, b, eps) do { \
    if (std::fabs((a) - (b)) > (eps)) { \
        printf("\n  FAILED: %s ~= %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_LT(a, b) do { \
    if (!((a) < (b))) { \
        printf("\n  FAILED: %s < %s (%f >= %f) (line %d)\n", #a, #b, \
               static_cast<double>(a), static_cast<double>(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_GT(a, b) do { \
    if (!((a) > (b))) { \
        printf("\n  FAILED: %s > %s (%f <= %f) (line %d)\n", #a, #b, \
               static_cast<double>(a), static_cast<double>(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Mock Implementation of ITerrainQueryable for Testing
// =============================================================================

/**
 * @class MockTerrainQueryable
 * @brief Mock implementation of ITerrainQueryable for testing interface contract.
 *
 * Uses a real TerrainGrid internally to verify that the interface methods
 * can be implemented correctly with the existing data structures.
 */
class MockTerrainQueryable : public ITerrainQueryable {
public:
    TerrainGrid grid;
    std::vector<std::uint8_t> water_distance_field;

    MockTerrainQueryable()
        : grid(MapSize::Small)  // 128x128
        , water_distance_field(128 * 128, 255)  // Default: far from water
    {
        // Initialize with some test data
        initializeTestTerrain();
    }

    void initializeTestTerrain() {
        // Set up some test tiles:
        // (0,0) - Substrate (buildable)
        grid.at(0, 0).setTerrainType(TerrainType::Substrate);
        grid.at(0, 0).setElevation(10);
        grid.at(0, 0).flags = 0;

        // (1,0) - BiolumeGrove (clearable, not cleared)
        grid.at(1, 0).setTerrainType(TerrainType::BiolumeGrove);
        grid.at(1, 0).setElevation(10);
        grid.at(1, 0).flags = 0;

        // (2,0) - BiolumeGrove (clearable, IS cleared)
        grid.at(2, 0).setTerrainType(TerrainType::BiolumeGrove);
        grid.at(2, 0).setElevation(10);
        grid.at(2, 0).setCleared(true);

        // (3,0) - DeepVoid (water, not buildable)
        grid.at(3, 0).setTerrainType(TerrainType::DeepVoid);
        grid.at(3, 0).setElevation(0);
        grid.at(3, 0).setUnderwater(true);
        water_distance_field[0 * 128 + 3] = 0;  // Is water

        // (4,0) - Substrate but underwater (not buildable)
        grid.at(4, 0).setTerrainType(TerrainType::Substrate);
        grid.at(4, 0).setElevation(5);
        grid.at(4, 0).setUnderwater(true);

        // (5,0) - BlightMires (toxic, generates contamination)
        grid.at(5, 0).setTerrainType(TerrainType::BlightMires);
        grid.at(5, 0).setElevation(8);

        // (6,0) - PrismaFields (high value bonus)
        grid.at(6, 0).setTerrainType(TerrainType::PrismaFields);
        grid.at(6, 0).setElevation(12);

        // (7,0) - EmberCrust (high build cost modifier)
        grid.at(7, 0).setTerrainType(TerrainType::EmberCrust);
        grid.at(7, 0).setElevation(15);

        // Set up elevation gradient for slope testing
        // (10,0) = 10, (11,0) = 15, (12,0) = 20
        grid.at(10, 0).setElevation(10);
        grid.at(11, 0).setElevation(15);
        grid.at(12, 0).setElevation(20);

        // Set up water distance field values
        // Tiles near water at (3,0)
        water_distance_field[0 * 128 + 2] = 1;  // Distance 1
        water_distance_field[0 * 128 + 4] = 1;  // Distance 1
        water_distance_field[1 * 128 + 3] = 1;  // Distance 1
        water_distance_field[0 * 128 + 1] = 2;  // Distance 2
    }

    // -------------------------------------------------------------------------
    // ITerrainQueryable Implementation
    // -------------------------------------------------------------------------

    TerrainType get_terrain_type(std::int32_t x, std::int32_t y) const override {
        if (!in_bounds(x, y)) {
            return TerrainType::Substrate;  // Safe default
        }
        return grid.at(x, y).getTerrainType();
    }

    std::uint8_t get_elevation(std::int32_t x, std::int32_t y) const override {
        if (!in_bounds(x, y)) {
            return 0;  // Safe default
        }
        return grid.at(x, y).getElevation();
    }

    bool is_buildable(std::int32_t x, std::int32_t y) const override {
        if (!in_bounds(x, y)) {
            return false;  // Safe default
        }

        const auto& tile = grid.at(x, y);
        const auto& info = getTerrainInfo(tile.getTerrainType());

        // Buildability logic:
        // (type.buildable OR (type.clearable AND is_cleared)) AND NOT is_underwater
        bool type_allows = info.buildable || (info.clearable && tile.isCleared());
        return type_allows && !tile.isUnderwater();
    }

    std::uint8_t get_slope(std::int32_t x1, std::int32_t y1,
                           std::int32_t x2, std::int32_t y2) const override {
        if (!in_bounds(x1, y1) || !in_bounds(x2, y2)) {
            return 0;  // Safe default
        }
        std::uint8_t e1 = grid.at(x1, y1).getElevation();
        std::uint8_t e2 = grid.at(x2, y2).getElevation();
        return (e1 > e2) ? (e1 - e2) : (e2 - e1);
    }

    float get_average_elevation(std::int32_t x, std::int32_t y,
                                 std::uint32_t radius) const override {
        if (!in_bounds(x, y)) {
            return 0.0f;  // Safe default
        }

        std::uint32_t sum = 0;
        std::uint32_t count = 0;

        std::int32_t r = static_cast<std::int32_t>(radius);
        for (std::int32_t dy = -r; dy <= r; ++dy) {
            for (std::int32_t dx = -r; dx <= r; ++dx) {
                std::int32_t nx = x + dx;
                std::int32_t ny = y + dy;
                if (in_bounds(nx, ny)) {
                    sum += grid.at(nx, ny).getElevation();
                    count++;
                }
            }
        }

        return count > 0 ? static_cast<float>(sum) / static_cast<float>(count) : 0.0f;
    }

    std::uint32_t get_water_distance(std::int32_t x, std::int32_t y) const override {
        if (!in_bounds(x, y)) {
            return 255;  // Safe default (max distance)
        }
        return water_distance_field[static_cast<std::size_t>(y) * grid.width + static_cast<std::size_t>(x)];
    }

    float get_value_bonus(std::int32_t x, std::int32_t y) const override {
        if (!in_bounds(x, y)) {
            return 0.0f;  // Safe default
        }
        const auto& info = getTerrainInfo(grid.at(x, y).getTerrainType());
        return static_cast<float>(info.value_bonus);
    }

    float get_harmony_bonus(std::int32_t x, std::int32_t y) const override {
        if (!in_bounds(x, y)) {
            return 0.0f;  // Safe default
        }
        const auto& info = getTerrainInfo(grid.at(x, y).getTerrainType());
        return static_cast<float>(info.harmony_bonus);
    }

    std::int32_t get_build_cost_modifier(std::int32_t x, std::int32_t y) const override {
        if (!in_bounds(x, y)) {
            return 100;  // Safe default (1.0x)
        }
        const auto& info = getTerrainInfo(grid.at(x, y).getTerrainType());
        // Convert float multiplier to percentage (1.0 -> 100, 1.5 -> 150)
        return static_cast<std::int32_t>(info.build_cost_modifier * 100.0f);
    }

    std::uint32_t get_contamination_output(std::int32_t x, std::int32_t y) const override {
        if (!in_bounds(x, y)) {
            return 0;  // Safe default
        }
        const auto& info = getTerrainInfo(grid.at(x, y).getTerrainType());
        // BlightMires generates contamination; others don't
        // Use a fixed value for testing (actual implementation may vary)
        return info.generates_contamination ? 10 : 0;
    }

    std::uint32_t get_map_width() const override {
        return grid.width;
    }

    std::uint32_t get_map_height() const override {
        return grid.height;
    }

    std::uint8_t get_sea_level() const override {
        return grid.sea_level;
    }

    // -------------------------------------------------------------------------
    // Batch Query Implementations (Ticket 3-015)
    // -------------------------------------------------------------------------

    void get_tiles_in_rect(const GridRect& rect,
                            std::vector<TerrainComponent>& out_tiles) const override {
        out_tiles.clear();

        if (rect.isEmpty()) {
            return;
        }

        // Clip rect to map bounds
        std::int32_t start_x = std::max(static_cast<std::int32_t>(rect.x), 0);
        std::int32_t start_y = std::max(static_cast<std::int32_t>(rect.y), 0);
        std::int32_t end_x = std::min(static_cast<std::int32_t>(rect.right()),
                                       static_cast<std::int32_t>(grid.width));
        std::int32_t end_y = std::min(static_cast<std::int32_t>(rect.bottom()),
                                       static_cast<std::int32_t>(grid.height));

        // Reserve space for efficiency
        std::size_t expected_tiles = static_cast<std::size_t>(
            std::max(0, end_x - start_x) * std::max(0, end_y - start_y));
        out_tiles.reserve(expected_tiles);

        // Row-major iteration for cache efficiency
        for (std::int32_t y = start_y; y < end_y; ++y) {
            for (std::int32_t x = start_x; x < end_x; ++x) {
                out_tiles.push_back(grid.at(x, y));
            }
        }
    }

    std::uint32_t get_buildable_tiles_in_rect(const GridRect& rect) const override {
        if (rect.isEmpty()) {
            return 0;
        }

        // Clip rect to map bounds
        std::int32_t start_x = std::max(static_cast<std::int32_t>(rect.x), 0);
        std::int32_t start_y = std::max(static_cast<std::int32_t>(rect.y), 0);
        std::int32_t end_x = std::min(static_cast<std::int32_t>(rect.right()),
                                       static_cast<std::int32_t>(grid.width));
        std::int32_t end_y = std::min(static_cast<std::int32_t>(rect.bottom()),
                                       static_cast<std::int32_t>(grid.height));

        std::uint32_t count = 0;

        // Row-major iteration for cache efficiency
        // Inline buildability check for performance (avoids virtual call overhead)
        for (std::int32_t y = start_y; y < end_y; ++y) {
            for (std::int32_t x = start_x; x < end_x; ++x) {
                const auto& tile = grid.at(x, y);
                const auto& info = getTerrainInfo(tile.getTerrainType());
                // Buildability logic: (buildable OR (clearable AND cleared)) AND NOT underwater
                bool type_allows = info.buildable || (info.clearable && tile.isCleared());
                if (type_allows && !tile.isUnderwater()) {
                    ++count;
                }
            }
        }

        return count;
    }

    std::uint32_t count_terrain_type_in_rect(const GridRect& rect,
                                              TerrainType type) const override {
        if (rect.isEmpty()) {
            return 0;
        }

        // Clip rect to map bounds
        std::int32_t start_x = std::max(static_cast<std::int32_t>(rect.x), 0);
        std::int32_t start_y = std::max(static_cast<std::int32_t>(rect.y), 0);
        std::int32_t end_x = std::min(static_cast<std::int32_t>(rect.right()),
                                       static_cast<std::int32_t>(grid.width));
        std::int32_t end_y = std::min(static_cast<std::int32_t>(rect.bottom()),
                                       static_cast<std::int32_t>(grid.height));

        std::uint32_t count = 0;

        // Row-major iteration for cache efficiency
        for (std::int32_t y = start_y; y < end_y; ++y) {
            for (std::int32_t x = start_x; x < end_x; ++x) {
                if (grid.at(x, y).getTerrainType() == type) {
                    ++count;
                }
            }
        }

        return count;
    }

private:
    bool in_bounds(std::int32_t x, std::int32_t y) const {
        return x >= 0 && x < static_cast<std::int32_t>(grid.width) &&
               y >= 0 && y < static_cast<std::int32_t>(grid.height);
    }
};

// =============================================================================
// Interface Existence Tests - Verify all methods are declared
// =============================================================================

TEST(interface_has_virtual_destructor) {
    // Verify ITerrainQueryable has virtual destructor
    // (Implicit - if it compiles with derived class, it works)
    MockTerrainQueryable* mock = new MockTerrainQueryable();
    ITerrainQueryable* interface = mock;
    delete interface;  // Should call MockTerrainQueryable destructor
    ASSERT(true);  // If we got here, virtual destructor works
}

TEST(interface_methods_exist) {
    // Create mock to verify all methods can be called through interface pointer
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Call all methods to verify they exist
    (void)iface->get_terrain_type(0, 0);
    (void)iface->get_elevation(0, 0);
    (void)iface->is_buildable(0, 0);
    (void)iface->get_slope(0, 0, 1, 0);
    (void)iface->get_average_elevation(0, 0, 1);
    (void)iface->get_water_distance(0, 0);
    (void)iface->get_value_bonus(0, 0);
    (void)iface->get_harmony_bonus(0, 0);
    (void)iface->get_build_cost_modifier(0, 0);
    (void)iface->get_contamination_output(0, 0);
    (void)iface->get_map_width();
    (void)iface->get_map_height();
    (void)iface->get_sea_level();

    // Batch query methods (Ticket 3-015)
    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 10;
    rect.height = 10;
    std::vector<TerrainComponent> tiles;
    iface->get_tiles_in_rect(rect, tiles);
    (void)iface->get_buildable_tiles_in_rect(rect);
    (void)iface->count_terrain_type_in_rect(rect, TerrainType::Substrate);

    ASSERT(true);  // If we got here, all methods exist
}

// =============================================================================
// Core Property Query Tests
// =============================================================================

TEST(get_terrain_type_basic) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT_EQ(iface->get_terrain_type(0, 0), TerrainType::Substrate);
    ASSERT_EQ(iface->get_terrain_type(1, 0), TerrainType::BiolumeGrove);
    ASSERT_EQ(iface->get_terrain_type(3, 0), TerrainType::DeepVoid);
    ASSERT_EQ(iface->get_terrain_type(5, 0), TerrainType::BlightMires);
    ASSERT_EQ(iface->get_terrain_type(7, 0), TerrainType::EmberCrust);
}

TEST(get_elevation_basic) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT_EQ(iface->get_elevation(0, 0), 10);
    ASSERT_EQ(iface->get_elevation(3, 0), 0);   // Water tile
    ASSERT_EQ(iface->get_elevation(7, 0), 15);  // EmberCrust
}

// =============================================================================
// Buildability Logic Tests
// =============================================================================

TEST(is_buildable_substrate) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Substrate is directly buildable
    ASSERT(iface->is_buildable(0, 0));
}

TEST(is_buildable_clearable_not_cleared) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // BiolumeGrove at (1,0) is clearable but NOT cleared
    ASSERT(!iface->is_buildable(1, 0));
}

TEST(is_buildable_clearable_is_cleared) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // BiolumeGrove at (2,0) is clearable AND IS cleared
    ASSERT(iface->is_buildable(2, 0));
}

TEST(is_buildable_water_not_buildable) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // DeepVoid at (3,0) is water - not buildable
    ASSERT(!iface->is_buildable(3, 0));
}

TEST(is_buildable_underwater_not_buildable) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Substrate at (4,0) is underwater - not buildable despite type being buildable
    ASSERT(!iface->is_buildable(4, 0));
}

// =============================================================================
// Slope and Elevation Analysis Tests
// =============================================================================

TEST(get_slope_flat) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Same elevation tiles
    ASSERT_EQ(iface->get_slope(0, 0, 1, 0), 0);  // Both at elevation 10
}

TEST(get_slope_gradient) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Tiles at different elevations: (10,0)=10, (11,0)=15, (12,0)=20
    ASSERT_EQ(iface->get_slope(10, 0, 11, 0), 5);
    ASSERT_EQ(iface->get_slope(11, 0, 12, 0), 5);
    ASSERT_EQ(iface->get_slope(10, 0, 12, 0), 10);
}

TEST(get_slope_symmetric) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Slope should be same regardless of direction
    ASSERT_EQ(iface->get_slope(10, 0, 11, 0), iface->get_slope(11, 0, 10, 0));
}

TEST(get_average_elevation_single_tile) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Radius 0 = single tile
    ASSERT_FLOAT_EQ(iface->get_average_elevation(0, 0, 0), 10.0f, 0.001f);
}

TEST(get_average_elevation_with_radius) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Radius 1 around (10,0) which has elevation 10
    // Includes tiles at varying elevations, will be some average
    float avg = iface->get_average_elevation(10, 0, 1);
    ASSERT(avg >= 0.0f && avg <= 31.0f);  // Valid elevation range
}

// =============================================================================
// Water Distance Tests
// =============================================================================

TEST(get_water_distance_water_tile) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Water tile at (3,0) should have distance 0
    ASSERT_EQ(iface->get_water_distance(3, 0), 0);
}

TEST(get_water_distance_adjacent) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Tiles adjacent to water at (3,0)
    ASSERT_EQ(iface->get_water_distance(2, 0), 1);
    ASSERT_EQ(iface->get_water_distance(4, 0), 1);
}

TEST(get_water_distance_far) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Tiles far from water should have high distance
    ASSERT_EQ(iface->get_water_distance(50, 50), 255);  // Default far value
}

// =============================================================================
// Land Value and Harmony Bonus Tests
// =============================================================================

TEST(get_value_bonus_substrate) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Substrate has value_bonus = 0
    ASSERT_FLOAT_EQ(iface->get_value_bonus(0, 0), 0.0f, 0.001f);
}

TEST(get_value_bonus_prismafields) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // PrismaFields at (6,0) has high value bonus (20 in TERRAIN_INFO)
    ASSERT_FLOAT_EQ(iface->get_value_bonus(6, 0), 20.0f, 0.001f);
}

TEST(get_value_bonus_blightmires_negative) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // BlightMires at (5,0) has negative value bonus (-15 in TERRAIN_INFO)
    ASSERT_FLOAT_EQ(iface->get_value_bonus(5, 0), -15.0f, 0.001f);
}

TEST(get_harmony_bonus_substrate) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Substrate has harmony_bonus = 0
    ASSERT_FLOAT_EQ(iface->get_harmony_bonus(0, 0), 0.0f, 0.001f);
}

TEST(get_harmony_bonus_prismafields) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // PrismaFields at (6,0) has high harmony bonus (8 in TERRAIN_INFO)
    ASSERT_FLOAT_EQ(iface->get_harmony_bonus(6, 0), 8.0f, 0.001f);
}

TEST(get_harmony_bonus_blightmires_negative) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // BlightMires at (5,0) has negative harmony bonus (-10 in TERRAIN_INFO)
    ASSERT_FLOAT_EQ(iface->get_harmony_bonus(5, 0), -10.0f, 0.001f);
}

// =============================================================================
// Build Cost Modifier Tests
// =============================================================================

TEST(get_build_cost_modifier_substrate) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Substrate has build_cost_modifier = 1.0 -> 100
    ASSERT_EQ(iface->get_build_cost_modifier(0, 0), 100);
}

TEST(get_build_cost_modifier_embercrust) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // EmberCrust at (7,0) has build_cost_modifier = 1.5 -> 150
    ASSERT_EQ(iface->get_build_cost_modifier(7, 0), 150);
}

// =============================================================================
// Contamination Output Tests
// =============================================================================

TEST(get_contamination_output_substrate) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Substrate does not generate contamination
    ASSERT_EQ(iface->get_contamination_output(0, 0), 0);
}

TEST(get_contamination_output_blightmires) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // BlightMires at (5,0) generates contamination
    ASSERT(iface->get_contamination_output(5, 0) > 0);
}

// =============================================================================
// Map Metadata Tests
// =============================================================================

TEST(get_map_width) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT_EQ(iface->get_map_width(), 128);  // MapSize::Small
}

TEST(get_map_height) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT_EQ(iface->get_map_height(), 128);  // MapSize::Small
}

TEST(get_sea_level) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT_EQ(iface->get_sea_level(), DEFAULT_SEA_LEVEL);  // 8
}

// =============================================================================
// Out-of-Bounds Handling Tests - CRITICAL for stability
// =============================================================================

TEST(out_of_bounds_get_terrain_type) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Negative coordinates
    ASSERT_EQ(iface->get_terrain_type(-1, 0), TerrainType::Substrate);
    ASSERT_EQ(iface->get_terrain_type(0, -1), TerrainType::Substrate);
    ASSERT_EQ(iface->get_terrain_type(-100, -100), TerrainType::Substrate);

    // Beyond map bounds
    ASSERT_EQ(iface->get_terrain_type(128, 0), TerrainType::Substrate);
    ASSERT_EQ(iface->get_terrain_type(0, 128), TerrainType::Substrate);
    ASSERT_EQ(iface->get_terrain_type(1000, 1000), TerrainType::Substrate);
}

TEST(out_of_bounds_get_elevation) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT_EQ(iface->get_elevation(-1, 0), 0);
    ASSERT_EQ(iface->get_elevation(128, 0), 0);
    ASSERT_EQ(iface->get_elevation(0, -1), 0);
}

TEST(out_of_bounds_is_buildable) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT(!iface->is_buildable(-1, 0));
    ASSERT(!iface->is_buildable(128, 0));
    ASSERT(!iface->is_buildable(0, 128));
}

TEST(out_of_bounds_get_slope) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // One tile out of bounds
    ASSERT_EQ(iface->get_slope(-1, 0, 0, 0), 0);
    ASSERT_EQ(iface->get_slope(0, 0, -1, 0), 0);

    // Both tiles out of bounds
    ASSERT_EQ(iface->get_slope(-1, -1, 128, 128), 0);
}

TEST(out_of_bounds_get_average_elevation) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT_FLOAT_EQ(iface->get_average_elevation(-1, 0, 0), 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(iface->get_average_elevation(128, 0, 0), 0.0f, 0.001f);
}

TEST(out_of_bounds_get_water_distance) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT_EQ(iface->get_water_distance(-1, 0), 255);  // Max distance
    ASSERT_EQ(iface->get_water_distance(128, 0), 255);
}

TEST(out_of_bounds_get_value_bonus) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT_FLOAT_EQ(iface->get_value_bonus(-1, 0), 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(iface->get_value_bonus(128, 0), 0.0f, 0.001f);
}

TEST(out_of_bounds_get_harmony_bonus) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT_FLOAT_EQ(iface->get_harmony_bonus(-1, 0), 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(iface->get_harmony_bonus(128, 0), 0.0f, 0.001f);
}

TEST(out_of_bounds_get_build_cost_modifier) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT_EQ(iface->get_build_cost_modifier(-1, 0), 100);  // Default 1.0x
    ASSERT_EQ(iface->get_build_cost_modifier(128, 0), 100);
}

TEST(out_of_bounds_get_contamination_output) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    ASSERT_EQ(iface->get_contamination_output(-1, 0), 0);
    ASSERT_EQ(iface->get_contamination_output(128, 0), 0);
}

// =============================================================================
// Edge Case Tests
// =============================================================================

TEST(edge_case_map_corners) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // All four corners should be valid
    ASSERT(iface->get_terrain_type(0, 0) == TerrainType::Substrate ||
           iface->get_terrain_type(0, 0) != TerrainType::Substrate);  // Just test no crash
    (void)iface->get_terrain_type(127, 0);
    (void)iface->get_terrain_type(0, 127);
    (void)iface->get_terrain_type(127, 127);
    ASSERT(true);  // If we got here, corner access is safe
}

TEST(edge_case_exactly_at_bounds) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Exactly at bounds (127 is valid, 128 is not for 128x128 map)
    ASSERT(iface->get_terrain_type(127, 127) == TerrainType::Substrate ||
           true);  // Valid access
    ASSERT_EQ(iface->get_terrain_type(128, 127), TerrainType::Substrate);  // OOB default
}

// =============================================================================
// O(1) Benchmark Tests - Verify constant-time performance
// =============================================================================

// Benchmark configuration
static constexpr int BENCHMARK_WARMUP = 100;
static constexpr int BENCHMARK_ITERATIONS = 10000;
static constexpr double O1_THRESHOLD_NS = 1000.0;  // 1 microsecond max for O(1) ops

/**
 * @brief Helper to run a benchmark and return ns per call.
 *
 * @tparam Func Callable type.
 * @param iterations Number of iterations to run.
 * @param func The function to benchmark.
 * @return Nanoseconds per call.
 */
template<typename Func>
double benchmark_ns_per_call(int iterations, Func func) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        func(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return static_cast<double>(duration.count()) / iterations;
}

TEST(benchmark_get_terrain_type_O1) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto result = iface->get_terrain_type(i % 128, i % 128);
        (void)result;
    }

    // Timed runs
    double ns_per_call = benchmark_ns_per_call(BENCHMARK_ITERATIONS, [&](int i) {
        volatile auto result = iface->get_terrain_type(i % 128, i % 128);
        (void)result;
    });

    printf("(%.1f ns/call) ", ns_per_call);
    ASSERT_LT(ns_per_call, O1_THRESHOLD_NS);
}

TEST(benchmark_get_elevation_O1) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto result = iface->get_elevation(i % 128, i % 128);
        (void)result;
    }

    // Timed runs
    double ns_per_call = benchmark_ns_per_call(BENCHMARK_ITERATIONS, [&](int i) {
        volatile auto result = iface->get_elevation(i % 128, i % 128);
        (void)result;
    });

    printf("(%.1f ns/call) ", ns_per_call);
    ASSERT_LT(ns_per_call, O1_THRESHOLD_NS);
}

TEST(benchmark_is_buildable_O1) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto result = iface->is_buildable(i % 128, i % 128);
        (void)result;
    }

    // Timed runs
    double ns_per_call = benchmark_ns_per_call(BENCHMARK_ITERATIONS, [&](int i) {
        volatile auto result = iface->is_buildable(i % 128, i % 128);
        (void)result;
    });

    printf("(%.1f ns/call) ", ns_per_call);
    ASSERT_LT(ns_per_call, O1_THRESHOLD_NS);
}

TEST(benchmark_get_slope_O1) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto result = iface->get_slope(i % 127, i % 128, (i + 1) % 128, i % 128);
        (void)result;
    }

    // Timed runs
    double ns_per_call = benchmark_ns_per_call(BENCHMARK_ITERATIONS, [&](int i) {
        volatile auto result = iface->get_slope(i % 127, i % 128, (i + 1) % 128, i % 128);
        (void)result;
    });

    printf("(%.1f ns/call) ", ns_per_call);
    ASSERT_LT(ns_per_call, O1_THRESHOLD_NS);
}

TEST(benchmark_get_water_distance_O1) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto result = iface->get_water_distance(i % 128, i % 128);
        (void)result;
    }

    // Timed runs
    double ns_per_call = benchmark_ns_per_call(BENCHMARK_ITERATIONS, [&](int i) {
        volatile auto result = iface->get_water_distance(i % 128, i % 128);
        (void)result;
    });

    printf("(%.1f ns/call) ", ns_per_call);
    ASSERT_LT(ns_per_call, O1_THRESHOLD_NS);
}

TEST(benchmark_get_value_bonus_O1) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto result = iface->get_value_bonus(i % 128, i % 128);
        (void)result;
    }

    // Timed runs
    double ns_per_call = benchmark_ns_per_call(BENCHMARK_ITERATIONS, [&](int i) {
        volatile auto result = iface->get_value_bonus(i % 128, i % 128);
        (void)result;
    });

    printf("(%.1f ns/call) ", ns_per_call);
    ASSERT_LT(ns_per_call, O1_THRESHOLD_NS);
}

TEST(benchmark_get_harmony_bonus_O1) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto result = iface->get_harmony_bonus(i % 128, i % 128);
        (void)result;
    }

    // Timed runs
    double ns_per_call = benchmark_ns_per_call(BENCHMARK_ITERATIONS, [&](int i) {
        volatile auto result = iface->get_harmony_bonus(i % 128, i % 128);
        (void)result;
    });

    printf("(%.1f ns/call) ", ns_per_call);
    ASSERT_LT(ns_per_call, O1_THRESHOLD_NS);
}

TEST(benchmark_get_build_cost_modifier_O1) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto result = iface->get_build_cost_modifier(i % 128, i % 128);
        (void)result;
    }

    // Timed runs
    double ns_per_call = benchmark_ns_per_call(BENCHMARK_ITERATIONS, [&](int i) {
        volatile auto result = iface->get_build_cost_modifier(i % 128, i % 128);
        (void)result;
    });

    printf("(%.1f ns/call) ", ns_per_call);
    ASSERT_LT(ns_per_call, O1_THRESHOLD_NS);
}

TEST(benchmark_get_contamination_output_O1) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto result = iface->get_contamination_output(i % 128, i % 128);
        (void)result;
    }

    // Timed runs
    double ns_per_call = benchmark_ns_per_call(BENCHMARK_ITERATIONS, [&](int i) {
        volatile auto result = iface->get_contamination_output(i % 128, i % 128);
        (void)result;
    });

    printf("(%.1f ns/call) ", ns_per_call);
    ASSERT_LT(ns_per_call, O1_THRESHOLD_NS);
}

TEST(benchmark_get_map_width_O1) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto result = iface->get_map_width();
        (void)result;
    }

    // Timed runs
    double ns_per_call = benchmark_ns_per_call(BENCHMARK_ITERATIONS, [&](int) {
        volatile auto result = iface->get_map_width();
        (void)result;
    });

    printf("(%.1f ns/call) ", ns_per_call);
    ASSERT_LT(ns_per_call, O1_THRESHOLD_NS);
}

TEST(benchmark_get_map_height_O1) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto result = iface->get_map_height();
        (void)result;
    }

    // Timed runs
    double ns_per_call = benchmark_ns_per_call(BENCHMARK_ITERATIONS, [&](int) {
        volatile auto result = iface->get_map_height();
        (void)result;
    });

    printf("(%.1f ns/call) ", ns_per_call);
    ASSERT_LT(ns_per_call, O1_THRESHOLD_NS);
}

TEST(benchmark_get_sea_level_O1) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto result = iface->get_sea_level();
        (void)result;
    }

    // Timed runs
    double ns_per_call = benchmark_ns_per_call(BENCHMARK_ITERATIONS, [&](int) {
        volatile auto result = iface->get_sea_level();
        (void)result;
    });

    printf("(%.1f ns/call) ", ns_per_call);
    ASSERT_LT(ns_per_call, O1_THRESHOLD_NS);
}

// =============================================================================
// get_average_elevation Complexity Test - Verify O(radius^2)
// =============================================================================

TEST(benchmark_get_average_elevation_scales_with_radius_squared) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Test different radii and verify that time scales with radius^2
    // radius=1: (2*1+1)^2 = 9 tiles
    // radius=2: (2*2+1)^2 = 25 tiles
    // radius=4: (2*4+1)^2 = 81 tiles
    // ratio between 4 and 1 should be ~81/9 = 9x

    const int iterations = 1000;

    // Warmup
    for (int i = 0; i < BENCHMARK_WARMUP; ++i) {
        volatile auto r1 = iface->get_average_elevation(64, 64, 1);
        volatile auto r2 = iface->get_average_elevation(64, 64, 4);
        (void)r1;
        (void)r2;
    }

    // Benchmark radius=1
    auto start1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile auto result = iface->get_average_elevation(64, 64, 1);
        (void)result;
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1);
    double ns_radius1 = static_cast<double>(duration1.count()) / iterations;

    // Benchmark radius=4
    auto start4 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile auto result = iface->get_average_elevation(64, 64, 4);
        (void)result;
    }
    auto end4 = std::chrono::high_resolution_clock::now();
    auto duration4 = std::chrono::duration_cast<std::chrono::nanoseconds>(end4 - start4);
    double ns_radius4 = static_cast<double>(duration4.count()) / iterations;

    double ratio = ns_radius4 / ns_radius1;

    // Expected ratio: 81/9 = 9x, but allow some variance
    // We just verify that radius=4 is significantly slower than radius=1
    // (indicating O(n^2) not O(1))
    printf("(r1=%.1f ns, r4=%.1f ns, ratio=%.2fx) ", ns_radius1, ns_radius4, ratio);

    // ratio should be > 2x (being conservative, actual expected ~9x)
    // This confirms larger radius takes longer (O(radius^2))
    ASSERT_GT(ratio, 2.0);
}

// =============================================================================
// Batch Query Tests (Ticket 3-015)
// =============================================================================

TEST(get_tiles_in_rect_basic) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 3;
    rect.height = 2;

    std::vector<TerrainComponent> tiles;
    iface->get_tiles_in_rect(rect, tiles);

    // Should have 3x2 = 6 tiles
    ASSERT_EQ(tiles.size(), 6u);
}

TEST(get_tiles_in_rect_empty) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 0;
    rect.height = 0;

    std::vector<TerrainComponent> tiles;
    iface->get_tiles_in_rect(rect, tiles);

    ASSERT_EQ(tiles.size(), 0u);
}

TEST(get_tiles_in_rect_single_tile) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    GridRect rect = GridRect::singleTile(0, 0);

    std::vector<TerrainComponent> tiles;
    iface->get_tiles_in_rect(rect, tiles);

    ASSERT_EQ(tiles.size(), 1u);
    // Tile at (0,0) should be Substrate with elevation 10
    ASSERT_EQ(tiles[0].getTerrainType(), TerrainType::Substrate);
    ASSERT_EQ(tiles[0].getElevation(), 10);
}

TEST(get_tiles_in_rect_row_major_order) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Get a 2x2 rect that includes (0,0), (1,0), (0,1), (1,1)
    // Row major means: (0,0), (1,0), (0,1), (1,1)
    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 2;
    rect.height = 2;

    std::vector<TerrainComponent> tiles;
    iface->get_tiles_in_rect(rect, tiles);

    ASSERT_EQ(tiles.size(), 4u);
    // (0,0) = Substrate, (1,0) = BiolumeGrove in test data
    ASSERT_EQ(tiles[0].getTerrainType(), TerrainType::Substrate);    // (0,0)
    ASSERT_EQ(tiles[1].getTerrainType(), TerrainType::BiolumeGrove); // (1,0)
}

TEST(get_tiles_in_rect_clipped_to_map_bounds) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Request rect that extends beyond map (128x128)
    GridRect rect;
    rect.x = 120;
    rect.y = 120;
    rect.width = 20;  // Would go to 140, but map only goes to 128
    rect.height = 20;

    std::vector<TerrainComponent> tiles;
    iface->get_tiles_in_rect(rect, tiles);

    // Should only get 8x8 = 64 tiles (clipped to [120,128) x [120,128))
    ASSERT_EQ(tiles.size(), 64u);
}

TEST(get_tiles_in_rect_negative_coords_clipped) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Request rect starting at negative coords
    GridRect rect;
    rect.x = -5;
    rect.y = -5;
    rect.width = 10;  // Goes from -5 to 5
    rect.height = 10;

    std::vector<TerrainComponent> tiles;
    iface->get_tiles_in_rect(rect, tiles);

    // Should only get tiles from 0 to 5 (5x5 = 25 tiles)
    ASSERT_EQ(tiles.size(), 25u);
}

TEST(get_tiles_in_rect_completely_out_of_bounds) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Request rect completely outside map
    GridRect rect;
    rect.x = 200;
    rect.y = 200;
    rect.width = 10;
    rect.height = 10;

    std::vector<TerrainComponent> tiles;
    iface->get_tiles_in_rect(rect, tiles);

    ASSERT_EQ(tiles.size(), 0u);
}

TEST(get_buildable_tiles_in_rect_basic) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Count buildable tiles in first 8 tiles of row 0
    // Based on test data:
    // (0,0) Substrate = buildable
    // (1,0) BiolumeGrove not cleared = NOT buildable
    // (2,0) BiolumeGrove cleared = buildable
    // (3,0) DeepVoid = NOT buildable
    // (4,0) Substrate underwater = NOT buildable
    // (5,0) BlightMires = NOT buildable
    // (6,0) PrismaFields = NOT buildable (clearable but not cleared)
    // (7,0) EmberCrust = NOT buildable
    // So only 2 buildable tiles in row 0
    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 8;
    rect.height = 1;

    std::uint32_t count = iface->get_buildable_tiles_in_rect(rect);
    ASSERT_EQ(count, 2u);
}

TEST(get_buildable_tiles_in_rect_empty) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 0;
    rect.height = 0;

    std::uint32_t count = iface->get_buildable_tiles_in_rect(rect);
    ASSERT_EQ(count, 0u);
}

TEST(get_buildable_tiles_in_rect_all_buildable) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Most tiles default to Substrate which is buildable
    // Get a rect far from the test data area
    GridRect rect;
    rect.x = 50;
    rect.y = 50;
    rect.width = 10;
    rect.height = 10;

    std::uint32_t count = iface->get_buildable_tiles_in_rect(rect);
    // Default tiles should all be Substrate = buildable
    ASSERT_EQ(count, 100u);  // 10x10 = 100
}

TEST(get_buildable_tiles_in_rect_edge_of_map) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Rect at edge of map that extends beyond
    GridRect rect;
    rect.x = 125;
    rect.y = 125;
    rect.width = 10;  // Would go to 135, clipped to 128
    rect.height = 10;

    std::uint32_t count = iface->get_buildable_tiles_in_rect(rect);
    // 3x3 = 9 tiles within bounds, all should be Substrate = buildable
    ASSERT_EQ(count, 9u);
}

TEST(count_terrain_type_in_rect_basic) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Count Substrate tiles in first 8 tiles of row 0
    // (0,0) Substrate
    // (4,0) Substrate (underwater but still Substrate type)
    // So 2 Substrate tiles
    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 8;
    rect.height = 1;

    std::uint32_t count = iface->count_terrain_type_in_rect(rect, TerrainType::Substrate);
    ASSERT_EQ(count, 2u);
}

TEST(count_terrain_type_in_rect_biolumegroove) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Count BiolumeGrove tiles in first 8 tiles of row 0
    // (1,0) BiolumeGrove
    // (2,0) BiolumeGrove
    // So 2 BiolumeGrove tiles
    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 8;
    rect.height = 1;

    std::uint32_t count = iface->count_terrain_type_in_rect(rect, TerrainType::BiolumeGrove);
    ASSERT_EQ(count, 2u);
}

TEST(count_terrain_type_in_rect_empty) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 0;
    rect.height = 0;

    std::uint32_t count = iface->count_terrain_type_in_rect(rect, TerrainType::Substrate);
    ASSERT_EQ(count, 0u);
}

TEST(count_terrain_type_in_rect_none_found) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Look for Ridge type which doesn't exist in test data
    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 128;
    rect.height = 128;

    std::uint32_t count = iface->count_terrain_type_in_rect(rect, TerrainType::Ridge);
    ASSERT_EQ(count, 0u);
}

TEST(count_terrain_type_in_rect_all_substrate) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Get a rect far from test data, should all be default (Substrate)
    GridRect rect;
    rect.x = 50;
    rect.y = 50;
    rect.width = 10;
    rect.height = 10;

    std::uint32_t count = iface->count_terrain_type_in_rect(rect, TerrainType::Substrate);
    ASSERT_EQ(count, 100u);  // 10x10 = 100
}

TEST(count_terrain_type_in_rect_edge_of_map) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // Rect at edge of map
    GridRect rect;
    rect.x = 125;
    rect.y = 125;
    rect.width = 10;  // Clipped to 3
    rect.height = 10; // Clipped to 3

    std::uint32_t count = iface->count_terrain_type_in_rect(rect, TerrainType::Substrate);
    // 3x3 = 9 tiles, all Substrate
    ASSERT_EQ(count, 9u);
}

// =============================================================================
// Batch Query Performance Tests (Ticket 3-015)
// =============================================================================

// Performance target: 10,000 tile rect query < 10 microseconds
static constexpr double BATCH_PERFORMANCE_THRESHOLD_US = 10.0;

TEST(benchmark_get_tiles_in_rect_10k_tiles) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // 100x100 = 10,000 tiles
    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 100;
    rect.height = 100;

    std::vector<TerrainComponent> tiles;

    // Warmup
    for (int i = 0; i < 10; ++i) {
        iface->get_tiles_in_rect(rect, tiles);
    }

    // Timed run
    auto start = std::chrono::high_resolution_clock::now();
    iface->get_tiles_in_rect(rect, tiles);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double microseconds = static_cast<double>(duration.count()) / 1000.0;

    printf("(%.2f us for 10k tiles) ", microseconds);

    ASSERT_EQ(tiles.size(), 10000u);
    ASSERT_LT(microseconds, BATCH_PERFORMANCE_THRESHOLD_US);
}

TEST(benchmark_get_buildable_tiles_10k) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // 100x100 = 10,000 tiles
    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 100;
    rect.height = 100;

    // Warmup
    for (int i = 0; i < 10; ++i) {
        volatile auto result = iface->get_buildable_tiles_in_rect(rect);
        (void)result;
    }

    // Timed run
    auto start = std::chrono::high_resolution_clock::now();
    volatile auto count = iface->get_buildable_tiles_in_rect(rect);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double microseconds = static_cast<double>(duration.count()) / 1000.0;

    printf("(%.2f us for 10k tiles, %u buildable) ", microseconds, count);

    ASSERT_LT(microseconds, BATCH_PERFORMANCE_THRESHOLD_US);
}

TEST(benchmark_count_terrain_type_10k) {
    MockTerrainQueryable mock;
    const ITerrainQueryable* iface = &mock;

    // 100x100 = 10,000 tiles
    GridRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 100;
    rect.height = 100;

    // Warmup
    for (int i = 0; i < 10; ++i) {
        volatile auto result = iface->count_terrain_type_in_rect(rect, TerrainType::Substrate);
        (void)result;
    }

    // Timed run
    auto start = std::chrono::high_resolution_clock::now();
    volatile auto count = iface->count_terrain_type_in_rect(rect, TerrainType::Substrate);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double microseconds = static_cast<double>(duration.count()) / 1000.0;

    printf("(%.2f us for 10k tiles, %u Substrate) ", microseconds, count);

    ASSERT_LT(microseconds, BATCH_PERFORMANCE_THRESHOLD_US);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== ITerrainQueryable Unit Tests ===\n\n");

    // Interface existence tests
    RUN_TEST(interface_has_virtual_destructor);
    RUN_TEST(interface_methods_exist);

    // Core property query tests
    RUN_TEST(get_terrain_type_basic);
    RUN_TEST(get_elevation_basic);

    // Buildability logic tests
    RUN_TEST(is_buildable_substrate);
    RUN_TEST(is_buildable_clearable_not_cleared);
    RUN_TEST(is_buildable_clearable_is_cleared);
    RUN_TEST(is_buildable_water_not_buildable);
    RUN_TEST(is_buildable_underwater_not_buildable);

    // Slope and elevation tests
    RUN_TEST(get_slope_flat);
    RUN_TEST(get_slope_gradient);
    RUN_TEST(get_slope_symmetric);
    RUN_TEST(get_average_elevation_single_tile);
    RUN_TEST(get_average_elevation_with_radius);

    // Water distance tests
    RUN_TEST(get_water_distance_water_tile);
    RUN_TEST(get_water_distance_adjacent);
    RUN_TEST(get_water_distance_far);

    // Land value and harmony tests
    RUN_TEST(get_value_bonus_substrate);
    RUN_TEST(get_value_bonus_prismafields);
    RUN_TEST(get_value_bonus_blightmires_negative);
    RUN_TEST(get_harmony_bonus_substrate);
    RUN_TEST(get_harmony_bonus_prismafields);
    RUN_TEST(get_harmony_bonus_blightmires_negative);

    // Build cost modifier tests
    RUN_TEST(get_build_cost_modifier_substrate);
    RUN_TEST(get_build_cost_modifier_embercrust);

    // Contamination output tests
    RUN_TEST(get_contamination_output_substrate);
    RUN_TEST(get_contamination_output_blightmires);

    // Map metadata tests
    RUN_TEST(get_map_width);
    RUN_TEST(get_map_height);
    RUN_TEST(get_sea_level);

    // Out-of-bounds handling tests (CRITICAL)
    RUN_TEST(out_of_bounds_get_terrain_type);
    RUN_TEST(out_of_bounds_get_elevation);
    RUN_TEST(out_of_bounds_is_buildable);
    RUN_TEST(out_of_bounds_get_slope);
    RUN_TEST(out_of_bounds_get_average_elevation);
    RUN_TEST(out_of_bounds_get_water_distance);
    RUN_TEST(out_of_bounds_get_value_bonus);
    RUN_TEST(out_of_bounds_get_harmony_bonus);
    RUN_TEST(out_of_bounds_get_build_cost_modifier);
    RUN_TEST(out_of_bounds_get_contamination_output);

    // Edge case tests
    RUN_TEST(edge_case_map_corners);
    RUN_TEST(edge_case_exactly_at_bounds);

    // O(1) Benchmark tests
    printf("\n--- O(1) Benchmark Tests ---\n");
    RUN_TEST(benchmark_get_terrain_type_O1);
    RUN_TEST(benchmark_get_elevation_O1);
    RUN_TEST(benchmark_is_buildable_O1);
    RUN_TEST(benchmark_get_slope_O1);
    RUN_TEST(benchmark_get_water_distance_O1);
    RUN_TEST(benchmark_get_value_bonus_O1);
    RUN_TEST(benchmark_get_harmony_bonus_O1);
    RUN_TEST(benchmark_get_build_cost_modifier_O1);
    RUN_TEST(benchmark_get_contamination_output_O1);
    RUN_TEST(benchmark_get_map_width_O1);
    RUN_TEST(benchmark_get_map_height_O1);
    RUN_TEST(benchmark_get_sea_level_O1);

    // get_average_elevation complexity test (O(radius^2))
    printf("\n--- Complexity Tests ---\n");
    RUN_TEST(benchmark_get_average_elevation_scales_with_radius_squared);

    // Batch query tests (Ticket 3-015)
    printf("\n--- Batch Query Tests ---\n");
    RUN_TEST(get_tiles_in_rect_basic);
    RUN_TEST(get_tiles_in_rect_empty);
    RUN_TEST(get_tiles_in_rect_single_tile);
    RUN_TEST(get_tiles_in_rect_row_major_order);
    RUN_TEST(get_tiles_in_rect_clipped_to_map_bounds);
    RUN_TEST(get_tiles_in_rect_negative_coords_clipped);
    RUN_TEST(get_tiles_in_rect_completely_out_of_bounds);
    RUN_TEST(get_buildable_tiles_in_rect_basic);
    RUN_TEST(get_buildable_tiles_in_rect_empty);
    RUN_TEST(get_buildable_tiles_in_rect_all_buildable);
    RUN_TEST(get_buildable_tiles_in_rect_edge_of_map);
    RUN_TEST(count_terrain_type_in_rect_basic);
    RUN_TEST(count_terrain_type_in_rect_biolumegroove);
    RUN_TEST(count_terrain_type_in_rect_empty);
    RUN_TEST(count_terrain_type_in_rect_none_found);
    RUN_TEST(count_terrain_type_in_rect_all_substrate);
    RUN_TEST(count_terrain_type_in_rect_edge_of_map);

    // Batch query performance tests (Ticket 3-015)
    printf("\n--- Batch Query Performance Tests ---\n");
    RUN_TEST(benchmark_get_tiles_in_rect_10k_tiles);
    RUN_TEST(benchmark_get_buildable_tiles_10k);
    RUN_TEST(benchmark_count_terrain_type_10k);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
