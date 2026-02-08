/**
 * @file test_pathway_decay.cpp
 * @brief Unit tests for PathwayDecay (Epic 7, Ticket E7-025)
 *
 * Tests:
 * - Decay runs every 100 ticks (should_decay)
 * - Traffic multiplier calculation (1.0 to 3.0 range)
 * - Health reduction with and without traffic
 * - Threshold crossing detection
 * - Edge cases: zero health, zero capacity, null traffic
 */

#include <sims3000/transport/PathwayDecay.h>
#include <sims3000/transport/TransportEvents.h>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000::transport;

// Helper: float approximate equality
static bool approx_eq(float a, float b, float eps = 0.01f) {
    return std::fabs(a - b) < eps;
}

void test_should_decay_at_100_ticks() {
    printf("Testing should_decay at 100-tick intervals...\n");

    DecayConfig cfg{};
    assert(cfg.decay_cycle_ticks == 100);

    // Tick 0 is a decay tick (0 % 100 == 0)
    assert(PathwayDecay::should_decay(0, cfg) == true);

    // Ticks 1-99 are not decay ticks
    assert(PathwayDecay::should_decay(1, cfg) == false);
    assert(PathwayDecay::should_decay(50, cfg) == false);
    assert(PathwayDecay::should_decay(99, cfg) == false);

    // Tick 100 is a decay tick
    assert(PathwayDecay::should_decay(100, cfg) == true);

    // Tick 200 is a decay tick
    assert(PathwayDecay::should_decay(200, cfg) == true);

    // Tick 300 is a decay tick
    assert(PathwayDecay::should_decay(300, cfg) == true);

    printf("  PASS: Decay runs every 100 ticks\n");
}

void test_should_decay_custom_interval() {
    printf("Testing should_decay with custom interval...\n");

    DecayConfig cfg{};
    cfg.decay_cycle_ticks = 50;

    assert(PathwayDecay::should_decay(0, cfg) == true);
    assert(PathwayDecay::should_decay(25, cfg) == false);
    assert(PathwayDecay::should_decay(50, cfg) == true);
    assert(PathwayDecay::should_decay(100, cfg) == true);

    printf("  PASS: Custom interval works correctly\n");
}

void test_should_decay_zero_interval() {
    printf("Testing should_decay with zero interval...\n");

    DecayConfig cfg{};
    cfg.decay_cycle_ticks = 0;

    // Zero interval should never trigger decay (avoid division by zero)
    assert(PathwayDecay::should_decay(0, cfg) == false);
    assert(PathwayDecay::should_decay(100, cfg) == false);

    printf("  PASS: Zero interval returns false\n");
}

void test_traffic_multiplier_no_traffic() {
    printf("Testing traffic multiplier with no traffic...\n");

    RoadComponent road{};
    road.base_capacity = 100;

    // No traffic component: multiplier should be 1.0
    float mult = PathwayDecay::get_traffic_multiplier(road, nullptr);
    assert(approx_eq(mult, 1.0f));

    printf("  PASS: No traffic = 1.0x multiplier\n");
}

void test_traffic_multiplier_zero_flow() {
    printf("Testing traffic multiplier with zero flow...\n");

    RoadComponent road{};
    road.base_capacity = 100;

    TrafficComponent traffic{};
    traffic.flow_current = 0;

    // Zero flow: 1.0 + 2.0 * (0/100) = 1.0
    float mult = PathwayDecay::get_traffic_multiplier(road, &traffic);
    assert(approx_eq(mult, 1.0f));

    printf("  PASS: Zero flow = 1.0x multiplier\n");
}

void test_traffic_multiplier_half_capacity() {
    printf("Testing traffic multiplier at half capacity...\n");

    RoadComponent road{};
    road.base_capacity = 100;

    TrafficComponent traffic{};
    traffic.flow_current = 50;

    // Half capacity: 1.0 + 2.0 * (50/100) = 2.0
    float mult = PathwayDecay::get_traffic_multiplier(road, &traffic);
    assert(approx_eq(mult, 2.0f));

    printf("  PASS: Half capacity = 2.0x multiplier\n");
}

