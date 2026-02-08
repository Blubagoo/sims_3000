/**
 * @file PathwayNetworkMessages.cpp
 * @brief Implementation of pathway network message serialization (Ticket E7-038)
 *
 * Uses little-endian encoding for all multi-byte fields, matching the
 * pattern established in TransportSerialization.cpp.
 */

#include <sims3000/transport/PathwayNetworkMessages.h>
#include <cstring>
#include <stdexcept>

namespace sims3000 {
namespace transport {

// ============================================================================
// Little-endian helper functions
// ============================================================================

namespace {

void write_u8(std::vector<uint8_t>& buf, uint8_t value) {
    buf.push_back(value);
}

void write_i32_le(std::vector<uint8_t>& buf, int32_t value) {
    uint32_t uval;
    std::memcpy(&uval, &value, sizeof(uval));
    buf.push_back(static_cast<uint8_t>(uval & 0xFF));
    buf.push_back(static_cast<uint8_t>((uval >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((uval >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((uval >> 24) & 0xFF));
}

void write_u32_le(std::vector<uint8_t>& buf, uint32_t value) {
    buf.push_back(static_cast<uint8_t>(value & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

int32_t read_i32_le(const uint8_t* data, size_t& offset) {
    uint32_t uval = static_cast<uint32_t>(data[offset])
                  | (static_cast<uint32_t>(data[offset + 1]) << 8)
                  | (static_cast<uint32_t>(data[offset + 2]) << 16)
                  | (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    int32_t val;
    std::memcpy(&val, &uval, sizeof(val));
    return val;
}

uint32_t read_u32_le(const uint8_t* data, size_t& offset) {
    uint32_t value = static_cast<uint32_t>(data[offset])
                   | (static_cast<uint32_t>(data[offset + 1]) << 8)
                   | (static_cast<uint32_t>(data[offset + 2]) << 16)
                   | (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    return value;
}

uint8_t read_u8(const uint8_t* data, size_t& offset) {
    return data[offset++];
}

} // anonymous namespace

// ============================================================================
// PathwayPlaceRequest
// ============================================================================

void PathwayPlaceRequest::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_i32_le(buffer, x);                                 // 4 bytes
    write_i32_le(buffer, y);                                 // 4 bytes
    write_u8(buffer, static_cast<uint8_t>(type));            // 1 byte
    write_u8(buffer, owner);                                 // 1 byte
    // Total: 10 bytes
}

PathwayPlaceRequest PathwayPlaceRequest::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("PathwayPlaceRequest::deserialize: buffer too small");
    }

    PathwayPlaceRequest msg;
    size_t offset = 0;
    msg.x = read_i32_le(data, offset);
    msg.y = read_i32_le(data, offset);
    msg.type = static_cast<PathwayType>(read_u8(data, offset));
    msg.owner = read_u8(data, offset);
    return msg;
}

// ============================================================================
// PathwayPlaceResponse
// ============================================================================

void PathwayPlaceResponse::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u8(buffer, success ? 1 : 0);                      // 1 byte
    write_u32_le(buffer, entity_id);                         // 4 bytes
    write_i32_le(buffer, x);                                 // 4 bytes
    write_i32_le(buffer, y);                                 // 4 bytes
    write_u8(buffer, error_code);                            // 1 byte
    // Total: 14 bytes
}

PathwayPlaceResponse PathwayPlaceResponse::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("PathwayPlaceResponse::deserialize: buffer too small");
    }

    PathwayPlaceResponse msg;
    size_t offset = 0;
    msg.success = (read_u8(data, offset) != 0);
    msg.entity_id = read_u32_le(data, offset);
    msg.x = read_i32_le(data, offset);
    msg.y = read_i32_le(data, offset);
    msg.error_code = read_u8(data, offset);
    return msg;
}

// ============================================================================
// PathwayDemolishRequest
// ============================================================================

void PathwayDemolishRequest::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u32_le(buffer, entity_id);                         // 4 bytes
    write_i32_le(buffer, x);                                 // 4 bytes
    write_i32_le(buffer, y);                                 // 4 bytes
    write_u8(buffer, owner);                                 // 1 byte
    // Total: 13 bytes
}

PathwayDemolishRequest PathwayDemolishRequest::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("PathwayDemolishRequest::deserialize: buffer too small");
    }

    PathwayDemolishRequest msg;
    size_t offset = 0;
    msg.entity_id = read_u32_le(data, offset);
    msg.x = read_i32_le(data, offset);
    msg.y = read_i32_le(data, offset);
    msg.owner = read_u8(data, offset);
    return msg;
}

// ============================================================================
// PathwayDemolishResponse
// ============================================================================

void PathwayDemolishResponse::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u8(buffer, success ? 1 : 0);                      // 1 byte
    write_u32_le(buffer, entity_id);                         // 4 bytes
    write_u8(buffer, error_code);                            // 1 byte
    // Total: 6 bytes
}

PathwayDemolishResponse PathwayDemolishResponse::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("PathwayDemolishResponse::deserialize: buffer too small");
    }

    PathwayDemolishResponse msg;
    size_t offset = 0;
    msg.success = (read_u8(data, offset) != 0);
    msg.entity_id = read_u32_le(data, offset);
    msg.error_code = read_u8(data, offset);
    return msg;
}

} // namespace transport
} // namespace sims3000
