/**
 * @file TradeAgreementComponent.h
 * @brief Trade agreement component structure for Epic 8 (Ticket E8-005)
 *
 * Defines:
 * - TradeAgreementComponent: Per-agreement data for inter-city/NPC trade deals
 *
 * Each trade agreement represents a deal between two parties (players or
 * NPC neighbors controlled by GAME_MASTER) that modifies trade capacity,
 * demand bonuses, and income modifiers for a limited duration.
 *
 * Supports:
 * - NPC neighbors: party_a = GAME_MASTER (0)
 * - Inter-player trade: both parties are player IDs (1-4)
 * - Duration tracking via cycles_remaining for deal expiration
 */

#pragma once

#include <cstdint>
#include <type_traits>
#include <sims3000/port/PortTypes.h>
#include <sims3000/core/types.h>

namespace sims3000 {
namespace port {

/**
 * @struct TradeAgreementComponent
 * @brief Per-agreement data for inter-city/NPC trade deals (16 bytes).
 *
 * Tracks the parties involved, agreement tier, duration, demand bonuses,
 * income modifiers, and per-cycle costs for each active trade agreement.
 *
 * Layout (16 bytes, packed):
 * - party_a:              1 byte  (PlayerID/uint8_t)          - first party (0 = GAME_MASTER/NPC)
 * - party_b:              1 byte  (PlayerID/uint8_t)          - second party
 * - agreement_type:       1 byte  (TradeAgreementType/uint8_t)- deal tier
 * - neighbor_id:          1 byte  (uint8_t)                   - NPC neighbor identifier
 * - cycles_remaining:     2 bytes (uint16_t)                  - ticks until expiration
 * - demand_bonus_a:       1 byte  (int8_t)                    - demand modifier for party A
 * - demand_bonus_b:       1 byte  (int8_t)                    - demand modifier for party B
 * - income_bonus_percent: 1 byte  (uint8_t)                   - income multiplier (100 = 1.0x)
 * - padding:              1 byte  (uint8_t)                   - alignment padding
 * - cost_per_cycle_a:     4 bytes (int32_t)                   - cost charged to party A per tick
 * - cost_per_cycle_b:     2 bytes (int16_t)                   - cost charged to party B per tick
 *
 * Total: 16 bytes (requires packing due to int32_t at offset 10)
 */
#pragma pack(push, 1)
struct TradeAgreementComponent {
    PlayerID party_a                       = 0;   ///< First party player ID (0 = GAME_MASTER/NPC neighbor)
    PlayerID party_b                       = 0;   ///< Second party player ID
    TradeAgreementType agreement_type      = TradeAgreementType::None; ///< Deal tier level
    uint8_t neighbor_id                    = 0;   ///< NPC neighbor identifier (when party is GAME_MASTER)
    uint16_t cycles_remaining              = 0;   ///< Simulation ticks until deal expires (0 = expired)
    int8_t demand_bonus_a                  = 0;   ///< Demand bonus applied to party A's zones
    int8_t demand_bonus_b                  = 0;   ///< Demand bonus applied to party B's zones
    uint8_t income_bonus_percent           = 100; ///< Income multiplier as percentage (100 = 1.0x, 150 = 1.5x)
    uint8_t padding                        = 0;   ///< Alignment padding
    int32_t cost_per_cycle_a               = 0;   ///< Credits charged to party A per simulation tick
    int16_t cost_per_cycle_b               = 0;   ///< Credits charged to party B per simulation tick
};
#pragma pack(pop)

// Verify TradeAgreementComponent size (16 bytes)
static_assert(sizeof(TradeAgreementComponent) == 16,
    "TradeAgreementComponent must be 16 bytes");

// Verify TradeAgreementComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<TradeAgreementComponent>::value,
    "TradeAgreementComponent must be trivially copyable");

} // namespace port
} // namespace sims3000
