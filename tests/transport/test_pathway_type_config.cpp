/**
 * @file test_pathway_type_config.cpp
 * @brief Unit tests for PathwayTypeConfig (Epic 7, Ticket E7-040)
 */

#include <sims3000/transport/PathwayTypeConfig.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_basic_pathway_stats() {
    printf("Testing BasicPathway stats...\n");

    auto stats = get_pathway_stats(PathwayType::BasicPathway);
    assert(stats.type == PathwayType::BasicPathway);
    assert(stats.base_capacity == 100);
    assert(stats.build_cost == 50);
    assert(stats.decay_rate_mult == 100);
    assert(stats.maintenance_cost == 2);

    printf("  PASS: BasicPathway stats are correct\n");
}

void test_transit_corridor_stats() {
    printf("Testing TransitCorridor stats...\n");

    auto stats = get_pathway_stats(PathwayType::TransitCorridor);
    assert(stats.type == PathwayType::TransitCorridor);
    assert(stats.base_capacity == 500);
    assert(stats.build_cost == 200);
    assert(stats.decay_rate_mult == 50);
    assert(stats.maintenance_cost == 8);

    printf("  PASS: TransitCorridor stats are correct\n");
}

void test_pedestrian_stats() {
    printf("Testing Pedestrian stats...\n");

    auto stats = get_pathway_stats(PathwayType::Pedestrian);
    assert(stats.type == PathwayType::Pedestrian);
    assert(stats.base_capacity == 50);
    assert(stats.build_cost == 25);
    assert(stats.decay_rate_mult == 100);
    assert(stats.maintenance_cost == 1);

    printf("  PASS: Pedestrian stats are correct\n");
}

void test_bridge_stats() {
    printf("Testing Bridge stats...\n");

    auto stats = get_pathway_stats(PathwayType::Bridge);
    assert(stats.type == PathwayType::Bridge);
    assert(stats.base_capacity == 200);
    assert(stats.build_cost == 300);
    assert(stats.decay_rate_mult == 75);
    assert(stats.maintenance_cost == 6);

    printf("  PASS: Bridge stats are correct\n");
}

void test_tunnel_stats() {
    printf("Testing Tunnel stats...\n");

    auto stats = get_pathway_stats(PathwayType::Tunnel);
    assert(stats.type == PathwayType::Tunnel);
    assert(stats.base_capacity == 200);
    assert(stats.build_cost == 400);
    assert(stats.decay_rate_mult == 60);
    assert(stats.maintenance_cost == 8);

    printf("  PASS: Tunnel stats are correct\n");
}

void test_default_fallback() {
    printf("Testing default fallback for unknown type...\n");

    auto stats = get_pathway_stats(static_cast<PathwayType>(255));
    assert(stats.type == PathwayType::BasicPathway);
    assert(stats.base_capacity == 100);
    assert(stats.build_cost == 50);
    assert(stats.decay_rate_mult == 100);
    assert(stats.maintenance_cost == 2);

    printf("  PASS: Default fallback returns BasicPathway stats\n");
}

void test_all_types_return_correct_type() {
    printf("Testing all types return matching type field...\n");

    assert(get_pathway_stats(PathwayType::BasicPathway).type == PathwayType::BasicPathway);
    assert(get_pathway_stats(PathwayType::TransitCorridor).type == PathwayType::TransitCorridor);
    assert(get_pathway_stats(PathwayType::Pedestrian).type == PathwayType::Pedestrian);
    assert(get_pathway_stats(PathwayType::Bridge).type == PathwayType::Bridge);
    assert(get_pathway_stats(PathwayType::Tunnel).type == PathwayType::Tunnel);

    printf("  PASS: All types return matching type field\n");
}

void test_capacity_ordering() {
    printf("Testing capacity ordering...\n");

    auto ped = get_pathway_stats(PathwayType::Pedestrian);
    auto basic = get_pathway_stats(PathwayType::BasicPathway);
    auto bridge = get_pathway_stats(PathwayType::Bridge);
    auto corridor = get_pathway_stats(PathwayType::TransitCorridor);

    assert(ped.base_capacity < basic.base_capacity);
    assert(basic.base_capacity < bridge.base_capacity);
    assert(bridge.base_capacity < corridor.base_capacity);

    printf("  PASS: Capacity ordering is Pedestrian < BasicPathway < Bridge < TransitCorridor\n");
}

int main() {
    printf("=== PathwayTypeConfig Unit Tests (Epic 7, Ticket E7-040) ===\n\n");

    test_basic_pathway_stats();
    test_transit_corridor_stats();
    test_pedestrian_stats();
    test_bridge_stats();
    test_tunnel_stats();
    test_default_fallback();
    test_all_types_return_correct_type();
    test_capacity_ordering();

    printf("\n=== All PathwayTypeConfig Tests Passed ===\n");
    return 0;
}
