/**
 * @file test_rail_network_messages.cpp
 * @brief Tests for rail and terminal network message serialization (Ticket E7-039)
 *
 * Tests cover:
 * - RailPlaceRequest round-trip serialization
 * - RailPlaceResponse round-trip serialization
 * - RailDemolishRequest round-trip serialization
 * - RailDemolishResponse round-trip serialization
 * - TerminalPlaceRequest round-trip serialization
 * - TerminalPlaceResponse round-trip serialization
 * - TerminalDemolishRequest round-trip serialization
 * - TerminalDemolishResponse round-trip serialization
 * - Serialized size validation
 * - Little-endian encoding verification
 * - Buffer overflow protection (truncated data)
 * - All RailType and TerminalType values
 * - Energy validation error codes
 */

#include <sims3000/transport/RailNetworkMessages.h>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

using namespace sims3000::transport;

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n  FAILED: %s (line %d)\n", #condition, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_THROW(expr, exception_type) do { \
    bool caught = false; \
    try { expr; } catch (const exception_type&) { caught = true; } \
    if (!caught) { \
        printf("\n  FAILED: expected exception at line %d\n", __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// ============================================================================
// RailPlaceRequest Tests
// ============================================================================

TEST(rail_place_request_serialized_size) {
    ASSERT_EQ(RailPlaceRequest::SERIALIZED_SIZE, 10u);
}

TEST(rail_place_request_round_trip_defaults) {
    RailPlaceRequest original;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), RailPlaceRequest::SERIALIZED_SIZE);

    auto deserialized = RailPlaceRequest::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.x, 0);
    ASSERT_EQ(deserialized.y, 0);
    ASSERT_EQ(static_cast<uint8_t>(deserialized.type), static_cast<uint8_t>(RailType::SurfaceRail));
    ASSERT_EQ(deserialized.owner, 0u);
}

TEST(rail_place_request_round_trip_custom) {
    RailPlaceRequest original;
    original.x = 100;
    original.y = -200;
    original.type = RailType::ElevatedRail;
    original.owner = 5;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    auto deserialized = RailPlaceRequest::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.x, 100);
    ASSERT_EQ(deserialized.y, -200);
    ASSERT_EQ(static_cast<uint8_t>(deserialized.type), static_cast<uint8_t>(RailType::ElevatedRail));
    ASSERT_EQ(deserialized.owner, 5u);
}

TEST(rail_place_request_all_rail_types) {
    RailType types[] = {
        RailType::SurfaceRail,
        RailType::ElevatedRail,
        RailType::SubterraRail
    };

    for (auto type : types) {
        RailPlaceRequest original;
        original.type = type;

        std::vector<uint8_t> buffer;
        original.serialize(buffer);

        auto deserialized = RailPlaceRequest::deserialize(buffer.data(), buffer.size());
        ASSERT_EQ(static_cast<uint8_t>(deserialized.type), static_cast<uint8_t>(type));
    }
}

TEST(rail_place_request_little_endian) {
    RailPlaceRequest original;
    original.x = 0xAABBCCDD;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    // x starts at offset 0
    ASSERT_EQ(buffer[0], 0xDDu); // LSB
    ASSERT_EQ(buffer[1], 0xCCu);
    ASSERT_EQ(buffer[2], 0xBBu);
    ASSERT_EQ(buffer[3], 0xAAu); // MSB
}

TEST(rail_place_request_buffer_too_small) {
    uint8_t small_buf[5] = {};
    ASSERT_THROW(RailPlaceRequest::deserialize(small_buf, 5), std::runtime_error);
}

// ============================================================================
// RailPlaceResponse Tests
// ============================================================================

TEST(rail_place_response_serialized_size) {
    ASSERT_EQ(RailPlaceResponse::SERIALIZED_SIZE, 6u);
}

TEST(rail_place_response_round_trip_success) {
    RailPlaceResponse original;
    original.success = true;
    original.entity_id = 7777;
    original.error_code = 0;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), RailPlaceResponse::SERIALIZED_SIZE);

    auto deserialized = RailPlaceResponse::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.success, true);
    ASSERT_EQ(deserialized.entity_id, 7777u);
    ASSERT_EQ(deserialized.error_code, 0u);
}

