/**
 * @file BuildingComponents.h
 * @brief Building component structures for Epic 4
 *
 * Defines:
 * - DebrisComponent: Deconstructed building debris (4-005)
 * - BuildingComponent: Core building data (4-003)
 * - ConstructionComponent: Transient construction progress data (4-004)
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#ifndef SIMS3000_BUILDING_BUILDINGCOMPONENTS_H
#define SIMS3000_BUILDING_BUILDINGCOMPONENTS_H

#include <sims3000/building/BuildingTypes.h>
#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace building {

/**
 * @struct DebrisComponent
 * @brief Debris from deconstructed buildings.
 *
 * Debris blocks new construction and auto-clears after a timer.
 * Occupies the same footprint as the original building.
 *
 * Debris entities also have PositionComponent for location and
 * may have OwnershipComponent for original owner tracking.
 *
 * Layout:
 * - original_template_id: 4 bytes (uint32_t) - building template that was demolished
 * - clear_timer: 2 bytes (uint16_t) - ticks until auto-clear (default: 60 ticks / 3 seconds)
 * - footprint_w: 1 byte (uint8_t) - width in tiles
 * - footprint_h: 1 byte (uint8_t) - height in tiles
 *
 * Total: 8 bytes
 */
struct DebrisComponent {
    std::uint32_t original_template_id;  ///< Template ID of demolished building
    std::uint16_t clear_timer;           ///< Ticks until auto-clear
    std::uint8_t footprint_w;            ///< Footprint width in tiles
    std::uint8_t footprint_h;            ///< Footprint height in tiles

    /// Default clear timer: 60 ticks (3 seconds at 20 ticks/second)
    static constexpr std::uint16_t DEFAULT_CLEAR_TIMER = 60;

    /**
     * @brief Default constructor.
     *
     * Initializes debris with default clear timer and 1x1 footprint.
     */
    DebrisComponent()
        : original_template_id(0)
        , clear_timer(DEFAULT_CLEAR_TIMER)
        , footprint_w(1)
        , footprint_h(1)
    {}

    /**
     * @brief Construct debris with specific parameters.
     *
     * @param template_id Original building template ID.
     * @param width Footprint width in tiles.
     * @param height Footprint height in tiles.
     * @param timer Optional custom clear timer (default: DEFAULT_CLEAR_TIMER).
     */
    DebrisComponent(std::uint32_t template_id, std::uint8_t width, std::uint8_t height,
                    std::uint16_t timer = DEFAULT_CLEAR_TIMER)
        : original_template_id(template_id)
        , clear_timer(timer)
        , footprint_w(width)
        , footprint_h(height)
    {}

    /**
     * @brief Check if debris timer has expired.
     * @return true if clear_timer is zero.
     */
    bool isExpired() const {
        return clear_timer == 0;
    }

    /**
     * @brief Decrement the clear timer by one tick.
     *
     * Does not decrement below zero.
     */
    void tick() {
        if (clear_timer > 0) {
            --clear_timer;
        }
    }
};

// Verify DebrisComponent size
static_assert(sizeof(DebrisComponent) == 8,
    "DebrisComponent should be 8 bytes");

// Verify DebrisComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<DebrisComponent>::value,
    "DebrisComponent must be trivially copyable");

/**
 * @struct BuildingComponent
 * @brief Core building data at 28 bytes packed (canonical per CCR-003).
 *
 * This component stores all per-building information for zone-grown structures.
 * Building entities also have PositionComponent, OwnershipComponent, and
 * optionally ConstructionComponent (during Materializing state).
 *
 * Per CCR-010: NO scale variation stored - rotation and color accent only.
 *
 * Layout (28 bytes packed or 32 bytes aligned):
 * All fields ordered from largest to smallest for optimal packing.
 */
