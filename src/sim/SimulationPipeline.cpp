/**
 * @file SimulationPipeline.cpp
 * @brief Implementation of SimulationPipeline (Ticket 4-048)
 */

#include <sims3000/sim/SimulationPipeline.h>

namespace sims3000 {
namespace sim {

void SimulationPipeline::register_system(int priority, std::function<void(float)> tick_fn, const char* name) {
    m_systems.push_back({priority, std::move(tick_fn), name});
    m_sorted = false;
}

void SimulationPipeline::tick(float delta_time) {
    if (!m_sorted) {
        std::stable_sort(m_systems.begin(), m_systems.end());
        m_sorted = true;
    }

    for (auto& entry : m_systems) {
        entry.tick_fn(delta_time);
    }
}

size_t SimulationPipeline::system_count() const {
    return m_systems.size();
}

std::vector<const char*> SimulationPipeline::get_execution_order() const {
    // Need to return sorted order without modifying const state
    std::vector<SystemEntry> sorted = m_systems;
    std::stable_sort(sorted.begin(), sorted.end());

    std::vector<const char*> names;
    names.reserve(sorted.size());
    for (const auto& entry : sorted) {
        names.push_back(entry.name);
    }
    return names;
}

} // namespace sim
} // namespace sims3000
