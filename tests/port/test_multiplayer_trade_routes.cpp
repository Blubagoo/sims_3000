/**
 * @file test_multiplayer_trade_routes.cpp
 * @brief Multiplayer integration tests for inter-player trade routes (Ticket E8-039)
 *
 * Tests cover:
 * - Trade offers sync correctly (create, accept, reject)
 * - Trade acceptance creates agreement
 * - Trade cancellation handled correctly
 * - Disconnection during trade handled (expired offers)
 * - Network message serialization round-trips for all trade messages
 * - Full trade lifecycle: offer -> accept -> agreement -> benefits
 * - Trade agreement benefits (demand bonus and income bonus)
 */

#include <sims3000/port/TradeOfferManager.h>
#include <sims3000/port/TradeNetworkMessages.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <sims3000/port/TradeAgreementBenefits.h>
#include <sims3000/port/TradeDealNegotiation.h>
#include <sims3000/port/TradeDealExpiration.h>
#include <sims3000/port/TradeEvents.h>
#include <sims3000/port/PortTypes.h>
#include <sims3000/port/PortSystem.h>
#include <sims3000/port/DemandBonus.h>
#include <sims3000/port/TradeIncome.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <stdexcept>
#include <vector>

using namespace sims3000::port;

// =============================================================================
// Helpers
// =============================================================================

static int tests_passed = 0;
static int tests_total = 0;

#define TEST(name) \
    do { \
        tests_total++; \
        printf("  TEST: %s ... ", name); \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while(0)

static bool approx(float a, float b, float eps = 0.01f) {
    return std::fabs(a - b) < eps;
}

// =============================================================================
// Trade Offer Sync Tests: Create
// =============================================================================

static void test_offer_create_sync() {
    TEST("Offer create: valid offer created with correct fields");
    TradeOfferManager manager;

    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 100);
    assert(id != 0);

    const TradeOffer* offer = manager.get_offer(id);
    assert(offer != nullptr);
    assert(offer->from_player == 1);
    assert(offer->to_player == 2);
    assert(offer->proposed_type == TradeAgreementType::Basic);
    assert(offer->is_pending == true);
    assert(offer->created_tick == 100);
    assert(offer->expiry_tick == 100 + TRADE_OFFER_EXPIRY_TICKS);
    PASS();
}

static void test_offer_create_all_tiers() {
    TEST("Offer create: all agreement tiers can be proposed");
    TradeOfferManager manager;

    uint32_t id_basic = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    uint32_t id_enhanced = manager.create_offer(1, 3, TradeAgreementType::Enhanced, 0);
    uint32_t id_premium = manager.create_offer(1, 4, TradeAgreementType::Premium, 0);

    assert(id_basic != 0);
    assert(id_enhanced != 0);
    assert(id_premium != 0);

    assert(manager.get_offer(id_basic)->proposed_type == TradeAgreementType::Basic);
    assert(manager.get_offer(id_enhanced)->proposed_type == TradeAgreementType::Enhanced);
    assert(manager.get_offer(id_premium)->proposed_type == TradeAgreementType::Premium);
    PASS();
}

static void test_offer_create_multiple_senders_to_one_target() {
    TEST("Offer create: multiple senders can target same player");
    TradeOfferManager manager;

    uint32_t id1 = manager.create_offer(1, 4, TradeAgreementType::Basic, 0);
    uint32_t id2 = manager.create_offer(2, 4, TradeAgreementType::Enhanced, 0);
    uint32_t id3 = manager.create_offer(3, 4, TradeAgreementType::Premium, 0);

    assert(id1 != 0 && id2 != 0 && id3 != 0);

    auto pending = manager.get_pending_offers_for(4);
    assert(pending.size() == 3);
    PASS();
}

static void test_offer_create_fails_none_type() {
    TEST("Offer create: rejected for None type");
    TradeOfferManager manager;

    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::None, 0);
    assert(id == 0);
    assert(manager.get_offer_count() == 0);
    PASS();
}

static void test_offer_create_fails_self_trade() {
    TEST("Offer create: rejected for self-trade");
    TradeOfferManager manager;

    uint32_t id = manager.create_offer(1, 1, TradeAgreementType::Basic, 0);
    assert(id == 0);
    PASS();
}

