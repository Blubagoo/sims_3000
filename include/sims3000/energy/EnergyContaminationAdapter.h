/**
 * @file EnergyContaminationAdapter.h
 * @brief Adapter that allows EnergySystem to act as a contamination source (E10-114)
 *
 * Wraps energy nexus data as an IContaminationSource, converting active
 * carbon/petrochem/gaseous nexuses into contamination entries.
 *
 * @see sims3000::contamination::IContaminationSource
 * @see E10-114, E10-084
 */

#ifndef SIMS3000_ENERGY_ENERGYCONTAMINATIONADAPTER_H
#define SIMS3000_ENERGY_ENERGYCONTAMINATIONADAPTER_H

#include <cstdint>
#include <vector>

#include <sims3000/contamination/IContaminationSource.h>

namespace sims3000 {
namespace energy {

/**
 * @struct EnergyNexusInfo
 * @brief Represents an energy nexus that may produce contamination.
 *
 * Contains grid position, nexus type, and active state.
 */
struct EnergyNexusInfo {
    int32_t x;              ///< Grid X coordinate
    int32_t y;              ///< Grid Y coordinate
    uint8_t nexus_type;     ///< 0=Carbon, 1=Petrochem, 2=Gaseous, >=3=Clean
    bool is_active;         ///< Whether the nexus is currently active
};

/// Contamination output for Carbon nexuses (type 0)
constexpr uint32_t CARBON_OUTPUT = 200;

/// Contamination output for Petrochem nexuses (type 1)
constexpr uint32_t PETROCHEM_OUTPUT = 120;

/// Contamination output for Gaseous nexuses (type 2)
constexpr uint32_t GASEOUS_OUTPUT = 40;

/**
 * @class EnergyContaminationAdapter
 * @brief Adapter that wraps energy nexus data as IContaminationSource.
 *
 * Converts EnergyNexusInfo into ContaminationSourceEntry, filtering
 * for active nexuses with contaminating types (< 3).
 */
class EnergyContaminationAdapter : public contamination::IContaminationSource {
public:
    /**
     * @brief Set the current list of energy nexuses.
     *
     * Replaces the internal nexus list with the provided data.
     *
     * @param nexuses Vector of energy nexus information.
     */
    void set_nexuses(const std::vector<EnergyNexusInfo>& nexuses);

    /**
     * @brief Clear all nexus data.
     *
     * Removes all nexuses from the internal list.
     */
    void clear();

    /**
     * @brief Get contamination sources from active energy nexuses.
     *
     * Iterates through nexuses and appends entries for:
     * - Active nexuses with type < 3
     * - Type 0 (Carbon): output = 200, type = Energy
     * - Type 1 (Petrochem): output = 120, type = Energy
     * - Type 2 (Gaseous): output = 40, type = Energy
     *
     * @param entries Vector to append contamination source entries to.
     */
    void get_contamination_sources(std::vector<contamination::ContaminationSourceEntry>& entries) const override;

private:
    std::vector<EnergyNexusInfo> m_nexuses; ///< Current list of energy nexuses
};

} // namespace energy
} // namespace sims3000

#endif // SIMS3000_ENERGY_ENERGYCONTAMINATIONADAPTER_H
