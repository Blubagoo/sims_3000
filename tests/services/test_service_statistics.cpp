/**
 * @file test_service_statistics.cpp
 * @brief Unit tests for ServiceStatistics and ServiceStatisticsManager
 *        (Ticket E9-053)
 *
 * Tests cover:
 * - Default values are all 0
 * - Update and retrieval (full struct and individual accessors)
 * - Multiple types and players
 * - Bounds checking (invalid type/player returns defaults)
 */

#include <sims3000/services/ServiceStatistics.h>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000::services;

// ============================================================================
// Default Values Tests
// ============================================================================

void test_default_statistics_struct() {
    printf("Testing ServiceStatistics default construction...\n");

    ServiceStatistics stats;
    assert(stats.building_count == 0);
    assert(stats.average_coverage == 0.0f);
    assert(stats.total_capacity == 0);
    assert(stats.effectiveness == 0.0f);

    printf("  PASS: Default ServiceStatistics has all zero values\n");
}

void test_default_manager_returns_zeros() {
    printf("Testing ServiceStatisticsManager defaults...\n");

    ServiceStatisticsManager mgr;

    // Check all valid type/player combinations return defaults
    for (uint8_t t = 0; t < ServiceStatisticsManager::SERVICE_TYPE_COUNT; ++t) {
        for (uint8_t p = 0; p < ServiceStatisticsManager::MAX_PLAYERS; ++p) {
            ServiceStatistics s = mgr.get(t, p);
            assert(s.building_count == 0);
            assert(s.average_coverage == 0.0f);
            assert(s.total_capacity == 0);
            assert(s.effectiveness == 0.0f);

            assert(mgr.get_building_count(t, p) == 0);
            assert(mgr.get_average_coverage(t, p) == 0.0f);
            assert(mgr.get_total_capacity(t, p) == 0);
            assert(mgr.get_effectiveness(t, p) == 0.0f);
        }
    }

    printf("  PASS: All default manager values are zero\n");
}

// ============================================================================
// Update and Retrieval Tests
// ============================================================================

void test_update_and_get_full_struct() {
    printf("Testing update and get (full struct)...\n");

    ServiceStatisticsManager mgr;

    ServiceStatistics stats;
    stats.building_count = 5;
    stats.average_coverage = 0.75f;
    stats.total_capacity = 1000;
    stats.effectiveness = 0.85f;

    mgr.update(0, 0, stats);

    ServiceStatistics result = mgr.get(0, 0);
    assert(result.building_count == 5);
    assert(result.average_coverage == 0.75f);
    assert(result.total_capacity == 1000);
    assert(result.effectiveness == 0.85f);

    printf("  PASS: Update and full struct retrieval work correctly\n");
}

void test_update_and_get_individual_accessors() {
    printf("Testing update and individual accessors...\n");

    ServiceStatisticsManager mgr;

    ServiceStatistics stats;
    stats.building_count = 3;
    stats.average_coverage = 0.5f;
    stats.total_capacity = 500;
    stats.effectiveness = 0.9f;

    mgr.update(1, 2, stats);

    assert(mgr.get_building_count(1, 2) == 3);
    assert(mgr.get_average_coverage(1, 2) == 0.5f);
    assert(mgr.get_total_capacity(1, 2) == 500);
    assert(mgr.get_effectiveness(1, 2) == 0.9f);

    printf("  PASS: Individual accessor retrieval works correctly\n");
}

void test_update_overwrites_previous() {
    printf("Testing update overwrites previous values...\n");

    ServiceStatisticsManager mgr;

    ServiceStatistics first;
    first.building_count = 10;
    first.average_coverage = 0.3f;
    first.total_capacity = 200;
    first.effectiveness = 0.4f;
    mgr.update(2, 1, first);

    ServiceStatistics second;
    second.building_count = 20;
    second.average_coverage = 0.6f;
    second.total_capacity = 400;
    second.effectiveness = 0.8f;
    mgr.update(2, 1, second);

    ServiceStatistics result = mgr.get(2, 1);
    assert(result.building_count == 20);
    assert(result.average_coverage == 0.6f);
    assert(result.total_capacity == 400);
    assert(result.effectiveness == 0.8f);

    printf("  PASS: Update correctly overwrites previous values\n");
}

// ============================================================================
// Multiple Types and Players Tests
// ============================================================================

