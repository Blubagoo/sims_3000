/**
 * @file test_building_types.cpp
 * @brief Unit tests for BuildingTypes (Epic 4, Ticket 4-002)
 */

#include <sims3000/building/BuildingTypes.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::building;

void test_building_state_enum() {
    printf("Testing BuildingState enum...\n");

    assert(static_cast<std::uint8_t>(BuildingState::Materializing) == 0);
    assert(static_cast<std::uint8_t>(BuildingState::Active) == 1);
    assert(static_cast<std::uint8_t>(BuildingState::Abandoned) == 2);
    assert(static_cast<std::uint8_t>(BuildingState::Derelict) == 3);
    assert(static_cast<std::uint8_t>(BuildingState::Deconstructed) == 4);
    assert(BUILDING_STATE_COUNT == 5);

    assert(isValidBuildingState(0));
    assert(isValidBuildingState(4));
    assert(!isValidBuildingState(5));

    printf("  PASS: BuildingState enum values correct\n");
}

void test_zone_building_type_enum() {
    printf("Testing ZoneBuildingType enum...\n");

    assert(static_cast<std::uint8_t>(ZoneBuildingType::Habitation) == 0);
    assert(static_cast<std::uint8_t>(ZoneBuildingType::Exchange) == 1);
    assert(static_cast<std::uint8_t>(ZoneBuildingType::Fabrication) == 2);
    assert(ZONE_BUILDING_TYPE_COUNT == 3);

    assert(isValidZoneBuildingType(0));
    assert(isValidZoneBuildingType(2));
    assert(!isValidZoneBuildingType(3));

    printf("  PASS: ZoneBuildingType enum values correct\n");
}

void test_density_level_enum() {
    printf("Testing DensityLevel enum...\n");

    assert(static_cast<std::uint8_t>(DensityLevel::Low) == 0);
    assert(static_cast<std::uint8_t>(DensityLevel::High) == 1);
    assert(DENSITY_LEVEL_COUNT == 2);

    assert(isValidDensityLevel(0));
    assert(isValidDensityLevel(1));
    assert(!isValidDensityLevel(2));

    printf("  PASS: DensityLevel enum values correct\n");
}

void test_construction_phase_enum() {
    printf("Testing ConstructionPhase enum...\n");

    assert(static_cast<std::uint8_t>(ConstructionPhase::Foundation) == 0);
    assert(static_cast<std::uint8_t>(ConstructionPhase::Framework) == 1);
    assert(static_cast<std::uint8_t>(ConstructionPhase::Exterior) == 2);
    assert(static_cast<std::uint8_t>(ConstructionPhase::Finalization) == 3);
    assert(CONSTRUCTION_PHASE_COUNT == 4);

    assert(isValidConstructionPhase(0));
    assert(isValidConstructionPhase(3));
    assert(!isValidConstructionPhase(4));

    printf("  PASS: ConstructionPhase enum values correct\n");
}

void test_phase_calculation() {
    printf("Testing phase calculation...\n");

    // Foundation: [0, 25)
    assert(getPhaseFromProgress(0) == ConstructionPhase::Foundation);
    assert(getPhaseFromProgress(24) == ConstructionPhase::Foundation);

    // Framework: [25, 50)
    assert(getPhaseFromProgress(25) == ConstructionPhase::Framework);
    assert(getPhaseFromProgress(49) == ConstructionPhase::Framework);

    // Exterior: [50, 75)
    assert(getPhaseFromProgress(50) == ConstructionPhase::Exterior);
    assert(getPhaseFromProgress(74) == ConstructionPhase::Exterior);

    // Finalization: [75, 100]
    assert(getPhaseFromProgress(75) == ConstructionPhase::Finalization);
    assert(getPhaseFromProgress(100) == ConstructionPhase::Finalization);

    printf("  PASS: Phase calculation works correctly\n");
}

void test_progress_calculation() {
    printf("Testing progress calculation...\n");

    assert(getProgressPercent(0, 100) == 0);
    assert(getProgressPercent(25, 100) == 25);
    assert(getProgressPercent(50, 100) == 50);
    assert(getProgressPercent(100, 100) == 100);

    // Different durations
    assert(getProgressPercent(50, 200) == 25);
    assert(getProgressPercent(100, 200) == 50);

    // Edge case: zero total
    assert(getProgressPercent(0, 0) == 100);

    // Edge case: elapsed > total
    assert(getProgressPercent(150, 100) == 100);

    printf("  PASS: Progress calculation works correctly\n");
}

int main() {
    printf("=== BuildingTypes Unit Tests (Epic 4, Ticket 4-002) ===\n\n");

    test_building_state_enum();
    test_zone_building_type_enum();
    test_density_level_enum();
    test_construction_phase_enum();
    test_phase_calculation();
    test_progress_calculation();

    printf("\n=== All BuildingTypes Tests Passed ===\n");
    return 0;
}
