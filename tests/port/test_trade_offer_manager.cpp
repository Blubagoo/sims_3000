/**
 * @file test_trade_offer_manager.cpp
 * @brief Unit tests for TradeOfferManager (Epic 8, Ticket E8-025)
 *
 * Tests cover:
 * - Offer creation with valid parameters
 * - Offer creation failures (same player, GAME_MASTER, None type, duplicates)
 * - Offer acceptance (server-authoritative)
 * - Offer rejection
 * - Offer expiration after 500 ticks
 * - Query: get_offer by ID
 * - Query: get_pending_offers_for a player
 * - Multiple offers between different players
 * - Cannot accept expired offer
 * - Cannot accept already-accepted offer
 * - Cannot reject already-rejected offer
 */

#include <sims3000/port/TradeOfferManager.h>
#include <sims3000/port/PortTypes.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::port;

// =============================================================================
// Test: Create offer succeeds with valid parameters
// =============================================================================

void test_create_offer_success() {
    printf("Testing offer creation with valid parameters...\n");

    TradeOfferManager manager;
    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 100);

    assert(id != 0);

    const TradeOffer* offer = manager.get_offer(id);
    assert(offer != nullptr);
    assert(offer->offer_id == id);
    assert(offer->from_player == 1);
    assert(offer->to_player == 2);
    assert(offer->proposed_type == TradeAgreementType::Basic);
    assert(offer->is_pending == true);
    assert(offer->created_tick == 100);
    assert(offer->expiry_tick == 100 + TRADE_OFFER_EXPIRY_TICKS);

    printf("  PASS: Offer created successfully (id=%u)\n", id);
}

// =============================================================================
// Test: Offer IDs are sequential
// =============================================================================

void test_sequential_ids() {
    printf("Testing offer IDs are sequential...\n");

    TradeOfferManager manager;
    uint32_t id1 = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    uint32_t id2 = manager.create_offer(2, 3, TradeAgreementType::Enhanced, 0);
    uint32_t id3 = manager.create_offer(3, 4, TradeAgreementType::Premium, 0);

    assert(id1 == 1);
    assert(id2 == 2);
    assert(id3 == 3);

    printf("  PASS: IDs are sequential (1, 2, 3)\n");
}

// =============================================================================
// Test: Fail - from and to are the same player
// =============================================================================

void test_create_fail_same_player() {
    printf("Testing offer creation fails with same from/to player...\n");

    TradeOfferManager manager;
    uint32_t id = manager.create_offer(1, 1, TradeAgreementType::Basic, 0);

    assert(id == 0);
    assert(manager.get_offer_count() == 0);

    printf("  PASS: Same player offer correctly rejected\n");
}

// =============================================================================
// Test: Fail - from player is GAME_MASTER
// =============================================================================

void test_create_fail_from_game_master() {
    printf("Testing offer creation fails with from=GAME_MASTER...\n");

    TradeOfferManager manager;
    uint32_t id = manager.create_offer(0, 2, TradeAgreementType::Basic, 0);

    assert(id == 0);

    printf("  PASS: GAME_MASTER as sender correctly rejected\n");
}

// =============================================================================
// Test: Fail - to player is GAME_MASTER
// =============================================================================

void test_create_fail_to_game_master() {
    printf("Testing offer creation fails with to=GAME_MASTER...\n");

    TradeOfferManager manager;
    uint32_t id = manager.create_offer(1, 0, TradeAgreementType::Basic, 0);

    assert(id == 0);

    printf("  PASS: GAME_MASTER as target correctly rejected\n");
}

// =============================================================================
// Test: Fail - proposed type is None
// =============================================================================

void test_create_fail_none_type() {
    printf("Testing offer creation fails with None type...\n");

    TradeOfferManager manager;
    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::None, 0);

    assert(id == 0);

    printf("  PASS: None type correctly rejected\n");
}

// =============================================================================
// Test: Fail - duplicate pending offer
// =============================================================================

void test_create_fail_duplicate() {
    printf("Testing offer creation fails with duplicate pending offer...\n");

    TradeOfferManager manager;
    uint32_t id1 = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    assert(id1 != 0);

    // Same from/to, should fail even with different type
    uint32_t id2 = manager.create_offer(1, 2, TradeAgreementType::Premium, 0);
    assert(id2 == 0);

    printf("  PASS: Duplicate pending offer correctly rejected\n");
}