void test_multiple_types_independent() {
    printf("Testing multiple service types are independent...\n");

    ServiceStatisticsManager mgr;

    // Set different stats for each type for player 0
    for (uint8_t t = 0; t < ServiceStatisticsManager::SERVICE_TYPE_COUNT; ++t) {
        ServiceStatistics stats;
        stats.building_count = static_cast<uint32_t>(t + 1) * 10;
        stats.average_coverage = static_cast<float>(t + 1) * 0.1f;
        stats.total_capacity = static_cast<uint32_t>(t + 1) * 100;
        stats.effectiveness = static_cast<float>(t + 1) * 0.2f;
        mgr.update(t, 0, stats);
    }

    // Verify each type has its own values
    for (uint8_t t = 0; t < ServiceStatisticsManager::SERVICE_TYPE_COUNT; ++t) {
        uint32_t expected_count = static_cast<uint32_t>(t + 1) * 10;
        float expected_coverage = static_cast<float>(t + 1) * 0.1f;
        uint32_t expected_capacity = static_cast<uint32_t>(t + 1) * 100;
        float expected_eff = static_cast<float>(t + 1) * 0.2f;

        assert(mgr.get_building_count(t, 0) == expected_count);
        assert(std::fabs(mgr.get_average_coverage(t, 0) - expected_coverage) < 0.001f);
        assert(mgr.get_total_capacity(t, 0) == expected_capacity);
        assert(std::fabs(mgr.get_effectiveness(t, 0) - expected_eff) < 0.001f);
    }

    printf("  PASS: Different service types store independent values\n");
}

void test_multiple_players_independent() {
    printf("Testing multiple players are independent...\n");

    ServiceStatisticsManager mgr;

    // Set different stats for each player for type 0
    for (uint8_t p = 0; p < ServiceStatisticsManager::MAX_PLAYERS; ++p) {
        ServiceStatistics stats;
        stats.building_count = static_cast<uint32_t>(p + 1) * 5;
        stats.average_coverage = static_cast<float>(p + 1) * 0.15f;
        stats.total_capacity = static_cast<uint32_t>(p + 1) * 250;
        stats.effectiveness = static_cast<float>(p + 1) * 0.2f;
        mgr.update(0, p, stats);
    }

    // Verify each player has its own values
    for (uint8_t p = 0; p < ServiceStatisticsManager::MAX_PLAYERS; ++p) {
        uint32_t expected_count = static_cast<uint32_t>(p + 1) * 5;
        float expected_coverage = static_cast<float>(p + 1) * 0.15f;
        uint32_t expected_capacity = static_cast<uint32_t>(p + 1) * 250;
        float expected_eff = static_cast<float>(p + 1) * 0.2f;

        assert(mgr.get_building_count(0, p) == expected_count);
        assert(std::fabs(mgr.get_average_coverage(0, p) - expected_coverage) < 0.001f);
        assert(mgr.get_total_capacity(0, p) == expected_capacity);
        assert(std::fabs(mgr.get_effectiveness(0, p) - expected_eff) < 0.001f);
    }

    printf("  PASS: Different players store independent values\n");
}

void test_type_player_cross_independence() {
    printf("Testing type+player cross-independence...\n");

    ServiceStatisticsManager mgr;

    // Set stats for type=1, player=2
    ServiceStatistics stats;
    stats.building_count = 42;
    stats.average_coverage = 0.99f;
    stats.total_capacity = 9999;
    stats.effectiveness = 1.0f;
    mgr.update(1, 2, stats);

    // Verify type=1, player=0 is still default
    assert(mgr.get_building_count(1, 0) == 0);
    assert(mgr.get_average_coverage(1, 0) == 0.0f);

    // Verify type=0, player=2 is still default
    assert(mgr.get_building_count(0, 2) == 0);
    assert(mgr.get_average_coverage(0, 2) == 0.0f);

    // Verify type=1, player=2 has the set values
    assert(mgr.get_building_count(1, 2) == 42);
    assert(mgr.get_average_coverage(1, 2) == 0.99f);
    assert(mgr.get_total_capacity(1, 2) == 9999);
    assert(mgr.get_effectiveness(1, 2) == 1.0f);

    printf("  PASS: Type+player combinations are fully independent\n");
}

// ============================================================================
// Bounds / Invalid Input Tests
// ============================================================================

void test_invalid_service_type_get() {
    printf("Testing invalid service type on get...\n");

    ServiceStatisticsManager mgr;

    // Set valid data first
    ServiceStatistics stats;
    stats.building_count = 10;
    mgr.update(0, 0, stats);

    // Invalid type indices should return defaults
    ServiceStatistics result = mgr.get(4, 0);
    assert(result.building_count == 0);
    assert(result.average_coverage == 0.0f);
    assert(result.total_capacity == 0);
    assert(result.effectiveness == 0.0f);

    result = mgr.get(255, 0);
    assert(result.building_count == 0);

    printf("  PASS: Invalid service type returns defaults on get\n");
}

