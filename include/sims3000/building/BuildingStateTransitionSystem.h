/**
 * @file BuildingStateTransitionSystem.h
 * @brief Building state transition system for lifecycle management (Ticket 4-028)
 *
 * Manages transitions between building states based on service availability:
 * - Active -> Abandoned: services lost beyond grace period
 * - Abandoned -> Active: services restored (BuildingRestoredEvent)
 * - Abandoned -> Derelict: abandon timer expired
 * - Derelict -> Deconstructed: derelict timer expired
 *
 * @see /docs/epics/epic-4/tickets.md (ticket 4-028)
 */

#ifndef SIMS3000_BUILDING_BUILDINGSTATETRANSITIONSYSTEM_H
#define SIMS3000_BUILDING_BUILDINGSTATETRANSITIONSYSTEM_H

#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingGrid.h>
#include <sims3000/building/BuildingEvents.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace sims3000 {
namespace building {

/**
 * @struct StateTransitionConfig
 * @brief Configuration parameters for state transition timers.
 *
 * Per-service grace periods (Ticket 4-029):
 * - energy_grace_period: Ticks without energy before abandon
 * - fluid_grace_period: Ticks without fluid before abandon
 * - transport_grace_period: Ticks without transport before abandon (0 = immediate)
 *
 * A grace period value of 0 means IMMEDIATE abandon (no grace for that service).
 * UINT32_MAX is a sentinel meaning "use service_grace_period" (backward compat).
 *
 * When per-service periods are UINT32_MAX, the legacy service_grace_period
 * value is used for that service. Explicitly setting a per-service period
 * overrides the legacy value.
 */
struct StateTransitionConfig {
    uint32_t service_grace_period = 100;    ///< Legacy: Ticks before Active->Abandoned (5 sec)
    uint32_t abandon_timer_ticks = 200;     ///< Ticks Abandoned->Derelict (10 sec)
    uint32_t derelict_timer_ticks = 500;    ///< Ticks Derelict->Deconstructed (25 sec)

    /// Sentinel for "use service_grace_period"
    static constexpr uint32_t USE_LEGACY = UINT32_MAX;

    // Per-service grace periods (Ticket 4-029)
    // UINT32_MAX = use service_grace_period (backward compatible default)
    uint32_t energy_grace_period = USE_LEGACY;    ///< Ticks without energy before abandon
    uint32_t fluid_grace_period = USE_LEGACY;     ///< Ticks without fluid before abandon
    uint32_t transport_grace_period = USE_LEGACY; ///< Ticks without transport before abandon

    /// Get effective energy grace period (resolves USE_LEGACY sentinel)
    uint32_t get_energy_grace() const {
        return energy_grace_period == USE_LEGACY ? service_grace_period : energy_grace_period;
    }

    /// Get effective fluid grace period (resolves USE_LEGACY sentinel)
    uint32_t get_fluid_grace() const {
        return fluid_grace_period == USE_LEGACY ? service_grace_period : fluid_grace_period;
    }

    /// Get effective transport grace period (resolves USE_LEGACY sentinel)
    uint32_t get_transport_grace() const {
        return transport_grace_period == USE_LEGACY ? service_grace_period : transport_grace_period;
    }
};

/**
 * @struct ServiceGraceState
 * @brief Per-entity grace period tracking for service loss.
 *
 * Tracks how many consecutive ticks each service has been unavailable.
 * When any counter exceeds the grace period, the building transitions
 * to Abandoned state.
 */
struct ServiceGraceState {
    uint32_t ticks_without_energy = 0;
    uint32_t ticks_without_fluid = 0;
    uint32_t ticks_without_transport = 0;
};

/**
 * @class BuildingStateTransitionSystem
 * @brief Manages building lifecycle state transitions based on service availability.
 *
 * Each tick, evaluates all building entities:
 * - Active buildings: check service availability, track grace period
 * - Abandoned buildings: check if services restored, track abandon timer
 * - Derelict buildings: track derelict timer until deconstructed
 *
 * Emits events for each state transition for UI/audio/stats systems.
 */
class BuildingStateTransitionSystem {
public:
    /**
     * @brief Construct BuildingStateTransitionSystem with dependency injection.
     *
     * @param factory Pointer to BuildingFactory for entity access.
     * @param grid Pointer to BuildingGrid for footprint operations.
     * @param energy Pointer to energy provider for power queries.
     * @param fluid Pointer to fluid provider for fluid queries.
     * @param transport Pointer to transport provider for road access queries.
     */
    BuildingStateTransitionSystem(
        BuildingFactory* factory,
        BuildingGrid* grid,
        const IEnergyProvider* energy,
        const IFluidProvider* fluid,
        const ITransportProvider* transport
    );

