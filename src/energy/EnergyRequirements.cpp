/**
 * @file EnergyRequirements.cpp
 * @brief Implementation of energy requirement lookup (Epic 5, ticket 5-037)
 */

#include <sims3000/energy/EnergyRequirements.h>

namespace sims3000 {
namespace energy {

uint32_t get_energy_requirement(uint8_t zone_type, uint8_t density) {
    // zone_type: 0=Habitation, 1=Exchange, 2=Fabrication
    // density:   0=Low, 1=High
    switch (zone_type) {
        case 0: // Habitation
            return (density == 0) ? ENERGY_REQ_HABITATION_LOW
                                  : ENERGY_REQ_HABITATION_HIGH;
        case 1: // Exchange
            return (density == 0) ? ENERGY_REQ_EXCHANGE_LOW
                                  : ENERGY_REQ_EXCHANGE_HIGH;
        case 2: // Fabrication
            return (density == 0) ? ENERGY_REQ_FABRICATION_LOW
                                  : ENERGY_REQ_FABRICATION_HIGH;
        default:
            return 0;
    }
}

} // namespace energy
} // namespace sims3000
