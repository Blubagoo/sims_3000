/**
 * @file PopulationSystem.h
 * @brief Population simulation system skeleton (Ticket E10-014)
 *
 * Manages per-player population and employment data. Runs at tick_priority 50
 * with frequency-gated sub-phases:
 * - Demographics: every 100 ticks (5s)
 * - Migration: every 20 ticks (1s)
 * - Employment: every tick
 *
 * Sub-phase implementations are stubs in this skeleton; they will be
 * filled in by later tickets (E10-015 through E10-018).
 */

#ifndef SIMS3000_POPULATION_POPULATIONSYSTEM_H
#define SIMS3000_POPULATION_POPULATIONSYSTEM_H

#include <cstdint>

#include "sims3000/core/ISimulatable.h"
#include "sims3000/core/types.h"
#include "sims3000/population/PopulationData.h"
#include "sims3000/population/EmploymentData.h"

namespace sims3000 {
namespace population {

/**
 * @class PopulationSystem
 * @brief Manages city population and employment simulation.
 *
 * Each active player has a PopulationData and EmploymentData instance.
 * The tick() method runs frequency-gated phases:
 * - Demographics (births/deaths/aging): every DEMOGRAPHIC_CYCLE_TICKS
 * - Migration (in/out flow): every MIGRATION_CYCLE_TICKS
 * - Employment (job matching): every tick
 */
class PopulationSystem : public ISimulatable {
public:
    PopulationSystem();

    // ISimulatable interface
    void tick(const ISimulationTime& time) override;
    int getPriority() const override { return 50; }
    const char* getName() const override { return "PopulationSystem"; }

    // Per-player data access (const)
    const PopulationData& get_population(PlayerID player_id) const;
    const EmploymentData& get_employment(PlayerID player_id) const;

    // Per-player data access (mutable)
    PopulationData& get_population_mut(PlayerID player_id);
    EmploymentData& get_employment_mut(PlayerID player_id);

    // Player management
    void add_player(PlayerID player_id);
    void remove_player(PlayerID player_id);
    bool has_player(PlayerID player_id) const;

    /// Frequency gating constants
    static constexpr uint32_t DEMOGRAPHIC_CYCLE_TICKS = 100; ///< Demographics every 100 ticks (5s)
    static constexpr uint32_t MIGRATION_CYCLE_TICKS = 20;    ///< Migration every 20 ticks (1s)
    static constexpr uint32_t EMPLOYMENT_CYCLE_TICKS = 1;    ///< Employment every tick

private:
    // Calculation phases (stubs for now, implemented in later tickets)
    void update_demographics(PlayerID player_id, const ISimulationTime& time);
    void update_employment(PlayerID player_id, const ISimulationTime& time);
    void update_migration(PlayerID player_id, const ISimulationTime& time);
    void update_history(PlayerID player_id);

    // Per-player data storage (max 4 players)
    static constexpr uint8_t MAX_PLAYERS = 4;
    PopulationData m_population[MAX_PLAYERS];
    EmploymentData m_employment[MAX_PLAYERS];
    bool m_player_active[MAX_PLAYERS];

    // Static default data for invalid queries
    static const PopulationData s_empty_population;
    static const EmploymentData s_empty_employment;
};

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_POPULATIONSYSTEM_H
