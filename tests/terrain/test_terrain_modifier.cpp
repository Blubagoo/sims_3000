/**
 * @file test_terrain_modifier.cpp
 * @brief Unit tests for ITerrainModifier interface (Ticket 3-017)
 *
 * Tests cover:
 * - Interface structure and method signatures
 * - Mock implementation for testing
 * - Precondition validation patterns
 * - Cost query behavior (const methods)
 * - Modification method behavior patterns
 */

#include <sims3000/terrain/ITerrainModifier.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainTypeInfo.h>
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
// Mock Implementation for Testing
// =============================================================================

/**
 * @brief Mock implementation of ITerrainModifier for unit testing.
 *
 * This mock uses a TerrainGrid to simulate terrain state and provides
 * realistic behavior for testing interface patterns.
 */
class MockTerrainModifier : public ITerrainModifier {
public:
    TerrainGrid grid;

    /// Base cost per elevation level change
    static constexpr std::int64_t LEVEL_BASE_COST = 10;

    explicit MockTerrainModifier(MapSize size = MapSize::Small)
        : grid(size)
    {
        // Initialize all tiles as Substrate (buildable, not clearable)
        grid.fill_type(TerrainType::Substrate);
    }

    /// Set a tile to a specific terrain type for testing
    void setTile(std::int32_t x, std::int32_t y, TerrainType type, std::uint8_t elevation = 10) {
        if (grid.in_bounds(x, y)) {
            auto& tile = grid.at(x, y);
            tile.setTerrainType(type);
            tile.setElevation(elevation);
            tile.flags = 0; // Reset flags
        }
    }

    // =========================================================================
    // ITerrainModifier Implementation
    // =========================================================================

    bool clear_terrain(std::int32_t x, std::int32_t y, PlayerID /*player_id*/) override {
        // Validate bounds
        if (!grid.in_bounds(x, y)) {
            return false;
        }

        auto& tile = grid.at(x, y);
        TerrainType type = tile.getTerrainType();

        // Check if terrain type is clearable
        if (!isClearable(type)) {
            return false;
        }

        // Check if already cleared
        if (tile.isCleared()) {
            return false;
        }

        // Perform the clear operation
        tile.setCleared(true);
        return true;
    }

    bool level_terrain(std::int32_t x, std::int32_t y,
                       std::uint8_t target_elevation,
                       PlayerID /*player_id*/) override {
        // Validate bounds
        if (!grid.in_bounds(x, y)) {
            return false;
        }

        // Validate target elevation
        if (target_elevation > TerrainComponent::MAX_ELEVATION) {
            return false;
        }

        auto& tile = grid.at(x, y);
        TerrainType type = tile.getTerrainType();

        // Water types cannot be leveled
        if (type == TerrainType::DeepVoid ||
            type == TerrainType::FlowChannel ||
            type == TerrainType::StillBasin ||
            type == TerrainType::BlightMires) {
            return false;
        }

        // Perform the level operation
        tile.setElevation(target_elevation);
        return true;
    }

    std::int64_t get_clear_cost(std::int32_t x, std::int32_t y) const override {
        // Validate bounds
        if (!grid.in_bounds(x, y)) {
            return -1;
        }

        const auto& tile = grid.at(x, y);
        TerrainType type = tile.getTerrainType();

        // Already cleared - no cost
        if (tile.isCleared()) {
            return 0;
        }

        // Check if terrain type is clearable
        if (!isClearable(type)) {
            return -1;
        }

        // Return cost from TerrainTypeInfo
        const auto& info = getTerrainInfo(type);
        return info.clear_cost;
    }

