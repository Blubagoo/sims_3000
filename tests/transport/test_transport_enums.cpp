/**
 * @file test_transport_enums.cpp
 * @brief Unit tests for TransportEnums (Epic 7, Ticket E7-001)
 *
 * Tests cover:
 * - PathwayType enum values (0-4)
 * - PathwayDirection enum values (0-4)
 * - pathway_type_to_string conversion
 * - pathway_direction_to_string conversion
 * - is_one_way helper
 * - Enum underlying type sizes (1 byte each)
 * - Count constants
 */

#include <sims3000/transport/TransportEnums.h>
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace sims3000::transport;

void test_pathway_type_enum_values() {
    printf("Testing PathwayType enum values...\n");

    assert(static_cast<uint8_t>(PathwayType::BasicPathway) == 0);
    assert(static_cast<uint8_t>(PathwayType::TransitCorridor) == 1);
    assert(static_cast<uint8_t>(PathwayType::Pedestrian) == 2);
    assert(static_cast<uint8_t>(PathwayType::Bridge) == 3);
    assert(static_cast<uint8_t>(PathwayType::Tunnel) == 4);

    printf("  PASS: PathwayType enum values correct\n");
}

void test_pathway_direction_enum_values() {
    printf("Testing PathwayDirection enum values...\n");

    assert(static_cast<uint8_t>(PathwayDirection::Bidirectional) == 0);
    assert(static_cast<uint8_t>(PathwayDirection::OneWayNorth) == 1);
    assert(static_cast<uint8_t>(PathwayDirection::OneWaySouth) == 2);
    assert(static_cast<uint8_t>(PathwayDirection::OneWayEast) == 3);
    assert(static_cast<uint8_t>(PathwayDirection::OneWayWest) == 4);

    printf("  PASS: PathwayDirection enum values correct\n");
}

void test_pathway_type_counts() {
    printf("Testing PathwayType counts...\n");

    assert(PATHWAY_TYPE_COUNT == 5);
    assert(PATHWAY_DIRECTION_COUNT == 5);

    printf("  PASS: Pathway counts correct\n");
}

void test_pathway_type_to_string() {
    printf("Testing pathway_type_to_string...\n");

    assert(strcmp(pathway_type_to_string(PathwayType::BasicPathway), "BasicPathway") == 0);
    assert(strcmp(pathway_type_to_string(PathwayType::TransitCorridor), "TransitCorridor") == 0);
    assert(strcmp(pathway_type_to_string(PathwayType::Pedestrian), "Pedestrian") == 0);
    assert(strcmp(pathway_type_to_string(PathwayType::Bridge), "Bridge") == 0);
    assert(strcmp(pathway_type_to_string(PathwayType::Tunnel), "Tunnel") == 0);

    // Test unknown value
    assert(strcmp(pathway_type_to_string(static_cast<PathwayType>(255)), "Unknown") == 0);

    printf("  PASS: pathway_type_to_string works correctly\n");
}

void test_pathway_direction_to_string() {
    printf("Testing pathway_direction_to_string...\n");

    assert(strcmp(pathway_direction_to_string(PathwayDirection::Bidirectional), "Bidirectional") == 0);
    assert(strcmp(pathway_direction_to_string(PathwayDirection::OneWayNorth), "OneWayNorth") == 0);
    assert(strcmp(pathway_direction_to_string(PathwayDirection::OneWaySouth), "OneWaySouth") == 0);
    assert(strcmp(pathway_direction_to_string(PathwayDirection::OneWayEast), "OneWayEast") == 0);
    assert(strcmp(pathway_direction_to_string(PathwayDirection::OneWayWest), "OneWayWest") == 0);

    // Test unknown value
    assert(strcmp(pathway_direction_to_string(static_cast<PathwayDirection>(255)), "Unknown") == 0);

    printf("  PASS: pathway_direction_to_string works correctly\n");
}

void test_is_one_way() {
    printf("Testing is_one_way...\n");

    assert(!is_one_way(PathwayDirection::Bidirectional));
    assert(is_one_way(PathwayDirection::OneWayNorth));
    assert(is_one_way(PathwayDirection::OneWaySouth));
    assert(is_one_way(PathwayDirection::OneWayEast));
    assert(is_one_way(PathwayDirection::OneWayWest));

    printf("  PASS: is_one_way works correctly\n");
}

void test_enum_underlying_type_sizes() {
    printf("Testing enum underlying type sizes...\n");

    assert(sizeof(PathwayType) == 1);
    assert(sizeof(PathwayDirection) == 1);

    printf("  PASS: Enum underlying type sizes correct (1 byte each)\n");
}

void test_enum_value_ranges() {
    printf("Testing enum value ranges...\n");

    // PathwayType range: 0-4
    for (uint8_t i = 0; i < PATHWAY_TYPE_COUNT; ++i) {
        PathwayType type = static_cast<PathwayType>(i);
        // Verify string conversion doesn't return "Unknown" for valid types
        assert(strcmp(pathway_type_to_string(type), "Unknown") != 0);
    }

    // PathwayDirection range: 0-4
    for (uint8_t i = 0; i < PATHWAY_DIRECTION_COUNT; ++i) {
        PathwayDirection dir = static_cast<PathwayDirection>(i);
        assert(strcmp(pathway_direction_to_string(dir), "Unknown") != 0);
    }

    printf("  PASS: Enum value ranges correct\n");
}

void test_alien_terminology() {
    printf("Testing alien terminology (no road/highway terms)...\n");

    // Verify we use "Pathway" not "Road"
    assert(strcmp(pathway_type_to_string(PathwayType::BasicPathway), "BasicPathway") == 0);
    // Verify we use "TransitCorridor" not "Highway"
    assert(strcmp(pathway_type_to_string(PathwayType::TransitCorridor), "TransitCorridor") == 0);

    printf("  PASS: Alien terminology verified\n");
}

int main() {
    printf("=== TransportEnums Unit Tests (Epic 7, Ticket E7-001) ===\n\n");

    test_pathway_type_enum_values();
    test_pathway_direction_enum_values();
    test_pathway_type_counts();
    test_pathway_type_to_string();
    test_pathway_direction_to_string();
    test_is_one_way();
    test_enum_underlying_type_sizes();
    test_enum_value_ranges();
    test_alien_terminology();

    printf("\n=== All TransportEnums Tests Passed ===\n");
    return 0;
}