static void test_offer_create_fails_game_master() {
    TEST("Offer create: rejected for GAME_MASTER (ID 0)");
    TradeOfferManager manager;

    uint32_t id_from = manager.create_offer(0, 2, TradeAgreementType::Basic, 0);
    uint32_t id_to = manager.create_offer(1, 0, TradeAgreementType::Basic, 0);
    assert(id_from == 0);
    assert(id_to == 0);
    PASS();
}

static void test_offer_create_fails_duplicate_pending() {
    TEST("Offer create: rejected for duplicate pending offer");
    TradeOfferManager manager;

    uint32_t id1 = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    assert(id1 != 0);

    uint32_t id2 = manager.create_offer(1, 2, TradeAgreementType::Premium, 0);
    assert(id2 == 0);
    PASS();
}

// =============================================================================
// Trade Offer Sync Tests: Accept
// =============================================================================

static void test_offer_accept_sync() {
    TEST("Offer accept: valid acceptance marks offer as not pending");
    TradeOfferManager manager;

    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Enhanced, 100);
    bool accepted = manager.accept_offer(id, 200);

    assert(accepted == true);
    assert(manager.get_offer(id)->is_pending == false);
    PASS();
}

static void test_offer_accept_creates_agreement_data() {
    TEST("Offer accept: accepted offer provides data for agreement creation");
    TradeOfferManager manager;

    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Premium, 100);
    bool accepted = manager.accept_offer(id, 200);
    assert(accepted);

    const TradeOffer* offer = manager.get_offer(id);
    assert(offer != nullptr);

    // Verify the offer data can be used to create a TradeAgreementComponent
    TradeAgreementComponent agreement;
    agreement.party_a = offer->from_player;
    agreement.party_b = offer->to_player;
    agreement.agreement_type = offer->proposed_type;
    agreement.cycles_remaining = 1000;

    assert(agreement.party_a == 1);
    assert(agreement.party_b == 2);
    assert(agreement.agreement_type == TradeAgreementType::Premium);
    PASS();
}

static void test_offer_accept_fails_expired() {
    TEST("Offer accept: fails for expired offer");
    TradeOfferManager manager;

    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 100);
    // Offer expires at tick 600 (100 + 500)
    bool accepted = manager.accept_offer(id, 600);
    assert(accepted == false);
    PASS();
}

static void test_offer_accept_fails_already_accepted() {
    TEST("Offer accept: fails for already accepted offer");
    TradeOfferManager manager;

    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    assert(manager.accept_offer(id, 100) == true);
    assert(manager.accept_offer(id, 100) == false);
    PASS();
}

static void test_offer_accept_fails_nonexistent() {
    TEST("Offer accept: fails for nonexistent offer ID");
    TradeOfferManager manager;

    assert(manager.accept_offer(9999, 0) == false);
    PASS();
}

// =============================================================================
// Trade Offer Sync Tests: Reject
// =============================================================================

static void test_offer_reject_sync() {
    TEST("Offer reject: valid rejection marks offer as not pending");
    TradeOfferManager manager;

    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    bool rejected = manager.reject_offer(id);

    assert(rejected == true);
    assert(manager.get_offer(id)->is_pending == false);
    PASS();
}

static void test_offer_reject_allows_new_offer() {
    TEST("Offer reject: allows new offer to same target after rejection");
    TradeOfferManager manager;

    uint32_t id1 = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    manager.reject_offer(id1);

    uint32_t id2 = manager.create_offer(1, 2, TradeAgreementType::Enhanced, 10);
    assert(id2 != 0);
    assert(id2 != id1);
    PASS();
}

static void test_offer_reject_fails_already_rejected() {
    TEST("Offer reject: fails for already rejected offer");
    TradeOfferManager manager;

    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    assert(manager.reject_offer(id) == true);
    assert(manager.reject_offer(id) == false);
    PASS();
}

static void test_offer_reject_fails_nonexistent() {
    TEST("Offer reject: fails for nonexistent offer ID");
    TradeOfferManager manager;

    assert(manager.reject_offer(9999) == false);
    PASS();
}

// =============================================================================
// Trade Acceptance Creates Agreement
// =============================================================================

