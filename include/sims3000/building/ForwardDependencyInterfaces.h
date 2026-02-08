/**
 * @file ForwardDependencyInterfaces.h
 * @brief Forward dependency interfaces for BuildingSystem (Epic 4)
 *
 * Defines six pure abstract interfaces that represent dependencies on systems
 * implemented in later epics. These interfaces enable BuildingSystem and
 * ZoneSystem to be developed and tested independently of future epics.
 *
 * Interfaces:
 * - IEnergyProvider: Power state queries (Epic 5)
 * - IFluidProvider: Water/fluid state queries (Epic 6)
 * - ITransportProvider: Pathway connectivity queries (Epic 7)
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
