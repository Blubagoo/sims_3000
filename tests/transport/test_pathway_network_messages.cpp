/**
 * @file test_pathway_network_messages.cpp
 * @brief Tests for pathway network message serialization (Ticket E7-038)
 *
 * Tests cover:
 * - PathwayPlaceRequest round-trip serialization
 * - PathwayPlaceResponse round-trip serialization
 * - PathwayDemolishRequest round-trip serialization
 * - PathwayDemolishResponse round-trip serialization
 * - Serialized size validation
 * - Little-endian encoding verification
 * - Buffer overflow protection (truncated data)
 * - Negative coordinate handling
 * - All PathwayType values
 * - Error code values
 */

#include <sims3000/transport/PathwayNetworkMessages.h>
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
// PathwayPlaceRequest Tests
// ============================================================================

TEST(place_request_serialized_size) {
    ASSERT_EQ(PathwayPlaceRequest::SERIALIZED_SIZE, 10u);
}

TEST(place_request_round_trip_defaults) {
    PathwayPlaceRequest original;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), PathwayPlaceRequest::SERIALIZED_SIZE);

    auto deserialized = PathwayPlaceRequest::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.x, 0);
    ASSERT_EQ(deserialized.y, 0);
    ASSERT_EQ(static_cast<uint8_t>(deserialized.type), static_cast<uint8_t>(PathwayType::BasicPathway));
    ASSERT_EQ(deserialized.owner, 0u);
}

TEST(place_request_round_trip_custom_values) {
    PathwayPlaceRequest original;
    original.x = 42;
    original.y = -10;
    original.type = PathwayType::TransitCorridor;
    original.owner = 3;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), PathwayPlaceRequest::SERIALIZED_SIZE);

    auto deserialized = PathwayPlaceRequest::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.x, 42);
    ASSERT_EQ(deserialized.y, -10);
    ASSERT_EQ(static_cast<uint8_t>(deserialized.type), static_cast<uint8_t>(PathwayType::TransitCorridor));
    ASSERT_EQ(deserialized.owner, 3u);
}

TEST(place_request_all_pathway_types) {
    PathwayType types[] = {
        PathwayType::BasicPathway,
        PathwayType::TransitCorridor,
        PathwayType::Pedestrian,
        PathwayType::Bridge,
        PathwayType::Tunnel
    };

    for (auto type : types) {
        PathwayPlaceRequest original;
        original.type = type;

        std::vector<uint8_t> buffer;
        original.serialize(buffer);

        auto deserialized = PathwayPlaceRequest::deserialize(buffer.data(), buffer.size());
        ASSERT_EQ(static_cast<uint8_t>(deserialized.type), static_cast<uint8_t>(type));
    }
}

TEST(place_request_negative_coordinates) {
    PathwayPlaceRequest original;
    original.x = -2147483647;
    original.y = 2147483647;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    auto deserialized = PathwayPlaceRequest::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.x, -2147483647);
    ASSERT_EQ(deserialized.y, 2147483647);
}

TEST(place_request_buffer_too_small) {
    uint8_t small_buf[5] = {};
    ASSERT_THROW(PathwayPlaceRequest::deserialize(small_buf, 5), std::runtime_error);
}

TEST(place_request_little_endian) {
    PathwayPlaceRequest original;
    original.x = 0x12345678;
    original.y = 0;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    // x starts at offset 0
    ASSERT_EQ(buffer[0], 0x78u); // LSB
    ASSERT_EQ(buffer[1], 0x56u);
    ASSERT_EQ(buffer[2], 0x34u);
    ASSERT_EQ(buffer[3], 0x12u); // MSB
}

// ============================================================================
// PathwayPlaceResponse Tests
// ============================================================================

TEST(place_response_serialized_size) {
    ASSERT_EQ(PathwayPlaceResponse::SERIALIZED_SIZE, 14u);
}

TEST(place_response_round_trip_success) {
    PathwayPlaceResponse original;
    original.success = true;
    original.entity_id = 12345;
    original.x = 10;
    original.y = 20;
    original.error_code = 0;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), PathwayPlaceResponse::SERIALIZED_SIZE);

    auto deserialized = PathwayPlaceResponse::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.success, true);
    ASSERT_EQ(deserialized.entity_id, 12345u);
    ASSERT_EQ(deserialized.x, 10);
    ASSERT_EQ(deserialized.y, 20);
    ASSERT_EQ(deserialized.error_code, 0u);
}