static void test_acceptance_creates_agreement_on_server() {
    TEST("Acceptance: creates agreement on server with correct parameters");
    TradeOfferManager manager;

    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Premium, 50);
    bool accepted = manager.accept_offer(id, 100);
    assert(accepted);

    const TradeOffer* offer = manager.get_offer(id);

    // Server creates agreement from accepted offer
    TradeAgreementComponent agreement;
    agreement.party_a = offer->from_player;
    agreement.party_b = offer->to_player;
    agreement.agreement_type = offer->proposed_type;
    agreement.cycles_remaining = 1500; // Premium default

    // Verify agreement matches offer
    assert(agreement.party_a == 1);
    assert(agreement.party_b == 2);
    assert(agreement.agreement_type == TradeAgreementType::Premium);
    assert(agreement.cycles_remaining == 1500);

    // Verify benefits are correct for Premium
    TradeAgreementBenefits benefits = get_agreement_benefits(TradeAgreementType::Premium);
    assert(benefits.demand_bonus == 10);
    assert(benefits.income_bonus_percent == 15);
    PASS();
}

static void test_acceptance_agreement_benefits_basic() {
    TEST("Acceptance: Basic agreement has correct benefits");
    TradeAgreementBenefits benefits = get_agreement_benefits(TradeAgreementType::Basic);
    assert(benefits.demand_bonus == 3);
    assert(benefits.income_bonus_percent == 5);
    PASS();
}

static void test_acceptance_agreement_benefits_enhanced() {
    TEST("Acceptance: Enhanced agreement has correct benefits");
    TradeAgreementBenefits benefits = get_agreement_benefits(TradeAgreementType::Enhanced);
    assert(benefits.demand_bonus == 6);
    assert(benefits.income_bonus_percent == 10);
    PASS();
}

static void test_acceptance_agreement_demand_bonus_calculation() {
    TEST("Acceptance: total demand bonus calculated from agreements");
    std::vector<TradeAgreementComponent> agreements;

    // Player 1 has Basic agreement with Player 2
    TradeAgreementComponent a1;
    a1.party_a = 1;
    a1.party_b = 2;
    a1.agreement_type = TradeAgreementType::Basic;
    a1.cycles_remaining = 100;
    agreements.push_back(a1);

    // Player 1 has Enhanced agreement with Player 3
    TradeAgreementComponent a2;
    a2.party_a = 1;
    a2.party_b = 3;
    a2.agreement_type = TradeAgreementType::Enhanced;
    a2.cycles_remaining = 100;
    agreements.push_back(a2);

    // Player 1's total demand bonus: Basic(+3) + Enhanced(+6) = +9
    int16_t total = calculate_total_demand_bonus(agreements, 1);
    assert(total == 9);

    // Player 2's total demand bonus: only Basic(+3)
    total = calculate_total_demand_bonus(agreements, 2);
    assert(total == 3);

    // Player 4 has no agreements
    total = calculate_total_demand_bonus(agreements, 4);
    assert(total == 0);
    PASS();
}

static void test_acceptance_agreement_income_bonus_application() {
    TEST("Acceptance: income bonus applied correctly to base income");
    std::vector<TradeAgreementComponent> agreements;

    TradeAgreementComponent a;
    a.party_a = 1;
    a.party_b = 2;
    a.agreement_type = TradeAgreementType::Premium;
    a.cycles_remaining = 100;
    agreements.push_back(a);

    // Player 1 base income: 1000
    // Premium gives +15% -> 1000 * (100 + 15) / 100 = 1150
    int64_t modified = apply_trade_agreement_income_bonus(1000, agreements, 1);
    assert(modified == 1150);

    // Player 2 also gets the bonus (symmetric)
    modified = apply_trade_agreement_income_bonus(1000, agreements, 2);
    assert(modified == 1150);

    // Player 3 does not get the bonus
    modified = apply_trade_agreement_income_bonus(1000, agreements, 3);
    assert(modified == 1000);
    PASS();
}

// =============================================================================
// Trade Cancellation Tests
// =============================================================================

static void test_cancellation_reject_pending_offer() {
    TEST("Cancellation: rejecting pending offer removes it from pending list");
    TradeOfferManager manager;

    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    assert(manager.get_pending_count() == 1);

    manager.reject_offer(id);
    assert(manager.get_pending_count() == 0);

    // Offer still exists in history but is not pending
    assert(manager.get_offer(id) != nullptr);
    assert(manager.get_offer(id)->is_pending == false);
    PASS();
}

