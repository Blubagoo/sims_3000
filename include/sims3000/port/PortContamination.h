/**
 * @file PortContamination.h
 * @brief Port noise/contamination effect calculation (Epic 8, Ticket E8-033)
 *
 * Implements negative effects of ports on surrounding tiles:
 * - Aero ports: Noise contamination in 10-tile radius, reduces sector_value
 * - Aqua ports: Industrial contamination in 8-tile radius
 *
 * Contamination intensity decreases linearly with distance from the source.
 * Multiple port sources stack (capped at 255).
 *
 * Header-only implementation (pure logic, no external dependencies beyond PortTypes.h).
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <algorithm>

namespace sims3000 {
namespace port {

/// Default contamination radius for Aero ports (noise)
constexpr uint8_t AERO_CONTAMINATION_RADIUS = 10;

/// Default contamination radius for Aqua ports (industrial)
constexpr uint8_t AQUA_CONTAMINATION_RADIUS = 8;

/// Maximum contamination intensity value
constexpr uint8_t MAX_CONTAMINATION = 255;

/**
 * @struct PortContaminationSource
 * @brief Describes a single port as a contamination source.
 *
 * Each operational port generates contamination in a radius around it.
 * Aero ports generate noise contamination (radius 10), aqua ports generate
 * industrial contamination (radius 8).
 */
struct PortContaminationSource {
    int32_t x;              ///< Port X position (tile coordinates)
    int32_t y;              ///< Port Y position (tile coordinates)
    PortType port_type;     ///< Aero (noise) or Aqua (industrial)
    uint8_t radius;         ///< Contamination radius: 10 for aero, 8 for aqua
    uint8_t intensity;      ///< Base intensity at source (0-255)
    bool is_operational;    ///< Only operational ports emit contamination
};

/**
 * @brief Get the default contamination radius for a port type.
 *
 * @param type The port type.
 * @return Default radius: 10 for Aero, 8 for Aqua.
 */
inline uint8_t get_default_contamination_radius(PortType type) {
    switch (type) {
        case PortType::Aero: return AERO_CONTAMINATION_RADIUS;
        case PortType::Aqua: return AQUA_CONTAMINATION_RADIUS;
        default:             return 0;
    }
}

/**
 * @brief Calculate contamination contribution from a single source at a position.
 *
 * Contamination intensity decreases linearly with Manhattan distance from
 * the source. At the source tile the full intensity is applied. At the
 * edge of the radius the intensity drops to near zero.
 *
 * Formula: contribution = intensity * (1.0 - distance / radius)
 * If distance > radius, contribution is 0.
 *
 * Non-operational sources contribute 0.
 *
 * @param query_x X coordinate of the query position.
 * @param query_y Y coordinate of the query position.
 * @param source The contamination source to evaluate.
 * @return Contamination contribution (0-255) from this single source.
 */
inline uint8_t calculate_single_source_contamination(int32_t query_x, int32_t query_y,
                                                       const PortContaminationSource& source) {
    if (!source.is_operational) {
        return 0;
    }

    if (source.radius == 0 || source.intensity == 0) {
        return 0;
    }

    // Manhattan distance
    int32_t dist = std::abs(query_x - source.x) + std::abs(query_y - source.y);

    if (dist > static_cast<int32_t>(source.radius)) {
        return 0;
    }

    // Linear falloff: full intensity at center, zero at radius edge
    float factor = 1.0f - static_cast<float>(dist) / static_cast<float>(source.radius);
    float result = static_cast<float>(source.intensity) * factor;

    // Round and clamp to uint8_t range
    if (result < 0.0f) return 0;
    if (result > 255.0f) return 255;
    return static_cast<uint8_t>(result + 0.5f);
}

/**
 * @brief Calculate total port contamination at a position from all sources.
 *
 * Sums contamination contributions from all operational port sources.
 * The total is capped at 255 (maximum uint8_t value).
 *
 * Can be used by ContaminationSystem to query port-related contamination
 * at any tile position. The returned value can be used to reduce sector_value.
 *
 * @param x X coordinate of the query position (tile coordinates).
 * @param y Y coordinate of the query position (tile coordinates).
 * @param sources Vector of all port contamination sources.
 * @return Total contamination level (0-255) at the given position.
 */
inline uint8_t calculate_port_contamination(int32_t x, int32_t y,
                                              const std::vector<PortContaminationSource>& sources) {
    uint32_t total = 0;

    for (const auto& source : sources) {
        total += calculate_single_source_contamination(x, y, source);
    }

    // Cap at maximum uint8_t
    return static_cast<uint8_t>(std::min(total, static_cast<uint32_t>(MAX_CONTAMINATION)));
}

/**
 * @brief Check whether a position is within any port's contamination zone.
 *
 * Useful for quickly determining if a tile is affected without calculating
 * the full contamination value.
 *
 * @param x X coordinate of the query position.
 * @param y Y coordinate of the query position.
 * @param sources Vector of all port contamination sources.
 * @return true if position is within at least one operational port's contamination radius.
 */
inline bool is_in_contamination_zone(int32_t x, int32_t y,
                                       const std::vector<PortContaminationSource>& sources) {
    for (const auto& source : sources) {
        if (!source.is_operational) continue;
        if (source.radius == 0) continue;

        int32_t dist = std::abs(x - source.x) + std::abs(y - source.y);
        if (dist <= static_cast<int32_t>(source.radius)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Get the contamination type name for display.
 *
 * @param type The port type generating contamination.
 * @return "Noise" for Aero, "Industrial" for Aqua, "Unknown" otherwise.
 */
inline const char* contamination_type_name(PortType type) {
    switch (type) {
        case PortType::Aero: return "Noise";
        case PortType::Aqua: return "Industrial";
        default:             return "Unknown";
    }
}

} // namespace port
} // namespace sims3000
