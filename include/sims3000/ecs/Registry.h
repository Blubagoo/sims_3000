/**
 * @file Registry.h
 * @brief ECS registry wrapper.
 */

#ifndef SIMS3000_ECS_REGISTRY_H
#define SIMS3000_ECS_REGISTRY_H

#include "sims3000/core/types.h"
#include <entt/entt.hpp>

namespace sims3000 {

/**
 * @class Registry
 * @brief Wrapper around EnTT registry with convenience methods.
 *
 * Provides a clean interface for entity management while
 * allowing direct access to the underlying EnTT registry
 * for performance-critical operations.
 */
class Registry {
public:
    Registry();
    ~Registry();

    // Non-copyable, movable
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = default;
    Registry& operator=(Registry&&) = default;

    /**
     * Create a new entity.
     * @return Entity ID
     */
    EntityID create();

    /**
     * Destroy an entity and all its components.
     * @param entity Entity to destroy
     */
    void destroy(EntityID entity);

    /**
     * Check if an entity exists.
     * @param entity Entity to check
     * @return true if entity is valid
     */
    bool valid(EntityID entity) const;

    /**
     * Add a component to an entity.
     * @tparam T Component type
     * @param entity Target entity
     * @param args Component constructor arguments
     * @return Reference to created component
     */
    template<typename T, typename... Args>
    T& emplace(EntityID entity, Args&&... args) {
        return m_registry.emplace<T>(
            static_cast<entt::entity>(entity),
            std::forward<Args>(args)...
        );
    }

    /**
     * Get a component from an entity.
     * @tparam T Component type
     * @param entity Target entity
     * @return Reference to component
     */
    template<typename T>
    T& get(EntityID entity) {
        return m_registry.get<T>(static_cast<entt::entity>(entity));
    }

    /**
     * Get a component from an entity (const).
     * @tparam T Component type
     * @param entity Target entity
     * @return Const reference to component
     */
    template<typename T>
    const T& get(EntityID entity) const {
        return m_registry.get<T>(static_cast<entt::entity>(entity));
    }

    /**
     * Try to get a component from an entity.
     * @tparam T Component type
     * @param entity Target entity
     * @return Pointer to component, or nullptr if not found
     */
    template<typename T>
    T* tryGet(EntityID entity) {
        return m_registry.try_get<T>(static_cast<entt::entity>(entity));
    }

    /**
     * Try to get a component from an entity (const).
     * @tparam T Component type
     * @param entity Target entity
     * @return Const pointer to component, or nullptr if not found
     */
    template<typename T>
    const T* tryGet(EntityID entity) const {
        return m_registry.try_get<T>(static_cast<entt::entity>(entity));
    }

    /**
     * Check if entity has a component.
     * @tparam T Component type
     * @param entity Target entity
     * @return true if entity has component
     */
    template<typename T>
    bool has(EntityID entity) const {
        return m_registry.all_of<T>(static_cast<entt::entity>(entity));
    }

    /**
     * Remove a component from an entity.
     * @tparam T Component type
     * @param entity Target entity
     */
    template<typename T>
    void remove(EntityID entity) {
        m_registry.remove<T>(static_cast<entt::entity>(entity));
    }

    /**
     * Get a view of entities with specific components.
     * @tparam Ts Component types
     * @return View for iteration
     */
    template<typename... Ts>
    auto view() {
        return m_registry.view<Ts...>();
    }

    /**
     * Get a view of entities with specific components (const).
     * @tparam Ts Component types
     * @return Const view for iteration
     */
    template<typename... Ts>
    auto view() const {
        return m_registry.view<Ts...>();
    }

    /**
     * Clear all entities and components.
     */
    void clear();

    /**
     * Get number of entities.
     * @return Entity count
     */
    size_t size() const;

    /**
     * Get direct access to underlying EnTT registry.
     * Use for advanced operations not covered by this wrapper.
     */
    entt::registry& raw();
    const entt::registry& raw() const;

private:
    entt::registry m_registry;
};

} // namespace sims3000

#endif // SIMS3000_ECS_REGISTRY_H
