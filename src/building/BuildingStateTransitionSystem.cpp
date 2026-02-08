/**
 * @file BuildingStateTransitionSystem.cpp
 * @brief Implementation of BuildingStateTransitionSystem (Ticket 4-028)
 */

#include <sims3000/building/BuildingStateTransitionSystem.h>

namespace sims3000 {
namespace building {

BuildingStateTransitionSystem::BuildingStateTransitionSystem(
    BuildingFactory* factory,
    BuildingGrid* grid,
    const IEnergyProvider* energy,
    const IFluidProvider* fluid,
    const ITransportProvider* transport)
    : m_factory(factory)
    , m_grid(grid)
    , m_energy(energy)
    , m_fluid(fluid)
    , m_transport(transport)
    , m_config()
    , m_grace_states()
    , m_pending_abandoned()
    , m_pending_restored()
    , m_pending_derelict()
    , m_pending_deconstructed()
{
}

void BuildingStateTransitionSystem::tick(uint32_t current_tick) {
    if (!m_factory) {
        return;
    }

    auto& entities = m_factory->get_entities_mut();

    for (auto& entity : entities) {
        BuildingState state = entity.building.getBuildingState();

        switch (state) {
            case BuildingState::Active:
                evaluate_active(entity, current_tick);
                break;
            case BuildingState::Abandoned:
                evaluate_abandoned(entity, current_tick);
                break;
            case BuildingState::Derelict:
                evaluate_derelict(entity, current_tick);
                break;
            default:
                // Materializing and Deconstructed are handled by other systems
                break;
        }
    }
}

void BuildingStateTransitionSystem::evaluate_active(BuildingEntity& entity, uint32_t current_tick) {
    // Get or create grace state for this entity
    auto& grace = m_grace_states[entity.entity_id];

    bool energy_ok = true;
    bool fluid_ok = true;
    bool transport_ok = true;

    // Check energy
    if (m_energy) {
        energy_ok = m_energy->is_powered(entity.entity_id);
    }

    // Check fluid
    if (m_fluid) {
        fluid_ok = m_fluid->has_fluid(entity.entity_id);
    }

    // Check transport
    if (m_transport) {
        transport_ok = m_transport->is_road_accessible_at(
            static_cast<uint32_t>(entity.grid_x),
            static_cast<uint32_t>(entity.grid_y),
            3
        );
    }

    // Update grace counters
    if (!energy_ok) {
        grace.ticks_without_energy++;
    } else {
        grace.ticks_without_energy = 0;
    }

    if (!fluid_ok) {
        grace.ticks_without_fluid++;
    } else {
        grace.ticks_without_fluid = 0;
    }

    if (!transport_ok) {
        grace.ticks_without_transport++;
    } else {
        grace.ticks_without_transport = 0;
    }

    // Check if any grace counter exceeds its per-service grace period (Ticket 4-029)
    // A grace_period of 0 means IMMEDIATE abandon (1 tick without service exceeds 0)
    // Uses get_*_grace() to resolve USE_LEGACY sentinel for backward compatibility
    bool energy_exceeded = (!energy_ok) &&
        (grace.ticks_without_energy > m_config.get_energy_grace());
    bool fluid_exceeded = (!fluid_ok) &&
        (grace.ticks_without_fluid > m_config.get_fluid_grace());
    bool transport_exceeded = (!transport_ok) &&
        (grace.ticks_without_transport > m_config.get_transport_grace());

    bool grace_exceeded = energy_exceeded || fluid_exceeded || transport_exceeded;

    if (grace_exceeded) {
        // Transition to Abandoned
        entity.building.setBuildingState(BuildingState::Abandoned);
        entity.building.state_changed_tick = current_tick;
        entity.building.abandon_timer = static_cast<uint16_t>(
            m_config.abandon_timer_ticks > 65535 ? 65535 : m_config.abandon_timer_ticks
        );

        // Reset grace state
        grace.ticks_without_energy = 0;
        grace.ticks_without_fluid = 0;
        grace.ticks_without_transport = 0;

        // Emit event
        m_pending_abandoned.emplace_back(
            entity.entity_id, entity.owner_id,
            entity.grid_x, entity.grid_y
        );
    }
}

void BuildingStateTransitionSystem::evaluate_abandoned(BuildingEntity& entity, uint32_t current_tick) {
    // Check if all services are restored
    if (are_all_services_available(entity)) {
        // Transition back to Active
        entity.building.setBuildingState(BuildingState::Active);
        entity.building.state_changed_tick = current_tick;
        entity.building.abandon_timer = 0;

        // Reset grace state
        m_grace_states[entity.entity_id] = ServiceGraceState{};

        // Emit restored event
        m_pending_restored.emplace_back(
            entity.entity_id, entity.owner_id,
            entity.grid_x, entity.grid_y
        );
        return;
    }

    // Decrement abandon timer
    if (entity.building.abandon_timer > 0) {
        entity.building.abandon_timer--;
    }

    // Check if abandon timer expired
    if (entity.building.abandon_timer == 0) {
        // Transition to Derelict
        entity.building.setBuildingState(BuildingState::Derelict);
        entity.building.state_changed_tick = current_tick;

        // Emit derelict event
        m_pending_derelict.emplace_back(
            entity.entity_id, entity.owner_id,
            entity.grid_x, entity.grid_y
        );
    }
}

void BuildingStateTransitionSystem::evaluate_derelict(BuildingEntity& entity, uint32_t current_tick) {
    // Check if enough time has passed since becoming derelict
    uint32_t ticks_in_derelict = current_tick - entity.building.state_changed_tick;

    if (ticks_in_derelict >= m_config.derelict_timer_ticks) {
        // Transition to Deconstructed
        entity.building.setBuildingState(BuildingState::Deconstructed);
        entity.building.state_changed_tick = current_tick;

        // Clear grid footprint
        if (m_grid) {
            m_grid->clear_footprint(
                entity.grid_x, entity.grid_y,
                entity.building.footprint_w, entity.building.footprint_h
            );
        }

        // Add debris data
        entity.has_debris = true;
        entity.debris = DebrisComponent(
            entity.building.template_id,
            entity.building.footprint_w,
            entity.building.footprint_h
        );

        // Remove construction flag if present
        entity.has_construction = false;

        // Clean up grace state
        m_grace_states.erase(entity.entity_id);

        // Emit deconstructed event
        m_pending_deconstructed.emplace_back(
            entity.entity_id, entity.owner_id,
            entity.grid_x, entity.grid_y,
            false  // was_player_initiated = false (automatic decay)
        );
    }
}

bool BuildingStateTransitionSystem::are_all_services_available(const BuildingEntity& entity) const {
    bool energy_ok = true;
    bool fluid_ok = true;
    bool transport_ok = true;

    if (m_energy) {
        energy_ok = m_energy->is_powered(entity.entity_id);
    }

    if (m_fluid) {
        fluid_ok = m_fluid->has_fluid(entity.entity_id);
    }

    if (m_transport) {
        transport_ok = m_transport->is_road_accessible_at(
            static_cast<uint32_t>(entity.grid_x),
            static_cast<uint32_t>(entity.grid_y),
            3
        );
    }

    return energy_ok && fluid_ok && transport_ok;
}

void BuildingStateTransitionSystem::set_config(const StateTransitionConfig& config) {
    m_config = config;
}

const StateTransitionConfig& BuildingStateTransitionSystem::get_config() const {
    return m_config;
}

const std::vector<BuildingAbandonedEvent>& BuildingStateTransitionSystem::get_pending_abandoned_events() const {
    return m_pending_abandoned;
}

const std::vector<BuildingRestoredEvent>& BuildingStateTransitionSystem::get_pending_restored_events() const {
    return m_pending_restored;
}

const std::vector<BuildingDerelictEvent>& BuildingStateTransitionSystem::get_pending_derelict_events() const {
    return m_pending_derelict;
}

const std::vector<BuildingDeconstructedEvent>& BuildingStateTransitionSystem::get_pending_deconstructed_events() const {
    return m_pending_deconstructed;
}

void BuildingStateTransitionSystem::clear_all_pending_events() {
    m_pending_abandoned.clear();
    m_pending_restored.clear();
    m_pending_derelict.clear();
    m_pending_deconstructed.clear();
}

} // namespace building
} // namespace sims3000
