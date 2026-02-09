/**
 * @file ServicesSystem.h
 * @brief Main services system orchestrator for Epic 9 (Tickets E9-003, E9-011)
 *
 * ServicesSystem manages all city service buildings (Enforcer, HazardResponse,
 * Medical, Education) and their coverage grids. It orchestrates per-tick
 * updates of service effectiveness and coverage calculations.
 *
 * E9-011: Per-type-per-player dirty flags and lazy-allocated coverage grids.
 * Only recalculates coverage for grids marked dirty.
 *
 * Implements ISimulatable at priority 55.
 * Runs after PopulationSystem (50), before EconomySystem (60).
 *
 * Uses canonical terminology per /docs/canon/terminology.yaml.
 */

#pragma once

#include <sims3000/core/ISimulatable.h>
#include <sims3000/services/ServiceTypes.h>
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace sims3000 {
namespace services {

// Forward declaration - will be implemented in a later ticket
class ServiceCoverageGrid;

/**
 * @class ServicesSystem
 * @brief Main orchestrator for city services and coverage grids.
 *
 * Implements ISimulatable at priority 55.
 * Manages per-player service building tracking and coverage grids.
 */
class ServicesSystem : public sims3000::ISimulatable {
public:
    static constexpr int TICK_PRIORITY = 55;
    static constexpr uint8_t MAX_PLAYERS = 4;

    ServicesSystem();
    ~ServicesSystem() override;

    // =========================================================================
    // ISimulatable interface
    // =========================================================================

    /**
     * @brief Called every simulation tick (20 Hz).
     *
     * Currently a stub - will be populated in later tickets to:
     * 1. Update service building states
     * 2. Recalculate coverage grids
     * 3. Apply service effects to population
     *
     * @param time Read-only access to simulation timing.
     */
    void tick(const ISimulationTime& time) override;

    /**
     * @brief Get execution priority (lower = earlier).
     * @return 55 - runs after PopulationSystem (50), before EconomySystem (60).
     */
    int getPriority() const override { return TICK_PRIORITY; }

    /**
     * @brief Get the system name for debugging.
     * @return "ServicesSystem"
     */
    const char* getName() const override { return "ServicesSystem"; }

    // =========================================================================
    // Lifecycle
    // =========================================================================

    /**
     * @brief Initialize the system with map dimensions.
     *
     * Sets up per-player data structures and prepares coverage grids
     * for the given map size.
     *
     * @param map_width Map width in tiles.
     * @param map_height Map height in tiles.
     */
    void init(uint32_t map_width, uint32_t map_height);

    /**
     * @brief Clean up all system state.
     *
     * Releases coverage grids and clears tracked entities.
     */
    void cleanup();

    // =========================================================================
    // Building Event Handlers (E9-012)
    // =========================================================================

    /**
     * @brief Handle a building being constructed.
     *
     * Adds the service building entity to per-player tracking vectors
     * and marks coverage as dirty for recalculation.
     *
     * Will later subscribe to BuildingConstructedEvent.
     *
     * @param entity_id The constructed building entity ID.
     * @param owner_id The owning player ID (0 to MAX_PLAYERS-1).
     */
    void on_building_constructed(uint32_t entity_id, uint8_t owner_id);

    /**
     * @brief Handle a building being deconstructed/demolished.
     *
     * Removes the service building entity from per-player tracking vectors
     * and marks coverage as dirty for recalculation.
     *
     * Will later subscribe to BuildingDeconstructedEvent.
     *
     * @param entity_id The deconstructed building entity ID.
     * @param owner_id The owning player ID (0 to MAX_PLAYERS-1).
     */
    void on_building_deconstructed(uint32_t entity_id, uint8_t owner_id);

    /**
     * @brief Handle a building's power state changing.
     *
     * Marks coverage as dirty so effectiveness is recalculated
     * on the next tick.
     *
     * Will later subscribe to power change events.
     *
     * @param entity_id The affected building entity ID.
     * @param owner_id The owning player ID (0 to MAX_PLAYERS-1).
     */
    void on_building_power_changed(uint32_t entity_id, uint8_t owner_id);

    // =========================================================================
    // Dirty Flag Management (E9-011)
    // =========================================================================

    /**
     * @brief Mark a specific service type's coverage as dirty for a player.
     *
     * The next tick() call will recalculate coverage for this type+player.
     *
     * @param type The service type to mark dirty.
     * @param player_id The player whose coverage needs recalculation.
     */
    void mark_dirty(ServiceType type, uint8_t player_id);

    /**
     * @brief Mark all service types as dirty for a player.
     *
     * Used when a change affects all services (e.g., funding change).
     *
     * @param player_id The player whose coverage needs recalculation.
     */
    void mark_all_dirty(uint8_t player_id);

    /**
     * @brief Check if a specific service type's coverage is dirty for a player.
     *
     * @param type The service type to check.
     * @param player_id The player to check.
     * @return true if coverage needs recalculation.
     */
    bool is_dirty(ServiceType type, uint8_t player_id) const;

    /**
     * @brief Recalculate coverage for all dirty grids.
     *
     * Iterates all type+player combinations and recalculates any
     * that are marked dirty. Marks them clean after recalculation.
     *
     * Called automatically from tick().
     */
    void recalculate_if_dirty();

    // =========================================================================
    // Queries
    // =========================================================================

    /**
     * @brief Check if the system has been initialized.
     * @return true if init() has been called and cleanup() has not.
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Get map width in tiles.
     * @return Map width, or 0 if not initialized.
     */
    uint32_t getMapWidth() const { return m_map_width; }

    /**
     * @brief Get map height in tiles.
     * @return Map height, or 0 if not initialized.
     */
    uint32_t getMapHeight() const { return m_map_height; }

    /**
     * @brief Check if any coverage needs recalculation (legacy API).
     * @return true if any coverage grid is dirty.
     */
    bool isCoverageDirty() const;

    /**
     * @brief Get the coverage grid for a specific service type and player.
     *
     * Returns nullptr if the grid has not been allocated yet (lazy allocation
     * happens on first recalculation).
     *
     * @param type The service type.
     * @param player_id The player ID.
     * @return Pointer to the coverage grid, or nullptr.
     */
    ServiceCoverageGrid* get_coverage_grid(ServiceType type, uint8_t player_id) const;

private:
    uint32_t m_map_width = 0;
    uint32_t m_map_height = 0;
    bool m_initialized = false;

    /// Per-player tracked service building entity IDs
    /// Index 0 = player 0, up to MAX_PLAYERS-1
    std::array<std::vector<uint32_t>, MAX_PLAYERS> m_service_entities;

    /// Per-player, per-type coverage grids (lazy allocated on first recalculation)
    /// Indexed as [SERVICE_TYPE][PLAYER_ID]
    std::unique_ptr<ServiceCoverageGrid> m_coverage_grids[SERVICE_TYPE_COUNT][MAX_PLAYERS];

    /// Per-player, per-type dirty flags
    /// Indexed as [SERVICE_TYPE][PLAYER_ID]
    bool m_dirty[SERVICE_TYPE_COUNT][MAX_PLAYERS] = {};
};

} // namespace services
} // namespace sims3000
