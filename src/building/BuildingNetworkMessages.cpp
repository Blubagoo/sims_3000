/**
 * @file BuildingNetworkMessages.cpp
 * @brief Implementation of building network message serialization (Ticket 4-040)
 *
 * Uses little-endian encoding for multi-byte fields with manual
 * byte-level serialization for fixed-size fields.
 */

#include <sims3000/building/BuildingNetworkMessages.h>
#include <cstring>

namespace sims3000 {
namespace building {

// =============================================================================
// Helper functions for little-endian encoding
// =============================================================================

namespace {

void write_le_u32(std::vector<std::uint8_t>& buf, std::uint32_t val) {
    buf.push_back(static_cast<std::uint8_t>(val & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((val >> 16) & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((val >> 24) & 0xFF));
}

std::uint32_t read_le_u32(const std::uint8_t* data) {
    return static_cast<std::uint32_t>(data[0])
         | (static_cast<std::uint32_t>(data[1]) << 8)
         | (static_cast<std::uint32_t>(data[2]) << 16)
         | (static_cast<std::uint32_t>(data[3]) << 24);
}

void write_le_i32(std::vector<std::uint8_t>& buf, std::int32_t val) {
    std::uint32_t uval;
    std::memcpy(&uval, &val, sizeof(uval));
    write_le_u32(buf, uval);
}

std::int32_t read_le_i32(const std::uint8_t* data) {
    std::uint32_t uval = read_le_u32(data);
    std::int32_t val;
    std::memcpy(&val, &uval, sizeof(val));
    return val;
}

void write_le_u16(std::vector<std::uint8_t>& buf, std::uint16_t val) {
    buf.push_back(static_cast<std::uint8_t>(val & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((val >> 8) & 0xFF));
}

std::uint16_t read_le_u16(const std::uint8_t* data) {
    return static_cast<std::uint16_t>(data[0])
         | (static_cast<std::uint16_t>(data[1]) << 8);
}

} // anonymous namespace

// =============================================================================
// DemolishRequestMessage
// =============================================================================

std::vector<std::uint8_t> DemolishRequestMessage::serialize() const {
    // Layout: version(1) + entity_id(4) = 5 bytes
    std::vector<std::uint8_t> buf;
    buf.reserve(5);
    buf.push_back(version);
    write_le_u32(buf, entity_id);
    return buf;
}

bool DemolishRequestMessage::deserialize(const std::vector<std::uint8_t>& data, DemolishRequestMessage& out) {
    if (data.size() < 5) {
        return false;
    }
    out.version = data[0];
    out.entity_id = read_le_u32(&data[1]);
    return true;
}

// =============================================================================
// ClearDebrisMessage
// =============================================================================

std::vector<std::uint8_t> ClearDebrisMessage::serialize() const {
    // Layout: version(1) + entity_id(4) = 5 bytes
    std::vector<std::uint8_t> buf;
    buf.reserve(5);
    buf.push_back(version);
    write_le_u32(buf, entity_id);
    return buf;
}

bool ClearDebrisMessage::deserialize(const std::vector<std::uint8_t>& data, ClearDebrisMessage& out) {
    if (data.size() < 5) {
        return false;
    }
    out.version = data[0];
    out.entity_id = read_le_u32(&data[1]);
    return true;
}

// =============================================================================
// BuildingSpawnedMessage
// =============================================================================

std::vector<std::uint8_t> BuildingSpawnedMessage::serialize() const {
    // Layout: version(1) + entity_id(4) + grid_x(4) + grid_y(4) + template_id(4)
    //       + owner_id(1) + rotation(1) + color_accent_index(1) = 20 bytes
    std::vector<std::uint8_t> buf;
    buf.reserve(20);
    buf.push_back(version);
    write_le_u32(buf, entity_id);
    write_le_i32(buf, grid_x);
    write_le_i32(buf, grid_y);
    write_le_u32(buf, template_id);
    buf.push_back(owner_id);
    buf.push_back(rotation);
    buf.push_back(color_accent_index);
    return buf;
}

bool BuildingSpawnedMessage::deserialize(const std::vector<std::uint8_t>& data, BuildingSpawnedMessage& out) {
    if (data.size() < 20) {
        return false;
    }
    out.version = data[0];
    out.entity_id = read_le_u32(&data[1]);
    out.grid_x = read_le_i32(&data[5]);
    out.grid_y = read_le_i32(&data[9]);
    out.template_id = read_le_u32(&data[13]);
    out.owner_id = data[17];
    out.rotation = data[18];
    out.color_accent_index = data[19];
    return true;
}

// =============================================================================
// BuildingStateChangedMessage
// =============================================================================

std::vector<std::uint8_t> BuildingStateChangedMessage::serialize() const {
    // Layout: version(1) + entity_id(4) + new_state(1) = 6 bytes
    std::vector<std::uint8_t> buf;
    buf.reserve(6);
    buf.push_back(version);
    write_le_u32(buf, entity_id);
    buf.push_back(new_state);
    return buf;
}

bool BuildingStateChangedMessage::deserialize(const std::vector<std::uint8_t>& data, BuildingStateChangedMessage& out) {
    if (data.size() < 6) {
        return false;
    }
    out.version = data[0];
    out.entity_id = read_le_u32(&data[1]);
    out.new_state = data[5];
    return true;
}

// =============================================================================
// BuildingUpgradedMessage
// =============================================================================

std::vector<std::uint8_t> BuildingUpgradedMessage::serialize() const {
    // Layout: version(1) + entity_id(4) + new_level(1) + new_template_id(4) = 10 bytes
    std::vector<std::uint8_t> buf;
    buf.reserve(10);
    buf.push_back(version);
    write_le_u32(buf, entity_id);
    buf.push_back(new_level);
    write_le_u32(buf, new_template_id);
    return buf;
}

bool BuildingUpgradedMessage::deserialize(const std::vector<std::uint8_t>& data, BuildingUpgradedMessage& out) {
    if (data.size() < 10) {
        return false;
    }
    out.version = data[0];
    out.entity_id = read_le_u32(&data[1]);
    out.new_level = data[5];
    out.new_template_id = read_le_u32(&data[6]);
    return true;
}

// =============================================================================
// ConstructionProgressMessage
// =============================================================================

std::vector<std::uint8_t> ConstructionProgressMessage::serialize() const {
    // Layout: version(1) + entity_id(4) + ticks_elapsed(2) + ticks_total(2) = 9 bytes
    std::vector<std::uint8_t> buf;
    buf.reserve(9);
    buf.push_back(version);
    write_le_u32(buf, entity_id);
    write_le_u16(buf, ticks_elapsed);
    write_le_u16(buf, ticks_total);
    return buf;
}

bool ConstructionProgressMessage::deserialize(const std::vector<std::uint8_t>& data, ConstructionProgressMessage& out) {
    if (data.size() < 9) {
        return false;
    }
    out.version = data[0];
    out.entity_id = read_le_u32(&data[1]);
    out.ticks_elapsed = read_le_u16(&data[5]);
    out.ticks_total = read_le_u16(&data[7]);
    return true;
}

// =============================================================================
// BuildingDemolishedMessage
// =============================================================================

std::vector<std::uint8_t> BuildingDemolishedMessage::serialize() const {
    // Layout: version(1) + entity_id(4) = 5 bytes
    std::vector<std::uint8_t> buf;
    buf.reserve(5);
    buf.push_back(version);
    write_le_u32(buf, entity_id);
    return buf;
}

bool BuildingDemolishedMessage::deserialize(const std::vector<std::uint8_t>& data, BuildingDemolishedMessage& out) {
    if (data.size() < 5) {
        return false;
    }
    out.version = data[0];
    out.entity_id = read_le_u32(&data[1]);
    return true;
}

// =============================================================================
// DebrisClearedMessage
// =============================================================================

std::vector<std::uint8_t> DebrisClearedMessage::serialize() const {
    // Layout: version(1) + entity_id(4) + grid_x(4) + grid_y(4) = 13 bytes
    std::vector<std::uint8_t> buf;
    buf.reserve(13);
    buf.push_back(version);
    write_le_u32(buf, entity_id);
    write_le_i32(buf, grid_x);
    write_le_i32(buf, grid_y);
    return buf;
}

bool DebrisClearedMessage::deserialize(const std::vector<std::uint8_t>& data, DebrisClearedMessage& out) {
    if (data.size() < 13) {
        return false;
    }
    out.version = data[0];
    out.entity_id = read_le_u32(&data[1]);
    out.grid_x = read_le_i32(&data[5]);
    out.grid_y = read_le_i32(&data[9]);
    return true;
}

} // namespace building
} // namespace sims3000