TEST(rail_place_response_energy_error) {
    RailPlaceResponse original;
    original.success = false;
    original.entity_id = 0;
    original.error_code = 4; // no_energy

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    auto deserialized = RailPlaceResponse::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.success, false);
    ASSERT_EQ(deserialized.entity_id, 0u);
    ASSERT_EQ(deserialized.error_code, 4u);
}

TEST(rail_place_response_all_error_codes) {
    for (uint8_t ec = 0; ec <= 4; ++ec) {
        RailPlaceResponse original;
        original.error_code = ec;

        std::vector<uint8_t> buffer;
        original.serialize(buffer);

        auto deserialized = RailPlaceResponse::deserialize(buffer.data(), buffer.size());
        ASSERT_EQ(deserialized.error_code, ec);
    }
}

TEST(rail_place_response_buffer_too_small) {
    uint8_t small_buf[3] = {};
    ASSERT_THROW(RailPlaceResponse::deserialize(small_buf, 3), std::runtime_error);
}

// ============================================================================
// RailDemolishRequest Tests
// ============================================================================

TEST(rail_demolish_request_serialized_size) {
    ASSERT_EQ(RailDemolishRequest::SERIALIZED_SIZE, 5u);
}

TEST(rail_demolish_request_round_trip) {
    RailDemolishRequest original;
    original.entity_id = 5555;
    original.owner = 3;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), RailDemolishRequest::SERIALIZED_SIZE);

    auto deserialized = RailDemolishRequest::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.entity_id, 5555u);
    ASSERT_EQ(deserialized.owner, 3u);
}

TEST(rail_demolish_request_defaults) {
    RailDemolishRequest original;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    auto deserialized = RailDemolishRequest::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.entity_id, 0u);
    ASSERT_EQ(deserialized.owner, 0u);
}

TEST(rail_demolish_request_buffer_too_small) {
    uint8_t small_buf[2] = {};
    ASSERT_THROW(RailDemolishRequest::deserialize(small_buf, 2), std::runtime_error);
}

// ============================================================================
// RailDemolishResponse Tests
// ============================================================================

TEST(rail_demolish_response_serialized_size) {
    ASSERT_EQ(RailDemolishResponse::SERIALIZED_SIZE, 6u);
}

TEST(rail_demolish_response_round_trip_success) {
    RailDemolishResponse original;
    original.success = true;
    original.entity_id = 100;
    original.error_code = 0;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), RailDemolishResponse::SERIALIZED_SIZE);

    auto deserialized = RailDemolishResponse::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.success, true);
    ASSERT_EQ(deserialized.entity_id, 100u);
    ASSERT_EQ(deserialized.error_code, 0u);
}

TEST(rail_demolish_response_round_trip_failure) {
    RailDemolishResponse original;
    original.success = false;
    original.entity_id = 100;
    original.error_code = 2; // not_owner

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    auto deserialized = RailDemolishResponse::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.success, false);
    ASSERT_EQ(deserialized.entity_id, 100u);
    ASSERT_EQ(deserialized.error_code, 2u);
}

TEST(rail_demolish_response_buffer_too_small) {
    uint8_t small_buf[3] = {};
    ASSERT_THROW(RailDemolishResponse::deserialize(small_buf, 3), std::runtime_error);
}

// ============================================================================
// TerminalPlaceRequest Tests
// ============================================================================

TEST(terminal_place_request_serialized_size) {
    ASSERT_EQ(TerminalPlaceRequest::SERIALIZED_SIZE, 10u);
}

TEST(terminal_place_request_round_trip_defaults) {
    TerminalPlaceRequest original;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), TerminalPlaceRequest::SERIALIZED_SIZE);

    auto deserialized = TerminalPlaceRequest::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.x, 0);
    ASSERT_EQ(deserialized.y, 0);
    ASSERT_EQ(static_cast<uint8_t>(deserialized.type), static_cast<uint8_t>(TerminalType::SurfaceStation));
    ASSERT_EQ(deserialized.owner, 0u);
}

TEST(terminal_place_request_round_trip_custom) {
    TerminalPlaceRequest original;
    original.x = 50;
    original.y = -75;
    original.type = TerminalType::IntermodalHub;
    original.owner = 7;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    auto deserialized = TerminalPlaceRequest::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.x, 50);
    ASSERT_EQ(deserialized.y, -75);
    ASSERT_EQ(static_cast<uint8_t>(deserialized.type), static_cast<uint8_t>(TerminalType::IntermodalHub));
    ASSERT_EQ(deserialized.owner, 7u);
}

