/**
 * @file SimulationPipeline.h
 * @brief SimulationPipeline class for ordered system execution (Ticket 4-048)
 *
 * Manages ISimulatable systems in priority order. Systems with lower
 * priority values execute earlier in the tick.
 *
 * Canonical ordering per /docs/canon/interfaces.yaml:
 * - TerrainSystem: priority 5
 * - EnergyStub: priority 10
 * - FluidStub: priority 20
 * - ZoneSystem: priority 30
 * - BuildingSystem: priority 40
 *
 * @see /docs/canon/interfaces.yaml
 * @see /docs/epics/epic-4/tickets.md (ticket 4-048)
 */

#ifndef SIMS3000_SIM_SIMULATIONPIPELINE_H
#define SIMS3000_SIM_SIMULATIONPIPELINE_H

#include <cstdint>
#include <vector>
#include <functional>
#include <algorithm>

namespace sims3000 {
namespace sim {

/**
 * @class SimulationPipeline
 * @brief Manages ISimulatable systems in priority order.
 *
 * Systems are registered with a priority and a tick function.
 * On tick(), all systems are called in priority order (lower = earlier).
 * Duplicate priorities are allowed; both systems will be called.
 */
class SimulationPipeline {
public:
    /**
     * @brief Register a system with its priority.
     *
     * @param priority Execution priority (lower = earlier).
     * @param tick_fn Function to call each tick, receiving delta_time.
     * @param name Human-readable system name.
     */
    void register_system(int priority, std::function<void(float)> tick_fn, const char* name);

    /**
     * @brief Tick all systems in priority order (lower = earlier).
     *
     * @param delta_time Time since last tick in seconds.
     */
    void tick(float delta_time);

    /**
     * @brief Get registered system count.
     * @return Number of registered systems.
     */
    size_t system_count() const;

    /**
     * @brief Get system names in execution order.
     * @return Vector of system names sorted by priority.
     */
    std::vector<const char*> get_execution_order() const;

private:
    /**
     * @struct SystemEntry
     * @brief Internal storage for a registered system.
     */
    struct SystemEntry {
        int priority;                       ///< Execution priority
        std::function<void(float)> tick_fn; ///< Tick callback
        const char* name;                   ///< System name

        bool operator<(const SystemEntry& other) const {
            return priority < other.priority;
        }
    };

    std::vector<SystemEntry> m_systems;  ///< Registered systems
    bool m_sorted = true;               ///< Whether systems are currently sorted
};

} // namespace sim
} // namespace sims3000

#endif // SIMS3000_SIM_SIMULATIONPIPELINE_H
