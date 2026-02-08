/**
 * @file NeighborRelationship.cpp
 * @brief Neighbor relationship evolution implementation (Epic 8, Ticket E8-034)
 *
 * Implements relationship tracking and tier unlocking based on trade history.
 * Relationship values are clamped to [-100, +100] and determine which
 * TradeAgreementType tiers are available for negotiation.
 */

#include <sims3000/port/NeighborRelationship.h>
#include <algorithm>

namespace sims3000 {
namespace port {

// =============================================================================
// Relationship Value -> Status Mapping
// =============================================================================

RelationshipStatus get_relationship_status(int32_t relationship_value) {
    if (relationship_value < -50) {
        return RelationshipStatus::Hostile;
    } else if (relationship_value < 0) {
        return RelationshipStatus::Cold;
    } else if (relationship_value < 25) {
        return RelationshipStatus::Neutral;
    } else if (relationship_value < 50) {
        return RelationshipStatus::Warm;
    } else if (relationship_value < 80) {
        return RelationshipStatus::Friendly;
    } else {
        return RelationshipStatus::Allied;
    }
}

// =============================================================================
// Relationship Value -> Max Available Tier
// =============================================================================

TradeAgreementType get_max_available_tier(int32_t relationship_value) {
    if (relationship_value < -50) {
        // Hostile: no trade allowed
        return TradeAgreementType::None;
    } else if (relationship_value < 25) {
        // Cold or Neutral: only Basic
        return TradeAgreementType::Basic;
    } else if (relationship_value < 80) {
        // Warm or Friendly: up to Enhanced
        return TradeAgreementType::Enhanced;
    } else {
        // Allied: up to Premium
        return TradeAgreementType::Premium;
    }
}

// =============================================================================
// Update Relationship
// =============================================================================

void update_relationship(NeighborRelationship& rel, int32_t points) {
    // Add points and clamp to valid range
    int32_t new_value = rel.relationship_value + points;
    new_value = std::max(RELATIONSHIP_MIN, std::min(RELATIONSHIP_MAX, new_value));
    rel.relationship_value = new_value;

    // Recalculate max available tier
    rel.max_available_tier = get_max_available_tier(new_value);
}

// =============================================================================
// Record Trade
// =============================================================================

void record_trade(NeighborRelationship& rel, int64_t trade_volume, int32_t relationship_points) {
    rel.total_trades += 1;
    rel.total_trade_volume += trade_volume;
    update_relationship(rel, relationship_points);
}

} // namespace port
} // namespace sims3000
