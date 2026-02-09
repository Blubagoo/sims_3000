/**
 * @file EnergyContamination.h
 * @brief Energy contamination generation from power plants (Ticket E10-084).
 *
 * Power plants (nexus buildings) produce energy contamination based on
 * their fuel type. Clean energy sources produce zero contamination.
 *
 * @see ContaminationGrid
 * @see ContaminationType
 * @see E10-084
 */

#ifndef SIMS3000_CONTAMINATION_ENERGYCONTAMINATION_H
#define SIMS3000_CONTAMINATION_ENERGYCONTAMINATION_H

#include <cstdint>
#include <vector>

#include "sims3000/contamination/ContaminationGrid.h"

namespace sims3000 {
namespace contamination {

/**
 * @struct EnergySource
 * @brief Represents a power plant that generates energy contamination.
 */
struct EnergySource {
    int32_t x, y;          ///< Grid position
    uint8_t nexus_type;    ///< 0=carbon, 1=petrochem, 2=gaseous, 3+=clean
    bool is_active;        ///< Whether the plant is currently operational
};

/// Contamination output per nexus type: carbon=200, petrochem=120, gaseous=40
constexpr uint8_t ENERGY_CONTAMINATION_OUTPUT[] = { 200, 120, 40 };

/**
 * @brief Apply energy contamination from power plants to the grid.
 *
 * Per source: if is_active && nexus_type < 3, output = ENERGY_CONTAMINATION_OUTPUT[nexus_type].
 * Calls add_contamination(x, y, output, ContaminationType::Energy).
 * Clean energy (type >= 3) produces 0 contamination.
 * Inactive sources produce 0 contamination.
 *
 * @param grid The contamination grid to write to.
 * @param sources Vector of energy contamination sources.
 */
void apply_energy_contamination(ContaminationGrid& grid,
                                 const std::vector<EnergySource>& sources);

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_ENERGYCONTAMINATION_H
