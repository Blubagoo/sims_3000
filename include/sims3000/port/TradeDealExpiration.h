/**
 * @file TradeDealExpiration.h
 * @brief Trade deal expiration, warning, and renewal for Epic 8 (Ticket E8-023)
 *
 * Defines:
 * - TRADE_DEAL_WARNING_CYCLES: Number of cycles before expiration to emit warning
 * - TradeDealExpirationWarningEvent: Emitted when deal is about to expire
 * - check_trade_deal_expiration(): Check and emit warning/expiration events
 * - renew_trade_deal(): Renew an active trade deal, resetting its duration
 *
 * Works alongside tick_trade_deal() from TradeDealNegotiation.h to add
 * warning notifications and renewal capabilities.
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <sims3000/port/TradeEvents.h>
#include <sims3000/port/TradeDealNegotiation.h>
#include <cstdint>

namespace sims3000 {
namespace port {

/// Number of cycles before expiration to emit a warning event
constexpr uint16_t TRADE_DEAL_WARNING_CYCLES = 5;

/**
 * @struct TradeDealExpirationWarningEvent
 * @brief Event emitted when a trade deal is approaching expiration.
 *
 * Emitted when cycles_remaining reaches TRADE_DEAL_WARNING_CYCLES (5).
 * Allows UI to notify the player and offer renewal.
 *
 * Consumed by:
 * - UISystem: Display "deal expiring soon" notification
 * - AudioSystem: Play warning alert sound
 */
struct TradeDealExpirationWarningEvent {
    uint32_t agreement;         ///< Trade agreement entity ID
    uint8_t party_a;            ///< First party PlayerID
    uint8_t party_b;            ///< Second party PlayerID
    uint16_t cycles_remaining;  ///< Cycles left before expiration
    TradeAgreementType type;    ///< Current agreement tier

    TradeDealExpirationWarningEvent()
        : agreement(0)
        , party_a(0)
        , party_b(0)
        , cycles_remaining(0)
        , type(TradeAgreementType::None)
    {}

    TradeDealExpirationWarningEvent(uint32_t agreement_id, uint8_t a, uint8_t b,
                                     uint16_t remaining, TradeAgreementType agreement_type)
        : agreement(agreement_id)
        , party_a(a)
        , party_b(b)
        , cycles_remaining(remaining)
        , type(agreement_type)
    {}
};

/**
 * @enum ExpirationCheckResult
 * @brief Result of checking a trade deal's expiration status.
 */
enum class ExpirationCheckResult : uint8_t {
    Active   = 0,   ///< Deal is active, no special status
    Warning  = 1,   ///< Deal is within warning threshold (<=5 cycles)
    Expired  = 2    ///< Deal has expired (0 cycles remaining)
};

/**
 * @brief Check the expiration status of a trade deal after ticking.
 *
 * Should be called after tick_trade_deal(). Examines cycles_remaining
 * and returns the appropriate status:
 * - Expired: agreement_type is None or cycles_remaining is 0
 * - Warning: cycles_remaining <= TRADE_DEAL_WARNING_CYCLES and > 0
 * - Active: cycles_remaining > TRADE_DEAL_WARNING_CYCLES
 *
 * @param agreement The agreement component to check.
 * @return ExpirationCheckResult indicating the deal's status.
 */
ExpirationCheckResult check_trade_deal_expiration(const TradeAgreementComponent& agreement);

/**
 * @brief Process one tick of a trade deal with expiration tracking.
 *
 * Combines tick_trade_deal() with expiration status checking.
 * Decrements cycles_remaining, checks for warning/expiration thresholds,
 * and populates output events as needed.
 *
 * @param agreement The agreement component to tick.
 * @param entity_id The entity ID of this agreement (for event emission).
 * @param[out] warning_event Populated if deal enters warning range. Check result == Warning.
 * @param[out] expired_event Populated if deal expires. Check result == Expired.
 * @return ExpirationCheckResult indicating what happened this tick.
 */
ExpirationCheckResult tick_trade_deal_with_expiration(
    TradeAgreementComponent& agreement,
    uint32_t entity_id,
    TradeDealExpirationWarningEvent& warning_event,
    TradeAgreementExpiredEvent& expired_event);

/**
 * @brief Renew an active trade deal, resetting its duration.
 *
 * Resets cycles_remaining to the default duration for the current tier.
 * The deal must still be active (agreement_type != None, cycles_remaining > 0).
 * The player must have sufficient treasury to cover at least one cycle cost.
 *
 * @param agreement The agreement component to renew.
 * @param player_treasury The renewing player's current treasury balance.
 * @return true if the deal was successfully renewed, false if it cannot be renewed.
 */
bool renew_trade_deal(TradeAgreementComponent& agreement, int64_t player_treasury);

} // namespace port
} // namespace sims3000