TEST(place_response_round_trip_failure) {
    PathwayPlaceResponse original;
    original.success = false;
    original.entity_id = 0;
    original.x = 5;
    original.y = 5;
    original.error_code = 1; // occupied

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    auto deserialized = PathwayPlaceResponse::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.success, false);
    ASSERT_EQ(deserialized.entity_id, 0u);
    ASSERT_EQ(deserialized.x, 5);
    ASSERT_EQ(deserialized.y, 5);
    ASSERT_EQ(deserialized.error_code, 1u);
}

TEST(place_response_all_error_codes) {
    for (uint8_t ec = 0; ec <= 3; ++ec) {
        PathwayPlaceResponse original;
        original.error_code = ec;

        std::vector<uint8_t> buffer;
        original.serialize(buffer);

        auto deserialized = PathwayPlaceResponse::deserialize(buffer.data(), buffer.size());
        ASSERT_EQ(deserialized.error_code, ec);
    }
}

TEST(place_response_buffer_too_small) {
    uint8_t small_buf[8] = {};
    ASSERT_THROW(PathwayPlaceResponse::deserialize(small_buf, 8), std::runtime_error);
}

// ============================================================================
// PathwayDemolishRequest Tests
// ============================================================================

TEST(demolish_request_serialized_size) {
    ASSERT_EQ(PathwayDemolishRequest::SERIALIZED_SIZE, 13u);
}

TEST(demolish_request_round_trip) {
    PathwayDemolishRequest original;
    original.entity_id = 9999;
    original.x = -50;
    original.y = 100;
    original.owner = 2;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), PathwayDemolishRequest::SERIALIZED_SIZE);

    auto deserialized = PathwayDemolishRequest::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.entity_id, 9999u);
    ASSERT_EQ(deserialized.x, -50);
    ASSERT_EQ(deserialized.y, 100);
    ASSERT_EQ(deserialized.owner, 2u);
}

TEST(demolish_request_defaults) {
    PathwayDemolishRequest original;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    auto deserialized = PathwayDemolishRequest::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.entity_id, 0u);
    ASSERT_EQ(deserialized.x, 0);
    ASSERT_EQ(deserialized.y, 0);
    ASSERT_EQ(deserialized.owner, 0u);
}

TEST(demolish_request_buffer_too_small) {
    uint8_t small_buf[6] = {};
    ASSERT_THROW(PathwayDemolishRequest::deserialize(small_buf, 6), std::runtime_error);
}

// ============================================================================
// PathwayDemolishResponse Tests
// ============================================================================

TEST(demolish_response_serialized_size) {
    ASSERT_EQ(PathwayDemolishResponse::SERIALIZED_SIZE, 6u);
}

TEST(demolish_response_round_trip_success) {
    PathwayDemolishResponse original;
    original.success = true;
    original.entity_id = 42;
    original.error_code = 0;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    ASSERT_EQ(buffer.size(), PathwayDemolishResponse::SERIALIZED_SIZE);

    auto deserialized = PathwayDemolishResponse::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.success, true);
    ASSERT_EQ(deserialized.entity_id, 42u);
    ASSERT_EQ(deserialized.error_code, 0u);
}

TEST(demolish_response_round_trip_failure) {
    PathwayDemolishResponse original;
    original.success = false;
    original.entity_id = 42;
    original.error_code = 2; // not_owner

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    auto deserialized = PathwayDemolishResponse::deserialize(buffer.data(), buffer.size());

    ASSERT_EQ(deserialized.success, false);
    ASSERT_EQ(deserialized.entity_id, 42u);
    ASSERT_EQ(deserialized.error_code, 2u);
}

TEST(demolish_response_all_error_codes) {
    for (uint8_t ec = 0; ec <= 2; ++ec) {
        PathwayDemolishResponse original;
        original.error_code = ec;

        std::vector<uint8_t> buffer;
        original.serialize(buffer);

        auto deserialized = PathwayDemolishResponse::deserialize(buffer.data(), buffer.size());
        ASSERT_EQ(deserialized.error_code, ec);
    }
}

TEST(demolish_response_buffer_too_small) {
    uint8_t small_buf[3] = {};
    ASSERT_THROW(PathwayDemolishResponse::deserialize(small_buf, 3), std::runtime_error);
}

// ============================================================================
// Cross-message Tests
// ============================================================================

