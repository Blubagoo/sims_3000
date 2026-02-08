/**
 * @file test_port_types.cpp
 * @brief Unit tests for PortTypes enums (Epic 8, Tickets E8-001, E8-004, E8-005)
 *
 * Tests cover:
 * - PortType enum values (0-1) and canonical terminology
 * - MapEdge enum values (0-3)
 * - ConnectionType enum values (0-3)
 * - TradeAgreementType enum values (0-3)
 * - String conversion functions
 * - Enum underlying type sizes (1 byte each)
 * - Count constants
 */

#include <sims3000/port/PortTypes.h>
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace sims3000::port;

void test_port_type_enum_values() {
    printf("Testing PortType enum values...\n");

    assert(static_cast<uint8_t>(PortType::Aero) == 0);
    assert(static_cast<uint8_t>(PortType::Aqua) == 1);

    printf("  PASS: PortType enum values correct\n");
}

void test_port_type_count() {
    printf("Testing PortType count...\n");

    assert(PORT_TYPE_COUNT == 2);

    printf("  PASS: PORT_TYPE_COUNT correct\n");
}

void test_port_type_to_string() {
    printf("Testing port_type_to_string...\n");

    assert(strcmp(port_type_to_string(PortType::Aero), "Aero") == 0);
    assert(strcmp(port_type_to_string(PortType::Aqua), "Aqua") == 0);

    // Test unknown value
    assert(strcmp(port_type_to_string(static_cast<PortType>(255)), "Unknown") == 0);

    printf("  PASS: port_type_to_string works correctly\n");
}

void test_map_edge_enum_values() {
    printf("Testing MapEdge enum values...\n");

    assert(static_cast<uint8_t>(MapEdge::North) == 0);
    assert(static_cast<uint8_t>(MapEdge::East)  == 1);
    assert(static_cast<uint8_t>(MapEdge::South) == 2);
    assert(static_cast<uint8_t>(MapEdge::West)  == 3);

    printf("  PASS: MapEdge enum values correct\n");
}

void test_map_edge_count() {
    printf("Testing MapEdge count...\n");

    assert(MAP_EDGE_COUNT == 4);

    printf("  PASS: MAP_EDGE_COUNT correct\n");
}

void test_map_edge_to_string() {
    printf("Testing map_edge_to_string...\n");

    assert(strcmp(map_edge_to_string(MapEdge::North), "North") == 0);
    assert(strcmp(map_edge_to_string(MapEdge::East), "East") == 0);
    assert(strcmp(map_edge_to_string(MapEdge::South), "South") == 0);
    assert(strcmp(map_edge_to_string(MapEdge::West), "West") == 0);

    // Test unknown value
    assert(strcmp(map_edge_to_string(static_cast<MapEdge>(255)), "Unknown") == 0);

    printf("  PASS: map_edge_to_string works correctly\n");
}

void test_connection_type_enum_values() {
    printf("Testing ConnectionType enum values...\n");

    assert(static_cast<uint8_t>(ConnectionType::Pathway) == 0);
    assert(static_cast<uint8_t>(ConnectionType::Rail)    == 1);
    assert(static_cast<uint8_t>(ConnectionType::Energy)  == 2);
    assert(static_cast<uint8_t>(ConnectionType::Fluid)   == 3);

    printf("  PASS: ConnectionType enum values correct\n");
}

void test_connection_type_count() {
    printf("Testing ConnectionType count...\n");

    assert(CONNECTION_TYPE_COUNT == 4);

    printf("  PASS: CONNECTION_TYPE_COUNT correct\n");
}

void test_connection_type_to_string() {
    printf("Testing connection_type_to_string...\n");

    assert(strcmp(connection_type_to_string(ConnectionType::Pathway), "Pathway") == 0);
    assert(strcmp(connection_type_to_string(ConnectionType::Rail), "Rail") == 0);
    assert(strcmp(connection_type_to_string(ConnectionType::Energy), "Energy") == 0);
    assert(strcmp(connection_type_to_string(ConnectionType::Fluid), "Fluid") == 0);

    // Test unknown value
    assert(strcmp(connection_type_to_string(static_cast<ConnectionType>(255)), "Unknown") == 0);

    printf("  PASS: connection_type_to_string works correctly\n");
}

