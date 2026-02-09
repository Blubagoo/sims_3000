/**
 * @file ServiceTypes.h
 * @brief Service type enumerations and configuration data for Epic 9
 *        (Tickets E9-001, E9-030)
 *
 * Defines:
 * - ServiceType enum: Enforcer, HazardResponse, Medical, Education
 * - ServiceTier enum: Post=1, Station=2, Nexus=3
 * - ServiceConfig struct: Per-type/tier configuration data
 * - get_service_config(): Config lookup by type+tier
 * - String conversion functions (to_string, from_string)
 * - Enforcer-specific constants (E9-030)
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace services {

// ============================================================================
// Service Type Enum (E9-001)
// ============================================================================

/**
 * @enum ServiceType
 * @brief City service categories.
 *
 * Each service type corresponds to a municipal service that provides
 * coverage within a radius and affects city metrics.
 */
enum class ServiceType : uint8_t {
    Enforcer       = 0,   ///< Law enforcement / order maintenance
    HazardResponse = 1,   ///< Fire/hazard response
    Medical        = 2,   ///< Healthcare services
    Education      = 3    ///< Educational services
};

/// Total number of service types
constexpr uint8_t SERVICE_TYPE_COUNT = 4;

// ============================================================================
// Service Tier Enum (E9-001)
// ============================================================================

/**
 * @enum ServiceTier
 * @brief Facility tier levels for service buildings.
 *
 * Higher tiers provide larger coverage radius and capacity.
 * Values start at 1 (no tier 0).
 */
enum class ServiceTier : uint8_t {
    Post    = 1,   ///< Small facility (1x1 footprint)
    Station = 2,   ///< Medium facility (2x2 footprint)
    Nexus   = 3    ///< Large facility (3x3 footprint)
};

/// Total number of service tiers
constexpr uint8_t SERVICE_TIER_COUNT = 3;

// ============================================================================
// Service Configuration (E9-001)
// ============================================================================

/**
 * @struct ServiceConfig
 * @brief Configuration data for a service type+tier combination.
 *
 * All fields are compile-time constants used by the service system
 * to determine coverage, effectiveness, and building footprint.
 */
struct ServiceConfig {
    uint8_t  base_radius;        ///< Coverage radius in tiles (0 = global)
    uint8_t  base_effectiveness; ///< Base effectiveness percentage (0-100)
    uint16_t capacity;           ///< Service capacity (population served)
    uint8_t  footprint_width;    ///< Building footprint width in tiles
    uint8_t  footprint_height;   ///< Building footprint height in tiles
};

// ============================================================================
// Enforcer Constants (E9-030)
// ============================================================================

/// Enforcer suppression multiplier: maximum disorder reduction (70%)
constexpr float ENFORCER_SUPPRESSION_MULTIPLIER = 0.7f;

// ============================================================================
// Service Configuration Lookup (E9-001, E9-030)
// ============================================================================

/**
 * @brief Get the service configuration for a given type and tier.
 *
 * Returns a ServiceConfig with the appropriate radius, effectiveness,
 * capacity, and footprint for the specified service type and tier.
 *
 * Enforcer configs (E9-030):
 * - Post:    radius=8,  effectiveness=100, capacity=0, footprint=1x1
 * - Station: radius=12, effectiveness=100, capacity=0, footprint=2x2
 * - Nexus:   radius=16, effectiveness=100, capacity=0, footprint=3x3
 *
 * @param type The service type.
 * @param tier The service tier (1-3).
 * @return ServiceConfig for the type+tier combination.
 */
inline ServiceConfig get_service_config(ServiceType type, ServiceTier tier) {
    switch (type) {
        case ServiceType::Enforcer:
            switch (tier) {
                case ServiceTier::Post:    return { 8, 100, 0, 1, 1 };
                case ServiceTier::Station: return { 12, 100, 0, 2, 2 };
                case ServiceTier::Nexus:   return { 16, 100, 0, 3, 3 };
                default:                   return { 0, 0, 0, 0, 0 };
            }
        case ServiceType::HazardResponse:
            switch (tier) {
                case ServiceTier::Post:    return { 8, 100, 0, 1, 1 };
                case ServiceTier::Station: return { 12, 100, 0, 2, 2 };
                case ServiceTier::Nexus:   return { 16, 100, 0, 3, 3 };
                default:                   return { 0, 0, 0, 0, 0 };
            }
        case ServiceType::Medical:
            switch (tier) {
                case ServiceTier::Post:    return { 8, 100, 100, 1, 1 };
                case ServiceTier::Station: return { 12, 100, 500, 2, 2 };
                case ServiceTier::Nexus:   return { 16, 100, 2000, 3, 3 };
                default:                   return { 0, 0, 0, 0, 0 };
            }
        case ServiceType::Education:
            switch (tier) {
                case ServiceTier::Post:    return { 8, 100, 200, 1, 1 };
                case ServiceTier::Station: return { 12, 100, 1000, 2, 2 };
                case ServiceTier::Nexus:   return { 16, 100, 5000, 3, 3 };
                default:                   return { 0, 0, 0, 0, 0 };
            }
        default:
            return { 0, 0, 0, 0, 0 };
    }
}

