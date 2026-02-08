/**
 * @file MigrationEffects.h
 * @brief Population migration calculation from external connections (Epic 8, Ticket E8-024)
 *
 * Calculates inbound and outbound migration based on external connection capacity:
 *
 * Inbound Migration:
 *   immigration_rate = migration_capacity * demand_factor * harmony_factor
 *   max_per_cycle = 10 + (external_connections * 5)
 *
 * Outbound Migration:
 *   emigration_rate = migration_capacity * (disorder_index / 100) * tribute_penalty
 *
 * Where:
 * - migration_capacity: sum of all active connections
 * - demand_factor: from NPC neighbors (0.5-1.5)
 * - harmony_factor: city satisfaction metric (0.0-1.0, default 0.5)
 * - disorder_index: 0-100 (higher = more emigration)
 * - tribute_penalty: multiplier (1.0 = no penalty, higher = more emigration)
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace port {

/**
 * @struct MigrationInput
 * @brief Input parameters for migration calculation.
 *
 * Aggregates all factors that influence immigration and emigration rates.
 */
struct MigrationInput {
    uint32_t total_migration_capacity = 0;  ///< Sum of all active connection migration capacities
    uint32_t external_connection_count = 0; ///< Number of active external connections
    float demand_factor = 1.0f;             ///< From NPC neighbors (0.5-1.5)
    float harmony_factor = 0.5f;            ///< City satisfaction metric (0.0-1.0)
    float disorder_index = 0.0f;            ///< Disorder level (0-100)
    float tribute_penalty = 1.0f;           ///< Tribute multiplier (1.0+ = more emigration)
};

/**
 * @struct MigrationResult
 * @brief Output of migration calculation.
 *
 * Contains computed immigration rate, emigration rate, net migration,
 * and the per-cycle immigration cap.
 */
struct MigrationResult {
    int32_t immigration_rate = 0;  ///< Computed inbound migration (before cap)
    int32_t emigration_rate = 0;   ///< Computed outbound migration
    int32_t net_migration = 0;     ///< immigration_rate - emigration_rate
    int32_t max_immigration = 0;   ///< Per-cycle immigration cap: 10 + (connections * 5)
};

/**
 * @brief Calculate population migration from external connections.
 *
 * Computes immigration and emigration rates based on connection capacity,
 * demand factors, city harmony, disorder, and tribute penalties.
 *
 * Immigration is capped at max_immigration = 10 + (external_connections * 5).
 * The immigration_rate in the result is the capped value.
 * Net migration = capped immigration - emigration.
 *
 * Input values are clamped to their valid ranges:
 * - demand_factor: clamped to [0.5, 1.5]
 * - harmony_factor: clamped to [0.0, 1.0]
 * - disorder_index: clamped to [0.0, 100.0]
 * - tribute_penalty: clamped to minimum 1.0
 *
 * @param input The migration input parameters.
 * @return MigrationResult with computed rates and cap.
 */
MigrationResult calculate_migration(const MigrationInput& input);

} // namespace port
} // namespace sims3000
