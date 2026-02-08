/**
 * @file test_stub_replacement.cpp
 * @brief Unit tests for stub replacement and grace period (Epic 7, Ticket E7-019)
 *
 * Tests cover:
 * - Grace period activation and expiration
 * - is_road_accessible_at returns true during grace period
 * - is_road_accessible_at uses real data after grace period
 * - update_tick tracks simulation time correctly
 * - TransportAccessLostEvent emission on access denial
 * - No cache = permissive behavior (like stub)
 * - Grace period config defaults
 * - Dependency injection strategy (ITransportProvider polymorphism)
 */

#include <sims3000/transport/TransportProviderImpl.h>
#include <sims3000/transport/PathwayGrid.h>
#include <sims3000/transport/ProximityCache.h>
#include <sims3000/transport/NetworkGraph.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/building/ForwardDependencyStubs.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::transport;
using namespace sims3000::building;

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
        printf("\n  FAILED: %s == %s (line %d, got %d vs %d)\n", #a, #b, __LINE__, (int)(a), (int)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// ============================================================================
// Helper: configured test fixture
// ============================================================================

struct TestFixture {
    PathwayGrid grid;
    ProximityCache cache;
    NetworkGraph graph;
    TransportProviderImpl provider;

    TestFixture(uint32_t w, uint32_t h)
        : grid(w, h), cache(w, h)
    {
        provider.set_proximity_cache(&cache);
        provider.set_pathway_grid(&grid);
        provider.set_network_graph(&graph);
    }

    void rebuild() {
        cache.mark_dirty();
        cache.rebuild_if_dirty(grid);
        graph.rebuild_from_grid(grid);
    }
};

// ============================================================================
// Grace period defaults
// ============================================================================

TEST(grace_config_defaults) {
    TransportProviderImpl provider;
    const TransportGraceConfig& cfg = provider.get_grace_config();

    ASSERT_EQ(cfg.grace_period_ticks, 500u);
    ASSERT(cfg.grace_active == false);
    ASSERT_EQ(cfg.grace_start_tick, 0u);
}

// ============================================================================
// Grace period activation
// ============================================================================

TEST(activate_grace_period) {
    TransportProviderImpl provider;
    provider.activate_grace_period(100);

    const TransportGraceConfig& cfg = provider.get_grace_config();
    ASSERT(cfg.grace_active == true);
    ASSERT_EQ(cfg.grace_start_tick, 100u);
}

TEST(is_in_grace_period_active) {
    TransportProviderImpl provider;
    provider.activate_grace_period(100);

    // At start tick
    ASSERT(provider.is_in_grace_period(100) == true);
    // Midway through
    ASSERT(provider.is_in_grace_period(350) == true);
    // Just before expiry (100 + 500 - 1 = 599)
    ASSERT(provider.is_in_grace_period(599) == true);
}

TEST(is_in_grace_period_expired) {
    TransportProviderImpl provider;
    provider.activate_grace_period(100);

    // At expiry tick (100 + 500 = 600)
    ASSERT(provider.is_in_grace_period(600) == false);
    // Well past expiry
    ASSERT(provider.is_in_grace_period(1000) == false);
}

TEST(is_in_grace_period_not_activated) {
    TransportProviderImpl provider;
    ASSERT(provider.is_in_grace_period(0) == false);
    ASSERT(provider.is_in_grace_period(100) == false);
}

// ============================================================================
// Grace period affects is_road_accessible_at
// ============================================================================

TEST(accessible_during_grace_period_no_roads) {
    TestFixture f(16, 16);
    // No pathways placed
    f.rebuild();

    // Without grace period, no roads means not accessible
    f.provider.update_tick(100);
    ASSERT(f.provider.is_road_accessible_at(5, 5, 3) == false);

    // Activate grace period at tick 100
    f.provider.activate_grace_period(100);
    f.provider.update_tick(100);

    // During grace period, should return true even without roads
    ASSERT(f.provider.is_road_accessible_at(5, 5, 3) == true);
    ASSERT(f.provider.is_road_accessible_at(0, 0, 3) == true);
    ASSERT(f.provider.is_road_accessible_at(15, 15, 3) == true);
}

TEST(not_accessible_after_grace_period_expires) {
    TestFixture f(16, 16);
    // No pathways placed
    f.rebuild();

    f.provider.activate_grace_period(100);

    // During grace period (tick 200)
    f.provider.update_tick(200);
    ASSERT(f.provider.is_road_accessible_at(5, 5, 3) == true);

    // After grace period expires (tick 600 = 100 + 500)
    f.provider.update_tick(600);
    ASSERT(f.provider.is_road_accessible_at(5, 5, 3) == false);
}

TEST(accessible_with_roads_after_grace_period) {
    TestFixture f(16, 16);
    // Place a pathway
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    f.provider.activate_grace_period(100);

    // After grace period, real checks apply
    f.provider.update_tick(700);

    // On the pathway: accessible
    ASSERT(f.provider.is_road_accessible_at(5, 5, 3) == true);
    // Adjacent: accessible
    ASSERT(f.provider.is_road_accessible_at(6, 5, 3) == true);
    // Far away: not accessible
    ASSERT(f.provider.is_road_accessible_at(15, 15, 3) == false);
}

// ============================================================================
// No cache = permissive (like stub)
// ============================================================================

TEST(no_cache_is_permissive) {
    TransportProviderImpl provider;
    // No cache, no grid, no graph set

    // With no cache, should be permissive (return true)
    provider.update_tick(0);
    ASSERT(provider.is_road_accessible_at(5, 5, 3) == true);
    ASSERT(provider.is_road_accessible_at(0, 0, 0) == true);
}

// ============================================================================
// update_tick
// ============================================================================

TEST(update_tick_tracks_time) {
    TestFixture f(16, 16);
    f.rebuild();

    f.provider.activate_grace_period(0);

    // Tick 0: in grace period
    f.provider.update_tick(0);
    ASSERT(f.provider.is_road_accessible_at(5, 5, 3) == true);

    // Tick 250: still in grace period
    f.provider.update_tick(250);
    ASSERT(f.provider.is_road_accessible_at(5, 5, 3) == true);

    // Tick 499: still in grace period
    f.provider.update_tick(499);
    ASSERT(f.provider.is_road_accessible_at(5, 5, 3) == true);

    // Tick 500: grace period expired
    f.provider.update_tick(500);
    ASSERT(f.provider.is_road_accessible_at(5, 5, 3) == false);
}

// ============================================================================
// TransportAccessLostEvent emission
// ============================================================================

TEST(access_lost_event_emitted_on_denial) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    f.provider.update_tick(0);

    // Query an inaccessible position (far from road)
    bool accessible = f.provider.is_road_accessible_at(15, 15, 3);
    ASSERT(accessible == false);

    // Should have emitted an access lost event
    auto events = f.provider.drain_access_lost_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].x, 15u);
    ASSERT_EQ(events[0].y, 15u);
    ASSERT_EQ(events[0].max_distance, 3u);
}