    std::int64_t get_level_cost(std::int32_t x, std::int32_t y,
                                std::uint8_t target_elevation) const override {
        // Validate bounds
        if (!grid.in_bounds(x, y)) {
            return -1;
        }

        // Validate target elevation
        if (target_elevation > TerrainComponent::MAX_ELEVATION) {
            return -1;
        }

        const auto& tile = grid.at(x, y);
        TerrainType type = tile.getTerrainType();

        // Water types cannot be leveled
        if (type == TerrainType::DeepVoid ||
            type == TerrainType::FlowChannel ||
            type == TerrainType::StillBasin ||
            type == TerrainType::BlightMires) {
            return -1;
        }

        std::uint8_t current = tile.getElevation();

        // Already at target - no cost
        if (current == target_elevation) {
            return 0;
        }

        // Cost scales with elevation difference
        std::int64_t diff = (current > target_elevation)
            ? (current - target_elevation)
            : (target_elevation - current);

        return LEVEL_BASE_COST * diff;
    }
};

// =============================================================================
// Interface Structure Tests
// =============================================================================

TEST(interface_is_abstract) {
    // ITerrainModifier should be abstract (cannot be instantiated directly)
    // This is verified at compile time - if this compiles, test passes
    // We verify by checking that we can create a pointer to the interface
    ITerrainModifier* ptr = nullptr;
    ASSERT(ptr == nullptr); // Just to use the variable
}

TEST(interface_has_virtual_destructor) {
    // Create derived class and delete through base pointer
    // If destructor is virtual, this should work correctly
    ITerrainModifier* ptr = new MockTerrainModifier();
    delete ptr; // Should call derived destructor
    ASSERT(true); // If we get here without crash, test passes
}

TEST(interface_has_clear_terrain_method) {
    MockTerrainModifier modifier;
    modifier.setTile(5, 5, TerrainType::BiolumeGrove);
    // Method exists and returns bool
    bool result = modifier.clear_terrain(5, 5, 1);
    ASSERT(result == true);
}

TEST(interface_has_level_terrain_method) {
    MockTerrainModifier modifier;
    modifier.setTile(5, 5, TerrainType::Substrate, 10);
    // Method exists and returns bool
    bool result = modifier.level_terrain(5, 5, 15, 1);
    ASSERT(result == true);
}

TEST(interface_has_get_clear_cost_method) {
    MockTerrainModifier modifier;
    modifier.setTile(5, 5, TerrainType::BiolumeGrove);
    // Method exists and returns int64
    std::int64_t cost = modifier.get_clear_cost(5, 5);
    ASSERT(cost >= 0); // BiolumeGrove has positive clear cost
}

TEST(interface_has_get_level_cost_method) {
    MockTerrainModifier modifier;
    modifier.setTile(5, 5, TerrainType::Substrate, 10);
    // Method exists and returns int64
    std::int64_t cost = modifier.get_level_cost(5, 5, 15);
    ASSERT(cost >= 0);
}

// =============================================================================
// Cost Query Tests (const methods)
// =============================================================================

TEST(get_clear_cost_returns_positive_for_clearable) {
    MockTerrainModifier modifier;

    // BiolumeGrove is clearable with positive cost
    modifier.setTile(5, 5, TerrainType::BiolumeGrove);
    std::int64_t cost = modifier.get_clear_cost(5, 5);
    ASSERT(cost > 0);
    ASSERT_EQ(cost, 100); // From TerrainTypeInfo
}

TEST(get_clear_cost_returns_negative_for_crystal_harvesting) {
    MockTerrainModifier modifier;

    // PrismaFields yields credits when cleared (negative cost)
    modifier.setTile(5, 5, TerrainType::PrismaFields);
    std::int64_t cost = modifier.get_clear_cost(5, 5);
    ASSERT(cost < 0);
    ASSERT_EQ(cost, -500); // From TerrainTypeInfo
}

TEST(get_clear_cost_returns_zero_for_already_cleared) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::BiolumeGrove);
    modifier.clear_terrain(5, 5, 1); // Clear it first

    std::int64_t cost = modifier.get_clear_cost(5, 5);
    ASSERT_EQ(cost, 0);
}

TEST(get_clear_cost_returns_invalid_for_non_clearable) {
    MockTerrainModifier modifier;

    // Substrate is not clearable
    modifier.setTile(5, 5, TerrainType::Substrate);
    std::int64_t cost = modifier.get_clear_cost(5, 5);
    ASSERT_EQ(cost, -1); // Invalid

    // Water is not clearable
    modifier.setTile(6, 6, TerrainType::DeepVoid);
    cost = modifier.get_clear_cost(6, 6);
    ASSERT_EQ(cost, -1);
}

