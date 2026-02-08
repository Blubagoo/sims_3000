/**
 * @file PortSerialization.cpp
 * @brief Implementation of port component serialization
 *        (Epic 8, ticket E8-002)
 */

#include <sims3000/port/PortSerialization.h>
#include <stdexcept>

namespace sims3000 {
namespace port {

// ============================================================================
// Little-endian helper functions
// ============================================================================

static void write_uint8(std::vector<uint8_t>& buf, uint8_t value) {
    buf.push_back(value);
}

static void write_uint16_le(std::vector<uint8_t>& buf, uint16_t value) {
    buf.push_back(static_cast<uint8_t>(value & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

static uint8_t read_uint8(const uint8_t* data, size_t& offset) {
    return data[offset++];
}

static uint16_t read_uint16_le(const uint8_t* data, size_t& offset) {
    uint16_t value = static_cast<uint16_t>(data[offset])
                   | (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;
    return value;
}

// ============================================================================
// PortComponent serialization (Ticket E8-002)
// ============================================================================

void serialize_port_component(const PortComponent& comp, std::vector<uint8_t>& buffer) {
    // Version byte
    write_uint8(buffer, PORT_SERIALIZATION_VERSION);                    // 1 byte

    // Field-by-field little-endian serialization
    write_uint8(buffer, static_cast<uint8_t>(comp.port_type));          // 1 byte
    write_uint16_le(buffer, comp.capacity);                             // 2 bytes
    write_uint16_le(buffer, comp.max_capacity);                         // 2 bytes
    write_uint8(buffer, comp.utilization);                              // 1 byte
    write_uint8(buffer, comp.infrastructure_level);                     // 1 byte
    write_uint8(buffer, comp.is_operational ? 1 : 0);                   // 1 byte
    write_uint8(buffer, comp.is_connected_to_edge ? 1 : 0);            // 1 byte
    write_uint8(buffer, comp.demand_bonus_radius);                      // 1 byte
    write_uint8(buffer, comp.connection_flags);                         // 1 byte
    // Total: 1 + 11 = 12 bytes (padding NOT serialized)
}

size_t deserialize_port_component(const uint8_t* data, size_t size, PortComponent& comp) {
    if (size < PORT_COMPONENT_SERIALIZED_SIZE) {
        throw std::runtime_error("PortComponent deserialization: buffer too small");
    }

    size_t offset = 0;
    const uint8_t version = read_uint8(data, offset);

    if (version != PORT_SERIALIZATION_VERSION) {
        throw std::runtime_error("PortComponent deserialization: unsupported version");
    }

    comp.port_type = static_cast<PortType>(read_uint8(data, offset));
    comp.capacity = read_uint16_le(data, offset);
    comp.max_capacity = read_uint16_le(data, offset);
    comp.utilization = read_uint8(data, offset);
    comp.infrastructure_level = read_uint8(data, offset);
    comp.is_operational = (read_uint8(data, offset) != 0);
    comp.is_connected_to_edge = (read_uint8(data, offset) != 0);
    comp.demand_bonus_radius = read_uint8(data, offset);
    comp.connection_flags = read_uint8(data, offset);
    // Padding is zeroed on deserialization (default initialization)
    comp.padding[0] = 0;
    comp.padding[1] = 0;
    comp.padding[2] = 0;
    comp.padding[3] = 0;

    return offset; // 12 bytes consumed
}

} // namespace port
} // namespace sims3000