static void test_cancellation_message_roundtrip() {
    TEST("Cancellation: TradeCancelRequestMsg serialization roundtrip");
    TradeCancelRequestMsg original;
    original.route_id = 42;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    assert(buffer.size() == TradeCancelRequestMsg::SERIALIZED_SIZE);

    TradeCancelRequestMsg deserialized =
        TradeCancelRequestMsg::deserialize(buffer.data(), buffer.size());
    assert(deserialized.route_id == 42);
    PASS();
}

static void test_cancellation_route_cancelled_msg() {
    TEST("Cancellation: TradeRouteCancelledMsg preserves cancelled_by player");
    TradeRouteCancelledMsg original;
    original.route_id = 100;
    original.cancelled_by = 2;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    TradeRouteCancelledMsg deserialized =
        TradeRouteCancelledMsg::deserialize(buffer.data(), buffer.size());
    assert(deserialized.route_id == 100);
    assert(deserialized.cancelled_by == 2);
    PASS();
}

// =============================================================================
// Disconnection During Trade Tests
// =============================================================================

static void test_disconnection_expired_offers() {
    TEST("Disconnection: offers expire after timeout (500 ticks)");
    TradeOfferManager manager;

    uint32_t id1 = manager.create_offer(1, 2, TradeAgreementType::Basic, 100);
    uint32_t id2 = manager.create_offer(3, 2, TradeAgreementType::Enhanced, 200);

    // Simulate disconnection: time passes, no responses
    // At tick 599, id1 should still be pending
    manager.expire_offers(599);
    assert(manager.get_offer(id1)->is_pending == true);
    assert(manager.get_offer(id2)->is_pending == true);

    // At tick 600, id1 should have expired (created at 100, expiry = 600)
    manager.expire_offers(600);
    assert(manager.get_offer(id1)->is_pending == false);
    assert(manager.get_offer(id2)->is_pending == true);

    // At tick 700, id2 should have expired (created at 200, expiry = 700)
    manager.expire_offers(700);
    assert(manager.get_offer(id2)->is_pending == false);
    PASS();
}

static void test_disconnection_server_cancellation_marker() {
    TEST("Disconnection: server cancellation uses cancelled_by = 0");
    TradeRouteCancelledMsg msg;
    msg.route_id = 200;
    msg.cancelled_by = 0; // 0 = server/disconnect

    std::vector<uint8_t> buffer;
    msg.serialize(buffer);

    TradeRouteCancelledMsg deserialized =
        TradeRouteCancelledMsg::deserialize(buffer.data(), buffer.size());

    assert(deserialized.cancelled_by == 0);
    assert(deserialized.route_id == 200);
    PASS();
}

static void test_disconnection_cannot_accept_after_expiry() {
    TEST("Disconnection: cannot accept offer after expiry");
    TradeOfferManager manager;

    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Premium, 0);

    // Simulate disconnection/timeout
    manager.expire_offers(TRADE_OFFER_EXPIRY_TICKS);
    assert(manager.get_offer(id)->is_pending == false);

    // Try to accept after expiration
    bool accepted = manager.accept_offer(id, TRADE_OFFER_EXPIRY_TICKS + 1);
    assert(accepted == false);
    PASS();
}

static void test_disconnection_pending_count_after_expiry() {
    TEST("Disconnection: pending count drops to zero after all expire");
    TradeOfferManager manager;

    manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    manager.create_offer(2, 3, TradeAgreementType::Enhanced, 0);
    manager.create_offer(3, 4, TradeAgreementType::Premium, 0);

    assert(manager.get_pending_count() == 3);

    // Expire all offers (all created at tick 0, expire at 500)
    manager.expire_offers(TRADE_OFFER_EXPIRY_TICKS);
    assert(manager.get_pending_count() == 0);

    // Total count still includes expired offers
    assert(manager.get_offer_count() == 3);
    PASS();
}

static void test_disconnection_new_offer_after_expiry() {
    TEST("Disconnection: new offer can be created after previous expires");
    TradeOfferManager manager;

    uint32_t id1 = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    assert(id1 != 0);

    // Expire the offer
    manager.expire_offers(TRADE_OFFER_EXPIRY_TICKS);
    assert(manager.get_offer(id1)->is_pending == false);

    // Should be able to create a new offer to same target
    uint32_t id2 = manager.create_offer(1, 2, TradeAgreementType::Enhanced, TRADE_OFFER_EXPIRY_TICKS + 1);
    assert(id2 != 0);
    assert(id2 != id1);
    PASS();
}