TEST(terminal_place_request_all_terminal_types) {
    TerminalType types[] = {
        TerminalType::SurfaceStation,
        TerminalType::SubterraStation,
        TerminalType::IntermodalHub
    };

    for (auto type : types) {
        TerminalPlaceRequest original;
        original.type = type;

        std::vector<uint8_t> buffer;
        original.serialize(buffer);

        auto deserialized = TerminalPlaceRequest::deserialize(buffer.data(), buffer.size());
        ASSERT_EQ(static_cast<uint8_t>(deserialized.type), static_cast<uint8_t>(type));
    }
}

TEST(terminal_place_request_buffer_too_small) {
    uint8_t small_buf[5] = {};
    ASSERT_THROW(TerminalPlaceRequest::deserialize(small_buf, 5), std::runtime_error);
}

// ============================================================================
// TerminalPlaceResponse Tests
// ============================================================================

TEST(terminal_place_response_serialized_size) {
    ASSERT_EQ(TerminalPlaceResponse::SERIALIZED_SIZE, 6u);
}

TEST(terminal_place_response_round_trip_success) {
    TerminalPlaceResponse original;
    original.success = true;
    original.entity_id = 3333;
    original.error_code = 0;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), TerminalPlaceResponse::SERIALIZED_SIZE);

    auto deserialized = TerminalPlaceResponse::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.success, true);
    ASSERT_EQ(deserialized.entity_id, 3333u);
    ASSERT_EQ(deserialized.error_code, 0u);
}

TEST(terminal_place_response_energy_error) {
    TerminalPlaceResponse original;
    original.success = false;
    original.entity_id = 0;
    original.error_code = 4; // no_energy

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    auto deserialized = TerminalPlaceResponse::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.success, false);
    ASSERT_EQ(deserialized.entity_id, 0u);
    ASSERT_EQ(deserialized.error_code, 4u);
}

TEST(terminal_place_response_buffer_too_small) {
    uint8_t small_buf[3] = {};
    ASSERT_THROW(TerminalPlaceResponse::deserialize(small_buf, 3), std::runtime_error);
}

// ============================================================================
// TerminalDemolishRequest Tests
// ============================================================================

TEST(terminal_demolish_request_serialized_size) {
    ASSERT_EQ(TerminalDemolishRequest::SERIALIZED_SIZE, 5u);
}

TEST(terminal_demolish_request_round_trip) {
    TerminalDemolishRequest original;
    original.entity_id = 8888;
    original.owner = 4;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), TerminalDemolishRequest::SERIALIZED_SIZE);

    auto deserialized = TerminalDemolishRequest::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.entity_id, 8888u);
    ASSERT_EQ(deserialized.owner, 4u);
}

TEST(terminal_demolish_request_buffer_too_small) {
    uint8_t small_buf[2] = {};
    ASSERT_THROW(TerminalDemolishRequest::deserialize(small_buf, 2), std::runtime_error);
}

// ============================================================================
// TerminalDemolishResponse Tests
// ============================================================================

TEST(terminal_demolish_response_serialized_size) {
    ASSERT_EQ(TerminalDemolishResponse::SERIALIZED_SIZE, 6u);
}

TEST(terminal_demolish_response_round_trip_success) {
    TerminalDemolishResponse original;
    original.success = true;
    original.entity_id = 200;
    original.error_code = 0;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), TerminalDemolishResponse::SERIALIZED_SIZE);

    auto deserialized = TerminalDemolishResponse::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.success, true);
    ASSERT_EQ(deserialized.entity_id, 200u);
    ASSERT_EQ(deserialized.error_code, 0u);
}

TEST(terminal_demolish_response_round_trip_failure) {
    TerminalDemolishResponse original;
    original.success = false;
    original.entity_id = 200;
    original.error_code = 1; // not_found

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    auto deserialized = TerminalDemolishResponse::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.success, false);
    ASSERT_EQ(deserialized.entity_id, 200u);
    ASSERT_EQ(deserialized.error_code, 1u);
}

TEST(terminal_demolish_response_buffer_too_small) {
    uint8_t small_buf[3] = {};
    ASSERT_THROW(TerminalDemolishResponse::deserialize(small_buf, 3), std::runtime_error);
}

