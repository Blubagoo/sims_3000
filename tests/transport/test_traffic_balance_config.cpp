/**
 * @file test_traffic_balance_config.cpp
 * @brief Unit tests for TrafficBalanceConfig (Epic 7, Ticket E7-048)
 */

#include <sims3000/transport/TrafficBalanceConfig.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_default_flow_values() {
    printf("Testing default flow values...\n");

    TrafficBalanceConfig cfg{};
    assert(cfg.habitation_flow == 2);
    assert(cfg.exchange_flow == 5);
    assert(cfg.fabrication_flow == 3);

    printf("  PASS: Default flow values are correct\n");
}

void test_default_level_multiplier() {
    printf("Testing default level multiplier...\n");

    TrafficBalanceConfig cfg{};
    assert(cfg.level_multiplier == 1);

    printf("  PASS: Default level multiplier is 1\n");
}

void test_default_congestion_thresholds() {
    printf("Testing default congestion thresholds...\n");

    TrafficBalanceConfig cfg{};
    assert(cfg.free_flow_max == 50);
    assert(cfg.light_max == 100);
    assert(cfg.moderate_max == 150);
    assert(cfg.heavy_max == 200);

    printf("  PASS: Default congestion thresholds are correct\n");
}

void test_congestion_thresholds_ordering() {
    printf("Testing congestion thresholds are in order...\n");

    TrafficBalanceConfig cfg{};
    assert(cfg.free_flow_max < cfg.light_max);
    assert(cfg.light_max < cfg.moderate_max);
    assert(cfg.moderate_max < cfg.heavy_max);
    assert(cfg.heavy_max < 255);

    printf("  PASS: Congestion thresholds are in ascending order\n");
}

void test_default_penalties() {
    printf("Testing default penalty values...\n");

    TrafficBalanceConfig cfg{};
    assert(cfg.light_penalty_pct == 5);
    assert(cfg.moderate_penalty_pct == 10);
    assert(cfg.heavy_penalty_pct == 15);

    printf("  PASS: Default penalty values are correct\n");
}

void test_penalties_increase_with_congestion() {
    printf("Testing penalties increase with congestion level...\n");

    TrafficBalanceConfig cfg{};
    assert(cfg.light_penalty_pct < cfg.moderate_penalty_pct);
    assert(cfg.moderate_penalty_pct < cfg.heavy_penalty_pct);

    printf("  PASS: Penalties increase with congestion\n");
}

void test_exchange_highest_flow() {
    printf("Testing exchange has highest base flow...\n");

    TrafficBalanceConfig cfg{};
    assert(cfg.exchange_flow > cfg.habitation_flow);
    assert(cfg.exchange_flow > cfg.fabrication_flow);

    printf("  PASS: Exchange has highest base flow\n");
}

void test_custom_values() {
    printf("Testing custom value assignment...\n");

    TrafficBalanceConfig cfg{};
    cfg.habitation_flow = 10;
    cfg.exchange_flow = 20;
    cfg.fabrication_flow = 15;
    cfg.level_multiplier = 3;
    cfg.free_flow_max = 30;
    cfg.light_max = 80;
    cfg.moderate_max = 130;
    cfg.heavy_max = 180;
    cfg.light_penalty_pct = 10;
    cfg.moderate_penalty_pct = 20;
    cfg.heavy_penalty_pct = 30;

    assert(cfg.habitation_flow == 10);
    assert(cfg.exchange_flow == 20);
    assert(cfg.fabrication_flow == 15);
    assert(cfg.level_multiplier == 3);
    assert(cfg.free_flow_max == 30);
    assert(cfg.light_max == 80);
    assert(cfg.moderate_max == 130);
    assert(cfg.heavy_max == 180);
    assert(cfg.light_penalty_pct == 10);
    assert(cfg.moderate_penalty_pct == 20);
    assert(cfg.heavy_penalty_pct == 30);

    printf("  PASS: Custom values work correctly\n");
}

void test_flow_ordering() {
    printf("Testing flow ordering: habitation < fabrication < exchange...\n");

    TrafficBalanceConfig cfg{};
    assert(cfg.habitation_flow < cfg.fabrication_flow);
    assert(cfg.fabrication_flow < cfg.exchange_flow);

    printf("  PASS: Flow ordering is correct\n");
}

int main() {
    printf("=== TrafficBalanceConfig Unit Tests (Epic 7, Ticket E7-048) ===\n\n");

    test_default_flow_values();
    test_default_level_multiplier();
    test_default_congestion_thresholds();
    test_congestion_thresholds_ordering();
    test_default_penalties();
    test_penalties_increase_with_congestion();
    test_exchange_highest_flow();
    test_custom_values();
    test_flow_ordering();

    printf("\n=== All TrafficBalanceConfig Tests Passed ===\n");
    return 0;
}