// =============================================================================
// Network Message Serialization Round-trip Tests
// =============================================================================

static void test_msg_offer_request_roundtrip() {
    TEST("Network: TradeOfferRequestMsg serialize/deserialize");
    TradeOfferRequestMsg original;
    original.target_player = 3;
    original.proposed_type = static_cast<uint8_t>(TradeAgreementType::Premium);

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    assert(buffer.size() == TradeOfferRequestMsg::SERIALIZED_SIZE);

    TradeOfferRequestMsg deserialized =
        TradeOfferRequestMsg::deserialize(buffer.data(), buffer.size());
    assert(deserialized.target_player == 3);
    assert(deserialized.proposed_type == static_cast<uint8_t>(TradeAgreementType::Premium));
    PASS();
}

static void test_msg_offer_response_accept_roundtrip() {
    TEST("Network: TradeOfferResponseMsg accept roundtrip");
    TradeOfferResponseMsg original;
    original.offer_id = 42;
    original.accepted = true;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    TradeOfferResponseMsg deserialized =
        TradeOfferResponseMsg::deserialize(buffer.data(), buffer.size());
    assert(deserialized.offer_id == 42);
    assert(deserialized.accepted == true);
    PASS();
}

static void test_msg_offer_response_reject_roundtrip() {
    TEST("Network: TradeOfferResponseMsg reject roundtrip");
    TradeOfferResponseMsg original;
    original.offer_id = 99;
    original.accepted = false;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    TradeOfferResponseMsg deserialized =
        TradeOfferResponseMsg::deserialize(buffer.data(), buffer.size());
    assert(deserialized.offer_id == 99);
    assert(deserialized.accepted == false);
    PASS();
}

static void test_msg_cancel_request_roundtrip() {
    TEST("Network: TradeCancelRequestMsg roundtrip");
    TradeCancelRequestMsg original;
    original.route_id = 0xABCD1234;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    assert(buffer.size() == TradeCancelRequestMsg::SERIALIZED_SIZE);

    TradeCancelRequestMsg deserialized =
        TradeCancelRequestMsg::deserialize(buffer.data(), buffer.size());
    assert(deserialized.route_id == 0xABCD1234);
    PASS();
}

static void test_msg_offer_notification_roundtrip() {
    TEST("Network: TradeOfferNotificationMsg roundtrip");
    TradeOfferNotificationMsg original;
    original.offer_id = 7;
    original.from_player = 1;
    original.proposed_type = static_cast<uint8_t>(TradeAgreementType::Enhanced);

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    assert(buffer.size() == TradeOfferNotificationMsg::SERIALIZED_SIZE);

    TradeOfferNotificationMsg deserialized =
        TradeOfferNotificationMsg::deserialize(buffer.data(), buffer.size());
    assert(deserialized.offer_id == 7);
    assert(deserialized.from_player == 1);
    assert(deserialized.proposed_type == static_cast<uint8_t>(TradeAgreementType::Enhanced));
    PASS();
}

static void test_msg_route_established_roundtrip() {
    TEST("Network: TradeRouteEstablishedMsg roundtrip");
    TradeRouteEstablishedMsg original;
    original.route_id = 555;
    original.party_a = 1;
    original.party_b = 3;
    original.agreement_type = static_cast<uint8_t>(TradeAgreementType::Basic);

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    assert(buffer.size() == TradeRouteEstablishedMsg::SERIALIZED_SIZE);

    TradeRouteEstablishedMsg deserialized =
        TradeRouteEstablishedMsg::deserialize(buffer.data(), buffer.size());
    assert(deserialized.route_id == 555);
    assert(deserialized.party_a == 1);
    assert(deserialized.party_b == 3);
    assert(deserialized.agreement_type == static_cast<uint8_t>(TradeAgreementType::Basic));
    PASS();
}