TEST(no_event_when_accessible) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    f.provider.update_tick(0);

    // Query an accessible position
    bool accessible = f.provider.is_road_accessible_at(5, 5, 3);
    ASSERT(accessible == true);

    // No events
    auto events = f.provider.drain_access_lost_events();
    ASSERT_EQ(events.size(), 0u);
}

TEST(no_event_during_grace_period) {
    TestFixture f(16, 16);
    f.rebuild();  // No pathways

    f.provider.activate_grace_period(0);
    f.provider.update_tick(0);

    // During grace period, no denial = no events
    bool accessible = f.provider.is_road_accessible_at(5, 5, 3);
    ASSERT(accessible == true);

    auto events = f.provider.drain_access_lost_events();
    ASSERT_EQ(events.size(), 0u);
}

TEST(drain_clears_events) {
    TestFixture f(16, 16);
    f.grid.set_pathway(0, 0, 1);
    f.rebuild();

    f.provider.update_tick(0);

    // Trigger two denials
    f.provider.is_road_accessible_at(15, 15, 3);
    f.provider.is_road_accessible_at(14, 14, 3);

    auto events = f.provider.drain_access_lost_events();
    ASSERT_EQ(events.size(), 2u);

    // Drain again - should be empty
    auto events2 = f.provider.drain_access_lost_events();
    ASSERT_EQ(events2.size(), 0u);
}

