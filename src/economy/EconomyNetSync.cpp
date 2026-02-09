/**
 * @file EconomyNetSync.cpp
 * @brief Implementation of economy network synchronization (Ticket E11-022)
 */

#include "sims3000/economy/EconomyNetSync.h"
#include "sims3000/economy/TreasuryState.h"
#include "sims3000/economy/BondRepayment.h"
#include <cstring>

namespace sims3000 {
namespace economy {

// ============================================================================
// Snapshot creation and application
// ============================================================================

TreasurySnapshot create_treasury_snapshot(const TreasuryState& treasury, uint8_t player_id) {
    TreasurySnapshot snapshot{};

    snapshot.balance = treasury.balance;
    snapshot.last_income = treasury.last_income;
    snapshot.last_expense = treasury.last_expense;

    snapshot.tribute_rate_habitation = treasury.tribute_rate_habitation;
    snapshot.tribute_rate_exchange = treasury.tribute_rate_exchange;
    snapshot.tribute_rate_fabrication = treasury.tribute_rate_fabrication;

    snapshot.funding_enforcer = treasury.funding_enforcer;
    snapshot.funding_hazard_response = treasury.funding_hazard_response;
    snapshot.funding_medical = treasury.funding_medical;
    snapshot.funding_education = treasury.funding_education;

    snapshot.active_bond_count = static_cast<uint8_t>(treasury.active_bonds.size());
    snapshot.total_debt = get_total_debt(treasury.active_bonds);

    snapshot.player_id = player_id;

    return snapshot;
}

void apply_treasury_snapshot(TreasuryState& treasury, const TreasurySnapshot& snapshot) {
    treasury.balance = snapshot.balance;
    treasury.last_income = snapshot.last_income;
    treasury.last_expense = snapshot.last_expense;

    treasury.tribute_rate_habitation = snapshot.tribute_rate_habitation;
    treasury.tribute_rate_exchange = snapshot.tribute_rate_exchange;
    treasury.tribute_rate_fabrication = snapshot.tribute_rate_fabrication;

    treasury.funding_enforcer = snapshot.funding_enforcer;
    treasury.funding_hazard_response = snapshot.funding_hazard_response;
    treasury.funding_medical = snapshot.funding_medical;
    treasury.funding_education = snapshot.funding_education;

    // Note: active_bond_count and total_debt are informational only;
    // the client does not reconstruct individual bonds from the snapshot.
}

// ============================================================================
// Treasury Snapshot serialization
// ============================================================================

size_t serialize_treasury_snapshot(const TreasurySnapshot& snapshot, uint8_t* buffer, size_t buffer_size) {
    if (buffer_size < TREASURY_SNAPSHOT_SERIALIZED_SIZE) {
        return 0;
    }

    // Write magic byte
    buffer[0] = TREASURY_SNAPSHOT_MAGIC;

    // Write snapshot data
    std::memcpy(buffer + 1, &snapshot, sizeof(TreasurySnapshot));

    return TREASURY_SNAPSHOT_SERIALIZED_SIZE;
}

bool deserialize_treasury_snapshot(const uint8_t* buffer, size_t buffer_size, TreasurySnapshot& out) {
    if (buffer_size < TREASURY_SNAPSHOT_SERIALIZED_SIZE) {
        return false;
    }

    // Validate magic byte
    if (buffer[0] != TREASURY_SNAPSHOT_MAGIC) {
        return false;
    }

    // Read snapshot data
    std::memcpy(&out, buffer + 1, sizeof(TreasurySnapshot));

    return true;
}

// ============================================================================
// Tribute Rate Change serialization
// ============================================================================

size_t serialize_tribute_rate_change(const TributeRateChangeMessage& msg, uint8_t* buffer, size_t buffer_size) {
    if (buffer_size < TRIBUTE_RATE_MSG_SERIALIZED_SIZE) {
        return 0;
    }

    buffer[0] = TRIBUTE_RATE_MSG_MAGIC;
    std::memcpy(buffer + 1, &msg, sizeof(TributeRateChangeMessage));

    return TRIBUTE_RATE_MSG_SERIALIZED_SIZE;
}

bool deserialize_tribute_rate_change(const uint8_t* buffer, size_t buffer_size, TributeRateChangeMessage& out) {
    if (buffer_size < TRIBUTE_RATE_MSG_SERIALIZED_SIZE) {
        return false;
    }

    if (buffer[0] != TRIBUTE_RATE_MSG_MAGIC) {
        return false;
    }

    std::memcpy(&out, buffer + 1, sizeof(TributeRateChangeMessage));

    return true;
}

// ============================================================================
// Funding Level Change serialization
// ============================================================================

size_t serialize_funding_level_change(const FundingLevelChangeMessage& msg, uint8_t* buffer, size_t buffer_size) {
    if (buffer_size < FUNDING_LEVEL_MSG_SERIALIZED_SIZE) {
        return 0;
    }

    buffer[0] = FUNDING_LEVEL_MSG_MAGIC;
    std::memcpy(buffer + 1, &msg, sizeof(FundingLevelChangeMessage));

    return FUNDING_LEVEL_MSG_SERIALIZED_SIZE;
}

bool deserialize_funding_level_change(const uint8_t* buffer, size_t buffer_size, FundingLevelChangeMessage& out) {
    if (buffer_size < FUNDING_LEVEL_MSG_SERIALIZED_SIZE) {
        return false;
    }

    if (buffer[0] != FUNDING_LEVEL_MSG_MAGIC) {
        return false;
    }

    std::memcpy(&out, buffer + 1, sizeof(FundingLevelChangeMessage));

    return true;
}

} // namespace economy
} // namespace sims3000