struct BuildingComponent {
    std::uint32_t template_id;          ///< Template ID for lookup (0 = invalid)
    std::uint32_t state_changed_tick;   ///< Tick when state last changed
    std::uint16_t capacity;             ///< Maximum occupancy
    std::uint16_t current_occupancy;    ///< Current occupants
    std::uint16_t abandon_timer;        ///< Ticks remaining in abandon grace period
    std::uint8_t zone_type;             ///< ZoneBuildingType value (0-2)
    std::uint8_t density;               ///< DensityLevel value (0-1)
    std::uint8_t state;                 ///< BuildingState value (0-4)
    std::uint8_t level;                 ///< Building level (0-255)
    std::uint8_t health;                ///< Health percentage (0-255, 255 = full)
    std::uint8_t footprint_w;           ///< Footprint width in tiles
    std::uint8_t footprint_h;           ///< Footprint height in tiles
    std::uint8_t rotation;              ///< Rotation (0-3 for 0/90/180/270 degrees)
    std::uint8_t color_accent_index;    ///< Index into template's accent palette
    std::uint8_t _padding;              ///< Explicit padding

    /**
     * @brief Default constructor.
     *
     * Initializes building with default values (invalid template, Active state).
     */
    BuildingComponent()
        : template_id(0)
        , state_changed_tick(0)
        , capacity(0)
        , current_occupancy(0)
        , abandon_timer(0)
        , zone_type(static_cast<std::uint8_t>(ZoneBuildingType::Habitation))
        , density(static_cast<std::uint8_t>(DensityLevel::Low))
        , state(static_cast<std::uint8_t>(BuildingState::Active))
        , level(0)
        , health(255)
        , footprint_w(1)
        , footprint_h(1)
        , rotation(0)
        , color_accent_index(0)
        , _padding(0)
    {}

    /**
     * @brief Get the zone type as the enum value.
     * @return ZoneBuildingType enum value.
     */
    ZoneBuildingType getZoneBuildingType() const {
        return static_cast<ZoneBuildingType>(zone_type);
    }

    /**
     * @brief Set the zone type.
     * @param type ZoneBuildingType to set.
     */
    void setZoneBuildingType(ZoneBuildingType type) {
        zone_type = static_cast<std::uint8_t>(type);
    }

    /**
     * @brief Get the density as the enum value.
     * @return DensityLevel enum value.
     */
    DensityLevel getDensityLevel() const {
        return static_cast<DensityLevel>(density);
    }

    /**
     * @brief Set the density.
     * @param d DensityLevel to set.
     */
    void setDensityLevel(DensityLevel d) {
        density = static_cast<std::uint8_t>(d);
    }

    /**
     * @brief Get the state as the enum value.
     * @return BuildingState enum value.
     */
    BuildingState getBuildingState() const {
        return static_cast<BuildingState>(state);
    }

    /**
     * @brief Set the state.
     * @param s BuildingState to set.
     */
    void setBuildingState(BuildingState s) {
        state = static_cast<std::uint8_t>(s);
    }

    /**
     * @brief Check if the building is in a specific state.
     * @param s BuildingState to check.
     * @return true if the building is in that state.
     */
    bool isInState(BuildingState s) const {
        return getBuildingState() == s;
    }

    /**
     * @brief Get health as a percentage (0-100).
     * @return Health percentage.
     */
    std::uint8_t getHealthPercent() const {
        return static_cast<std::uint8_t>((static_cast<std::uint16_t>(health) * 100) / 255);
    }

    /**
     * @brief Set health from a percentage (0-100).
     * @param percent Health percentage to set.
     */
    void setHealthPercent(std::uint8_t percent) {
        if (percent > 100) percent = 100;
        health = static_cast<std::uint8_t>((static_cast<std::uint16_t>(percent) * 255) / 100);
    }

    /**
     * @brief Get rotation in degrees.
     * @return Rotation in degrees (0, 90, 180, or 270).
     */
    std::uint16_t getRotationDegrees() const {
        return static_cast<std::uint16_t>((rotation % 4) * 90);
    }

    /**
     * @brief Set rotation from degrees (0, 90, 180, 270).
     * @param degrees Rotation in degrees (will be quantized to nearest 90-degree increment).
     */
    void setRotationDegrees(std::uint16_t degrees) {
        rotation = static_cast<std::uint8_t>((degrees / 90) % 4);
    }
};

// Verify BuildingComponent size (28 bytes packed or 32 bytes aligned per CCR-003)
static_assert(sizeof(BuildingComponent) <= 32,
    "BuildingComponent must be 32 bytes or less");

// Verify BuildingComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<BuildingComponent>::value,
    "BuildingComponent must be trivially copyable");

