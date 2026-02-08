/**
 * @file TradeAgreementBenefits.h
 * @brief Inter-player trade agreement benefits for Epic 8 (Ticket E8-027)
 *
 * Defines:
 * - TradeAgreementBenefits: Per-tier demand and income bonuses
 * - get_agreement_benefits(): Look up benefits for a given TradeAgreementType
 * - apply_trade_agreement_income_bonus(): Calculate income with agreement bonuses
 *
 * Benefits table (symmetric for both players):
 * | TradeAgreementType | Demand Bonus (each) | Income Bonus |
 * |--------------------|---------------------|--------------|
 * | None               | +0                  | +0%          |
 * | Basic              | +3                  | +5%          |
 * | Enhanced           | +6                  | +10%         |
 * | Premium            | +10                 | +15%         |
 *
 * Maps ticket terminology:
 * - "Basic Trade"           -> TradeAgreementType::Basic
 * - "Advanced Trade"        -> TradeAgreementType::Enhanced
 * - "Strategic Partnership" -> TradeAgreementType::Premium
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace port {

/**
 * @struct TradeAgreementBenefits
 * @brief Per-tier benefits for inter-player trade agreements.
 *
 * Both players in the agreement receive these symmetric bonuses:
 * - demand_bonus: Applied to relevant zones for both parties
 * - income_bonus_percent: Added to base 100% for port trade income
 */
struct TradeAgreementBenefits {
    int8_t demand_bonus;            ///< Demand bonus applied to both players' zones
    uint8_t income_bonus_percent;   ///< Income bonus percentage (added to base 100%)
};

/**
 * @brief Get the inter-player trade agreement benefits for a given tier.
 *
 * Returns the symmetric benefits that both players receive from
 * an inter-player trade agreement at the specified tier.
 *
 * @param type The trade agreement tier.
 * @return TradeAgreementBenefits for the specified tier.
 */
inline TradeAgreementBenefits get_agreement_benefits(TradeAgreementType type) {
    switch (type) {
        case TradeAgreementType::Basic:
            return { 3, 5 };    // +3 demand, +5% income
        case TradeAgreementType::Enhanced:
            return { 6, 10 };   // +6 demand, +10% income
        case TradeAgreementType::Premium:
            return { 10, 15 };  // +10 demand, +15% income
        case TradeAgreementType::None:
        default:
            return { 0, 0 };   // No benefits
    }
}

/**
 * @brief Calculate total demand bonus from all active trade agreements for a player.
 *
 * Sums the demand_bonus from all agreements where the specified player
 * is either party_a or party_b and the agreement is active (type != None).
 *
 * @param agreements Vector of active trade agreement components.
 * @param owner The player ID to calculate bonuses for.
 * @return Total demand bonus from all qualifying agreements.
 */
inline int16_t calculate_total_demand_bonus(
    const std::vector<TradeAgreementComponent>& agreements,
    uint8_t owner) {

    int16_t total = 0;
    for (const auto& agreement : agreements) {
        // Skip inactive agreements
        if (agreement.agreement_type == TradeAgreementType::None) {
            continue;
        }
        // Check if this player is part of this agreement
        if (agreement.party_a == owner || agreement.party_b == owner) {
            TradeAgreementBenefits benefits = get_agreement_benefits(agreement.agreement_type);
            total += benefits.demand_bonus;
        }
    }
    return total;
}

/**
 * @brief Apply trade agreement income bonuses to base port income.
 *
 * Calculates the modified income by summing all income bonus percentages
 * from active agreements where the player is a party, then applying
 * them to the base income.
 *
 * Formula: modified_income = base_income * (100 + sum_of_bonuses) / 100
 *
 * @param base_income The base port trade income before bonuses.
 * @param agreements Vector of active trade agreement components.
 * @param owner The player ID whose income is being calculated.
 * @return The modified income after applying all agreement bonuses.
 */
inline int64_t apply_trade_agreement_income_bonus(
    int64_t base_income,
    const std::vector<TradeAgreementComponent>& agreements,
    uint8_t owner) {

    int32_t total_bonus_percent = 0;

    for (const auto& agreement : agreements) {
        // Skip inactive agreements
        if (agreement.agreement_type == TradeAgreementType::None) {
            continue;
        }
        // Check if this player is part of this agreement
        if (agreement.party_a == owner || agreement.party_b == owner) {
            TradeAgreementBenefits benefits = get_agreement_benefits(agreement.agreement_type);
            total_bonus_percent += benefits.income_bonus_percent;
        }
    }

    // Apply: base_income * (100 + total_bonus) / 100
    return base_income * (100 + total_bonus_percent) / 100;
}

} // namespace port
} // namespace sims3000
