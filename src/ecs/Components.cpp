/**
 * @file Components.cpp
 * @brief Component serialization implementations.
 */

#include "sims3000/ecs/Components.h"
#include "sims3000/net/NetworkBuffer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstring>

namespace sims3000 {

void PositionComponent::serialize(std::vector<std::uint8_t>& buffer) const {
    // Serialize GridPosition (x, y) - 4 bytes
    const auto* posBytes = reinterpret_cast<const std::uint8_t*>(&pos);
    buffer.insert(buffer.end(), posBytes, posBytes + sizeof(GridPosition));

    // Serialize elevation - 2 bytes
    const auto* elevBytes = reinterpret_cast<const std::uint8_t*>(&elevation);
    buffer.insert(buffer.end(), elevBytes, elevBytes + sizeof(elevation));
}

PositionComponent PositionComponent::deserialize(const std::uint8_t* data, std::size_t& offset) {
    PositionComponent result;

    // Deserialize GridPosition (x, y) - 4 bytes
    std::memcpy(&result.pos, data + offset, sizeof(GridPosition));
    offset += sizeof(GridPosition);

    // Deserialize elevation - 2 bytes
    std::memcpy(&result.elevation, data + offset, sizeof(result.elevation));
    offset += sizeof(result.elevation);

    return result;
}

void OwnershipComponent::serialize(std::vector<std::uint8_t>& buffer) const {
    // Serialize owner - 1 byte
    buffer.push_back(owner);

    // Serialize state - 1 byte
    buffer.push_back(static_cast<std::uint8_t>(state));

    // Serialize state_changed_at - 8 bytes
    const auto* tickBytes = reinterpret_cast<const std::uint8_t*>(&state_changed_at);
    buffer.insert(buffer.end(), tickBytes, tickBytes + sizeof(state_changed_at));
}

OwnershipComponent OwnershipComponent::deserialize(const std::uint8_t* data, std::size_t& offset) {
    OwnershipComponent result;

    // Deserialize owner - 1 byte
    result.owner = data[offset++];

    // Deserialize state - 1 byte
    result.state = static_cast<OwnershipState>(data[offset++]);

    // Deserialize state_changed_at - 8 bytes
    std::memcpy(&result.state_changed_at, data + offset, sizeof(result.state_changed_at));
    offset += sizeof(result.state_changed_at);

    return result;
}

// =============================================================================
// Network Serialization (using NetworkBuffer)
// =============================================================================

// -----------------------------------------------------------------------------
// PositionComponent Network Serialization
// -----------------------------------------------------------------------------

void PositionComponent::serialize_net(NetworkBuffer& buffer) const {
    // Write version byte first for backward compatibility
    buffer.write_u8(ComponentVersion::Position);

    // Write grid position (x, y) as signed 16-bit integers
    // Note: NetworkBuffer doesn't have write_i16, so we use write_u16 with reinterpret
    std::uint16_t x_as_u16;
    std::uint16_t y_as_u16;
    std::uint16_t elev_as_u16;
    std::memcpy(&x_as_u16, &pos.x, sizeof(pos.x));
    std::memcpy(&y_as_u16, &pos.y, sizeof(pos.y));
    std::memcpy(&elev_as_u16, &elevation, sizeof(elevation));

    buffer.write_u16(x_as_u16);
    buffer.write_u16(y_as_u16);
    buffer.write_u16(elev_as_u16);
}

PositionComponent PositionComponent::deserialize_net(NetworkBuffer& buffer) {
    PositionComponent result;

    // Read and validate version
    std::uint8_t version = buffer.read_u8();

    if (version >= 1) {
        // Version 1 format: grid_x, grid_y, elevation
        std::uint16_t x_as_u16 = buffer.read_u16();
        std::uint16_t y_as_u16 = buffer.read_u16();
        std::uint16_t elev_as_u16 = buffer.read_u16();

        std::memcpy(&result.pos.x, &x_as_u16, sizeof(result.pos.x));
        std::memcpy(&result.pos.y, &y_as_u16, sizeof(result.pos.y));
        std::memcpy(&result.elevation, &elev_as_u16, sizeof(result.elevation));
    }
    // Future versions would add else-if branches here
    // Default values are already set in the struct definition

    return result;
}

// -----------------------------------------------------------------------------
// OwnershipComponent Network Serialization
// -----------------------------------------------------------------------------

void OwnershipComponent::serialize_net(NetworkBuffer& buffer) const {
    // Write version byte first for backward compatibility
    buffer.write_u8(ComponentVersion::Ownership);

    // Write owner (PlayerID is u8)
    buffer.write_u8(owner);

    // Write state (OwnershipState is u8 enum)
    buffer.write_u8(static_cast<std::uint8_t>(state));

    // Write state_changed_at (SimulationTick is u64)
    // NetworkBuffer doesn't have write_u64, so write as two u32s (low, high)
    std::uint32_t low = static_cast<std::uint32_t>(state_changed_at & 0xFFFFFFFF);
    std::uint32_t high = static_cast<std::uint32_t>((state_changed_at >> 32) & 0xFFFFFFFF);
    buffer.write_u32(low);
    buffer.write_u32(high);
}

OwnershipComponent OwnershipComponent::deserialize_net(NetworkBuffer& buffer) {
    OwnershipComponent result;

    // Read and validate version
    std::uint8_t version = buffer.read_u8();

    if (version >= 1) {
        // Version 1 format: owner, state, state_changed_at
        result.owner = buffer.read_u8();
        result.state = static_cast<OwnershipState>(buffer.read_u8());

        // Read state_changed_at as two u32s (low, high)
        std::uint32_t low = buffer.read_u32();
        std::uint32_t high = buffer.read_u32();
        result.state_changed_at = static_cast<SimulationTick>(low) |
                                  (static_cast<SimulationTick>(high) << 32);
    }
    // Future versions would add else-if branches here
    // Default values are already set in the struct definition

    return result;
}

// -----------------------------------------------------------------------------
// TransformComponent Helper Methods
// -----------------------------------------------------------------------------

void TransformComponent::recompute_matrix() {
    // Model matrix = T * R * S (Translation * Rotation * Scale)
    // Start with identity matrix
    model_matrix = glm::mat4(1.0f);

    // Apply translation
    model_matrix = glm::translate(model_matrix, position);

    // Apply rotation (quaternion to rotation matrix)
    model_matrix *= glm::mat4_cast(rotation);

    // Apply scale
    model_matrix = glm::scale(model_matrix, scale);

    // Clear dirty flag
    dirty = false;
}

// -----------------------------------------------------------------------------
// TransformComponent Network Serialization
// -----------------------------------------------------------------------------

void TransformComponent::serialize_net(NetworkBuffer& buffer) const {
    // Version 2 format: full transform with quaternion rotation
    buffer.write_u8(ComponentVersion::Transform);

    // Position (12 bytes)
    buffer.write_f32(position.x);
    buffer.write_f32(position.y);
    buffer.write_f32(position.z);

    // Rotation quaternion (16 bytes) - note: glm::quat stores as (w, x, y, z)
    buffer.write_f32(rotation.w);
    buffer.write_f32(rotation.x);
    buffer.write_f32(rotation.y);
    buffer.write_f32(rotation.z);

    // Scale (12 bytes)
    buffer.write_f32(scale.x);
    buffer.write_f32(scale.y);
    buffer.write_f32(scale.z);

    // Dirty flag (1 byte)
    buffer.write_u8(dirty ? 1 : 0);

    // Model matrix (64 bytes) - column-major order
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            buffer.write_f32(model_matrix[col][row]);
        }
    }
}

