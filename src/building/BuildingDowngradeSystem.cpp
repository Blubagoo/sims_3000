/**
 * @file BuildingDowngradeSystem.cpp
 * @brief Implementation of BuildingDowngradeSystem (Ticket 4-033)
 */

#include <sims3000/building/BuildingDowngradeSystem.h>
#include <sims3000/zone/ZoneSystem.h>
#include <sims3000/zone/ZoneTypes.h>

namespace sims3000 {
namespace building {

BuildingDowngradeSystem::BuildingDowngradeSystem(BuildingFactory* factory, zone::ZoneSystem* zone_system)
    : m_factory(factory)
    , m_zone_system(zone_system)
    , m_config()
    , m_pending_events()
{
}

void BuildingDowngradeSystem::tick(uint32_t current_tick) {
    if (!m_factory) {
        return;
    }

    // Only check at check_interval
    if (m_config.check_interval > 0 && (current_tick % m_config.check_interval) != 0) {
        return;
    }

    auto& entities = m_factory->get_entities_mut();

    for (auto& entity : entities) {
        if (check_downgrade_conditions(entity, current_tick)) {
            execute_downgrade(entity, current_tick);
        }
    }
}

void BuildingDowngradeSystem::set_config(const DowngradeConfig& config) {
    m_config = config;
}

const DowngradeConfig& BuildingDowngradeSystem::get_config() const {
    return m_config;
}

const std::vector<BuildingDowngradedEvent>& BuildingDowngradeSystem::get_pending_events() const {
    return m_pending_events;
}

void BuildingDowngradeSystem::clear_pending_events() {
    m_pending_events.clear();
}

bool BuildingDowngradeSystem::check_downgrade_conditions(const BuildingEntity& entity, uint32_t current_tick) const {
    // State must be Active
    if (entity.building.getBuildingState() != BuildingState::Active) {
        return false;
    }

    // Level > min_level
    if (entity.building.level <= m_config.min_level) {
        return false;
    }

    // Check for land value below threshold: level * 50
    // Land value check uses zone system desirability (same approach as upgrade)
    // For now, we check demand as the primary trigger

    // Check for sustained negative demand
    if (m_zone_system) {
        zone::ZoneType zone_type = static_cast<zone::ZoneType>(entity.building.zone_type);
        std::int8_t demand = m_zone_system->get_demand_for_type(zone_type, entity.owner_id);
        if (demand < 0) {
            // Check if negative demand has been sustained for downgrade_delay ticks
            if (current_tick > entity.building.state_changed_tick) {
                uint32_t ticks_since_change = current_tick - entity.building.state_changed_tick;
                if (ticks_since_change >= m_config.downgrade_delay) {
                    return true;
                }
            }
        }
    }

    return false;
}

void BuildingDowngradeSystem::execute_downgrade(BuildingEntity& entity, uint32_t current_tick) {
    uint8_t old_level = entity.building.level;
    uint8_t new_level = old_level - 1;

    // Decrement level
    entity.building.level = new_level;

    // Recalculate capacity: base_capacity * level_multipliers[level]
    if (new_level < m_config.level_multipliers.size() && old_level < m_config.level_multipliers.size()) {
        float old_multiplier = m_config.level_multipliers[old_level];
        float new_multiplier = m_config.level_multipliers[new_level];
        if (old_multiplier > 0.0f) {
            float base_capacity = static_cast<float>(entity.building.capacity) / old_multiplier;
            entity.building.capacity = static_cast<uint16_t>(base_capacity * new_multiplier);
        }
    }

    // Update state_changed_tick
    entity.building.state_changed_tick = current_tick;

    // Emit BuildingDowngradedEvent
    m_pending_events.emplace_back(entity.entity_id, old_level, new_level);
}

} // namespace building
} // namespace sims3000
