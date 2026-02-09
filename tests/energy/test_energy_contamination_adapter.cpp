/**
 * @file test_energy_contamination_adapter.cpp
 * @brief Tests for EnergyContaminationAdapter (E10-114)
 */

#include <sims3000/energy/EnergyContaminationAdapter.h>
#include <cstdio>
#include <cassert>

using namespace sims3000;
using namespace sims3000::energy;
using namespace sims3000::contamination;

void test_empty_adapter() {
    printf("[test_empty_adapter]\n");
    EnergyContaminationAdapter adapter;

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    assert(entries.empty());
    printf("  PASS: empty adapter produces no entries\n");
}

void test_single_carbon_nexus() {
    printf("[test_single_carbon_nexus]\n");
    EnergyContaminationAdapter adapter;

    EnergyNexusInfo nexus;
    nexus.x = 10;
    nexus.y = 20;
    nexus.nexus_type = 0; // Carbon
    nexus.is_active = true;

    adapter.set_nexuses({nexus});

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    assert(entries.size() == 1);
    assert(entries[0].x == 10);
    assert(entries[0].y == 20);
    assert(entries[0].output == CARBON_OUTPUT);
    assert(entries[0].type == ContaminationType::Energy);

    printf("  PASS: carbon nexus produces output=%u\n", CARBON_OUTPUT);
}

void test_single_petrochem_nexus() {
    printf("[test_single_petrochem_nexus]\n");
    EnergyContaminationAdapter adapter;

    EnergyNexusInfo nexus;
    nexus.x = 5;
    nexus.y = 15;
    nexus.nexus_type = 1; // Petrochem
    nexus.is_active = true;

    adapter.set_nexuses({nexus});

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    assert(entries.size() == 1);
    assert(entries[0].x == 5);
    assert(entries[0].y == 15);
    assert(entries[0].output == PETROCHEM_OUTPUT);
    assert(entries[0].type == ContaminationType::Energy);

    printf("  PASS: petrochem nexus produces output=%u\n", PETROCHEM_OUTPUT);
}

void test_single_gaseous_nexus() {
    printf("[test_single_gaseous_nexus]\n");
    EnergyContaminationAdapter adapter;

    EnergyNexusInfo nexus;
    nexus.x = 8;
    nexus.y = 12;
    nexus.nexus_type = 2; // Gaseous
    nexus.is_active = true;

    adapter.set_nexuses({nexus});

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    assert(entries.size() == 1);
    assert(entries[0].x == 8);
    assert(entries[0].y == 12);
    assert(entries[0].output == GASEOUS_OUTPUT);
    assert(entries[0].type == ContaminationType::Energy);

    printf("  PASS: gaseous nexus produces output=%u\n", GASEOUS_OUTPUT);
}

void test_clean_energy_produces_no_contamination() {
    printf("[test_clean_energy_produces_no_contamination]\n");
    EnergyContaminationAdapter adapter;

    std::vector<EnergyNexusInfo> nexuses;
    for (uint8_t type = 3; type <= 10; ++type) {
        EnergyNexusInfo nexus;
        nexus.x = type * 10;
        nexus.y = type * 10;
        nexus.nexus_type = type; // Clean energy types (>= 3)
        nexus.is_active = true;
        nexuses.push_back(nexus);
    }

    adapter.set_nexuses(nexuses);

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    assert(entries.empty());
    printf("  PASS: clean energy types (>=3) produce no contamination\n");
}

void test_inactive_nexuses_produce_no_contamination() {
    printf("[test_inactive_nexuses_produce_no_contamination]\n");
    EnergyContaminationAdapter adapter;

    std::vector<EnergyNexusInfo> nexuses;
    for (uint8_t type = 0; type < 3; ++type) {
        EnergyNexusInfo nexus;
        nexus.x = type * 10;
        nexus.y = type * 10;
        nexus.nexus_type = type;
        nexus.is_active = false; // Inactive
        nexuses.push_back(nexus);
    }

    adapter.set_nexuses(nexuses);

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    assert(entries.empty());
    printf("  PASS: inactive nexuses produce no contamination\n");
}

void test_mixed_nexuses() {
    printf("[test_mixed_nexuses]\n");
    EnergyContaminationAdapter adapter;

    std::vector<EnergyNexusInfo> nexuses;

    // Active carbon
    nexuses.push_back({10, 10, 0, true});
    // Inactive carbon
    nexuses.push_back({20, 20, 0, false});
    // Active petrochem
    nexuses.push_back({30, 30, 1, true});
    // Active gaseous
    nexuses.push_back({40, 40, 2, true});
    // Active clean
    nexuses.push_back({50, 50, 3, true});
    // Inactive clean
    nexuses.push_back({60, 60, 4, false});

    adapter.set_nexuses(nexuses);

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    // Only 3 active contaminating nexuses (carbon, petrochem, gaseous)
    assert(entries.size() == 3);

    // Verify each entry
    assert(entries[0].x == 10 && entries[0].y == 10 && entries[0].output == CARBON_OUTPUT);
    assert(entries[1].x == 30 && entries[1].y == 30 && entries[1].output == PETROCHEM_OUTPUT);
    assert(entries[2].x == 40 && entries[2].y == 40 && entries[2].output == GASEOUS_OUTPUT);

    printf("  PASS: mixed nexuses correctly filtered (3 active contaminating)\n");
}

void test_clear() {
    printf("[test_clear]\n");
    EnergyContaminationAdapter adapter;

    EnergyNexusInfo nexus;
    nexus.x = 10;
    nexus.y = 20;
    nexus.nexus_type = 0;
    nexus.is_active = true;

    adapter.set_nexuses({nexus});

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);
    assert(entries.size() == 1);

    // Clear and verify
    adapter.clear();
    entries.clear();
    adapter.get_contamination_sources(entries);
    assert(entries.empty());

    printf("  PASS: clear removes all nexuses\n");
}

int main() {
    printf("=== EnergyContaminationAdapter Tests ===\n\n");

    test_empty_adapter();
    test_single_carbon_nexus();
    test_single_petrochem_nexus();
    test_single_gaseous_nexus();
    test_clean_energy_produces_no_contamination();
    test_inactive_nexuses_produce_no_contamination();
    test_mixed_nexuses();
    test_clear();

    printf("\n=== All EnergyContaminationAdapter tests passed! ===\n");
    return 0;
}
