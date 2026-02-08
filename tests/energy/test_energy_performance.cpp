/**
 * @file test_energy_performance.cpp
 * @brief Performance benchmarks for EnergySystem (Ticket 5-043)
 *
 * Measures critical performance paths with generous CI-safe thresholds:
 * 1. Coverage recalculation: 256x256 grid with 1,000 conduits
 * 2. Pool calculation: 1,000 consumers
 * 3. Full tick(): 128x128 grid
 *
 * Thresholds are 10x relaxed from production targets for CI stability.
 *
 * @see /docs/epics/epic-5/tickets.md (ticket 5-043)
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyConduitComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/energy/EnergyPriorities.h>
#include <entt/entt.hpp>
#include <chrono>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::energy;

// =============================================================================
// Test framework macros
// =============================================================================

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
// Performance thresholds (10x relaxed for CI safety)
// =============================================================================

/// Coverage recalculation: target <10ms, CI threshold <100ms
static constexpr double COVERAGE_RECALC_THRESHOLD_MS = 100.0;

/// Pool calculation with 1,000 consumers: target <1ms, CI threshold <10ms
static constexpr double POOL_CALC_THRESHOLD_MS = 10.0;

/// Full tick at 128x128: target <2ms, CI threshold <20ms
static constexpr double FULL_TICK_THRESHOLD_MS = 20.0;

// =============================================================================
// Timing helper
// =============================================================================

using Clock = std::chrono::high_resolution_clock;

static double elapsed_ms(Clock::time_point start, Clock::time_point end) {
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return static_cast<double>(duration.count()) / 1000.0;
}

// =============================================================================
// Benchmark 1: Coverage recalculation with 1,000 conduits on 256x256 grid
// =============================================================================

TEST(coverage_recalc_1000_conduits_256x256) {
    entt::registry reg;
    constexpr uint32_t MAP_SIZE = 256;
    EnergySystem sys(MAP_SIZE, MAP_SIZE);
    sys.set_registry(&reg);

    // Place a nexus at the center to seed coverage BFS
    uint32_t nexus_id = sys.place_nexus(NexusType::Carbon, MAP_SIZE / 2, MAP_SIZE / 2, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    // Initial tick to establish nexus output
    sys.tick(0.05f);

    // Place 1,000 conduits in a spreading pattern from the nexus.
    // We lay conduits in a grid pattern within a region near the nexus.
    // Conduit coverage_radius=3, so we space them every 4 tiles to form a network.
    uint32_t conduits_placed = 0;
    const uint32_t center = MAP_SIZE / 2;
    // Start adjacent to nexus and spread outward in a spiral-like grid
    for (uint32_t ring = 1; conduits_placed < 1000; ++ring) {
        for (uint32_t dx = 0; dx <= ring * 4 && conduits_placed < 1000; dx += 4) {
            for (uint32_t dy = 0; dy <= ring * 4 && conduits_placed < 1000; dy += 4) {
                // Skip the center (nexus is there)
                if (dx == 0 && dy == 0) continue;

                // Place in all four quadrants
                uint32_t positions[][2] = {
                    { center + dx, center + dy },
                    { center + dx, center - dy },
                    { center - dx, center + dy },
                    { center - dx, center - dy }
                };

                for (auto& pos : positions) {
                    if (conduits_placed >= 1000) break;
                    uint32_t px = pos[0];
                    uint32_t py = pos[1];
                    if (px >= MAP_SIZE || py >= MAP_SIZE) continue;
                    if (px == center && py == center) continue;

                    uint32_t cid = sys.place_conduit(px, py, 0);
                    if (cid != INVALID_ENTITY_ID) {
                        conduits_placed++;
                    }
                }
            }
        }
    }

    printf(" (%u conduits placed) ", conduits_placed);

    // Mark coverage dirty so recalculate_coverage is triggered
    sys.mark_coverage_dirty(0);

    // Time the coverage recalculation (happens during tick)
    auto start = Clock::now();
    sys.recalculate_coverage(0);
    auto end = Clock::now();

    double ms = elapsed_ms(start, end);
    printf("[%.2fms] ", ms);

    // Verify coverage was actually computed
    uint32_t coverage_count = sys.get_coverage_count(1);  // overseer_id = player_id + 1
    ASSERT(coverage_count > 0);

    // Performance check
    ASSERT(ms < COVERAGE_RECALC_THRESHOLD_MS);
}

// =============================================================================
// Benchmark 2: Pool calculation with 1,000 consumers
// =============================================================================

TEST(pool_calc_1000_consumers) {
    entt::registry reg;
    constexpr uint32_t MAP_SIZE = 128;
    EnergySystem sys(MAP_SIZE, MAP_SIZE);
    sys.set_registry(&reg);

    const uint32_t center = MAP_SIZE / 2;

    // Place a Nuclear nexus at the center (coverage_radius=10)
    sys.place_nexus(NexusType::Nuclear, center, center, 0);

    // Extend coverage with conduit chains in all 4 cardinal directions
    // This creates a large coverage area for placing 1,000 consumers.
    // Conduits placed along x-axis and y-axis from nexus, each with coverage_radius=3.
    for (uint32_t d = 1; d <= 20; ++d) {
        if (center + d < MAP_SIZE) sys.place_conduit(center + d, center, 0);
        if (center >= d)           sys.place_conduit(center - d, center, 0);
        if (center + d < MAP_SIZE) sys.place_conduit(center, center + d, 0);
        if (center >= d)           sys.place_conduit(center, center - d, 0);
    }

    // Tick to compute coverage via BFS
    sys.tick(0.05f);

    // Register 1,000 consumers across the coverage area
    // Coverage extends approximately +-23 in x and y from center
    // (nexus radius 10 + conduit chain 20 + conduit radius 3 = 33, but BFS limits it)
    uint32_t consumers_registered = 0;
    for (uint32_t y = center - 22; y <= center + 22 && consumers_registered < 1000; ++y) {
        for (uint32_t x = center - 22; x <= center + 22 && consumers_registered < 1000; ++x) {
            if (x >= MAP_SIZE || y >= MAP_SIZE) continue;
            // Skip tiles occupied by nexus or conduits (avoid collisions)
            if (x == center && y == center) continue;
            if ((x == center || y == center) && x != y) continue;

            // Only place consumers at positions that are in coverage
            if (!sys.is_in_coverage(x, y, 1)) continue;  // overseer_id = 1

            auto entity = reg.create();
            EnergyComponent ec{};
            ec.energy_required = 5;
            ec.priority = ENERGY_PRIORITY_NORMAL;
            reg.emplace<EnergyComponent>(entity, ec);

            uint32_t eid = static_cast<uint32_t>(entity);
            sys.register_consumer(eid, 0);
            sys.register_consumer_position(eid, 0, x, y);
            consumers_registered++;
        }
    }

    printf(" (%u consumers) ", consumers_registered);

    // Ensure coverage is current
    sys.recalculate_coverage(0);

    // Time pool calculation
    auto start = Clock::now();
    sys.calculate_pool(0);
    auto end = Clock::now();

    double ms = elapsed_ms(start, end);
    printf("[%.2fms] ", ms);

    // Verify pool was calculated
    const auto& pool = sys.get_pool(0);
    ASSERT(pool.total_consumed > 0);
    ASSERT(pool.total_generated > 0);
    ASSERT(consumers_registered >= 500);  // Should get many consumers, at least 500

    // Performance check
    ASSERT(ms < POOL_CALC_THRESHOLD_MS);
}

// =============================================================================
// Benchmark 3: Full tick() at 128x128
// =============================================================================

TEST(full_tick_128x128) {
    entt::registry reg;
    constexpr uint32_t MAP_SIZE = 128;
    EnergySystem sys(MAP_SIZE, MAP_SIZE);
    sys.set_registry(&reg);

    // Place a nexus
    sys.place_nexus(NexusType::Carbon, MAP_SIZE / 2, MAP_SIZE / 2, 0);

    // Place some conduits to form a network
    const uint32_t center = MAP_SIZE / 2;
    uint32_t conduits_placed = 0;
    for (uint32_t x = center - 20; x <= center + 20; x += 4) {
        for (uint32_t y = center - 20; y <= center + 20; y += 4) {
            if (x == center && y == center) continue;
            if (x >= MAP_SIZE || y >= MAP_SIZE) continue;
            uint32_t cid = sys.place_conduit(x, y, 0);
            if (cid != INVALID_ENTITY_ID) {
                conduits_placed++;
            }
        }
    }

    // Create 100 consumers in coverage
    uint32_t consumers_registered = 0;
    for (uint32_t x = center - 8; x <= center + 8 && consumers_registered < 100; ++x) {
        for (uint32_t y = center - 8; y <= center + 8 && consumers_registered < 100; ++y) {
            if (x == center && y == center) continue;
            if (x >= MAP_SIZE || y >= MAP_SIZE) continue;

            auto entity = reg.create();
            EnergyComponent ec{};
            ec.energy_required = 5;
            ec.priority = ENERGY_PRIORITY_NORMAL;
            reg.emplace<EnergyComponent>(entity, ec);

            uint32_t eid = static_cast<uint32_t>(entity);
            sys.register_consumer(eid, 0);
            sys.register_consumer_position(eid, 0, x, y);
            consumers_registered++;
        }
    }

    // First tick to warm up (initial coverage computation)
    sys.tick(0.05f);

    // Mark dirty to force full recomputation on next tick
    sys.mark_coverage_dirty(0);

    printf(" (%u conduits, %u consumers) ", conduits_placed, consumers_registered);

    // Time a full tick
    auto start = Clock::now();
    sys.tick(0.05f);
    auto end = Clock::now();

    double ms = elapsed_ms(start, end);
    printf("[%.2fms] ", ms);

    // Verify tick produced valid results
    const auto& pool = sys.get_pool(0);
    ASSERT(pool.total_generated > 0);

    // Performance check
    ASSERT(ms < FULL_TICK_THRESHOLD_MS);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== EnergySystem Performance Benchmarks (Ticket 5-043) ===\n\n");

    RUN_TEST(coverage_recalc_1000_conduits_256x256);
    RUN_TEST(pool_calc_1000_consumers);
    RUN_TEST(full_tick_128x128);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    printf("\nThresholds (10x relaxed for CI):\n");
    printf("  Coverage recalc: < %.0fms\n", COVERAGE_RECALC_THRESHOLD_MS);
    printf("  Pool calc:       < %.0fms\n", POOL_CALC_THRESHOLD_MS);
    printf("  Full tick:       < %.0fms\n", FULL_TICK_THRESHOLD_MS);

    return tests_failed > 0 ? 1 : 0;
}
