/**
 * @file test_conduit_placement.cpp
 * @brief Unit tests for fluid conduit placement and validation (Ticket 6-029)
 *
 * Tests cover:
 * - Place conduit, verify entity created
 * - Place conduit, verify dirty flag set
 * - Place conduit, verify event emitted
 * - Place conduit out of bounds, verify failure
 * - Multiple conduit placements
 * - Validate conduit placement (bounds, owner, terrain)
 * - Component defaults (coverage_radius=3, is_connected=false, is_active=false, conduit_level=1)
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidConduitComponent.h>
#include <sims3000/fluid/FluidEvents.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::fluid;

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
// Stub terrain for testing
// =============================================================================

class StubTerrain : public sims3000::terrain::ITerrainQueryable {
public:
    bool buildable_value = true;

    sims3000::terrain::TerrainType get_terrain_type(
        std::int32_t, std::int32_t) const override {
        return sims3000::terrain::TerrainType::Substrate;
    }
    std::uint8_t get_elevation(std::int32_t, std::int32_t) const override {
        return 10;
    }
    bool is_buildable(std::int32_t, std::int32_t) const override {
        return buildable_value;
    }
    std::uint8_t get_slope(std::int32_t, std::int32_t,
                           std::int32_t, std::int32_t) const override {
        return 0;
    }
    float get_average_elevation(std::int32_t, std::int32_t,
                                std::uint32_t) const override {
        return 10.0f;
    }
    std::uint32_t get_water_distance(std::int32_t, std::int32_t) const override {
        return 255;
    }
    float get_value_bonus(std::int32_t, std::int32_t) const override {
        return 0.0f;
    }
    float get_harmony_bonus(std::int32_t, std::int32_t) const override {
        return 0.0f;
    }
    std::int32_t get_build_cost_modifier(std::int32_t, std::int32_t) const override {
        return 100;
    }
    std::uint32_t get_contamination_output(std::int32_t, std::int32_t) const override {
        return 0;
    }
    std::uint32_t get_map_width() const override { return 128; }
    std::uint32_t get_map_height() const override { return 128; }
    std::uint8_t get_sea_level() const override { return 8; }
    void get_tiles_in_rect(const sims3000::terrain::GridRect&,
                           std::vector<sims3000::terrain::TerrainComponent>& out_tiles) const override {
        out_tiles.clear();
    }
    std::uint32_t get_buildable_tiles_in_rect(
        const sims3000::terrain::GridRect&) const override {
        return 0;
    }
    std::uint32_t count_terrain_type_in_rect(
        const sims3000::terrain::GridRect&,
        sims3000::terrain::TerrainType) const override {
        return 0;
    }
};

// =============================================================================
// place_conduit: Entity creation
// =============================================================================

TEST(place_conduit_creates_entity) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    auto entity = static_cast<entt::entity>(eid);
    ASSERT(registry.valid(entity));
}

TEST(place_conduit_has_conduit_component) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    auto entity = static_cast<entt::entity>(eid);

    ASSERT(registry.all_of<FluidConduitComponent>(entity));
}

TEST(place_conduit_component_defaults) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    auto entity = static_cast<entt::entity>(eid);

    const auto& conduit = registry.get<FluidConduitComponent>(entity);
    ASSERT_EQ(conduit.coverage_radius, static_cast<uint8_t>(3));
    ASSERT(!conduit.is_connected);
    ASSERT(!conduit.is_active);
    ASSERT_EQ(conduit.conduit_level, static_cast<uint8_t>(1));
}

// =============================================================================
// place_conduit: Dirty flag
// =============================================================================

TEST(place_conduit_marks_coverage_dirty) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT(!sys.is_coverage_dirty(0));
    sys.place_conduit(64, 64, 0);
    ASSERT(sys.is_coverage_dirty(0));
}

// =============================================================================
// place_conduit: Event emission
// =============================================================================

TEST(place_conduit_emits_event) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    const auto& events = sys.get_conduit_placed_events();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    ASSERT_EQ(events[0].entity_id, eid);
    ASSERT_EQ(events[0].owner_id, static_cast<uint8_t>(0));
    ASSERT_EQ(events[0].grid_x, 64u);
    ASSERT_EQ(events[0].grid_y, 64u);
}

// =============================================================================
// place_conduit: Out of bounds
// =============================================================================

TEST(place_conduit_out_of_bounds_fails) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(200, 200, 0);
    ASSERT_EQ(eid, INVALID_ENTITY_ID);
    ASSERT_EQ(sys.get_conduit_position_count(0), 0u);
}

TEST(place_conduit_x_out_of_bounds_fails) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(128, 64, 0);
    ASSERT_EQ(eid, INVALID_ENTITY_ID);
}

TEST(place_conduit_y_out_of_bounds_fails) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(64, 128, 0);
    ASSERT_EQ(eid, INVALID_ENTITY_ID);
}

// =============================================================================
// place_conduit: No registry
// =============================================================================

TEST(place_conduit_returns_invalid_without_registry) {
    FluidSystem sys(128, 128);
    // No registry set
    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT_EQ(eid, INVALID_ENTITY_ID);
}

// =============================================================================
// place_conduit: Multiple placements
// =============================================================================

TEST(place_conduit_multiple_at_different_positions) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid1 = sys.place_conduit(10, 10, 0);
    uint32_t eid2 = sys.place_conduit(20, 20, 0);
    uint32_t eid3 = sys.place_conduit(30, 30, 0);

    ASSERT(eid1 != INVALID_ENTITY_ID);
    ASSERT(eid2 != INVALID_ENTITY_ID);
    ASSERT(eid3 != INVALID_ENTITY_ID);
    ASSERT(eid1 != eid2);
    ASSERT(eid2 != eid3);
    ASSERT_EQ(sys.get_conduit_position_count(0), 3u);
}

TEST(place_conduit_different_players) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid0 = sys.place_conduit(10, 10, 0);
    uint32_t eid1 = sys.place_conduit(20, 20, 1);
    uint32_t eid2 = sys.place_conduit(30, 30, 2);

    ASSERT(eid0 != INVALID_ENTITY_ID);
    ASSERT(eid1 != INVALID_ENTITY_ID);
    ASSERT(eid2 != INVALID_ENTITY_ID);
    ASSERT_EQ(sys.get_conduit_position_count(0), 1u);
    ASSERT_EQ(sys.get_conduit_position_count(1), 1u);
    ASSERT_EQ(sys.get_conduit_position_count(2), 1u);
}

// =============================================================================
// validate_conduit_placement
// =============================================================================

TEST(validate_conduit_in_bounds_succeeds) {
    FluidSystem sys(128, 128);
    ASSERT(sys.validate_conduit_placement(64, 64, 0));
}

TEST(validate_conduit_at_origin_succeeds) {
    FluidSystem sys(128, 128);
    ASSERT(sys.validate_conduit_placement(0, 0, 0));
}

TEST(validate_conduit_at_max_bound_succeeds) {
    FluidSystem sys(128, 128);
    ASSERT(sys.validate_conduit_placement(127, 127, 0));
}

TEST(validate_conduit_x_out_of_bounds_fails) {
    FluidSystem sys(128, 128);
    ASSERT(!sys.validate_conduit_placement(128, 64, 0));
}

TEST(validate_conduit_y_out_of_bounds_fails) {
    FluidSystem sys(128, 128);
    ASSERT(!sys.validate_conduit_placement(64, 128, 0));
}

TEST(validate_conduit_invalid_owner_fails) {
    FluidSystem sys(128, 128);
    ASSERT(!sys.validate_conduit_placement(64, 64, MAX_PLAYERS));
}

TEST(validate_conduit_nullptr_terrain_passes) {
    FluidSystem sys(128, 128, nullptr);
    ASSERT(sys.validate_conduit_placement(64, 64, 0));
}

TEST(validate_conduit_buildable_terrain_passes) {
    StubTerrain terrain;
    terrain.buildable_value = true;
    FluidSystem sys(128, 128, &terrain);
    ASSERT(sys.validate_conduit_placement(64, 64, 0));
}

TEST(validate_conduit_non_buildable_terrain_fails) {
    StubTerrain terrain;
    terrain.buildable_value = false;
    FluidSystem sys(128, 128, &terrain);
    ASSERT(!sys.validate_conduit_placement(64, 64, 0));
}

TEST(place_conduit_returns_invalid_on_non_buildable) {
    StubTerrain terrain;
    terrain.buildable_value = false;
    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT_EQ(eid, INVALID_ENTITY_ID);
    ASSERT_EQ(sys.get_conduit_position_count(0), 0u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Fluid Conduit Placement Unit Tests (Ticket 6-029) ===\n\n");

    // Entity creation
    RUN_TEST(place_conduit_creates_entity);
    RUN_TEST(place_conduit_has_conduit_component);
    RUN_TEST(place_conduit_component_defaults);

    // Dirty flag
    RUN_TEST(place_conduit_marks_coverage_dirty);

    // Event emission
    RUN_TEST(place_conduit_emits_event);

    // Out of bounds
    RUN_TEST(place_conduit_out_of_bounds_fails);
    RUN_TEST(place_conduit_x_out_of_bounds_fails);
    RUN_TEST(place_conduit_y_out_of_bounds_fails);

    // No registry
    RUN_TEST(place_conduit_returns_invalid_without_registry);

    // Multiple placements
    RUN_TEST(place_conduit_multiple_at_different_positions);
    RUN_TEST(place_conduit_different_players);

    // Validation
    RUN_TEST(validate_conduit_in_bounds_succeeds);
    RUN_TEST(validate_conduit_at_origin_succeeds);
    RUN_TEST(validate_conduit_at_max_bound_succeeds);
    RUN_TEST(validate_conduit_x_out_of_bounds_fails);
    RUN_TEST(validate_conduit_y_out_of_bounds_fails);
    RUN_TEST(validate_conduit_invalid_owner_fails);
    RUN_TEST(validate_conduit_nullptr_terrain_passes);
    RUN_TEST(validate_conduit_buildable_terrain_passes);
    RUN_TEST(validate_conduit_non_buildable_terrain_fails);
    RUN_TEST(place_conduit_returns_invalid_on_non_buildable);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
