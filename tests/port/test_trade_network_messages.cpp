/**
 * @file test_trade_network_messages.cpp
 * @brief Unit tests for trade network messages (Epic 8, Ticket E8-026)
 *
 * Tests cover:
 * - All 6 message types: serialize + deserialize roundtrip
 * - Default constructor values
 * - Serialized size constants
 * - Buffer too small throws exception
 * - Field value preservation across serialization
 * - TradeMessageType enum values
 */

#include <sims3000/port/TradeNetworkMessages.h>
#include <sims3000/port/PortTypes.h>
#include <cassert>
#include <cstdio>
#include <stdexcept>

using namespace sims3000::port;

// =============================================================================
// Test: TradeMessageType enum values
// =============================================================================

void test_message_type_enum() {
    printf("Testing TradeMessageType enum values...\n");

    assert(static_cast<uint8_t>(TradeMessageType::OfferRequest) == 0);
    assert(static_cast<uint8_t>(TradeMessageType::OfferResponse) == 1);
    assert(static_cast<uint8_t>(TradeMessageType::CancelRequest) == 2);
    assert(static_cast<uint8_t>(TradeMessageType::OfferNotification) == 3);
    assert(static_cast<uint8_t>(TradeMessageType::RouteEstablished) == 4);
    assert(static_cast<uint8_t>(TradeMessageType::RouteCancelled) == 5);

    printf("  PASS: TradeMessageType enum values correct\n");
}

// =============================================================================
// Test: TradeOfferRequestMsg default values
// =============================================================================

void test_offer_request_defaults() {
    printf("Testing TradeOfferRequestMsg default values...\n");

    TradeOfferRequestMsg msg;
    assert(msg.target_player == 0);
    assert(msg.proposed_type == 0);

    printf("  PASS: TradeOfferRequestMsg defaults correct\n");
}

// =============================================================================
// Test: TradeOfferRequestMsg serialize/deserialize roundtrip
// =============================================================================

void test_offer_request_roundtrip() {
    printf("Testing TradeOfferRequestMsg serialize/deserialize...\n");

    TradeOfferRequestMsg original;
    original.target_player = 3;
    original.proposed_type = static_cast<uint8_t>(TradeAgreementType::Enhanced);

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    assert(buffer.size() == TradeOfferRequestMsg::SERIALIZED_SIZE);
    assert(buffer.size() == 2);

    TradeOfferRequestMsg deserialized =
        TradeOfferRequestMsg::deserialize(buffer.data(), buffer.size());

    assert(deserialized.target_player == 3);
    assert(deserialized.proposed_type == static_cast<uint8_t>(TradeAgreementType::Enhanced));

    printf("  PASS: TradeOfferRequestMsg roundtrip correct\n");
}

// =============================================================================
// Test: TradeOfferRequestMsg deserialize buffer too small
// =============================================================================

