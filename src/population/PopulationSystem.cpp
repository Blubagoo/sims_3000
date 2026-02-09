/**
 * @file PopulationSystem.cpp
 * @brief Population simulation system implementation (Ticket E10-014)
 *
 * Skeleton implementation with frequency-gated tick phases.
 * Sub-phase bodies are stubs; filled in by later tickets.
 */

#include "sims3000/population/PopulationSystem.h"

namespace sims3000 {
namespace population {

// Static default data for invalid queries
const PopulationData PopulationSystem::s_empty_population = PopulationData{};
const EmploymentData PopulationSystem::s_empty_employment = EmploymentData{};

PopulationSystem::PopulationSystem() {
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        m_player_active[i] = false;
        m_population[i] = PopulationData{};
        m_employment[i] = EmploymentData{};
    }
}

void PopulationSystem::tick(const ISimulationTime& time) {
    const SimulationTick current_tick = time.getCurrentTick();

    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        if (!m_player_active[i]) {
            continue;
        }

        const PlayerID player_id = i;

        // Demographics phase: every DEMOGRAPHIC_CYCLE_TICKS
        if (current_tick % DEMOGRAPHIC_CYCLE_TICKS == 0) {
            update_demographics(player_id, time);
        }

        // Migration phase: every MIGRATION_CYCLE_TICKS
        if (current_tick % MIGRATION_CYCLE_TICKS == 0) {
            update_migration(player_id, time);
        }

        // Employment phase: every tick
        update_employment(player_id, time);
    }
}

const PopulationData& PopulationSystem::get_population(PlayerID player_id) const {
    if (player_id >= MAX_PLAYERS || !m_player_active[player_id]) {
        return s_empty_population;
    }
    return m_population[player_id];
}

PopulationData& PopulationSystem::get_population_mut(PlayerID player_id) {
    // Note: returns mutable reference; caller must ensure player exists.
    // If invalid, we return the first slot (defensive). Real code should
    // check has_player() first.
    if (player_id >= MAX_PLAYERS || !m_player_active[player_id]) {
        // Return first slot as fallback (should not happen in correct usage)
        return m_population[0];
    }
    return m_population[player_id];
}

const EmploymentData& PopulationSystem::get_employment(PlayerID player_id) const {
    if (player_id >= MAX_PLAYERS || !m_player_active[player_id]) {
        return s_empty_employment;
    }
    return m_employment[player_id];
}

EmploymentData& PopulationSystem::get_employment_mut(PlayerID player_id) {
    if (player_id >= MAX_PLAYERS || !m_player_active[player_id]) {
        return m_employment[0];
    }
    return m_employment[player_id];
}

void PopulationSystem::add_player(PlayerID player_id) {
    if (player_id >= MAX_PLAYERS) {
        return;
    }
    m_player_active[player_id] = true;
    m_population[player_id] = PopulationData{};
    m_employment[player_id] = EmploymentData{};
}

void PopulationSystem::remove_player(PlayerID player_id) {
    if (player_id >= MAX_PLAYERS) {
        return;
    }
    m_player_active[player_id] = false;
    m_population[player_id] = PopulationData{};
    m_employment[player_id] = EmploymentData{};
}

bool PopulationSystem::has_player(PlayerID player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return false;
    }
    return m_player_active[player_id];
}

// Stub implementations — filled in by later tickets (E10-015 through E10-018)
void PopulationSystem::update_demographics(PlayerID /*player_id*/, const ISimulationTime& /*time*/) {
    // Stub: demographics calculation (E10-015)
}

void PopulationSystem::update_employment(PlayerID /*player_id*/, const ISimulationTime& /*time*/) {
    // Stub: employment matching (E10-016)
}

void PopulationSystem::update_migration(PlayerID /*player_id*/, const ISimulationTime& /*time*/) {
    // Stub: migration calculation (E10-017)
}

void PopulationSystem::update_history(PlayerID /*player_id*/) {
    // Stub: history ring buffer update (E10-018)
}

} // namespace population
} // namespace sims3000
