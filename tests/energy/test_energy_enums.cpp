/**
 * @file test_energy_enums.cpp
 * @brief Unit tests for EnergyEnums (Epic 5, Ticket 5-001)
 */

#include <sims3000/energy/EnergyEnums.h>
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace sims3000::energy;

void test_nexus_type_enum_values() {
    printf("Testing NexusType enum values...\n");

    assert(static_cast<uint8_t>(NexusType::Carbon) == 0);
    assert(static_cast<uint8_t>(NexusType::Petrochemical) == 1);
    assert(static_cast<uint8_t>(NexusType::Gaseous) == 2);
    assert(static_cast<uint8_t>(NexusType::Nuclear) == 3);
    assert(static_cast<uint8_t>(NexusType::Wind) == 4);
    assert(static_cast<uint8_t>(NexusType::Solar) == 5);
    assert(static_cast<uint8_t>(NexusType::Hydro) == 6);
    assert(static_cast<uint8_t>(NexusType::Geothermal) == 7);
    assert(static_cast<uint8_t>(NexusType::Fusion) == 8);
    assert(static_cast<uint8_t>(NexusType::MicrowaveReceiver) == 9);

    printf("  PASS: NexusType enum values correct\n");
}

void test_nexus_type_counts() {
    printf("Testing NexusType counts...\n");

    assert(NEXUS_TYPE_COUNT == 10);
    assert(NEXUS_TYPE_MVP_COUNT == 6);

    printf("  PASS: NexusType counts correct\n");
}

void test_energy_pool_state_enum() {
    printf("Testing EnergyPoolState enum...\n");

    assert(static_cast<uint8_t>(EnergyPoolState::Healthy) == 0);
    assert(static_cast<uint8_t>(EnergyPoolState::Marginal) == 1);
    assert(static_cast<uint8_t>(EnergyPoolState::Deficit) == 2);
    assert(static_cast<uint8_t>(EnergyPoolState::Collapse) == 3);

    printf("  PASS: EnergyPoolState enum values correct\n");
}

void test_terrain_requirement_enum() {
    printf("Testing TerrainRequirement enum...\n");

    assert(static_cast<uint8_t>(TerrainRequirement::None) == 0);
    assert(static_cast<uint8_t>(TerrainRequirement::Water) == 1);
    assert(static_cast<uint8_t>(TerrainRequirement::EmberCrust) == 2);
    assert(static_cast<uint8_t>(TerrainRequirement::Ridges) == 3);

    printf("  PASS: TerrainRequirement enum values correct\n");
}

void test_nexus_type_to_string() {
    printf("Testing nexus_type_to_string...\n");

    assert(strcmp(nexus_type_to_string(NexusType::Carbon), "Carbon") == 0);
    assert(strcmp(nexus_type_to_string(NexusType::Petrochemical), "Petrochemical") == 0);
    assert(strcmp(nexus_type_to_string(NexusType::Gaseous), "Gaseous") == 0);
    assert(strcmp(nexus_type_to_string(NexusType::Nuclear), "Nuclear") == 0);
    assert(strcmp(nexus_type_to_string(NexusType::Wind), "Wind") == 0);
    assert(strcmp(nexus_type_to_string(NexusType::Solar), "Solar") == 0);
    assert(strcmp(nexus_type_to_string(NexusType::Hydro), "Hydro") == 0);
    assert(strcmp(nexus_type_to_string(NexusType::Geothermal), "Geothermal") == 0);
    assert(strcmp(nexus_type_to_string(NexusType::Fusion), "Fusion") == 0);
    assert(strcmp(nexus_type_to_string(NexusType::MicrowaveReceiver), "MicrowaveReceiver") == 0);

    // Test unknown value
    assert(strcmp(nexus_type_to_string(static_cast<NexusType>(255)), "Unknown") == 0);

    printf("  PASS: nexus_type_to_string works correctly\n");
}

void test_is_mvp_nexus_type() {
    printf("Testing is_mvp_nexus_type...\n");

    // MVP types (first 6)
    assert(is_mvp_nexus_type(NexusType::Carbon));
    assert(is_mvp_nexus_type(NexusType::Petrochemical));
    assert(is_mvp_nexus_type(NexusType::Gaseous));
    assert(is_mvp_nexus_type(NexusType::Nuclear));
    assert(is_mvp_nexus_type(NexusType::Wind));
    assert(is_mvp_nexus_type(NexusType::Solar));

    // Non-MVP types
    assert(!is_mvp_nexus_type(NexusType::Hydro));
    assert(!is_mvp_nexus_type(NexusType::Geothermal));
    assert(!is_mvp_nexus_type(NexusType::Fusion));
    assert(!is_mvp_nexus_type(NexusType::MicrowaveReceiver));

    printf("  PASS: is_mvp_nexus_type works correctly\n");
}

void test_enum_value_ranges() {
    printf("Testing enum value ranges...\n");

    // NexusType range: 0-9
    for (uint8_t i = 0; i < NEXUS_TYPE_COUNT; ++i) {
        NexusType type = static_cast<NexusType>(i);
        // Verify string conversion doesn't return "Unknown" for valid types
        assert(strcmp(nexus_type_to_string(type), "Unknown") != 0);
    }

    // EnergyPoolState range: 0-3
    assert(static_cast<uint8_t>(EnergyPoolState::Healthy) == 0);
    assert(static_cast<uint8_t>(EnergyPoolState::Collapse) == 3);

    // TerrainRequirement range: 0-3
    assert(static_cast<uint8_t>(TerrainRequirement::None) == 0);
    assert(static_cast<uint8_t>(TerrainRequirement::Ridges) == 3);

    // Enum underlying type sizes
    assert(sizeof(NexusType) == 1);
    assert(sizeof(EnergyPoolState) == 1);
    assert(sizeof(TerrainRequirement) == 1);

    printf("  PASS: Enum value ranges correct\n");
}

int main() {
    printf("=== EnergyEnums Unit Tests (Epic 5, Ticket 5-001) ===\n\n");

    test_nexus_type_enum_values();
    test_nexus_type_counts();
    test_energy_pool_state_enum();
    test_terrain_requirement_enum();
    test_nexus_type_to_string();
    test_is_mvp_nexus_type();
    test_enum_value_ranges();

    printf("\n=== All EnergyEnums Tests Passed ===\n");
    return 0;
}
