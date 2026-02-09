/**
 * @file ServiceConfigs.h
 * @brief Service building configuration data for Epic 9
 *        (Tickets E9-030, E9-031, E9-032, E9-033)
 *
 * Defines constexpr configuration data for city service buildings across
 * four service types (Enforcer, HazardResponse, Medical, Education), each with
 * three tiers (Post/Station/Nexus).
 *
 * Also defines service-specific gameplay constants:
 * - Hazard: fire suppression speed
 * - Medical: longevity bonus parameters
 * - Education: knowledge quotient bonus
 *
 * Uses canonical enums from ServiceTypes.h (E9-001).
 *
 * @see ServiceTypes.h
 */

#pragma once

#include <sims3000/services/ServiceTypes.h>

namespace sims3000 {
namespace services {

/// Total number of service configs (types * tiers)
constexpr uint8_t SERVICE_CONFIG_COUNT = SERVICE_TYPE_COUNT * SERVICE_TIER_COUNT;

// =============================================================================
// Service Building Config Struct
// =============================================================================

/**
 * @struct ServiceBuildingConfig
 * @brief Static configuration data for a service building.
 *
 * Each service building type+tier combination has fixed stats.
 * Runtime values (like current effectiveness, power state) are
 * tracked separately in ECS components.
 */
struct ServiceBuildingConfig {
    ServiceType type;           ///< Service type identifier
    ServiceTier tier;           ///< Building tier
    const char* name;           ///< Human-readable building name
    uint8_t radius;             ///< Coverage radius in tiles (0 = global effect)
    uint8_t effectiveness;      ///< Base effectiveness percentage (0-100)
    uint16_t capacity;          ///< Population/being capacity (0 = N/A)
    uint8_t footprint_w;        ///< Building footprint width in tiles
    uint8_t footprint_h;        ///< Building footprint height in tiles
    bool requires_power;        ///< Whether building needs energy to function
};

// =============================================================================
// Service-Specific Gameplay Constants
// =============================================================================

/// Hazard fire suppression speed: 3x faster fire suppression in coverage area
constexpr float HAZARD_SUPPRESSION_SPEED = 3.0f;

/// Medical base longevity in simulation cycles
constexpr uint32_t MEDICAL_BASE_LONGEVITY = 60;

/// Medical maximum longevity bonus at 100% coverage (added to base)
constexpr uint32_t MEDICAL_MAX_LONGEVITY_BONUS = 40;

/// Number of beings served per medical capacity unit
constexpr uint32_t BEINGS_PER_MEDICAL_UNIT = 500;

/// Number of beings served per education capacity unit
constexpr uint32_t BEINGS_PER_EDUCATION_UNIT = 300;

/// Education knowledge bonus: 10% land value bonus at 100% coverage
constexpr float EDUCATION_KNOWLEDGE_BONUS = 0.1f;

// =============================================================================
// Service Building Configurations (constexpr)
// =============================================================================

/**
 * @brief All service building configurations indexed by
 *        (ServiceType ordinal * SERVICE_TIER_COUNT + (ServiceTier ordinal - 1)).
 *
 * Layout:
 *   [0]  Enforcer Post       [1]  Enforcer Station    [2]  Enforcer Nexus
 *   [3]  Hazard Post         [4]  Hazard Station      [5]  Hazard Nexus
 *   [6]  Medical Post        [7]  Medical Center       [8]  Medical Nexus
 *   [9]  Learning Center     [10] Archive              [11] Knowledge Nexus
 */
constexpr ServiceBuildingConfig SERVICE_CONFIGS[SERVICE_CONFIG_COUNT] = {
    // =========================================================================
    // Enforcer (Ticket E9-030) - Radius-based coverage, no capacity
    // =========================================================================
    {
        ServiceType::Enforcer,      // type
        ServiceTier::Post,          // tier
        "Enforcer Post",            // name
        8,                          // radius (tiles)
        100,                        // effectiveness (%)
        0,                          // capacity (N/A for enforcer)
        1, 1,                       // footprint (1x1)
        true                        // requires_power
    },
    {
        ServiceType::Enforcer,      // type
        ServiceTier::Station,       // tier
        "Enforcer Station",         // name
        12,                         // radius (tiles)
        100,                        // effectiveness (%)
        0,                          // capacity
        2, 2,                       // footprint (2x2)
        true                        // requires_power
    },
    {
        ServiceType::Enforcer,      // type
        ServiceTier::Nexus,         // tier
        "Enforcer Nexus",           // name
        16,                         // radius (tiles)
        100,                        // effectiveness (%)
        0,                          // capacity
        3, 3,                       // footprint (3x3)
        true                        // requires_power
    },

    // =========================================================================
    // HazardResponse (Ticket E9-031) - Radius-based coverage, no capacity
    // =========================================================================
    {
        ServiceType::HazardResponse, // type
        ServiceTier::Post,          // tier
        "Hazard Post",              // name
        10,                         // radius (tiles)
        100,                        // effectiveness (%)
        0,                          // capacity (N/A for hazard)
        1, 1,                       // footprint (1x1)
        true                        // requires_power
    },
    {
        ServiceType::HazardResponse, // type
        ServiceTier::Station,       // tier
        "Hazard Station",           // name
        15,                         // radius (tiles)
        100,                        // effectiveness (%)
        0,                          // capacity
        2, 2,                       // footprint (2x2)
        true                        // requires_power
    },
    {
        ServiceType::HazardResponse, // type
        ServiceTier::Nexus,         // tier
        "Hazard Nexus",             // name
        20,                         // radius (tiles)
        100,                        // effectiveness (%)
        0,                          // capacity
        3, 3,                       // footprint (3x3)
        true                        // requires_power
    },

    // =========================================================================
    // Medical (Ticket E9-032) - Global effect (radius=0), capacity-based
    // =========================================================================
    {
        ServiceType::Medical,       // type
        ServiceTier::Post,          // tier
        "Medical Post",             // name
        0,                          // radius (0 = global effect)
        100,                        // effectiveness (%)
        500,                        // capacity (beings served)
        1, 1,                       // footprint (1x1)
        true                        // requires_power
    },
    {
        ServiceType::Medical,       // type
        ServiceTier::Station,       // tier
        "Medical Center",           // name
        0,                          // radius (0 = global effect)
        100,                        // effectiveness (%)
        2000,                       // capacity (beings served)
        2, 2,                       // footprint (2x2)
        true                        // requires_power
    },
    {
        ServiceType::Medical,       // type
        ServiceTier::Nexus,         // tier
        "Medical Nexus",            // name
        0,                          // radius (0 = global effect)
        100,                        // effectiveness (%)
        5000,                       // capacity (beings served)
        3, 3,                       // footprint (3x3)
        true                        // requires_power
    },

    // =========================================================================
    // Education (Ticket E9-033) - Global effect (radius=0), capacity-based
    // =========================================================================
    {
        ServiceType::Education,     // type
        ServiceTier::Post,          // tier
        "Learning Center",          // name
        0,                          // radius (0 = global effect)
        100,                        // effectiveness (%)
        300,                        // capacity (beings served)
        1, 1,                       // footprint (1x1)
        true                        // requires_power
    },
    {
        ServiceType::Education,     // type
        ServiceTier::Station,       // tier
        "Archive",                  // name
        0,                          // radius (0 = global effect)
        100,                        // effectiveness (%)
        1200,                       // capacity (beings served)
        2, 2,                       // footprint (2x2)
        true                        // requires_power
    },
    {
        ServiceType::Education,     // type
        ServiceTier::Nexus,         // tier
        "Knowledge Nexus",          // name
        0,                          // radius (0 = global effect)
        100,                        // effectiveness (%)
        3000,                       // capacity (beings served)
        3, 3,                       // footprint (3x3)
        true                        // requires_power
    }
};

// =============================================================================
// Lookup Functions
// =============================================================================

/**
 * @brief Compute the index into SERVICE_CONFIGS for a given type and tier.
 *
 * ServiceTier values are 1-based (Post=1, Station=2, Nexus=3),
 * so we subtract 1 for the array index.
 *
 * @param type The ServiceType.
 * @param tier The ServiceTier.
 * @return Index into SERVICE_CONFIGS array.
 */
constexpr uint8_t service_config_index(ServiceType type, ServiceTier tier) {
    return static_cast<uint8_t>(type) * SERVICE_TIER_COUNT
         + (static_cast<uint8_t>(tier) - 1);
}

/**
 * @brief Look up service building configuration by type and tier.
 *
 * Returns the constexpr configuration for the given ServiceType and ServiceTier.
 *
 * @param type The ServiceType to look up.
 * @param tier The ServiceTier to look up.
 * @return Reference to the ServiceBuildingConfig for the given type and tier.
 */
constexpr const ServiceBuildingConfig& get_service_building_config(ServiceType type,
                                                                    ServiceTier tier) {
    return SERVICE_CONFIGS[service_config_index(type, tier)];
}

/**
 * @brief Get the footprint area (width * height) for a service building.
 *
 * @param type The ServiceType.
 * @param tier The ServiceTier.
 * @return Footprint area in tiles.
 */
constexpr uint8_t get_service_footprint_area(ServiceType type,
                                              ServiceTier tier) {
    const auto& cfg = get_service_building_config(type, tier);
    return cfg.footprint_w * cfg.footprint_h;
}

/**
 * @brief Check if a service type uses radius-based coverage.
 *
 * Enforcer and HazardResponse use radius-based coverage (radius > 0).
 * Medical and Education use global/capacity-based coverage (radius = 0).
 *
 * @param type The ServiceType to check.
 * @return true if the service type uses radius-based coverage.
 */
constexpr bool is_radius_based_service(ServiceType type) {
    return type == ServiceType::Enforcer || type == ServiceType::HazardResponse;
}

/**
 * @brief Check if a service type uses capacity-based coverage.
 *
 * Medical and Education use capacity-based global coverage.
 * Enforcer and HazardResponse use radius-based local coverage.
 *
 * @param type The ServiceType to check.
 * @return true if the service type uses capacity-based coverage.
 */
constexpr bool is_capacity_based_service(ServiceType type) {
    return type == ServiceType::Medical || type == ServiceType::Education;
}

} // namespace services
} // namespace sims3000
