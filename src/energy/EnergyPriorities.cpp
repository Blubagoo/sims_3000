/**
 * @file EnergyPriorities.cpp
 * @brief Implementation of energy priority lookup (Epic 5, ticket 5-038)
 */

#include <sims3000/energy/EnergyPriorities.h>

namespace sims3000 {
namespace energy {

uint8_t get_energy_priority_for_zone(uint8_t zone_type) {
    // zone_type: 0=Habitation, 1=Exchange, 2=Fabrication
    switch (zone_type) {
        case 0: // Habitation - lowest priority during rationing
            return ENERGY_PRIORITY_LOW;
        case 1: // Exchange - normal priority
            return ENERGY_PRIORITY_NORMAL;
        case 2: // Fabrication - normal priority
            return ENERGY_PRIORITY_NORMAL;
        default:
            return ENERGY_PRIORITY_DEFAULT;
    }
}

} // namespace energy
} // namespace sims3000
