/**
 * @file test_nexus_output.cpp
 * @brief Unit tests for nexus output calculation (Ticket 5-010)
 *
 * Tests cover:
 * - update_nexus_output with normal efficiency/age
 * - Offline nexus returns 0 current_output
 * - Wind/Solar variable output with weather stub factor (0.75)
 * - Contamination goes to 0 when offline (CCR-007)
 * - Contamination goes to 0 when current_output == 0 (CCR-007)
 * - get_total_generation sums correctly across multiple nexuses
 * - update_all_nexus_outputs iterates all registered nexuses
 * - tick() integrates nexus output updates
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
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
// update_nexus_output - Basic calculation tests
// =============================================================================

TEST(update_output_full_efficiency) {
    EnergyProducerComponent comp{};
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    EnergySystem::update_nexus_output(comp);
    ASSERT_EQ(comp.current_output, 1000u);
}

TEST(update_output_reduced_efficiency) {
    EnergyProducerComponent comp{};
    comp.base_output = 1000;
    comp.efficiency = 0.5f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    EnergySystem::update_nexus_output(comp);
    ASSERT_EQ(comp.current_output, 500u);
}

TEST(update_output_with_age_degradation) {
    EnergyProducerComponent comp{};
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 0.8f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Nuclear);

    EnergySystem::update_nexus_output(comp);
    ASSERT_EQ(comp.current_output, 800u);
}

TEST(update_output_combined_efficiency_and_age) {
    EnergyProducerComponent comp{};
    comp.base_output = 1000;
    comp.efficiency = 0.5f;
    comp.age_factor = 0.8f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    EnergySystem::update_nexus_output(comp);
    ASSERT_EQ(comp.current_output, 400u);
}

// =============================================================================
// update_nexus_output - Offline behavior
// =============================================================================

TEST(update_output_offline_returns_zero) {
    EnergyProducerComponent comp{};
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = false;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    EnergySystem::update_nexus_output(comp);
    ASSERT_EQ(comp.current_output, 0u);
}

TEST(update_output_offline_with_high_base_returns_zero) {
    EnergyProducerComponent comp{};
    comp.base_output = 50000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = false;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Nuclear);

    EnergySystem::update_nexus_output(comp);
    ASSERT_EQ(comp.current_output, 0u);
}

// =============================================================================
// update_nexus_output - Wind/Solar weather stub
// =============================================================================

TEST(update_output_wind_applies_weather_factor) {
    EnergyProducerComponent comp{};
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Wind);

    EnergySystem::update_nexus_output(comp);
    // 1000 * 1.0 * 1.0 * 0.75 = 750
    ASSERT_EQ(comp.current_output, 750u);
}

TEST(update_output_solar_applies_weather_factor) {
    EnergyProducerComponent comp{};
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Solar);

    EnergySystem::update_nexus_output(comp);
    // 1000 * 1.0 * 1.0 * 0.75 = 750
    ASSERT_EQ(comp.current_output, 750u);
}

TEST(update_output_solar_with_reduced_efficiency) {
    EnergyProducerComponent comp{};
    comp.base_output = 1000;
    comp.efficiency = 0.8f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Solar);

    EnergySystem::update_nexus_output(comp);
    // 1000 * 0.8 * 1.0 * 0.75 = 600
    ASSERT_EQ(comp.current_output, 600u);
}

TEST(update_output_non_variable_no_weather_factor) {
    // Nuclear should NOT have weather factor applied
    EnergyProducerComponent comp{};
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Nuclear);

    EnergySystem::update_nexus_output(comp);
    ASSERT_EQ(comp.current_output, 1000u);
}

TEST(update_output_hydro_no_weather_factor) {
    // Hydro is NOT variable (only Wind/Solar are)
    EnergyProducerComponent comp{};
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Hydro);

    EnergySystem::update_nexus_output(comp);
    ASSERT_EQ(comp.current_output, 1000u);
}

// =============================================================================
// update_nexus_output - Contamination (CCR-007)
// =============================================================================

TEST(contamination_zero_when_offline) {
    EnergyProducerComponent comp{};
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = false;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    comp.contamination_output = 50;  // Set non-zero contamination

    EnergySystem::update_nexus_output(comp);
    ASSERT_EQ(comp.current_output, 0u);
    ASSERT_EQ(comp.contamination_output, 0u);  // CCR-007
}

TEST(contamination_persists_when_online_producing) {
    EnergyProducerComponent comp{};
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    comp.contamination_output = 50;

    EnergySystem::update_nexus_output(comp);
    ASSERT_EQ(comp.current_output, 1000u);
    ASSERT_EQ(comp.contamination_output, 50u);  // Should stay non-zero
}

TEST(contamination_zero_when_zero_output) {
    // Online but base_output is 0 => current_output 0 => contamination cleared
    EnergyProducerComponent comp{};
    comp.base_output = 0;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    comp.contamination_output = 50;

    EnergySystem::update_nexus_output(comp);
    ASSERT_EQ(comp.current_output, 0u);
    ASSERT_EQ(comp.contamination_output, 0u);  // CCR-007
}

// =============================================================================
// get_total_generation - Sum tests with registry
// =============================================================================

TEST(get_total_generation_no_registry_returns_zero) {
    EnergySystem sys(64, 64);
    // No registry set
    ASSERT_EQ(sys.get_total_generation(0), 0u);
}

TEST(get_total_generation_no_nexuses_returns_zero) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    ASSERT_EQ(sys.get_total_generation(0), 0u);
}

TEST(get_total_generation_single_nexus) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create an entity with EnergyProducerComponent
    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);

    // Must update output first
    sys.update_all_nexus_outputs(0);

    ASSERT_EQ(sys.get_total_generation(0), 1000u);
}

TEST(get_total_generation_multiple_nexuses) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create 3 nexus entities for player 0
    auto e1 = reg.create();
    auto& c1 = reg.emplace<EnergyProducerComponent>(e1);
    c1.base_output = 500;
    c1.efficiency = 1.0f;
    c1.age_factor = 1.0f;
    c1.is_online = true;
    c1.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    auto e2 = reg.create();
    auto& c2 = reg.emplace<EnergyProducerComponent>(e2);
    c2.base_output = 300;
    c2.efficiency = 1.0f;
    c2.age_factor = 1.0f;
    c2.is_online = true;
    c2.nexus_type = static_cast<uint8_t>(NexusType::Nuclear);

    auto e3 = reg.create();
    auto& c3 = reg.emplace<EnergyProducerComponent>(e3);
    c3.base_output = 200;
    c3.efficiency = 1.0f;
    c3.age_factor = 1.0f;
    c3.is_online = true;
    c3.nexus_type = static_cast<uint8_t>(NexusType::Gaseous);

    sys.register_nexus(static_cast<uint32_t>(e1), 0);
    sys.register_nexus(static_cast<uint32_t>(e2), 0);
    sys.register_nexus(static_cast<uint32_t>(e3), 0);

    sys.update_all_nexus_outputs(0);

    // 500 + 300 + 200 = 1000
    ASSERT_EQ(sys.get_total_generation(0), 1000u);
}

TEST(get_total_generation_excludes_offline) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto e1 = reg.create();
    auto& c1 = reg.emplace<EnergyProducerComponent>(e1);
    c1.base_output = 500;
    c1.efficiency = 1.0f;
    c1.age_factor = 1.0f;
    c1.is_online = true;
    c1.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    auto e2 = reg.create();
    auto& c2 = reg.emplace<EnergyProducerComponent>(e2);
    c2.base_output = 300;
    c2.efficiency = 1.0f;
    c2.age_factor = 1.0f;
    c2.is_online = false;  // Offline
    c2.nexus_type = static_cast<uint8_t>(NexusType::Nuclear);

    sys.register_nexus(static_cast<uint32_t>(e1), 0);
    sys.register_nexus(static_cast<uint32_t>(e2), 0);

    sys.update_all_nexus_outputs(0);

    // Only e1 contributes: 500
    ASSERT_EQ(sys.get_total_generation(0), 500u);
}

TEST(get_total_generation_per_player_isolation) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto e1 = reg.create();
    auto& c1 = reg.emplace<EnergyProducerComponent>(e1);
    c1.base_output = 500;
    c1.efficiency = 1.0f;
    c1.age_factor = 1.0f;
    c1.is_online = true;
    c1.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    auto e2 = reg.create();
    auto& c2 = reg.emplace<EnergyProducerComponent>(e2);
    c2.base_output = 700;
    c2.efficiency = 1.0f;
    c2.age_factor = 1.0f;
    c2.is_online = true;
    c2.nexus_type = static_cast<uint8_t>(NexusType::Nuclear);

    sys.register_nexus(static_cast<uint32_t>(e1), 0);  // Player 0
    sys.register_nexus(static_cast<uint32_t>(e2), 1);  // Player 1

    sys.update_all_nexus_outputs(0);
    sys.update_all_nexus_outputs(1);

    ASSERT_EQ(sys.get_total_generation(0), 500u);
    ASSERT_EQ(sys.get_total_generation(1), 700u);
}

TEST(get_total_generation_invalid_owner) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    ASSERT_EQ(sys.get_total_generation(MAX_PLAYERS), 0u);
    ASSERT_EQ(sys.get_total_generation(255), 0u);
}

// =============================================================================
// update_all_nexus_outputs - Registry integration
// =============================================================================

TEST(update_all_no_registry_is_noop) {
    EnergySystem sys(64, 64);
    // No registry set - should not crash
    sys.register_nexus(42, 0);
    sys.update_all_nexus_outputs(0);
    // get_total_generation also requires registry
    ASSERT_EQ(sys.get_total_generation(0), 0u);
}

TEST(update_all_invalid_owner_is_noop) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Should not crash with invalid owner
    sys.update_all_nexus_outputs(MAX_PLAYERS);
    sys.update_all_nexus_outputs(255);
}

TEST(update_all_updates_each_nexus) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto e1 = reg.create();
    auto& c1 = reg.emplace<EnergyProducerComponent>(e1);
    c1.base_output = 1000;
    c1.efficiency = 0.5f;
    c1.age_factor = 0.8f;
    c1.is_online = true;
    c1.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    auto e2 = reg.create();
    auto& c2 = reg.emplace<EnergyProducerComponent>(e2);
    c2.base_output = 2000;
    c2.efficiency = 1.0f;
    c2.age_factor = 1.0f;
    c2.is_online = true;
    c2.nexus_type = static_cast<uint8_t>(NexusType::Wind);

    sys.register_nexus(static_cast<uint32_t>(e1), 0);
    sys.register_nexus(static_cast<uint32_t>(e2), 0);

    sys.update_all_nexus_outputs(0);

    // e1: 1000 * 0.5 * 0.8 = 400 (Carbon, no weather factor)
    ASSERT_EQ(c1.current_output, 400u);
    // e2: 2000 * 1.0 * 1.0 * 0.75 = 1500 (Wind, weather factor applied)
    ASSERT_EQ(c2.current_output, 1500u);
}

// =============================================================================
// tick() integration
// =============================================================================

TEST(tick_updates_all_player_nexus_outputs) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Player 0 nexus
    auto e1 = reg.create();
    auto& c1 = reg.emplace<EnergyProducerComponent>(e1);
    c1.base_output = 1000;
    c1.efficiency = 1.0f;
    c1.age_factor = 1.0f;
    c1.is_online = true;
    c1.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    // Player 1 nexus
    auto e2 = reg.create();
    auto& c2 = reg.emplace<EnergyProducerComponent>(e2);
    c2.base_output = 2000;
    c2.efficiency = 1.0f;
    c2.age_factor = 1.0f;
    c2.is_online = true;
    c2.nexus_type = static_cast<uint8_t>(NexusType::Solar);

    sys.register_nexus(static_cast<uint32_t>(e1), 0);
    sys.register_nexus(static_cast<uint32_t>(e2), 1);

    // tick calls aging (Ticket 5-022) then output calculation for all players.
    // After 1 tick of aging from ticks=0:
    //   Carbon age_factor = 0.60 + 0.40*exp(-0.0001*1) ~= 0.99996
    //   Solar age_factor = 0.85 + 0.15*exp(-0.0001*1) ~= 0.99998
    // So outputs are very close to but slightly below their un-aged values.
    sys.tick(0.05f);

    // Carbon: 1000 * 1.0 * ~0.99996 = 999 (truncated)
    ASSERT(c1.current_output >= 999u);
    ASSERT(c1.current_output <= 1000u);
    // Solar: 2000 * 1.0 * ~0.99998 * 0.75 = ~1499 (truncated)
    ASSERT(c2.current_output >= 1499u);
    ASSERT(c2.current_output <= 1500u);

    ASSERT(sys.get_total_generation(0) >= 999u);
    ASSERT(sys.get_total_generation(0) <= 1000u);
    ASSERT(sys.get_total_generation(1) >= 1499u);
    ASSERT(sys.get_total_generation(1) <= 1500u);
}

TEST(tick_no_registry_does_not_crash) {
    EnergySystem sys(64, 64);
    // No registry set, tick should not crash
    sys.register_nexus(42, 0);
    sys.tick(0.05f);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Nexus Output Calculation Unit Tests (Ticket 5-010) ===\n\n");

    // Basic output calculation
    RUN_TEST(update_output_full_efficiency);
    RUN_TEST(update_output_reduced_efficiency);
    RUN_TEST(update_output_with_age_degradation);
    RUN_TEST(update_output_combined_efficiency_and_age);

    // Offline behavior
    RUN_TEST(update_output_offline_returns_zero);
    RUN_TEST(update_output_offline_with_high_base_returns_zero);

    // Wind/Solar weather stub
    RUN_TEST(update_output_wind_applies_weather_factor);
    RUN_TEST(update_output_solar_applies_weather_factor);
    RUN_TEST(update_output_solar_with_reduced_efficiency);
    RUN_TEST(update_output_non_variable_no_weather_factor);
    RUN_TEST(update_output_hydro_no_weather_factor);

    // Contamination (CCR-007)
    RUN_TEST(contamination_zero_when_offline);
    RUN_TEST(contamination_persists_when_online_producing);
    RUN_TEST(contamination_zero_when_zero_output);

    // get_total_generation
    RUN_TEST(get_total_generation_no_registry_returns_zero);
    RUN_TEST(get_total_generation_no_nexuses_returns_zero);
    RUN_TEST(get_total_generation_single_nexus);
    RUN_TEST(get_total_generation_multiple_nexuses);
    RUN_TEST(get_total_generation_excludes_offline);
    RUN_TEST(get_total_generation_per_player_isolation);
    RUN_TEST(get_total_generation_invalid_owner);

    // update_all_nexus_outputs
    RUN_TEST(update_all_no_registry_is_noop);
    RUN_TEST(update_all_invalid_owner_is_noop);
    RUN_TEST(update_all_updates_each_nexus);

    // tick() integration
    RUN_TEST(tick_updates_all_player_nexus_outputs);
    RUN_TEST(tick_no_registry_does_not_crash);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
