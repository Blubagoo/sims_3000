/**
 * @file test_nexus_placement.cpp
 * @brief Unit tests for nexus placement validation (Ticket 5-026)
 *
 * Tests cover:
 * - Bounds check: out-of-bounds coordinates rejected
 * - Ownership check: stub always passes
 * - Terrain buildable check: non-buildable terrain rejected, nullptr terrain passes
 * - No existing structure check: stub always passes
 * - Type-specific terrain requirements: Hydro/Geothermal stubbed as valid
 * - place_nexus() creates entity with correct components
 * - place_nexus() registers nexus and position
 * - place_nexus() marks coverage dirty
 * - place_nexus() returns 0 on failure
 * - place_nexus() returns 0 with no registry
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/energy/NexusTypeConfig.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::energy;

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

/**
 * @brief Stub terrain that returns configurable buildability.
 *
 * All other ITerrainQueryable methods return safe defaults.
 */
class StubTerrain : public sims3000::terrain::ITerrainQueryable {
public:
    bool buildable_value = true;

    sims3000::terrain::TerrainType get_terrain_type(
        std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return sims3000::terrain::TerrainType::Substrate;
    }

    std::uint8_t get_elevation(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return 10;
    }

    bool is_buildable(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return buildable_value;
    }

    std::uint8_t get_slope(std::int32_t /*x1*/, std::int32_t /*y1*/,
                           std::int32_t /*x2*/, std::int32_t /*y2*/) const override {
        return 0;
    }

    float get_average_elevation(std::int32_t /*x*/, std::int32_t /*y*/,
                                std::uint32_t /*radius*/) const override {
        return 10.0f;
    }

    std::uint32_t get_water_distance(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return 255;
    }

    float get_value_bonus(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return 0.0f;
    }

    float get_harmony_bonus(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return 0.0f;
    }

    std::int32_t get_build_cost_modifier(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return 100;
    }

    std::uint32_t get_contamination_output(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return 0;
    }

    std::uint32_t get_map_width() const override { return 128; }
    std::uint32_t get_map_height() const override { return 128; }
    std::uint8_t get_sea_level() const override { return 8; }

    void get_tiles_in_rect(const sims3000::terrain::GridRect& /*rect*/,
                           std::vector<sims3000::terrain::TerrainComponent>& out_tiles) const override {
        out_tiles.clear();
    }

    std::uint32_t get_buildable_tiles_in_rect(
        const sims3000::terrain::GridRect& /*rect*/) const override {
        return 0;
    }

    std::uint32_t count_terrain_type_in_rect(
        const sims3000::terrain::GridRect& /*rect*/,
        sims3000::terrain::TerrainType /*type*/) const override {
        return 0;
    }
};

// =============================================================================
// Validation: Bounds check
// =============================================================================

TEST(validate_nexus_in_bounds_succeeds) {
    EnergySystem sys(128, 128);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 64, 64, 0);
    ASSERT(result.success);
}

TEST(validate_nexus_at_origin_succeeds) {
    EnergySystem sys(128, 128);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 0, 0, 0);
    ASSERT(result.success);
}

TEST(validate_nexus_at_max_bound_succeeds) {
    EnergySystem sys(128, 128);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 127, 127, 0);
    ASSERT(result.success);
}

TEST(validate_nexus_x_out_of_bounds_fails) {
    EnergySystem sys(128, 128);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 128, 64, 0);
    ASSERT(!result.success);
    ASSERT(result.reason[0] != '\0'); // reason should not be empty
}

TEST(validate_nexus_y_out_of_bounds_fails) {
    EnergySystem sys(128, 128);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 64, 128, 0);
    ASSERT(!result.success);
}

TEST(validate_nexus_both_out_of_bounds_fails) {
    EnergySystem sys(128, 128);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 200, 200, 0);
    ASSERT(!result.success);
}

TEST(validate_nexus_large_coords_out_of_bounds_fails) {
    EnergySystem sys(128, 128);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 999999, 999999, 0);
    ASSERT(!result.success);
}

// =============================================================================
// Validation: Ownership check (stub: always true)
// =============================================================================

TEST(validate_nexus_ownership_stub_passes_player0) {
    EnergySystem sys(128, 128);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 64, 64, 0);
    ASSERT(result.success);
}

TEST(validate_nexus_ownership_stub_passes_player3) {
    EnergySystem sys(128, 128);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 64, 64, 3);
    ASSERT(result.success);
}

// =============================================================================
// Validation: Terrain buildable check
// =============================================================================

TEST(validate_nexus_nullptr_terrain_passes) {
    EnergySystem sys(128, 128, nullptr);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 64, 64, 0);
    ASSERT(result.success);
}