void test_traffic_multiplier_full_capacity() {
    printf("Testing traffic multiplier at full capacity...\n");

    RoadComponent road{};
    road.base_capacity = 100;

    TrafficComponent traffic{};
    traffic.flow_current = 100;

    // Full capacity: 1.0 + 2.0 * (100/100) = 3.0
    float mult = PathwayDecay::get_traffic_multiplier(road, &traffic);
    assert(approx_eq(mult, 3.0f));

    printf("  PASS: Full capacity = 3.0x multiplier\n");
}

void test_traffic_multiplier_capped_at_max() {
    printf("Testing traffic multiplier capped at max...\n");

    RoadComponent road{};
    road.base_capacity = 100;

    TrafficComponent traffic{};
    traffic.flow_current = 200;  // Over capacity

    DecayConfig cfg{};
    cfg.max_traffic_multiplier = 3;

    // Over capacity: 1.0 + 2.0 * (200/100) = 5.0, but capped at 3.0
    float mult = PathwayDecay::get_traffic_multiplier(road, &traffic, cfg);
    assert(approx_eq(mult, 3.0f));

    printf("  PASS: Multiplier capped at 3.0x\n");
}

void test_traffic_multiplier_zero_capacity() {
    printf("Testing traffic multiplier with zero base capacity...\n");

    RoadComponent road{};
    road.base_capacity = 0;

    TrafficComponent traffic{};
    traffic.flow_current = 50;

    // Zero capacity: should return 1.0 (avoid division by zero)
    float mult = PathwayDecay::get_traffic_multiplier(road, &traffic);
    assert(approx_eq(mult, 1.0f));

    printf("  PASS: Zero capacity = 1.0x multiplier\n");
}

void test_apply_decay_basic() {
    printf("Testing basic decay application...\n");

    RoadComponent road{};
    road.health = 255;
    road.base_capacity = 100;

    DecayConfig cfg{};
    cfg.base_decay_per_cycle = 1;

    // No traffic: decay by 1 (1 * 1.0)
    bool crossed = PathwayDecay::apply_decay(road, nullptr, cfg);

    assert(road.health == 254);
    assert(crossed == false);  // 254 is still Pristine (>= 200)

    printf("  PASS: Basic decay reduces health by 1\n");
}

void test_apply_decay_with_traffic() {
    printf("Testing decay with traffic load...\n");

    RoadComponent road{};
    road.health = 255;
    road.base_capacity = 100;

    TrafficComponent traffic{};
    traffic.flow_current = 100;  // Full capacity

    DecayConfig cfg{};
    cfg.base_decay_per_cycle = 1;

    // Full traffic: decay by 3 (1 * 3.0)
    bool crossed = PathwayDecay::apply_decay(road, &traffic, cfg);

    assert(road.health == 252);
    assert(crossed == false);  // Still Pristine

    printf("  PASS: Decay with traffic multiplier applied correctly\n");
}

void test_apply_decay_threshold_crossing() {
    printf("Testing threshold crossing detection...\n");

    RoadComponent road{};
    road.health = 201;  // Just above Pristine->Good boundary (200)
    road.base_capacity = 100;

    DecayConfig cfg{};
    cfg.base_decay_per_cycle = 2;

    // Decay by 2: 201 -> 199 (crosses Pristine->Good boundary at 200)
    bool crossed = PathwayDecay::apply_decay(road, nullptr, cfg);

    assert(road.health == 199);
    assert(crossed == true);

    printf("  PASS: Threshold crossing detected\n");
}

void test_apply_decay_good_to_worn() {
    printf("Testing Good->Worn threshold crossing...\n");

    RoadComponent road{};
    road.health = 151;  // Just above Good->Worn boundary (150)
    road.base_capacity = 100;

    DecayConfig cfg{};
    cfg.base_decay_per_cycle = 2;

    bool crossed = PathwayDecay::apply_decay(road, nullptr, cfg);

    assert(road.health == 149);
    assert(crossed == true);

    printf("  PASS: Good->Worn crossing detected\n");
}

