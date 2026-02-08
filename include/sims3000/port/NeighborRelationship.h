/**
 * @file NeighborRelationship.h
 * @brief Neighbor relationship evolution for Epic 8 (Ticket E8-034)
 *
 * Tracks trade history per NPC neighbor and evolves relationships based on
 * cumulative trade activity. Higher relationships unlock better trade deal
 * tiers (TradeAgreementType).
 *
 * Relationship thresholds:
 * | Status   | Threshold  | Max Tier Available |
 * |----------|------------|-------------------|
 * | Hostile  | < -50      | None              |
 * | Cold     | -50 to -1  | Basic             |
 * | Neutral  | 0 to 24    | Basic             |
 * | Warm     | 25 to 49   | Enhanced          |
 * | Friendly | 50 to 79   | Enhanced          |
 * | Allied   | 80+        | Premium           |
 *
 * Depends: E8-015 (NPC neighbors)
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <cstdint>

namespace sims3000 {
namespace port {

/// Minimum relationship value
constexpr int32_t RELATIONSHIP_MIN = -100;

/// Maximum relationship value
constexpr int32_t RELATIONSHIP_MAX = 100;

/// Threshold: below this value = Hostile (no trade)
constexpr int32_t RELATIONSHIP_HOSTILE_MAX = -51;

/// Threshold: Cold range upper bound
constexpr int32_t RELATIONSHIP_COLD_MAX = -1;

/// Threshold: Neutral range upper bound
constexpr int32_t RELATIONSHIP_NEUTRAL_MAX = 24;

/// Threshold: Warm range upper bound
constexpr int32_t RELATIONSHIP_WARM_MAX = 49;

/// Threshold: Friendly range upper bound
constexpr int32_t RELATIONSHIP_FRIENDLY_MAX = 79;

/// Threshold: Allied minimum value
constexpr int32_t RELATIONSHIP_ALLIED_MIN = 80;

/**
 * @enum RelationshipStatus
 * @brief Named relationship tiers derived from relationship_value.
 */
enum class RelationshipStatus : uint8_t {
    Hostile  = 0,   ///< relationship_value < -50
    Cold     = 1,   ///< relationship_value -50 to -1
    Neutral  = 2,   ///< relationship_value 0 to 24
    Warm     = 3,   ///< relationship_value 25 to 49
    Friendly = 4,   ///< relationship_value 50 to 79
    Allied   = 5    ///< relationship_value 80+
};

/**
 * @brief Convert RelationshipStatus enum to a human-readable string.
 *
 * @param status The RelationshipStatus to convert.
 * @return Null-terminated string name of the relationship status.
 */
inline const char* relationship_status_to_string(RelationshipStatus status) {
    switch (status) {
        case RelationshipStatus::Hostile:  return "Hostile";
        case RelationshipStatus::Cold:     return "Cold";
        case RelationshipStatus::Neutral:  return "Neutral";
        case RelationshipStatus::Warm:     return "Warm";
        case RelationshipStatus::Friendly: return "Friendly";
        case RelationshipStatus::Allied:   return "Allied";
        default:                           return "Unknown";
    }
}

/**
 * @struct NeighborRelationship
 * @brief Tracks trade history and relationship with an NPC neighbor.
 *
 * Each NPC neighbor (from E8-015) has one NeighborRelationship instance.
 * Trade completions add relationship points; relationship level determines
 * what TradeAgreementType tiers are available for negotiation.
 */
struct NeighborRelationship {
    uint8_t neighbor_id = 0;             ///< NPC neighbor identifier (1-4)
    int32_t relationship_value = 0;      ///< Current relationship (-100 to +100)
    uint32_t total_trades = 0;           ///< Historical completed trade count
    int64_t total_trade_volume = 0;      ///< Cumulative credits traded
    TradeAgreementType max_available_tier = TradeAgreementType::Basic; ///< Unlocked by relationship
};

/**
 * @brief Update a neighbor's relationship value by adding points.
 *
 * Clamps the result to [RELATIONSHIP_MIN, RELATIONSHIP_MAX] (-100 to +100).
 * After updating the value, recalculates max_available_tier based on the
 * new relationship_value using get_max_available_tier().
 *
 * Each completed trade cycle should add positive points.
 * Failed deals or hostile actions add negative points.
 *
 * @param rel The neighbor relationship to update.
 * @param points Points to add (positive or negative).
 */
void update_relationship(NeighborRelationship& rel, int32_t points);

/**
 * @brief Get the maximum trade agreement tier available at a relationship value.
 *
 * Mapping:
 * - Hostile  (< -50):   None
 * - Cold     (-50 to -1): Basic
 * - Neutral  (0 to 24):   Basic
 * - Warm     (25 to 49):  Enhanced
 * - Friendly (50 to 79):  Enhanced
 * - Allied   (80+):       Premium
 *
 * @param relationship_value The current relationship value (-100 to +100).
 * @return The highest TradeAgreementType available at this relationship level.
 */
TradeAgreementType get_max_available_tier(int32_t relationship_value);

/**
 * @brief Get the RelationshipStatus enum for a relationship value.
 *
 * @param relationship_value The current relationship value (-100 to +100).
 * @return The RelationshipStatus corresponding to the value.
 */
RelationshipStatus get_relationship_status(int32_t relationship_value);

/**
 * @brief Record a completed trade and update relationship.
 *
 * Increments total_trades by 1, adds trade_volume to total_trade_volume,
 * and calls update_relationship with the specified relationship points.
 *
 * @param rel The neighbor relationship to update.
 * @param trade_volume The credit volume of the completed trade.
 * @param relationship_points Points to add from this trade completion.
 */
void record_trade(NeighborRelationship& rel, int64_t trade_volume, int32_t relationship_points);

} // namespace port
} // namespace sims3000
