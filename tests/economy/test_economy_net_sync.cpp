/**
 * @file test_economy_net_sync.cpp
 * @brief Unit tests for EconomyNetSync (E11-022)
 */

#include "sims3000/economy/EconomyNetSync.h"
#include "sims3000/economy/TreasuryState.h"
#include "sims3000/economy/BondRepayment.h"
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace sims3000::economy;

// ============================================================================
// Snapshot Creation Tests
// ============================================================================

void test_snapshot_creation_basic() {
    printf("Testing snapshot creation basic fields...\n");

    TreasuryState treasury;
    treasury.balance = 50000;
    treasury.last_income = 3000;
    treasury.last_expense = 1500;
    treasury.tribute_rate_habitation = 7;
    treasury.tribute_rate_exchange = 5;
    treasury.tribute_rate_fabrication = 10;
    treasury.funding_enforcer = 100;
    treasury.funding_hazard_response = 80;
    treasury.funding_medical = 120;
    treasury.funding_education = 90;

    auto snapshot = create_treasury_snapshot(treasury, 3);

    assert(snapshot.balance == 50000);
    assert(snapshot.last_income == 3000);
    assert(snapshot.last_expense == 1500);
    assert(snapshot.tribute_rate_habitation == 7);
    assert(snapshot.tribute_rate_exchange == 5);
    assert(snapshot.tribute_rate_fabrication == 10);
    assert(snapshot.funding_enforcer == 100);
    assert(snapshot.funding_hazard_response == 80);
    assert(snapshot.funding_medical == 120);
    assert(snapshot.funding_education == 90);
    assert(snapshot.player_id == 3);
    assert(snapshot.active_bond_count == 0);
    assert(snapshot.total_debt == 0);

    printf("  PASS: Snapshot created with correct fields\n");
}

void test_snapshot_creation_with_bonds() {
    printf("Testing snapshot creation with active bonds...\n");

    TreasuryState treasury;
    treasury.balance = 10000;

    CreditAdvance bond1;
    bond1.principal = 5000;
    bond1.remaining_principal = 4000;
    bond1.interest_rate_basis_points = 500;
    bond1.term_phases = 12;
    bond1.phases_remaining = 10;

    CreditAdvance bond2;
    bond2.principal = 25000;
    bond2.remaining_principal = 20000;
    bond2.interest_rate_basis_points = 750;
    bond2.term_phases = 24;
    bond2.phases_remaining = 18;

    treasury.active_bonds.push_back(bond1);
    treasury.active_bonds.push_back(bond2);

    auto snapshot = create_treasury_snapshot(treasury, 1);

    assert(snapshot.active_bond_count == 2);
    assert(snapshot.total_debt == 24000); // 4000 + 20000
    assert(snapshot.balance == 10000);

    printf("  PASS: Snapshot includes bond count and total debt\n");
}

void test_snapshot_creation_default_treasury() {
    printf("Testing snapshot creation with default treasury...\n");

    TreasuryState treasury; // default values

    auto snapshot = create_treasury_snapshot(treasury, 0);

    assert(snapshot.balance == 20000); // default starting balance
    assert(snapshot.last_income == 0);
    assert(snapshot.last_expense == 0);
    assert(snapshot.tribute_rate_habitation == 7); // default rate
    assert(snapshot.tribute_rate_exchange == 7);
    assert(snapshot.tribute_rate_fabrication == 7);
    assert(snapshot.funding_enforcer == 100); // default funding
    assert(snapshot.funding_hazard_response == 100);
    assert(snapshot.funding_medical == 100);
    assert(snapshot.funding_education == 100);
    assert(snapshot.active_bond_count == 0);
    assert(snapshot.total_debt == 0);
    assert(snapshot.player_id == 0);

    printf("  PASS: Default treasury snapshot correct\n");
}

// ============================================================================
// Snapshot Serialization Round-trip Tests
// ============================================================================

void test_serialize_deserialize_roundtrip() {
    printf("Testing snapshot serialize/deserialize round-trip...\n");

    TreasurySnapshot original{};
    original.balance = -5000;
    original.last_income = 2000;
    original.last_expense = 7000;
    original.tribute_rate_habitation = 12;
    original.tribute_rate_exchange = 3;
    original.tribute_rate_fabrication = 20;
    original.funding_enforcer = 50;
    original.funding_hazard_response = 150;
    original.funding_medical = 0;
    original.funding_education = 100;
    original.active_bond_count = 3;
    original.total_debt = 75000;
    original.player_id = 7;

    uint8_t buffer[128];
    size_t written = serialize_treasury_snapshot(original, buffer, sizeof(buffer));

    assert(written == TREASURY_SNAPSHOT_SERIALIZED_SIZE);

    TreasurySnapshot deserialized{};
    bool ok = deserialize_treasury_snapshot(buffer, written, deserialized);

    assert(ok);
    assert(deserialized.balance == original.balance);
    assert(deserialized.last_income == original.last_income);
    assert(deserialized.last_expense == original.last_expense);
    assert(deserialized.tribute_rate_habitation == original.tribute_rate_habitation);
    assert(deserialized.tribute_rate_exchange == original.tribute_rate_exchange);
    assert(deserialized.tribute_rate_fabrication == original.tribute_rate_fabrication);
    assert(deserialized.funding_enforcer == original.funding_enforcer);
    assert(deserialized.funding_hazard_response == original.funding_hazard_response);
    assert(deserialized.funding_medical == original.funding_medical);
    assert(deserialized.funding_education == original.funding_education);
    assert(deserialized.active_bond_count == original.active_bond_count);
    assert(deserialized.total_debt == original.total_debt);
    assert(deserialized.player_id == original.player_id);

    printf("  PASS: Round-trip preserves all fields\n");
}

