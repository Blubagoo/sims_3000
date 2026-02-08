/**
 * @file PortCapacity.h
 * @brief Port capacity calculation functions for Epic 8 (Ticket E8-010)
 *
 * Provides capacity calculation for aero and aqua ports:
 * - Aero capacity: zone_tiles * 10 * runway_bonus * access_bonus (cap 2500)
 * - Aqua capacity: zone_tiles * 15 * dock_bonus * water_access * rail_bonus (cap 5000)
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 * (aero_port, aqua_port - not airport/seaport)
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <sims3000/port/PortZoneComponent.h>
#include <cstdint>

namespace sims3000 {
namespace port {

/// Maximum capacity for aero ports
constexpr uint16_t AERO_PORT_MAX_CAPACITY = 2500;

/// Maximum capacity for aqua ports
constexpr uint16_t AQUA_PORT_MAX_CAPACITY = 5000;

/// Base capacity multiplier per zone tile for aero ports
constexpr uint16_t AERO_BASE_CAPACITY_PER_TILE = 10;

/// Base capacity multiplier per zone tile for aqua ports
constexpr uint16_t AQUA_BASE_CAPACITY_PER_TILE = 15;

/// Runway bonus multiplier when runway is present
constexpr float AERO_RUNWAY_BONUS = 1.5f;

/// Runway bonus multiplier when runway is absent
constexpr float AERO_NO_RUNWAY_BONUS = 0.5f;

/// Access bonus when pathway is connected
constexpr float AERO_ACCESS_CONNECTED = 1.0f;

/// Access bonus when pathway is not connected (no capacity)
constexpr float AERO_ACCESS_DISCONNECTED = 0.0f;

/// Dock bonus base value
constexpr float AQUA_DOCK_BONUS_BASE = 1.0f;

/// Dock bonus increment per dock
constexpr float AQUA_DOCK_BONUS_PER_DOCK = 0.2f;

/// Water access bonus when sufficient adjacent water (>= 4)
constexpr float AQUA_WATER_ACCESS_FULL = 1.0f;

/// Water access bonus when insufficient adjacent water (< 4)
constexpr float AQUA_WATER_ACCESS_PARTIAL = 0.5f;

/// Minimum adjacent water tiles for full water access bonus
constexpr uint8_t AQUA_MIN_ADJACENT_WATER = 4;

/// Rail bonus when rail connection is present
constexpr float AQUA_RAIL_BONUS_CONNECTED = 1.5f;

/// Rail bonus when rail connection is absent
constexpr float AQUA_RAIL_BONUS_DISCONNECTED = 1.0f;

/**
 * @brief Calculate aero port capacity.
 *
 * Formula:
 *   base_capacity = zone_tiles * 10
 *   runway_bonus = has_runway ? 1.5 : 0.5
 *   access_bonus = pathway_connected ? 1.0 : 0.0
 *   aero_capacity = base_capacity * runway_bonus * access_bonus
 *   capped at AERO_PORT_MAX_CAPACITY (2500)
 *
 * @param zone The port zone component with zone metadata.
 * @param pathway_connected Whether the port has pathway connectivity.
 * @return Calculated capacity (0 to AERO_PORT_MAX_CAPACITY).
 */
uint16_t calculate_aero_capacity(const PortZoneComponent& zone,
                                  bool pathway_connected);

/**
 * @brief Calculate aqua port capacity.
 *
 * Formula:
 *   base_capacity = zone_tiles * 15
 *   dock_bonus = 1.0 + (dock_count * 0.2)
 *   water_access = adjacent_water >= 4 ? 1.0 : 0.5
 *   rail_bonus = has_rail_connection ? 1.5 : 1.0
 *   aqua_capacity = base_capacity * dock_bonus * water_access * rail_bonus
 *   capped at AQUA_PORT_MAX_CAPACITY (5000)
 *
 * @param zone The port zone component with zone metadata.
 * @param adjacent_water Number of water-adjacent perimeter tiles.
 * @param has_rail_connection Whether the port has a rail connection.
 * @return Calculated capacity (0 to AQUA_PORT_MAX_CAPACITY).
 */
uint16_t calculate_aqua_capacity(const PortZoneComponent& zone,
                                  uint32_t adjacent_water,
                                  bool has_rail_connection);

/**
 * @brief Calculate capacity for any port type.
 *
 * Dispatches to calculate_aero_capacity or calculate_aqua_capacity
 * based on the port zone's port_type.
 *
 * @param zone The port zone component.
 * @param pathway_connected Whether the port has pathway connectivity.
 * @param adjacent_water Number of water-adjacent perimeter tiles (aqua only).
 * @param has_rail_connection Whether the port has a rail connection (aqua only).
 * @return Calculated capacity capped per port type.
 */
uint16_t calculate_port_capacity(const PortZoneComponent& zone,
                                  bool pathway_connected,
                                  uint32_t adjacent_water,
                                  bool has_rail_connection);

/**
 * @brief Get the maximum capacity cap for a port type.
 *
 * @param type The port type.
 * @return Maximum capacity: 2500 for aero, 5000 for aqua.
 */
uint16_t get_max_capacity(PortType type);

} // namespace port
} // namespace sims3000