// =============================================================================
// Test: After rejecting, can create new offer to same target
// =============================================================================

void test_create_after_reject() {
    printf("Testing offer creation succeeds after rejecting previous...\n");

    TradeOfferManager manager;
    uint32_t id1 = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    assert(id1 != 0);

    bool rejected = manager.reject_offer(id1);
    assert(rejected == true);

    // Now should be able to create a new offer
    uint32_t id2 = manager.create_offer(1, 2, TradeAgreementType::Enhanced, 10);
    assert(id2 != 0);
    assert(id2 != id1);

    printf("  PASS: New offer created after rejection\n");
}

// =============================================================================
// Test: Accept offer succeeds
// =============================================================================

void test_accept_offer_success() {
    printf("Testing offer acceptance succeeds...\n");

    TradeOfferManager manager;
    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 100);

    bool accepted = manager.accept_offer(id, 200); // before expiry (100 + 500 = 600)

    assert(accepted == true);

    const TradeOffer* offer = manager.get_offer(id);
    assert(offer != nullptr);
    assert(offer->is_pending == false);

    printf("  PASS: Offer accepted successfully\n");
}

// =============================================================================
// Test: Accept fails for expired offer
// =============================================================================

void test_accept_fail_expired() {
    printf("Testing offer acceptance fails when expired...\n");

    TradeOfferManager manager;
    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 100);

    // Try to accept at expiry tick (100 + 500 = 600)
    bool accepted = manager.accept_offer(id, 600);
    assert(accepted == false);

    // Try to accept after expiry
    // Need a new offer since the previous one was marked not pending
    uint32_t id2 = manager.create_offer(2, 3, TradeAgreementType::Basic, 100);
    accepted = manager.accept_offer(id2, 700);
    assert(accepted == false);

    printf("  PASS: Expired offer acceptance correctly rejected\n");
}

// =============================================================================
// Test: Accept fails for already accepted offer
// =============================================================================

void test_accept_fail_already_accepted() {
    printf("Testing double-acceptance fails...\n");

    TradeOfferManager manager;
    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);

    bool first = manager.accept_offer(id, 100);
    assert(first == true);

    bool second = manager.accept_offer(id, 100);
    assert(second == false);

    printf("  PASS: Double-acceptance correctly rejected\n");
}

// =============================================================================
// Test: Accept fails for non-existent offer
// =============================================================================

void test_accept_fail_not_found() {
    printf("Testing acceptance fails for non-existent offer...\n");

    TradeOfferManager manager;

    bool accepted = manager.accept_offer(999, 0);
    assert(accepted == false);

    printf("  PASS: Non-existent offer acceptance correctly rejected\n");
}

// =============================================================================
// Test: Reject offer succeeds
// =============================================================================

void test_reject_offer_success() {
    printf("Testing offer rejection succeeds...\n");

    TradeOfferManager manager;
    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);

    bool rejected = manager.reject_offer(id);

    assert(rejected == true);

    const TradeOffer* offer = manager.get_offer(id);
    assert(offer != nullptr);
    assert(offer->is_pending == false);

    printf("  PASS: Offer rejected successfully\n");
}

// =============================================================================
// Test: Reject fails for already rejected offer
// =============================================================================

void test_reject_fail_already_rejected() {
    printf("Testing double-rejection fails...\n");

    TradeOfferManager manager;
    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);

    bool first = manager.reject_offer(id);
    assert(first == true);

    bool second = manager.reject_offer(id);
    assert(second == false);

    printf("  PASS: Double-rejection correctly rejected\n");
}

// =============================================================================
// Test: Reject fails for non-existent offer
// =============================================================================

void test_reject_fail_not_found() {
    printf("Testing rejection fails for non-existent offer...\n");

    TradeOfferManager manager;

    bool rejected = manager.reject_offer(999);
    assert(rejected == false);

    printf("  PASS: Non-existent offer rejection correctly rejected\n");
}

// =============================================================================
// Test: Expire offers marks stale offers as not pending
// =============================================================================

