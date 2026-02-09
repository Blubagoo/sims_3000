/**
 * @file ProbeFunction.h
 * @brief Probe query tool that aggregates tile data from simulation systems.
 *
 * Provides a decoupled query mechanism for the Overseer's probe tool.
 * Each simulation system (terrain, building, energy, etc.) implements
 * IProbeQueryProvider and registers with the ProbeFunction. When the
 * player probes a tile, all providers are queried and their results
 * aggregated into a single TileQueryResult for display.
 *
 * Usage:
 * @code
 *   ProbeFunction probe;
 *
 *   // Systems register themselves
 *   probe.register_provider(&terrain_system);
 *   probe.register_provider(&building_system);
 *
 *   // On probe click
 *   TileQueryResult result = probe.query({42, 17});
 *
 *   // Or query and display in one step
 *   probe.query_and_display({42, 17}, data_readout_panel);
 * @endcode
 *
 * Thread safety: not thread-safe. Call from the main/simulation thread only.
 */

#ifndef SIMS3000_UI_PROBEFUNCTION_H
#define SIMS3000_UI_PROBEFUNCTION_H

#include "sims3000/ui/DataReadoutPanel.h"
#include "sims3000/core/types.h"

#include <vector>

namespace sims3000 {
namespace ui {

/**
 * @class IProbeQueryProvider
 * @brief Interface for systems that supply tile data to the probe tool.
 *
 * Each simulation system implements this interface to contribute its
 * portion of a TileQueryResult. The probe function calls every
 * registered provider in order, allowing each to fill in the fields
 * it owns. Multiple providers may write to non-overlapping fields of
 * the same result struct.
 */
class IProbeQueryProvider {
public:
    virtual ~IProbeQueryProvider() = default;

    /**
     * Fill in query result fields owned by this system.
     * @param pos  Grid position being queried
     * @param result  Result struct to populate (may already contain data
     *                from earlier providers)
     */
    virtual void fill_query(GridPosition pos, TileQueryResult& result) const = 0;
};

/**
 * @class ProbeFunction
 * @brief Aggregates tile data from all registered query providers.
 *
 * Maintains a list of non-owning pointers to IProbeQueryProvider
 * implementations. On query, iterates all providers and merges their
 * contributions into a single TileQueryResult.
 */
class ProbeFunction {
public:
    ProbeFunction();

    /**
     * Register a query provider (non-owning).
     * Duplicate registrations are silently ignored.
     * @param provider Pointer to a provider; must outlive this object
     */
    void register_provider(IProbeQueryProvider* provider);

    /**
     * Remove a previously registered provider.
     * No-op if the provider is not registered.
     * @param provider Pointer to the provider to remove
     */
    void unregister_provider(IProbeQueryProvider* provider);

    /**
     * Execute a probe at the given grid position.
     * Queries all registered providers and returns the aggregated result.
     * @param pos Grid position to query
     * @return Aggregated TileQueryResult from all providers
     */
    TileQueryResult query(GridPosition pos) const;

    /**
     * Execute a probe and send the result to a DataReadoutPanel.
     * Convenience method equivalent to panel.show_query(query(pos)).
     * @param pos   Grid position to query
     * @param panel Panel to display the result in
     */
    void query_and_display(GridPosition pos, DataReadoutPanel& panel) const;

    /**
     * Get the number of registered providers.
     * @return Provider count
     */
    size_t provider_count() const;

private:
    /// Registered query providers (non-owning)
    std::vector<IProbeQueryProvider*> m_providers;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_PROBEFUNCTION_H
