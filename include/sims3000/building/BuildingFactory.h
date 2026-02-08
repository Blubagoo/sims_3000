/**
 * @file BuildingFactory.h
 * @brief Building entity creation and management (Epic 4, ticket 4-025)
 *
 * Defines BuildingEntity data holder and BuildingFactory class for
 * creating, querying, and removing building entities. Entities are
 * stored in a simple vector (no real ECS) as an Epic 4 simplification.
 *
 * @see /docs/epics/epic-4/tickets.md (ticket 4-025)
 */

#ifndef SIMS3000_BUILDING_BUILDINGFACTORY_H
#define SIMS3000_BUILDING_BUILDINGFACTORY_H

#include <sims3000/building/BuildingComponents.h>
#include <sims3000/building/BuildingTemplate.h>
#include <sims3000/building/TemplateSelector.h>
#include <sims3000/building/BuildingGrid.h>
#include <cstdint>
#include <vector>

// Forward declaration
namespace sims3000 {
namespace zone {
    class ZoneSystem;
} // namespace zone
} // namespace sims3000

namespace sims3000 {
namespace building {

/**
 * @struct BuildingEntity
 * @brief Simple entity data holder for building instances.
 *
 * Since we do not have a real ECS yet, this struct bundles all
 * per-building data: the core BuildingComponent, optional
 * ConstructionComponent (during Materializing), optional
 * DebrisComponent (during Deconstructed), and positional data.
 */
struct BuildingEntity {
    uint32_t entity_id;                 ///< Unique entity identifier
    BuildingComponent building;         ///< Core building data
    ConstructionComponent construction; ///< Construction progress (valid when has_construction)
    DebrisComponent debris;             ///< Debris data (valid when has_debris)
    int32_t grid_x;                     ///< Grid X coordinate (top-left of footprint)
    int32_t grid_y;                     ///< Grid Y coordinate (top-left of footprint)
    uint8_t owner_id;                   ///< Owning overseer PlayerID
    bool has_construction;              ///< True during Materializing state
    bool has_debris;                    ///< True during Deconstructed state

    BuildingEntity()
        : entity_id(0)
        , building()
        , construction()
        , debris()
        , grid_x(0)
        , grid_y(0)
        , owner_id(0)
        , has_construction(false)
        , has_debris(false)
    {}
};

/**
 * @class BuildingFactory
 * @brief Creates, stores, and manages building entities.
 *
 * Responsible for:
 * - Spawning new building entities from template selection results
 * - Registering footprints in BuildingGrid
 * - Setting zone state to Occupied via ZoneSystem
 * - Providing entity lookup and iteration
 * - Removing entities
 */
class BuildingFactory {
public:
    /**
     * @brief Construct BuildingFactory with dependency injection.
     *
     * @param grid Pointer to BuildingGrid for spatial registration.
     * @param zone_system Pointer to ZoneSystem for zone state updates.
     */
    BuildingFactory(BuildingGrid* grid, zone::ZoneSystem* zone_system);

    /**
     * @brief Create a building entity from template selection result.
     *
     * Steps:
     * 1. Generate unique entity_id
     * 2. Initialize BuildingComponent from template and selection
     * 3. Initialize ConstructionComponent from template
     * 4. Register footprint in BuildingGrid
     * 5. Set zone state to Occupied for all footprint tiles
     * 6. Store entity
     *
     * @param templ The building template to use.
     * @param selection Template selection result (rotation, color accent).
     * @param grid_x Top-left grid X coordinate.
     * @param grid_y Top-left grid Y coordinate.
     * @param owner_id Owning overseer PlayerID.
     * @param current_tick Current simulation tick.
     * @return Entity ID of the newly created building.
     */
    uint32_t spawn_building(
        const BuildingTemplate& templ,
        const TemplateSelectionResult& selection,
        int32_t grid_x, int32_t grid_y,
        uint8_t owner_id,
        uint32_t current_tick
    );

    /**
     * @brief Get entity by ID (const).
     * @param entity_id The entity ID to look up.
     * @return Pointer to entity, or nullptr if not found.
     */
    const BuildingEntity* get_entity(uint32_t entity_id) const;

    /**
     * @brief Get entity by ID (mutable).
     * @param entity_id The entity ID to look up.
     * @return Pointer to entity, or nullptr if not found.
     */
    BuildingEntity* get_entity_mut(uint32_t entity_id);

    /**
     * @brief Get all entities (const).
     * @return Const reference to entities vector.
     */
    const std::vector<BuildingEntity>& get_entities() const;

    /**
     * @brief Get all entities (mutable).
     * @return Mutable reference to entities vector.
     */
    std::vector<BuildingEntity>& get_entities_mut();

    /**
     * @brief Remove entity by ID.
     * @param entity_id The entity ID to remove.
     * @return true if entity was found and removed, false otherwise.
     */
    bool remove_entity(uint32_t entity_id);

private:
    BuildingGrid* m_grid;                   ///< Building grid for spatial registration
    zone::ZoneSystem* m_zone_system;        ///< Zone system for state updates
    std::vector<BuildingEntity> m_entities;  ///< Entity storage
    uint32_t m_next_entity_id = 1;          ///< Next entity ID to assign
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGFACTORY_H
