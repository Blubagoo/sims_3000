/**
 * @file test_fluid_producer_component.cpp
 * @brief Unit tests for FluidProducerComponent (Epic 6, Ticket 6-003)
 *
 * Tests cover:
 * - Size verification (12 bytes)
 * - Trivially copyable for serialization
 * - Default initialization values
 * - Output calculation: current_output = base_output * water_factor * (powered ? 1 : 0)
 * - Non-operational behavior: current_output should be 0 when !is_operational
 * - Producer type assignment (Extractor/Reservoir)
 * - Water distance tracking
 * - NO aging fields, NO contamination fields
 */

#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/fluid/FluidEnums.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::fluid;

void test_producer_component_size() {
    printf("Testing FluidProducerComponent size...\n");

    assert(sizeof(FluidProducerComponent) == 12);

    printf("  PASS: FluidProducerComponent is 12 bytes\n");
}

void test_producer_trivially_copyable() {
    printf("Testing FluidProducerComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<FluidProducerComponent>::value);

    printf("  PASS: FluidProducerComponent is trivially copyable\n");
}

void test_producer_default_initialization() {
    printf("Testing default initialization...\n");

    FluidProducerComponent fpc{};
    assert(fpc.base_output == 0);
    assert(fpc.current_output == 0);
    assert(fpc.max_water_distance == 5);
    assert(fpc.current_water_distance == 0);
    assert(fpc.is_operational == false);
    assert(fpc.producer_type == 0);

    printf("  PASS: Default initialization works correctly\n");
}

void test_producer_type_values() {
    printf("Testing producer type assignment...\n");

    FluidProducerComponent fpc{};

    // Extractor type
    fpc.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);
    assert(fpc.producer_type == 0);

    // Reservoir type
    fpc.producer_type = static_cast<uint8_t>(FluidProducerType::Reservoir);
    assert(fpc.producer_type == 1);

    // Round-trip through FluidProducerType
    FluidProducerType type = static_cast<FluidProducerType>(fpc.producer_type);
    assert(type == FluidProducerType::Reservoir);

    printf("  PASS: Producer type assignment works correctly\n");
}

void test_producer_output_operational() {
    printf("Testing output calculation when operational...\n");

    FluidProducerComponent fpc{};
    fpc.base_output = 1000;
    fpc.is_operational = true;
    fpc.current_water_distance = 2;
    fpc.max_water_distance = 5;

    // Simulate: current_output = base_output * water_factor * (operational ? 1 : 0)
    // water_factor = 1.0 when within max_water_distance
    float water_factor = (fpc.current_water_distance <= fpc.max_water_distance) ? 1.0f : 0.0f;
    uint32_t calculated = fpc.is_operational
        ? static_cast<uint32_t>(static_cast<float>(fpc.base_output) * water_factor)
        : 0;
    fpc.current_output = calculated;

    assert(fpc.current_output == 1000);

    printf("  PASS: Operational output is correct\n");
}

void test_producer_output_not_operational() {
    printf("Testing non-operational producer produces zero output...\n");

    FluidProducerComponent fpc{};
    fpc.base_output = 1000;
    fpc.is_operational = false;
    fpc.current_water_distance = 2;
    fpc.max_water_distance = 5;

    // Non-operational should produce 0
    uint32_t calculated = fpc.is_operational
        ? static_cast<uint32_t>(static_cast<float>(fpc.base_output))
        : 0;
    fpc.current_output = calculated;

    assert(fpc.current_output == 0);

    printf("  PASS: Non-operational producer produces zero output\n");
}

void test_producer_output_too_far_from_water() {
    printf("Testing producer too far from water produces zero...\n");

    FluidProducerComponent fpc{};
    fpc.base_output = 1000;
    fpc.is_operational = true;
    fpc.current_water_distance = 10;
    fpc.max_water_distance = 5;

    // Beyond max water distance: water_factor = 0
    float water_factor = (fpc.current_water_distance <= fpc.max_water_distance) ? 1.0f : 0.0f;
    uint32_t calculated = fpc.is_operational
        ? static_cast<uint32_t>(static_cast<float>(fpc.base_output) * water_factor)
        : 0;
    fpc.current_output = calculated;

    assert(fpc.current_output == 0);

    printf("  PASS: Producer too far from water produces zero\n");
}

