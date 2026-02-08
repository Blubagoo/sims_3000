/**
 * @file IZoneQueryable.h
 * @brief Read-only zone query interface for downstream systems (Ticket 4-035)
 *
 * IZoneQueryable provides a read-only view of zone data for systems that
 * need to query zone state without modifying it. This decouples downstream
 * systems from ZoneSystem's internal implementation.
 *
 * Implemented by ZoneSystem.
 */

#ifndef SIMS3000_ZONE_IZONEQUERYABLE_H
#define SIMS3000_ZONE_IZONEQUERYABLE_H

#include <sims3000/zone/ZoneTypes.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace zone {

/**
 * @struct GridPosition
 * @brief Simple 2D grid coordinate for zone queries.
 */
struct GridPosition {
    std::int32_t x;
    std::int32_t y;

    GridPosition() : x(0), y(0) {}
    GridPosition(std::int32_t x_, std::int32_t y_) : x(x_), y(y_) {}

    bool operator==(const GridPosition& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const GridPosition& other) const {
        return !(*this == other);
    }
};

/**
 * @interface IZoneQueryable
 * @brief Read-only zone data queries for gameplay systems.
 *
 * Abstract interface that ZoneSystem implements. All methods are const
 * to ensure read-only access.
 */
class IZoneQueryable {
public:
    virtual ~IZoneQueryable() = default;

    /**
     * @brief Get zone type at grid position.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param out_type Output zone type.
     * @return true if zone exists at position, false otherwise.
     */
    virtual bool get_zone_type_at(std::int32_t x, std::int32_t y, ZoneType& out_type) const = 0;

    /**
     * @brief Get zone density at grid position.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param out_density Output density.
     * @return true if zone exists at position, false otherwise.
     */
    virtual bool get_zone_density_at(std::int32_t x, std::int32_t y, ZoneDensity& out_density) const = 0;

    /**
     * @brief Check if position is zoned.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return true if there is a zone at position.
     */
    virtual bool is_zoned_at(std::int32_t x, std::int32_t y) const = 0;

    /**
     * @brief Get zone count for a specific overseer and type.
     * @param player_id Overseer ID (0-4).
     * @param type Zone type to count.
     * @return Count of zones of that type for the player.
     */
    virtual std::uint32_t get_zone_count_for(std::uint8_t player_id, ZoneType type) const = 0;

    /**
     * @brief Get all designated zone positions for a player and type.
     * @param player_id Overseer ID (0-4).
     * @param type Zone type to filter.
     * @return Vector of GridPosition for matching designated zones.
     */
    virtual std::vector<GridPosition> get_designated_zones(std::uint8_t player_id, ZoneType type) const = 0;

    /**
     * @brief Get demand for zone type.
     * @param type Zone type.
     * @param player_id Overseer ID.
     * @return Demand value as float (-100 to +100).
     */
    virtual float get_demand_for(ZoneType type, std::uint8_t player_id) const = 0;
};

} // namespace zone
} // namespace sims3000

#endif // SIMS3000_ZONE_IZONEQUERYABLE_H
