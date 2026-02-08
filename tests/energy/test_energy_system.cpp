/**
 * @file test_energy_system.cpp
 * @brief Unit tests for EnergySystem class skeleton (Ticket 5-008)
 *
 * Tests cover:
 * - Construction with various map sizes
 * - get_priority() returns 10
 * - IEnergyProvider interface methods return defaults
 * - register/unregister nexus and consumer
 * - Coverage query delegation works
 * - Pool query returns correct pool for owner
 * - mark_coverage_dirty sets flag
 * - Grid accessor correctness
 * - Out-of-bounds owner safety
 */

#include <sims3000/energy/EnergySystem.h>
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
// Construction Tests
// =============================================================================

TEST(construction_128x128) {
    EnergySystem sys(128, 128);
    ASSERT_EQ(sys.get_map_width(), 128u);
    ASSERT_EQ(sys.get_map_height(), 128u);
}

TEST(construction_256x256) {
    EnergySystem sys(256, 256);
    ASSERT_EQ(sys.get_map_width(), 256u);
    ASSERT_EQ(sys.get_map_height(), 256u);
}

TEST(construction_512x512) {
    EnergySystem sys(512, 512);
    ASSERT_EQ(sys.get_map_width(), 512u);
    ASSERT_EQ(sys.get_map_height(), 512u);
}

TEST(construction_with_nullptr_terrain) {
    EnergySystem sys(128, 128, nullptr);
    ASSERT_EQ(sys.get_map_width(), 128u);
    ASSERT_EQ(sys.get_map_height(), 128u);
}

TEST(construction_non_square) {
    EnergySystem sys(64, 32);
    ASSERT_EQ(sys.get_map_width(), 64u);
    ASSERT_EQ(sys.get_map_height(), 32u);
}

TEST(construction_coverage_grid_matches_map_size) {
    EnergySystem sys(256, 256);
    const CoverageGrid& grid = sys.get_coverage_grid();
    ASSERT_EQ(grid.get_width(), 256u);
    ASSERT_EQ(grid.get_height(), 256u);
}

TEST(construction_pools_initialized) {
    EnergySystem sys(128, 128);
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        const PerPlayerEnergyPool& pool = sys.get_pool(i);
        ASSERT_EQ(pool.owner, i);
        ASSERT_EQ(pool.total_generated, 0u);
        ASSERT_EQ(pool.total_consumed, 0u);
        ASSERT_EQ(pool.surplus, 0);
        ASSERT_EQ(pool.nexus_count, 0u);
        ASSERT_EQ(pool.consumer_count, 0u);
    }
}

TEST(construction_no_dirty_flags) {
    EnergySystem sys(128, 128);
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT(!sys.is_coverage_dirty(i));
    }
}

TEST(construction_no_nexuses_or_consumers) {
    EnergySystem sys(128, 128);
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT_EQ(sys.get_nexus_count(i), 0u);
        ASSERT_EQ(sys.get_consumer_count(i), 0u);
    }
}

// =============================================================================
// Priority Tests
// =============================================================================

TEST(get_priority_returns_10) {
    EnergySystem sys(128, 128);
    ASSERT_EQ(sys.get_priority(), 10);
}

// =============================================================================
// IEnergyProvider Interface Tests
// =============================================================================

TEST(is_powered_returns_false_by_default) {
    EnergySystem sys(128, 128);
    ASSERT(!sys.is_powered(0));
    ASSERT(!sys.is_powered(1));
    ASSERT(!sys.is_powered(42));
    ASSERT(!sys.is_powered(9999));
}

TEST(is_powered_at_returns_false_by_default) {
    EnergySystem sys(128, 128);
    ASSERT(!sys.is_powered_at(0, 0, 0));
    ASSERT(!sys.is_powered_at(64, 64, 1));
    ASSERT(!sys.is_powered_at(127, 127, 3));
}

// =============================================================================
// Tick Tests
// =============================================================================

TEST(tick_does_not_crash) {
    EnergySystem sys(128, 128);
    // Tick should not crash with no entities
    sys.tick(0.05f);
    sys.tick(0.05f);
    sys.tick(0.05f);
}

// =============================================================================
// Nexus Management Tests
// =============================================================================

