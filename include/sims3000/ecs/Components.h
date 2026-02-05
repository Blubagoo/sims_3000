/**
 * @file Components.h
 * @brief Core ECS component definitions.
 *
 * All components are trivially copyable pure data structs
 * suitable for serialization and network transfer.
 */

#ifndef SIMS3000_ECS_COMPONENTS_H
#define SIMS3000_ECS_COMPONENTS_H

#include "sims3000/core/types.h"
#include "sims3000/core/Serialization.h"
#include "sims3000/assets/ModelLoader.h"
#include "sims3000/assets/TextureLoader.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace sims3000 {

/// Special player ID for game-controlled entities (neutral/system-owned).
constexpr PlayerID GAME_MASTER = 0;

/**
 * @enum OwnershipState
 * @brief States describing entity ownership status.
 */
enum class OwnershipState : std::uint8_t {
    Owned,      ///< Normal ownership by a player
    Abandoned,  ///< Was owned, now unclaimed
    Neutral,    ///< Never been owned (game entities)
    Contested   ///< Ownership being transferred
};

/**
 * @struct PositionComponent
 * @brief Tile-based position on the game grid.
 *
 * Includes elevation for terrain height and multi-story buildings.
 * Elevation units are in discrete levels (e.g., floors, terrain steps).
 */
struct PositionComponent {
    GridPosition pos;
    std::int16_t elevation = 0;  ///< Vertical level (terrain height/building floor)

    /**
     * @brief Serialize component to binary buffer.
     * @param buffer Output buffer to append serialized data.
     */
    void serialize(std::vector<std::uint8_t>& buffer) const;

    /**
     * @brief Deserialize component from binary data.
     * @param data Pointer to serialized data.
     * @param offset Current read offset (updated after read).
     * @return Deserialized PositionComponent.
     */
    static PositionComponent deserialize(const std::uint8_t* data, std::size_t& offset);
};
static_assert(sizeof(PositionComponent) == 6, "PositionComponent size check");

/**
 * @struct OwnershipComponent
 * @brief Entity ownership for multiplayer.
 *
 * Tracks who owns an entity, the ownership state, and when the state last changed.
 * Uses GAME_MASTER (0) as the default owner for neutral/game-controlled entities.
 */
struct OwnershipComponent {
    PlayerID owner = GAME_MASTER;                    ///< Owner player ID (0 = game-controlled)
    OwnershipState state = OwnershipState::Neutral;  ///< Current ownership state
    std::uint8_t padding[6] = {};                    ///< Padding for alignment
    SimulationTick state_changed_at = 0;             ///< Tick when ownership state last changed

    /**
     * @brief Serialize component to binary buffer.
     * @param buffer Output buffer to append serialized data.
     */
    void serialize(std::vector<std::uint8_t>& buffer) const;

    /**
     * @brief Deserialize component from binary data.
     * @param data Pointer to serialized data.
     * @param offset Current read offset (updated after read).
     * @return Deserialized OwnershipComponent.
     */
    static OwnershipComponent deserialize(const std::uint8_t* data, std::size_t& offset);
};
static_assert(sizeof(OwnershipComponent) == 16, "OwnershipComponent size check");

/**
 * @struct TransformComponent
 * @brief 3D world position for rendering.
 * Derived from grid position but includes height and rotation.
 */
struct TransformComponent {
    glm::vec3 position{0.0f};
    float rotation = 0.0f;  // Y-axis rotation in radians
};
static_assert(sizeof(TransformComponent) == 16, "TransformComponent size check");

/**
 * @struct RenderComponent
 * @brief Rendering information for an entity.
 * Uses handles to cached assets.
 */
struct RenderComponent {
    ModelHandle model = nullptr;
    TextureHandle texture = nullptr;
    glm::vec4 tintColor{1.0f, 1.0f, 1.0f, 1.0f};
    bool visible = true;
    std::uint8_t padding[3] = {};
};

/**
 * @struct BuildingComponent
 * @brief Data for building entities.
 */
struct BuildingComponent {
    std::uint32_t buildingType = 0;
    std::uint8_t level = 1;
    std::uint8_t health = 100;
    std::uint8_t flags = 0;
    std::uint8_t padding = 0;
};
static_assert(sizeof(BuildingComponent) == 8, "BuildingComponent size check");

/**
 * @struct EnergyComponent
 * @brief Energy consumption/production.
 */
struct EnergyComponent {
    std::int32_t consumption = 0;  // Negative = produces
    std::int32_t capacity = 0;
    std::uint8_t connected = 0;
    std::uint8_t padding[3] = {};
};
static_assert(sizeof(EnergyComponent) == 12, "EnergyComponent size check");

/**
 * @struct PopulationComponent
 * @brief Population for residential buildings.
 */
struct PopulationComponent {
    std::uint16_t current = 0;
    std::uint16_t capacity = 0;
    std::uint8_t happiness = 50;
    std::uint8_t employmentRate = 0;
    std::uint8_t padding[2] = {};
};
static_assert(sizeof(PopulationComponent) == 8, "PopulationComponent size check");

/**
 * @struct ZoneComponent
 * @brief Zone type assignment.
 */
struct ZoneComponent {
    std::uint8_t zoneType = 0;     // 0=none, 1=residential, 2=commercial, 3=industrial
    std::uint8_t density = 1;      // 1=low, 2=medium, 3=high
    std::uint8_t desirability = 50;
    std::uint8_t padding = 0;
};
static_assert(sizeof(ZoneComponent) == 4, "ZoneComponent size check");