void test_expire_offers() {
    printf("Testing expire_offers marks stale offers...\n");

    TradeOfferManager manager;
    uint32_t id1 = manager.create_offer(1, 2, TradeAgreementType::Basic, 100);
    uint32_t id2 = manager.create_offer(2, 3, TradeAgreementType::Enhanced, 200);

    // At tick 599, id1 should still be pending (expiry = 600)
    manager.expire_offers(599);
    assert(manager.get_offer(id1)->is_pending == true);
    assert(manager.get_offer(id2)->is_pending == true);

    // At tick 600, id1 should expire
    manager.expire_offers(600);
    assert(manager.get_offer(id1)->is_pending == false);
    assert(manager.get_offer(id2)->is_pending == true);

    // At tick 700, id2 should also expire
    manager.expire_offers(700);
    assert(manager.get_offer(id2)->is_pending == false);

    printf("  PASS: Offers expire at correct ticks\n");
}

// =============================================================================
// Test: get_pending_offers_for returns correct subset
// =============================================================================

void test_get_pending_offers_for() {
    printf("Testing get_pending_offers_for returns correct subset...\n");

    TradeOfferManager manager;
    manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    manager.create_offer(3, 2, TradeAgreementType::Enhanced, 0);
    manager.create_offer(1, 3, TradeAgreementType::Premium, 0);

    auto pending_for_2 = manager.get_pending_offers_for(2);
    assert(pending_for_2.size() == 2);

    auto pending_for_3 = manager.get_pending_offers_for(3);
    assert(pending_for_3.size() == 1);
    assert(pending_for_3[0].proposed_type == TradeAgreementType::Premium);

    auto pending_for_1 = manager.get_pending_offers_for(1);
    assert(pending_for_1.size() == 0); // Player 1 is sender, not target

    auto pending_for_4 = manager.get_pending_offers_for(4);
    assert(pending_for_4.size() == 0); // No offers for player 4

    printf("  PASS: get_pending_offers_for returns correct results\n");
}

// =============================================================================
// Test: get_pending_offers_for excludes non-pending offers
// =============================================================================

void test_pending_excludes_rejected() {
    printf("Testing get_pending_offers_for excludes rejected offers...\n");

    TradeOfferManager manager;
    uint32_t id1 = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    manager.create_offer(3, 2, TradeAgreementType::Enhanced, 0);

    manager.reject_offer(id1);

    auto pending = manager.get_pending_offers_for(2);
    assert(pending.size() == 1);
    assert(pending[0].from_player == 3);

    printf("  PASS: Rejected offers excluded from pending query\n");
}

// =============================================================================
// Test: get_offer_count and get_pending_count
// =============================================================================

void test_offer_counts() {
    printf("Testing offer count tracking...\n");

    TradeOfferManager manager;
    assert(manager.get_offer_count() == 0);
    assert(manager.get_pending_count() == 0);

    manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    assert(manager.get_offer_count() == 1);
    assert(manager.get_pending_count() == 1);

    uint32_t id2 = manager.create_offer(2, 3, TradeAgreementType::Enhanced, 0);
    assert(manager.get_offer_count() == 2);
    assert(manager.get_pending_count() == 2);

    manager.reject_offer(id2);
    assert(manager.get_offer_count() == 2);
    assert(manager.get_pending_count() == 1);

    printf("  PASS: Offer counts track correctly\n");
}

// =============================================================================
// Test: get_offer returns nullptr for non-existent ID
// =============================================================================

void test_get_offer_not_found() {
    printf("Testing get_offer returns nullptr for non-existent ID...\n");

    TradeOfferManager manager;
    const TradeOffer* offer = manager.get_offer(42);
    assert(offer == nullptr);

    printf("  PASS: Non-existent offer returns nullptr\n");
}

// =============================================================================
// Test: Multiple offers from different senders to same target
// =============================================================================

void test_multiple_offers_to_same_target() {
    printf("Testing multiple offers from different senders to same target...\n");

    TradeOfferManager manager;
    uint32_t id1 = manager.create_offer(1, 4, TradeAgreementType::Basic, 0);
    uint32_t id2 = manager.create_offer(2, 4, TradeAgreementType::Enhanced, 0);
    uint32_t id3 = manager.create_offer(3, 4, TradeAgreementType::Premium, 0);

    assert(id1 != 0);
    assert(id2 != 0);
    assert(id3 != 0);

    auto pending = manager.get_pending_offers_for(4);
    assert(pending.size() == 3);

    printf("  PASS: Multiple offers to same target all created\n");
}

