/**
 * @file TradeIncome.h
 * @brief Trade income calculation from ports and external connections (Epic 8, Ticket E8-019)
 *
 * Calculates trade income from port facilities and trade agreements:
 *
 * port_trade_income = capacity * utilization * trade_multiplier * external_demand_factor
 *
 * Income rates per unit of utilized capacity:
 * - Aero ports: 0.8 credits/phase
 * - Aqua ports: 0.6 credits/phase
 *
 * Trade agreements apply an income multiplier via income_bonus_percent:
 * - None:     0.5x (50%)
 * - Basic:    0.8x (80%)
 * - Enhanced: 1.0x (100%)
 * - Premium:  1.2x (120%)
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/port/DemandBonus.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace port {

/// Income per unit of utilized capacity for aero ports (credits/phase)
constexpr float AERO_INCOME_PER_UNIT = 0.8f;

/// Income per unit of utilized capacity for aqua ports (credits/phase)
constexpr float AQUA_INCOME_PER_UNIT = 0.6f;

/// Default external demand factor when no neighbors/agreements exist
constexpr float DEFAULT_EXTERNAL_DEMAND_FACTOR = 1.0f;

/// Default trade multiplier (no agreements)
constexpr float DEFAULT_TRADE_MULTIPLIER = 1.0f;

/**
 * @struct TradeIncomeBreakdown
 * @brief Detailed breakdown of trade income by source.
 *
 * Provides per-source income values and a total for a single player.
 */
struct TradeIncomeBreakdown {
    int64_t aero_income = 0;           ///< Income from aero port operations
    int64_t aqua_income = 0;           ///< Income from aqua port operations
    int64_t trade_deal_bonuses = 0;    ///< Additional income from trade agreements
    int64_t total = 0;                 ///< Total trade income (sum of all sources)
};

/**
 * @brief Calculate the income rate per unit for a given port type.
 *
 * @param port_type The port type (Aero or Aqua).
 * @return Income rate per unit of utilized capacity (credits/phase).
 */
float get_income_rate(PortType port_type);

/**
 * @brief Calculate effective utilization for a port.
 *
 * Maps the port's capacity to a simulated utilization factor:
 * - Ports with 0 capacity have 0 utilization
 * - Non-operational ports have 0 utilization
 * - Operational ports use a utilization estimate based on capacity tiers:
 *   - Small (< 500):    0.5 utilization
 *   - Medium (500-1999): 0.7 utilization
 *   - Large (>= 2000):   0.9 utilization
 *
 * This provides a reasonable default until full demand/supply tracking
 * is implemented in later epics.
 *
 * @param port The port data to evaluate.
 * @return Utilization factor (0.0 to 1.0).
 */
float estimate_port_utilization(const PortData& port);

/**
 * @brief Calculate the aggregate trade multiplier from active trade agreements.
 *
 * Takes the best (highest) income_bonus_percent from all active agreements
 * that apply to the given owner. Converts from percentage to multiplier
 * (e.g., 120 -> 1.2x).
 *
 * If no agreements apply, returns DEFAULT_TRADE_MULTIPLIER (1.0).
 *
 * @param owner Owner player ID.
 * @param agreements Vector of all trade agreements.
 * @return Trade multiplier (0.5 to 1.2 typically).
 */
float get_trade_multiplier(uint8_t owner,
                            const std::vector<TradeAgreementComponent>& agreements);

/**
 * @brief Calculate trade income from ports and trade agreements.
 *
 * For each operational port owned by the player:
 *   income += capacity * utilization * income_rate * trade_multiplier * external_demand_factor
 *
 * Trade deal bonuses are calculated as the difference between income with
 * the trade multiplier and income without it (i.e., the portion of income
 * attributable to trade agreements).
 *
 * @param owner Owner player ID.
 * @param ports Vector of all port data.
 * @param agreements Vector of all trade agreements.
 * @return TradeIncomeBreakdown with per-source and total income.
 */
TradeIncomeBreakdown calculate_trade_income(uint8_t owner,
    const std::vector<PortData>& ports,
    const std::vector<TradeAgreementComponent>& agreements);

} // namespace port
} // namespace sims3000