static void test_msg_route_cancelled_roundtrip() {
    TEST("Network: TradeRouteCancelledMsg roundtrip");
    TradeRouteCancelledMsg original;
    original.route_id = 999;
    original.cancelled_by = 4;

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    assert(buffer.size() == TradeRouteCancelledMsg::SERIALIZED_SIZE);

    TradeRouteCancelledMsg deserialized =
        TradeRouteCancelledMsg::deserialize(buffer.data(), buffer.size());
    assert(deserialized.route_id == 999);
    assert(deserialized.cancelled_by == 4);
    PASS();
}

static void test_msg_buffer_too_small_throws() {
    TEST("Network: deserialize with too-small buffer throws runtime_error");
    bool all_threw = true;

    // TradeOfferRequestMsg (needs 2 bytes)
    {
        uint8_t data[1] = {0};
        bool threw = false;
        try { TradeOfferRequestMsg::deserialize(data, 1); }
        catch (const std::runtime_error&) { threw = true; }
        all_threw = all_threw && threw;
    }

    // TradeOfferResponseMsg (needs 5 bytes)
    {
        uint8_t data[4] = {0};
        bool threw = false;
        try { TradeOfferResponseMsg::deserialize(data, 4); }
        catch (const std::runtime_error&) { threw = true; }
        all_threw = all_threw && threw;
    }

    // TradeCancelRequestMsg (needs 4 bytes)
    {
        uint8_t data[3] = {0};
        bool threw = false;
        try { TradeCancelRequestMsg::deserialize(data, 3); }
        catch (const std::runtime_error&) { threw = true; }
        all_threw = all_threw && threw;
    }

    // TradeOfferNotificationMsg (needs 6 bytes)
    {
        uint8_t data[5] = {0};
        bool threw = false;
        try { TradeOfferNotificationMsg::deserialize(data, 5); }
        catch (const std::runtime_error&) { threw = true; }
        all_threw = all_threw && threw;
    }

    // TradeRouteEstablishedMsg (needs 7 bytes)
    {
        uint8_t data[6] = {0};
        bool threw = false;
        try { TradeRouteEstablishedMsg::deserialize(data, 6); }
        catch (const std::runtime_error&) { threw = true; }
        all_threw = all_threw && threw;
    }

    // TradeRouteCancelledMsg (needs 5 bytes)
    {
        uint8_t data[4] = {0};
        bool threw = false;
        try { TradeRouteCancelledMsg::deserialize(data, 4); }
        catch (const std::runtime_error&) { threw = true; }
        all_threw = all_threw && threw;
    }

    assert(all_threw);
    PASS();
}

static void test_msg_large_values_preserved() {
    TEST("Network: large field values preserved in roundtrip");

    // Max uint32_t offer_id
    TradeOfferResponseMsg response;
    response.offer_id = 0xFFFFFFFF;
    response.accepted = true;

    std::vector<uint8_t> buffer;
    response.serialize(buffer);

    TradeOfferResponseMsg deserialized =
        TradeOfferResponseMsg::deserialize(buffer.data(), buffer.size());
    assert(deserialized.offer_id == 0xFFFFFFFF);
    assert(deserialized.accepted == true);

    // Max uint32_t route_id
    TradeRouteEstablishedMsg route;
    route.route_id = 0xFFFFFFFF;
    route.party_a = 255;
    route.party_b = 255;
    route.agreement_type = 255;

    buffer.clear();
    route.serialize(buffer);

    TradeRouteEstablishedMsg route_deserialized =
        TradeRouteEstablishedMsg::deserialize(buffer.data(), buffer.size());
    assert(route_deserialized.route_id == 0xFFFFFFFF);
    assert(route_deserialized.party_a == 255);
    assert(route_deserialized.party_b == 255);
    PASS();
}

// =============================================================================
// Full Trade Lifecycle Tests
// =============================================================================

