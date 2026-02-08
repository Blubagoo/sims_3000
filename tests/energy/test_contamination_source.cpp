/**
 * @file test_contamination_source.cpp
 * @brief Unit tests for IContaminationSource (Ticket 5-025)
 *
 * Tests cover:
 * - ContaminationType enum values
 * - ContaminationSourceData struct layout
 * - get_contamination_sources returns empty for no nexuses
 * - get_contamination_sources returns empty when no registry
 * - get_contamination_sources returns empty for invalid owner
 * - get_contamination_sources includes online nexuses with contamination
 * - get_contamination_sources excludes offline nexuses
 * - get_contamination_sources excludes zero-contamination nexuses
 * - get_contamination_sources excludes zero-output nexuses
 * - get_contamination_sources returns correct data fields
 * - get_contamination_sources handles multiple nexuses
 * - get_contamination_sources per-player isolation
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/energy/IContaminationSource.h>
#include <sims3000/energy/NexusTypeConfig.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

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
// ContaminationType enum
// =============================================================================

TEST(contamination_type_none_is_zero) {
    ASSERT_EQ(static_cast<uint8_t>(ContaminationType::None), 0u);
}

TEST(contamination_type_energy_is_one) {
    ASSERT_EQ(static_cast<uint8_t>(ContaminationType::Energy), 1u);
}

// =============================================================================
// get_contamination_sources - Empty / no data
// =============================================================================

TEST(get_sources_empty_no_nexuses) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto sources = sys.get_contamination_sources(0);
    ASSERT(sources.empty());
}

TEST(get_sources_empty_no_registry) {
    EnergySystem sys(64, 64);
    // No registry set
    auto sources = sys.get_contamination_sources(0);
    ASSERT(sources.empty());
}

TEST(get_sources_empty_invalid_owner) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto sources = sys.get_contamination_sources(MAX_PLAYERS);
    ASSERT(sources.empty());

    sources = sys.get_contamination_sources(255);
    ASSERT(sources.empty());
}

// =============================================================================
// get_contamination_sources - Online nexus with contamination
// =============================================================================

TEST(get_sources_includes_online_contaminating_nexus) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create a Carbon nexus (has contamination = 200 per config)
    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 100;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    comp.contamination_output = 200;

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);
    sys.register_nexus_position(eid, 0, 10, 20);

    // Must update output so current_output > 0
    EnergySystem::update_nexus_output(comp);

    auto sources = sys.get_contamination_sources(0);
    ASSERT_EQ(sources.size(), static_cast<size_t>(1));
    ASSERT_EQ(sources[0].entity_id, eid);
    ASSERT_EQ(sources[0].owner_id, 0u);
    ASSERT_EQ(sources[0].contamination_output, 200u);
    ASSERT(sources[0].type == ContaminationType::Energy);
    ASSERT_EQ(sources[0].x, 10u);
    ASSERT_EQ(sources[0].y, 20u);
    // Carbon coverage_radius = 8
    ASSERT_EQ(sources[0].radius, 8u);
}

// =============================================================================
// get_contamination_sources - Excludes offline nexuses
// =============================================================================

TEST(get_sources_excludes_offline_nexus) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 100;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = false;  // Offline
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    comp.contamination_output = 200;

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);
    sys.register_nexus_position(eid, 0, 10, 20);

    // Update output (offline => current_output = 0, contamination_output = 0)
    EnergySystem::update_nexus_output(comp);

    auto sources = sys.get_contamination_sources(0);
    ASSERT(sources.empty());
}

// =============================================================================
// get_contamination_sources - Excludes zero-contamination nexuses
// =============================================================================

TEST(get_sources_excludes_zero_contamination) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Nuclear has 0 contamination
    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 400;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Nuclear);
    comp.contamination_output = 0;  // Zero contamination

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);
    sys.register_nexus_position(eid, 0, 10, 20);

    EnergySystem::update_nexus_output(comp);

    auto sources = sys.get_contamination_sources(0);
    ASSERT(sources.empty());
}

// =============================================================================
// get_contamination_sources - Excludes zero-output nexuses
// =============================================================================

TEST(get_sources_excludes_zero_output) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 0;  // Zero base output
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    comp.contamination_output = 200;

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);
    sys.register_nexus_position(eid, 0, 10, 20);

    // update_nexus_output: base 0 => current 0 => contamination zeroed
    EnergySystem::update_nexus_output(comp);

    auto sources = sys.get_contamination_sources(0);
    ASSERT(sources.empty());
}

// =============================================================================
// get_contamination_sources - Correct data fields
// =============================================================================

TEST(get_sources_correct_radius_for_petro) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 150;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Petrochemical);
    comp.contamination_output = 120;

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);
    sys.register_nexus_position(eid, 0, 30, 40);

    EnergySystem::update_nexus_output(comp);

    auto sources = sys.get_contamination_sources(0);
    ASSERT_EQ(sources.size(), static_cast<size_t>(1));
    ASSERT_EQ(sources[0].x, 30u);
    ASSERT_EQ(sources[0].y, 40u);
    ASSERT_EQ(sources[0].contamination_output, 120u);
    // Petrochemical coverage_radius = 8
    ASSERT_EQ(sources[0].radius, 8u);
}

TEST(get_sources_correct_radius_for_gaseous) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 120;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Gaseous);
    comp.contamination_output = 40;

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);
    sys.register_nexus_position(eid, 0, 15, 25);

    EnergySystem::update_nexus_output(comp);

    auto sources = sys.get_contamination_sources(0);
    ASSERT_EQ(sources.size(), static_cast<size_t>(1));
    ASSERT_EQ(sources[0].contamination_output, 40u);
    // Gaseous coverage_radius = 8
    ASSERT_EQ(sources[0].radius, 8u);
}

// =============================================================================
// get_contamination_sources - Multiple nexuses
// =============================================================================

TEST(get_sources_multiple_nexuses) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Carbon nexus (contamination = 200)
    auto e1 = reg.create();
    auto& c1 = reg.emplace<EnergyProducerComponent>(e1);
    c1.base_output = 100;
    c1.efficiency = 1.0f;
    c1.age_factor = 1.0f;
    c1.is_online = true;
    c1.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    c1.contamination_output = 200;

    // Petrochemical nexus (contamination = 120)
    auto e2 = reg.create();
    auto& c2 = reg.emplace<EnergyProducerComponent>(e2);
    c2.base_output = 150;
    c2.efficiency = 1.0f;
    c2.age_factor = 1.0f;
    c2.is_online = true;
    c2.nexus_type = static_cast<uint8_t>(NexusType::Petrochemical);
    c2.contamination_output = 120;

    // Nuclear nexus (contamination = 0) - should be excluded
    auto e3 = reg.create();
    auto& c3 = reg.emplace<EnergyProducerComponent>(e3);
    c3.base_output = 400;
    c3.efficiency = 1.0f;
    c3.age_factor = 1.0f;
    c3.is_online = true;
    c3.nexus_type = static_cast<uint8_t>(NexusType::Nuclear);
    c3.contamination_output = 0;

    uint32_t eid1 = static_cast<uint32_t>(e1);
    uint32_t eid2 = static_cast<uint32_t>(e2);
    uint32_t eid3 = static_cast<uint32_t>(e3);

    sys.register_nexus(eid1, 0);
    sys.register_nexus_position(eid1, 0, 10, 10);
    sys.register_nexus(eid2, 0);
    sys.register_nexus_position(eid2, 0, 20, 20);
    sys.register_nexus(eid3, 0);
    sys.register_nexus_position(eid3, 0, 30, 30);

    EnergySystem::update_nexus_output(c1);
    EnergySystem::update_nexus_output(c2);
    EnergySystem::update_nexus_output(c3);

    auto sources = sys.get_contamination_sources(0);
    // Only Carbon and Petrochemical should be included (Nuclear has 0 contamination)
    ASSERT_EQ(sources.size(), static_cast<size_t>(2));

    // Verify both contaminating nexuses are present
    bool found_carbon = false;
    bool found_petro = false;
    for (const auto& src : sources) {
        if (src.entity_id == eid1) {
            ASSERT_EQ(src.contamination_output, 200u);
            ASSERT(src.type == ContaminationType::Energy);
            found_carbon = true;
        } else if (src.entity_id == eid2) {
            ASSERT_EQ(src.contamination_output, 120u);
            ASSERT(src.type == ContaminationType::Energy);
            found_petro = true;
        }
    }
    ASSERT(found_carbon);
    ASSERT(found_petro);
}

// =============================================================================
// get_contamination_sources - Per-player isolation
// =============================================================================

TEST(get_sources_per_player_isolation) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Player 0: Carbon nexus
    auto e1 = reg.create();
    auto& c1 = reg.emplace<EnergyProducerComponent>(e1);
    c1.base_output = 100;
    c1.efficiency = 1.0f;
    c1.age_factor = 1.0f;
    c1.is_online = true;
    c1.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    c1.contamination_output = 200;

    // Player 1: Petrochemical nexus
    auto e2 = reg.create();
    auto& c2 = reg.emplace<EnergyProducerComponent>(e2);
    c2.base_output = 150;
    c2.efficiency = 1.0f;
    c2.age_factor = 1.0f;
    c2.is_online = true;
    c2.nexus_type = static_cast<uint8_t>(NexusType::Petrochemical);
    c2.contamination_output = 120;

    uint32_t eid1 = static_cast<uint32_t>(e1);
    uint32_t eid2 = static_cast<uint32_t>(e2);

    sys.register_nexus(eid1, 0);
    sys.register_nexus_position(eid1, 0, 10, 10);
    sys.register_nexus(eid2, 1);
    sys.register_nexus_position(eid2, 1, 20, 20);

    EnergySystem::update_nexus_output(c1);
    EnergySystem::update_nexus_output(c2);

    // Player 0 should only see Carbon
    auto sources0 = sys.get_contamination_sources(0);
    ASSERT_EQ(sources0.size(), static_cast<size_t>(1));
    ASSERT_EQ(sources0[0].entity_id, eid1);
    ASSERT_EQ(sources0[0].owner_id, 0u);

    // Player 1 should only see Petrochemical
    auto sources1 = sys.get_contamination_sources(1);
    ASSERT_EQ(sources1.size(), static_cast<size_t>(1));
    ASSERT_EQ(sources1[0].entity_id, eid2);
    ASSERT_EQ(sources1[0].owner_id, 1u);
}

// =============================================================================
// get_contamination_sources - via place_nexus integration
// =============================================================================

TEST(get_sources_via_place_nexus) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Place a Carbon nexus via place_nexus
    uint32_t eid = sys.place_nexus(NexusType::Carbon, 10, 10, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    // Update outputs so current_output > 0
    sys.update_all_nexus_outputs(0);

    auto sources = sys.get_contamination_sources(0);
    ASSERT_EQ(sources.size(), static_cast<size_t>(1));
    ASSERT_EQ(sources[0].entity_id, eid);
    ASSERT(sources[0].type == ContaminationType::Energy);
    ASSERT_EQ(sources[0].x, 10u);
    ASSERT_EQ(sources[0].y, 10u);
    // Carbon contamination from config = 200
    ASSERT_EQ(sources[0].contamination_output, 200u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== IContaminationSource Unit Tests (Ticket 5-025) ===\n\n");

    // Enum values
    RUN_TEST(contamination_type_none_is_zero);
    RUN_TEST(contamination_type_energy_is_one);

    // Empty / no data
    RUN_TEST(get_sources_empty_no_nexuses);
    RUN_TEST(get_sources_empty_no_registry);
    RUN_TEST(get_sources_empty_invalid_owner);

    // Online contaminating nexus
    RUN_TEST(get_sources_includes_online_contaminating_nexus);

    // Exclusions
    RUN_TEST(get_sources_excludes_offline_nexus);
    RUN_TEST(get_sources_excludes_zero_contamination);
    RUN_TEST(get_sources_excludes_zero_output);

    // Correct data fields
    RUN_TEST(get_sources_correct_radius_for_petro);
    RUN_TEST(get_sources_correct_radius_for_gaseous);

    // Multiple nexuses
    RUN_TEST(get_sources_multiple_nexuses);

    // Per-player isolation
    RUN_TEST(get_sources_per_player_isolation);

    // Integration via place_nexus
    RUN_TEST(get_sources_via_place_nexus);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
