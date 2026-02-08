/**
 * @file TradeOfferManager.h
 * @brief Inter-player trade offer system for Epic 8 (Ticket E8-025)
 *
 * Defines:
 * - TradeOffer: Data for a single pending trade offer between players
 * - TradeOfferManager: Server-authoritative manager for creating, accepting,
 *   rejecting, and expiring trade offers
 *
 * Trade offers allow players to propose trade agreements to each other.
 * Offers expire after TRADE_OFFER_EXPIRY_TICKS (500 ticks) if not acted upon.
 * The manager is server-authoritative: only the server creates/accepts offers.
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace port {

/// Number of ticks before a trade offer expires
constexpr uint32_t TRADE_OFFER_EXPIRY_TICKS = 500;

/**
 * @struct TradeOffer
 * @brief Data for a single trade offer between two players.
 *
 * Represents a proposed trade agreement from one player to another.
 * The offer has a limited lifetime and must be accepted or rejected
 * before it expires.
 */
struct TradeOffer {
    uint32_t offer_id;                  ///< Unique offer identifier
    uint8_t from_player;                ///< Offering player's PlayerID
    uint8_t to_player;                  ///< Target player's PlayerID
    TradeAgreementType proposed_type;   ///< Proposed trade agreement tier
    bool is_pending;                    ///< Whether the offer is still pending
    uint32_t created_tick;              ///< Tick when the offer was created
    uint32_t expiry_tick;               ///< Tick when the offer expires

    TradeOffer()
        : offer_id(0)
        , from_player(0)
        , to_player(0)
        , proposed_type(TradeAgreementType::None)
        , is_pending(false)
        , created_tick(0)
        , expiry_tick(0)
    {}

    TradeOffer(uint32_t id, uint8_t from, uint8_t to,
               TradeAgreementType type, uint32_t tick)
        : offer_id(id)
        , from_player(from)
        , to_player(to)
        , proposed_type(type)
        , is_pending(true)
        , created_tick(tick)
        , expiry_tick(tick + TRADE_OFFER_EXPIRY_TICKS)
    {}
};

/**
 * @class TradeOfferManager
 * @brief Server-authoritative manager for inter-player trade offers.
 *
 * Manages the lifecycle of trade offers:
 * 1. Player A creates an offer targeting Player B
 * 2. Player B receives notification (via TradeDealOfferReceivedEvent)
 * 3. Player B accepts or rejects the offer
 * 4. Offers that are not acted upon expire after TRADE_OFFER_EXPIRY_TICKS
 *
 * The manager is designed to be server-authoritative: all mutations
 * should go through this class to ensure consistency.
 */
class TradeOfferManager {
public:
    TradeOfferManager();

    /**
     * @brief Create a new trade offer from one player to another.
     *
     * Validates that:
     * - from and to are different players
     * - from and to are valid player IDs (not 0/GAME_MASTER)
     * - proposed_type is not None
     * - No duplicate pending offer exists from the same sender to the same target
     *
     * @param from The offering player's ID.
     * @param to The target player's ID.
     * @param type The proposed trade agreement tier.
     * @param current_tick The current simulation tick.
     * @return The new offer's ID on success, 0 on failure.
     */
    uint32_t create_offer(uint8_t from, uint8_t to,
                           TradeAgreementType type, uint32_t current_tick);

    /**
     * @brief Accept a pending trade offer.
     *
     * Validates that:
     * - The offer exists and is still pending
     * - The offer has not expired (current_tick < expiry_tick)
     *
     * On success, marks the offer as no longer pending.
     *
     * @param offer_id The ID of the offer to accept.
     * @param current_tick The current simulation tick.
     * @return true if the offer was successfully accepted, false otherwise.
     */
    bool accept_offer(uint32_t offer_id, uint32_t current_tick);

    /**
     * @brief Reject a pending trade offer.
     *
     * Marks the offer as no longer pending. Does not remove it from history.
     *
     * @param offer_id The ID of the offer to reject.
     * @return true if the offer was found and rejected, false otherwise.
     */
    bool reject_offer(uint32_t offer_id);

    /**
     * @brief Expire all offers that have passed their expiry tick.
     *
     * Should be called each simulation tick to clean up stale offers.
     * Marks expired offers as no longer pending.
     *
     * @param current_tick The current simulation tick.
     */
    void expire_offers(uint32_t current_tick);

    /**
     * @brief Get a specific offer by ID.
     *
     * @param offer_id The ID of the offer to look up.
     * @return Pointer to the offer if found, nullptr otherwise.
     */
    const TradeOffer* get_offer(uint32_t offer_id) const;

    /**
     * @brief Get all pending offers targeted at a specific player.
     *
     * Returns offers where to_player == player_id and is_pending == true.
     *
     * @param player_id The player to query pending offers for.
     * @return Vector of pending TradeOffer copies for the specified player.
     */
    std::vector<TradeOffer> get_pending_offers_for(uint8_t player_id) const;

    /**
     * @brief Get the total number of offers (including expired/rejected).
     *
     * @return Total offer count.
     */
    size_t get_offer_count() const;

    /**
     * @brief Get the number of currently pending offers.
     *
     * @return Pending offer count.
     */
    size_t get_pending_count() const;

private:
    std::vector<TradeOffer> offers_;
    uint32_t next_offer_id_;
};

} // namespace port
} // namespace sims3000