static void test_lifecycle_offer_to_agreement() {
    TEST("Lifecycle: offer -> accept -> agreement -> benefits");
    TradeOfferManager manager;

    // Step 1: Player 1 creates offer to Player 2
    uint32_t offer_id = manager.create_offer(1, 2, TradeAgreementType::Enhanced, 0);
    assert(offer_id != 0);
    assert(manager.get_pending_count() == 1);

    // Step 2: Player 2 accepts
    bool accepted = manager.accept_offer(offer_id, 100);
    assert(accepted);
    assert(manager.get_pending_count() == 0);

    // Step 3: Server creates agreement from accepted offer
    const TradeOffer* offer = manager.get_offer(offer_id);
    TradeAgreementComponent agreement;
    agreement.party_a = offer->from_player;
    agreement.party_b = offer->to_player;
    agreement.agreement_type = offer->proposed_type;
    agreement.cycles_remaining = 1000; // Enhanced default

    // Step 4: Verify benefits
    TradeAgreementBenefits benefits = get_agreement_benefits(agreement.agreement_type);
    assert(benefits.demand_bonus == 6);
    assert(benefits.income_bonus_percent == 10);

    // Step 5: Calculate demand bonus with this agreement
    std::vector<TradeAgreementComponent> agreements = {agreement};
    int16_t bonus_p1 = calculate_total_demand_bonus(agreements, 1);
    int16_t bonus_p2 = calculate_total_demand_bonus(agreements, 2);
    assert(bonus_p1 == 6);
    assert(bonus_p2 == 6); // Symmetric

    // Step 6: Calculate income bonus
    int64_t income_p1 = apply_trade_agreement_income_bonus(1000, agreements, 1);
    int64_t income_p2 = apply_trade_agreement_income_bonus(1000, agreements, 2);
    assert(income_p1 == 1100); // 1000 * (100+10)/100
    assert(income_p2 == 1100); // Symmetric
    PASS();
}

static void test_lifecycle_offer_reject_new_offer() {
    TEST("Lifecycle: offer -> reject -> new offer");
    TradeOfferManager manager;

    uint32_t id1 = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    assert(id1 != 0);

    manager.reject_offer(id1);
    assert(manager.get_pending_count() == 0);

    // Player 1 can create a new offer (maybe different tier)
    uint32_t id2 = manager.create_offer(1, 2, TradeAgreementType::Premium, 50);
    assert(id2 != 0);
    assert(manager.get_pending_count() == 1);

    // Player 2 accepts the upgraded offer
    bool accepted = manager.accept_offer(id2, 100);
    assert(accepted);

    const TradeOffer* offer = manager.get_offer(id2);
    assert(offer->proposed_type == TradeAgreementType::Premium);
    PASS();
}

static void test_lifecycle_multiple_concurrent_agreements() {
    TEST("Lifecycle: multiple concurrent agreements between different pairs");
    TradeOfferManager manager;

    // Player 1 <-> Player 2: Basic
    uint32_t id12 = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    manager.accept_offer(id12, 10);

    // Player 1 <-> Player 3: Enhanced
    uint32_t id13 = manager.create_offer(1, 3, TradeAgreementType::Enhanced, 0);
    manager.accept_offer(id13, 10);

    // Player 2 <-> Player 3: Premium
    uint32_t id23 = manager.create_offer(2, 3, TradeAgreementType::Premium, 0);
    manager.accept_offer(id23, 10);

    // All offers accepted, none pending
    assert(manager.get_pending_count() == 0);
    assert(manager.get_offer_count() == 3);

    // Build agreements
    std::vector<TradeAgreementComponent> agreements;

    const TradeOffer* o12 = manager.get_offer(id12);
    const TradeOffer* o13 = manager.get_offer(id13);
    const TradeOffer* o23 = manager.get_offer(id23);

    TradeAgreementComponent a12;
    a12.party_a = o12->from_player;
    a12.party_b = o12->to_player;
    a12.agreement_type = o12->proposed_type;
    agreements.push_back(a12);

    TradeAgreementComponent a13;
    a13.party_a = o13->from_player;
    a13.party_b = o13->to_player;
    a13.agreement_type = o13->proposed_type;
    agreements.push_back(a13);

    TradeAgreementComponent a23;
    a23.party_a = o23->from_player;
    a23.party_b = o23->to_player;
    a23.agreement_type = o23->proposed_type;
    agreements.push_back(a23);

    // Player 1: Basic(+3) + Enhanced(+6) = +9
    assert(calculate_total_demand_bonus(agreements, 1) == 9);

    // Player 2: Basic(+3) + Premium(+10) = +13
    assert(calculate_total_demand_bonus(agreements, 2) == 13);

    // Player 3: Enhanced(+6) + Premium(+10) = +16
    assert(calculate_total_demand_bonus(agreements, 3) == 16);

    // Player 4: no agreements
    assert(calculate_total_demand_bonus(agreements, 4) == 0);
    PASS();
}

