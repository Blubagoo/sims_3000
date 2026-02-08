/**
 * @file RailNetworkMessages.cpp
 * @brief Implementation of rail and terminal network message serialization (Ticket E7-039)
 *
 * Uses little-endian encoding for all multi-byte fields, matching the
 * pattern established in TransportSerialization.cpp.
 */

#include <sims3000/transport/RailNetworkMessages.h>
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
// RailPlaceRequest
// ============================================================================

void RailPlaceRequest::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_i32_le(buffer, x);                                 // 4 bytes
    write_i32_le(buffer, y);                                 // 4 bytes
    write_u8(buffer, static_cast<uint8_t>(type));            // 1 byte
    write_u8(buffer, owner);                                 // 1 byte
    // Total: 10 bytes
}

RailPlaceRequest RailPlaceRequest::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("RailPlaceRequest::deserialize: buffer too small");
    }

    RailPlaceRequest msg;
    size_t offset = 0;
    msg.x = read_i32_le(data, offset);
    msg.y = read_i32_le(data, offset);
    msg.type = static_cast<RailType>(read_u8(data, offset));
    msg.owner = read_u8(data, offset);
    return msg;
}

// ============================================================================
// RailPlaceResponse
// ============================================================================

void RailPlaceResponse::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u8(buffer, success ? 1 : 0);                      // 1 byte
    write_u32_le(buffer, entity_id);                         // 4 bytes
    write_u8(buffer, error_code);                            // 1 byte
    // Total: 6 bytes
}

RailPlaceResponse RailPlaceResponse::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("RailPlaceResponse::deserialize: buffer too small");
    }

    RailPlaceResponse msg;
    size_t offset = 0;
    msg.success = (read_u8(data, offset) != 0);
    msg.entity_id = read_u32_le(data, offset);
    msg.error_code = read_u8(data, offset);
    return msg;
}

// ============================================================================
// RailDemolishRequest
// ============================================================================

void RailDemolishRequest::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u32_le(buffer, entity_id);                         // 4 bytes
    write_u8(buffer, owner);                                 // 1 byte
    // Total: 5 bytes
}

RailDemolishRequest RailDemolishRequest::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("RailDemolishRequest::deserialize: buffer too small");
    }

    RailDemolishRequest msg;
    size_t offset = 0;
    msg.entity_id = read_u32_le(data, offset);
    msg.owner = read_u8(data, offset);
    return msg;
}

// ============================================================================
// RailDemolishResponse
// ============================================================================

void RailDemolishResponse::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u8(buffer, success ? 1 : 0);                      // 1 byte
    write_u32_le(buffer, entity_id);                         // 4 bytes
    write_u8(buffer, error_code);                            // 1 byte
    // Total: 6 bytes
}

RailDemolishResponse RailDemolishResponse::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("RailDemolishResponse::deserialize: buffer too small");
    }

    RailDemolishResponse msg;
    size_t offset = 0;
    msg.success = (read_u8(data, offset) != 0);
    msg.entity_id = read_u32_le(data, offset);
    msg.error_code = read_u8(data, offset);
    return msg;
}

// ============================================================================
// TerminalPlaceRequest
// ============================================================================

void TerminalPlaceRequest::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_i32_le(buffer, x);                                 // 4 bytes
    write_i32_le(buffer, y);                                 // 4 bytes
    write_u8(buffer, static_cast<uint8_t>(type));            // 1 byte
    write_u8(buffer, owner);                                 // 1 byte
    // Total: 10 bytes
}

TerminalPlaceRequest TerminalPlaceRequest::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("TerminalPlaceRequest::deserialize: buffer too small");
    }

    TerminalPlaceRequest msg;
    size_t offset = 0;
    msg.x = read_i32_le(data, offset);
    msg.y = read_i32_le(data, offset);
    msg.type = static_cast<TerminalType>(read_u8(data, offset));
    msg.owner = read_u8(data, offset);
    return msg;
}

// ============================================================================
// TerminalPlaceResponse
// ============================================================================

void TerminalPlaceResponse::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u8(buffer, success ? 1 : 0);                      // 1 byte
    write_u32_le(buffer, entity_id);                         // 4 bytes
    write_u8(buffer, error_code);                            // 1 byte
    // Total: 6 bytes
}

TerminalPlaceResponse TerminalPlaceResponse::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("TerminalPlaceResponse::deserialize: buffer too small");
    }

    TerminalPlaceResponse msg;
    size_t offset = 0;
    msg.success = (read_u8(data, offset) != 0);
    msg.entity_id = read_u32_le(data, offset);
    msg.error_code = read_u8(data, offset);
    return msg;
}

// ============================================================================
// TerminalDemolishRequest
// ============================================================================

void TerminalDemolishRequest::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u32_le(buffer, entity_id);                         // 4 bytes
    write_u8(buffer, owner);                                 // 1 byte
    // Total: 5 bytes
}

TerminalDemolishRequest TerminalDemolishRequest::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("TerminalDemolishRequest::deserialize: buffer too small");
    }

    TerminalDemolishRequest msg;
    size_t offset = 0;
    msg.entity_id = read_u32_le(data, offset);
    msg.owner = read_u8(data, offset);
    return msg;
}

// ============================================================================
// TerminalDemolishResponse
// ============================================================================

void TerminalDemolishResponse::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u8(buffer, success ? 1 : 0);                      // 1 byte
    write_u32_le(buffer, entity_id);                         // 4 bytes
    write_u8(buffer, error_code);                            // 1 byte
    // Total: 6 bytes
}

TerminalDemolishResponse TerminalDemolishResponse::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("TerminalDemolishResponse::deserialize: buffer too small");
    }

    TerminalDemolishResponse msg;
    size_t offset = 0;
    msg.success = (read_u8(data, offset) != 0);
    msg.entity_id = read_u32_le(data, offset);
    msg.error_code = read_u8(data, offset);
    return msg;
}

} // namespace transport
} // namespace sims3000
