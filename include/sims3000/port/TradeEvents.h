/**
 * @file TradeEvents.h
 * @brief Trade system event definitions for Epic 8 (Ticket E8-029)
 *
 * Defines all events emitted by the trade agreement system:
 * - TradeAgreementCreatedEvent: New trade agreement established between parties
 * - TradeAgreementExpiredEvent: Trade agreement expired or was cancelled
 * - TradeAgreementUpgradedEvent: Trade agreement tier level changed
 * - TradeDealOfferReceivedEvent: Incoming trade deal offer from another player
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <cstdint>

namespace sims3000 {
namespace port {

/**
 * @struct TradeAgreementCreatedEvent
 * @brief Event emitted when a new trade agreement is established.
 *
 * Emitted when two players/regions agree on a trade agreement,
 * enabling resource exchange between their cities.
 *
 * Consumed by:
 * - UISystem: Show trade agreement notification
 * - EconomySystem: Enable trade flows between parties
 * - StatisticsSystem: Track trade agreement creation
 * - AudioSystem: Play agreement established sound
 */
struct TradeAgreementCreatedEvent {
    uint32_t agreement;             ///< Trade agreement entity ID
    uint8_t party_a;                ///< First party PlayerID
    uint8_t party_b;                ///< Second party PlayerID
    TradeAgreementType type;        ///< Agreement tier level

    TradeAgreementCreatedEvent()
        : agreement(0)
        , party_a(0)
        , party_b(0)
        , type(TradeAgreementType::None)
    {}

    TradeAgreementCreatedEvent(uint32_t agreement_id, uint8_t a, uint8_t b,
                               TradeAgreementType agreement_type)
        : agreement(agreement_id)
        , party_a(a)
        , party_b(b)
        , type(agreement_type)
    {}
};

/**
 * @struct TradeAgreementExpiredEvent
 * @brief Event emitted when a trade agreement expires or is cancelled.
 *
 * Emitted when a trade agreement reaches its expiration date or when
 * one of the parties cancels the agreement.
 *
 * Consumed by:
 * - UISystem: Show trade agreement expiration notification
 * - EconomySystem: Disable trade flows between parties
 * - StatisticsSystem: Track trade agreement expiration
 */
struct TradeAgreementExpiredEvent {
    uint32_t agreement;     ///< Trade agreement entity ID
    uint8_t party_a;        ///< First party PlayerID
    uint8_t party_b;        ///< Second party PlayerID

    TradeAgreementExpiredEvent()
        : agreement(0)
        , party_a(0)
        , party_b(0)
    {}

    TradeAgreementExpiredEvent(uint32_t agreement_id, uint8_t a, uint8_t b)
        : agreement(agreement_id)
        , party_a(a)
        , party_b(b)
    {}
};

/**
 * @struct TradeAgreementUpgradedEvent
 * @brief Event emitted when a trade agreement's tier level changes.
 *
 * Emitted when a trade agreement is upgraded to a higher tier,
 * unlocking better terms, capacity, and resource sharing options.
 *
 * Consumed by:
 * - UISystem: Show upgrade notification with new tier details
 * - EconomySystem: Recalculate trade capacity limits
 * - StatisticsSystem: Track agreement upgrades
 */
struct TradeAgreementUpgradedEvent {
    uint32_t agreement;             ///< Trade agreement entity ID
    TradeAgreementType old_type;    ///< Previous agreement tier
    TradeAgreementType new_type;    ///< New agreement tier

    TradeAgreementUpgradedEvent()
        : agreement(0)
        , old_type(TradeAgreementType::None)
        , new_type(TradeAgreementType::None)
    {}

    TradeAgreementUpgradedEvent(uint32_t agreement_id,
                                TradeAgreementType old_tier,
                                TradeAgreementType new_tier)
        : agreement(agreement_id)
        , old_type(old_tier)
        , new_type(new_tier)
    {}
};

/**
 * @struct TradeDealOfferReceivedEvent
 * @brief Event emitted when a trade deal offer is received from another player.
 *
 * Emitted when another player proposes a new trade agreement or upgrade.
 * The receiving player can accept, reject, or counter-offer.
 *
 * Consumed by:
 * - UISystem: Display trade offer dialog/notification
 * - AudioSystem: Play incoming offer alert sound
 * - StatisticsSystem: Log trade offer for replay/debug
 */
struct TradeDealOfferReceivedEvent {
    uint32_t offer_id;                  ///< Unique offer identifier
    uint8_t from;                       ///< Offering player PlayerID
    TradeAgreementType proposed;        ///< Proposed agreement tier

    TradeDealOfferReceivedEvent()
        : offer_id(0)
        , from(0)
        , proposed(TradeAgreementType::None)
    {}

    TradeDealOfferReceivedEvent(uint32_t id, uint8_t from_player,
                                TradeAgreementType proposed_type)
        : offer_id(id)
        , from(from_player)
        , proposed(proposed_type)
    {}
};

} // namespace port
} // namespace sims3000