void test_invalid_player_id_get() {
    printf("Testing invalid player ID on get...\n");

    ServiceStatisticsManager mgr;

    ServiceStatistics stats;
    stats.building_count = 10;
    mgr.update(0, 0, stats);

    // Invalid player indices should return defaults
    ServiceStatistics result = mgr.get(0, 4);
    assert(result.building_count == 0);

    result = mgr.get(0, 255);
    assert(result.building_count == 0);

    printf("  PASS: Invalid player ID returns defaults on get\n");
}

void test_invalid_type_individual_accessors() {
    printf("Testing invalid type on individual accessors...\n");

    ServiceStatisticsManager mgr;

    assert(mgr.get_building_count(4, 0) == 0);
    assert(mgr.get_average_coverage(4, 0) == 0.0f);
    assert(mgr.get_total_capacity(4, 0) == 0);
    assert(mgr.get_effectiveness(4, 0) == 0.0f);

    assert(mgr.get_building_count(255, 0) == 0);
    assert(mgr.get_average_coverage(255, 0) == 0.0f);
    assert(mgr.get_total_capacity(255, 0) == 0);
    assert(mgr.get_effectiveness(255, 0) == 0.0f);

    printf("  PASS: Invalid type returns defaults on individual accessors\n");
}

void test_invalid_player_individual_accessors() {
    printf("Testing invalid player on individual accessors...\n");

    ServiceStatisticsManager mgr;

    assert(mgr.get_building_count(0, 4) == 0);
    assert(mgr.get_average_coverage(0, 4) == 0.0f);
    assert(mgr.get_total_capacity(0, 4) == 0);
    assert(mgr.get_effectiveness(0, 4) == 0.0f);

    assert(mgr.get_building_count(0, 255) == 0);
    assert(mgr.get_average_coverage(0, 255) == 0.0f);
    assert(mgr.get_total_capacity(0, 255) == 0);
    assert(mgr.get_effectiveness(0, 255) == 0.0f);

    printf("  PASS: Invalid player returns defaults on individual accessors\n");
}

void test_invalid_update_ignored() {
    printf("Testing invalid update is silently ignored...\n");

    ServiceStatisticsManager mgr;

    ServiceStatistics stats;
    stats.building_count = 999;
    stats.average_coverage = 1.0f;
    stats.total_capacity = 9999;
    stats.effectiveness = 1.0f;

    // These updates should be silently ignored (no crash)
    mgr.update(4, 0, stats);
    mgr.update(255, 0, stats);
    mgr.update(0, 4, stats);
    mgr.update(0, 255, stats);
    mgr.update(255, 255, stats);

    // Verify no valid slots were affected
    for (uint8_t t = 0; t < ServiceStatisticsManager::SERVICE_TYPE_COUNT; ++t) {
        for (uint8_t p = 0; p < ServiceStatisticsManager::MAX_PLAYERS; ++p) {
            assert(mgr.get_building_count(t, p) == 0);
        }
    }

    printf("  PASS: Invalid update calls are silently ignored\n");
}

// ============================================================================
// Constants Tests
// ============================================================================

void test_manager_constants() {
    printf("Testing ServiceStatisticsManager constants...\n");

    assert(ServiceStatisticsManager::MAX_PLAYERS == 4);
    assert(ServiceStatisticsManager::SERVICE_TYPE_COUNT == 4);

    printf("  PASS: Manager constants are correct\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== ServiceStatistics Unit Tests (Ticket E9-053) ===\n\n");

    // Default values
    test_default_statistics_struct();
    test_default_manager_returns_zeros();

    // Update and retrieval
    test_update_and_get_full_struct();
    test_update_and_get_individual_accessors();
    test_update_overwrites_previous();

    // Multiple types and players
    test_multiple_types_independent();
    test_multiple_players_independent();
    test_type_player_cross_independence();

    // Bounds / invalid input
    test_invalid_service_type_get();
    test_invalid_player_id_get();
    test_invalid_type_individual_accessors();
    test_invalid_player_individual_accessors();
    test_invalid_update_ignored();

    // Constants
    test_manager_constants();

    printf("\n=== All ServiceStatistics Tests Passed ===\n");
    return 0;
}
