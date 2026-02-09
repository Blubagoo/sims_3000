/**
 * @file DemandSystem.cpp
 * @brief Demand simulation system implementation (Ticket E10-042)
 *
 * Skeleton implementation with frequency-gated tick updates.
 * The update_demand() body is a stub; filled in by later tickets
 * (E10-043 through E10-045).
 */

#include "sims3000/demand/DemandSystem.h"

namespace sims3000 {
namespace demand {

// Static default data for invalid queries
const DemandData DemandSystem::s_empty_demand = DemandData{};

DemandSystem::DemandSystem() {
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        m_player_active[i] = false;
        m_demand[i] = DemandData{};
    }
}

void DemandSystem::tick(const ISimulationTime& time) {
    const SimulationTick current_tick = time.getCurrentTick();

    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        if (!m_player_active[i]) {
            continue;
        }

        // Frequency gating: update demand every DEMAND_CYCLE_TICKS
        if (current_tick % DEMAND_CYCLE_TICKS == 0) {
            update_demand(i, time);
        }
    }
}

float DemandSystem::get_demand(uint8_t zone_type, uint32_t player_id) const {
    if (player_id >= MAX_PLAYERS || !m_player_active[player_id]) {
        return 0.0f;
    }

    const DemandData& data = m_demand[player_id];
    switch (zone_type) {
        case 0: return static_cast<float>(data.habitation_demand);
        case 1: return static_cast<float>(data.exchange_demand);
        case 2: return static_cast<float>(data.fabrication_demand);
        default: return 0.0f;
    }
}

uint32_t DemandSystem::get_demand_cap(uint8_t zone_type, uint32_t player_id) const {
    if (player_id >= MAX_PLAYERS || !m_player_active[player_id]) {
        return 0;
    }

    const DemandData& data = m_demand[player_id];
    switch (zone_type) {
        case 0: return data.habitation_cap;
        case 1: return data.exchange_cap;
        case 2: return data.fabrication_cap;
        default: return 0;
    }
}

bool DemandSystem::has_positive_demand(uint8_t zone_type, uint32_t player_id) const {
    return get_demand(zone_type, player_id) > 0.0f;
}

const DemandData& DemandSystem::get_demand_data(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS || !m_player_active[player_id]) {
        return s_empty_demand;
    }
    return m_demand[player_id];
}

DemandData& DemandSystem::get_demand_data_mut(uint8_t player_id) {
    // Note: returns mutable reference; caller must ensure player exists.
    // If invalid, return first slot as defensive fallback.
    if (player_id >= MAX_PLAYERS || !m_player_active[player_id]) {
        return m_demand[0];
    }
    return m_demand[player_id];
}

void DemandSystem::add_player(uint8_t player_id) {
    if (player_id >= MAX_PLAYERS) {
        return;
    }
    m_player_active[player_id] = true;
    m_demand[player_id] = DemandData{};
}

void DemandSystem::remove_player(uint8_t player_id) {
    if (player_id >= MAX_PLAYERS) {
        return;
    }
    m_player_active[player_id] = false;
    m_demand[player_id] = DemandData{};
}

bool DemandSystem::has_player(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return false;
    }
    return m_player_active[player_id];
}

// Stub implementation — filled in by later tickets (E10-043 through E10-045)
void DemandSystem::update_demand(uint8_t /*player_id*/, const ISimulationTime& /*time*/) {
    // Stub: demand formula calculations (E10-043/044/045)
}

} // namespace demand
} // namespace sims3000
