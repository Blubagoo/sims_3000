/**
 * @file IContaminationSource.h
 * @brief Interface for systems that produce contamination, and an aggregator
 *        to collect and apply contamination from all registered sources
 *        (Ticket E10-082).
 *
 * Any system that generates contamination (energy, industry, terrain, traffic)
 * implements IContaminationSource. The ContaminationAggregator collects
 * entries from all registered sources and applies them to the grid.
 *
 * @note The energy module has its own energy::IContaminationSource.h with
 *       energy::ContaminationSourceData. This contamination-module interface
 *       is the canonical aggregation point used by ContaminationSystem.
 *
 * @see ContaminationGrid
 * @see ContaminationType
 * @see E10-082
 */

#ifndef SIMS3000_CONTAMINATION_ICONTAMINATIONSOURCE_H
#define SIMS3000_CONTAMINATION_ICONTAMINATIONSOURCE_H

#include <cstdint>
#include <vector>

#include "sims3000/contamination/ContaminationGrid.h"
#include "sims3000/contamination/ContaminationType.h"

namespace sims3000 {
namespace contamination {

/**
 * @struct ContaminationSourceEntry
 * @brief A single contamination contribution from a source system.
 *
 * Represents one point of contamination emission at a grid position
 * with a specified output amount and type.
 */
struct ContaminationSourceEntry {
    int32_t x;                 ///< Grid X coordinate
    int32_t y;                 ///< Grid Y coordinate
    uint32_t output;           ///< Contamination output per tick (clamped to 255 on apply)
    ContaminationType type;    ///< Type of contamination being emitted
};

/**
 * @class IContaminationSource
 * @brief Interface for systems that generate contamination.
 *
 * Systems that emit contamination (e.g. energy plants, factories, terrain)
 * implement this interface so the ContaminationAggregator can query them.
 */
class IContaminationSource {
public:
    virtual ~IContaminationSource() = default;

    /**
     * @brief Append this system's contamination sources to the output vector.
     *
     * Implementations should push_back one ContaminationSourceEntry per
     * active contamination emitter.
     *
     * @param out Vector to append source entries to.
     */
    virtual void get_contamination_sources(std::vector<ContaminationSourceEntry>& out) const = 0;
};

/**
 * @class ContaminationAggregator
 * @brief Collects contamination from all registered sources and applies to a grid.
 *
 * Systems register/unregister themselves as contamination sources. During
 * the generate phase, apply_all_sources() queries every registered source
 * and adds their contamination to the grid.
 */
class ContaminationAggregator {
public:
    /**
     * @brief Register a contamination source.
     *
     * The source pointer must remain valid until unregistered.
     * Duplicate registrations are allowed but will result in
     * double-counting.
     *
     * @param source Pointer to the contamination source to register.
     */
    void register_source(IContaminationSource* source);

    /**
     * @brief Unregister a contamination source.
     *
     * Removes the first occurrence of the source pointer. No-op if
     * the pointer is not found.
     *
     * @param source Pointer to the contamination source to unregister.
     */
    void unregister_source(IContaminationSource* source);

    /**
     * @brief Collect all sources and apply contamination to the grid.
     *
     * Iterates all registered sources, calls get_contamination_sources(),
     * and for each entry calls grid.add_contamination(x, y, min(output, 255), type).
     *
     * @param grid The contamination grid to apply to.
     */
    void apply_all_sources(ContaminationGrid& grid);

    /**
     * @brief Get the number of currently registered sources.
     * @return Number of registered source pointers.
     */
    size_t get_source_count() const;

private:
    std::vector<IContaminationSource*> m_sources; ///< Registered source pointers
};

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_ICONTAMINATIONSOURCE_H
