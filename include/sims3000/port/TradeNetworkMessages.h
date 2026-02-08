/**
 * @file TradeNetworkMessages.h
 * @brief Network message definitions for inter-player trade (Ticket E8-026)
 *
 * Defines serializable network messages for trade route operations:
 *
 * Client -> Server:
 * - TradeOfferRequestMsg: Client proposes trade to another player
 * - TradeOfferResponseMsg: Client accepts/rejects a trade offer
 * - TradeCancelRequestMsg: Client requests cancellation of active trade route
 *
 * Server -> Client:
 * - TradeOfferNotificationMsg: Server notifies client of incoming offer
 * - TradeRouteEstablishedMsg: Server broadcasts new trade route
 * - TradeRouteCancelledMsg: Server broadcasts trade route cancellation
 *
 * All multi-byte fields use little-endian encoding.
 * Handles disconnection gracefully via status flags.
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace port {

/**
 * @enum TradeMessageType
 * @brief Message types for trade network operations.
 */
enum class TradeMessageType : uint8_t {
    OfferRequest        = 0,   ///< Client -> Server: propose trade
    OfferResponse       = 1,   ///< Client -> Server: accept/reject offer
    CancelRequest       = 2,   ///< Client -> Server: cancel trade route
    OfferNotification   = 3,   ///< Server -> Client: incoming offer
    RouteEstablished    = 4,   ///< Server -> Client: trade route created
    RouteCancelled      = 5    ///< Server -> Client: trade route cancelled
};

// ============================================================================
// Client -> Server Messages
// ============================================================================

/**
 * @struct TradeOfferRequestMsg
 * @brief Client requests a trade offer to another player.
 *
 * Sent when a player initiates a trade proposal targeting another player.
 * Server validates and creates the offer in TradeOfferManager.
 */
struct TradeOfferRequestMsg {
    uint8_t target_player = 0;      ///< Target player's PlayerID
    uint8_t proposed_type = 0;      ///< TradeAgreementType as uint8_t

    /// Serialized size: 1(target_player) + 1(proposed_type) = 2 bytes
    static constexpr size_t SERIALIZED_SIZE = 2;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static TradeOfferRequestMsg deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct TradeOfferResponseMsg
 * @brief Client responds to a trade offer (accept/reject).
 *
 * Sent when a player accepts or rejects an incoming trade offer.
 * Server validates the offer state and processes accordingly.
 */
struct TradeOfferResponseMsg {
    uint32_t offer_id = 0;          ///< ID of the offer being responded to
    bool accepted = false;          ///< true = accept, false = reject

    /// Serialized size: 4(offer_id) + 1(accepted) = 5 bytes
    static constexpr size_t SERIALIZED_SIZE = 5;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static TradeOfferResponseMsg deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct TradeCancelRequestMsg
 * @brief Client requests cancellation of an active trade route.
 *
 * Sent when a player wants to cancel an existing trade agreement.
 * Server validates ownership and cancels the route.
 */
struct TradeCancelRequestMsg {
    uint32_t route_id = 0;          ///< Trade route entity ID to cancel

    /// Serialized size: 4(route_id) = 4 bytes
    static constexpr size_t SERIALIZED_SIZE = 4;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static TradeCancelRequestMsg deserialize(const uint8_t* data, size_t len);
};

// ============================================================================
// Server -> Client Messages
// ============================================================================

/**
 * @struct TradeOfferNotificationMsg
 * @brief Server notifies a client of an incoming trade offer.
 *
 * Sent to the target player when another player proposes a trade.
 * Client should display a UI prompt for acceptance/rejection.
 */
struct TradeOfferNotificationMsg {
    uint32_t offer_id = 0;          ///< Unique offer identifier
    uint8_t from_player = 0;        ///< Offering player's PlayerID
    uint8_t proposed_type = 0;      ///< TradeAgreementType as uint8_t

    /// Serialized size: 4(offer_id) + 1(from_player) + 1(proposed_type) = 6 bytes
    static constexpr size_t SERIALIZED_SIZE = 6;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static TradeOfferNotificationMsg deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct TradeRouteEstablishedMsg
 * @brief Server broadcasts that a trade route has been established.
 *
 * Sent to both parties (and optionally all clients) when a trade
 * offer is accepted and the route becomes active.
 */
struct TradeRouteEstablishedMsg {
    uint32_t route_id = 0;          ///< Trade route entity ID
    uint8_t party_a = 0;            ///< First party PlayerID
    uint8_t party_b = 0;            ///< Second party PlayerID
    uint8_t agreement_type = 0;     ///< TradeAgreementType as uint8_t

    /// Serialized size: 4(route_id) + 1(party_a) + 1(party_b) + 1(agreement_type) = 7 bytes
    static constexpr size_t SERIALIZED_SIZE = 7;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static TradeRouteEstablishedMsg deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct TradeRouteCancelledMsg
 * @brief Server broadcasts that a trade route has been cancelled.
 *
 * Sent to relevant clients when a trade route is cancelled by
 * either party or due to disconnection.
 */
struct TradeRouteCancelledMsg {
    uint32_t route_id = 0;          ///< Trade route entity ID that was cancelled
    uint8_t cancelled_by = 0;       ///< PlayerID of the cancelling party (0 = server/disconnect)

    /// Serialized size: 4(route_id) + 1(cancelled_by) = 5 bytes
    static constexpr size_t SERIALIZED_SIZE = 5;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static TradeRouteCancelledMsg deserialize(const uint8_t* data, size_t len);
};

} // namespace port
} // namespace sims3000