static void test_lifecycle_reverse_direction_offers() {
    TEST("Lifecycle: reverse direction offers are independent");
    TradeOfferManager manager;

    // Player 1 -> Player 2
    uint32_t id1 = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    // Player 2 -> Player 1
    uint32_t id2 = manager.create_offer(2, 1, TradeAgreementType::Enhanced, 0);

    assert(id1 != 0 && id2 != 0);

    auto pending_for_1 = manager.get_pending_offers_for(1);
    assert(pending_for_1.size() == 1);
    assert(pending_for_1[0].from_player == 2);

    auto pending_for_2 = manager.get_pending_offers_for(2);
    assert(pending_for_2.size() == 1);
    assert(pending_for_2[0].from_player == 1);
    PASS();
}

// =============================================================================
// Trade Agreement with PortSystem Integration
// =============================================================================

static void test_port_system_trade_agreement_integration() {
    TEST("PortSystem: trade agreements affect get_trade_income after tick");
    PortSystem sys(100, 100);

    // Add an operational aero port
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});

    // Tick without agreement
    sys.tick(0.05f);
    int64_t income_no_agreement = sys.get_trade_income(1);
    // Expected: 1000 * 0.7 * 0.8 * 1.0 = 560
    assert(income_no_agreement == 560);

    // Add inter-player agreement (Player 1 <-> Player 2, Premium 1.2x)
    TradeAgreementComponent agreement;
    agreement.party_a = 1;
    agreement.party_b = 2;
    agreement.agreement_type = TradeAgreementType::Premium;
    agreement.cycles_remaining = 100;
    agreement.income_bonus_percent = 120;
    sys.add_trade_agreement(agreement);

    // Tick with agreement
    sys.tick(0.05f);
    int64_t income_with_agreement = sys.get_trade_income(1);
    // Expected: 560 * 1.2 = 672
    assert(income_with_agreement == 672);
    assert(income_with_agreement > income_no_agreement);
    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Multiplayer Trade Routes Tests (Ticket E8-039) ===\n");

    // Trade offer create tests
    test_offer_create_sync();
    test_offer_create_all_tiers();
    test_offer_create_multiple_senders_to_one_target();
    test_offer_create_fails_none_type();
    test_offer_create_fails_self_trade();
    test_offer_create_fails_game_master();
    test_offer_create_fails_duplicate_pending();

    // Trade offer accept tests
    test_offer_accept_sync();
    test_offer_accept_creates_agreement_data();
    test_offer_accept_fails_expired();
    test_offer_accept_fails_already_accepted();
    test_offer_accept_fails_nonexistent();

    // Trade offer reject tests
    test_offer_reject_sync();
    test_offer_reject_allows_new_offer();
    test_offer_reject_fails_already_rejected();
    test_offer_reject_fails_nonexistent();

    // Trade acceptance creates agreement
    test_acceptance_creates_agreement_on_server();
    test_acceptance_agreement_benefits_basic();
    test_acceptance_agreement_benefits_enhanced();
    test_acceptance_agreement_demand_bonus_calculation();
    test_acceptance_agreement_income_bonus_application();

    // Trade cancellation
    test_cancellation_reject_pending_offer();
    test_cancellation_message_roundtrip();
    test_cancellation_route_cancelled_msg();

    // Disconnection during trade
    test_disconnection_expired_offers();
    test_disconnection_server_cancellation_marker();
    test_disconnection_cannot_accept_after_expiry();
    test_disconnection_pending_count_after_expiry();
    test_disconnection_new_offer_after_expiry();

    // Network message serialization roundtrips
    test_msg_offer_request_roundtrip();
    test_msg_offer_response_accept_roundtrip();
    test_msg_offer_response_reject_roundtrip();
    test_msg_cancel_request_roundtrip();
    test_msg_offer_notification_roundtrip();
    test_msg_route_established_roundtrip();
    test_msg_route_cancelled_roundtrip();
    test_msg_buffer_too_small_throws();
    test_msg_large_values_preserved();

    // Full trade lifecycle
    test_lifecycle_offer_to_agreement();
    test_lifecycle_offer_reject_new_offer();
    test_lifecycle_multiple_concurrent_agreements();
    test_lifecycle_reverse_direction_offers();

    // PortSystem integration
    test_port_system_trade_agreement_integration();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    return (tests_passed == tests_total) ? 0 : 1;
}
