/**
 * @file ForwardDependencyInterfaces.h
 * @brief Forward dependency interfaces for BuildingSystem (Epic 4)
 *
 * Defines eight pure abstract interfaces that represent dependencies on systems
 * implemented in later epics. These interfaces enable BuildingSystem and
 * ZoneSystem to be developed and tested independently of future epics.
 *
 * Interfaces:
 * - IEnergyProvider: Power state queries (Epic 5)
 * - IFluidProvider: Water/fluid state queries (Epic 6)
 * - ITransportProvider: Pathway connectivity queries (Epic 7)
 * - IPortProvider: Port facility and trade queries (Epic 8)
 * - IServiceQueryable: Service coverage/effectiveness queries (Epic 9)
 * - ILandValueProvider: Sector desirability queries (Epic 10)
 * - IDemandProvider: Zone growth pressure queries (Epic 10)
 * - ICreditProvider: Treasury/credit deduction (Epic 11)
 *
 * All interfaces are pure abstract with virtual destructors. Stub implementations
 * (ticket 4-020) provide permissive defaults for testing.
 *
 * @see /docs/canon/interfaces.yaml (Epic 4 forward dependency stubs)
 * @see /docs/epics/epic-4/tickets.md (ticket 4-019)
 */

#ifndef SIMS3000_BUILDING_FORWARDDEPENDENCYINTERFACES_H
#define SIMS3000_BUILDING_FORWARDDEPENDENCYINTERFACES_H

#include <cstdint>

namespace sims3000 {
namespace building {

// Forward declarations
enum class ZoneBuildingType : std::uint8_t;

/**
 * @interface IEnergyProvider
 * @brief Power state query interface (Epic 5 dependency)
 *
 * Allows BuildingSystem to query whether entities or tiles have power.
 * Will be implemented by EnergySystem in Epic 5.
 */
class IEnergyProvider {
public:
    /// Virtual destructor for proper polymorphic cleanup
    virtual ~IEnergyProvider() = default;

    /**
     * @brief Check if entity is currently powered.
     * @param entity_id The entity to check.
     * @return true if entity is receiving power, false otherwise.
     */
    virtual bool is_powered(std::uint32_t entity_id) const = 0;

    /**
     * @brief Check if position has power coverage and surplus.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param player_id Owner player ID.
     * @return true if position has power coverage AND pool has surplus.
     */
    virtual bool is_powered_at(std::uint32_t x, std::uint32_t y, std::uint32_t player_id) const = 0;
};

/**
 * @interface IFluidProvider
 * @brief Fluid/water state query interface (Epic 6 dependency)
 *
 * Allows BuildingSystem to query whether entities or tiles have fluid (water).
 * Will be implemented by FluidSystem in Epic 6.
 */
class IFluidProvider {
public:
    /// Virtual destructor for proper polymorphic cleanup
    virtual ~IFluidProvider() = default;

    /**
     * @brief Check if entity is currently receiving fluid.
     * @param entity_id The entity to check.
     * @return true if entity is receiving fluid, false otherwise.
     */
    virtual bool has_fluid(std::uint32_t entity_id) const = 0;

    /**
     * @brief Check if position has fluid coverage and surplus.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param player_id Owner player ID.
     * @return true if position has fluid coverage AND pool has surplus.
     */
    virtual bool has_fluid_at(std::uint32_t x, std::uint32_t y, std::uint32_t player_id) const = 0;
};

/**
 * @interface ITransportProvider
 * @brief Pathway connectivity query interface (Epic 7 dependency)
 *
 * Allows BuildingSystem to query pathway (road) proximity and connectivity.
 * Will be implemented by TransportSystem in Epic 7.
 *
 * Original methods (Epic 4): is_road_accessible_at, get_nearest_road_distance
 * Extended methods (Epic 7, Ticket E7-016): is_road_accessible, is_connected_to_network,
 *   are_connected, get_congestion_at, get_traffic_volume_at, get_network_id_at
 */
class ITransportProvider {
public:
    /// EntityID type alias for clarity
    using EntityID = std::uint32_t;

    /// Virtual destructor for proper polymorphic cleanup
    virtual ~ITransportProvider() = default;

    // =========================================================================
    // Original methods (Epic 4)
    // =========================================================================

    /**
     * @brief Check if position is within max_distance of a pathway.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param max_distance Maximum distance in tiles (default 3 for building spawn rule).
     * @return true if within max_distance tiles of a pathway.
     */
    virtual bool is_road_accessible_at(std::uint32_t x, std::uint32_t y, std::uint32_t max_distance) const = 0;

    /**
     * @brief Get distance to nearest pathway.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Distance to nearest pathway (0 if adjacent, 255 if none).
     */
    virtual std::uint32_t get_nearest_road_distance(std::uint32_t x, std::uint32_t y) const = 0;

