/**
 * @file EnergyContaminationAdapter.cpp
 * @brief Implementation of EnergyContaminationAdapter (E10-114)
 */

#include <sims3000/energy/EnergyContaminationAdapter.h>

namespace sims3000 {
namespace energy {

void EnergyContaminationAdapter::set_nexuses(const std::vector<EnergyNexusInfo>& nexuses) {
    m_nexuses = nexuses;
}

void EnergyContaminationAdapter::clear() {
    m_nexuses.clear();
}

void EnergyContaminationAdapter::get_contamination_sources(
    std::vector<contamination::ContaminationSourceEntry>& entries) const {

    for (const auto& nexus : m_nexuses) {
        // Skip inactive nexuses
        if (!nexus.is_active) {
            continue;
        }

        // Skip clean energy (type >= 3)
        if (nexus.nexus_type >= 3) {
            continue;
        }

        // Determine output based on nexus type
        uint32_t output = 0;
        switch (nexus.nexus_type) {
            case 0: // Carbon
                output = CARBON_OUTPUT;
                break;
            case 1: // Petrochem
                output = PETROCHEM_OUTPUT;
                break;
            case 2: // Gaseous
                output = GASEOUS_OUTPUT;
                break;
            default:
                // Should not reach here due to type >= 3 check above
                continue;
        }

        // Add contamination source entry
        contamination::ContaminationSourceEntry entry;
        entry.x = nexus.x;
        entry.y = nexus.y;
        entry.output = output;
        entry.type = contamination::ContaminationType::Energy;

        entries.push_back(entry);
    }
}

} // namespace energy
} // namespace sims3000
