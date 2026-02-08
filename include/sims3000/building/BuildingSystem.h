/**
 * @file BuildingSystem.h
 * @brief Top-level BuildingSystem integrating all building subsystems (Ticket 4-034)
 *
 * BuildingSystem owns and orchestrates all building subsystems:
 * - BuildingGrid: spatial index
 * - BuildingFactory: entity creation/storage
 * - BuildingTemplateRegistry: template data
 * - BuildingSpawnChecker: spawn precondition validation
 * - BuildingSpawningLoop: per-tick spawning scan
 * - ConstructionProgressSystem: construction tick advancement
 * - BuildingStateTransitionSystem: lifecycle state machine
 * - DemolitionHandler: overseer demolition
 * - DebrisClearSystem: debris auto-clear
 *
 * Implements ISimulatable interface (not inherited to avoid diamond)
 * at priority 40 per /docs/canon/interfaces.yaml.
 *
 * @see /docs/epics/epic-4/tickets.md (ticket 4-034)
 */

#ifndef SIMS3000_BUILDING_BUILDINGSYSTEM_H
#define SIMS3000_BUILDING_BUILDINGSYSTEM_H

#include <sims3000/building/BuildingGrid.h>
#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingTemplate.h>
#include <sims3000/building/InitialTemplates.h>
#include <sims3000/building/BuildingSpawnChecker.h>
#include <sims3000/building/BuildingSpawningLoop.h>
#include <sims3000/building/ConstructionProgressSystem.h>
#include <sims3000/building/BuildingStateTransitionSystem.h>
#include <sims3000/building/DemolitionHandler.h>
#include <sims3000/building/DebrisClearSystem.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/building/IBuildingQueryable.h>
#include <cstdint>
#include <optional>
#include <vector>

// Forward declarations
namespace sims3000 {
namespace zone {
    class ZoneSystem;
} // namespace zone
namespace terrain {
    class ITerrainQueryable;
} // namespace terrain
} // namespace sims3000

namespace sims3000 {
namespace building {

/**
 * @class BuildingSystem
 * @brief Top-level system orchestrating all building subsystems.
 *
 * Implements ISimulatable interface (duck-typed, not inherited to avoid
 * diamond with zone::ISimulatable) at priority 40.
 *
 * Construction requires a ZoneSystem pointer and optional terrain pointer.
 * Forward dependency providers (energy, fluid, transport, land value,
 * demand, credit) are injected via setter methods after construction.
 */
class BuildingSystem : public IBuildingQueryable {
public:
    /**
     * @brief Construct BuildingSystem with dependency injection.
     *
     * Initializes all owned subsystems. Forward dependency providers
     * default to nullptr and can be set later via setters.
     *
     * @param zone_system Pointer to ZoneSystem for zone queries.
     * @param terrain Pointer to terrain query interface (may be nullptr).
     * @param grid_size Grid dimension (must be 128, 256, or 512). Default 256.
     */
    BuildingSystem(
        zone::ZoneSystem* zone_system,
        terrain::ITerrainQueryable* terrain,
        uint16_t grid_size = 256
    );

    // =========================================================================
    // ISimulatable interface (duck-typed)
    // =========================================================================

    /**
     * @brief Called every simulation tick.
     *
     * Tick order:
     * 1. m_spawning_loop.tick(m_tick_count)
     * 2. m_construction_system.tick(m_tick_count)
     * 3. m_state_system.tick(m_tick_count)
     * 4. m_debris_clear_system.tick()
     *
     * @param delta_time Time since last tick in seconds (unused, tick-based).
     */
    void tick(float delta_time);

    /**
     * @brief Get execution priority (lower = earlier).
     * @return 40 per canonical interface spec.
     */
    int get_priority() const;

    // =========================================================================
    // Provider setters (dependency injection)
    // =========================================================================

    /** @brief Set energy provider for power queries. */
    void set_energy_provider(IEnergyProvider* provider);

    /** @brief Set fluid provider for fluid queries. */
    void set_fluid_provider(IFluidProvider* provider);

    /** @brief Set transport provider for road access queries. */
    void set_transport_provider(ITransportProvider* provider);

    /** @brief Set land value provider for desirability queries. */
    void set_land_value_provider(ILandValueProvider* provider);