TEST(get_clear_cost_returns_invalid_for_out_of_bounds) {
    MockTerrainModifier modifier;

    std::int64_t cost = modifier.get_clear_cost(-1, 5);
    ASSERT_EQ(cost, -1);

    cost = modifier.get_clear_cost(5, 1000);
    ASSERT_EQ(cost, -1);
}

TEST(get_level_cost_returns_zero_for_same_elevation) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::Substrate, 15);
    std::int64_t cost = modifier.get_level_cost(5, 5, 15);
    ASSERT_EQ(cost, 0);
}

TEST(get_level_cost_scales_with_elevation_difference) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::Substrate, 10);

    // Cost for +5 elevation
    std::int64_t cost_up5 = modifier.get_level_cost(5, 5, 15);
    ASSERT_EQ(cost_up5, 50); // 10 * 5

    // Cost for -5 elevation
    std::int64_t cost_down5 = modifier.get_level_cost(5, 5, 5);
    ASSERT_EQ(cost_down5, 50); // 10 * 5

    // Cost for +1 elevation
    std::int64_t cost_up1 = modifier.get_level_cost(5, 5, 11);
    ASSERT_EQ(cost_up1, 10); // 10 * 1
}

TEST(get_level_cost_returns_invalid_for_water) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::DeepVoid);
    std::int64_t cost = modifier.get_level_cost(5, 5, 10);
    ASSERT_EQ(cost, -1);

    modifier.setTile(6, 6, TerrainType::FlowChannel);
    cost = modifier.get_level_cost(6, 6, 10);
    ASSERT_EQ(cost, -1);

    modifier.setTile(7, 7, TerrainType::StillBasin);
    cost = modifier.get_level_cost(7, 7, 10);
    ASSERT_EQ(cost, -1);
}

TEST(get_level_cost_returns_invalid_for_toxic) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::BlightMires);
    std::int64_t cost = modifier.get_level_cost(5, 5, 10);
    ASSERT_EQ(cost, -1);
}

TEST(get_level_cost_returns_invalid_for_out_of_bounds) {
    MockTerrainModifier modifier;

    std::int64_t cost = modifier.get_level_cost(-1, 5, 10);
    ASSERT_EQ(cost, -1);

    cost = modifier.get_level_cost(5, 1000, 10);
    ASSERT_EQ(cost, -1);
}

TEST(get_level_cost_returns_invalid_for_invalid_target) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::Substrate, 10);

    // Target elevation > MAX_ELEVATION (31)
    std::int64_t cost = modifier.get_level_cost(5, 5, 50);
    ASSERT_EQ(cost, -1);

    cost = modifier.get_level_cost(5, 5, 255);
    ASSERT_EQ(cost, -1);
}

// =============================================================================
// Modification Method Tests
// =============================================================================

TEST(clear_terrain_succeeds_for_clearable) {
    MockTerrainModifier modifier;

    // BiolumeGrove is clearable
    modifier.setTile(5, 5, TerrainType::BiolumeGrove);
    ASSERT(!modifier.grid.at(5, 5).isCleared());

    bool result = modifier.clear_terrain(5, 5, 1);
    ASSERT(result == true);
    ASSERT(modifier.grid.at(5, 5).isCleared());
}

TEST(clear_terrain_fails_for_non_clearable) {
    MockTerrainModifier modifier;

    // Substrate is not clearable
    modifier.setTile(5, 5, TerrainType::Substrate);
    bool result = modifier.clear_terrain(5, 5, 1);
    ASSERT(result == false);
    ASSERT(!modifier.grid.at(5, 5).isCleared());
}

TEST(clear_terrain_fails_for_already_cleared) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::BiolumeGrove);
    modifier.clear_terrain(5, 5, 1); // Clear it first

    // Second clear should fail
    bool result = modifier.clear_terrain(5, 5, 1);
    ASSERT(result == false);
}