TEST(validate_nexus_buildable_terrain_passes) {
    StubTerrain terrain;
    terrain.buildable_value = true;
    EnergySystem sys(128, 128, &terrain);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 64, 64, 0);
    ASSERT(result.success);
}

TEST(validate_nexus_non_buildable_terrain_fails) {
    StubTerrain terrain;
    terrain.buildable_value = false;
    EnergySystem sys(128, 128, &terrain);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 64, 64, 0);
    ASSERT(!result.success);
    ASSERT(result.reason[0] != '\0'); // reason should not be empty
}

// =============================================================================
// Validation: No existing structure (stub: always passes)
// =============================================================================

TEST(validate_nexus_no_existing_structure_stub_passes) {
    EnergySystem sys(128, 128);
    // Even after placing something, the stub always passes
    PlacementResult result = sys.validate_nexus_placement(NexusType::Carbon, 64, 64, 0);
    ASSERT(result.success);
}

// =============================================================================
// Validation: Type-specific terrain requirements (Hydro/Geothermal stub)
// =============================================================================

TEST(validate_nexus_hydro_stubbed_valid) {
    EnergySystem sys(128, 128);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Hydro, 64, 64, 0);
    ASSERT(result.success);
}

TEST(validate_nexus_geothermal_stubbed_valid) {
    EnergySystem sys(128, 128);
    PlacementResult result = sys.validate_nexus_placement(NexusType::Geothermal, 64, 64, 0);
    ASSERT(result.success);
}

TEST(validate_nexus_all_mvp_types_pass) {
    EnergySystem sys(128, 128);
    NexusType types[] = {
        NexusType::Carbon, NexusType::Petrochemical, NexusType::Gaseous,
        NexusType::Nuclear, NexusType::Wind, NexusType::Solar
    };
    for (auto type : types) {
        PlacementResult result = sys.validate_nexus_placement(type, 64, 64, 0);
        ASSERT(result.success);
    }
}

// =============================================================================
// place_nexus(): Entity creation
// =============================================================================

TEST(place_nexus_creates_entity) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    auto entity = static_cast<entt::entity>(eid);
    ASSERT(registry.valid(entity));
}

TEST(place_nexus_has_producer_component) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_nexus(NexusType::Carbon, 64, 64, 0);
    auto entity = static_cast<entt::entity>(eid);

    ASSERT(registry.all_of<EnergyProducerComponent>(entity));
}

TEST(place_nexus_producer_has_correct_type) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_nexus(NexusType::Nuclear, 64, 64, 0);
    auto entity = static_cast<entt::entity>(eid);

    const auto& producer = registry.get<EnergyProducerComponent>(entity);
    ASSERT_EQ(producer.nexus_type, static_cast<uint8_t>(NexusType::Nuclear));
}

TEST(place_nexus_producer_has_correct_base_output) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_nexus(NexusType::Carbon, 64, 64, 0);
    auto entity = static_cast<entt::entity>(eid);

    const auto& producer = registry.get<EnergyProducerComponent>(entity);
    const NexusTypeConfig& config = get_nexus_config(NexusType::Carbon);
    ASSERT_EQ(producer.base_output, config.base_output);
}

TEST(place_nexus_producer_starts_online) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_nexus(NexusType::Wind, 64, 64, 0);
    auto entity = static_cast<entt::entity>(eid);

    const auto& producer = registry.get<EnergyProducerComponent>(entity);
    ASSERT(producer.is_online);
}

TEST(place_nexus_producer_initial_efficiency) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_nexus(NexusType::Solar, 64, 64, 0);
    auto entity = static_cast<entt::entity>(eid);

    const auto& producer = registry.get<EnergyProducerComponent>(entity);
    ASSERT(producer.efficiency == 1.0f);
    ASSERT(producer.age_factor == 1.0f);
    ASSERT_EQ(producer.ticks_since_built, static_cast<uint16_t>(0));
}

TEST(place_nexus_producer_has_contamination) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_nexus(NexusType::Carbon, 64, 64, 0);
    auto entity = static_cast<entt::entity>(eid);

    const auto& producer = registry.get<EnergyProducerComponent>(entity);
    const NexusTypeConfig& config = get_nexus_config(NexusType::Carbon);
    ASSERT_EQ(producer.contamination_output, config.contamination);
}

// =============================================================================
// place_nexus(): Registration
// =============================================================================

TEST(place_nexus_registers_nexus) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT_EQ(sys.get_nexus_count(0), 0u);
    sys.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT_EQ(sys.get_nexus_count(0), 1u);
}

