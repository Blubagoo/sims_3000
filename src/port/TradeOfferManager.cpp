/**
 * @file TradeOfferManager.cpp
 * @brief Inter-player trade offer system implementation for Epic 8 (Ticket E8-025)
 *
 * Implements server-authoritative trade offer management:
 * - Offer creation with duplicate prevention
 * - Server-authoritative acceptance
 * - Rejection
 * - Time-based expiration
 */

#include <sims3000/port/TradeOfferManager.h>

namespace sims3000 {
namespace port {

// =============================================================================
// Constructor
// =============================================================================

TradeOfferManager::TradeOfferManager()
    : next_offer_id_(1)
{
}

// =============================================================================
// create_offer
// =============================================================================

uint32_t TradeOfferManager::create_offer(uint8_t from, uint8_t to,
                                          TradeAgreementType type,
                                          uint32_t current_tick) {
    // Validate: from and to must be different
    if (from == to) {
        return 0;
    }

    // Validate: neither can be GAME_MASTER (0)
    if (from == 0 || to == 0) {
        return 0;
    }

    // Validate: proposed type must not be None
    if (type == TradeAgreementType::None) {
        return 0;
    }

    // Check for duplicate pending offer from same sender to same target
    for (const auto& offer : offers_) {
        if (offer.is_pending &&
            offer.from_player == from &&
            offer.to_player == to) {
            return 0;
        }
    }

    // Create the offer
    uint32_t id = next_offer_id_++;
    offers_.emplace_back(id, from, to, type, current_tick);

    return id;
}

// =============================================================================
// accept_offer
// =============================================================================

bool TradeOfferManager::accept_offer(uint32_t offer_id, uint32_t current_tick) {
    for (auto& offer : offers_) {
        if (offer.offer_id == offer_id) {
            // Must be pending
            if (!offer.is_pending) {
                return false;
            }

            // Must not be expired
            if (current_tick >= offer.expiry_tick) {
                offer.is_pending = false;
                return false;
            }

            // Accept: mark as no longer pending
            offer.is_pending = false;
            return true;
        }
    }

    // Offer not found
    return false;
}

// =============================================================================
// reject_offer
// =============================================================================

bool TradeOfferManager::reject_offer(uint32_t offer_id) {
    for (auto& offer : offers_) {
        if (offer.offer_id == offer_id) {
            if (!offer.is_pending) {
                return false;
            }
            offer.is_pending = false;
            return true;
        }
    }

    // Offer not found
    return false;
}

// =============================================================================
// expire_offers
// =============================================================================

void TradeOfferManager::expire_offers(uint32_t current_tick) {
    for (auto& offer : offers_) {
        if (offer.is_pending && current_tick >= offer.expiry_tick) {
            offer.is_pending = false;
        }
    }
}

// =============================================================================
// get_offer
// =============================================================================

const TradeOffer* TradeOfferManager::get_offer(uint32_t offer_id) const {
    for (const auto& offer : offers_) {
        if (offer.offer_id == offer_id) {
            return &offer;
        }
    }
    return nullptr;
}

// =============================================================================
// get_pending_offers_for
// =============================================================================

std::vector<TradeOffer> TradeOfferManager::get_pending_offers_for(uint8_t player_id) const {
    std::vector<TradeOffer> result;
    for (const auto& offer : offers_) {
        if (offer.is_pending && offer.to_player == player_id) {
            result.push_back(offer);
        }
    }
    return result;
}

// =============================================================================
// get_offer_count
// =============================================================================

size_t TradeOfferManager::get_offer_count() const {
    return offers_.size();
}

// =============================================================================
// get_pending_count
// =============================================================================

size_t TradeOfferManager::get_pending_count() const {
    size_t count = 0;
    for (const auto& offer : offers_) {
        if (offer.is_pending) {
            ++count;
        }
    }
    return count;
}

} // namespace port
} // namespace sims3000
