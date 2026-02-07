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
#include "sims3000/render/RenderLayer.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>
#include <vector>

// Forward declaration for NetworkBuffer
namespace sims3000 {
class NetworkBuffer;
}

namespace sims3000 {

// =============================================================================
// Component Type IDs for Network Serialization
// =============================================================================
// Unique IDs for each syncable component type. Used in wire format to identify
// component types during network serialization/deserialization.

namespace ComponentTypeID {
    constexpr std::uint8_t Position = 1;
    constexpr std::uint8_t Ownership = 2;
    constexpr std::uint8_t Transform = 3;
    constexpr std::uint8_t Building = 4;
    constexpr std::uint8_t Energy = 5;
    constexpr std::uint8_t Population = 6;
    constexpr std::uint8_t Zone = 7;
    constexpr std::uint8_t Transport = 8;
    constexpr std::uint8_t ServiceCoverage = 9;
    constexpr std::uint8_t Taxable = 10;
    // RenderComponent is not synced (SyncPolicy::None), no type ID needed
    // Reserve 0 for invalid/unknown
    constexpr std::uint8_t Invalid = 0;
} // namespace ComponentTypeID

// =============================================================================
// Component Version Constants
// =============================================================================
// Version byte per component type for backward compatibility.
// Increment when changing component serialization format.

namespace ComponentVersion {
    constexpr std::uint8_t Position = 1;
    constexpr std::uint8_t Ownership = 1;
    constexpr std::uint8_t Transform = 2;  // v2: quaternion rotation, scale, cached matrix
    constexpr std::uint8_t Building = 1;
    constexpr std::uint8_t Energy = 1;
    constexpr std::uint8_t Population = 1;
    constexpr std::uint8_t Zone = 1;
    constexpr std::uint8_t Transport = 1;
    constexpr std::uint8_t ServiceCoverage = 1;
    constexpr std::uint8_t Taxable = 1;
} // namespace ComponentVersion

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
 *
 * Network wire format (version 1):
 *   [1 byte version][2 bytes grid_x][2 bytes grid_y][2 bytes elevation]
 *   Total: 7 bytes
 */
struct PositionComponent {
    GridPosition pos;
    std::int16_t elevation = 0;  ///< Vertical level (terrain height/building floor)

    /**
     * @brief Serialize component to binary buffer (legacy format).
     * @param buffer Output buffer to append serialized data.
     */
    void serialize(std::vector<std::uint8_t>& buffer) const;

    /**
     * @brief Deserialize component from binary data (legacy format).
     * @param data Pointer to serialized data.
     * @param offset Current read offset (updated after read).
     * @return Deserialized PositionComponent.
     */
    static PositionComponent deserialize(const std::uint8_t* data, std::size_t& offset);

    // =========================================================================
    // Network Serialization (using NetworkBuffer)
    // =========================================================================

    /**
     * @brief Serialize component to NetworkBuffer for network transmission.
     * @param buffer NetworkBuffer to write to.
     *
     * Wire format includes version byte for backward compatibility.
     */
    void serialize_net(NetworkBuffer& buffer) const;

    /**
     * @brief Deserialize component from NetworkBuffer.
     * @param buffer NetworkBuffer to read from.
     * @return Deserialized PositionComponent.
     *
     * Handles version byte and applies default values for missing fields
     * when deserializing older versions.
     */
    static PositionComponent deserialize_net(NetworkBuffer& buffer);

    /**
     * @brief Get the serialized size in bytes for pre-allocation optimization.
     * @return Size in bytes when serialized via serialize_net().
     */
    static constexpr std::size_t get_serialized_size() {
        // version (1) + grid_x (2) + grid_y (2) + elevation (2) = 7 bytes
        return 7;
    }

    /**
     * @brief Get the component type ID for wire format.
     * @return Unique component type identifier.
     */
    static constexpr std::uint8_t get_type_id() {
        return ComponentTypeID::Position;
    }
};
static_assert(sizeof(PositionComponent) == 6, "PositionComponent size check");

/**
 * @struct OwnershipComponent
 * @brief Entity ownership for multiplayer.
 *
 * Tracks who owns an entity, the ownership state, and when the state last changed.
 * Uses GAME_MASTER (0) as the default owner for neutral/game-controlled entities.
 *
 * Network wire format (version 1):
 *   [1 byte version][1 byte owner][1 byte state][8 bytes state_changed_at]
 *   Total: 11 bytes
 */
struct OwnershipComponent {
    PlayerID owner = GAME_MASTER;                    ///< Owner player ID (0 = game-controlled)
    OwnershipState state = OwnershipState::Neutral;  ///< Current ownership state
    std::uint8_t padding[6] = {};                    ///< Padding for alignment
    SimulationTick state_changed_at = 0;             ///< Tick when ownership state last changed

