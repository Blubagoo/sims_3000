/**
 * @file TradeDealExpiration.cpp
 * @brief Trade deal expiration, warning, and renewal implementation (Ticket E8-023)
 *
 * Implements expiration status checking, combined tick-with-expiration,
 * and deal renewal logic for trade agreements.
 */

#include <sims3000/port/TradeDealExpiration.h>

namespace sims3000 {
namespace port {

// =============================================================================
// check_trade_deal_expiration
// =============================================================================

ExpirationCheckResult check_trade_deal_expiration(const TradeAgreementComponent& agreement) {
    // Expired: no deal or no cycles left
    if (agreement.agreement_type == TradeAgreementType::None ||
        agreement.cycles_remaining == 0) {
        return ExpirationCheckResult::Expired;
    }

    // Warning: within threshold
    if (agreement.cycles_remaining <= TRADE_DEAL_WARNING_CYCLES) {
        return ExpirationCheckResult::Warning;
    }

    // Active: plenty of time left
    return ExpirationCheckResult::Active;
}

// =============================================================================
// tick_trade_deal_with_expiration
// =============================================================================

ExpirationCheckResult tick_trade_deal_with_expiration(
    TradeAgreementComponent& agreement,
    uint32_t entity_id,
    TradeDealExpirationWarningEvent& warning_event,
    TradeAgreementExpiredEvent& expired_event) {

    // Tick the deal (handles decrement and zeroing)
    bool still_active = tick_trade_deal(agreement);

    if (!still_active) {
        // Deal expired this tick
        expired_event = TradeAgreementExpiredEvent(
            entity_id,
            agreement.party_a,
            agreement.party_b
        );
        return ExpirationCheckResult::Expired;
    }

    // Check if we entered the warning zone
    if (agreement.cycles_remaining <= TRADE_DEAL_WARNING_CYCLES) {
        warning_event = TradeDealExpirationWarningEvent(
            entity_id,
            agreement.party_a,
            agreement.party_b,
            agreement.cycles_remaining,
            agreement.agreement_type
        );
        return ExpirationCheckResult::Warning;
    }

    return ExpirationCheckResult::Active;
}

// =============================================================================
// renew_trade_deal
// =============================================================================

bool renew_trade_deal(TradeAgreementComponent& agreement, int64_t player_treasury) {
    // Cannot renew an expired or None deal
    if (agreement.agreement_type == TradeAgreementType::None) {
        return false;
    }
    if (agreement.cycles_remaining == 0) {
        return false;
    }

    // Get current tier's config for default duration and cost
    TradeDealConfig config = get_trade_deal_config(agreement.agreement_type);

    // Validate player has enough treasury for at least one cycle
    if (player_treasury < static_cast<int64_t>(config.cost_per_cycle)) {
        return false;
    }

    // Reset duration to default for the current tier
    agreement.cycles_remaining = config.default_duration;

    return true;
}

} // namespace port
} // namespace sims3000
