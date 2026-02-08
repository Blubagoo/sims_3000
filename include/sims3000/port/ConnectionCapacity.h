/**
 * @file ConnectionCapacity.h
 * @brief External connection capacity calculation for Epic 8 (Ticket E8-014)
 *
 * Calculates trade and migration capacity for external connections:
 *
 * | Connection Type | Trade Capacity/tile | Migration Capacity/tile |
 * |-----------------|---------------------|-------------------------|
 * | Pathway         | 100                 | 50                      |
 * | Rail            | +200 bonus          | +25 bonus               |
 *
 * Rail connections provide a bonus to the connection itself. The ticket
 * specifies "Rail connections provide bonus to adjacent pathway connections"
 * which is handled at a higher system level; this function calculates
 * per-connection capacity including the rail bonus flag.
 *
 * Header-only implementation (pure logic, no external dependencies).
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/port/ExternalConnectionComponent.h>
#include <sims3000/port/PortTypes.h>
#include <cstdint>

namespace sims3000 {
namespace port {

/// Trade capacity per tile for Pathway connections
constexpr uint16_t PATHWAY_TRADE_CAPACITY_PER_TILE = 100;

/// Migration capacity per tile for Pathway connections
constexpr uint16_t PATHWAY_MIGRATION_CAPACITY_PER_TILE = 50;

/// Trade capacity bonus for Rail connections (added to pathway base)
constexpr uint16_t RAIL_TRADE_CAPACITY_BONUS = 200;

/// Migration capacity bonus for Rail connections (added to pathway base)
constexpr uint16_t RAIL_MIGRATION_CAPACITY_BONUS = 25;

/**
 * @brief Calculate and set trade/migration capacity for an external connection.
 *
 * For Pathway connections:
 *   trade_capacity = PATHWAY_TRADE_CAPACITY_PER_TILE (100)
 *   migration_capacity = PATHWAY_MIGRATION_CAPACITY_PER_TILE (50)
 *
 * For Rail connections:
 *   trade_capacity = PATHWAY_TRADE_CAPACITY_PER_TILE + RAIL_TRADE_CAPACITY_BONUS (300)
 *   migration_capacity = PATHWAY_MIGRATION_CAPACITY_PER_TILE + RAIL_MIGRATION_CAPACITY_BONUS (75)
 *
 * For Energy and Fluid connections:
 *   trade_capacity = 0 (these do not carry trade goods)
 *   migration_capacity = 0 (these do not carry population)
 *
 * @param conn The external connection component to update in-place.
 */
inline void calculate_connection_capacity(ExternalConnectionComponent& conn) {
    switch (conn.connection_type) {
        case ConnectionType::Pathway:
            conn.trade_capacity = PATHWAY_TRADE_CAPACITY_PER_TILE;
            conn.migration_capacity = PATHWAY_MIGRATION_CAPACITY_PER_TILE;
            break;

        case ConnectionType::Rail:
            conn.trade_capacity = PATHWAY_TRADE_CAPACITY_PER_TILE
                                + RAIL_TRADE_CAPACITY_BONUS;
            conn.migration_capacity = PATHWAY_MIGRATION_CAPACITY_PER_TILE
                                    + RAIL_MIGRATION_CAPACITY_BONUS;
            break;

        case ConnectionType::Energy:
        case ConnectionType::Fluid:
        default:
            conn.trade_capacity = 0;
            conn.migration_capacity = 0;
            break;
    }
}

/**
 * @brief Apply rail adjacency bonus to a pathway connection.
 *
 * When a Rail connection is adjacent to a Pathway connection along the
 * same map edge, the pathway gets the rail bonus added to its capacity.
 * This function should only be called on Pathway connections that have
 * an adjacent Rail connection.
 *
 * @param conn The pathway connection component to boost. Must be Pathway type.
 */
inline void apply_rail_adjacency_bonus(ExternalConnectionComponent& conn) {
    if (conn.connection_type != ConnectionType::Pathway) {
        return;
    }
    conn.trade_capacity = static_cast<uint16_t>(
        conn.trade_capacity + RAIL_TRADE_CAPACITY_BONUS);
    conn.migration_capacity = static_cast<uint16_t>(
        conn.migration_capacity + RAIL_MIGRATION_CAPACITY_BONUS);
}

} // namespace port
} // namespace sims3000
