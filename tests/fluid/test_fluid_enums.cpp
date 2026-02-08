/**
 * @file test_fluid_enums.cpp
 * @brief Unit tests for FluidEnums (Epic 6, Ticket 6-001)
 */

#include <sims3000/fluid/FluidEnums.h>
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace sims3000::fluid;

void test_fluid_pool_state_enum_values() {
    printf("Testing FluidPoolState enum values...\n");

    assert(static_cast<uint8_t>(FluidPoolState::Healthy) == 0);
    assert(static_cast<uint8_t>(FluidPoolState::Marginal) == 1);
    assert(static_cast<uint8_t>(FluidPoolState::Deficit) == 2);
    assert(static_cast<uint8_t>(FluidPoolState::Collapse) == 3);

    printf("  PASS: FluidPoolState enum values correct\n");
}

void test_fluid_producer_type_enum_values() {
    printf("Testing FluidProducerType enum values...\n");

    assert(static_cast<uint8_t>(FluidProducerType::Extractor) == 0);
    assert(static_cast<uint8_t>(FluidProducerType::Reservoir) == 1);

    printf("  PASS: FluidProducerType enum values correct\n");
}

void test_fluid_producer_type_count() {
    printf("Testing FluidProducerType count...\n");

    assert(FLUID_PRODUCER_TYPE_COUNT == 2);

    printf("  PASS: FluidProducerType count correct\n");
}

void test_constants() {
    printf("Testing constants...\n");

    assert(MAX_PLAYERS == 4);
    assert(INVALID_ENTITY_ID == UINT32_MAX);

    printf("  PASS: Constants correct\n");
}

void test_fluid_pool_state_to_string() {
    printf("Testing fluid_pool_state_to_string...\n");

    assert(strcmp(fluid_pool_state_to_string(FluidPoolState::Healthy), "Healthy") == 0);
    assert(strcmp(fluid_pool_state_to_string(FluidPoolState::Marginal), "Marginal") == 0);
    assert(strcmp(fluid_pool_state_to_string(FluidPoolState::Deficit), "Deficit") == 0);
    assert(strcmp(fluid_pool_state_to_string(FluidPoolState::Collapse), "Collapse") == 0);

    // Test unknown value
    assert(strcmp(fluid_pool_state_to_string(static_cast<FluidPoolState>(255)), "Unknown") == 0);

    printf("  PASS: fluid_pool_state_to_string works correctly\n");
}

void test_fluid_producer_type_to_string() {
    printf("Testing fluid_producer_type_to_string...\n");

    assert(strcmp(fluid_producer_type_to_string(FluidProducerType::Extractor), "Extractor") == 0);
    assert(strcmp(fluid_producer_type_to_string(FluidProducerType::Reservoir), "Reservoir") == 0);

    // Test unknown value
    assert(strcmp(fluid_producer_type_to_string(static_cast<FluidProducerType>(255)), "Unknown") == 0);

    printf("  PASS: fluid_producer_type_to_string works correctly\n");
}

void test_enum_underlying_type_sizes() {
    printf("Testing enum underlying type sizes...\n");

    assert(sizeof(FluidPoolState) == 1);
    assert(sizeof(FluidProducerType) == 1);

    printf("  PASS: Enum underlying type sizes correct\n");
}

void test_enum_value_ranges() {
    printf("Testing enum value ranges...\n");

    // FluidPoolState range: 0-3
    assert(static_cast<uint8_t>(FluidPoolState::Healthy) == 0);
    assert(static_cast<uint8_t>(FluidPoolState::Collapse) == 3);

    // FluidProducerType range: 0-1
    assert(static_cast<uint8_t>(FluidProducerType::Extractor) == 0);
    assert(static_cast<uint8_t>(FluidProducerType::Reservoir) == 1);

    // All valid FluidProducerType values have non-Unknown string
    for (uint8_t i = 0; i < FLUID_PRODUCER_TYPE_COUNT; ++i) {
        FluidProducerType type = static_cast<FluidProducerType>(i);
        assert(strcmp(fluid_producer_type_to_string(type), "Unknown") != 0);
    }

    // All valid FluidPoolState values have non-Unknown string
    for (uint8_t i = 0; i <= 3; ++i) {
        FluidPoolState state = static_cast<FluidPoolState>(i);
        assert(strcmp(fluid_pool_state_to_string(state), "Unknown") != 0);
    }

    printf("  PASS: Enum value ranges correct\n");
}

int main() {
    printf("=== FluidEnums Unit Tests (Epic 6, Ticket 6-001) ===\n\n");

    test_fluid_pool_state_enum_values();
    test_fluid_producer_type_enum_values();
    test_fluid_producer_type_count();
    test_constants();
    test_fluid_pool_state_to_string();
    test_fluid_producer_type_to_string();
    test_enum_underlying_type_sizes();
    test_enum_value_ranges();

    printf("\n=== All FluidEnums Tests Passed ===\n");
    return 0;
}
