/**
 * @file Components.cpp
 * @brief Component serialization implementations.
 */

#include "sims3000/ecs/Components.h"
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

} // namespace sims3000