    // =========================================================================
    // Extended methods (Epic 7, Ticket E7-016)
    // Default implementations provided so existing implementors are not broken.
    // TransportSystem will override these with real implementations.
    // =========================================================================

    /**
     * @brief Check if an entity (building) has road access.
     * @param entity The entity ID to check.
     * @return true if the entity is adjacent to a road tile.
     */
    virtual bool is_road_accessible(EntityID entity) const {
        (void)entity;
        return true; // Permissive default
    }

    /**
     * @brief Check if a position is connected to any road network.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if the position is part of or adjacent to a road network.
     */
    virtual bool is_connected_to_network(std::int32_t x, std::int32_t y) const {
        (void)x; (void)y;
        return true; // Permissive default
    }

    /**
     * @brief Check if two positions are connected via the road network.
     * @param x1 X coordinate of first position.
     * @param y1 Y coordinate of first position.
     * @param x2 X coordinate of second position.
     * @param y2 Y coordinate of second position.
     * @return true if both positions are in the same connected road network component.
     */
    virtual bool are_connected(std::int32_t x1, std::int32_t y1,
                               std::int32_t x2, std::int32_t y2) const {
        (void)x1; (void)y1; (void)x2; (void)y2;
        return true; // Permissive default
    }

    /**
     * @brief Get congestion level at a position.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Congestion factor (0.0 = no congestion, 1.0 = fully congested).
     */
    virtual float get_congestion_at(std::int32_t x, std::int32_t y) const {
        (void)x; (void)y;
        return 0.0f; // No congestion default
    }

    /**
     * @brief Get traffic volume at a position.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Traffic volume (0 = no traffic, higher = more traffic).
     */
    virtual std::uint32_t get_traffic_volume_at(std::int32_t x, std::int32_t y) const {
        (void)x; (void)y;
        return 0; // No traffic default
    }

    /**
     * @brief Get the network component ID at a position.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Network ID (0 = not part of any network, >0 = network component ID).
     */
    virtual std::uint16_t get_network_id_at(std::int32_t x, std::int32_t y) const {
        (void)x; (void)y;
        return 0; // Not in any network default
    }
};

/**
 * @interface IPortProvider
 * @brief Port facility and trade query interface (Epic 8 dependency)
 *
 * Allows BuildingSystem and ZoneSystem to query port capacity, utilization,
 * demand bonuses, external connections, and trade income.
 * Will be implemented by PortSystem in Epic 8.
 *
 * All enum parameters use uint8_t to avoid circular includes,
 * matching the pattern used by other interfaces in this file.
 */
class IPortProvider {
public:
    /// Virtual destructor for proper polymorphic cleanup
    virtual ~IPortProvider() = default;

    // =========================================================================
    // Port state queries
    // =========================================================================

    /**
     * @brief Get total capacity for a port type.
     * @param port_type Port type (cast from port::PortType).
     * @param owner Owning player ID.
     * @return Total capacity across all ports of this type for this owner.
     */
    virtual std::uint32_t get_port_capacity(std::uint8_t port_type, std::uint8_t owner) const = 0;

    /**
     * @brief Get utilization ratio for a port type.
     * @param port_type Port type (cast from port::PortType).
     * @param owner Owning player ID.
     * @return Utilization ratio (0.0 = idle, 1.0 = fully utilized).
     */
    virtual float get_port_utilization(std::uint8_t port_type, std::uint8_t owner) const = 0;

    /**
     * @brief Check if an operational port of the given type exists.
     * @param port_type Port type (cast from port::PortType).
     * @param owner Owning player ID.
     * @return true if at least one operational port of this type exists.
     */
    virtual bool has_operational_port(std::uint8_t port_type, std::uint8_t owner) const = 0;

    /**
     * @brief Get count of ports of the given type.
     * @param port_type Port type (cast from port::PortType).
     * @param owner Owning player ID.
     * @return Number of ports of this type for this owner.
     */
    virtual std::uint32_t get_port_count(std::uint8_t port_type, std::uint8_t owner) const = 0;

    // =========================================================================
    // Demand bonus queries
    // =========================================================================

    /**
     * @brief Get global demand bonus for a zone type from all ports.
     * @param zone_type Zone type to query.
     * @param owner Owning player ID.
     * @return Global demand bonus factor (0.0 = no bonus).
     */
    virtual float get_global_demand_bonus(std::uint8_t zone_type, std::uint8_t owner) const = 0;

    /**
     * @brief Get local demand bonus at a position from nearby ports.
     * @param zone_type Zone type to query.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner Owning player ID.
     * @return Local demand bonus factor (0.0 = no bonus).
     */
    virtual float get_local_demand_bonus(std::uint8_t zone_type, std::int32_t x, std::int32_t y, std::uint8_t owner) const = 0;

    // =========================================================================
    // External connection queries
    // =========================================================================

