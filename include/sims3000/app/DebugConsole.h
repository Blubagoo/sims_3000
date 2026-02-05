/**
 * @file DebugConsole.h
 * @brief Debug console output for development monitoring.
 *
 * Outputs frame timing, tick rate, and system stats to stdout.
 * Toggle with F3 key.
 */

#ifndef SIMS3000_APP_DEBUGCONSOLE_H
#define SIMS3000_APP_DEBUGCONSOLE_H

#include "sims3000/app/FrameStats.h"
#include <cstddef>

namespace sims3000 {

// Forward declarations
class AssetManager;
class Registry;

/**
 * @class DebugConsole
 * @brief Periodic debug output to console.
 *
 * When enabled, outputs performance metrics at configurable intervals.
 */
class DebugConsole {
public:
    DebugConsole();
    ~DebugConsole() = default;

    /**
     * Toggle debug output on/off.
     */
    void toggle();

    /**
     * Enable or disable debug output.
     */
    void setEnabled(bool enabled);

    /**
     * Check if debug output is enabled.
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * Set output interval in seconds.
     * @param seconds Time between outputs (default 1.0)
     */
    void setOutputInterval(float seconds);

    /**
     * Update and potentially output stats.
     * @param deltaTime Frame delta time in seconds
     * @param frameStats Current frame statistics
     * @param assets Asset manager for cache stats (optional)
     * @param registry ECS registry for entity count (optional)
     */
    void update(float deltaTime,
                const FrameStats& frameStats,
                const AssetManager* assets = nullptr,
                const Registry* registry = nullptr);

private:
    void outputStats(const FrameStats& frameStats,
                     const AssetManager* assets,
                     const Registry* registry);

    bool m_enabled = false;
    float m_outputInterval = 1.0f;
    float m_timeSinceOutput = 0.0f;
};

} // namespace sims3000

#endif // SIMS3000_APP_DEBUGCONSOLE_H