void test_apply_decay_worn_to_poor() {
    printf("Testing Worn->Poor threshold crossing...\n");

    RoadComponent road{};
    road.health = 101;
    road.base_capacity = 100;

    DecayConfig cfg{};
    cfg.base_decay_per_cycle = 2;

    bool crossed = PathwayDecay::apply_decay(road, nullptr, cfg);

    assert(road.health == 99);
    assert(crossed == true);

    printf("  PASS: Worn->Poor crossing detected\n");
}

void test_apply_decay_poor_to_crumbling() {
    printf("Testing Poor->Crumbling threshold crossing...\n");

    RoadComponent road{};
    road.health = 51;
    road.base_capacity = 100;

    DecayConfig cfg{};
    cfg.base_decay_per_cycle = 2;

    bool crossed = PathwayDecay::apply_decay(road, nullptr, cfg);

    assert(road.health == 49);
    assert(crossed == true);

    printf("  PASS: Poor->Crumbling crossing detected\n");
}

void test_apply_decay_clamps_to_zero() {
    printf("Testing decay clamps health to zero...\n");

    RoadComponent road{};
    road.health = 2;
    road.base_capacity = 100;

    DecayConfig cfg{};
    cfg.base_decay_per_cycle = 10;

    // Decay by 10 would go negative, should clamp to 0
    PathwayDecay::apply_decay(road, nullptr, cfg);

    assert(road.health == 0);

    printf("  PASS: Health clamped to 0\n");
}

void test_apply_decay_zero_health_no_change() {
    printf("Testing decay with zero health...\n");

    RoadComponent road{};
    road.health = 0;
    road.base_capacity = 100;

    DecayConfig cfg{};
    cfg.base_decay_per_cycle = 5;

    bool crossed = PathwayDecay::apply_decay(road, nullptr, cfg);

    assert(road.health == 0);
    assert(crossed == false);

    printf("  PASS: Zero health = no decay applied\n");
}

void test_apply_decay_no_crossing_within_same_state() {
    printf("Testing no crossing when staying in same state...\n");

    RoadComponent road{};
    road.health = 230;  // Well within Pristine range
    road.base_capacity = 100;

    DecayConfig cfg{};
    cfg.base_decay_per_cycle = 1;

    bool crossed = PathwayDecay::apply_decay(road, nullptr, cfg);

    assert(road.health == 229);
    assert(crossed == false);

    printf("  PASS: No crossing when staying in same state\n");
}

void test_apply_decay_large_decay_multiple_crossings() {
    printf("Testing large decay crossing multiple thresholds...\n");

    RoadComponent road{};
    road.health = 255;
    road.base_capacity = 100;

    TrafficComponent traffic{};
    traffic.flow_current = 100;  // 3x multiplier

    DecayConfig cfg{};
    cfg.base_decay_per_cycle = 30;  // 30 * 3.0 = 90 decay

    // Health: 255 -> 165 (Pristine -> Good, crosses at 200)
    bool crossed = PathwayDecay::apply_decay(road, &traffic, cfg);

    assert(road.health == 165);
    assert(crossed == true);  // Crossed at least one threshold

    printf("  PASS: Large decay crossing detected\n");
}

int main() {
    printf("=== PathwayDecay Unit Tests (Epic 7, Ticket E7-025) ===\n\n");

    test_should_decay_at_100_ticks();
    test_should_decay_custom_interval();
    test_should_decay_zero_interval();
    test_traffic_multiplier_no_traffic();
    test_traffic_multiplier_zero_flow();
    test_traffic_multiplier_half_capacity();
    test_traffic_multiplier_full_capacity();
    test_traffic_multiplier_capped_at_max();
    test_traffic_multiplier_zero_capacity();
    test_apply_decay_basic();
    test_apply_decay_with_traffic();
    test_apply_decay_threshold_crossing();
    test_apply_decay_good_to_worn();
    test_apply_decay_worn_to_poor();
    test_apply_decay_poor_to_crumbling();
    test_apply_decay_clamps_to_zero();
    test_apply_decay_zero_health_no_change();
    test_apply_decay_no_crossing_within_same_state();
    test_apply_decay_large_decay_multiple_crossings();

    printf("\n=== All PathwayDecay Tests Passed ===\n");
    return 0;
}
