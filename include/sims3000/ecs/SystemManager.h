/**
 * @file SystemManager.h
 * @brief Manages and executes simulation systems.
 */

#ifndef SIMS3000_ECS_SYSTEMMANAGER_H
#define SIMS3000_ECS_SYSTEMMANAGER_H

#include "sims3000/core/ISimulatable.h"
#include <memory>
#include <vector>
#include <string>

namespace sims3000 {

/**
 * @class SystemManager
 * @brief Registers and executes simulation systems.
 *
 * Systems are executed in priority order (lower = earlier).
 * Provides profiling data for debugging.
 */
class SystemManager {
public:
    SystemManager();
    ~SystemManager();

    // Non-copyable
    SystemManager(const SystemManager&) = delete;
    SystemManager& operator=(const SystemManager&) = delete;

    /**
     * Register a system.
     * @param system System to register (takes ownership)
     */
    void addSystem(std::unique_ptr<ISimulatable> system);

    /**
     * Register a system (convenience for make_unique).
     * @tparam T System type
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Reference to created system
     */
    template<typename T, typename... Args>
    T& createSystem(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *system;
        addSystem(std::move(system));
        return ref;
    }

    /**
     * Remove a system by name.
     * @param name System name to remove
     * @return true if system was found and removed
     */
    bool removeSystem(const char* name);

    /**
     * Tick all systems.
     * @param time Simulation time interface
     */
    void tick(const ISimulationTime& time);

    /**
     * Get number of registered systems.
     */
    size_t getSystemCount() const;

    /**
     * Get system by index.
     * @param index System index
     * @return System pointer, or nullptr if out of range
     */
    ISimulatable* getSystem(size_t index);
    const ISimulatable* getSystem(size_t index) const;

    /**
     * Get system by name.
     * @param name System name
     * @return System pointer, or nullptr if not found
     */
    ISimulatable* getSystem(const char* name);
    const ISimulatable* getSystem(const char* name) const;

    /**
     * Enable/disable profiling.
     */
    void setProfilingEnabled(bool enabled);

    /**
     * Get profiling data for a system.
     * @param index System index
     * @return Average tick time in milliseconds
     */
    float getSystemTickTime(size_t index) const;

    /**
     * Get total tick time for all systems.
     * @return Total tick time in milliseconds
     */
    float getTotalTickTime() const;

    /**
     * Clear all systems.
     */
    void clear();

private:
    void sortSystems();

    struct SystemEntry {
        std::unique_ptr<ISimulatable> system;
        float avgTickTime = 0.0f;
    };

    std::vector<SystemEntry> m_systems;
    bool m_sorted = true;
    bool m_profilingEnabled = false;
    float m_totalTickTime = 0.0f;
};

} // namespace sims3000

#endif // SIMS3000_ECS_SYSTEMMANAGER_H