    /**
     * @brief Get count of active external connections.
     * @param owner Owning player ID.
     * @return Number of active external connections for this owner.
     */
    virtual std::uint32_t get_external_connection_count(std::uint8_t owner) const = 0;

    /**
     * @brief Check if a specific map edge has a connection.
     * @param edge Map edge side (cast from port::MapEdge).
     * @param owner Owning player ID.
     * @return true if connected to the specified edge.
     */
    virtual bool is_connected_to_edge(std::uint8_t edge, std::uint8_t owner) const = 0;

    // =========================================================================
    // Trade income queries
    // =========================================================================

    /**
     * @brief Get total trade income for a player.
     * @param owner Owning player ID.
     * @return Trade income in credits per cycle.
     */
    virtual std::int64_t get_trade_income(std::uint8_t owner) const = 0;
};

/**
 * @interface IServiceQueryable
 * @brief Service coverage and effectiveness query interface (Epic 9 dependency)
 *
 * Allows BuildingSystem and other systems to query city service coverage
 * and effectiveness without depending on ServicesSystem directly.
 * Will be implemented by ServicesSystem in Epic 9.
 *
 * All enum parameters use uint8_t to avoid circular includes with
 * services::ServiceType, matching the pattern used by other interfaces
 * in this file.
 */
class IServiceQueryable {
public:
    /// Virtual destructor for proper polymorphic cleanup
    virtual ~IServiceQueryable() = default;

    /**
     * @brief Get overall coverage for a service type and player.
     * @param service_type Service type (cast from services::ServiceType).
     * @param player_id Owner player ID.
     * @return Coverage ratio (0.0 = no coverage, 1.0 = full coverage).
     */
    virtual float get_coverage(std::uint8_t service_type, std::uint8_t player_id) const = 0;

    /**
     * @brief Get service coverage at a specific tile position.
     * @param service_type Service type (cast from services::ServiceType).
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Coverage ratio at the tile (0.0 = no coverage, 1.0 = full coverage).
     */
    virtual float get_coverage_at(std::uint8_t service_type, std::int32_t x, std::int32_t y) const = 0;

    /**
     * @brief Get funding-adjusted effectiveness for a service type and player.
     * @param service_type Service type (cast from services::ServiceType).
     * @param player_id Owner player ID.
     * @return Effectiveness ratio (0.0 = completely ineffective, 1.0 = fully effective).
     */
    virtual float get_effectiveness(std::uint8_t service_type, std::uint8_t player_id) const = 0;
};

/**
 * @interface ILandValueProvider
 * @brief Sector desirability query interface (Epic 10 dependency)
 *
 * Allows BuildingSystem to query land value for template selection.
 * Will be implemented by LandValueSystem in Epic 10 (via IGridOverlay pattern).
 */
class ILandValueProvider {
public:
    /// Virtual destructor for proper polymorphic cleanup
    virtual ~ILandValueProvider() = default;

    /**
     * @brief Get land value at position.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Land value (0-255, higher = more desirable).
     */
    virtual float get_land_value(std::uint32_t x, std::uint32_t y) const = 0;
};

/**
 * @interface IDemandProvider
 * @brief Zone growth pressure query interface (Epic 10 dependency)
 *
 * Allows BuildingSystem and ZoneSystem to query demand for zone types.
 * Will be implemented by DemandSystem in Epic 10.
 */
class IDemandProvider {
public:
    /// Virtual destructor for proper polymorphic cleanup
    virtual ~IDemandProvider() = default;

    /**
     * @brief Get demand for zone type.
     * @param zone_type Zone type to query.
     * @param player_id Owner player ID.
     * @return Demand value (-100 to +100, positive = growth pressure).
     */
    virtual float get_demand(std::uint8_t zone_type, std::uint32_t player_id) const = 0;
};

/**
 * @interface ICreditProvider
 * @brief Treasury/credit management interface (Epic 11 dependency)
 *
 * Allows BuildingSystem to deduct construction costs and check credit availability.
 * Will be implemented by EconomySystem in Epic 11.
 */
class ICreditProvider {
public:
    /// Virtual destructor for proper polymorphic cleanup
    virtual ~ICreditProvider() = default;

    /**
     * @brief Deduct credits from player treasury.
     * @param player_id Owner player ID.
     * @param amount Amount to deduct (signed for flexibility).
     * @return true if deduction succeeded, false if insufficient credits.
     */
    virtual bool deduct_credits(std::uint32_t player_id, std::int64_t amount) = 0;

    /**
     * @brief Check if player has sufficient credits.
     * @param player_id Owner player ID.
     * @param amount Amount to check.
     * @return true if player has at least amount credits, false otherwise.
     */
    virtual bool has_credits(std::uint32_t player_id, std::int64_t amount) const = 0;
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_FORWARDDEPENDENCYINTERFACES_H