TEST(place_nexus_registers_position) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT_EQ(sys.get_nexus_position_count(0), 0u);
    sys.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT_EQ(sys.get_nexus_position_count(0), 1u);
}

TEST(place_nexus_marks_coverage_dirty) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT(!sys.is_coverage_dirty(0));
    sys.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT(sys.is_coverage_dirty(0));
}

TEST(place_nexus_different_player) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    sys.place_nexus(NexusType::Carbon, 64, 64, 2);
    ASSERT_EQ(sys.get_nexus_count(2), 1u);
    ASSERT_EQ(sys.get_nexus_count(0), 0u);
    ASSERT(sys.is_coverage_dirty(2));
}

// =============================================================================
// place_nexus(): Failure cases
// =============================================================================

TEST(place_nexus_returns_zero_on_out_of_bounds) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_nexus(NexusType::Carbon, 200, 200, 0);
    ASSERT_EQ(eid, INVALID_ENTITY_ID);
}

TEST(place_nexus_returns_zero_without_registry) {
    EnergySystem sys(128, 128);
    // No registry set
    uint32_t eid = sys.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT_EQ(eid, INVALID_ENTITY_ID);
}

TEST(place_nexus_returns_zero_on_non_buildable) {
    StubTerrain terrain;
    terrain.buildable_value = false;
    EnergySystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT_EQ(eid, INVALID_ENTITY_ID);
    ASSERT_EQ(sys.get_nexus_count(0), 0u);
}

TEST(place_nexus_multiple_at_different_positions) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid1 = sys.place_nexus(NexusType::Carbon, 10, 10, 0);
    uint32_t eid2 = sys.place_nexus(NexusType::Nuclear, 50, 50, 0);
    uint32_t eid3 = sys.place_nexus(NexusType::Wind, 90, 90, 0);

    ASSERT(eid1 != INVALID_ENTITY_ID);
    ASSERT(eid2 != INVALID_ENTITY_ID);
    ASSERT(eid3 != INVALID_ENTITY_ID);
    ASSERT(eid1 != eid2);
    ASSERT(eid2 != eid3);
    ASSERT_EQ(sys.get_nexus_count(0), 3u);
    ASSERT_EQ(sys.get_nexus_position_count(0), 3u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Nexus Placement Validation Unit Tests (Ticket 5-026) ===\n\n");

    // Bounds check
    RUN_TEST(validate_nexus_in_bounds_succeeds);
    RUN_TEST(validate_nexus_at_origin_succeeds);
    RUN_TEST(validate_nexus_at_max_bound_succeeds);
    RUN_TEST(validate_nexus_x_out_of_bounds_fails);
    RUN_TEST(validate_nexus_y_out_of_bounds_fails);
    RUN_TEST(validate_nexus_both_out_of_bounds_fails);
    RUN_TEST(validate_nexus_large_coords_out_of_bounds_fails);

    // Ownership check (stub)
    RUN_TEST(validate_nexus_ownership_stub_passes_player0);
    RUN_TEST(validate_nexus_ownership_stub_passes_player3);

    // Terrain buildable check
    RUN_TEST(validate_nexus_nullptr_terrain_passes);
    RUN_TEST(validate_nexus_buildable_terrain_passes);
    RUN_TEST(validate_nexus_non_buildable_terrain_fails);

    // No existing structure (stub)
    RUN_TEST(validate_nexus_no_existing_structure_stub_passes);

    // Type-specific terrain requirements
    RUN_TEST(validate_nexus_hydro_stubbed_valid);
    RUN_TEST(validate_nexus_geothermal_stubbed_valid);
    RUN_TEST(validate_nexus_all_mvp_types_pass);

    // place_nexus entity creation
    RUN_TEST(place_nexus_creates_entity);
    RUN_TEST(place_nexus_has_producer_component);
    RUN_TEST(place_nexus_producer_has_correct_type);
    RUN_TEST(place_nexus_producer_has_correct_base_output);
    RUN_TEST(place_nexus_producer_starts_online);
    RUN_TEST(place_nexus_producer_initial_efficiency);
    RUN_TEST(place_nexus_producer_has_contamination);

    // place_nexus registration
    RUN_TEST(place_nexus_registers_nexus);
    RUN_TEST(place_nexus_registers_position);
    RUN_TEST(place_nexus_marks_coverage_dirty);
    RUN_TEST(place_nexus_different_player);

    // place_nexus failure cases
    RUN_TEST(place_nexus_returns_zero_on_out_of_bounds);
    RUN_TEST(place_nexus_returns_zero_without_registry);
    RUN_TEST(place_nexus_returns_zero_on_non_buildable);
    RUN_TEST(place_nexus_multiple_at_different_positions);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
