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
#include "sims3000/assets/ModelLoader.h"
#include "sims3000/assets/TextureLoader.h"

#include <glm/glm.hpp>
#include <cstdint>

namespace sims3000 {

/**
 * @struct PositionComponent
 * @brief Tile-based position on the game grid.
 */
struct PositionComponent {
    GridPosition pos;
};
static_assert(sizeof(PositionComponent) == 4, "PositionComponent size check");

/**
 * @struct OwnershipComponent
 * @brief Entity ownership for multiplayer.
 */
struct OwnershipComponent {
    PlayerID owner = 0;  // 0 = no owner (neutral)
};
static_assert(sizeof(OwnershipComponent) == 1, "OwnershipComponent size check");

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

} // namespace sims3000

#endif // SIMS3000_ECS_COMPONENTS_H
