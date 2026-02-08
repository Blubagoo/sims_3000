/**
 * @file TradeDealNegotiation.cpp
 * @brief Trade deal negotiation implementation for Epic 8 (Ticket E8-022)
 *
 * Implements trade deal creation with NPC neighbors including:
 * - Tier-based configuration lookup
 * - Deal initiation with validation
 * - Deal downgrade logic
 * - Per-cycle tick processing with expiration
 */

#include <sims3000/port/TradeDealNegotiation.h>
#include <sims3000/ecs/Components.h>

namespace sims3000 {
namespace port {

// =============================================================================
// Static deal configuration table
// =============================================================================

/// Configuration table indexed by TradeAgreementType underlying value.
/// Order: None(0), Basic(1), Enhanced(2), Premium(3)
static const TradeDealConfig DEAL_CONFIGS[] = {
    // None:     no cost, 0.5x income, +0 demand, 0 duration
    { 0,    0.5f,  0,  0 },
    // Basic:    1000 cr/cycle, 0.8x income, +5 demand, 500 cycles
    { 1000, 0.8f,  5,  500 },
    // Enhanced: 2500 cr/cycle, 1.0x income, +10 demand, 1000 cycles
    { 2500, 1.0f, 10, 1000 },
    // Premium:  5000 cr/cycle, 1.2x income, +15 demand, 1500 cycles
    { 5000, 1.2f, 15, 1500 },
};

// =============================================================================
// get_trade_deal_config
// =============================================================================

TradeDealConfig get_trade_deal_config(TradeAgreementType type) {
    uint8_t index = static_cast<uint8_t>(type);
    if (index >= TRADE_AGREEMENT_TYPE_COUNT) {
        // Return None config as fallback
        return DEAL_CONFIGS[0];
    }
    return DEAL_CONFIGS[index];
}

// =============================================================================
// initiate_trade_deal
// =============================================================================

bool initiate_trade_deal(TradeAgreementComponent& agreement,
                          uint8_t player_id,
                          uint8_t neighbor_id,
                          TradeAgreementType proposed_type,
                          int64_t player_treasury) {
    // Validate: proposed type must not be None
    if (proposed_type == TradeAgreementType::None) {
        return false;
    }

    // Validate: player_id must not be GAME_MASTER (0)
    if (player_id == GAME_MASTER) {
        return false;
    }

    // Validate: neighbor_id must be in range [1, 4]
    if (neighbor_id == 0 || neighbor_id > 4) {
        return false;
    }

    // Get config for the proposed tier
    TradeDealConfig config = get_trade_deal_config(proposed_type);

    // Validate: player must have enough treasury for at least one cycle
    if (player_treasury < static_cast<int64_t>(config.cost_per_cycle)) {
        return false;
    }

    // Populate the agreement component
    agreement.party_a = GAME_MASTER;          // NPC neighbor side
    agreement.party_b = player_id;            // Player side
    agreement.agreement_type = proposed_type;
    agreement.neighbor_id = neighbor_id;
    agreement.cycles_remaining = config.default_duration;
    agreement.demand_bonus_a = 0;             // NPC gets no demand bonus
    agreement.demand_bonus_b = config.demand_bonus; // Player gets the bonus
    agreement.income_bonus_percent = static_cast<uint8_t>(config.income_multiplier * 100.0f);
    agreement.cost_per_cycle_a = 0;           // NPC pays nothing
    agreement.cost_per_cycle_b = static_cast<int16_t>(config.cost_per_cycle);

    return true;
}

// =============================================================================
// downgrade_trade_deal
// =============================================================================

TradeAgreementType downgrade_trade_deal(TradeAgreementComponent& agreement) {
    TradeAgreementType current = agreement.agreement_type;
    TradeAgreementType new_type = TradeAgreementType::None;

    switch (current) {
        case TradeAgreementType::Premium:
            new_type = TradeAgreementType::Enhanced;
            break;
        case TradeAgreementType::Enhanced:
            new_type = TradeAgreementType::Basic;
            break;
        case TradeAgreementType::Basic:
            new_type = TradeAgreementType::None;
            break;
        case TradeAgreementType::None:
        default:
            new_type = TradeAgreementType::None;
            break;
    }

    // Update agreement with new tier's config
    TradeDealConfig config = get_trade_deal_config(new_type);

    agreement.agreement_type = new_type;
    agreement.demand_bonus_b = config.demand_bonus;
    agreement.income_bonus_percent = static_cast<uint8_t>(config.income_multiplier * 100.0f);
    agreement.cost_per_cycle_b = static_cast<int16_t>(config.cost_per_cycle);
    agreement.cycles_remaining = config.default_duration;

    // If downgraded to None, clear NPC-side fields too
    if (new_type == TradeAgreementType::None) {
        agreement.demand_bonus_a = 0;
        agreement.cost_per_cycle_a = 0;
    }

    return new_type;
}

// =============================================================================
// tick_trade_deal
// =============================================================================

bool tick_trade_deal(TradeAgreementComponent& agreement) {
    // Already expired or no deal
    if (agreement.agreement_type == TradeAgreementType::None ||
        agreement.cycles_remaining == 0) {
        // Ensure it's fully zeroed
        agreement.agreement_type = TradeAgreementType::None;
        agreement.cycles_remaining = 0;
        agreement.demand_bonus_a = 0;
        agreement.demand_bonus_b = 0;
        agreement.income_bonus_percent = static_cast<uint8_t>(0.5f * 100.0f); // None tier default
        agreement.cost_per_cycle_a = 0;
        agreement.cost_per_cycle_b = 0;
        return false;
    }

    // Decrement duration
    agreement.cycles_remaining--;

    // Check if just expired
    if (agreement.cycles_remaining == 0) {
        agreement.agreement_type = TradeAgreementType::None;
        agreement.demand_bonus_a = 0;
        agreement.demand_bonus_b = 0;
        agreement.income_bonus_percent = static_cast<uint8_t>(0.5f * 100.0f);
        agreement.cost_per_cycle_a = 0;
        agreement.cost_per_cycle_b = 0;
        return false;
    }

    return true;
}

} // namespace port
} // namespace sims3000
