/**
 * @file TradeIncome.cpp
 * @brief Trade income calculation implementation (Epic 8, Ticket E8-019)
 *
 * Calculates trade income from port operations and trade agreements:
 * - Aero ports: 0.8 credits/phase per utilized capacity unit
 * - Aqua ports: 0.6 credits/phase per utilized capacity unit
 * - Trade agreements modify income via multiplier
 *
 * @see TradeIncome.h for interface documentation.
 */

#include <sims3000/port/TradeIncome.h>
#include <sims3000/port/DemandBonus.h>
#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace port {

// =============================================================================
// Income Rate
// =============================================================================

float get_income_rate(PortType port_type) {
    switch (port_type) {
        case PortType::Aero: return AERO_INCOME_PER_UNIT;
        case PortType::Aqua: return AQUA_INCOME_PER_UNIT;
        default: return 0.0f;
    }
}

// =============================================================================
// Utilization Estimation
// =============================================================================

float estimate_port_utilization(const PortData& port) {
    if (!port.is_operational || port.capacity == 0) {
        return 0.0f;
    }

    // Capacity-based utilization estimate
    if (port.capacity < PORT_SIZE_MEDIUM_MIN) {
        return 0.5f;  // Small ports: 50% utilization
    }
    if (port.capacity <= PORT_SIZE_MEDIUM_MAX) {
        return 0.7f;  // Medium ports: 70% utilization
    }
    return 0.9f;      // Large ports: 90% utilization
}

// =============================================================================
// Trade Multiplier
// =============================================================================

float get_trade_multiplier(uint8_t owner,
                            const std::vector<TradeAgreementComponent>& agreements) {
    float best_multiplier = 0.0f;
    bool found = false;

    for (const auto& agreement : agreements) {
        // Check if this agreement applies to the owner
        bool applies = (agreement.party_a == owner || agreement.party_b == owner);
        if (!applies) continue;

        // Skip expired agreements
        if (agreement.cycles_remaining == 0 &&
            agreement.agreement_type != TradeAgreementType::None) {
            continue;
        }

        // Convert income_bonus_percent to multiplier (e.g., 120 -> 1.2)
        float multiplier = static_cast<float>(agreement.income_bonus_percent) / 100.0f;

        if (!found || multiplier > best_multiplier) {
            best_multiplier = multiplier;
            found = true;
        }
    }

    return found ? best_multiplier : DEFAULT_TRADE_MULTIPLIER;
}

// =============================================================================
// Trade Income Calculation
// =============================================================================

TradeIncomeBreakdown calculate_trade_income(uint8_t owner,
    const std::vector<PortData>& ports,
    const std::vector<TradeAgreementComponent>& agreements) {

    TradeIncomeBreakdown result;

    // Get the trade multiplier from agreements
    float trade_multiplier = get_trade_multiplier(owner, agreements);

    // Calculate income for each operational port
    float raw_aero_income = 0.0f;
    float raw_aqua_income = 0.0f;

    for (const auto& port : ports) {
        if (port.owner != owner) continue;
        if (!port.is_operational) continue;
        if (port.capacity == 0) continue;

        float utilization = estimate_port_utilization(port);
        float income_rate = get_income_rate(port.port_type);
        float external_demand_factor = DEFAULT_EXTERNAL_DEMAND_FACTOR;

        // port_trade_income = capacity * utilization * income_rate * external_demand_factor
        float base_income = static_cast<float>(port.capacity) *
                            utilization * income_rate * external_demand_factor;

        if (port.port_type == PortType::Aero) {
            raw_aero_income += base_income;
        } else {
            raw_aqua_income += base_income;
        }
    }

    // Apply trade multiplier to get final income
    float total_raw = raw_aero_income + raw_aqua_income;
    float total_with_multiplier = total_raw * trade_multiplier;

    // Calculate trade deal bonus as the difference from base (1.0x) income
    float trade_bonus = total_with_multiplier - total_raw;

    // Store as integer credits (floor for determinism)
    result.aero_income = static_cast<int64_t>(std::floor(raw_aero_income * trade_multiplier));
    result.aqua_income = static_cast<int64_t>(std::floor(raw_aqua_income * trade_multiplier));
    result.trade_deal_bonuses = static_cast<int64_t>(std::floor(trade_bonus));
    result.total = result.aero_income + result.aqua_income;

    return result;
}

} // namespace port
} // namespace sims3000
