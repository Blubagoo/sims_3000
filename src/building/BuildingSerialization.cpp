/**
 * @file BuildingSerialization.cpp
 * @brief Implementation of building component serialization/deserialization
 */

#include <sims3000/building/BuildingSerialization.h>
#include <stdexcept>

namespace sims3000 {
namespace building {

// ============================================================================
// Little-endian helper functions
// ============================================================================

static void write_uint8(std::vector<std::uint8_t>& buf, std::uint8_t value) {
    buf.push_back(value);
}

static void write_uint16_le(std::vector<std::uint8_t>& buf, std::uint16_t value) {
    buf.push_back(static_cast<std::uint8_t>(value & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
}

static void write_uint32_le(std::vector<std::uint8_t>& buf, std::uint32_t value) {
    buf.push_back(static_cast<std::uint8_t>(value & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
}

static void write_reserved(std::vector<std::uint8_t>& buf, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        buf.push_back(0);
    }
}

static std::uint8_t read_uint8(const std::uint8_t* data, std::size_t& offset) {
    return data[offset++];
}

static std::uint16_t read_uint16_le(const std::uint8_t* data, std::size_t& offset) {
    std::uint16_t value = static_cast<std::uint16_t>(data[offset])
                        | (static_cast<std::uint16_t>(data[offset + 1]) << 8);
    offset += 2;
    return value;
}

static std::uint32_t read_uint32_le(const std::uint8_t* data, std::size_t& offset) {
    std::uint32_t value = static_cast<std::uint32_t>(data[offset])
                        | (static_cast<std::uint32_t>(data[offset + 1]) << 8)
                        | (static_cast<std::uint32_t>(data[offset + 2]) << 16)
                        | (static_cast<std::uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    return value;
}

static void skip_reserved(const std::uint8_t* /*data*/, std::size_t& offset, std::size_t count) {
    offset += count;
}

// ============================================================================
// BuildingComponent serialization
// ============================================================================

void serialize_building_component(const BuildingComponent& comp, std::vector<std::uint8_t>& buffer) {
    write_uint8(buffer, BUILDING_SERIALIZATION_VERSION);   // 1
    write_uint32_le(buffer, comp.template_id);             // 4
    write_uint8(buffer, comp.zone_type);                   // 1
    write_uint8(buffer, comp.density);                     // 1
    write_uint8(buffer, comp.state);                       // 1
    write_uint8(buffer, comp.level);                       // 1
    write_uint8(buffer, comp.health);                      // 1
    write_uint16_le(buffer, comp.capacity);                // 2
    write_uint16_le(buffer, comp.current_occupancy);       // 2
    write_uint8(buffer, comp.footprint_w);                 // 1
    write_uint8(buffer, comp.footprint_h);                 // 1
    write_uint32_le(buffer, comp.state_changed_tick);      // 4
    write_uint16_le(buffer, comp.abandon_timer);           // 2
    write_uint8(buffer, comp.rotation);                    // 1
    write_uint8(buffer, comp.color_accent_index);          // 1
    write_reserved(buffer, 4);                             // 4 reserved
    // Total: 28 bytes
}

BuildingComponent deserialize_building_component(const std::uint8_t* data, std::size_t size) {
    // Minimum size check: at least the non-reserved fields
    // For version compatibility, accept the pre-reserved size too
    if (size < 24) { // 28 - 4 reserved = 24 minimum for v1 without reserved
        throw std::runtime_error("BuildingComponent deserialization: buffer too small");
    }

    std::size_t offset = 0;
    std::uint8_t version = read_uint8(data, offset);

    BuildingComponent comp;
    comp.template_id = read_uint32_le(data, offset);
    comp.zone_type = read_uint8(data, offset);
    comp.density = read_uint8(data, offset);
    comp.state = read_uint8(data, offset);
    comp.level = read_uint8(data, offset);
    comp.health = read_uint8(data, offset);
    comp.capacity = read_uint16_le(data, offset);
    comp.current_occupancy = read_uint16_le(data, offset);
    comp.footprint_w = read_uint8(data, offset);
    comp.footprint_h = read_uint8(data, offset);
    comp.state_changed_tick = read_uint32_le(data, offset);
    comp.abandon_timer = read_uint16_le(data, offset);
    comp.rotation = read_uint8(data, offset);
    comp.color_accent_index = read_uint8(data, offset);

    // Skip reserved bytes if present
    if (version >= 1 && size >= 28) {
        skip_reserved(data, offset, 4);
    }

    return comp;
}

// ============================================================================
// ConstructionComponent serialization
// ============================================================================

void serialize_construction_component(const ConstructionComponent& comp, std::vector<std::uint8_t>& buffer) {
    write_uint8(buffer, BUILDING_SERIALIZATION_VERSION);   // 1
    write_uint16_le(buffer, comp.ticks_total);             // 2
    write_uint16_le(buffer, comp.ticks_elapsed);           // 2
    write_uint8(buffer, comp.phase);                       // 1
    write_uint8(buffer, comp.phase_progress);              // 1
    write_uint32_le(buffer, comp.construction_cost);       // 4
    write_uint8(buffer, comp.is_paused);                   // 1
    write_reserved(buffer, 4);                             // 4 reserved
    // Total: 16 bytes
}

ConstructionComponent deserialize_construction_component(const std::uint8_t* data, std::size_t size) {
    if (size < 12) { // 16 - 4 reserved = 12 minimum
        throw std::runtime_error("ConstructionComponent deserialization: buffer too small");
    }

    std::size_t offset = 0;
    std::uint8_t version = read_uint8(data, offset);

    ConstructionComponent comp;
    comp.ticks_total = read_uint16_le(data, offset);
    comp.ticks_elapsed = read_uint16_le(data, offset);
    comp.phase = read_uint8(data, offset);
    comp.phase_progress = read_uint8(data, offset);
    comp.construction_cost = read_uint32_le(data, offset);
    comp.is_paused = read_uint8(data, offset);

    // Skip reserved bytes if present
    if (version >= 1 && size >= 16) {
        skip_reserved(data, offset, 4);
    }

    return comp;
}

// ============================================================================
// DebrisComponent serialization
// ============================================================================

void serialize_debris_component(const DebrisComponent& comp, std::vector<std::uint8_t>& buffer) {
    write_uint8(buffer, BUILDING_SERIALIZATION_VERSION);   // 1
    write_uint32_le(buffer, comp.original_template_id);    // 4
    write_uint16_le(buffer, comp.clear_timer);             // 2
    write_uint8(buffer, comp.footprint_w);                 // 1
    write_uint8(buffer, comp.footprint_h);                 // 1
    write_reserved(buffer, 4);                             // 4 reserved
    // Total: 13 bytes
}

DebrisComponent deserialize_debris_component(const std::uint8_t* data, std::size_t size) {
    if (size < 9) { // 13 - 4 reserved = 9 minimum
        throw std::runtime_error("DebrisComponent deserialization: buffer too small");
    }

    std::size_t offset = 0;
    std::uint8_t version = read_uint8(data, offset);

    DebrisComponent comp;
    comp.original_template_id = read_uint32_le(data, offset);
    comp.clear_timer = read_uint16_le(data, offset);
    comp.footprint_w = read_uint8(data, offset);
    comp.footprint_h = read_uint8(data, offset);

    // Skip reserved bytes if present
    if (version >= 1 && size >= 13) {
        skip_reserved(data, offset, 4);
    }

    return comp;
}

} // namespace building
} // namespace sims3000