/**
 * @struct ConstructionComponent
 * @brief Transient component for buildings in Materializing state (12 bytes).
 *
 * This component is added when a building begins construction and removed
 * when construction completes. The BuildingSystem uses this to track
 * construction progress and determine the current construction phase.
 *
 * Per CCR-011, construction has 4 phases derived from progress percentage:
 * - Foundation: 0-25% progress
 * - Framework: 25-50% progress
 * - Exterior: 50-75% progress
 * - Finalization: 75-100% progress
 *
 * Layout (12 bytes):
 * Fields ordered from largest to smallest for optimal packing.
 */
struct ConstructionComponent {
    std::uint32_t construction_cost;    ///< Credits spent on construction
    std::uint16_t ticks_total;          ///< Total construction duration in ticks
    std::uint16_t ticks_elapsed;        ///< Ticks elapsed since construction started
    std::uint8_t phase;                 ///< ConstructionPhase value (0-3)
    std::uint8_t phase_progress;        ///< Progress within current phase (0-255)
    std::uint8_t is_paused;             ///< True (1) if construction is paused
    std::uint8_t _padding;              ///< Explicit padding

    /**
     * @brief Default constructor.
     *
     * Initializes construction component with default values.
     */
    ConstructionComponent()
        : construction_cost(0)
        , ticks_total(100)
        , ticks_elapsed(0)
        , phase(static_cast<std::uint8_t>(ConstructionPhase::Foundation))
        , phase_progress(0)
        , is_paused(0)
        , _padding(0)
    {}

    /**
     * @brief Construct with specific parameters.
     *
     * @param total Total construction duration in ticks.
     * @param cost Construction cost in credits.
     */
    ConstructionComponent(std::uint16_t total, std::uint32_t cost)
        : construction_cost(cost)
        , ticks_total(total)
        , ticks_elapsed(0)
        , phase(static_cast<std::uint8_t>(ConstructionPhase::Foundation))
        , phase_progress(0)
        , is_paused(0)
        , _padding(0)
    {}

    /**
     * @brief Get the construction phase as the enum value.
     * @return ConstructionPhase enum value.
     */
    ConstructionPhase getPhase() const {
        return static_cast<ConstructionPhase>(phase);
    }

    /**
     * @brief Calculate progress percentage (0-100).
     * @return Progress percentage.
     */
    std::uint8_t getProgressPercent() const {
        return building::getProgressPercent(ticks_elapsed, ticks_total);
    }

    /**
     * @brief Update phase based on current progress percentage.
     *
     * This should be called after updating ticks_elapsed to ensure
     * phase and phase_progress are in sync.
     */
    void updatePhase() {
        std::uint8_t percent = getProgressPercent();
        phase = static_cast<std::uint8_t>(building::getPhaseFromProgress(percent));

        // Calculate phase_progress (0-255 within current 25% phase)
        std::uint8_t phase_base = static_cast<std::uint8_t>(phase) * 25;
        std::uint8_t phase_offset = percent - phase_base;
        phase_progress = static_cast<std::uint8_t>((static_cast<std::uint16_t>(phase_offset) * 255) / 25);
    }

    /**
     * @brief Check if construction is paused.
     * @return true if paused.
     */
    bool isPaused() const {
        return is_paused != 0;
    }

    /**
     * @brief Set pause state.
     * @param paused True to pause, false to unpause.
     */
    void setPaused(bool paused) {
        is_paused = paused ? 1 : 0;
    }

    /**
     * @brief Advance construction by one tick.
     *
     * Increments ticks_elapsed and updates phase accordingly.
     * Does nothing if paused or already complete.
     *
     * @return true if construction advanced, false if paused or complete.
     */
    bool tick() {
        if (isPaused() || ticks_elapsed >= ticks_total) {
            return false;
        }
        ++ticks_elapsed;
        updatePhase();
        return true;
    }

    /**
     * @brief Check if construction is complete.
     * @return true if ticks_elapsed >= ticks_total.
     */
    bool isComplete() const {
        return ticks_elapsed >= ticks_total;
    }
};

// Verify ConstructionComponent size (12 bytes)
static_assert(sizeof(ConstructionComponent) <= 12,
    "ConstructionComponent must be 12 bytes or less");

// Verify ConstructionComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<ConstructionComponent>::value,
    "ConstructionComponent must be trivially copyable");

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGCOMPONENTS_H
