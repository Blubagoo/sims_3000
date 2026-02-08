/**
 * @file test_energy_producer_component.cpp
 * @brief Unit tests for EnergyProducerComponent (Epic 5, Ticket 5-003)
 *
 * Tests cover:
 * - Size verification (24 bytes)
 * - Trivially copyable for serialization
 * - Default initialization values
 * - Output calculation: current_output = base_output * efficiency * age_factor
 * - Offline behavior: current_output should be 0 when !is_online
 */

#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace sims3000::energy;

void test_producer_component_size() {
    printf("Testing EnergyProducerComponent size...\n");

    assert(sizeof(EnergyProducerComponent) == 24);

    printf("  PASS: EnergyProducerComponent is 24 bytes\n");
}

void test_producer_trivially_copyable() {
    printf("Testing EnergyProducerComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<EnergyProducerComponent>::value);

    printf("  PASS: EnergyProducerComponent is trivially copyable\n");
}

void test_producer_default_initialization() {
    printf("Testing default initialization...\n");

    EnergyProducerComponent epc{};
    assert(epc.base_output == 0);
    assert(epc.current_output == 0);
    assert(epc.efficiency == 1.0f);
    assert(epc.age_factor == 1.0f);
    assert(epc.ticks_since_built == 0);
    assert(epc.nexus_type == 0);
    assert(epc.is_online == true);
    assert(epc.contamination_output == 0);

    printf("  PASS: Default initialization works correctly\n");
}

void test_producer_nexus_type_values() {
    printf("Testing nexus type assignment...\n");

    EnergyProducerComponent epc{};

    // Carbon nexus
    epc.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    assert(epc.nexus_type == 0);

    // Solar nexus
    epc.nexus_type = static_cast<uint8_t>(NexusType::Solar);
    assert(epc.nexus_type == 5);

    // Nuclear nexus
    epc.nexus_type = static_cast<uint8_t>(NexusType::Nuclear);
    assert(epc.nexus_type == 3);

    // Round-trip through NexusType
    NexusType type = static_cast<NexusType>(epc.nexus_type);
    assert(type == NexusType::Nuclear);

    printf("  PASS: Nexus type assignment works correctly\n");
}

void test_producer_output_calculation_full_efficiency() {
    printf("Testing output calculation at full efficiency...\n");

    EnergyProducerComponent epc{};
    epc.base_output = 1000;
    epc.efficiency = 1.0f;
    epc.age_factor = 1.0f;
    epc.is_online = true;

    // Simulate: current_output = base_output * efficiency * age_factor
    uint32_t calculated = static_cast<uint32_t>(
        static_cast<float>(epc.base_output) * epc.efficiency * epc.age_factor);
    epc.current_output = calculated;

    assert(epc.current_output == 1000);

    printf("  PASS: Full efficiency output is correct\n");
}

void test_producer_output_calculation_reduced_efficiency() {
    printf("Testing output calculation at reduced efficiency...\n");

    EnergyProducerComponent epc{};
    epc.base_output = 1000;
    epc.efficiency = 0.75f;
    epc.age_factor = 1.0f;
    epc.is_online = true;

    uint32_t calculated = static_cast<uint32_t>(
        static_cast<float>(epc.base_output) * epc.efficiency * epc.age_factor);
    epc.current_output = calculated;

    assert(epc.current_output == 750);

    printf("  PASS: Reduced efficiency output is correct\n");
}

void test_producer_output_calculation_with_aging() {
    printf("Testing output calculation with aging degradation...\n");

    EnergyProducerComponent epc{};
    epc.base_output = 1000;
    epc.efficiency = 1.0f;
    epc.age_factor = 0.8f;
    epc.is_online = true;

    uint32_t calculated = static_cast<uint32_t>(
        static_cast<float>(epc.base_output) * epc.efficiency * epc.age_factor);
    epc.current_output = calculated;

    assert(epc.current_output == 800);

    printf("  PASS: Aged output is correct\n");
}

void test_producer_output_calculation_combined() {
    printf("Testing output calculation with both efficiency and aging...\n");

    EnergyProducerComponent epc{};
    epc.base_output = 1000;
    epc.efficiency = 0.5f;
    epc.age_factor = 0.8f;
    epc.is_online = true;

    uint32_t calculated = static_cast<uint32_t>(
        static_cast<float>(epc.base_output) * epc.efficiency * epc.age_factor);
    epc.current_output = calculated;

    assert(epc.current_output == 400);

    printf("  PASS: Combined efficiency + aging output is correct\n");
}

void test_producer_offline_output() {
    printf("Testing offline nexus produces zero output...\n");

    EnergyProducerComponent epc{};
    epc.base_output = 1000;
    epc.efficiency = 1.0f;
    epc.age_factor = 1.0f;
    epc.is_online = false;

    // Simulate: offline nexus should produce 0
    uint32_t calculated = 0;
    if (epc.is_online) {
        calculated = static_cast<uint32_t>(
            static_cast<float>(epc.base_output) * epc.efficiency * epc.age_factor);
    }
    epc.current_output = calculated;

    assert(epc.current_output == 0);

    printf("  PASS: Offline nexus produces zero output\n");
}

void test_producer_contamination() {
    printf("Testing contamination output...\n");

    EnergyProducerComponent epc{};
    epc.contamination_output = 0;
    assert(epc.contamination_output == 0);

    // Carbon nexus produces contamination
    epc.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    epc.contamination_output = 50;
    assert(epc.contamination_output == 50);

    // Solar nexus produces no contamination
    epc.nexus_type = static_cast<uint8_t>(NexusType::Solar);
    epc.contamination_output = 0;
    assert(epc.contamination_output == 0);

    printf("  PASS: Contamination output works correctly\n");
}

void test_producer_ticks_since_built() {
    printf("Testing ticks_since_built tracking...\n");

    EnergyProducerComponent epc{};
    assert(epc.ticks_since_built == 0);

    // Simulate aging
    epc.ticks_since_built = 1000;
    assert(epc.ticks_since_built == 1000);

    // Max value (uint16_t cap)
    epc.ticks_since_built = 65535;
    assert(epc.ticks_since_built == 65535);

    printf("  PASS: ticks_since_built tracking works correctly\n");
}

void test_producer_copy() {
    printf("Testing copy semantics...\n");

    EnergyProducerComponent original{};
    original.base_output = 500;
    original.current_output = 375;
    original.efficiency = 0.75f;
    original.age_factor = 0.9f;
    original.ticks_since_built = 200;
    original.nexus_type = static_cast<uint8_t>(NexusType::Wind);
    original.is_online = true;
    original.contamination_output = 0;

    EnergyProducerComponent copy = original;
    assert(copy.base_output == 500);
    assert(copy.current_output == 375);
    assert(copy.efficiency == 0.75f);
    assert(copy.age_factor == 0.9f);
    assert(copy.ticks_since_built == 200);
    assert(copy.nexus_type == static_cast<uint8_t>(NexusType::Wind));
    assert(copy.is_online == true);
    assert(copy.contamination_output == 0);

    printf("  PASS: Copy semantics work correctly\n");
}

int main() {
    printf("=== EnergyProducerComponent Unit Tests (Epic 5, Ticket 5-003) ===\n\n");

    test_producer_component_size();
    test_producer_trivially_copyable();
    test_producer_default_initialization();
    test_producer_nexus_type_values();
    test_producer_output_calculation_full_efficiency();
    test_producer_output_calculation_reduced_efficiency();
    test_producer_output_calculation_with_aging();
    test_producer_output_calculation_combined();
    test_producer_offline_output();
    test_producer_contamination();
    test_producer_ticks_since_built();
    test_producer_copy();

    printf("\n=== All EnergyProducerComponent Tests Passed ===\n");
    return 0;
}