void test_serialize_buffer_too_small() {
    printf("Testing serialize with buffer too small...\n");

    TreasurySnapshot snapshot{};
    uint8_t buffer[4]; // too small
    size_t written = serialize_treasury_snapshot(snapshot, buffer, sizeof(buffer));

    assert(written == 0);

    printf("  PASS: Returns 0 for undersized buffer\n");
}

void test_deserialize_buffer_too_small() {
    printf("Testing deserialize with buffer too small...\n");

    uint8_t buffer[4] = {TREASURY_SNAPSHOT_MAGIC, 0, 0, 0};
    TreasurySnapshot out{};
    bool ok = deserialize_treasury_snapshot(buffer, sizeof(buffer), out);

    assert(!ok);

    printf("  PASS: Returns false for undersized buffer\n");
}

void test_deserialize_wrong_magic() {
    printf("Testing deserialize with wrong magic byte...\n");

    TreasurySnapshot original{};
    original.balance = 12345;

    uint8_t buffer[128];
    serialize_treasury_snapshot(original, buffer, sizeof(buffer));

    // Corrupt magic byte
    buffer[0] = 0xFF;

    TreasurySnapshot out{};
    bool ok = deserialize_treasury_snapshot(buffer, TREASURY_SNAPSHOT_SERIALIZED_SIZE, out);

    assert(!ok);

    printf("  PASS: Rejects invalid magic byte\n");
}

void test_serialize_magic_byte_present() {
    printf("Testing magic byte is written correctly...\n");

    TreasurySnapshot snapshot{};
    uint8_t buffer[128];
    serialize_treasury_snapshot(snapshot, buffer, sizeof(buffer));

    assert(buffer[0] == TREASURY_SNAPSHOT_MAGIC);

    printf("  PASS: Magic byte 0x%02X present at offset 0\n", TREASURY_SNAPSHOT_MAGIC);
}

// ============================================================================
// Apply Snapshot Tests
// ============================================================================

void test_apply_snapshot_to_treasury() {
    printf("Testing apply snapshot to treasury...\n");

    TreasuryState treasury;
    treasury.balance = 99999; // will be overwritten

    TreasurySnapshot snapshot{};
    snapshot.balance = 42000;
    snapshot.last_income = 5000;
    snapshot.last_expense = 3000;
    snapshot.tribute_rate_habitation = 10;
    snapshot.tribute_rate_exchange = 15;
    snapshot.tribute_rate_fabrication = 5;
    snapshot.funding_enforcer = 80;
    snapshot.funding_hazard_response = 120;
    snapshot.funding_medical = 60;
    snapshot.funding_education = 140;

    apply_treasury_snapshot(treasury, snapshot);

    assert(treasury.balance == 42000);
    assert(treasury.last_income == 5000);
    assert(treasury.last_expense == 3000);
    assert(treasury.tribute_rate_habitation == 10);
    assert(treasury.tribute_rate_exchange == 15);
    assert(treasury.tribute_rate_fabrication == 5);
    assert(treasury.funding_enforcer == 80);
    assert(treasury.funding_hazard_response == 120);
    assert(treasury.funding_medical == 60);
    assert(treasury.funding_education == 140);

    printf("  PASS: Treasury state updated from snapshot\n");
}

// ============================================================================
// Tribute Rate Change Message Tests
// ============================================================================

void test_tribute_rate_change_roundtrip() {
    printf("Testing tribute rate change message round-trip...\n");

    TributeRateChangeMessage original;
    original.player_id = 2;
    original.zone_type = 1; // Exchange
    original.new_rate = 15;

    uint8_t buffer[16];
    size_t written = serialize_tribute_rate_change(original, buffer, sizeof(buffer));

    assert(written == TRIBUTE_RATE_MSG_SERIALIZED_SIZE);
    assert(buffer[0] == TRIBUTE_RATE_MSG_MAGIC);

    TributeRateChangeMessage deserialized{};
    bool ok = deserialize_tribute_rate_change(buffer, written, deserialized);

    assert(ok);
    assert(deserialized.player_id == 2);
    assert(deserialized.zone_type == 1);
    assert(deserialized.new_rate == 15);

    printf("  PASS: Tribute rate change round-trip correct\n");
}