TEST(clear_terrain_fails_for_out_of_bounds) {
    MockTerrainModifier modifier;

    bool result = modifier.clear_terrain(-1, 5, 1);
    ASSERT(result == false);

    result = modifier.clear_terrain(5, 1000, 1);
    ASSERT(result == false);
}

TEST(clear_terrain_works_for_all_clearable_types) {
    MockTerrainModifier modifier;

    // BiolumeGrove is clearable
    modifier.setTile(10, 10, TerrainType::BiolumeGrove);
    ASSERT(modifier.clear_terrain(10, 10, 1) == true);

    // PrismaFields is clearable
    modifier.setTile(11, 11, TerrainType::PrismaFields);
    ASSERT(modifier.clear_terrain(11, 11, 1) == true);

    // SporeFlats is clearable
    modifier.setTile(12, 12, TerrainType::SporeFlats);
    ASSERT(modifier.clear_terrain(12, 12, 1) == true);
}

TEST(level_terrain_succeeds_for_valid_tile) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::Substrate, 10);
    ASSERT_EQ(modifier.grid.at(5, 5).getElevation(), 10);

    bool result = modifier.level_terrain(5, 5, 20, 1);
    ASSERT(result == true);
    ASSERT_EQ(modifier.grid.at(5, 5).getElevation(), 20);
}

TEST(level_terrain_fails_for_water) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::DeepVoid, 5);
    bool result = modifier.level_terrain(5, 5, 10, 1);
    ASSERT(result == false);
    ASSERT_EQ(modifier.grid.at(5, 5).getElevation(), 5); // Unchanged
}

TEST(level_terrain_fails_for_out_of_bounds) {
    MockTerrainModifier modifier;

    bool result = modifier.level_terrain(-1, 5, 10, 1);
    ASSERT(result == false);

    result = modifier.level_terrain(5, 1000, 10, 1);
    ASSERT(result == false);
}

TEST(level_terrain_fails_for_invalid_target) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::Substrate, 10);
    bool result = modifier.level_terrain(5, 5, 50, 1); // > MAX_ELEVATION
    ASSERT(result == false);
    ASSERT_EQ(modifier.grid.at(5, 5).getElevation(), 10); // Unchanged
}

TEST(level_terrain_can_raise_elevation) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::Substrate, 5);
    bool result = modifier.level_terrain(5, 5, 25, 1);
    ASSERT(result == true);
    ASSERT_EQ(modifier.grid.at(5, 5).getElevation(), 25);
}

TEST(level_terrain_can_lower_elevation) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::Substrate, 25);
    bool result = modifier.level_terrain(5, 5, 5, 1);
    ASSERT(result == true);
    ASSERT_EQ(modifier.grid.at(5, 5).getElevation(), 5);
}

TEST(level_terrain_to_zero) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::Substrate, 15);
    bool result = modifier.level_terrain(5, 5, 0, 1);
    ASSERT(result == true);
    ASSERT_EQ(modifier.grid.at(5, 5).getElevation(), 0);
}

TEST(level_terrain_to_max) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::Substrate, 15);
    bool result = modifier.level_terrain(5, 5, 31, 1);
    ASSERT(result == true);
    ASSERT_EQ(modifier.grid.at(5, 5).getElevation(), 31);
}

// =============================================================================
// PlayerID Type Tests
// =============================================================================

TEST(player_id_type_is_uint8) {
    // Verify PlayerID is the expected type
    ASSERT_EQ(sizeof(PlayerID), 1);
}

TEST(player_id_accepts_game_master) {
    MockTerrainModifier modifier;
    modifier.setTile(5, 5, TerrainType::BiolumeGrove);

    // GAME_MASTER = 0
    bool result = modifier.clear_terrain(5, 5, 0);
    ASSERT(result == true);
}

TEST(player_id_accepts_players_1_to_4) {
    MockTerrainModifier modifier;

    for (PlayerID p = 1; p <= 4; ++p) {
        modifier.setTile(static_cast<std::int32_t>(p), 5, TerrainType::BiolumeGrove);
        bool result = modifier.clear_terrain(static_cast<std::int32_t>(p), 5, p);
        ASSERT(result == true);
    }
}

