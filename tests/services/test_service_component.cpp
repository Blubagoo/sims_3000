/**
 * @file test_service_component.cpp
 * @brief Unit tests for ServiceProviderComponent and serialization
 *        (Epic 9, Ticket E9-002)
 *
 * Tests cover:
 * - ServiceProviderComponent size assertion (4 bytes)
 * - Trivially copyable check
 * - Default initialization
 * - Custom value assignment
 * - All service type assignments
 * - Tier range (1-3)
 * - Copy semantics
 * - Serialization round-trip
 * - Serialization version check
 * - Buffer too small check
 */

#include <sims3000/services/ServiceProviderComponent.h>
#include <sims3000/services/ServiceSerialization.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

using namespace sims3000::services;

void test_component_size() {
    printf("Testing ServiceProviderComponent size...\n");

    assert(sizeof(ServiceProviderComponent) == 4);

    printf("  PASS: ServiceProviderComponent is 4 bytes\n");
}

void test_trivially_copyable() {
    printf("Testing ServiceProviderComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<ServiceProviderComponent>::value);

    printf("  PASS: ServiceProviderComponent is trivially copyable\n");
}

void test_default_initialization() {
    printf("Testing default initialization...\n");

    ServiceProviderComponent comp{};
    assert(comp.service_type == ServiceType::Enforcer);
    assert(comp.tier == 1);
    assert(comp.current_effectiveness == 0);
    assert(comp.is_active == false);

    printf("  PASS: Default initialization works correctly\n");
}

void test_custom_values() {
    printf("Testing custom value assignment...\n");

    ServiceProviderComponent comp{};
    comp.service_type = ServiceType::Medical;
    comp.tier = 3;
    comp.current_effectiveness = 200;
    comp.is_active = true;

    assert(comp.service_type == ServiceType::Medical);
    assert(comp.tier == 3);
    assert(comp.current_effectiveness == 200);
    assert(comp.is_active == true);

    printf("  PASS: Custom value assignment works correctly\n");
}

void test_all_service_types() {
    printf("Testing all service types can be assigned...\n");

    ServiceProviderComponent comp{};

    comp.service_type = ServiceType::Enforcer;
    assert(comp.service_type == ServiceType::Enforcer);
    assert(static_cast<uint8_t>(comp.service_type) == 0);

    comp.service_type = ServiceType::HazardResponse;
    assert(comp.service_type == ServiceType::HazardResponse);
    assert(static_cast<uint8_t>(comp.service_type) == 1);

    comp.service_type = ServiceType::Medical;
    assert(comp.service_type == ServiceType::Medical);
    assert(static_cast<uint8_t>(comp.service_type) == 2);

    comp.service_type = ServiceType::Education;
    assert(comp.service_type == ServiceType::Education);
    assert(static_cast<uint8_t>(comp.service_type) == 3);

    printf("  PASS: All service types can be assigned\n");
}

void test_tier_range() {
    printf("Testing tier values 1-3...\n");

    ServiceProviderComponent comp{};

    for (uint8_t tier = 1; tier <= 3; ++tier) {
        comp.tier = tier;
        assert(comp.tier == tier);
    }

    printf("  PASS: Tier values 1-3 work correctly\n");
}

void test_effectiveness_range() {
    printf("Testing effectiveness range 0-255...\n");

    ServiceProviderComponent comp{};

    comp.current_effectiveness = 0;
    assert(comp.current_effectiveness == 0);

    comp.current_effectiveness = 100;
    assert(comp.current_effectiveness == 100);

    comp.current_effectiveness = 255;
    assert(comp.current_effectiveness == 255);

    printf("  PASS: Effectiveness range works correctly\n");
}

void test_copy_semantics() {
    printf("Testing copy semantics...\n");

    ServiceProviderComponent original{};
    original.service_type = ServiceType::Education;
    original.tier = 2;
    original.current_effectiveness = 150;
    original.is_active = true;

    ServiceProviderComponent copy = original;
    assert(copy.service_type == ServiceType::Education);
    assert(copy.tier == 2);
    assert(copy.current_effectiveness == 150);
    assert(copy.is_active == true);

    printf("  PASS: Copy semantics work correctly\n");
}

void test_memcpy_safe() {
    printf("Testing memcpy safety...\n");

    ServiceProviderComponent original{};
    original.service_type = ServiceType::HazardResponse;
    original.tier = 3;
    original.current_effectiveness = 99;
    original.is_active = true;

    ServiceProviderComponent copy{};
    std::memcpy(&copy, &original, sizeof(ServiceProviderComponent));

    assert(copy.service_type == ServiceType::HazardResponse);
    assert(copy.tier == 3);
    assert(copy.current_effectiveness == 99);
    assert(copy.is_active == true);

    printf("  PASS: memcpy safety works correctly\n");
}

// ============================================================================
// Serialization tests
// ============================================================================

void test_serialization_round_trip_default() {
    printf("Testing serialization round-trip (default values)...\n");

    ServiceProviderComponent original{};
    std::vector<uint8_t> buffer;
    serialize_service_provider(original, buffer);

    assert(buffer.size() == SERVICE_PROVIDER_SERIALIZED_SIZE);

    ServiceProviderComponent deserialized{};
    deserialized.service_type = ServiceType::Education;  // Set to non-default
    deserialized.tier = 3;
    deserialized.current_effectiveness = 255;
    deserialized.is_active = true;

    size_t consumed = deserialize_service_provider(buffer.data(), buffer.size(), deserialized);
    assert(consumed == SERVICE_PROVIDER_SERIALIZED_SIZE);

    assert(deserialized.service_type == original.service_type);
    assert(deserialized.tier == original.tier);
    assert(deserialized.current_effectiveness == original.current_effectiveness);
    assert(deserialized.is_active == original.is_active);

    printf("  PASS: Serialization round-trip (default) correct\n");
}

void test_serialization_round_trip_custom() {
    printf("Testing serialization round-trip (custom values)...\n");

    ServiceProviderComponent original{};
    original.service_type = ServiceType::Medical;
    original.tier = 2;
    original.current_effectiveness = 175;
    original.is_active = true;

    std::vector<uint8_t> buffer;
    serialize_service_provider(original, buffer);

    assert(buffer.size() == SERVICE_PROVIDER_SERIALIZED_SIZE);

    ServiceProviderComponent deserialized{};
    size_t consumed = deserialize_service_provider(buffer.data(), buffer.size(), deserialized);
    assert(consumed == SERVICE_PROVIDER_SERIALIZED_SIZE);

    assert(deserialized.service_type == ServiceType::Medical);
    assert(deserialized.tier == 2);
    assert(deserialized.current_effectiveness == 175);
    assert(deserialized.is_active == true);

    printf("  PASS: Serialization round-trip (custom) correct\n");
}

void test_serialization_all_types() {
    printf("Testing serialization round-trip for all service types...\n");

    for (uint8_t t = 0; t < SERVICE_TYPE_COUNT; ++t) {
        ServiceProviderComponent original{};
        original.service_type = static_cast<ServiceType>(t);
        original.tier = static_cast<uint8_t>((t % 3) + 1);
        original.current_effectiveness = static_cast<uint8_t>(t * 50);
        original.is_active = (t % 2 == 0);

        std::vector<uint8_t> buffer;
        serialize_service_provider(original, buffer);

        ServiceProviderComponent deserialized{};
        deserialize_service_provider(buffer.data(), buffer.size(), deserialized);

        assert(deserialized.service_type == original.service_type);
        assert(deserialized.tier == original.tier);
        assert(deserialized.current_effectiveness == original.current_effectiveness);
        assert(deserialized.is_active == original.is_active);
    }

    printf("  PASS: Serialization round-trip for all types correct\n");
}

void test_serialization_version_byte() {
    printf("Testing serialization version byte...\n");

    ServiceProviderComponent comp{};
    std::vector<uint8_t> buffer;
    serialize_service_provider(comp, buffer);

    // First byte should be the version
    assert(buffer[0] == SERVICE_SERIALIZATION_VERSION);

    printf("  PASS: Serialization version byte correct\n");
}

void test_serialization_buffer_too_small() {
    printf("Testing deserialization with buffer too small...\n");

    uint8_t small_buffer[3] = {0, 0, 0};
    ServiceProviderComponent comp{};

    bool threw = false;
    try {
        deserialize_service_provider(small_buffer, 3, comp);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    printf("  PASS: Buffer too small throws correctly\n");
}

void test_serialization_version_mismatch() {
    printf("Testing deserialization with version mismatch...\n");

    // Create a buffer with wrong version
    uint8_t bad_version_buffer[5] = {99, 0, 1, 100, 1};  // version=99
    ServiceProviderComponent comp{};

    bool threw = false;
    try {
        deserialize_service_provider(bad_version_buffer, 5, comp);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    printf("  PASS: Version mismatch throws correctly\n");
}

void test_serialized_size_constant() {
    printf("Testing SERVICE_PROVIDER_SERIALIZED_SIZE constant...\n");

    assert(SERVICE_PROVIDER_SERIALIZED_SIZE == 5);

    printf("  PASS: SERVICE_PROVIDER_SERIALIZED_SIZE is 5\n");
}

int main() {
    printf("=== ServiceProviderComponent Unit Tests (Epic 9, Ticket E9-002) ===\n\n");

    test_component_size();
    test_trivially_copyable();
    test_default_initialization();
    test_custom_values();
    test_all_service_types();
    test_tier_range();
    test_effectiveness_range();
    test_copy_semantics();
    test_memcpy_safe();
    test_serialization_round_trip_default();
    test_serialization_round_trip_custom();
    test_serialization_all_types();
    test_serialization_version_byte();
    test_serialization_buffer_too_small();
    test_serialization_version_mismatch();
    test_serialized_size_constant();

    printf("\n=== All ServiceProviderComponent Tests Passed ===\n");
    return 0;
}