TransformComponent TransformComponent::deserialize_net(NetworkBuffer& buffer) {
    TransformComponent result;
    std::uint8_t version = buffer.read_u8();

    if (version >= 2) {
        // Version 2: full transform with quaternion rotation
        // Position
        result.position.x = buffer.read_f32();
        result.position.y = buffer.read_f32();
        result.position.z = buffer.read_f32();

        // Rotation quaternion
        result.rotation.w = buffer.read_f32();
        result.rotation.x = buffer.read_f32();
        result.rotation.y = buffer.read_f32();
        result.rotation.z = buffer.read_f32();

        // Scale
        result.scale.x = buffer.read_f32();
        result.scale.y = buffer.read_f32();
        result.scale.z = buffer.read_f32();

        // Dirty flag
        result.dirty = buffer.read_u8() != 0;

        // Model matrix (column-major order)
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                result.model_matrix[col][row] = buffer.read_f32();
            }
        }
    } else if (version >= 1) {
        // Version 1 compatibility: old format with position + Y-axis rotation
        result.position.x = buffer.read_f32();
        result.position.y = buffer.read_f32();
        result.position.z = buffer.read_f32();

        // Old rotation was Y-axis rotation in radians
        float y_rotation = buffer.read_f32();
        result.rotation = glm::angleAxis(y_rotation, glm::vec3(0.0f, 1.0f, 0.0f));

        // Default scale (1,1,1) is already set
        // Mark as dirty so matrix gets recomputed
        result.dirty = true;
    }

    return result;
}

// -----------------------------------------------------------------------------
// BuildingComponent Network Serialization
// -----------------------------------------------------------------------------

void BuildingComponent::serialize_net(NetworkBuffer& buffer) const {
    buffer.write_u8(ComponentVersion::Building);
    buffer.write_u32(buildingType);
    buffer.write_u8(level);
    buffer.write_u8(health);
    buffer.write_u8(flags);
}

