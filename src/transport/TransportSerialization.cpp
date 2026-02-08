/**
 * @file TransportSerialization.cpp
 * @brief Implementation of transport component serialization
 *        (Epic 7, tickets E7-036, E7-037)
 */

#include <sims3000/transport/TransportSerialization.h>
#include <stdexcept>

namespace sims3000 {
namespace transport {

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

static void write_uint32_le(std::vector<uint8_t>& buf, uint32_t value) {
    buf.push_back(static_cast<uint8_t>(value & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
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

static uint32_t read_uint32_le(const uint8_t* data, size_t& offset) {
    uint32_t value = static_cast<uint32_t>(data[offset])
                   | (static_cast<uint32_t>(data[offset + 1]) << 8)
                   | (static_cast<uint32_t>(data[offset + 2]) << 16)
                   | (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    return value;
}

// ============================================================================
// RoadComponent serialization (Ticket E7-036)
// ============================================================================

void serialize_road_component(const RoadComponent& comp, std::vector<uint8_t>& buffer) {
    // Version byte
    write_uint8(buffer, TRANSPORT_SERIALIZATION_VERSION);       // 1 byte

    // Field-by-field little-endian serialization
    write_uint8(buffer, static_cast<uint8_t>(comp.type));       // 1 byte
    write_uint8(buffer, static_cast<uint8_t>(comp.direction));  // 1 byte
    write_uint16_le(buffer, comp.base_capacity);                // 2 bytes
    write_uint16_le(buffer, comp.current_capacity);             // 2 bytes
    write_uint8(buffer, comp.health);                           // 1 byte
    write_uint8(buffer, comp.decay_rate);                       // 1 byte
    write_uint8(buffer, comp.connection_mask);                  // 1 byte
    write_uint8(buffer, comp.is_junction ? 1 : 0);             // 1 byte
    write_uint16_le(buffer, comp.network_id);                   // 2 bytes
    write_uint32_le(buffer, comp.last_maintained_tick);         // 4 bytes
    // Total: 1 + 16 = 17 bytes
}

size_t deserialize_road_component(const uint8_t* data, size_t size, RoadComponent& comp) {
    if (size < ROAD_COMPONENT_SERIALIZED_SIZE) {
        throw std::runtime_error("RoadComponent deserialization: buffer too small");
    }

    size_t offset = 0;
    const uint8_t version = read_uint8(data, offset);

    if (version != TRANSPORT_SERIALIZATION_VERSION) {
        throw std::runtime_error("RoadComponent deserialization: unsupported version");
    }

    comp.type = static_cast<PathwayType>(read_uint8(data, offset));
    comp.direction = static_cast<PathwayDirection>(read_uint8(data, offset));
    comp.base_capacity = read_uint16_le(data, offset);
    comp.current_capacity = read_uint16_le(data, offset);
    comp.health = read_uint8(data, offset);
    comp.decay_rate = read_uint8(data, offset);
    comp.connection_mask = read_uint8(data, offset);
    comp.is_junction = (read_uint8(data, offset) != 0);
    comp.network_id = read_uint16_le(data, offset);
    comp.last_maintained_tick = read_uint32_le(data, offset);

    return offset; // 17 bytes consumed
}

// ============================================================================
// TrafficComponent serialization (Ticket E7-037)
// ============================================================================

void serialize_traffic_component(const TrafficComponent& comp, std::vector<uint8_t>& buffer) {
    // Version byte
    write_uint8(buffer, TRANSPORT_SERIALIZATION_VERSION);       // 1 byte

    // Field-by-field little-endian serialization (skip padding)
    write_uint32_le(buffer, comp.flow_current);                 // 4 bytes
    write_uint32_le(buffer, comp.flow_previous);                // 4 bytes
    write_uint16_le(buffer, comp.flow_sources);                 // 2 bytes
    write_uint8(buffer, comp.congestion_level);                 // 1 byte
    write_uint8(buffer, comp.flow_blockage_ticks);              // 1 byte
    write_uint8(buffer, comp.contamination_rate);               // 1 byte
    // Total: 1 + 13 = 14 bytes (padding NOT serialized)
}

size_t deserialize_traffic_component(const uint8_t* data, size_t size, TrafficComponent& comp) {
    if (size < TRAFFIC_COMPONENT_SERIALIZED_SIZE) {
        throw std::runtime_error("TrafficComponent deserialization: buffer too small");
    }

    size_t offset = 0;
    const uint8_t version = read_uint8(data, offset);

    if (version != TRANSPORT_SERIALIZATION_VERSION) {
        throw std::runtime_error("TrafficComponent deserialization: unsupported version");
    }

    comp.flow_current = read_uint32_le(data, offset);
    comp.flow_previous = read_uint32_le(data, offset);
    comp.flow_sources = read_uint16_le(data, offset);
    comp.congestion_level = read_uint8(data, offset);
    comp.flow_blockage_ticks = read_uint8(data, offset);
    comp.contamination_rate = read_uint8(data, offset);
    // Padding is zeroed on deserialization (default initialization)
    comp.padding[0] = 0;
    comp.padding[1] = 0;
    comp.padding[2] = 0;

    return offset; // 14 bytes consumed
}

} // namespace transport
} // namespace sims3000
