/**
 * @file Registry.cpp
 * @brief Registry implementation.
 */

#include "sims3000/ecs/Registry.h"

namespace sims3000 {

Registry::Registry() = default;
Registry::~Registry() = default;

EntityID Registry::create() {
    return static_cast<EntityID>(m_registry.create());
}

void Registry::destroy(EntityID entity) {
    m_registry.destroy(static_cast<entt::entity>(entity));
}

bool Registry::valid(EntityID entity) const {
    return m_registry.valid(static_cast<entt::entity>(entity));
}

void Registry::clear() {
    m_registry.clear();
}

size_t Registry::size() const {
    // In EnTT 3.16+, use storage size minus free list size
    const auto* storage = m_registry.storage<entt::entity>();
    if (!storage) return 0;

    // Count only valid entities by iterating
    size_t count = 0;
    for (auto it = storage->begin(); it != storage->end(); ++it) {
        if (m_registry.valid(*it)) {
            ++count;
        }
    }
    return count;
}

entt::registry& Registry::raw() {
    return m_registry;
}

const entt::registry& Registry::raw() const {
    return m_registry;
}

} // namespace sims3000