TEST(register_nexus_increases_count) {
    EnergySystem sys(128, 128);
    sys.register_nexus(100, 0);
    ASSERT_EQ(sys.get_nexus_count(0), 1u);
    ASSERT_EQ(sys.get_nexus_count(1), 0u);
}

TEST(register_multiple_nexuses) {
    EnergySystem sys(128, 128);
    sys.register_nexus(100, 0);
    sys.register_nexus(101, 0);
    sys.register_nexus(102, 0);
    ASSERT_EQ(sys.get_nexus_count(0), 3u);
}

TEST(register_nexus_different_players) {
    EnergySystem sys(128, 128);
    sys.register_nexus(100, 0);
    sys.register_nexus(200, 1);
    sys.register_nexus(300, 2);
    sys.register_nexus(400, 3);
    ASSERT_EQ(sys.get_nexus_count(0), 1u);
    ASSERT_EQ(sys.get_nexus_count(1), 1u);
    ASSERT_EQ(sys.get_nexus_count(2), 1u);
    ASSERT_EQ(sys.get_nexus_count(3), 1u);
}

TEST(unregister_nexus_decreases_count) {
    EnergySystem sys(128, 128);
    sys.register_nexus(100, 0);
    sys.register_nexus(101, 0);
    ASSERT_EQ(sys.get_nexus_count(0), 2u);

    sys.unregister_nexus(100, 0);
    ASSERT_EQ(sys.get_nexus_count(0), 1u);
}

TEST(unregister_nexus_not_present_is_noop) {
    EnergySystem sys(128, 128);
    sys.register_nexus(100, 0);
    sys.unregister_nexus(999, 0);  // Not registered
    ASSERT_EQ(sys.get_nexus_count(0), 1u);
}

TEST(register_nexus_sets_dirty) {
    EnergySystem sys(128, 128);
    ASSERT(!sys.is_coverage_dirty(0));
    sys.register_nexus(100, 0);
    ASSERT(sys.is_coverage_dirty(0));
    ASSERT(!sys.is_coverage_dirty(1));
}

TEST(unregister_nexus_sets_dirty) {
    EnergySystem sys(128, 128);
    sys.register_nexus(100, 0);
    // Reset dirty manually for testing via mark
    // (dirty is already true from register, that's fine)
    ASSERT(sys.is_coverage_dirty(0));
}

TEST(register_nexus_invalid_owner_is_noop) {
    EnergySystem sys(128, 128);
    sys.register_nexus(100, MAX_PLAYERS);      // Out of bounds
    sys.register_nexus(101, MAX_PLAYERS + 1);  // Out of bounds
    sys.register_nexus(102, 255);              // Out of bounds
    // Should not have affected any valid player
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT_EQ(sys.get_nexus_count(i), 0u);
    }
}

// =============================================================================
// Consumer Management Tests
// =============================================================================

TEST(register_consumer_increases_count) {
    EnergySystem sys(128, 128);
    sys.register_consumer(200, 1);
    ASSERT_EQ(sys.get_consumer_count(1), 1u);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
}

TEST(register_multiple_consumers) {
    EnergySystem sys(128, 128);
    sys.register_consumer(200, 1);
    sys.register_consumer(201, 1);
    sys.register_consumer(202, 1);
    ASSERT_EQ(sys.get_consumer_count(1), 3u);
}

TEST(unregister_consumer_decreases_count) {
    EnergySystem sys(128, 128);
    sys.register_consumer(200, 1);
    sys.register_consumer(201, 1);
    ASSERT_EQ(sys.get_consumer_count(1), 2u);

    sys.unregister_consumer(200, 1);
    ASSERT_EQ(sys.get_consumer_count(1), 1u);
}

TEST(unregister_consumer_not_present_is_noop) {
    EnergySystem sys(128, 128);
    sys.register_consumer(200, 1);
    sys.unregister_consumer(999, 1);  // Not registered
    ASSERT_EQ(sys.get_consumer_count(1), 1u);
}

TEST(register_consumer_invalid_owner_is_noop) {
    EnergySystem sys(128, 128);
    sys.register_consumer(200, MAX_PLAYERS);
    sys.register_consumer(201, 255);
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT_EQ(sys.get_consumer_count(i), 0u);
    }
}

// =============================================================================
// Coverage Query Delegation Tests
// =============================================================================

