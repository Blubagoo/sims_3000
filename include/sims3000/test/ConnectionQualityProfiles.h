/**
 * @file ConnectionQualityProfiles.h
 * @brief Network condition presets for testing.
 *
 * Defines standard network quality profiles with configurable:
 * - Latency (base + jitter)
 * - Packet loss percentage
 * - Bandwidth limits
 *
 * Usage:
 *   MockSocket socket(ConnectionQualityProfiles::POOR_WIFI);
 *   // or
 *   MockSocket socket;
 *   socket.setNetworkConditions(ConnectionQualityProfiles::MOBILE_3G);
 *
 * Ownership: Value types, no cleanup needed.
 */

#ifndef SIMS3000_TEST_CONNECTIONQUALITYPROFILES_H
#define SIMS3000_TEST_CONNECTIONQUALITYPROFILES_H

#include <cstdint>

namespace sims3000 {

/**
 * @struct NetworkConditions
 * @brief Configurable network condition parameters.
 */
struct NetworkConditions {
    /// Base latency in milliseconds (one-way delay)
    std::uint32_t latencyMs = 0;

    /// Latency jitter in milliseconds (+/- random variation)
    std::uint32_t jitterMs = 0;

    /// Packet loss percentage (0-100)
    float packetLossPercent = 0.0f;

    /// Bandwidth limit in bytes per second (0 = unlimited)
    std::uint32_t bandwidthBytesPerSec = 0;

    /// Whether packets can arrive out of order
    bool allowReordering = false;

    /// Duplicate packet percentage (0-100)
    float duplicatePercent = 0.0f;

    /// Create conditions with no degradation (ideal network)
    static NetworkConditions perfect() {
        return NetworkConditions{};
    }

    /// Check if conditions represent perfect network
    bool isPerfect() const {
        return latencyMs == 0 && jitterMs == 0 &&
               packetLossPercent == 0.0f && bandwidthBytesPerSec == 0 &&
               !allowReordering && duplicatePercent == 0.0f;
    }
};

/**
 * @namespace ConnectionQualityProfiles
 * @brief Standard network condition presets for testing.
 */
namespace ConnectionQualityProfiles {

/**
 * Perfect connection - no latency, no loss, unlimited bandwidth.
 * Use for unit tests that need deterministic behavior.
 */
inline const NetworkConditions PERFECT = {
    0,      // latencyMs
    0,      // jitterMs
    0.0f,   // packetLossPercent
    0,      // bandwidthBytesPerSec
    false,  // allowReordering
    0.0f    // duplicatePercent
};

/**
 * LAN connection - very low latency, no loss.
 * Simulates local network gaming.
 */
inline const NetworkConditions LAN = {
    1,      // latencyMs
    0,      // jitterMs
    0.0f,   // packetLossPercent
    0,      // bandwidthBytesPerSec (~100+ Mbps, effectively unlimited)
    false,  // allowReordering
    0.0f    // duplicatePercent
};

/**
 * Good WiFi connection - low latency, minimal loss.
 * Simulates typical home WiFi gaming.
 */
inline const NetworkConditions GOOD_WIFI = {
    20,                 // latencyMs
    5,                  // jitterMs
    0.1f,               // packetLossPercent
    10 * 1024 * 1024,   // bandwidthBytesPerSec (~80 Mbps)
    false,              // allowReordering
    0.0f                // duplicatePercent
};

/**
 * Poor WiFi connection - moderate latency, some loss.
 * Simulates congested or distant WiFi.
 */
inline const NetworkConditions POOR_WIFI = {
    80,                 // latencyMs
    30,                 // jitterMs
    2.0f,               // packetLossPercent
    1 * 1024 * 1024,    // bandwidthBytesPerSec (~8 Mbps)
    true,               // allowReordering
    0.1f                // duplicatePercent
};

/**
 * Mobile 3G connection - high latency, significant loss.
 * Simulates mobile gaming on older networks.
 */
inline const NetworkConditions MOBILE_3G = {
    150,            // latencyMs
    50,             // jitterMs
    5.0f,           // packetLossPercent
    128 * 1024,     // bandwidthBytesPerSec (~1 Mbps)
    true,           // allowReordering
    0.5f            // duplicatePercent
};

/**
 * Hostile connection - extreme conditions for stress testing.
 * High latency, high loss, severe bandwidth limits.
 */
inline const NetworkConditions HOSTILE = {
    500,            // latencyMs
    200,            // jitterMs
    20.0f,          // packetLossPercent
    32 * 1024,      // bandwidthBytesPerSec (~256 Kbps)
    true,           // allowReordering
    5.0f            // duplicatePercent
};

/**
 * Get a profile by name string.
 * @param name Profile name (case-insensitive)
 * @return NetworkConditions for the profile, or PERFECT if not found.
 */
inline NetworkConditions getByName(const char* name) {
    if (!name) return PERFECT;

    // Simple case-insensitive comparison (ASCII only)
    auto cmp = [](const char* a, const char* b) {
        while (*a && *b) {
            char ca = (*a >= 'A' && *a <= 'Z') ? (*a + 32) : *a;
            char cb = (*b >= 'A' && *b <= 'Z') ? (*b + 32) : *b;
            if (ca != cb) return false;
            a++;
            b++;
        }
        return *a == *b;
    };

    if (cmp(name, "perfect")) return PERFECT;
    if (cmp(name, "lan")) return LAN;
    if (cmp(name, "good_wifi")) return GOOD_WIFI;
    if (cmp(name, "poor_wifi")) return POOR_WIFI;
    if (cmp(name, "mobile_3g")) return MOBILE_3G;
    if (cmp(name, "hostile")) return HOSTILE;

    return PERFECT;
}

} // namespace ConnectionQualityProfiles

} // namespace sims3000

#endif // SIMS3000_TEST_CONNECTIONQUALITYPROFILES_H
