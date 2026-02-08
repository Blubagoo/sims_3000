/**
 * @file PortTypes.h
 * @brief Port and external connection enumerations for Epic 8 (Tickets E8-001, E8-004)
 *
 * Defines:
 * - PortType enum: Port facility types (aero_port, aqua_port)
 * - MapEdge enum: Map edge sides for external connections
 * - ConnectionType enum: External connection pathway types
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 * (aero_port, aqua_port - not airport/seaport)
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace port {

/**
 * @enum PortType
 * @brief Port facility types.
 *
 * Defines the different classes of port facilities that provide
 * external trade connections. Each port type boosts demand for
 * specific zone types.
 *
 * Uses alien terminology:
 * - "aero_port" instead of "airport"
 * - "aqua_port" instead of "seaport"
 */
enum class PortType : uint8_t {
    Aero = 0,   ///< aero_port (airport) - boosts exchange demand
    Aqua = 1    ///< aqua_port (seaport) - boosts fabrication demand
};

/// Total number of port types
constexpr uint8_t PORT_TYPE_COUNT = 2;

/**
 * @enum MapEdge
 * @brief Map edge sides for external connections.
 *
 * Identifies which edge of the map an external connection is
 * located on. Used by ExternalConnectionComponent to position
 * connections along the map boundary.
 */
enum class MapEdge : uint8_t {
    North = 0,  ///< Northern map edge
    East  = 1,  ///< Eastern map edge
    South = 2,  ///< Southern map edge
    West  = 3   ///< Western map edge
};

/// Total number of map edges
constexpr uint8_t MAP_EDGE_COUNT = 4;

/**
 * @enum ConnectionType
 * @brief External connection pathway types.
 *
 * Defines what kind of infrastructure an external connection
 * represents. Each type enables different kinds of cross-map
 * trade and migration flows.
 */
enum class ConnectionType : uint8_t {
    Pathway = 0,   ///< Surface pathway connection (road equivalent)
    Rail    = 1,   ///< Rail connection
    Energy  = 2,   ///< Energy grid connection (power import/export)
    Fluid   = 3    ///< Fluid network connection (water import/export)
};

/// Total number of connection types
constexpr uint8_t CONNECTION_TYPE_COUNT = 4;

/**
 * @brief Convert PortType enum to a human-readable string.
 *
 * @param type The PortType to convert.
 * @return Null-terminated string name of the port type.
 */
inline const char* port_type_to_string(PortType type) {
    switch (type) {
        case PortType::Aero: return "Aero";
        case PortType::Aqua: return "Aqua";
        default:             return "Unknown";
    }
}

/**
 * @brief Convert MapEdge enum to a human-readable string.
 *
 * @param edge The MapEdge to convert.
 * @return Null-terminated string name of the map edge.
 */
inline const char* map_edge_to_string(MapEdge edge) {
    switch (edge) {
        case MapEdge::North: return "North";
        case MapEdge::East:  return "East";
        case MapEdge::South: return "South";
        case MapEdge::West:  return "West";
        default:             return "Unknown";
    }
}

/**
 * @brief Convert ConnectionType enum to a human-readable string.
 *
 * @param type The ConnectionType to convert.
 * @return Null-terminated string name of the connection type.
 */
inline const char* connection_type_to_string(ConnectionType type) {
    switch (type) {
        case ConnectionType::Pathway: return "Pathway";
        case ConnectionType::Rail:    return "Rail";
        case ConnectionType::Energy:  return "Energy";
        case ConnectionType::Fluid:   return "Fluid";
        default:                      return "Unknown";
    }
}

/**
 * @enum TradeAgreementType
 * @brief Tier level of a trade agreement between players/regions.
 *
 * Trade agreements unlock progressively better trade terms,
 * capacity, and resource sharing options.
 */
enum class TradeAgreementType : uint8_t {
    None     = 0,   ///< No trade agreement
    Basic    = 1,   ///< Basic trade agreement (limited capacity)
    Enhanced = 2,   ///< Enhanced trade agreement (increased capacity)
    Premium  = 3    ///< Premium trade agreement (maximum capacity)
};

/// Total number of trade agreement types (excluding None)
constexpr uint8_t TRADE_AGREEMENT_TYPE_COUNT = 4;

/**
 * @brief Convert TradeAgreementType enum to a human-readable string.
 *
 * @param type The TradeAgreementType to convert.
 * @return Null-terminated string name of the trade agreement type.
 */
inline const char* trade_agreement_type_to_string(TradeAgreementType type) {
    switch (type) {
        case TradeAgreementType::None:     return "None";
        case TradeAgreementType::Basic:    return "Basic";
        case TradeAgreementType::Enhanced: return "Enhanced";
        case TradeAgreementType::Premium:  return "Premium";
        default:                           return "Unknown";
    }
}

} // namespace port
} // namespace sims3000