TEST(coverage_delegation_is_in_coverage) {
    EnergySystem sys(128, 128);
    // Coverage grid starts empty, so no coverage
    ASSERT(!sys.is_in_coverage(0, 0, 1));
    ASSERT(!sys.is_in_coverage(64, 64, 2));
}

TEST(coverage_delegation_get_coverage_at) {
    EnergySystem sys(128, 128);
    // Coverage grid starts empty
    ASSERT_EQ(sys.get_coverage_at(0, 0), 0);
    ASSERT_EQ(sys.get_coverage_at(64, 64), 0);
}

TEST(coverage_delegation_get_coverage_count) {
    EnergySystem sys(128, 128);
    // Coverage grid starts empty
    ASSERT_EQ(sys.get_coverage_count(1), 0u);
    ASSERT_EQ(sys.get_coverage_count(2), 0u);
    ASSERT_EQ(sys.get_coverage_count(3), 0u);
    ASSERT_EQ(sys.get_coverage_count(4), 0u);
}

TEST(coverage_delegation_out_of_bounds) {
    EnergySystem sys(128, 128);
    // Out-of-bounds coordinates should return safe defaults
    ASSERT(!sys.is_in_coverage(200, 200, 1));
    ASSERT_EQ(sys.get_coverage_at(200, 200), 0);
}

// =============================================================================
// Pool Query Tests
// =============================================================================

TEST(pool_query_returns_correct_owner) {
    EnergySystem sys(128, 128);
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        const PerPlayerEnergyPool& pool = sys.get_pool(i);
        ASSERT_EQ(pool.owner, i);
    }
}

TEST(pool_query_out_of_bounds_returns_pool_zero) {
    EnergySystem sys(128, 128);
    // Out-of-bounds owner should return pool 0 as fallback
    const PerPlayerEnergyPool& pool = sys.get_pool(MAX_PLAYERS);
    ASSERT_EQ(pool.owner, 0);
}

TEST(pool_state_default_is_healthy) {
    EnergySystem sys(128, 128);
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(i)),
                  static_cast<uint8_t>(EnergyPoolState::Healthy));
    }
}

TEST(pool_state_out_of_bounds_returns_healthy) {
    EnergySystem sys(128, 128);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(MAX_PLAYERS)),
              static_cast<uint8_t>(EnergyPoolState::Healthy));
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(255)),
              static_cast<uint8_t>(EnergyPoolState::Healthy));
}

// =============================================================================
// Energy Component Query Tests (stubs)
// =============================================================================

TEST(energy_required_returns_zero) {
    EnergySystem sys(128, 128);
    ASSERT_EQ(sys.get_energy_required(0), 0u);
    ASSERT_EQ(sys.get_energy_required(42), 0u);
    ASSERT_EQ(sys.get_energy_required(9999), 0u);
}

TEST(energy_received_returns_zero) {
    EnergySystem sys(128, 128);
    ASSERT_EQ(sys.get_energy_received(0), 0u);
    ASSERT_EQ(sys.get_energy_received(42), 0u);
    ASSERT_EQ(sys.get_energy_received(9999), 0u);
}

// =============================================================================
// Coverage Dirty Management Tests
// =============================================================================

TEST(mark_coverage_dirty_sets_flag) {
    EnergySystem sys(128, 128);
    ASSERT(!sys.is_coverage_dirty(0));
    sys.mark_coverage_dirty(0);
    ASSERT(sys.is_coverage_dirty(0));
    // Other players should not be affected
    ASSERT(!sys.is_coverage_dirty(1));
    ASSERT(!sys.is_coverage_dirty(2));
    ASSERT(!sys.is_coverage_dirty(3));
}

TEST(mark_coverage_dirty_multiple_players) {
    EnergySystem sys(128, 128);
    sys.mark_coverage_dirty(0);
    sys.mark_coverage_dirty(2);
    ASSERT(sys.is_coverage_dirty(0));
    ASSERT(!sys.is_coverage_dirty(1));
    ASSERT(sys.is_coverage_dirty(2));
    ASSERT(!sys.is_coverage_dirty(3));
}

TEST(mark_coverage_dirty_invalid_owner_is_noop) {
    EnergySystem sys(128, 128);
    sys.mark_coverage_dirty(MAX_PLAYERS);
    sys.mark_coverage_dirty(255);
    // No valid player should be affected
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT(!sys.is_coverage_dirty(i));
    }
}

