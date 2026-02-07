/**
 * @file TerrainGridSerializer.cpp
 * @brief Implementation of TerrainGridSerializer.
 */

#include <sims3000/terrain/TerrainGridSerializer.h>
#include <cstring>

namespace sims3000 {
namespace terrain {

std::size_t TerrainGridSerializer::calculateSerializedSize(std::uint16_t width, std::uint16_t height) {
    const std::size_t tileCount = static_cast<std::size_t>(width) * height;
    // Header: 12 bytes
    // Tiles: tileCount * 4 bytes (TerrainComponent)
    // Water body IDs: tileCount * 2 bytes (uint16)
    // Flow directions: tileCount * 1 byte (uint8)
    return sizeof(TerrainGridHeader) + (tileCount * 4) + (tileCount * 2) + tileCount;
}

void TerrainGridSerializer::serialize(
    WriteBuffer& buffer,
    const TerrainGrid& grid,
    const WaterData& waterData,
    std::uint32_t mapSeed
) const {
    // Prepare header
    TerrainGridHeader header{};
    header.version = TERRAIN_GRID_FORMAT_VERSION;
    header.width = grid.width;
    header.height = grid.height;
    header.sea_level = grid.sea_level;
    header.reserved = 0;
    header.map_seed = mapSeed;

    // Write header
    writeHeader(buffer, header);

    // Write tile data
    writeTiles(buffer, grid);

    // Write water body IDs
    writeWaterBodyIds(buffer, waterData.water_body_ids);

    // Write flow directions
    writeFlowDirections(buffer, waterData.flow_directions);
}

TerrainSerializeResult TerrainGridSerializer::deserialize(
    ReadBuffer& buffer,
    TerrainGrid& grid,
    WaterData& waterData,
    std::uint32_t& mapSeed
) const {
    // Read and validate header (this consumes the header bytes)
    TerrainGridHeader header{};
    TerrainSerializeResult headerResult = validateHeader(buffer, header);
    if (headerResult != TerrainSerializeResult::Success) {
        return headerResult;
    }

    // Calculate expected data size
    const std::size_t tileCount = static_cast<std::size_t>(header.width) * header.height;
    const std::size_t expectedRemaining = (tileCount * 4) + (tileCount * 2) + tileCount;

    if (buffer.remaining() < expectedRemaining) {
        return TerrainSerializeResult::InsufficientData;
    }

    // Initialize grid with dimensions from header
    if (header.width == 128) {
        grid.initialize(MapSize::Small, header.sea_level);
    } else if (header.width == 256) {
        grid.initialize(MapSize::Medium, header.sea_level);
    } else if (header.width == 512) {
        grid.initialize(MapSize::Large, header.sea_level);
    } else {
        return TerrainSerializeResult::InvalidDimensions;
    }

    // Initialize water data with same dimensions
    waterData.water_body_ids.initialize(static_cast<MapSize>(header.width));
    waterData.flow_directions.initialize(static_cast<MapSize>(header.width));

    // Read tile data
    if (!readTiles(buffer, grid)) {
        return TerrainSerializeResult::CorruptData;
    }

    // Read water body IDs
    if (!readWaterBodyIds(buffer, waterData.water_body_ids)) {
        return TerrainSerializeResult::CorruptData;
    }

    // Read flow directions
    if (!readFlowDirections(buffer, waterData.flow_directions)) {
        return TerrainSerializeResult::CorruptData;
    }

    // Output map seed
    mapSeed = header.map_seed;

    return TerrainSerializeResult::Success;
}

TerrainSerializeResult TerrainGridSerializer::validateHeader(
    ReadBuffer& buffer,
    TerrainGridHeader& header
) const {
    // Check if we have enough data for header
    if (buffer.remaining() < sizeof(TerrainGridHeader)) {
        return TerrainSerializeResult::InsufficientData;
    }

    // Peek at header without consuming
    const std::size_t savedPos = buffer.position();

    // Read header fields
    header.version = buffer.readU16();
    header.width = buffer.readU16();
    header.height = buffer.readU16();
    header.sea_level = buffer.readU8();
    header.reserved = buffer.readU8();
    header.map_seed = buffer.readU32();

    // Validate version
    if (header.version < TERRAIN_GRID_MIN_VERSION || header.version > TERRAIN_GRID_FORMAT_VERSION) {
        return TerrainSerializeResult::InvalidVersion;
    }

    // Validate dimensions
    if (!isValidMapSize(header.width) || !isValidMapSize(header.height)) {
        return TerrainSerializeResult::InvalidDimensions;
    }

    // Validate square map
    if (header.width != header.height) {
        return TerrainSerializeResult::InvalidDimensions;
    }

    // Note: We've consumed the header bytes; caller should use this header
    // or create a fresh ReadBuffer if they need to re-read

    return TerrainSerializeResult::Success;
}

void TerrainGridSerializer::writeHeader(
    WriteBuffer& buffer,
    const TerrainGridHeader& header
) const {
    // Write each field in little-endian (native on most platforms)
    buffer.writeU16(header.version);
    buffer.writeU16(header.width);
    buffer.writeU16(header.height);
    buffer.writeU8(header.sea_level);
    buffer.writeU8(header.reserved);
    buffer.writeU32(header.map_seed);
}

bool TerrainGridSerializer::readHeader(
    ReadBuffer& buffer,
    TerrainGridHeader& header
) const {
    if (buffer.remaining() < sizeof(TerrainGridHeader)) {
        return false;
    }

    header.version = buffer.readU16();
    header.width = buffer.readU16();
    header.height = buffer.readU16();
    header.sea_level = buffer.readU8();
    header.reserved = buffer.readU8();
    header.map_seed = buffer.readU32();

    return true;
}

void TerrainGridSerializer::writeTiles(
    WriteBuffer& buffer,
    const TerrainGrid& grid
) const {
    // Write tiles in row-major order (matching grid storage)
    // TerrainComponent is trivially copyable, 4 bytes each
    for (const auto& tile : grid.tiles) {
        buffer.writeU8(tile.terrain_type);
        buffer.writeU8(tile.elevation);
        buffer.writeU8(tile.moisture);
        buffer.writeU8(tile.flags);
    }
}

bool TerrainGridSerializer::readTiles(
    ReadBuffer& buffer,
    TerrainGrid& grid
) const {
    const std::size_t tileCount = grid.tile_count();
    const std::size_t bytesNeeded = tileCount * 4;

    if (buffer.remaining() < bytesNeeded) {
        return false;
    }

    for (std::size_t i = 0; i < tileCount; ++i) {
        grid.tiles[i].terrain_type = buffer.readU8();
        grid.tiles[i].elevation = buffer.readU8();
        grid.tiles[i].moisture = buffer.readU8();
        grid.tiles[i].flags = buffer.readU8();
    }

    return true;
}

void TerrainGridSerializer::writeWaterBodyIds(
    WriteBuffer& buffer,
    const WaterBodyGrid& waterBodyGrid
) const {
    // Write water body IDs in row-major order
    for (const auto& bodyId : waterBodyGrid.body_ids) {
        buffer.writeU16(bodyId);
    }
}

bool TerrainGridSerializer::readWaterBodyIds(
    ReadBuffer& buffer,
    WaterBodyGrid& waterBodyGrid
) const {
    const std::size_t tileCount = waterBodyGrid.tile_count();
    const std::size_t bytesNeeded = tileCount * 2;

    if (buffer.remaining() < bytesNeeded) {
        return false;
    }

    for (std::size_t i = 0; i < tileCount; ++i) {
        waterBodyGrid.body_ids[i] = buffer.readU16();
    }

    return true;
}

void TerrainGridSerializer::writeFlowDirections(
    WriteBuffer& buffer,
    const FlowDirectionGrid& flowGrid
) const {
    // Write flow directions in row-major order
    for (const auto& dir : flowGrid.directions) {
        buffer.writeU8(static_cast<std::uint8_t>(dir));
    }
}

bool TerrainGridSerializer::readFlowDirections(
    ReadBuffer& buffer,
    FlowDirectionGrid& flowGrid
) const {
    const std::size_t tileCount = flowGrid.tile_count();
    const std::size_t bytesNeeded = tileCount;

    if (buffer.remaining() < bytesNeeded) {
        return false;
    }

    for (std::size_t i = 0; i < tileCount; ++i) {
        std::uint8_t value = buffer.readU8();
        // Validate flow direction value
        if (isValidFlowDirection(value)) {
            flowGrid.directions[i] = static_cast<FlowDirection>(value);
        } else {
            flowGrid.directions[i] = FlowDirection::None;
        }
    }

    return true;
}

} // namespace terrain
} // namespace sims3000