    /** @brief Set demand provider for zone growth queries. */
    void set_demand_provider(IDemandProvider* provider);

    /** @brief Set credit provider for cost deduction. */
    void set_credit_provider(ICreditProvider* provider);

    // =========================================================================
    // Subsystem access (for external callers)
    // =========================================================================

    /** @brief Get mutable reference to BuildingFactory. */
    BuildingFactory& get_factory();

    /** @brief Get const reference to BuildingFactory. */
    const BuildingFactory& get_factory() const;

    /** @brief Get mutable reference to BuildingGrid. */
    BuildingGrid& get_grid();

    /** @brief Get const reference to BuildingGrid. */
    const BuildingGrid& get_grid() const;

    /** @brief Get mutable reference to DemolitionHandler. */
    DemolitionHandler& get_demolition_handler();

    /** @brief Get mutable reference to DebrisClearSystem. */
    DebrisClearSystem& get_debris_clear_system();

    /** @brief Get mutable reference to BuildingSpawningLoop. */
    BuildingSpawningLoop& get_spawning_loop();

    /** @brief Get mutable reference to ConstructionProgressSystem. */
    ConstructionProgressSystem& get_construction_system();

    /** @brief Get mutable reference to BuildingStateTransitionSystem. */
    BuildingStateTransitionSystem& get_state_system();

    // =========================================================================
    // IBuildingQueryable interface implementation
    // =========================================================================

    /** @brief Get building entity ID at grid position (0 = no building). */
    uint32_t get_building_at(int32_t x, int32_t y) const override;

    /** @brief Check if tile is occupied by a building. */
    bool is_tile_occupied(int32_t x, int32_t y) const override;

    /** @brief Check if a rectangular footprint is available. */
    bool is_footprint_available(int32_t x, int32_t y, uint8_t w, uint8_t h) const override;

    /** @brief Get all building entity IDs within a rectangular area. */
    std::vector<uint32_t> get_buildings_in_rect(int32_t x, int32_t y, int32_t w, int32_t h) const override;

    /** @brief Get all building entity IDs owned by a specific player. */
    std::vector<uint32_t> get_buildings_by_owner(uint8_t player_id) const override;

    /**
     * @brief Get total number of building entities.
     * @return Count of all entities in the factory.
     */
    uint32_t get_building_count() const override;

    /**
     * @brief Get number of buildings in a specific state.
     * @param state The BuildingState to count.
     * @return Count of entities in the specified state.
     */
    uint32_t get_building_count_by_state(BuildingState state) const override;

    /** @brief Get building state for an entity. */
    std::optional<BuildingState> get_building_state(uint32_t entity_id) const override;

    /** @brief Get total capacity for a zone building type and player. */
    uint32_t get_total_capacity(ZoneBuildingType type, uint8_t player_id) const override;

    /** @brief Get total occupancy for a zone building type and player. */
    uint32_t get_total_occupancy(ZoneBuildingType type, uint8_t player_id) const override;

    // =========================================================================
    // Template registry access
    // =========================================================================

    /**
     * @brief Get const reference to the template registry.
     * @return Const reference to BuildingTemplateRegistry.
     */
    const BuildingTemplateRegistry& get_template_registry() const;

    /**
     * @brief Get current tick count.
     * @return Number of ticks since construction.
     */
    uint32_t get_tick_count() const;

private:
    // Owned subsystems (order matters for initialization)
    BuildingGrid m_grid;
    BuildingFactory m_factory;
    BuildingTemplateRegistry m_registry;
    BuildingSpawnChecker m_spawn_checker;
    BuildingSpawningLoop m_spawning_loop;
    ConstructionProgressSystem m_construction_system;
    BuildingStateTransitionSystem m_state_system;
    DemolitionHandler m_demolition_handler;
    DebrisClearSystem m_debris_clear_system;

    // Providers (non-owning)
    IEnergyProvider* m_energy = nullptr;
    IFluidProvider* m_fluid = nullptr;
    ITransportProvider* m_transport = nullptr;
    ILandValueProvider* m_land_value = nullptr;
    IDemandProvider* m_demand = nullptr;
    ICreditProvider* m_credits = nullptr;

    // Internal tick counter
    uint32_t m_tick_count = 0;
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGSYSTEM_H
