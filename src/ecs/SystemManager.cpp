/**
 * @file SystemManager.cpp
 * @brief SystemManager implementation.
 */

#include "sims3000/ecs/SystemManager.h"
#include <algorithm>
#include <chrono>
#include <cstring>

namespace sims3000 {

SystemManager::SystemManager() = default;
SystemManager::~SystemManager() = default;

void SystemManager::addSystem(std::unique_ptr<ISimulatable> system) {
    if (system) {
        m_systems.push_back({std::move(system), 0.0f});
        m_sorted = false;
    }
}

bool SystemManager::removeSystem(const char* name) {
    auto it = std::find_if(m_systems.begin(), m_systems.end(),
        [name](const SystemEntry& entry) {
            return std::strcmp(entry.system->getName(), name) == 0;
        });

    if (it != m_systems.end()) {
        m_systems.erase(it);
        return true;
    }
    return false;
}

void SystemManager::tick(const ISimulationTime& time) {
    if (!m_sorted) {
        sortSystems();
    }

    m_totalTickTime = 0.0f;

    for (auto& entry : m_systems) {
        if (m_profilingEnabled) {
            auto start = std::chrono::high_resolution_clock::now();
            entry.system->tick(time);
            auto end = std::chrono::high_resolution_clock::now();

            float ms = std::chrono::duration<float, std::milli>(end - start).count();

            // Exponential moving average
            entry.avgTickTime = entry.avgTickTime * 0.9f + ms * 0.1f;
            m_totalTickTime += ms;
        } else {
            entry.system->tick(time);
        }
    }
}

size_t SystemManager::getSystemCount() const {
    return m_systems.size();
}

ISimulatable* SystemManager::getSystem(size_t index) {
    return (index < m_systems.size()) ? m_systems[index].system.get() : nullptr;
}

const ISimulatable* SystemManager::getSystem(size_t index) const {
    return (index < m_systems.size()) ? m_systems[index].system.get() : nullptr;
}

ISimulatable* SystemManager::getSystem(const char* name) {
    for (auto& entry : m_systems) {
        if (std::strcmp(entry.system->getName(), name) == 0) {
            return entry.system.get();
        }
    }
    return nullptr;
}

const ISimulatable* SystemManager::getSystem(const char* name) const {
    for (const auto& entry : m_systems) {
        if (std::strcmp(entry.system->getName(), name) == 0) {
            return entry.system.get();
        }
    }
    return nullptr;
}

void SystemManager::setProfilingEnabled(bool enabled) {
    m_profilingEnabled = enabled;
    if (!enabled) {
        // Reset profiling data
        for (auto& entry : m_systems) {
            entry.avgTickTime = 0.0f;
        }
        m_totalTickTime = 0.0f;
    }
}

float SystemManager::getSystemTickTime(size_t index) const {
    return (index < m_systems.size()) ? m_systems[index].avgTickTime : 0.0f;
}

float SystemManager::getTotalTickTime() const {
    return m_totalTickTime;
}

void SystemManager::clear() {
    m_systems.clear();
    m_sorted = true;
}

void SystemManager::sortSystems() {
    std::stable_sort(m_systems.begin(), m_systems.end(),
        [](const SystemEntry& a, const SystemEntry& b) {
            return a.system->getPriority() < b.system->getPriority();
        });
    m_sorted = true;
}

} // namespace sims3000
