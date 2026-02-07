/**
 * @file TerrainGridSerializer.h
 * @brief Binary serialization for TerrainGrid save/load and network transfer.
 *
 * Implements ISerializable for TerrainGrid to support:
 * - Save/load (Epic 16) with versioned format
 * - Full snapshot network transfer
 *
 * Binary format (little-endian):
 * - Header (12 bytes):
 *   - version: uint16 (format version for backwards compatibility)
 *   - width: uint16 (128, 256, or 512)
 *   - height: uint16 (128, 256, or 512)
 *   - sea_level: uint8
 *   - reserved: uint8 (padding/future use)
 *   - map_seed: uint32 (for reproducibility)
 * - Tile data (width * height * 4 bytes):
 *   - TerrainComponent: 4 bytes per tile (terrain_type, elevation, moisture, flags)
 * - Water body IDs (width * height * 2 bytes):
 *   - WaterBodyID: 2 bytes per tile (uint16)
 * - Flow directions (width * height * 1 byte):
 *   - FlowDirection: 1 byte per tile (uint8)
 *
 * Total size: 12 + (width * height * 7) bytes
 * - 128x128: 12 + 114,688 = 114,700 bytes (~112KB)
 * - 256x256: 12 + 458,752 = 458,764 bytes (~448KB)
 * - 512x512: 12 + 1,835,008 = 1,835,020 bytes (~1.75MB)
 *
 * @see /docs/canon/patterns.yaml (persistence.save_file_format)
 */

#ifndef SIMS3000_TERRAIN_TERRAINGRIDSERIALIZER_H
#define SIMS3000_TERRAIN_TERRAINGRIDSERIALIZER_H

#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/WaterData.h>
#include <sims3000/core/Serialization.h>
#include <cstdint>

namespace sims3000 {
namespace terrain {

/**
 * @brief Current terrain grid serialization format version.
 *
 * Increment when format changes to support backwards compatibility.
 * Version history:
 * - v1: Initial format (header + tiles + water body IDs + flow directions)
 */
constexpr std::uint16_t TERRAIN_GRID_FORMAT_VERSION = 1;

/**
 * @brief Minimum supported format version for deserialization.
 *
 * Versions older than this cannot be loaded.
 */
constexpr std::uint16_t TERRAIN_GRID_MIN_VERSION = 1;

/**
 * @struct TerrainGridHeader
 * @brief Binary header for serialized terrain data.
 *
 * Fixed 12-byte header at the start of serialized data.
 * All fields are little-endian.
 */
struct TerrainGridHeader {
    std::uint16_t version;      ///< Format version (for migration support)
    std::uint16_t width;        ///< Grid width (128, 256, or 512)
    std::uint16_t height;       ///< Grid height (128, 256, or 512)
    std::uint8_t sea_level;     ///< Sea level elevation (0-31)
    std::uint8_t reserved;      ///< Reserved for future use (currently 0)
    std::uint32_t map_seed;     ///< Map generation seed for reproducibility
};

// Verify header is exactly 12 bytes
static_assert(sizeof(TerrainGridHeader) == 12,
    "TerrainGridHeader must be exactly 12 bytes");

/**
 * @enum TerrainSerializeResult
 * @brief Result codes for terrain serialization operations.
 */
enum class TerrainSerializeResult : std::uint8_t {
    Success = 0,            ///< Operation completed successfully
    InvalidVersion = 1,     ///< Version not supported
    InvalidDimensions = 2,  ///< Width/height not valid (must be 128, 256, or 512)
    InsufficientData = 3,   ///< Buffer too small for expected data
    CorruptData = 4,        ///< Data integrity check failed
    SizeMismatch = 5        ///< Water data dimensions don't match terrain grid
};

/**
 * @class TerrainGridSerializer
 * @brief Serializes TerrainGrid with associated WaterData for save/load and network transfer.
 *
 * This class handles the complete serialization of terrain data including:
 * - TerrainGrid (dimensions, sea level, tile data)
 * - WaterData (water body IDs, flow directions)
 * - Map seed (for reproducibility)
 *
 * The serializer uses fixed-size types with explicit little-endian encoding
 * for cross-platform compatibility.
 *
 * Usage:
 * @code
 * // Serialize
 * TerrainGrid grid(MapSize::Medium);
 * WaterData waterData(MapSize::Medium);
 * uint32_t mapSeed = 12345;
 *
 * TerrainGridSerializer serializer;
 * WriteBuffer buffer;
 * serializer.serialize(buffer, grid, waterData, mapSeed);
 *
 * // Deserialize
 * TerrainGrid loadedGrid;
 * WaterData loadedWaterData;
 * uint32_t loadedSeed = 0;
 *
 * ReadBuffer readBuf(buffer.data(), buffer.size());
 * auto result = serializer.deserialize(readBuf, loadedGrid, loadedWaterData, loadedSeed);
 * if (result != TerrainSerializeResult::Success) {
 *     // Handle error
 * }
 * @endcode
 */
class TerrainGridSerializer {
public:
    /**
     * @brief Calculate expected serialized size for given dimensions.
     *
     * @param width Grid width.
     * @param height Grid height.
     * @return Total size in bytes: header (12) + tiles (w*h*4) + water IDs (w*h*2) + flow (w*h*1)
     */
    static std::size_t calculateSerializedSize(std::uint16_t width, std::uint16_t height);