void test_producer_water_distance_at_boundary() {
    printf("Testing water distance at exact boundary...\n");

    FluidProducerComponent fpc{};
    fpc.base_output = 500;
    fpc.is_operational = true;
    fpc.max_water_distance = 5;

    // Exactly at boundary
    fpc.current_water_distance = 5;
    float water_factor = (fpc.current_water_distance <= fpc.max_water_distance) ? 1.0f : 0.0f;
    uint32_t calculated = fpc.is_operational
        ? static_cast<uint32_t>(static_cast<float>(fpc.base_output) * water_factor)
        : 0;
    fpc.current_output = calculated;
    assert(fpc.current_output == 500);

    // One past boundary
    fpc.current_water_distance = 6;
    water_factor = (fpc.current_water_distance <= fpc.max_water_distance) ? 1.0f : 0.0f;
    calculated = fpc.is_operational
        ? static_cast<uint32_t>(static_cast<float>(fpc.base_output) * water_factor)
        : 0;
    fpc.current_output = calculated;
    assert(fpc.current_output == 0);

    printf("  PASS: Water distance boundary behavior is correct\n");
}

void test_producer_water_distance_tracking() {
    printf("Testing water distance tracking...\n");

    FluidProducerComponent fpc{};

    // Default max distance is 5
    assert(fpc.max_water_distance == 5);

    // Set custom max distance
    fpc.max_water_distance = 3;
    assert(fpc.max_water_distance == 3);

    // Track current distance
    fpc.current_water_distance = 0;
    assert(fpc.current_water_distance == 0);

    fpc.current_water_distance = 255;  // uint8_t max
    assert(fpc.current_water_distance == 255);

    printf("  PASS: Water distance tracking works correctly\n");
}

void test_producer_copy() {
    printf("Testing copy semantics...\n");

    FluidProducerComponent original{};
    original.base_output = 500;
    original.current_output = 500;
    original.max_water_distance = 5;
    original.current_water_distance = 3;
    original.is_operational = true;
    original.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);

    FluidProducerComponent copy = original;
    assert(copy.base_output == 500);
    assert(copy.current_output == 500);
    assert(copy.max_water_distance == 5);
    assert(copy.current_water_distance == 3);
    assert(copy.is_operational == true);
    assert(copy.producer_type == static_cast<uint8_t>(FluidProducerType::Extractor));

    printf("  PASS: Copy semantics work correctly\n");
}

void test_producer_no_aging_no_contamination() {
    printf("Testing no aging/contamination fields (simpler than energy)...\n");

    // FluidProducerComponent is exactly 12 bytes.
    // EnergyProducerComponent is 24 bytes (has efficiency, age_factor,
    // ticks_since_built, contamination_output).
    // Verify fluid is simpler by size alone.
    assert(sizeof(FluidProducerComponent) == 12);

    // Verify only expected fields exist by setting all and checking size
    FluidProducerComponent fpc{};
    fpc.base_output = 1000;
    fpc.current_output = 1000;
    fpc.max_water_distance = 5;
    fpc.current_water_distance = 2;
    fpc.is_operational = true;
    fpc.producer_type = 1;

    // All fields accessible, no extra fields
    assert(fpc.base_output == 1000);
    assert(fpc.current_output == 1000);
    assert(fpc.max_water_distance == 5);
    assert(fpc.current_water_distance == 2);
    assert(fpc.is_operational == true);
    assert(fpc.producer_type == 1);

    printf("  PASS: No aging or contamination fields present\n");
}

int main() {
    printf("=== FluidProducerComponent Unit Tests (Epic 6, Ticket 6-003) ===\n\n");

    test_producer_component_size();
    test_producer_trivially_copyable();
    test_producer_default_initialization();
    test_producer_type_values();
    test_producer_output_operational();
    test_producer_output_not_operational();
    test_producer_output_too_far_from_water();
    test_producer_water_distance_at_boundary();
    test_producer_water_distance_tracking();
    test_producer_copy();
    test_producer_no_aging_no_contamination();

    printf("\n=== All FluidProducerComponent Tests Passed ===\n");
    return 0;
}
