/**
 * @file FluidRequirements.cpp
 * @brief Implementation of fluid requirement lookup (Epic 6, ticket 6-039)
 */

#include <sims3000/fluid/FluidRequirements.h>

namespace sims3000 {
namespace fluid {

uint32_t get_zone_fluid_requirement(uint8_t zone_type, uint8_t density) {
    // zone_type: 0=Habitation, 1=Exchange, 2=Fabrication
    // density:   0=Low, 1=High
    switch (zone_type) {
        case 0: // Habitation
            return (density == 0) ? FLUID_REQ_HABITATION_LOW
                                  : FLUID_REQ_HABITATION_HIGH;
        case 1: // Exchange
            return (density == 0) ? FLUID_REQ_EXCHANGE_LOW
                                  : FLUID_REQ_EXCHANGE_HIGH;
        case 2: // Fabrication
            return (density == 0) ? FLUID_REQ_FABRICATION_LOW
                                  : FLUID_REQ_FABRICATION_HIGH;
        default:
            return 0;
    }
}

uint32_t get_service_fluid_requirement(uint8_t service_type) {
    // service_type: 0=Small, 1=Medium, 2=Large
    switch (service_type) {
        case 0: return FLUID_REQ_SERVICE_SMALL;
        case 1: return FLUID_REQ_SERVICE_MEDIUM;
        case 2: return FLUID_REQ_SERVICE_LARGE;
        default: return FLUID_REQ_SERVICE_MEDIUM; // Safe default
    }
}

} // namespace fluid
} // namespace sims3000
