/**
 * @file TrafficContaminationAdapter.h
 * @brief Adapter that allows TransportSystem to act as a contamination source (E10-115)
 *
 * Wraps traffic tile data as an IContaminationSource, converting congested
 * road tiles into contamination entries with output lerp(5, 50, congestion).
 *
 * @see sims3000::contamination::IContaminationSource
 * @see E10-115, E10-085
 */

#ifndef SIMS3000_TRANSPORT_TRAFFICCONTAMINATIONADAPTER_H
#define SIMS3000_TRANSPORT_TRAFFICCONTAMINATIONADAPTER_H

#include <cstdint>
#include <vector>

#include <sims3000/contamination/IContaminationSource.h>

namespace sims3000 {
namespace transport {

/**
 * @struct TrafficTileInfo
 * @brief Represents a road tile with traffic data.
 *
 * Contains grid position, congestion level (0.0-1.0), and active state.
 */
struct TrafficTileInfo {
    int32_t x;              ///< Grid X coordinate
    int32_t y;              ///< Grid Y coordinate
    float congestion;       ///< Congestion level (0.0 = empty, 1.0 = jammed)
    bool is_active;         ///< Whether the tile is an active road
};

/// Minimum traffic contamination output (at congestion = 0.0)
constexpr uint32_t TRAFFIC_CONTAM_MIN = 5;

/// Maximum traffic contamination output (at congestion = 1.0)
constexpr uint32_t TRAFFIC_CONTAM_MAX = 50;

/// Minimum congestion threshold to produce contamination
constexpr float MIN_CONGESTION_THRESHOLD = 0.1f;

/**
 * @class TrafficContaminationAdapter
 * @brief Adapter that wraps traffic tile data as IContaminationSource.
 *
 * Converts TrafficTileInfo into ContaminationSourceEntry, filtering
 * for active tiles with congestion >= MIN_CONGESTION_THRESHOLD.
 * Output is computed as lerp(TRAFFIC_CONTAM_MIN, TRAFFIC_CONTAM_MAX, congestion).
 */
class TrafficContaminationAdapter : public contamination::IContaminationSource {
public:
    /**
     * @brief Set the current list of traffic tiles.
     *
     * Replaces the internal tile list with the provided data.
     *
     * @param tiles Vector of traffic tile information.
     */
    void set_traffic_tiles(const std::vector<TrafficTileInfo>& tiles);

    /**
     * @brief Clear all traffic tile data.
     *
     * Removes all tiles from the internal list.
     */
    void clear();

    /**
     * @brief Get contamination sources from congested traffic tiles.
     *
     * Iterates through tiles and appends entries for:
     * - Active tiles with congestion >= MIN_CONGESTION_THRESHOLD
     * - output = lerp(TRAFFIC_CONTAM_MIN, TRAFFIC_CONTAM_MAX, congestion)
     * - type = Traffic
     *
     * @param entries Vector to append contamination source entries to.
     */
    void get_contamination_sources(std::vector<contamination::ContaminationSourceEntry>& entries) const override;

private:
    std::vector<TrafficTileInfo> m_tiles; ///< Current list of traffic tiles
};

} // namespace transport
} // namespace sims3000

#endif // SIMS3000_TRANSPORT_TRAFFICCONTAMINATIONADAPTER_H