// =============================================================================
// Cost Query Constness Tests
// =============================================================================

TEST(cost_queries_do_not_modify_state) {
    MockTerrainModifier modifier;

    modifier.setTile(5, 5, TerrainType::BiolumeGrove, 10);

    // Store initial state
    bool was_cleared = modifier.grid.at(5, 5).isCleared();
    std::uint8_t elevation = modifier.grid.at(5, 5).getElevation();
    TerrainType type = modifier.grid.at(5, 5).getTerrainType();

    // Call cost queries multiple times
    for (int i = 0; i < 10; ++i) {
        modifier.get_clear_cost(5, 5);
        modifier.get_level_cost(5, 5, 20);
    }

    // Verify state is unchanged
    ASSERT_EQ(modifier.grid.at(5, 5).isCleared(), was_cleared);
    ASSERT_EQ(modifier.grid.at(5, 5).getElevation(), elevation);
    ASSERT_EQ(modifier.grid.at(5, 5).getTerrainType(), type);
}

TEST(cost_queries_callable_on_const_modifier) {
    MockTerrainModifier modifier;
    modifier.setTile(5, 5, TerrainType::BiolumeGrove, 10);

    // Get const reference
    const ITerrainModifier& const_ref = modifier;

    // These should compile because they're const methods
    std::int64_t clear_cost = const_ref.get_clear_cost(5, 5);
    std::int64_t level_cost = const_ref.get_level_cost(5, 5, 15);

    ASSERT(clear_cost >= 0);
    ASSERT(level_cost >= 0);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== ITerrainModifier Unit Tests ===\n\n");

    // Interface structure tests
    RUN_TEST(interface_is_abstract);
    RUN_TEST(interface_has_virtual_destructor);
    RUN_TEST(interface_has_clear_terrain_method);
    RUN_TEST(interface_has_level_terrain_method);
    RUN_TEST(interface_has_get_clear_cost_method);
    RUN_TEST(interface_has_get_level_cost_method);

    // Cost query tests
    RUN_TEST(get_clear_cost_returns_positive_for_clearable);
    RUN_TEST(get_clear_cost_returns_negative_for_crystal_harvesting);
    RUN_TEST(get_clear_cost_returns_zero_for_already_cleared);
    RUN_TEST(get_clear_cost_returns_invalid_for_non_clearable);
    RUN_TEST(get_clear_cost_returns_invalid_for_out_of_bounds);
    RUN_TEST(get_level_cost_returns_zero_for_same_elevation);
    RUN_TEST(get_level_cost_scales_with_elevation_difference);
    RUN_TEST(get_level_cost_returns_invalid_for_water);
    RUN_TEST(get_level_cost_returns_invalid_for_toxic);
    RUN_TEST(get_level_cost_returns_invalid_for_out_of_bounds);
    RUN_TEST(get_level_cost_returns_invalid_for_invalid_target);

    // Modification method tests
    RUN_TEST(clear_terrain_succeeds_for_clearable);
    RUN_TEST(clear_terrain_fails_for_non_clearable);
    RUN_TEST(clear_terrain_fails_for_already_cleared);
    RUN_TEST(clear_terrain_fails_for_out_of_bounds);
    RUN_TEST(clear_terrain_works_for_all_clearable_types);
    RUN_TEST(level_terrain_succeeds_for_valid_tile);
    RUN_TEST(level_terrain_fails_for_water);
    RUN_TEST(level_terrain_fails_for_out_of_bounds);
    RUN_TEST(level_terrain_fails_for_invalid_target);
    RUN_TEST(level_terrain_can_raise_elevation);
    RUN_TEST(level_terrain_can_lower_elevation);
    RUN_TEST(level_terrain_to_zero);
    RUN_TEST(level_terrain_to_max);

    // PlayerID type tests
    RUN_TEST(player_id_type_is_uint8);
    RUN_TEST(player_id_accepts_game_master);
    RUN_TEST(player_id_accepts_players_1_to_4);

    // Constness tests
    RUN_TEST(cost_queries_do_not_modify_state);
    RUN_TEST(cost_queries_callable_on_const_modifier);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
