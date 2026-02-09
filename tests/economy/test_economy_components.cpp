/**
 * @file test_economy_components.cpp
 * @brief Unit tests for EconomyComponents (E11-001)
 */

#include "sims3000/economy/EconomyComponents.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <type_traits>

using namespace sims3000::economy;
using sims3000::building::ZoneBuildingType;

void test_tributable_trivially_copyable() {
    printf("Testing TributableComponent is trivially copyable...\n");

    static_assert(std::is_trivially_copyable<TributableComponent>::value,
                  "TributableComponent must be trivially copyable");

    printf("  PASS: TributableComponent is trivially copyable\n");
}

void test_tributable_size() {
    printf("Testing TributableComponent size...\n");

    // Should be approximately 12 bytes (with possible padding up to 16)
    assert(sizeof(TributableComponent) <= 16);

    printf("  PASS: TributableComponent size is %zu bytes (<= 16)\n",
           sizeof(TributableComponent));
}

void test_tributable_defaults() {
    printf("Testing TributableComponent default values...\n");

    TributableComponent tc;

    assert(tc.zone_type == ZoneBuildingType::Habitation);
    assert(tc.base_value == 100);
    assert(tc.density_level == 0);
    assert(std::fabs(tc.tribute_modifier - 1.0f) < 0.001f);

    printf("  PASS: TributableComponent defaults are correct\n");
}

void test_tributable_zone_types() {
    printf("Testing TributableComponent zone type values...\n");

    TributableComponent hab;
    hab.zone_type = ZoneBuildingType::Habitation;
    assert(static_cast<uint8_t>(hab.zone_type) == 0);

    TributableComponent exc;
    exc.zone_type = ZoneBuildingType::Exchange;
    assert(static_cast<uint8_t>(exc.zone_type) == 1);

    TributableComponent fab;
    fab.zone_type = ZoneBuildingType::Fabrication;
    assert(static_cast<uint8_t>(fab.zone_type) == 2);

    printf("  PASS: ZoneBuildingType enum values match expected\n");
}

void test_maintenance_trivially_copyable() {
    printf("Testing MaintenanceCostComponent is trivially copyable...\n");

    static_assert(std::is_trivially_copyable<MaintenanceCostComponent>::value,
                  "MaintenanceCostComponent must be trivially copyable");

    printf("  PASS: MaintenanceCostComponent is trivially copyable\n");
}

void test_maintenance_size() {
    printf("Testing MaintenanceCostComponent size...\n");

    assert(sizeof(MaintenanceCostComponent) == 8);

    printf("  PASS: MaintenanceCostComponent size is %zu bytes (== 8)\n",
           sizeof(MaintenanceCostComponent));
}

void test_maintenance_defaults() {
    printf("Testing MaintenanceCostComponent default values...\n");

    MaintenanceCostComponent mc;

    assert(mc.base_cost == 0);
    assert(std::fabs(mc.cost_multiplier - 1.0f) < 0.001f);

    printf("  PASS: MaintenanceCostComponent defaults are correct\n");
}

void test_zone_building_type_enum() {
    printf("Testing ZoneBuildingType enum values...\n");

    // Verify the three canonical zone building types exist and have correct values
    assert(static_cast<uint8_t>(ZoneBuildingType::Habitation) == 0);
    assert(static_cast<uint8_t>(ZoneBuildingType::Exchange) == 1);
    assert(static_cast<uint8_t>(ZoneBuildingType::Fabrication) == 2);

    printf("  PASS: ZoneBuildingType enum values are correct\n");
}

int main() {
    printf("=== Economy Components Unit Tests (E11-001) ===\n\n");

    test_tributable_trivially_copyable();
    test_tributable_size();
    test_tributable_defaults();
    test_tributable_zone_types();
    test_maintenance_trivially_copyable();
    test_maintenance_size();
    test_maintenance_defaults();
    test_zone_building_type_enum();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