    /**
     * @brief Serialize component to binary buffer (legacy format).
     * @param buffer Output buffer to append serialized data.
     */
    void serialize(std::vector<std::uint8_t>& buffer) const;

    /**
     * @brief Deserialize component from binary data (legacy format).
     * @param data Pointer to serialized data.
     * @param offset Current read offset (updated after read).
     * @return Deserialized OwnershipComponent.
     */
    static OwnershipComponent deserialize(const std::uint8_t* data, std::size_t& offset);

    // =========================================================================
    // Network Serialization (using NetworkBuffer)
    // =========================================================================

    /**
     * @brief Serialize component to NetworkBuffer for network transmission.
     * @param buffer NetworkBuffer to write to.
     *
     * Wire format includes version byte for backward compatibility.
     */
    void serialize_net(NetworkBuffer& buffer) const;

    /**
     * @brief Deserialize component from NetworkBuffer.
     * @param buffer NetworkBuffer to read from.
     * @return Deserialized OwnershipComponent.
     *
     * Handles version byte and applies default values for missing fields
     * when deserializing older versions.
     */
    static OwnershipComponent deserialize_net(NetworkBuffer& buffer);

    /**
     * @brief Get the serialized size in bytes for pre-allocation optimization.
     * @return Size in bytes when serialized via serialize_net().
     */
    static constexpr std::size_t get_serialized_size() {
        // version (1) + owner (1) + state (1) + state_changed_at (8) = 11 bytes
        return 11;
    }

    /**
     * @brief Get the component type ID for wire format.
     * @return Unique component type identifier.
     */
    static constexpr std::uint8_t get_type_id() {
        return ComponentTypeID::Ownership;
    }
};
static_assert(sizeof(OwnershipComponent) == 16, "OwnershipComponent size check");

/**
 * @struct TransformComponent
 * @brief 3D rendering transform for entities.
 *
 * This is the RENDERING transform, separate from PositionComponent which is for
 * game logic. PositionComponent uses GridPosition (int16 x,y) for tile-based
 * simulation. TransformComponent uses float Vec3 for smooth rendering.
 *
 * Uses quaternion for rotation (required for free camera where models are viewed
 * from all angles). Euler angles would suffer from gimbal lock.
 *
 * The cached model matrix is recomputed only when dirty flag is set, avoiding
 * redundant matrix calculations each frame.
 *
 * Position-to-Transform sync is handled by a separate system (ticket 2-033).
 *
 * Network wire format (version 2):
 *   [1 byte version]
 *   [12 bytes position (3x float)]
 *   [16 bytes rotation quaternion (4x float)]
 *   [12 bytes scale (3x float)]
 *   [1 byte dirty flag]
 *   [64 bytes model matrix (16x float)]
 *   Total: 106 bytes
 *
 * @note Component is trivially copyable (no pointers). Padding ensures alignment.
 */
struct TransformComponent {
    /// World-space position (floats for smooth rendering)
    glm::vec3 position{0.0f, 0.0f, 0.0f};

    /// Rotation as quaternion (required for free camera, avoids gimbal lock)
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  // Identity quaternion (w, x, y, z)

    /// Non-uniform scale (default 1,1,1 = no scaling)
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    /// Dirty flag for matrix recomputation
    /// Set to true when position, rotation, or scale changes.
    /// Rendering system clears this after recomputing model_matrix.
    bool dirty = true;

    /// Padding for alignment (3 bytes to align model_matrix to 4-byte boundary)
    std::uint8_t padding[3] = {};

    /// Cached model matrix (Mat4) - recomputed when dirty flag is set.
    /// Combines position, rotation, and scale into a single transform matrix.
    /// mat4 = T * R * S (Translation * Rotation * Scale)
    glm::mat4 model_matrix{1.0f};  // Identity matrix

    // =========================================================================
    // Helper Methods
    // =========================================================================

