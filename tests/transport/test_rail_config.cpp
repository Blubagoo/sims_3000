/**
 * @file test_rail_config.cpp
 * @brief Unit tests for RailConfig (Epic 7, Ticket E7-047)
 */

#include <sims3000/transport/RailConfig.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_surface_rail_stats() {
    printf("Testing SurfaceRail stats...\n");

    auto stats = get_rail_stats(RailType::SurfaceRail);
    assert(stats.type == RailType::SurfaceRail);
    assert(stats.capacity == 500);
    assert(stats.build_cost == 200);
    assert(stats.power_required == 50);

    printf("  PASS: SurfaceRail stats are correct\n");
}

void test_elevated_rail_stats() {
    printf("Testing ElevatedRail stats...\n");

    auto stats = get_rail_stats(RailType::ElevatedRail);
    assert(stats.type == RailType::ElevatedRail);
    assert(stats.capacity == 500);
    assert(stats.build_cost == 350);
    assert(stats.power_required == 75);

    printf("  PASS: ElevatedRail stats are correct\n");
}

void test_subterra_rail_stats() {
    printf("Testing SubterraRail stats...\n");

    auto stats = get_rail_stats(RailType::SubterraRail);
    assert(stats.type == RailType::SubterraRail);
    assert(stats.capacity == 750);
    assert(stats.build_cost == 500);
    assert(stats.power_required == 100);

    printf("  PASS: SubterraRail stats are correct\n");
}

void test_default_fallback() {
    printf("Testing default fallback for unknown type...\n");

    auto stats = get_rail_stats(static_cast<RailType>(255));
    assert(stats.type == RailType::SurfaceRail);
    assert(stats.capacity == 500);
    assert(stats.build_cost == 200);
    assert(stats.power_required == 50);

    printf("  PASS: Default fallback returns SurfaceRail stats\n");
}

void test_surface_station_stats() {
    printf("Testing surface station stats...\n");

    auto stats = get_surface_station_stats();
    assert(stats.capacity == 200);
    assert(stats.build_cost == 300);
    assert(stats.power_required == 100);

    printf("  PASS: Surface station stats are correct\n");
}

void test_subterra_station_stats() {
    printf("Testing subterra station stats...\n");

    auto stats = get_subterra_station_stats();
    assert(stats.capacity == 400);
    assert(stats.build_cost == 500);
    assert(stats.power_required == 150);

    printf("  PASS: Subterra station stats are correct\n");
}

void test_rail_cycle_ticks() {
    printf("Testing RAIL_CYCLE_TICKS constant...\n");

    assert(RAIL_CYCLE_TICKS == 100);

    printf("  PASS: RAIL_CYCLE_TICKS is 100\n");
}

void test_subterra_higher_capacity() {
    printf("Testing SubterraRail has highest capacity...\n");

    auto surface = get_rail_stats(RailType::SurfaceRail);
    auto elevated = get_rail_stats(RailType::ElevatedRail);
    auto subterra = get_rail_stats(RailType::SubterraRail);

    assert(subterra.capacity > surface.capacity);
    assert(subterra.capacity > elevated.capacity);

    printf("  PASS: SubterraRail has highest capacity\n");
}

void test_cost_scales_with_complexity() {
    printf("Testing build cost scales with complexity...\n");

    auto surface = get_rail_stats(RailType::SurfaceRail);
    auto elevated = get_rail_stats(RailType::ElevatedRail);
    auto subterra = get_rail_stats(RailType::SubterraRail);

    assert(surface.build_cost < elevated.build_cost);
    assert(elevated.build_cost < subterra.build_cost);

    printf("  PASS: Build cost scales Surface < Elevated < Subterra\n");
}

void test_power_scales_with_complexity() {
    printf("Testing power requirement scales with complexity...\n");

    auto surface = get_rail_stats(RailType::SurfaceRail);
    auto elevated = get_rail_stats(RailType::ElevatedRail);
    auto subterra = get_rail_stats(RailType::SubterraRail);

    assert(surface.power_required < elevated.power_required);
    assert(elevated.power_required < subterra.power_required);

    printf("  PASS: Power requirement scales Surface < Elevated < Subterra\n");
}

void test_subterra_station_more_expensive() {
    printf("Testing subterra station is more expensive than surface...\n");

    auto surface_st = get_surface_station_stats();
    auto subterra_st = get_subterra_station_stats();

    assert(subterra_st.build_cost > surface_st.build_cost);
    assert(subterra_st.capacity > surface_st.capacity);
    assert(subterra_st.power_required > surface_st.power_required);

    printf("  PASS: Subterra station is more expensive and capable\n");
}

int main() {
    printf("=== RailConfig Unit Tests (Epic 7, Ticket E7-047) ===\n\n");

    test_surface_rail_stats();
    test_elevated_rail_stats();
    test_subterra_rail_stats();
    test_default_fallback();
    test_surface_station_stats();
    test_subterra_station_stats();
    test_rail_cycle_ticks();
    test_subterra_higher_capacity();
    test_cost_scales_with_complexity();
    test_power_scales_with_complexity();
    test_subterra_station_more_expensive();

    printf("\n=== All RailConfig Tests Passed ===\n");
    return 0;
}