BuildingComponent BuildingComponent::deserialize_net(NetworkBuffer& buffer) {
    BuildingComponent result;
    std::uint8_t version = buffer.read_u8();

    if (version >= 1) {
        result.buildingType = buffer.read_u32();
        result.level = buffer.read_u8();
        result.health = buffer.read_u8();
        result.flags = buffer.read_u8();
    }

    return result;
}

// -----------------------------------------------------------------------------
// EnergyComponent Network Serialization
// -----------------------------------------------------------------------------

void EnergyComponent::serialize_net(NetworkBuffer& buffer) const {
    buffer.write_u8(ComponentVersion::Energy);
    buffer.write_i32(consumption);
    buffer.write_i32(capacity);
    buffer.write_u8(connected);
}

EnergyComponent EnergyComponent::deserialize_net(NetworkBuffer& buffer) {
    EnergyComponent result;
    std::uint8_t version = buffer.read_u8();

    if (version >= 1) {
        result.consumption = buffer.read_i32();
        result.capacity = buffer.read_i32();
        result.connected = buffer.read_u8();
    }

    return result;
}

// -----------------------------------------------------------------------------
// PopulationComponent Network Serialization
// -----------------------------------------------------------------------------

void PopulationComponent::serialize_net(NetworkBuffer& buffer) const {
    buffer.write_u8(ComponentVersion::Population);
    buffer.write_u16(current);
    buffer.write_u16(capacity);
    buffer.write_u8(happiness);
    buffer.write_u8(employmentRate);
}

PopulationComponent PopulationComponent::deserialize_net(NetworkBuffer& buffer) {
    PopulationComponent result;
    std::uint8_t version = buffer.read_u8();

    if (version >= 1) {
        result.current = buffer.read_u16();
        result.capacity = buffer.read_u16();
        result.happiness = buffer.read_u8();
        result.employmentRate = buffer.read_u8();
    }

    return result;
}

// -----------------------------------------------------------------------------
// ZoneComponent Network Serialization
// -----------------------------------------------------------------------------

void ZoneComponent::serialize_net(NetworkBuffer& buffer) const {
    buffer.write_u8(ComponentVersion::Zone);
    buffer.write_u8(zoneType);
    buffer.write_u8(density);
    buffer.write_u8(desirability);
}

ZoneComponent ZoneComponent::deserialize_net(NetworkBuffer& buffer) {
    ZoneComponent result;
    std::uint8_t version = buffer.read_u8();

    if (version >= 1) {
        result.zoneType = buffer.read_u8();
        result.density = buffer.read_u8();
        result.desirability = buffer.read_u8();
    }

    return result;
}

// -----------------------------------------------------------------------------
// TransportComponent Network Serialization
// -----------------------------------------------------------------------------

void TransportComponent::serialize_net(NetworkBuffer& buffer) const {
    buffer.write_u8(ComponentVersion::Transport);
    buffer.write_u32(roadConnectionId);
    buffer.write_u16(trafficLoad);
    buffer.write_u8(accessibility);
}

TransportComponent TransportComponent::deserialize_net(NetworkBuffer& buffer) {
    TransportComponent result;
    std::uint8_t version = buffer.read_u8();

    if (version >= 1) {
        result.roadConnectionId = buffer.read_u32();
        result.trafficLoad = buffer.read_u16();
        result.accessibility = buffer.read_u8();
    }

    return result;
}

// -----------------------------------------------------------------------------
// ServiceCoverageComponent Network Serialization
// -----------------------------------------------------------------------------

void ServiceCoverageComponent::serialize_net(NetworkBuffer& buffer) const {
    buffer.write_u8(ComponentVersion::ServiceCoverage);
    buffer.write_u8(police);
    buffer.write_u8(fire);
    buffer.write_u8(health);
    buffer.write_u8(education);
    buffer.write_u8(parks);
}

ServiceCoverageComponent ServiceCoverageComponent::deserialize_net(NetworkBuffer& buffer) {
    ServiceCoverageComponent result;
    std::uint8_t version = buffer.read_u8();

    if (version >= 1) {
        result.police = buffer.read_u8();
        result.fire = buffer.read_u8();
        result.health = buffer.read_u8();
        result.education = buffer.read_u8();
        result.parks = buffer.read_u8();
    }

    return result;
}

// -----------------------------------------------------------------------------
// TaxableComponent Network Serialization
// -----------------------------------------------------------------------------

void TaxableComponent::serialize_net(NetworkBuffer& buffer) const {
    buffer.write_u8(ComponentVersion::Taxable);
    buffer.write_i32(income);
    buffer.write_i32(taxPaid);
    buffer.write_u8(taxBracket);
}

TaxableComponent TaxableComponent::deserialize_net(NetworkBuffer& buffer) {
    TaxableComponent result;
    std::uint8_t version = buffer.read_u8();

    if (version >= 1) {
        result.income = buffer.read_i32();
        result.taxPaid = buffer.read_i32();
        result.taxBracket = buffer.read_u8();
    }

    return result;
}

} // namespace sims3000
