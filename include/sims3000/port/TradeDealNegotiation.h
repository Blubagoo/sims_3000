/**
 * @file TradeDealNegotiation.h
 * @brief Trade deal negotiation with NPC neighbors for Epic 8 (Ticket E8-022)
 *
 * Defines:
 * - TradeDealConfig: Per-tier configuration for trade deal costs, income, and demand
 * - get_trade_deal_config(): Look up config for a given TradeAgreementType
 * - initiate_trade_deal(): Create a trade deal with an NPC neighbor
 * - downgrade_trade_deal(): Revert a deal to the next lower tier
 * - tick_trade_deal(): Process one simulation cycle for a trade deal
 *
 * Deal tiers and their parameters:
 * | Level    | Cost/Cycle | Income Multiplier | Demand Bonus | Default Duration |
 * |----------|------------|-------------------|--------------|------------------|
 * | None     | 0          | 0.5x (50%)        | +0           | 0                |
 * | Basic    | 1,000 cr   | 0.8x (80%)        | +5           | 500              |
 * | Enhanced | 2,500 cr   | 1.0x (100%)       | +10          | 1000             |
 * | Premium  | 5,000 cr   | 1.2x (120%)       | +15          | 1500             |
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <sims3000/core/types.h>
#include <cstdint>

namespace sims3000 {
namespace port {

/**
 * @struct TradeDealConfig
 * @brief Configuration for a single trade deal tier level.
 *
 * Each tier defines the per-cycle cost, income multiplier, demand bonus,
 * and default duration for trade deals at that level.
 */
struct TradeDealConfig {
    int32_t cost_per_cycle;      ///< Credits charged per simulation cycle
    float income_multiplier;     ///< Income multiplier (0.5x - 1.2x)
    int8_t demand_bonus;         ///< Demand bonus applied to player zones (+0 to +15)
    uint16_t default_duration;   ///< Default number of cycles the deal lasts
};

/**
 * @brief Get the trade deal configuration for a given agreement type.
 *
 * Returns the static config (cost, income multiplier, demand bonus, duration)
 * for each tier level. The returned config is from a static table and should
 * not be modified.
 *
 * @param type The trade agreement tier.
 * @return TradeDealConfig for the specified tier.
 */
TradeDealConfig get_trade_deal_config(TradeAgreementType type);

/**
 * @brief Initiate a trade deal between a player and an NPC neighbor.
 *
 * Validates that:
 * - The proposed type is not None
 * - The player has enough treasury to cover at least the first cycle cost
 * - The neighbor_id is valid (1-4)
 * - The player_id is valid (not GAME_MASTER)
 *
 * On success, populates the agreement component with the deal parameters:
 * - party_a = GAME_MASTER (0, NPC neighbor)
 * - party_b = player_id
 * - agreement_type = proposed_type
 * - neighbor_id = neighbor_id
 * - cycles_remaining = default_duration for the tier
 * - demand_bonus_b = config demand_bonus (player gets the bonus)
 * - income_bonus_percent = config income_multiplier * 100
 * - cost_per_cycle_b = config cost_per_cycle (player pays)
 *
 * @param agreement Output component to populate.
 * @param player_id The player initiating the deal (1-4).
 * @param neighbor_id The NPC neighbor identifier (1-4).
 * @param proposed_type The desired trade agreement tier.
 * @param player_treasury The player's current treasury balance.
 * @return true if the deal was successfully initiated, false otherwise.
 */
bool initiate_trade_deal(TradeAgreementComponent& agreement,
                          uint8_t player_id,
                          uint8_t neighbor_id,
                          TradeAgreementType proposed_type,
                          int64_t player_treasury);

/**
 * @brief Downgrade a trade deal to the next lower tier.
 *
 * Moves the agreement down one tier level:
 * - Premium -> Enhanced
 * - Enhanced -> Basic
 * - Basic -> None (deal effectively ends)
 *
 * Updates all deal parameters (cost, income, demand bonus) to match
 * the new tier, and resets cycles_remaining to the new tier's default.
 *
 * @param agreement The agreement component to downgrade.
 * @return The new TradeAgreementType after downgrade.
 */
TradeAgreementType downgrade_trade_deal(TradeAgreementComponent& agreement);

/**
 * @brief Process one simulation cycle for a trade deal.
 *
 * Decrements cycles_remaining by 1. If cycles reach 0, the deal
 * expires and agreement_type is set to None with zeroed parameters.
 *
 * @param agreement The agreement component to tick.
 * @return true if the deal is still active after this tick, false if expired.
 */
bool tick_trade_deal(TradeAgreementComponent& agreement);

} // namespace port
} // namespace sims3000
