/**
 * @file ZoneSerialization.cpp
 * @brief Implementation of zone data serialization/deserialization
 */

#include <sims3000/zone/ZoneSerialization.h>
#include <stdexcept>
#include <cstring>

namespace sims3000 {
namespace zone {

// ============================================================================
// Little-endian helper functions
// ============================================================================

static void write_uint8(std::vector<std::uint8_t>& buf, std::uint8_t value) {
    buf.push_back(value);
}

static void write_int8(std::vector<std::uint8_t>& buf, std::int8_t value) {
    buf.push_back(static_cast<std::uint8_t>(value));
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

static std::uint8_t read_uint8(const std::uint8_t* data, std::size_t& offset) {
    return data[offset++];
}

static std::int8_t read_int8(const std::uint8_t* data, std::size_t& offset) {
    return static_cast<std::int8_t>(data[offset++]);
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

// ============================================================================
// ZoneComponent serialization
// ============================================================================

void serialize_zone_component(const ZoneComponent& comp, std::vector<std::uint8_t>& buffer) {
    write_uint8(buffer, ZONE_SERIALIZATION_VERSION);
    write_uint8(buffer, comp.zone_type);
    write_uint8(buffer, comp.density);
    write_uint8(buffer, comp.state);
    write_uint8(buffer, comp.desirability);
}

ZoneComponent deserialize_zone_component(const std::uint8_t* data, std::size_t size) {
    if (size < 5) {
        throw std::runtime_error("ZoneComponent deserialization: buffer too small");
    }

    std::size_t offset = 0;
    std::uint8_t version = read_uint8(data, offset);
    (void)version; // Version 1 is the only version for now

    ZoneComponent comp;
    comp.zone_type = read_uint8(data, offset);
    comp.density = read_uint8(data, offset);
    comp.state = read_uint8(data, offset);
    comp.desirability = read_uint8(data, offset);

    return comp;
}

// ============================================================================
// ZoneGrid serialization
// ============================================================================

void serialize_zone_grid(const ZoneGrid& grid, std::vector<std::uint8_t>& buffer) {
    write_uint8(buffer, ZONE_SERIALIZATION_VERSION);
    write_uint16_le(buffer, grid.getWidth());
    write_uint16_le(buffer, grid.getHeight());

    // Count non-empty cells for cell_count
    std::uint32_t cell_count = 0;
    for (std::int32_t y = 0; y < static_cast<std::int32_t>(grid.getHeight()); ++y) {
        for (std::int32_t x = 0; x < static_cast<std::int32_t>(grid.getWidth()); ++x) {
            if (grid.get_zone_at(x, y) != INVALID_ENTITY) {
                ++cell_count;
            }
        }
    }

    write_uint32_le(buffer, cell_count);

    // Write each non-empty cell as (x:uint16, y:uint16, entity_id:uint32) = 8 bytes each
    for (std::int32_t y = 0; y < static_cast<std::int32_t>(grid.getHeight()); ++y) {
        for (std::int32_t x = 0; x < static_cast<std::int32_t>(grid.getWidth()); ++x) {
            std::uint32_t entity_id = grid.get_zone_at(x, y);
            if (entity_id != INVALID_ENTITY) {
                write_uint16_le(buffer, static_cast<std::uint16_t>(x));
                write_uint16_le(buffer, static_cast<std::uint16_t>(y));
                write_uint32_le(buffer, entity_id);
            }
        }
    }
}

ZoneGrid deserialize_zone_grid(const std::uint8_t* data, std::size_t size) {
    if (size < 9) { // version(1) + width(2) + height(2) + cell_count(4) = 9
        throw std::runtime_error("ZoneGrid deserialization: buffer too small");
    }

    std::size_t offset = 0;
    std::uint8_t version = read_uint8(data, offset);
    (void)version;

    std::uint16_t width = read_uint16_le(data, offset);
    std::uint16_t height = read_uint16_le(data, offset);
    std::uint32_t cell_count = read_uint32_le(data, offset);

    // Handle empty/zero-dimension grid
    if (width == 0 || height == 0) {
        return ZoneGrid();
    }

    ZoneGrid grid(width, height);

    // Read cells
    std::size_t required = 9 + static_cast<std::size_t>(cell_count) * 8;
    if (size < required) {
        throw std::runtime_error("ZoneGrid deserialization: buffer too small for cells");
    }

    for (std::uint32_t i = 0; i < cell_count; ++i) {
        std::uint16_t x = read_uint16_le(data, offset);
        std::uint16_t y = read_uint16_le(data, offset);
        std::uint32_t entity_id = read_uint32_le(data, offset);
        grid.place_zone(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y), entity_id);
    }

    return grid;
}

// ============================================================================
// ZoneCounts serialization
// ============================================================================

void serialize_zone_counts(const ZoneCounts& counts, std::vector<std::uint8_t>& buffer) {
    write_uint8(buffer, ZONE_SERIALIZATION_VERSION);
    write_uint32_le(buffer, counts.habitation_total);
    write_uint32_le(buffer, counts.exchange_total);
    write_uint32_le(buffer, counts.fabrication_total);
    write_uint32_le(buffer, counts.aeroport_total);
    write_uint32_le(buffer, counts.aquaport_total);
    write_uint32_le(buffer, counts.low_density_total);
    write_uint32_le(buffer, counts.high_density_total);
    write_uint32_le(buffer, counts.designated_total);
    write_uint32_le(buffer, counts.occupied_total);
    write_uint32_le(buffer, counts.stalled_total);
    write_uint32_le(buffer, counts.total);
}

ZoneCounts deserialize_zone_counts(const std::uint8_t* data, std::size_t size) {
    if (size < 45) { // version(1) + 11 * uint32(4) = 45
        throw std::runtime_error("ZoneCounts deserialization: buffer too small");
    }

    std::size_t offset = 0;
    std::uint8_t version = read_uint8(data, offset);
    (void)version;

    ZoneCounts counts;
    counts.habitation_total = read_uint32_le(data, offset);
    counts.exchange_total = read_uint32_le(data, offset);
    counts.fabrication_total = read_uint32_le(data, offset);
    counts.aeroport_total = read_uint32_le(data, offset);
    counts.aquaport_total = read_uint32_le(data, offset);
    counts.low_density_total = read_uint32_le(data, offset);
    counts.high_density_total = read_uint32_le(data, offset);
    counts.designated_total = read_uint32_le(data, offset);
    counts.occupied_total = read_uint32_le(data, offset);
    counts.stalled_total = read_uint32_le(data, offset);
    counts.total = read_uint32_le(data, offset);

    return counts;
}

// ============================================================================
// ZoneDemandData serialization
// ============================================================================

void serialize_zone_demand_data(const ZoneDemandData& demand, std::vector<std::uint8_t>& buffer) {
    write_uint8(buffer, ZONE_SERIALIZATION_VERSION);
    write_int8(buffer, demand.habitation_demand);
    write_int8(buffer, demand.exchange_demand);
    write_int8(buffer, demand.fabrication_demand);
}

ZoneDemandData deserialize_zone_demand_data(const std::uint8_t* data, std::size_t size) {
    if (size < 4) { // version(1) + 3 * int8(1) = 4
        throw std::runtime_error("ZoneDemandData deserialization: buffer too small");
    }

    std::size_t offset = 0;
    std::uint8_t version = read_uint8(data, offset);
    (void)version;

    ZoneDemandData demand;
    demand.habitation_demand = read_int8(data, offset);
    demand.exchange_demand = read_int8(data, offset);
    demand.fabrication_demand = read_int8(data, offset);

    return demand;
}

} // namespace zone
} // namespace sims3000