// =============================================================================
// Test: Offer at tick boundary - accept at expiry_tick - 1 succeeds
// =============================================================================

void test_accept_at_boundary() {
    printf("Testing acceptance at expiry boundary...\n");

    TradeOfferManager manager;
    uint32_t id = manager.create_offer(1, 2, TradeAgreementType::Basic, 100);

    // expiry_tick = 100 + 500 = 600, accept at 599 should work
    bool accepted = manager.accept_offer(id, 599);
    assert(accepted == true);

    printf("  PASS: Offer accepted at tick 599 (expires at 600)\n");
}

// =============================================================================
// Test: Expiry constant is 500 ticks
// =============================================================================

void test_expiry_constant() {
    printf("Testing TRADE_OFFER_EXPIRY_TICKS is 500...\n");

    assert(TRADE_OFFER_EXPIRY_TICKS == 500);

    printf("  PASS: TRADE_OFFER_EXPIRY_TICKS == 500\n");
}

// =============================================================================
// Test: Reverse direction offers are independent
// =============================================================================

void test_reverse_direction_offers() {
    printf("Testing reverse direction offers are independent...\n");

    TradeOfferManager manager;
    uint32_t id1 = manager.create_offer(1, 2, TradeAgreementType::Basic, 0);
    uint32_t id2 = manager.create_offer(2, 1, TradeAgreementType::Enhanced, 0);

    assert(id1 != 0);
    assert(id2 != 0);
    assert(id1 != id2);

    auto pending_for_1 = manager.get_pending_offers_for(1);
    assert(pending_for_1.size() == 1);
    assert(pending_for_1[0].from_player == 2);

    auto pending_for_2 = manager.get_pending_offers_for(2);
    assert(pending_for_2.size() == 1);
    assert(pending_for_2[0].from_player == 1);

    printf("  PASS: Reverse direction offers are independent\n");
}

// =============================================================================
// Test: TradeOffer default constructor
// =============================================================================

void test_offer_default_constructor() {
    printf("Testing TradeOffer default constructor...\n");

    TradeOffer offer;
    assert(offer.offer_id == 0);
    assert(offer.from_player == 0);
    assert(offer.to_player == 0);
    assert(offer.proposed_type == TradeAgreementType::None);
    assert(offer.is_pending == false);
    assert(offer.created_tick == 0);
    assert(offer.expiry_tick == 0);

    printf("  PASS: Default TradeOffer has expected values\n");
}

// =============================================================================
// Test: TradeOffer parameterized constructor
// =============================================================================

void test_offer_param_constructor() {
    printf("Testing TradeOffer parameterized constructor...\n");

    TradeOffer offer(42, 1, 2, TradeAgreementType::Premium, 100);
    assert(offer.offer_id == 42);
    assert(offer.from_player == 1);
    assert(offer.to_player == 2);
    assert(offer.proposed_type == TradeAgreementType::Premium);
    assert(offer.is_pending == true);
    assert(offer.created_tick == 100);
    assert(offer.expiry_tick == 600); // 100 + 500

    printf("  PASS: Parameterized constructor sets fields correctly\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Trade Offer Manager Tests (Epic 8, Ticket E8-025) ===\n\n");

    // Offer creation tests
    test_create_offer_success();
    test_sequential_ids();
    test_create_fail_same_player();
    test_create_fail_from_game_master();
    test_create_fail_to_game_master();
    test_create_fail_none_type();
    test_create_fail_duplicate();
    test_create_after_reject();

    // Accept/reject tests
    test_accept_offer_success();
    test_accept_fail_expired();
    test_accept_fail_already_accepted();
    test_accept_fail_not_found();
    test_reject_offer_success();
    test_reject_fail_already_rejected();
    test_reject_fail_not_found();

    // Expiration tests
    test_expire_offers();

    // Query tests
    test_get_pending_offers_for();
    test_pending_excludes_rejected();
    test_offer_counts();
    test_get_offer_not_found();
    test_multiple_offers_to_same_target();
    test_accept_at_boundary();
    test_expiry_constant();
    test_reverse_direction_offers();

    // Constructor tests
    test_offer_default_constructor();
    test_offer_param_constructor();

    printf("\n=== All Trade Offer Manager Tests Passed ===\n");
    return 0;
}
