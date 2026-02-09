/**
 * @file EnforcerSuppression.h
 * @brief Enforcer-based disorder suppression integration (Ticket E10-076).
 *
 * Applies disorder suppression from enforcer posts (stub until Epic 9).
 * Queries enforcer coverage via IServiceQueryable and reduces disorder
 * on tiles with enforcer presence using a linear falloff model.
 *
 * Suppression formula:
 * suppression = coverage * effectiveness * ENFORCER_SUPPRESSION_MULTIPLIER * current_disorder
 *
 * Where:
 * - coverage: 0.0-1.0, from IServiceQueryable::get_coverage_at()
 * - effectiveness: 0.0-1.0, from IServiceQueryable::get_effectiveness()
 * - ENFORCER_SUPPRESSION_MULTIPLIER: 0.7 (70% max reduction)
 * - current_disorder: Current disorder level at tile (from current buffer)
 *
 * @see E10-076
 */

#ifndef SIMS3000_DISORDER_ENFORCERSUPPRESSION_H
#define SIMS3000_DISORDER_ENFORCERSUPPRESSION_H

#include <sims3000/disorder/DisorderGrid.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <cstdint>

namespace sims3000 {
namespace disorder {

/**
 * @brief Apply enforcer-based disorder suppression across the grid.
 *
 * For each tile with enforcer coverage > 0, calculates suppression based on
 * coverage, effectiveness, and the ENFORCER_SUPPRESSION_MULTIPLIER (0.7).
 * Subtracts the suppression amount from the current disorder level.
 *
 * Until Epic 9 is implemented, the service_query stub returns 0 coverage,
 * so this function performs no actual suppression. However, the infrastructure
 * is in place for real IServiceQueryable implementations.
 *
 * Algorithm per tile (x, y):
 * - Get coverage from service_query.get_coverage_at (enforcer type, x, y, player_id)
 * - If coverage <= 0, skip
 * - Get effectiveness from service_query.get_effectiveness (enforcer type, player_id)
 * - Get current_level from grid.get_level (x, y)
 * - Calculate suppression = current_level * coverage * effectiveness * 0.7
 * - Apply suppression to grid
 *
 * @param grid Disorder grid to apply suppression to (modified in place).
 * @param service_query Service queryable interface for coverage/effectiveness.
 * @param player_id Player ID to query coverage for (default 0).
 */
void apply_enforcer_suppression(
    DisorderGrid& grid,
    const building::IServiceQueryable& service_query,
    uint8_t player_id = 0
);

} // namespace disorder
} // namespace sims3000

#endif // SIMS3000_DISORDER_ENFORCERSUPPRESSION_H