// ============================================================================
// String Conversion Functions (E9-001)
// ============================================================================

/**
 * @brief Convert ServiceType enum to a human-readable string.
 *
 * @param type The ServiceType to convert.
 * @return Null-terminated string name of the service type.
 */
inline const char* service_type_to_string(ServiceType type) {
    switch (type) {
        case ServiceType::Enforcer:       return "Enforcer";
        case ServiceType::HazardResponse: return "HazardResponse";
        case ServiceType::Medical:        return "Medical";
        case ServiceType::Education:      return "Education";
        default:                          return "Unknown";
    }
}

/**
 * @brief Convert a string to a ServiceType enum.
 *
 * @param str The string to convert.
 * @param out_type Output ServiceType value.
 * @return true if conversion succeeded, false if string not recognized.
 */
inline bool service_type_from_string(const char* str, ServiceType& out_type) {
    if (!str) return false;

    // Manual string comparison (no <cstring> dependency in header)
    // Compare character by character for each known type
    const char* enforcer = "Enforcer";
    const char* hazard = "HazardResponse";
    const char* medical = "Medical";
    const char* education = "Education";

    // Simple strcmp inline
    auto str_eq = [](const char* a, const char* b) -> bool {
        while (*a && *b) {
            if (*a != *b) return false;
            ++a;
            ++b;
        }
        return *a == *b;
    };

    if (str_eq(str, enforcer))  { out_type = ServiceType::Enforcer;       return true; }
    if (str_eq(str, hazard))    { out_type = ServiceType::HazardResponse; return true; }
    if (str_eq(str, medical))   { out_type = ServiceType::Medical;        return true; }
    if (str_eq(str, education)) { out_type = ServiceType::Education;      return true; }

    return false;
}

/**
 * @brief Convert ServiceTier enum to a human-readable string.
 *
 * @param tier The ServiceTier to convert.
 * @return Null-terminated string name of the service tier.
 */
inline const char* service_tier_to_string(ServiceTier tier) {
    switch (tier) {
        case ServiceTier::Post:    return "Post";
        case ServiceTier::Station: return "Station";
        case ServiceTier::Nexus:   return "Nexus";
        default:                   return "Unknown";
    }
}

/**
 * @brief Convert a string to a ServiceTier enum.
 *
 * @param str The string to convert.
 * @param out_tier Output ServiceTier value.
 * @return true if conversion succeeded, false if string not recognized.
 */
inline bool service_tier_from_string(const char* str, ServiceTier& out_tier) {
    if (!str) return false;

    auto str_eq = [](const char* a, const char* b) -> bool {
        while (*a && *b) {
            if (*a != *b) return false;
            ++a;
            ++b;
        }
        return *a == *b;
    };

    if (str_eq(str, "Post"))    { out_tier = ServiceTier::Post;    return true; }
    if (str_eq(str, "Station")) { out_tier = ServiceTier::Station; return true; }
    if (str_eq(str, "Nexus"))   { out_tier = ServiceTier::Nexus;   return true; }

    return false;
}

/**
 * @brief Check if a ServiceType value is valid.
 * @param value The raw uint8 value to check.
 * @return true if value is a valid ServiceType (0-3).
 */
constexpr bool isValidServiceType(uint8_t value) {
    return value < SERVICE_TYPE_COUNT;
}

/**
 * @brief Check if a ServiceTier value is valid.
 * @param value The raw uint8 value to check.
 * @return true if value is a valid ServiceTier (1-3).
 */
constexpr bool isValidServiceTier(uint8_t value) {
    return value >= 1 && value <= SERVICE_TIER_COUNT;
}

} // namespace services
} // namespace sims3000