void test_trade_agreement_type_enum_values() {
    printf("Testing TradeAgreementType enum values...\n");

    assert(static_cast<uint8_t>(TradeAgreementType::None)     == 0);
    assert(static_cast<uint8_t>(TradeAgreementType::Basic)    == 1);
    assert(static_cast<uint8_t>(TradeAgreementType::Enhanced) == 2);
    assert(static_cast<uint8_t>(TradeAgreementType::Premium)  == 3);

    printf("  PASS: TradeAgreementType enum values correct\n");
}

void test_trade_agreement_type_count() {
    printf("Testing TradeAgreementType count...\n");

    assert(TRADE_AGREEMENT_TYPE_COUNT == 4);

    printf("  PASS: TRADE_AGREEMENT_TYPE_COUNT correct\n");
}

void test_trade_agreement_type_to_string() {
    printf("Testing trade_agreement_type_to_string...\n");

    assert(strcmp(trade_agreement_type_to_string(TradeAgreementType::None), "None") == 0);
    assert(strcmp(trade_agreement_type_to_string(TradeAgreementType::Basic), "Basic") == 0);
    assert(strcmp(trade_agreement_type_to_string(TradeAgreementType::Enhanced), "Enhanced") == 0);
    assert(strcmp(trade_agreement_type_to_string(TradeAgreementType::Premium), "Premium") == 0);

    // Test unknown value
    assert(strcmp(trade_agreement_type_to_string(static_cast<TradeAgreementType>(255)), "Unknown") == 0);

    printf("  PASS: trade_agreement_type_to_string works correctly\n");
}

void test_enum_underlying_type_sizes() {
    printf("Testing enum underlying type sizes...\n");

    assert(sizeof(PortType) == 1);
    assert(sizeof(MapEdge) == 1);
    assert(sizeof(ConnectionType) == 1);
    assert(sizeof(TradeAgreementType) == 1);

    printf("  PASS: All enum underlying type sizes correct (1 byte each)\n");
}

void test_all_port_types_have_strings() {
    printf("Testing all port types have valid string representations...\n");

    for (uint8_t i = 0; i < PORT_TYPE_COUNT; ++i) {
        PortType type = static_cast<PortType>(i);
        assert(strcmp(port_type_to_string(type), "Unknown") != 0);
    }

    printf("  PASS: All port types have valid strings\n");
}

void test_all_map_edges_have_strings() {
    printf("Testing all map edges have valid string representations...\n");

    for (uint8_t i = 0; i < MAP_EDGE_COUNT; ++i) {
        MapEdge edge = static_cast<MapEdge>(i);
        assert(strcmp(map_edge_to_string(edge), "Unknown") != 0);
    }

    printf("  PASS: All map edges have valid strings\n");
}

void test_all_connection_types_have_strings() {
    printf("Testing all connection types have valid string representations...\n");

    for (uint8_t i = 0; i < CONNECTION_TYPE_COUNT; ++i) {
        ConnectionType type = static_cast<ConnectionType>(i);
        assert(strcmp(connection_type_to_string(type), "Unknown") != 0);
    }

    printf("  PASS: All connection types have valid strings\n");
}

void test_canonical_terminology() {
    printf("Testing canonical alien terminology...\n");

    // Verify we use "Aero" (for aero_port) not "Airport"
    assert(strcmp(port_type_to_string(PortType::Aero), "Aero") == 0);
    // Verify we use "Aqua" (for aqua_port) not "Seaport"
    assert(strcmp(port_type_to_string(PortType::Aqua), "Aqua") == 0);
    // Verify we use "Pathway" not "Road"
    assert(strcmp(connection_type_to_string(ConnectionType::Pathway), "Pathway") == 0);

    printf("  PASS: Canonical alien terminology verified\n");
}

int main() {
    printf("=== PortTypes Unit Tests (Epic 8, Tickets E8-001, E8-004, E8-005) ===\n\n");

    test_port_type_enum_values();
    test_port_type_count();
    test_port_type_to_string();
    test_map_edge_enum_values();
    test_map_edge_count();
    test_map_edge_to_string();
    test_connection_type_enum_values();
    test_connection_type_count();
    test_connection_type_to_string();
    test_trade_agreement_type_enum_values();
    test_trade_agreement_type_count();
    test_trade_agreement_type_to_string();
    test_enum_underlying_type_sizes();
    test_all_port_types_have_strings();
    test_all_map_edges_have_strings();
    test_all_connection_types_have_strings();
    test_canonical_terminology();

    printf("\n=== All PortTypes Tests Passed ===\n");
    return 0;
}
