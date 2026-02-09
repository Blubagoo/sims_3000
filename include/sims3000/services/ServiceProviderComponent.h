/**
 * @file ServiceProviderComponent.h
 * @brief Service provider ECS component for Epic 9 (Ticket E9-002)
 *
 * Defines:
 * - ServiceProviderComponent: Per-service-facility data (pure data, no methods)
 *
 * Each service building entity gets this component to track its
 * service type, tier, effectiveness, and active status.
 */

#pragma once

#include <cstdint>
#include <type_traits>
#include <sims3000/services/ServiceTypes.h>

namespace sims3000 {
namespace services {

/**
 * @struct ServiceProviderComponent
 * @brief Per-service-facility data for city services (4 bytes).
 *
 * Pure data component following ECS pattern (no methods).
 *
 * Layout (4 bytes):
 * - service_type:          1 byte  (ServiceType/uint8_t)  - service classification
 * - tier:                  1 byte  (uint8_t)              - facility tier (1-3)
 * - current_effectiveness: 1 byte  (uint8_t)              - current effectiveness (0-255)
 * - is_active:             1 byte  (bool)                 - whether facility is operational
 *
 * Total: 4 bytes
 */
struct ServiceProviderComponent {
    ServiceType service_type          = ServiceType::Enforcer;  ///< Service classification
    uint8_t     tier                  = 1;                      ///< Facility tier (1-3)
    uint8_t     current_effectiveness = 0;                      ///< Current effectiveness (0-255)
    bool        is_active             = false;                  ///< Whether facility is operational
};

// Verify ServiceProviderComponent size (4 bytes, well under 8-byte requirement)
static_assert(sizeof(ServiceProviderComponent) == 4,
    "ServiceProviderComponent must be 4 bytes");

// Verify ServiceProviderComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<ServiceProviderComponent>::value,
    "ServiceProviderComponent must be trivially copyable");

} // namespace services
} // namespace sims3000