    /**
     * @brief Serialize terrain grid with water data to buffer.
     *
     * Writes header followed by tile data, water body IDs, and flow directions.
     * All multi-byte values are written in little-endian format.
     *
     * @param buffer Output buffer to write to.
     * @param grid The terrain grid to serialize.
     * @param waterData Water body IDs and flow directions.
     * @param mapSeed Map generation seed.
     */
    void serialize(
        WriteBuffer& buffer,
        const TerrainGrid& grid,
        const WaterData& waterData,
        std::uint32_t mapSeed
    ) const;

    /**
     * @brief Deserialize terrain grid with water data from buffer.
     *
     * Reads header and validates version/dimensions before loading tile data.
     * Grid and water data are resized to match the dimensions in the header.
     *
     * @param buffer Input buffer to read from.
     * @param grid Output terrain grid (will be resized).
     * @param waterData Output water data (will be resized).
     * @param mapSeed Output map seed.
     * @return Result code indicating success or error type.
     */
    TerrainSerializeResult deserialize(
        ReadBuffer& buffer,
        TerrainGrid& grid,
        WaterData& waterData,
        std::uint32_t& mapSeed
    ) const;

    /**
     * @brief Validate header without deserializing full data.
     *
     * Useful for checking if data can be loaded before allocating.
     *
     * @param buffer Input buffer to read header from.
     * @param header Output header structure.
     * @return Result code indicating if header is valid.
     */
    TerrainSerializeResult validateHeader(
        ReadBuffer& buffer,
        TerrainGridHeader& header
    ) const;

private:
    /**
     * @brief Write header to buffer.
     */
    void writeHeader(
        WriteBuffer& buffer,
        const TerrainGridHeader& header
    ) const;

    /**
     * @brief Read header from buffer.
     */
    bool readHeader(
        ReadBuffer& buffer,
        TerrainGridHeader& header
    ) const;

    /**
     * @brief Write all tile data to buffer.
     */
    void writeTiles(
        WriteBuffer& buffer,
        const TerrainGrid& grid
    ) const;

    /**
     * @brief Read all tile data from buffer.
     */
    bool readTiles(
        ReadBuffer& buffer,
        TerrainGrid& grid
    ) const;

    /**
     * @brief Write water body IDs to buffer.
     */
    void writeWaterBodyIds(
        WriteBuffer& buffer,
        const WaterBodyGrid& waterBodyGrid
    ) const;

    /**
     * @brief Read water body IDs from buffer.
     */
    bool readWaterBodyIds(
        ReadBuffer& buffer,
        WaterBodyGrid& waterBodyGrid
    ) const;

    /**
     * @brief Write flow directions to buffer.
     */
    void writeFlowDirections(
        WriteBuffer& buffer,
        const FlowDirectionGrid& flowGrid
    ) const;

    /**
     * @brief Read flow directions from buffer.
     */
    bool readFlowDirections(
        ReadBuffer& buffer,
        FlowDirectionGrid& flowGrid
    ) const;
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINGRIDSERIALIZER_H
