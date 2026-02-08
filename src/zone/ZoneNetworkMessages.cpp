/**
 * @file ZoneNetworkMessages.cpp
 * @brief Implementation of zone network message serialization (Ticket 4-038)
 *
 * Uses little-endian encoding for multi-byte fields with memcpy-based
 * serialization for fixed-size fields.
 */

#include <sims3000/zone/ZoneNetworkMessages.h>
#include <cstring>

namespace sims3000 {
namespace zone {

// =============================================================================
// Helper functions for little-endian encoding
// =============================================================================

namespace {

void write_le_i32(std::vector<std::uint8_t>& buf, std::int32_t val) {
    std::uint32_t uval;
    std::memcpy(&uval, &val, sizeof(uval));
    buf.push_back(static_cast<std::uint8_t>(uval & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((uval >> 8) & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((uval >> 16) & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((uval >> 24) & 0xFF));
}

std::int32_t read_le_i32(const std::uint8_t* data) {
    std::uint32_t uval = static_cast<std::uint32_t>(data[0])
                        | (static_cast<std::uint32_t>(data[1]) << 8)
                        | (static_cast<std::uint32_t>(data[2]) << 16)
                        | (static_cast<std::uint32_t>(data[3]) << 24);
    std::int32_t val;
    std::memcpy(&val, &uval, sizeof(val));
    return val;
}

} // anonymous namespace

// =============================================================================
// ZonePlacementRequestMsg
// =============================================================================

std::vector<std::uint8_t> ZonePlacementRequestMsg::serialize() const {
    // Layout: version(1) + x(4) + y(4) + width(4) + height(4) + zone_type(1) + density(1) = 19 bytes
    std::vector<std::uint8_t> buf;
    buf.reserve(19);
    buf.push_back(version);
    write_le_i32(buf, x);
    write_le_i32(buf, y);
    write_le_i32(buf, width);
    write_le_i32(buf, height);
    buf.push_back(zone_type);
    buf.push_back(density);
    return buf;
}

bool ZonePlacementRequestMsg::deserialize(const std::vector<std::uint8_t>& data, ZonePlacementRequestMsg& out) {
    if (data.size() < 19) {
        return false;
    }
    out.version = data[0];
    out.x = read_le_i32(&data[1]);
    out.y = read_le_i32(&data[5]);
    out.width = read_le_i32(&data[9]);
    out.height = read_le_i32(&data[13]);
    out.zone_type = data[17];
    out.density = data[18];
    return true;
}

// =============================================================================
// DezoneRequestMsg
// =============================================================================

std::vector<std::uint8_t> DezoneRequestMsg::serialize() const {
    // Layout: version(1) + x(4) + y(4) + width(4) + height(4) = 17 bytes
    std::vector<std::uint8_t> buf;
    buf.reserve(17);
    buf.push_back(version);
    write_le_i32(buf, x);
    write_le_i32(buf, y);
    write_le_i32(buf, width);
    write_le_i32(buf, height);
    return buf;
}

bool DezoneRequestMsg::deserialize(const std::vector<std::uint8_t>& data, DezoneRequestMsg& out) {
    if (data.size() < 17) {
        return false;
    }
    out.version = data[0];
    out.x = read_le_i32(&data[1]);
    out.y = read_le_i32(&data[5]);
    out.width = read_le_i32(&data[9]);
    out.height = read_le_i32(&data[13]);
    return true;
}

// =============================================================================
// RedesignateRequestMsg
// =============================================================================

std::vector<std::uint8_t> RedesignateRequestMsg::serialize() const {
    // Layout: version(1) + x(4) + y(4) + new_zone_type(1) + new_density(1) = 11 bytes
    std::vector<std::uint8_t> buf;
    buf.reserve(11);
    buf.push_back(version);
    write_le_i32(buf, x);
    write_le_i32(buf, y);
    buf.push_back(new_zone_type);
    buf.push_back(new_density);
    return buf;
}

bool RedesignateRequestMsg::deserialize(const std::vector<std::uint8_t>& data, RedesignateRequestMsg& out) {
    if (data.size() < 11) {
        return false;
    }
    out.version = data[0];
    out.x = read_le_i32(&data[1]);
    out.y = read_le_i32(&data[5]);
    out.new_zone_type = data[9];
    out.new_density = data[10];
    return true;
}

// =============================================================================
// ZoneDemandSyncMsg
// =============================================================================

std::vector<std::uint8_t> ZoneDemandSyncMsg::serialize() const {
    // Layout: version(1) + player_id(1) + habitation(1) + exchange(1) + fabrication(1) = 5 bytes
    std::vector<std::uint8_t> buf;
    buf.reserve(5);
    buf.push_back(version);
    buf.push_back(player_id);
    buf.push_back(static_cast<std::uint8_t>(habitation_demand));
    buf.push_back(static_cast<std::uint8_t>(exchange_demand));
    buf.push_back(static_cast<std::uint8_t>(fabrication_demand));
    return buf;
}

bool ZoneDemandSyncMsg::deserialize(const std::vector<std::uint8_t>& data, ZoneDemandSyncMsg& out) {
    if (data.size() < 5) {
        return false;
    }
    out.version = data[0];
    out.player_id = data[1];
    out.habitation_demand = static_cast<std::int8_t>(data[2]);
    out.exchange_demand = static_cast<std::int8_t>(data[3]);
    out.fabrication_demand = static_cast<std::int8_t>(data[4]);
    return true;
}

} // namespace zone
} // namespace sims3000