TEST(place_request_and_response_in_same_buffer) {
    PathwayPlaceRequest req;
    req.x = 10;
    req.y = 20;
    req.type = PathwayType::Bridge;
    req.owner = 1;

    PathwayPlaceResponse resp;
    resp.success = true;
    resp.entity_id = 555;
    resp.x = 10;
    resp.y = 20;
    resp.error_code = 0;

    std::vector<uint8_t> buffer;
    req.serialize(buffer);
    resp.serialize(buffer);

    ASSERT_EQ(buffer.size(), PathwayPlaceRequest::SERIALIZED_SIZE + PathwayPlaceResponse::SERIALIZED_SIZE);

    auto req_out = PathwayPlaceRequest::deserialize(buffer.data(), buffer.size());
    ASSERT_EQ(req_out.x, 10);
    ASSERT_EQ(req_out.y, 20);
    ASSERT_EQ(static_cast<uint8_t>(req_out.type), static_cast<uint8_t>(PathwayType::Bridge));
    ASSERT_EQ(req_out.owner, 1u);

    auto resp_out = PathwayPlaceResponse::deserialize(
        buffer.data() + PathwayPlaceRequest::SERIALIZED_SIZE,
        buffer.size() - PathwayPlaceRequest::SERIALIZED_SIZE);
    ASSERT_EQ(resp_out.success, true);
    ASSERT_EQ(resp_out.entity_id, 555u);
    ASSERT_EQ(resp_out.x, 10);
    ASSERT_EQ(resp_out.y, 20);
    ASSERT_EQ(resp_out.error_code, 0u);
}

TEST(demolish_request_and_response_in_same_buffer) {
    PathwayDemolishRequest req;
    req.entity_id = 42;
    req.x = -5;
    req.y = 10;
    req.owner = 2;

    PathwayDemolishResponse resp;
    resp.success = true;
    resp.entity_id = 42;
    resp.error_code = 0;

    std::vector<uint8_t> buffer;
    req.serialize(buffer);
    resp.serialize(buffer);

    ASSERT_EQ(buffer.size(), PathwayDemolishRequest::SERIALIZED_SIZE + PathwayDemolishResponse::SERIALIZED_SIZE);

    auto req_out = PathwayDemolishRequest::deserialize(buffer.data(), buffer.size());
    ASSERT_EQ(req_out.entity_id, 42u);
    ASSERT_EQ(req_out.x, -5);
    ASSERT_EQ(req_out.y, 10);
    ASSERT_EQ(req_out.owner, 2u);

    auto resp_out = PathwayDemolishResponse::deserialize(
        buffer.data() + PathwayDemolishRequest::SERIALIZED_SIZE,
        buffer.size() - PathwayDemolishRequest::SERIALIZED_SIZE);
    ASSERT_EQ(resp_out.success, true);
    ASSERT_EQ(resp_out.entity_id, 42u);
    ASSERT_EQ(resp_out.error_code, 0u);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== PathwayNetworkMessages Unit Tests (Ticket E7-038) ===\n\n");

    // PathwayPlaceRequest
    RUN_TEST(place_request_serialized_size);
    RUN_TEST(place_request_round_trip_defaults);
    RUN_TEST(place_request_round_trip_custom_values);
    RUN_TEST(place_request_all_pathway_types);
    RUN_TEST(place_request_negative_coordinates);
    RUN_TEST(place_request_buffer_too_small);
    RUN_TEST(place_request_little_endian);

    // PathwayPlaceResponse
    RUN_TEST(place_response_serialized_size);
    RUN_TEST(place_response_round_trip_success);
    RUN_TEST(place_response_round_trip_failure);
    RUN_TEST(place_response_all_error_codes);
    RUN_TEST(place_response_buffer_too_small);

    // PathwayDemolishRequest
    RUN_TEST(demolish_request_serialized_size);
    RUN_TEST(demolish_request_round_trip);
    RUN_TEST(demolish_request_defaults);
    RUN_TEST(demolish_request_buffer_too_small);

    // PathwayDemolishResponse
    RUN_TEST(demolish_response_serialized_size);
    RUN_TEST(demolish_response_round_trip_success);
    RUN_TEST(demolish_response_round_trip_failure);
    RUN_TEST(demolish_response_all_error_codes);
    RUN_TEST(demolish_response_buffer_too_small);

    // Cross-message
    RUN_TEST(place_request_and_response_in_same_buffer);
    RUN_TEST(demolish_request_and_response_in_same_buffer);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
