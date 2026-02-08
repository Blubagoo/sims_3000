/**
 * @file PortIncomeUI.h
 * @brief Trade income breakdown UI data structures (Epic 8, Ticket E8-021)
 *
 * Provides detailed per-port and per-trade-deal income data for the UI
 * (future Epic 12). Includes historical income tracking over the last
 * 12 simulation phases.
 *
 * Structs:
 * - PortIncomeDetail: Per-port income data for display
 * - TradeIncomeUIData: Aggregate UI data with port details, breakdown, and history
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <sims3000/port/TradeIncome.h>
#include <array>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace port {

/// Number of historical income phases to track
constexpr std::size_t INCOME_HISTORY_SIZE = 12;

/**
 * @struct PortIncomeDetail
 * @brief Per-port income data for UI display.
 *
 * Contains the entity ID, port type, income contribution, capacity,
 * and utilization percentage for a single port facility.
 */
struct PortIncomeDetail {
    uint32_t entity_id = 0;         ///< Entity ID of the port facility
    PortType port_type = PortType::Aero; ///< Port facility type
    int64_t income = 0;             ///< Income contribution from this port (credits/phase)
    uint16_t capacity = 0;          ///< Current port capacity
    uint8_t utilization = 0;        ///< Utilization percentage (0-100)
};

/**
 * @struct TradeIncomeUIData
 * @brief Aggregate trade income data for UI display.
 *
 * Combines per-port detail, overall breakdown, and historical income
 * tracking for presentation in the budget/finance UI panel.
 */
struct TradeIncomeUIData {
    std::vector<PortIncomeDetail> port_details;             ///< Per-port income details
    TradeIncomeBreakdown breakdown;                         ///< Aggregate income breakdown
    std::array<int64_t, INCOME_HISTORY_SIZE> income_history = {}; ///< Last 12 phases of total income
};

} // namespace port
} // namespace sims3000
