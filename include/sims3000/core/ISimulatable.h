/**
 * @file ISimulatable.h
 * @brief Interface for systems that participate in simulation ticks.
 */

#ifndef SIMS3000_CORE_ISIMULATABLE_H
#define SIMS3000_CORE_ISIMULATABLE_H

#include "sims3000/core/ISimulationTime.h"

namespace sims3000 {

/**
 * @interface ISimulatable
 * @brief Interface for all systems that run during simulation ticks.
 *
 * Systems implementing this interface are registered with the SystemManager
 * and called in priority order each simulation tick (20 Hz).
 *
 * The tick() method receives read-only access to simulation time.
 * Systems should not store mutable references to ISimulationTime.
 */
class ISimulatable {
public:
    virtual ~ISimulatable() = default;

    /**
     * Called once per simulation tick (20 Hz).
     * @param time Read-only access to simulation timing
     */
    virtual void tick(const ISimulationTime& time) = 0;

    /**
     * Get the priority of this system.
     * Lower values run first. Default is 100.
     * @return System priority
     */
    virtual int getPriority() const { return 100; }

    /**
     * Get the name of this system for debugging.
     * @return System name
     */
    virtual const char* getName() const { return "UnnamedSystem"; }
};

} // namespace sims3000

#endif // SIMS3000_CORE_ISIMULATABLE_H