    /**
     * @brief Mark transform as needing matrix recomputation.
     */
    void set_dirty() { dirty = true; }

    /**
     * @brief Recompute model_matrix from position, rotation, and scale.
     *
     * Call this when dirty flag is true before using model_matrix for rendering.
     * Clears the dirty flag after recomputation.
     */
    void recompute_matrix();

    // =========================================================================
    // Network Serialization
    // =========================================================================

    void serialize_net(NetworkBuffer& buffer) const;
    static TransformComponent deserialize_net(NetworkBuffer& buffer);

    /**
     * @brief Get the serialized size in bytes.
     * version(1) + position(12) + rotation(16) + scale(12) + dirty(1) + model_matrix(64) = 106
     */
    static constexpr std::size_t get_serialized_size() { return 106; }
    static constexpr std::uint8_t get_type_id() { return ComponentTypeID::Transform; }
};
static_assert(sizeof(TransformComponent) == 108, "TransformComponent size check");

// RenderLayer enum is defined in sims3000/render/RenderLayer.h
// Included above for RenderComponent layer assignment.

/**
 * @struct RenderComponent
 * @brief Rendering information for entities that should be rendered.
 *
 * Contains all per-instance rendering data:
 * - Model and texture asset references
 * - Render layer for draw ordering
 * - Visibility and tint color
 * - Scale factor for size variation
 * - Emissive properties for bioluminescent glow
 *
 * Emissive state reflects power status:
 * - Powered buildings: emissive_intensity > 0
 * - Unpowered buildings: emissive_intensity = 0
 *
 * SyncPolicy::None - client-only visual data, never synced.
 */
struct RenderComponent {
    // Asset references
    ModelHandle model = nullptr;        ///< Handle to 3D model asset
    TextureHandle texture = nullptr;    ///< Handle to texture/material asset

    // Render properties
    glm::vec4 tint_color{1.0f, 1.0f, 1.0f, 1.0f};  ///< Instance tint color (RGBA), default white
    glm::vec3 emissive_color{0.0f, 1.0f, 0.8f};    ///< Bioluminescent glow color (RGB), default teal
    float scale = 1.0f;                             ///< Uniform scale factor for size variety
    float emissive_intensity = 0.0f;                ///< Glow intensity [0.0-1.0], 0=unpowered

    // State and classification
    RenderLayer layer = RenderLayer::Buildings;    ///< Render layer assignment
    bool visible = true;                           ///< Visibility flag
    std::uint8_t padding[2] = {};                  ///< Padding for alignment
};
static_assert(sizeof(RenderComponent) == 56, "RenderComponent size check");

/**
 * @struct BuildingComponent
 * @brief Data for building entities.
 *
 * Network wire format (version 1):
 *   [1 byte version][4 bytes buildingType][1 byte level][1 byte health][1 byte flags]
 *   Total: 8 bytes
 */
struct BuildingComponent {
    std::uint32_t buildingType = 0;
    std::uint8_t level = 1;
    std::uint8_t health = 100;
    std::uint8_t flags = 0;
    std::uint8_t padding = 0;

    void serialize_net(NetworkBuffer& buffer) const;
    static BuildingComponent deserialize_net(NetworkBuffer& buffer);
    static constexpr std::size_t get_serialized_size() { return 8; }
    static constexpr std::uint8_t get_type_id() { return ComponentTypeID::Building; }
};
static_assert(sizeof(BuildingComponent) == 8, "BuildingComponent size check");

/**
 * @struct EnergyComponent
 * @brief Energy consumption/production.
 *
 * Network wire format (version 1):
 *   [1 byte version][4 bytes consumption][4 bytes capacity][1 byte connected]
 *   Total: 10 bytes
 */
struct EnergyComponent {
    std::int32_t consumption = 0;  // Negative = produces
    std::int32_t capacity = 0;
    std::uint8_t connected = 0;
    std::uint8_t padding[3] = {};