void test_offer_request_too_small() {
    printf("Testing TradeOfferRequestMsg deserialize with small buffer...\n");

    uint8_t data[1] = { 0 };
    bool threw = false;
    try {
        TradeOfferRequestMsg::deserialize(data, 1);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    printf("  PASS: Small buffer throws runtime_error\n");
}

// =============================================================================
// Test: TradeOfferResponseMsg default values
// =============================================================================

void test_offer_response_defaults() {
    printf("Testing TradeOfferResponseMsg default values...\n");

    TradeOfferResponseMsg msg;
    assert(msg.offer_id == 0);
    assert(msg.accepted == false);

    printf("  PASS: TradeOfferResponseMsg defaults correct\n");
}

// =============================================================================
// Test: TradeOfferResponseMsg serialize/deserialize roundtrip - accepted
// =============================================================================

void test_offer_response_roundtrip_accepted() {
    printf("Testing TradeOfferResponseMsg roundtrip (accepted)...\n");

    TradeOfferResponseMsg original;
    original.offer_id = 12345;
    original.accepted = true;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    assert(buffer.size() == TradeOfferResponseMsg::SERIALIZED_SIZE);
    assert(buffer.size() == 5);

    TradeOfferResponseMsg deserialized =
        TradeOfferResponseMsg::deserialize(buffer.data(), buffer.size());

    assert(deserialized.offer_id == 12345);
    assert(deserialized.accepted == true);

    printf("  PASS: Accepted response roundtrip correct\n");
}

// =============================================================================
// Test: TradeOfferResponseMsg serialize/deserialize roundtrip - rejected
// =============================================================================

void test_offer_response_roundtrip_rejected() {
    printf("Testing TradeOfferResponseMsg roundtrip (rejected)...\n");

    TradeOfferResponseMsg original;
    original.offer_id = 99;
    original.accepted = false;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    TradeOfferResponseMsg deserialized =
        TradeOfferResponseMsg::deserialize(buffer.data(), buffer.size());

    assert(deserialized.offer_id == 99);
    assert(deserialized.accepted == false);

    printf("  PASS: Rejected response roundtrip correct\n");
}

// =============================================================================
// Test: TradeOfferResponseMsg deserialize buffer too small
// =============================================================================

void test_offer_response_too_small() {
    printf("Testing TradeOfferResponseMsg deserialize with small buffer...\n");

    uint8_t data[4] = { 0 };
    bool threw = false;
    try {
        TradeOfferResponseMsg::deserialize(data, 4);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    printf("  PASS: Small buffer throws runtime_error\n");
}

// =============================================================================
// Test: TradeCancelRequestMsg default values
// =============================================================================

void test_cancel_request_defaults() {
    printf("Testing TradeCancelRequestMsg default values...\n");

    TradeCancelRequestMsg msg;
    assert(msg.route_id == 0);

    printf("  PASS: TradeCancelRequestMsg defaults correct\n");
}

// =============================================================================
// Test: TradeCancelRequestMsg serialize/deserialize roundtrip
// =============================================================================

void test_cancel_request_roundtrip() {
    printf("Testing TradeCancelRequestMsg serialize/deserialize...\n");

    TradeCancelRequestMsg original;
    original.route_id = 0xDEADBEEF;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    assert(buffer.size() == TradeCancelRequestMsg::SERIALIZED_SIZE);
    assert(buffer.size() == 4);

    TradeCancelRequestMsg deserialized =
        TradeCancelRequestMsg::deserialize(buffer.data(), buffer.size());

    assert(deserialized.route_id == 0xDEADBEEF);

    printf("  PASS: TradeCancelRequestMsg roundtrip correct\n");
}

// =============================================================================
// Test: TradeCancelRequestMsg deserialize buffer too small
// =============================================================================

void test_cancel_request_too_small() {
    printf("Testing TradeCancelRequestMsg deserialize with small buffer...\n");

    uint8_t data[3] = { 0 };
    bool threw = false;
    try {
        TradeCancelRequestMsg::deserialize(data, 3);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    printf("  PASS: Small buffer throws runtime_error\n");
}

// =============================================================================
// Test: TradeOfferNotificationMsg default values
// =============================================================================

void test_offer_notification_defaults() {
    printf("Testing TradeOfferNotificationMsg default values...\n");

    TradeOfferNotificationMsg msg;
    assert(msg.offer_id == 0);
    assert(msg.from_player == 0);
    assert(msg.proposed_type == 0);

    printf("  PASS: TradeOfferNotificationMsg defaults correct\n");
}

// =============================================================================
// Test: TradeOfferNotificationMsg serialize/deserialize roundtrip
// =============================================================================

void test_offer_notification_roundtrip() {
    printf("Testing TradeOfferNotificationMsg serialize/deserialize...\n");

    TradeOfferNotificationMsg original;
    original.offer_id = 42;
    original.from_player = 2;
    original.proposed_type = static_cast<uint8_t>(TradeAgreementType::Premium);

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    assert(buffer.size() == TradeOfferNotificationMsg::SERIALIZED_SIZE);
    assert(buffer.size() == 6);

    TradeOfferNotificationMsg deserialized =
        TradeOfferNotificationMsg::deserialize(buffer.data(), buffer.size());

    assert(deserialized.offer_id == 42);
    assert(deserialized.from_player == 2);
    assert(deserialized.proposed_type == static_cast<uint8_t>(TradeAgreementType::Premium));

    printf("  PASS: TradeOfferNotificationMsg roundtrip correct\n");
}

// =============================================================================
// Test: TradeOfferNotificationMsg deserialize buffer too small
// =============================================================================

void test_offer_notification_too_small() {
    printf("Testing TradeOfferNotificationMsg deserialize with small buffer...\n");

    uint8_t data[5] = { 0 };
    bool threw = false;
    try {
        TradeOfferNotificationMsg::deserialize(data, 5);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    printf("  PASS: Small buffer throws runtime_error\n");
}

// =============================================================================
// Test: TradeRouteEstablishedMsg default values
// =============================================================================

void test_route_established_defaults() {
    printf("Testing TradeRouteEstablishedMsg default values...\n");

    TradeRouteEstablishedMsg msg;
    assert(msg.route_id == 0);
    assert(msg.party_a == 0);
    assert(msg.party_b == 0);
    assert(msg.agreement_type == 0);

    printf("  PASS: TradeRouteEstablishedMsg defaults correct\n");
}

// =============================================================================
// Test: TradeRouteEstablishedMsg serialize/deserialize roundtrip
// =============================================================================

void test_route_established_roundtrip() {
    printf("Testing TradeRouteEstablishedMsg serialize/deserialize...\n");

    TradeRouteEstablishedMsg original;
    original.route_id = 7777;
    original.party_a = 1;
    original.party_b = 4;
    original.agreement_type = static_cast<uint8_t>(TradeAgreementType::Enhanced);

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    assert(buffer.size() == TradeRouteEstablishedMsg::SERIALIZED_SIZE);
    assert(buffer.size() == 7);

    TradeRouteEstablishedMsg deserialized =
        TradeRouteEstablishedMsg::deserialize(buffer.data(), buffer.size());

    assert(deserialized.route_id == 7777);
    assert(deserialized.party_a == 1);
    assert(deserialized.party_b == 4);
    assert(deserialized.agreement_type == static_cast<uint8_t>(TradeAgreementType::Enhanced));

    printf("  PASS: TradeRouteEstablishedMsg roundtrip correct\n");
}

// =============================================================================
// Test: TradeRouteEstablishedMsg deserialize buffer too small
// =============================================================================

void test_route_established_too_small() {
    printf("Testing TradeRouteEstablishedMsg deserialize with small buffer...\n");

    uint8_t data[6] = { 0 };
    bool threw = false;
    try {
        TradeRouteEstablishedMsg::deserialize(data, 6);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    printf("  PASS: Small buffer throws runtime_error\n");
}

// =============================================================================
// Test: TradeRouteCancelledMsg default values
// =============================================================================

void test_route_cancelled_defaults() {
    printf("Testing TradeRouteCancelledMsg default values...\n");

    TradeRouteCancelledMsg msg;
    assert(msg.route_id == 0);
    assert(msg.cancelled_by == 0);

    printf("  PASS: TradeRouteCancelledMsg defaults correct\n");
}

// =============================================================================
// Test: TradeRouteCancelledMsg serialize/deserialize roundtrip
// =============================================================================

void test_route_cancelled_roundtrip() {
    printf("Testing TradeRouteCancelledMsg serialize/deserialize...\n");

    TradeRouteCancelledMsg original;
    original.route_id = 54321;
    original.cancelled_by = 3;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    assert(buffer.size() == TradeRouteCancelledMsg::SERIALIZED_SIZE);
    assert(buffer.size() == 5);

    TradeRouteCancelledMsg deserialized =
        TradeRouteCancelledMsg::deserialize(buffer.data(), buffer.size());

    assert(deserialized.route_id == 54321);
    assert(deserialized.cancelled_by == 3);

    printf("  PASS: TradeRouteCancelledMsg roundtrip correct\n");
}

// =============================================================================
// Test: TradeRouteCancelledMsg - server-initiated cancellation (disconnection)
// =============================================================================

void test_route_cancelled_by_server() {
    printf("Testing TradeRouteCancelledMsg with server cancellation (0)...\n");

    TradeRouteCancelledMsg original;
    original.route_id = 100;
    original.cancelled_by = 0;  // 0 = server/disconnect

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    TradeRouteCancelledMsg deserialized =
        TradeRouteCancelledMsg::deserialize(buffer.data(), buffer.size());

    assert(deserialized.cancelled_by == 0);

    printf("  PASS: Server cancellation (0) serialized correctly\n");
}

// =============================================================================
// Test: TradeRouteCancelledMsg deserialize buffer too small
// =============================================================================

void test_route_cancelled_too_small() {
    printf("Testing TradeRouteCancelledMsg deserialize with small buffer...\n");

    uint8_t data[4] = { 0 };
    bool threw = false;
    try {
        TradeRouteCancelledMsg::deserialize(data, 4);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    printf("  PASS: Small buffer throws runtime_error\n");
}

// =============================================================================
// Test: Serialized size constants
// =============================================================================

void test_serialized_sizes() {
    printf("Testing SERIALIZED_SIZE constants...\n");

    assert(TradeOfferRequestMsg::SERIALIZED_SIZE == 2);
    assert(TradeOfferResponseMsg::SERIALIZED_SIZE == 5);
    assert(TradeCancelRequestMsg::SERIALIZED_SIZE == 4);
    assert(TradeOfferNotificationMsg::SERIALIZED_SIZE == 6);
    assert(TradeRouteEstablishedMsg::SERIALIZED_SIZE == 7);
    assert(TradeRouteCancelledMsg::SERIALIZED_SIZE == 5);

    printf("  PASS: All SERIALIZED_SIZE constants correct\n");
}

// =============================================================================
// Test: Large offer_id value (uint32_t max)
// =============================================================================

void test_large_offer_id() {
    printf("Testing large offer_id serialization (0xFFFFFFFF)...\n");

    TradeOfferResponseMsg original;
    original.offer_id = 0xFFFFFFFF;
    original.accepted = true;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    TradeOfferResponseMsg deserialized =
        TradeOfferResponseMsg::deserialize(buffer.data(), buffer.size());

    assert(deserialized.offer_id == 0xFFFFFFFF);

    printf("  PASS: Large offer_id preserved\n");
}

// =============================================================================
// Test: Multiple messages in sequence
// =============================================================================

void test_sequential_serialization() {
    printf("Testing sequential message serialization...\n");

    std::vector<uint8_t> buffer;

    TradeOfferRequestMsg req;
    req.target_player = 2;
    req.proposed_type = 1;
    req.serialize(buffer);

    TradeOfferNotificationMsg notif;
    notif.offer_id = 10;
    notif.from_player = 1;
    notif.proposed_type = 1;
    notif.serialize(buffer);

    assert(buffer.size() ==
        TradeOfferRequestMsg::SERIALIZED_SIZE + TradeOfferNotificationMsg::SERIALIZED_SIZE);

    // Deserialize first message
    TradeOfferRequestMsg req2 =
        TradeOfferRequestMsg::deserialize(buffer.data(), TradeOfferRequestMsg::SERIALIZED_SIZE);
    assert(req2.target_player == 2);

    // Deserialize second message from offset
    TradeOfferNotificationMsg notif2 =
        TradeOfferNotificationMsg::deserialize(
            buffer.data() + TradeOfferRequestMsg::SERIALIZED_SIZE,
            TradeOfferNotificationMsg::SERIALIZED_SIZE);
    assert(notif2.offer_id == 10);
    assert(notif2.from_player == 1);

    printf("  PASS: Sequential serialization works correctly\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Trade Network Messages Tests (Epic 8, Ticket E8-026) ===\n\n");

    // Enum tests
    test_message_type_enum();

    // TradeOfferRequestMsg tests
    test_offer_request_defaults();
    test_offer_request_roundtrip();
    test_offer_request_too_small();

    // TradeOfferResponseMsg tests
    test_offer_response_defaults();
    test_offer_response_roundtrip_accepted();
    test_offer_response_roundtrip_rejected();
    test_offer_response_too_small();

    // TradeCancelRequestMsg tests
    test_cancel_request_defaults();
    test_cancel_request_roundtrip();
    test_cancel_request_too_small();

    // TradeOfferNotificationMsg tests
    test_offer_notification_defaults();
    test_offer_notification_roundtrip();
    test_offer_notification_too_small();

    // TradeRouteEstablishedMsg tests
    test_route_established_defaults();
    test_route_established_roundtrip();
    test_route_established_too_small();

    // TradeRouteCancelledMsg tests
    test_route_cancelled_defaults();
    test_route_cancelled_roundtrip();
    test_route_cancelled_by_server();
    test_route_cancelled_too_small();

    // General tests
    test_serialized_sizes();
    test_large_offer_id();
    test_sequential_serialization();

    printf("\n=== All Trade Network Messages Tests Passed ===\n");
    return 0;
}