    /**
     * @brief Process state transitions for all building entities.
     *
     * Called each simulation tick. Evaluates Active, Abandoned, and Derelict
     * buildings, performing state transitions as needed.
     *
     * @param current_tick Current simulation tick.
     */
    void tick(uint32_t current_tick);

    /**
     * @brief Set state transition configuration.
     * @param config The StateTransitionConfig to apply.
     */
    void set_config(const StateTransitionConfig& config);

    /**
     * @brief Get current state transition configuration.
     * @return Const reference to current StateTransitionConfig.
     */
    const StateTransitionConfig& get_config() const;

    // =========================================================================
    // Pending Events
    // =========================================================================

    /** @brief Get pending abandoned events. */
    const std::vector<BuildingAbandonedEvent>& get_pending_abandoned_events() const;

    /** @brief Get pending restored events. */
    const std::vector<BuildingRestoredEvent>& get_pending_restored_events() const;

    /** @brief Get pending derelict events. */
    const std::vector<BuildingDerelictEvent>& get_pending_derelict_events() const;

    /** @brief Get pending deconstructed events. */
    const std::vector<BuildingDeconstructedEvent>& get_pending_deconstructed_events() const;

    /** @brief Clear all pending events from all event queues. */
    void clear_all_pending_events();

private:
    BuildingFactory* m_factory;
    BuildingGrid* m_grid;
    const IEnergyProvider* m_energy;
    const IFluidProvider* m_fluid;
    const ITransportProvider* m_transport;
    StateTransitionConfig m_config;

    /// Per-entity grace state (indexed by entity_id)
    std::unordered_map<uint32_t, ServiceGraceState> m_grace_states;

    /// Pending events
    std::vector<BuildingAbandonedEvent> m_pending_abandoned;
    std::vector<BuildingRestoredEvent> m_pending_restored;
    std::vector<BuildingDerelictEvent> m_pending_derelict;
    std::vector<BuildingDeconstructedEvent> m_pending_deconstructed;

    /**
     * @brief Evaluate an Active building for service loss.
     *
     * Checks energy, fluid, and transport availability.
     * If any service is lost, increments grace counter.
     * If all services are available, resets grace counter.
     * If any grace counter exceeds service_grace_period, transitions to Abandoned.
     *
     * @param entity The building entity to evaluate.
     * @param current_tick Current simulation tick.
     */
    void evaluate_active(BuildingEntity& entity, uint32_t current_tick);

    /**
     * @brief Evaluate an Abandoned building for restoration or further decay.
     *
     * If all services are restored, transitions back to Active.
     * Otherwise, tracks abandon timer. If timer expires, transitions to Derelict.
     *
     * @param entity The building entity to evaluate.
     * @param current_tick Current simulation tick.
     */
    void evaluate_abandoned(BuildingEntity& entity, uint32_t current_tick);

    /**
     * @brief Evaluate a Derelict building for deconstructon.
     *
     * Tracks time in derelict state using state_changed_tick.
     * If derelict_timer_ticks exceeded, transitions to Deconstructed,
     * clears grid footprint, adds debris data, and emits event.
     *
     * @param entity The building entity to evaluate.
     * @param current_tick Current simulation tick.
     */
    void evaluate_derelict(BuildingEntity& entity, uint32_t current_tick);

    /**
     * @brief Check if all services are available for a building at its position.
     *
     * @param entity The building entity to check.
     * @return true if all services (energy, fluid, transport) are available.
     */
    bool are_all_services_available(const BuildingEntity& entity) const;
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGSTATETRANSITIONSYSTEM_H
