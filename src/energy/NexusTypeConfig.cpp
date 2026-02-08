/**
 * @file NexusTypeConfig.cpp
 * @brief Nexus type configuration data (Ticket 5-023)
 *
 * Defines the static configuration array for all MVP nexus types
 * and the lookup function.
 *
 * Values per CCR-004 (Energy Nexus Balance Document).
 */

#include <sims3000/energy/NexusTypeConfig.h>

namespace sims3000 {
namespace energy {

// =============================================================================
// MVP Nexus Type Configurations (CCR-004)
// =============================================================================

const NexusTypeConfig NEXUS_CONFIGS[NEXUS_TYPE_MVP_COUNT] = {
    // Carbon: Coal-equivalent, high contamination, cheapest to build
    {
        NexusType::Carbon,          // type
        "Carbon",                   // name
        100,                        // base_output (energy units/tick)
        5000,                       // build_cost (credits)
        50,                         // maintenance_cost (credits/tick)
        200,                        // contamination (units/tick)
        8,                          // coverage_radius (tiles)
        0.60f,                      // aging_floor (minimum efficiency)
        TerrainRequirement::None,   // terrain_req
        false                       // is_variable_output
    },
    // Petrochemical: Oil/gas equivalent, moderate contamination
    {
        NexusType::Petrochemical,   // type
        "Petrochemical",            // name
        150,                        // base_output
        8000,                       // build_cost
        75,                         // maintenance_cost
        120,                        // contamination
        8,                          // coverage_radius
        0.65f,                      // aging_floor
        TerrainRequirement::None,   // terrain_req
        false                       // is_variable_output
    },
    // Gaseous: Natural gas equivalent, lower contamination
    {
        NexusType::Gaseous,         // type
        "Gaseous",                  // name
        120,                        // base_output
        10000,                      // build_cost
        60,                         // maintenance_cost
        40,                         // contamination
        8,                          // coverage_radius
        0.70f,                      // aging_floor
        TerrainRequirement::None,   // terrain_req
        false                       // is_variable_output
    },
    // Nuclear: Fission, zero contamination, very high output, expensive
    {
        NexusType::Nuclear,         // type
        "Nuclear",                  // name
        400,                        // base_output
        50000,                      // build_cost
        200,                        // maintenance_cost
        0,                          // contamination
        10,                         // coverage_radius
        0.75f,                      // aging_floor
        TerrainRequirement::None,   // terrain_req
        false                       // is_variable_output
    },
    // Wind: Renewable, zero contamination, low output, variable
    {
        NexusType::Wind,            // type
        "Wind",                     // name
        30,                         // base_output
        3000,                       // build_cost
        20,                         // maintenance_cost
        0,                          // contamination
        4,                          // coverage_radius
        0.80f,                      // aging_floor
        TerrainRequirement::Ridges, // terrain_req (preferred)
        true                        // is_variable_output
    },
    // Solar: Renewable, zero contamination, moderate output, variable
    {
        NexusType::Solar,           // type
        "Solar",                    // name
        50,                         // base_output
        6000,                       // build_cost
        30,                         // maintenance_cost
        0,                          // contamination
        6,                          // coverage_radius
        0.85f,                      // aging_floor
        TerrainRequirement::None,   // terrain_req
        true                        // is_variable_output
    }
};

// =============================================================================
// Lookup Function
// =============================================================================

const NexusTypeConfig& get_nexus_config(NexusType type) {
    uint8_t index = static_cast<uint8_t>(type);
    if (index >= NEXUS_TYPE_MVP_COUNT) {
        // Fallback to Carbon for non-MVP types
        return NEXUS_CONFIGS[0];
    }
    return NEXUS_CONFIGS[index];
}

} // namespace energy
} // namespace sims3000