TEST(multiple_access_lost_events) {
    TestFixture f(16, 16);
    f.grid.set_pathway(0, 0, 1);
    f.rebuild();

    f.provider.update_tick(0);

    // Multiple inaccessible queries
    f.provider.is_road_accessible_at(10, 10, 3);
    f.provider.is_road_accessible_at(11, 11, 3);
    f.provider.is_road_accessible_at(12, 12, 3);

    auto events = f.provider.drain_access_lost_events();
    ASSERT_EQ(events.size(), 3u);

    ASSERT_EQ(events[0].x, 10u);
    ASSERT_EQ(events[0].y, 10u);
    ASSERT_EQ(events[1].x, 11u);
    ASSERT_EQ(events[1].y, 11u);
    ASSERT_EQ(events[2].x, 12u);
    ASSERT_EQ(events[2].y, 12u);
}

// ============================================================================
// Dependency injection via ITransportProvider
// ============================================================================

TEST(polymorphic_injection_real) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    // Use real provider through interface pointer
    ITransportProvider* iface = &f.provider;
    ASSERT(iface->is_road_accessible_at(5, 5, 3) == true);
    ASSERT(iface->is_road_accessible_at(15, 15, 3) == false);
}

TEST(polymorphic_injection_stub) {
    StubTransportProvider stub;

    // Use stub through interface pointer
    ITransportProvider* iface = &stub;
    ASSERT(iface->is_road_accessible_at(5, 5, 3) == true);
    ASSERT(iface->is_road_accessible_at(15, 15, 3) == true);  // Stub always true
}

TEST(swap_stub_for_real) {
    // Simulates the Application.cpp swap pattern
    StubTransportProvider stub;
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    // Phase 1: Using stub
    ITransportProvider* provider = &stub;
    ASSERT(provider->is_road_accessible_at(15, 15, 3) == true);  // Stub permissive

    // Phase 2: Swap to real provider with grace period
    f.provider.activate_grace_period(0);
    f.provider.update_tick(0);
    provider = &f.provider;

    // Still permissive during grace period
    ASSERT(provider->is_road_accessible_at(15, 15, 3) == true);

    // Phase 3: After grace period, real checks apply
    f.provider.update_tick(500);
    ASSERT(provider->is_road_accessible_at(15, 15, 3) == false);
    ASSERT(provider->is_road_accessible_at(5, 5, 3) == true);
}

// ============================================================================
// TransportAccessLostEvent struct
// ============================================================================

TEST(access_lost_event_default_constructor) {
    TransportAccessLostEvent event;
    ASSERT_EQ(event.x, 0u);
    ASSERT_EQ(event.y, 0u);
    ASSERT_EQ(event.max_distance, 0u);
    ASSERT_EQ(event.actual_distance, 0u);
}

TEST(access_lost_event_parameterized_constructor) {
    TransportAccessLostEvent event(10, 20, 3, 15);
    ASSERT_EQ(event.x, 10u);
    ASSERT_EQ(event.y, 20u);
    ASSERT_EQ(event.max_distance, 3u);
    ASSERT_EQ(event.actual_distance, 15u);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Stub Replacement Tests (Ticket E7-019) ===\n\n");

    // Grace period config
    RUN_TEST(grace_config_defaults);

    // Grace period activation
    RUN_TEST(activate_grace_period);
    RUN_TEST(is_in_grace_period_active);
    RUN_TEST(is_in_grace_period_expired);
    RUN_TEST(is_in_grace_period_not_activated);

    // Grace period affects accessibility
    RUN_TEST(accessible_during_grace_period_no_roads);
    RUN_TEST(not_accessible_after_grace_period_expires);
    RUN_TEST(accessible_with_roads_after_grace_period);

    // No cache = permissive
    RUN_TEST(no_cache_is_permissive);

    // update_tick
    RUN_TEST(update_tick_tracks_time);

    // TransportAccessLostEvent emission
    RUN_TEST(access_lost_event_emitted_on_denial);
    RUN_TEST(no_event_when_accessible);
    RUN_TEST(no_event_during_grace_period);
    RUN_TEST(drain_clears_events);
    RUN_TEST(multiple_access_lost_events);

    // Dependency injection
    RUN_TEST(polymorphic_injection_real);
    RUN_TEST(polymorphic_injection_stub);
    RUN_TEST(swap_stub_for_real);

    // Event struct
    RUN_TEST(access_lost_event_default_constructor);
    RUN_TEST(access_lost_event_parameterized_constructor);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