void test_tribute_rate_change_bad_magic() {
    printf("Testing tribute rate change with bad magic...\n");

    uint8_t buffer[16] = {0xFF, 1, 2, 3};
    TributeRateChangeMessage out{};
    bool ok = deserialize_tribute_rate_change(buffer, sizeof(buffer), out);

    assert(!ok);

    printf("  PASS: Rejects bad magic for tribute rate message\n");
}

// ============================================================================
// Funding Level Change Message Tests
// ============================================================================

void test_funding_level_change_roundtrip() {
    printf("Testing funding level change message round-trip...\n");

    FundingLevelChangeMessage original;
    original.player_id = 5;
    original.service_type = 3; // Education
    original.new_level = 130;

    uint8_t buffer[16];
    size_t written = serialize_funding_level_change(original, buffer, sizeof(buffer));

    assert(written == FUNDING_LEVEL_MSG_SERIALIZED_SIZE);
    assert(buffer[0] == FUNDING_LEVEL_MSG_MAGIC);

    FundingLevelChangeMessage deserialized{};
    bool ok = deserialize_funding_level_change(buffer, written, deserialized);

    assert(ok);
    assert(deserialized.player_id == 5);
    assert(deserialized.service_type == 3);
    assert(deserialized.new_level == 130);

    printf("  PASS: Funding level change round-trip correct\n");
}

void test_funding_level_change_buffer_too_small() {
    printf("Testing funding level change with small buffer...\n");

    FundingLevelChangeMessage msg;
    msg.player_id = 0;
    msg.service_type = 0;
    msg.new_level = 100;

    uint8_t buffer[2]; // too small
    size_t written = serialize_funding_level_change(msg, buffer, sizeof(buffer));

    assert(written == 0);

    printf("  PASS: Returns 0 for undersized funding message buffer\n");
}

// ============================================================================
// Full Pipeline Test: create -> serialize -> deserialize -> apply
// ============================================================================

void test_full_pipeline() {
    printf("Testing full pipeline: create -> serialize -> deserialize -> apply...\n");

    // Server side: create treasury and snapshot
    TreasuryState server_treasury;
    server_treasury.balance = 35000;
    server_treasury.last_income = 8000;
    server_treasury.last_expense = 4500;
    server_treasury.tribute_rate_habitation = 9;
    server_treasury.tribute_rate_exchange = 11;
    server_treasury.tribute_rate_fabrication = 6;
    server_treasury.funding_enforcer = 75;
    server_treasury.funding_hazard_response = 110;
    server_treasury.funding_medical = 100;
    server_treasury.funding_education = 50;

    CreditAdvance bond;
    bond.principal = 25000;
    bond.remaining_principal = 15000;
    bond.interest_rate_basis_points = 750;
    bond.term_phases = 24;
    bond.phases_remaining = 14;
    server_treasury.active_bonds.push_back(bond);

    auto snapshot = create_treasury_snapshot(server_treasury, 4);

    // Serialize
    uint8_t buffer[128];
    size_t written = serialize_treasury_snapshot(snapshot, buffer, sizeof(buffer));
    assert(written > 0);

    // Client side: deserialize and apply
    TreasurySnapshot received{};
    bool ok = deserialize_treasury_snapshot(buffer, written, received);
    assert(ok);

    TreasuryState client_treasury; // starts with defaults
    apply_treasury_snapshot(client_treasury, received);

    // Verify client matches server
    assert(client_treasury.balance == 35000);
    assert(client_treasury.last_income == 8000);
    assert(client_treasury.last_expense == 4500);
    assert(client_treasury.tribute_rate_habitation == 9);
    assert(client_treasury.tribute_rate_exchange == 11);
    assert(client_treasury.tribute_rate_fabrication == 6);
    assert(client_treasury.funding_enforcer == 75);
    assert(client_treasury.funding_hazard_response == 110);
    assert(client_treasury.funding_medical == 100);
    assert(client_treasury.funding_education == 50);

    printf("  PASS: Full pipeline preserves all treasury data\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Economy Network Sync Unit Tests (E11-022) ===\n\n");

    // Snapshot creation
    test_snapshot_creation_basic();
    test_snapshot_creation_with_bonds();
    test_snapshot_creation_default_treasury();

    // Serialization round-trip
    test_serialize_deserialize_roundtrip();
    test_serialize_buffer_too_small();
    test_deserialize_buffer_too_small();
    test_deserialize_wrong_magic();
    test_serialize_magic_byte_present();

    // Apply snapshot
    test_apply_snapshot_to_treasury();

    // Tribute rate change messages
    test_tribute_rate_change_roundtrip();
    test_tribute_rate_change_bad_magic();

    // Funding level change messages
    test_funding_level_change_roundtrip();
    test_funding_level_change_buffer_too_small();

    // Full pipeline
    test_full_pipeline();

    printf("\n=== All tests passed! (14 tests) ===\n");
    return 0;
}