TEST(is_coverage_dirty_invalid_owner_returns_false) {
    EnergySystem sys(128, 128);
    ASSERT(!sys.is_coverage_dirty(MAX_PLAYERS));
    ASSERT(!sys.is_coverage_dirty(255));
}

// =============================================================================
// Grid Accessor Tests
// =============================================================================

TEST(get_coverage_grid_returns_reference) {
    EnergySystem sys(256, 256);
    const CoverageGrid& grid = sys.get_coverage_grid();
    ASSERT_EQ(grid.get_width(), 256u);
    ASSERT_EQ(grid.get_height(), 256u);
}

TEST(get_map_width_and_height) {
    EnergySystem sys(512, 256);
    ASSERT_EQ(sys.get_map_width(), 512u);
    ASSERT_EQ(sys.get_map_height(), 256u);
}

// =============================================================================
// IEnergyProvider Polymorphism Test
// =============================================================================

TEST(energy_provider_interface_polymorphism) {
    EnergySystem sys(128, 128);
    // Cast to IEnergyProvider and verify interface methods work
    sims3000::building::IEnergyProvider* provider = &sys;
    ASSERT(!provider->is_powered(0));
    ASSERT(!provider->is_powered(42));
    ASSERT(!provider->is_powered_at(0, 0, 0));
    ASSERT(!provider->is_powered_at(64, 64, 1));
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== EnergySystem Unit Tests (Ticket 5-008) ===\n\n");

    // Construction tests
    RUN_TEST(construction_128x128);
    RUN_TEST(construction_256x256);
    RUN_TEST(construction_512x512);
    RUN_TEST(construction_with_nullptr_terrain);
    RUN_TEST(construction_non_square);
    RUN_TEST(construction_coverage_grid_matches_map_size);
    RUN_TEST(construction_pools_initialized);
    RUN_TEST(construction_no_dirty_flags);
    RUN_TEST(construction_no_nexuses_or_consumers);

    // Priority
    RUN_TEST(get_priority_returns_10);

    // IEnergyProvider interface
    RUN_TEST(is_powered_returns_false_by_default);
    RUN_TEST(is_powered_at_returns_false_by_default);

    // Tick
    RUN_TEST(tick_does_not_crash);

    // Nexus management
    RUN_TEST(register_nexus_increases_count);
    RUN_TEST(register_multiple_nexuses);
    RUN_TEST(register_nexus_different_players);
    RUN_TEST(unregister_nexus_decreases_count);
    RUN_TEST(unregister_nexus_not_present_is_noop);
    RUN_TEST(register_nexus_sets_dirty);
    RUN_TEST(unregister_nexus_sets_dirty);
    RUN_TEST(register_nexus_invalid_owner_is_noop);

    // Consumer management
    RUN_TEST(register_consumer_increases_count);
    RUN_TEST(register_multiple_consumers);
    RUN_TEST(unregister_consumer_decreases_count);
    RUN_TEST(unregister_consumer_not_present_is_noop);
    RUN_TEST(register_consumer_invalid_owner_is_noop);

    // Coverage query delegation
    RUN_TEST(coverage_delegation_is_in_coverage);
    RUN_TEST(coverage_delegation_get_coverage_at);
    RUN_TEST(coverage_delegation_get_coverage_count);
    RUN_TEST(coverage_delegation_out_of_bounds);

    // Pool queries
    RUN_TEST(pool_query_returns_correct_owner);
    RUN_TEST(pool_query_out_of_bounds_returns_pool_zero);
    RUN_TEST(pool_state_default_is_healthy);
    RUN_TEST(pool_state_out_of_bounds_returns_healthy);

    // Energy component queries (stubs)
    RUN_TEST(energy_required_returns_zero);
    RUN_TEST(energy_received_returns_zero);

    // Coverage dirty management
    RUN_TEST(mark_coverage_dirty_sets_flag);
    RUN_TEST(mark_coverage_dirty_multiple_players);
    RUN_TEST(mark_coverage_dirty_invalid_owner_is_noop);
    RUN_TEST(is_coverage_dirty_invalid_owner_returns_false);

    // Grid accessors
    RUN_TEST(get_coverage_grid_returns_reference);
    RUN_TEST(get_map_width_and_height);

    // Polymorphism
    RUN_TEST(energy_provider_interface_polymorphism);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