    void serialize_net(NetworkBuffer& buffer) const;
    static EnergyComponent deserialize_net(NetworkBuffer& buffer);
    static constexpr std::size_t get_serialized_size() { return 10; }
    static constexpr std::uint8_t get_type_id() { return ComponentTypeID::Energy; }
};
static_assert(sizeof(EnergyComponent) == 12, "EnergyComponent size check");

/**
 * @struct PopulationComponent
 * @brief Population for residential buildings.
 *
 * Network wire format (version 1):
 *   [1 byte version][2 bytes current][2 bytes capacity][1 byte happiness][1 byte employmentRate]
 *   Total: 8 bytes
 */
struct PopulationComponent {
    std::uint16_t current = 0;
    std::uint16_t capacity = 0;
    std::uint8_t happiness = 50;
    std::uint8_t employmentRate = 0;
    std::uint8_t padding[2] = {};

    void serialize_net(NetworkBuffer& buffer) const;
    static PopulationComponent deserialize_net(NetworkBuffer& buffer);
    static constexpr std::size_t get_serialized_size() { return 8; }
    static constexpr std::uint8_t get_type_id() { return ComponentTypeID::Population; }
};
static_assert(sizeof(PopulationComponent) == 8, "PopulationComponent size check");

/**
 * @struct ZoneComponent
 * @brief Zone type assignment.
 *
 * Network wire format (version 1):
 *   [1 byte version][1 byte zoneType][1 byte density][1 byte desirability]
 *   Total: 4 bytes
 */
struct ZoneComponent {
    std::uint8_t zoneType = 0;     // 0=none, 1=residential, 2=commercial, 3=industrial
    std::uint8_t density = 1;      // 1=low, 2=medium, 3=high
    std::uint8_t desirability = 50;
    std::uint8_t padding = 0;

    void serialize_net(NetworkBuffer& buffer) const;
    static ZoneComponent deserialize_net(NetworkBuffer& buffer);
    static constexpr std::size_t get_serialized_size() { return 4; }
    static constexpr std::uint8_t get_type_id() { return ComponentTypeID::Zone; }
};
static_assert(sizeof(ZoneComponent) == 4, "ZoneComponent size check");

/**
 * @struct TransportComponent
 * @brief Transport network participation.
 *
 * Network wire format (version 1):
 *   [1 byte version][4 bytes roadConnectionId][2 bytes trafficLoad][1 byte accessibility]
 *   Total: 8 bytes
 */
struct TransportComponent {
    std::uint32_t roadConnectionId = 0;
    std::uint16_t trafficLoad = 0;
    std::uint8_t accessibility = 50;
    std::uint8_t padding = 0;

    void serialize_net(NetworkBuffer& buffer) const;
    static TransportComponent deserialize_net(NetworkBuffer& buffer);
    static constexpr std::size_t get_serialized_size() { return 8; }
    static constexpr std::uint8_t get_type_id() { return ComponentTypeID::Transport; }
};
static_assert(sizeof(TransportComponent) == 8, "TransportComponent size check");

/**
 * @struct ServiceCoverageComponent
 * @brief Service coverage levels (0-100).
 *
 * Network wire format (version 1):
 *   [1 byte version][1 byte police][1 byte fire][1 byte health][1 byte education][1 byte parks]
 *   Total: 6 bytes
 */
struct ServiceCoverageComponent {
    std::uint8_t police = 0;
    std::uint8_t fire = 0;
    std::uint8_t health = 0;
    std::uint8_t education = 0;
    std::uint8_t parks = 0;
    std::uint8_t padding[3] = {};

    void serialize_net(NetworkBuffer& buffer) const;
    static ServiceCoverageComponent deserialize_net(NetworkBuffer& buffer);
    static constexpr std::size_t get_serialized_size() { return 6; }
    static constexpr std::uint8_t get_type_id() { return ComponentTypeID::ServiceCoverage; }
};
static_assert(sizeof(ServiceCoverageComponent) == 8, "ServiceCoverageComponent size check");

/**
 * @struct TaxableComponent
 * @brief Economic/taxation data.
 *
 * Network wire format (version 1):
 *   [1 byte version][4 bytes income][4 bytes taxPaid][1 byte taxBracket]
 *   Total: 10 bytes
 */
struct TaxableComponent {
    std::int32_t income = 0;
    std::int32_t taxPaid = 0;
    std::uint8_t taxBracket = 10;
    std::uint8_t padding[3] = {};

    void serialize_net(NetworkBuffer& buffer) const;
    static TaxableComponent deserialize_net(NetworkBuffer& buffer);
    static constexpr std::size_t get_serialized_size() { return 10; }
    static constexpr std::uint8_t get_type_id() { return ComponentTypeID::Taxable; }
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
