/**
 * @file ServiceStatistics.h
 * @brief Service statistics data and query manager for UI display
 *        (Ticket E9-053)
 *
 * Provides:
 * - ServiceStatistics struct: cached per-type/player statistics
 * - ServiceStatisticsManager: update/query interface (IStatQueryable)
 *
 * Query methods accept uint8_t params (not ServiceType enum) to match
 * the interface convention used across the codebase.
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace services {

// ============================================================================
// Service Statistics Data (E9-053)
// ============================================================================

/**
 * @struct ServiceStatistics
 * @brief Cached statistics for a single service type + player combination.
 *
 * Updated periodically by the service system and queried by the UI layer.
 */
struct ServiceStatistics {
    uint32_t building_count  = 0;      ///< Number of service buildings placed
    float    average_coverage = 0.0f;  ///< Average coverage ratio (0.0 - 1.0)
    uint32_t total_capacity  = 0;      ///< Total service capacity across buildings
    float    effectiveness   = 0.0f;   ///< Overall service effectiveness (0.0 - 1.0)
};

// ============================================================================
// Service Statistics Manager (E9-053)
// ============================================================================

/**
 * @class ServiceStatisticsManager
 * @brief Manages cached service statistics for all service types and players.
 *
 * Provides both bulk access (get full ServiceStatistics) and individual
 * field accessors for UI convenience. Invalid type/player indices return
 * default-constructed (zero) values.
 */
class ServiceStatisticsManager {
public:
    /// Maximum supported players
    static constexpr uint8_t MAX_PLAYERS = 4;

    /// Number of service types (mirrors SERVICE_TYPE_COUNT from ServiceTypes.h)
    static constexpr uint8_t SERVICE_TYPE_COUNT = 4;

    /**
     * @brief Update cached statistics for a specific service type and player.
     *
     * @param service_type Service type index (0-3). Out-of-range is ignored.
     * @param player_id    Player index (0-3). Out-of-range is ignored.
     * @param stats        The statistics to cache.
     */
    void update(uint8_t service_type, uint8_t player_id,
                const ServiceStatistics& stats) {
        if (service_type >= SERVICE_TYPE_COUNT || player_id >= MAX_PLAYERS) {
            return;
        }
        m_stats[service_type][player_id] = stats;
    }

    /**
     * @brief Retrieve cached statistics for a specific service type and player.
     *
     * @param service_type Service type index (0-3).
     * @param player_id    Player index (0-3).
     * @return ServiceStatistics for the combination, or defaults if out-of-range.
     */
    ServiceStatistics get(uint8_t service_type, uint8_t player_id) const {
        if (service_type >= SERVICE_TYPE_COUNT || player_id >= MAX_PLAYERS) {
            return ServiceStatistics{};
        }
        return m_stats[service_type][player_id];
    }

    // ========================================================================
    // Individual field accessors (IStatQueryable interface)
    // ========================================================================

    /**
     * @brief Get the number of service buildings for a type and player.
     * @return Building count, or 0 if indices are out of range.
     */
    uint32_t get_building_count(uint8_t service_type, uint8_t player_id) const {
        if (service_type >= SERVICE_TYPE_COUNT || player_id >= MAX_PLAYERS) {
            return 0;
        }
        return m_stats[service_type][player_id].building_count;
    }

    /**
     * @brief Get the average coverage for a type and player.
     * @return Coverage ratio (0.0-1.0), or 0.0f if indices are out of range.
     */
    float get_average_coverage(uint8_t service_type, uint8_t player_id) const {
        if (service_type >= SERVICE_TYPE_COUNT || player_id >= MAX_PLAYERS) {
            return 0.0f;
        }
        return m_stats[service_type][player_id].average_coverage;
    }

    /**
     * @brief Get the total capacity for a type and player.
     * @return Total capacity, or 0 if indices are out of range.
     */
    uint32_t get_total_capacity(uint8_t service_type, uint8_t player_id) const {
        if (service_type >= SERVICE_TYPE_COUNT || player_id >= MAX_PLAYERS) {
            return 0;
        }
        return m_stats[service_type][player_id].total_capacity;
    }

    /**
     * @brief Get the service effectiveness for a type and player.
     * @return Effectiveness ratio (0.0-1.0), or 0.0f if indices are out of range.
     */
    float get_effectiveness(uint8_t service_type, uint8_t player_id) const {
        if (service_type >= SERVICE_TYPE_COUNT || player_id >= MAX_PLAYERS) {
            return 0.0f;
        }
        return m_stats[service_type][player_id].effectiveness;
    }

private:
    /// Per-type, per-player statistics cache. Default-initialized to zero.
    ServiceStatistics m_stats[SERVICE_TYPE_COUNT][MAX_PLAYERS] = {};
};

} // namespace services
} // namespace sims3000
