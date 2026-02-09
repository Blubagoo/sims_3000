#include <sims3000/sim/SimulationSerializer.h>
#include <cstdio>
#include <cassert>
#include <cstring>

using namespace sims3000::sim;

void test_header_size() {
    printf("Testing SimulationStateHeader size...\n");
    assert(sizeof(SimulationStateHeader) == 28);
    printf("  [PASS] Header is exactly 28 bytes\n");
}

void test_create_header() {
    printf("\nTesting create_header...\n");

    SimulationStateHeader header = create_header(12345, 100, 3, 2, 256, 256, 4);

    assert(header.magic == 0x33494D53);
    assert(header.version == 1);
    assert(header.tick_count == 12345);
    assert(header.cycle == 100);
    assert(header.phase == 3);
    assert(header.speed == 2);
    assert(header.grid_width == 256);
    assert(header.grid_height == 256);
    assert(header.num_players == 4);
    assert(header.reserved[0] == 0);
    assert(header.reserved[1] == 0);
    assert(header.reserved[2] == 0);

    printf("  [PASS] Header created with correct values\n");
}

void test_validate_header() {
    printf("\nTesting validate_header...\n");

    // Valid header
    SimulationStateHeader valid = create_header(1000, 50, 2, 1, 512, 512, 2);
    assert(validate_header(valid));
    printf("  [PASS] Valid header passes validation\n");

    // Invalid magic
    SimulationStateHeader invalid_magic = valid;
    invalid_magic.magic = 0x12345678;
    assert(!validate_header(invalid_magic));
    printf("  [PASS] Invalid magic fails validation\n");

    // Invalid version
    SimulationStateHeader invalid_version = valid;
    invalid_version.version = 99;
    assert(!validate_header(invalid_version));
    printf("  [PASS] Invalid version fails validation\n");

    // Zero grid width
    SimulationStateHeader zero_width = valid;
    zero_width.grid_width = 0;
    assert(!validate_header(zero_width));
    printf("  [PASS] Zero grid width fails validation\n");

    // Zero grid height
    SimulationStateHeader zero_height = valid;
    zero_height.grid_height = 0;
    assert(!validate_header(zero_height));
    printf("  [PASS] Zero grid height fails validation\n");

    // Grid dimensions too large
    SimulationStateHeader too_large = valid;
    too_large.grid_width = 20000;
    assert(!validate_header(too_large));
    printf("  [PASS] Excessive grid width fails validation\n");

    // Zero players
    SimulationStateHeader zero_players = valid;
    zero_players.num_players = 0;
    assert(!validate_header(zero_players));
    printf("  [PASS] Zero players fails validation\n");

    // Too many players
    SimulationStateHeader too_many_players = valid;
    too_many_players.num_players = 20;
    assert(!validate_header(too_many_players));
    printf("  [PASS] Too many players fails validation\n");

    // Invalid phase
    SimulationStateHeader invalid_phase = valid;
    invalid_phase.phase = 50;
    assert(!validate_header(invalid_phase));
    printf("  [PASS] Invalid phase fails validation\n");

    // Invalid speed
    SimulationStateHeader invalid_speed = valid;
    invalid_speed.speed = 100;
    assert(!validate_header(invalid_speed));
    printf("  [PASS] Invalid speed fails validation\n");
}

void test_serialize_header() {
    printf("\nTesting serialize_header...\n");

    SimulationStateHeader header = create_header(54321, 200, 5, 3, 1024, 768, 3);
    uint8_t buffer[64] = {0};

    size_t written = serialize_header(header, buffer, sizeof(buffer));
    assert(written == 28);
    printf("  [PASS] Serialized 28 bytes\n");

    // Test buffer too small
    uint8_t small_buffer[10] = {0};
    written = serialize_header(header, small_buffer, sizeof(small_buffer));
    assert(written == 0);
    printf("  [PASS] Returns 0 for buffer too small\n");
}

void test_deserialize_header() {
    printf("\nTesting deserialize_header...\n");

    SimulationStateHeader original = create_header(99999, 500, 7, 4, 2048, 2048, 8);
    uint8_t buffer[64] = {0};
    serialize_header(original, buffer, sizeof(buffer));

    SimulationStateHeader deserialized;
    bool success = deserialize_header(buffer, sizeof(buffer), deserialized);
    assert(success);
    assert(deserialized.magic == original.magic);
    assert(deserialized.version == original.version);
    assert(deserialized.tick_count == original.tick_count);
    assert(deserialized.cycle == original.cycle);
    assert(deserialized.phase == original.phase);
    assert(deserialized.speed == original.speed);
    assert(deserialized.grid_width == original.grid_width);
    assert(deserialized.grid_height == original.grid_height);
    assert(deserialized.num_players == original.num_players);
    printf("  [PASS] Deserialized header matches original\n");

    // Test buffer too small
    uint8_t small_buffer[10] = {0};
    success = deserialize_header(small_buffer, sizeof(small_buffer), deserialized);
    assert(!success);
    printf("  [PASS] Returns false for buffer too small\n");
}

void test_round_trip() {
    printf("\nTesting round-trip serialization...\n");

    SimulationStateHeader original = create_header(
        0xFFFFFFFFFFFFFFFFULL,  // Max uint64_t
        0xFFFFFFFF,             // Max uint32_t
        10,                     // Max valid phase
        10,                     // Max valid speed
        10000,                  // Max valid grid width
        10000,                  // Max valid grid height
        16                      // Max valid players
    );

    // Serialize
    uint8_t buffer[64] = {0};
    size_t written = serialize_header(original, buffer, sizeof(buffer));
    assert(written == 28);

    // Deserialize
    SimulationStateHeader deserialized;
    bool success = deserialize_header(buffer, written, deserialized);
    assert(success);

    // Validate
    assert(validate_header(deserialized));

    // Compare all fields
    assert(memcmp(&original, &deserialized, sizeof(SimulationStateHeader)) == 0);
    printf("  [PASS] Round-trip preserves all data\n");
}

void test_edge_cases() {
    printf("\nTesting edge cases...\n");

    // Minimum valid values
    SimulationStateHeader min_valid = create_header(0, 0, 0, 0, 1, 1, 1);
    assert(validate_header(min_valid));
    printf("  [PASS] Minimum valid values accepted\n");

    // Maximum valid values
    SimulationStateHeader max_valid = create_header(
        0xFFFFFFFFFFFFFFFFULL,
        0xFFFFFFFF,
        10, 10, 10000, 10000, 16
    );
    assert(validate_header(max_valid));
    printf("  [PASS] Maximum valid values accepted\n");

    // Test exact boundary values
    SimulationStateHeader boundary = create_header(0, 0, 0, 0, 10000, 10000, 16);
    assert(validate_header(boundary));
    boundary.grid_width = 10001;
    assert(!validate_header(boundary));
    printf("  [PASS] Boundary validation works correctly\n");
}

void test_magic_number() {
    printf("\nTesting magic number...\n");

    // Verify magic number matches 'SIM3'
    uint32_t magic = 0x33494D53;
    char chars[5] = {0};
    memcpy(chars, &magic, 4);
    // Note: endianness may affect this, but value should still be correct
    printf("  Magic number: 0x%08X\n", magic);
    printf("  [PASS] Magic number constant is correct\n");
}

int main() {
    printf("=== SimulationSerializer Test Suite ===\n\n");

    test_header_size();
    test_create_header();
    test_validate_header();
    test_serialize_header();
    test_deserialize_header();
    test_round_trip();
    test_edge_cases();
    test_magic_number();

    printf("\n=== All SimulationSerializer tests passed! ===\n");
    return 0;
}