// ============================================================================
// Cross-message Tests
// ============================================================================

TEST(rail_and_terminal_in_same_buffer) {
    RailPlaceRequest rail_req;
    rail_req.x = 10;
    rail_req.y = 20;
    rail_req.type = RailType::SubterraRail;
    rail_req.owner = 1;

    TerminalPlaceRequest term_req;
    term_req.x = 10;
    term_req.y = 20;
    term_req.type = TerminalType::SubterraStation;
    term_req.owner = 1;

    std::vector<uint8_t> buffer;
    rail_req.serialize(buffer);
    term_req.serialize(buffer);

    ASSERT_EQ(buffer.size(), RailPlaceRequest::SERIALIZED_SIZE + TerminalPlaceRequest::SERIALIZED_SIZE);

    auto rail_out = RailPlaceRequest::deserialize(buffer.data(), buffer.size());
    ASSERT_EQ(rail_out.x, 10);
    ASSERT_EQ(rail_out.y, 20);
    ASSERT_EQ(static_cast<uint8_t>(rail_out.type), static_cast<uint8_t>(RailType::SubterraRail));

    auto term_out = TerminalPlaceRequest::deserialize(
        buffer.data() + RailPlaceRequest::SERIALIZED_SIZE,
        buffer.size() - RailPlaceRequest::SERIALIZED_SIZE);
    ASSERT_EQ(term_out.x, 10);
    ASSERT_EQ(term_out.y, 20);
    ASSERT_EQ(static_cast<uint8_t>(term_out.type), static_cast<uint8_t>(TerminalType::SubterraStation));
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== RailNetworkMessages Unit Tests (Ticket E7-039) ===\n\n");

    // RailPlaceRequest
    RUN_TEST(rail_place_request_serialized_size);
    RUN_TEST(rail_place_request_round_trip_defaults);
    RUN_TEST(rail_place_request_round_trip_custom);
    RUN_TEST(rail_place_request_all_rail_types);
    RUN_TEST(rail_place_request_little_endian);
    RUN_TEST(rail_place_request_buffer_too_small);

    // RailPlaceResponse
    RUN_TEST(rail_place_response_serialized_size);
    RUN_TEST(rail_place_response_round_trip_success);
    RUN_TEST(rail_place_response_energy_error);
    RUN_TEST(rail_place_response_all_error_codes);
    RUN_TEST(rail_place_response_buffer_too_small);

    // RailDemolishRequest
    RUN_TEST(rail_demolish_request_serialized_size);
    RUN_TEST(rail_demolish_request_round_trip);
    RUN_TEST(rail_demolish_request_defaults);
    RUN_TEST(rail_demolish_request_buffer_too_small);

    // RailDemolishResponse
    RUN_TEST(rail_demolish_response_serialized_size);
    RUN_TEST(rail_demolish_response_round_trip_success);
    RUN_TEST(rail_demolish_response_round_trip_failure);
    RUN_TEST(rail_demolish_response_buffer_too_small);

    // TerminalPlaceRequest
    RUN_TEST(terminal_place_request_serialized_size);
    RUN_TEST(terminal_place_request_round_trip_defaults);
    RUN_TEST(terminal_place_request_round_trip_custom);
    RUN_TEST(terminal_place_request_all_terminal_types);
    RUN_TEST(terminal_place_request_buffer_too_small);

    // TerminalPlaceResponse
    RUN_TEST(terminal_place_response_serialized_size);
    RUN_TEST(terminal_place_response_round_trip_success);
    RUN_TEST(terminal_place_response_energy_error);
    RUN_TEST(terminal_place_response_buffer_too_small);

    // TerminalDemolishRequest
    RUN_TEST(terminal_demolish_request_serialized_size);
    RUN_TEST(terminal_demolish_request_round_trip);
    RUN_TEST(terminal_demolish_request_buffer_too_small);

    // TerminalDemolishResponse
    RUN_TEST(terminal_demolish_response_serialized_size);
    RUN_TEST(terminal_demolish_response_round_trip_success);
    RUN_TEST(terminal_demolish_response_round_trip_failure);
    RUN_TEST(terminal_demolish_response_buffer_too_small);

    // Cross-message
    RUN_TEST(rail_and_terminal_in_same_buffer);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
