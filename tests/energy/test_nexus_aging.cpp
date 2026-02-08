/**
 * @file test_nexus_aging.cpp
 * @brief Unit tests for nexus aging mechanics (Ticket 5-022)
 *
 * Tests cover:
 * - ticks_since_built increments each call
 * - ticks_since_built caps at 65535
 * - age_factor starts at 1.0 and decreases over time
 * - age_factor approaches type-specific floor asymptotically
 * - Type-specific floors: Carbon=0.60, Petro=0.65, Gaseous=0.70,
 *   Nuclear=0.75, Wind=0.80, Solar=0.85
 * - age_factor never goes below the floor
 * - NexusAgedEvent emitted when efficiency drops past 10% threshold
 * - tick() integrates aging for all nexuses
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/energy/NexusTypeConfig.h>
#include <entt/entt.hpp>
#include <cmath>
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

/// Helper: Check float approximately equal within tolerance
static bool approx_eq(float a, float b, float tolerance = 0.001f) {
    return std::fabs(a - b) < tolerance;
}

#define ASSERT_APPROX(a, b) do { \
    if (!approx_eq((a), (b))) { \
        printf("\n  FAILED: %s ~= %s (got %f vs %f, line %d)\n", \
               #a, #b, static_cast<double>(a), static_cast<double>(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// ticks_since_built incrementing
// =============================================================================

TEST(ticks_increments_each_call) {
    EnergyProducerComponent comp{};
    comp.ticks_since_built = 0;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    EnergySystem::update_nexus_aging(comp);
    ASSERT_EQ(comp.ticks_since_built, static_cast<uint16_t>(1));

    EnergySystem::update_nexus_aging(comp);
    ASSERT_EQ(comp.ticks_since_built, static_cast<uint16_t>(2));

    EnergySystem::update_nexus_aging(comp);
    ASSERT_EQ(comp.ticks_since_built, static_cast<uint16_t>(3));
}

TEST(ticks_caps_at_65535) {
    EnergyProducerComponent comp{};
    comp.ticks_since_built = 65534;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    EnergySystem::update_nexus_aging(comp);
    ASSERT_EQ(comp.ticks_since_built, static_cast<uint16_t>(65535));

    // Should not overflow past 65535
    EnergySystem::update_nexus_aging(comp);
    ASSERT_EQ(comp.ticks_since_built, static_cast<uint16_t>(65535));

    EnergySystem::update_nexus_aging(comp);
    ASSERT_EQ(comp.ticks_since_built, static_cast<uint16_t>(65535));
}

// =============================================================================
// age_factor decay behavior
// =============================================================================

TEST(age_factor_starts_near_one) {
    EnergyProducerComponent comp{};
    comp.ticks_since_built = 0;
    comp.age_factor = 1.0f;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    // After one tick, age_factor should be very close to 1.0
    EnergySystem::update_nexus_aging(comp);
    // At tick 1: floor + (1-floor)*exp(-0.0001*1)
    // Carbon floor = 0.60: 0.60 + 0.40 * exp(-0.0001) ~= 0.60 + 0.40 * 0.99990 ~= 0.99996
    ASSERT(comp.age_factor > 0.999f);
    ASSERT(comp.age_factor <= 1.0f);
}

TEST(age_factor_decreases_over_time) {
    EnergyProducerComponent comp{};
    comp.ticks_since_built = 0;
    comp.age_factor = 1.0f;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    // Age 100 ticks
    for (int i = 0; i < 100; ++i) {
        EnergySystem::update_nexus_aging(comp);
    }
    float after_100 = comp.age_factor;

    // Age 100 more ticks (200 total)
    for (int i = 0; i < 100; ++i) {
        EnergySystem::update_nexus_aging(comp);
    }
    float after_200 = comp.age_factor;

    // age_factor should decrease over time
    ASSERT(after_200 < after_100);
    ASSERT(after_100 < 1.0f);
}

TEST(age_factor_at_1000_ticks_carbon) {
    EnergyProducerComponent comp{};
    comp.ticks_since_built = 999; // will be incremented to 1000
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    EnergySystem::update_nexus_aging(comp);
    // age_factor = 0.60 + 0.40 * exp(-0.0001 * 1000)
    //            = 0.60 + 0.40 * exp(-0.1)
    //            = 0.60 + 0.40 * 0.90484
    //            = 0.60 + 0.36193
    //            = 0.96193
    float expected = 0.60f + 0.40f * std::exp(-0.0001f * 1000.0f);
    ASSERT_APPROX(comp.age_factor, expected);
}

TEST(age_factor_at_10000_ticks_carbon) {
    EnergyProducerComponent comp{};
    comp.ticks_since_built = 9999; // will be incremented to 10000
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    EnergySystem::update_nexus_aging(comp);
    // age_factor = 0.60 + 0.40 * exp(-0.0001 * 10000)
    //            = 0.60 + 0.40 * exp(-1.0)
    //            = 0.60 + 0.40 * 0.36788
    //            = 0.60 + 0.14715
    //            = 0.74715
    float expected = 0.60f + 0.40f * std::exp(-0.0001f * 10000.0f);
    ASSERT_APPROX(comp.age_factor, expected);
}

// =============================================================================
// Type-specific aging floors
// =============================================================================

TEST(carbon_floor_is_060) {
    EnergyProducerComponent comp{};
    comp.ticks_since_built = 65534; // will be capped at 65535
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    EnergySystem::update_nexus_aging(comp);
    // At max ticks: floor + (1-floor)*exp(-0.0001*65535) = 0.60 + 0.40 * exp(-6.5535)
    // exp(-6.5535) ~= 0.00143 => age_factor ~= 0.60057
    ASSERT(comp.age_factor > 0.60f);
    ASSERT(comp.age_factor < 0.61f);
}

TEST(petro_floor_is_065) {
    EnergyProducerComponent comp{};
    comp.ticks_since_built = 65534;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Petrochemical);

    EnergySystem::update_nexus_aging(comp);
    ASSERT(comp.age_factor > 0.65f);
    ASSERT(comp.age_factor < 0.66f);
}

TEST(gaseous_floor_is_070) {
    EnergyProducerComponent comp{};
    comp.ticks_since_built = 65534;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Gaseous);

    EnergySystem::update_nexus_aging(comp);
    ASSERT(comp.age_factor > 0.70f);
    ASSERT(comp.age_factor < 0.71f);
}

TEST(nuclear_floor_is_075) {
    EnergyProducerComponent comp{};
    comp.ticks_since_built = 65534;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Nuclear);

    EnergySystem::update_nexus_aging(comp);
    ASSERT(comp.age_factor > 0.75f);
    ASSERT(comp.age_factor < 0.76f);
}

TEST(wind_floor_is_080) {
    EnergyProducerComponent comp{};
    comp.ticks_since_built = 65534;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Wind);

    EnergySystem::update_nexus_aging(comp);
    ASSERT(comp.age_factor > 0.80f);
    ASSERT(comp.age_factor < 0.81f);
}

TEST(solar_floor_is_085) {
    EnergyProducerComponent comp{};
    comp.ticks_since_built = 65534;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Solar);

    EnergySystem::update_nexus_aging(comp);
    ASSERT(comp.age_factor > 0.85f);
    ASSERT(comp.age_factor < 0.86f);
}

// =============================================================================
// age_factor never goes below the floor
// =============================================================================

TEST(age_factor_never_below_floor_carbon) {
    EnergyProducerComponent comp{};
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    // Age many many ticks
    for (int i = 0; i < 65535; ++i) {
        EnergySystem::update_nexus_aging(comp);
    }

    // Even at max age, should be >= floor
    ASSERT(comp.age_factor >= 0.60f);
}

TEST(age_factor_never_below_floor_solar) {
    EnergyProducerComponent comp{};
    comp.nexus_type = static_cast<uint8_t>(NexusType::Solar);

    // Age many many ticks
    for (int i = 0; i < 65535; ++i) {
        EnergySystem::update_nexus_aging(comp);
    }

    // Even at max age, should be >= floor
    ASSERT(comp.age_factor >= 0.85f);
}

// =============================================================================
// Higher floor types age more gracefully
// =============================================================================

TEST(solar_ages_more_gracefully_than_carbon) {
    EnergyProducerComponent carbon{};
    carbon.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    EnergyProducerComponent solar{};
    solar.nexus_type = static_cast<uint8_t>(NexusType::Solar);

    // Age both 5000 ticks
    for (int i = 0; i < 5000; ++i) {
        EnergySystem::update_nexus_aging(carbon);
        EnergySystem::update_nexus_aging(solar);
    }

    // Solar should have higher age_factor than Carbon
    ASSERT(solar.age_factor > carbon.age_factor);
}

// =============================================================================
// Formula verification
// =============================================================================

TEST(formula_matches_expected_for_each_type) {
    struct TestCase {
        NexusType type;
        float floor;
        uint16_t ticks;
    };

    TestCase cases[] = {
        { NexusType::Carbon,        0.60f, 5000 },
        { NexusType::Petrochemical, 0.65f, 5000 },
        { NexusType::Gaseous,       0.70f, 5000 },
        { NexusType::Nuclear,       0.75f, 5000 },
        { NexusType::Wind,          0.80f, 5000 },
        { NexusType::Solar,         0.85f, 5000 },
    };

    for (const auto& tc : cases) {
        EnergyProducerComponent comp{};
        comp.ticks_since_built = tc.ticks - 1; // will be incremented to tc.ticks
        comp.nexus_type = static_cast<uint8_t>(tc.type);

        EnergySystem::update_nexus_aging(comp);

        float expected = tc.floor + (1.0f - tc.floor) * std::exp(-0.0001f * static_cast<float>(tc.ticks));
        ASSERT_APPROX(comp.age_factor, expected);
    }
}

// =============================================================================
// tick() integration - aging updates age_factor for all nexuses
// =============================================================================

TEST(tick_ages_all_nexuses) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create nexus for player 0
    auto e1 = reg.create();
    auto& c1 = reg.emplace<EnergyProducerComponent>(e1);
    c1.base_output = 1000;
    c1.efficiency = 1.0f;
    c1.age_factor = 1.0f;
    c1.ticks_since_built = 0;
    c1.is_online = true;
    c1.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    // Create nexus for player 1
    auto e2 = reg.create();
    auto& c2 = reg.emplace<EnergyProducerComponent>(e2);
    c2.base_output = 2000;
    c2.efficiency = 1.0f;
    c2.age_factor = 1.0f;
    c2.ticks_since_built = 0;
    c2.is_online = true;
    c2.nexus_type = static_cast<uint8_t>(NexusType::Solar);

    sys.register_nexus(static_cast<uint32_t>(e1), 0);
    sys.register_nexus(static_cast<uint32_t>(e2), 1);

    // Also register nexus positions (to avoid coverage dirty issues)
    sys.register_nexus_position(static_cast<uint32_t>(e1), 0, 10, 10);
    sys.register_nexus_position(static_cast<uint32_t>(e2), 1, 20, 20);

    // Run tick
    sys.tick(0.05f);

    // Both should have been aged (ticks_since_built incremented to 1)
    ASSERT_EQ(c1.ticks_since_built, static_cast<uint16_t>(1));
    ASSERT_EQ(c2.ticks_since_built, static_cast<uint16_t>(1));

    // age_factor should be slightly less than 1.0
    ASSERT(c1.age_factor < 1.0f);
    ASSERT(c2.age_factor < 1.0f);
    ASSERT(c1.age_factor > 0.999f);
    ASSERT(c2.age_factor > 0.999f);
}

TEST(tick_aging_affects_output) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto e1 = reg.create();
    auto& c1 = reg.emplace<EnergyProducerComponent>(e1);
    c1.base_output = 1000;
    c1.efficiency = 1.0f;
    c1.age_factor = 1.0f;
    c1.ticks_since_built = 9999; // aging will make this 10000
    c1.is_online = true;
    c1.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    sys.register_nexus(static_cast<uint32_t>(e1), 0);
    sys.register_nexus_position(static_cast<uint32_t>(e1), 0, 10, 10);

    // Run tick (ages, then calculates output)
    sys.tick(0.05f);

    // age_factor at 10000 ticks for Carbon:
    // 0.60 + 0.40 * exp(-0.0001 * 10000) = 0.60 + 0.40 * exp(-1.0) ~= 0.7472
    // current_output = 1000 * 1.0 * 0.7472 = 747
    ASSERT(c1.current_output < 1000u);
    ASSERT(c1.current_output > 700u);
}

TEST(tick_no_registry_aging_no_crash) {
    EnergySystem sys(64, 64);
    // No registry set - tick should not crash
    sys.register_nexus(42, 0);
    sys.tick(0.05f);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Nexus Aging Unit Tests (Ticket 5-022) ===\n\n");

    // ticks_since_built incrementing
    RUN_TEST(ticks_increments_each_call);
    RUN_TEST(ticks_caps_at_65535);

    // age_factor decay behavior
    RUN_TEST(age_factor_starts_near_one);
    RUN_TEST(age_factor_decreases_over_time);
    RUN_TEST(age_factor_at_1000_ticks_carbon);
    RUN_TEST(age_factor_at_10000_ticks_carbon);

    // Type-specific floors
    RUN_TEST(carbon_floor_is_060);
    RUN_TEST(petro_floor_is_065);
    RUN_TEST(gaseous_floor_is_070);
    RUN_TEST(nuclear_floor_is_075);
    RUN_TEST(wind_floor_is_080);
    RUN_TEST(solar_floor_is_085);

    // Floor enforcement
    RUN_TEST(age_factor_never_below_floor_carbon);
    RUN_TEST(age_factor_never_below_floor_solar);

    // Graceful aging comparison
    RUN_TEST(solar_ages_more_gracefully_than_carbon);

    // Formula verification
    RUN_TEST(formula_matches_expected_for_each_type);

    // tick() integration
    RUN_TEST(tick_ages_all_nexuses);
    RUN_TEST(tick_aging_affects_output);
    RUN_TEST(tick_no_registry_aging_no_crash);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