/**
 * @struct TransportComponent
 * @brief Transport network participation.
 */
struct TransportComponent {
    std::uint32_t roadConnectionId = 0;
    std::uint16_t trafficLoad = 0;
    std::uint8_t accessibility = 50;
    std::uint8_t padding = 0;
};
static_assert(sizeof(TransportComponent) == 8, "TransportComponent size check");

/**
 * @struct ServiceCoverageComponent
 * @brief Service coverage levels (0-100).
 */
struct ServiceCoverageComponent {
    std::uint8_t police = 0;
    std::uint8_t fire = 0;
    std::uint8_t health = 0;
    std::uint8_t education = 0;
    std::uint8_t parks = 0;
    std::uint8_t padding[3] = {};
};
static_assert(sizeof(ServiceCoverageComponent) == 8, "ServiceCoverageComponent size check");

/**
 * @struct TaxableComponent
 * @brief Economic/taxation data.
 */
struct TaxableComponent {
    std::int32_t income = 0;
    std::int32_t taxPaid = 0;
    std::uint8_t taxBracket = 10;
    std::uint8_t padding[3] = {};
};
static_assert(sizeof(TaxableComponent) == 12, "TaxableComponent size check");

// =============================================================================
// ComponentMeta Specializations
// =============================================================================
// Define sync policies and metadata for each component type.
// See Serialization.h for the ComponentMeta template and helpers.

/**
 * @brief PositionComponent metadata.
 * Frequently updated - use Unreliable for best-effort sync.
 * Interpolated for smooth visual rendering.
 */
template<>
struct ComponentMeta<PositionComponent> {
    static constexpr SyncPolicy syncPolicy = SyncPolicy::Unreliable;
    static constexpr bool interpolated = true;
    static constexpr const char* name = "PositionComponent";
};

/**
 * @brief OwnershipComponent metadata.
 * Critical state - use Reliable for guaranteed delivery.
 * Not interpolated (discrete state).
 */
template<>
struct ComponentMeta<OwnershipComponent> {
    static constexpr SyncPolicy syncPolicy = SyncPolicy::Reliable;
    static constexpr bool interpolated = false;
    static constexpr const char* name = "OwnershipComponent";
};

/**
 * @brief TransformComponent metadata.
 * Derived from PositionComponent, frequently updated for rendering.
 * Interpolated for smooth visual transitions.
 */
template<>
struct ComponentMeta<TransformComponent> {
    static constexpr SyncPolicy syncPolicy = SyncPolicy::Unreliable;
    static constexpr bool interpolated = true;
    static constexpr const char* name = "TransformComponent";
};

/**
 * @brief RenderComponent metadata.
 * Client-only visual data, never synced.
 */
template<>
struct ComponentMeta<RenderComponent> {
    static constexpr SyncPolicy syncPolicy = SyncPolicy::None;
    static constexpr bool interpolated = false;
    static constexpr const char* name = "RenderComponent";
};

/**
 * @brief BuildingComponent metadata.
 * Changes infrequently - sync when building state changes.
 */
template<>
struct ComponentMeta<BuildingComponent> {
    static constexpr SyncPolicy syncPolicy = SyncPolicy::OnChange;
    static constexpr bool interpolated = false;
    static constexpr const char* name = "BuildingComponent";
};

/**
 * @brief EnergyComponent metadata.
 * Network state that changes periodically (simulation ticks).
 */
template<>
struct ComponentMeta<EnergyComponent> {
    static constexpr SyncPolicy syncPolicy = SyncPolicy::Periodic;
    static constexpr bool interpolated = false;
    static constexpr const char* name = "EnergyComponent";
};

/**
 * @brief PopulationComponent metadata.
 * Simulation state that changes periodically.
 */
template<>
struct ComponentMeta<PopulationComponent> {
    static constexpr SyncPolicy syncPolicy = SyncPolicy::Periodic;
    static constexpr bool interpolated = false;
    static constexpr const char* name = "PopulationComponent";
};

/**
 * @brief ZoneComponent metadata.
 * Critical state - use Reliable for guaranteed delivery.
 * Zone assignments must be consistent across all clients.
 */
template<>
struct ComponentMeta<ZoneComponent> {
    static constexpr SyncPolicy syncPolicy = SyncPolicy::Reliable;
    static constexpr bool interpolated = false;
    static constexpr const char* name = "ZoneComponent";
};

/**
 * @brief TransportComponent metadata.
 * Traffic data updates frequently but isn't critical.
 */
template<>
struct ComponentMeta<TransportComponent> {
    static constexpr SyncPolicy syncPolicy = SyncPolicy::Unreliable;
    static constexpr bool interpolated = false;
    static constexpr const char* name = "TransportComponent";
};

/**
 * @brief ServiceCoverageComponent metadata.
 * Simulation state that changes periodically.
 */
template<>
struct ComponentMeta<ServiceCoverageComponent> {
    static constexpr SyncPolicy syncPolicy = SyncPolicy::Periodic;
    static constexpr bool interpolated = false;
    static constexpr const char* name = "ServiceCoverageComponent";
};

/**
 * @brief TaxableComponent metadata.
 * Economic data that changes periodically (budget cycles).
 */
template<>
struct ComponentMeta<TaxableComponent> {
    static constexpr SyncPolicy syncPolicy = SyncPolicy::Periodic;
    static constexpr bool interpolated = false;
    static constexpr const char* name = "TaxableComponent";
};

} // namespace sims3000

#endif // SIMS3000_ECS_COMPONENTS_H
