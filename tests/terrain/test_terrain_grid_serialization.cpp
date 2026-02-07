/**
 * @file test_terrain_grid_serialization.cpp
 * @brief Unit tests for TerrainGridSerializer (Ticket 3-036).
 *
 * Tests:
 * 1. TerrainGrid implements ISerializable with version field
 * 2. Serialization includes all required data
 * 3. Fixed-size types with little-endian encoding
 * 4. Deserialization validates version and data integrity
 * 5. Round-trip test: serialize -> deserialize -> binary compare
 * 6. Uncompressed size matches expected formula
 * 7. Format extensibility via version field
 */

#include <sims3000/terrain/TerrainGridSerializer.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/WaterData.h>
#include <sims3000/core/Serialization.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>

using namespace sims3000;
using namespace sims3000::terrain;

// Test counter
static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(name) \
    void test_##name(); \
    struct TestRunner_##name { \
        TestRunner_##name() { \
            std::cout << "Running " #name "... "; \
            try { \
                test_##name(); \
                std::cout << "PASSED" << std::endl; \
                ++g_testsPassed; \
            } catch (const std::exception& e) { \
                std::cout << "FAILED: " << e.what() << std::endl; \
                ++g_testsFailed; \
            } catch (...) { \
                std::cout << "FAILED: Unknown exception" << std::endl; \
                ++g_testsFailed; \
            } \
        } \
    } g_runner_##name; \
    void test_##name()

#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            throw std::runtime_error("Assertion failed: " #cond); \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            throw std::runtime_error("Assertion failed: " #a " == " #b); \
        } \
    } while(0)

// =============================================================================
// Test: Header size is exactly 12 bytes
// =============================================================================
TEST(HeaderSizeIs12Bytes) {
    ASSERT_EQ(sizeof(TerrainGridHeader), 12u);
}

// =============================================================================
// Test: TerrainComponent size is 4 bytes
// =============================================================================
TEST(TerrainComponentSizeIs4Bytes) {
    ASSERT_EQ(sizeof(TerrainComponent), 4u);
}

// =============================================================================
// Test: WaterBodyID size is 2 bytes
// =============================================================================
TEST(WaterBodyIdSizeIs2Bytes) {
    ASSERT_EQ(sizeof(WaterBodyID), 2u);
}

// =============================================================================
// Test: FlowDirection size is 1 byte
// =============================================================================
TEST(FlowDirectionSizeIs1Byte) {
    ASSERT_EQ(sizeof(FlowDirection), 1u);
}

// =============================================================================
// Test: Calculate serialized size for 128x128
// =============================================================================
TEST(CalculateSerializedSize128x128) {
    // Header: 12 bytes
    // Tiles: 128 * 128 * 4 = 65,536 bytes
    // Water IDs: 128 * 128 * 2 = 32,768 bytes
    // Flow dirs: 128 * 128 * 1 = 16,384 bytes
    // Total: 12 + 65,536 + 32,768 + 16,384 = 114,700 bytes
    std::size_t expected = 12 + (128 * 128 * 4) + (128 * 128 * 2) + (128 * 128 * 1);
    std::size_t actual = TerrainGridSerializer::calculateSerializedSize(128, 128);
    ASSERT_EQ(actual, expected);
    ASSERT_EQ(actual, 114700u);
}

// =============================================================================
// Test: Calculate serialized size for 256x256
// =============================================================================
TEST(CalculateSerializedSize256x256) {
    std::size_t expected = 12 + (256 * 256 * 4) + (256 * 256 * 2) + (256 * 256 * 1);
    std::size_t actual = TerrainGridSerializer::calculateSerializedSize(256, 256);
    ASSERT_EQ(actual, expected);
    ASSERT_EQ(actual, 458764u);
}

// =============================================================================
// Test: Calculate serialized size for 512x512
// =============================================================================
TEST(CalculateSerializedSize512x512) {
    // Header: 12 bytes
    // Tiles: 512 * 512 * 4 = 1,048,576 bytes
    // Water IDs: 512 * 512 * 2 = 524,288 bytes
    // Flow dirs: 512 * 512 * 1 = 262,144 bytes
    // Total: 12 + 1,048,576 + 524,288 + 262,144 = 1,835,020 bytes
    std::size_t expected = 12 + (512 * 512 * 4) + (512 * 512 * 2) + (512 * 512 * 1);
    std::size_t actual = TerrainGridSerializer::calculateSerializedSize(512, 512);
    ASSERT_EQ(actual, expected);
    ASSERT_EQ(actual, 1835020u);
}

// =============================================================================
// Test: Empty grid serialization
// =============================================================================
TEST(EmptyGridSerialization) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);
    const std::uint32_t mapSeed = 12345;

    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, mapSeed);

    std::size_t expectedSize = TerrainGridSerializer::calculateSerializedSize(128, 128);
    ASSERT_EQ(buffer.size(), expectedSize);
}

// =============================================================================
// Test: Round-trip serialization with empty grid
// =============================================================================
TEST(RoundTripEmptyGrid) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);
    const std::uint32_t mapSeed = 42;

    // Serialize
    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, mapSeed);

    // Deserialize
    TerrainGrid loadedGrid;
    WaterData loadedWaterData;
    std::uint32_t loadedSeed = 0;

    ReadBuffer readBuf(buffer.data(), buffer.size());
    TerrainSerializeResult result = serializer.deserialize(readBuf, loadedGrid, loadedWaterData, loadedSeed);

    ASSERT_EQ(static_cast<int>(result), static_cast<int>(TerrainSerializeResult::Success));
    ASSERT_EQ(loadedGrid.width, grid.width);
    ASSERT_EQ(loadedGrid.height, grid.height);
    ASSERT_EQ(loadedGrid.sea_level, grid.sea_level);
    ASSERT_EQ(loadedSeed, mapSeed);
}

// =============================================================================
// Test: Round-trip with populated terrain data
// =============================================================================
TEST(RoundTripPopulatedTerrain) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);
    const std::uint32_t mapSeed = 999;

    // Populate terrain with varied data
    for (std::size_t i = 0; i < grid.tile_count(); ++i) {
        grid.tiles[i].terrain_type = static_cast<std::uint8_t>(i % 10);
        grid.tiles[i].elevation = static_cast<std::uint8_t>((i * 3) % 32);
        grid.tiles[i].moisture = static_cast<std::uint8_t>(i % 256);
        grid.tiles[i].flags = static_cast<std::uint8_t>((i * 7) % 16);
    }

    // Populate water data
    for (std::size_t i = 0; i < waterData.water_body_ids.tile_count(); ++i) {
        waterData.water_body_ids.body_ids[i] = static_cast<WaterBodyID>(i % 100);
        waterData.flow_directions.directions[i] = static_cast<FlowDirection>(i % 9);
    }

    // Serialize
    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, mapSeed);

    // Deserialize
    TerrainGrid loadedGrid;
    WaterData loadedWaterData;
    std::uint32_t loadedSeed = 0;

    ReadBuffer readBuf(buffer.data(), buffer.size());
    TerrainSerializeResult result = serializer.deserialize(readBuf, loadedGrid, loadedWaterData, loadedSeed);

    ASSERT_EQ(static_cast<int>(result), static_cast<int>(TerrainSerializeResult::Success));

    // Verify terrain data matches
    ASSERT_EQ(loadedGrid.tile_count(), grid.tile_count());
    for (std::size_t i = 0; i < grid.tile_count(); ++i) {
        ASSERT_EQ(loadedGrid.tiles[i].terrain_type, grid.tiles[i].terrain_type);
        ASSERT_EQ(loadedGrid.tiles[i].elevation, grid.tiles[i].elevation);
        ASSERT_EQ(loadedGrid.tiles[i].moisture, grid.tiles[i].moisture);
        ASSERT_EQ(loadedGrid.tiles[i].flags, grid.tiles[i].flags);
    }

    // Verify water data matches
    for (std::size_t i = 0; i < waterData.water_body_ids.tile_count(); ++i) {
        ASSERT_EQ(loadedWaterData.water_body_ids.body_ids[i], waterData.water_body_ids.body_ids[i]);
        ASSERT_EQ(static_cast<int>(loadedWaterData.flow_directions.directions[i]),
                  static_cast<int>(waterData.flow_directions.directions[i]));
    }

    ASSERT_EQ(loadedSeed, mapSeed);
}

// =============================================================================
// Test: Binary compare after round-trip (serialize -> deserialize -> serialize)
// =============================================================================
TEST(BinaryCompareRoundTrip) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);
    const std::uint32_t mapSeed = 7777;

    // Populate with deterministic data
    for (std::size_t i = 0; i < grid.tile_count(); ++i) {
        grid.tiles[i].terrain_type = static_cast<std::uint8_t>((i + 1) % 10);
        grid.tiles[i].elevation = static_cast<std::uint8_t>((i + 2) % 32);
        grid.tiles[i].moisture = static_cast<std::uint8_t>((i + 3) % 256);
        grid.tiles[i].flags = static_cast<std::uint8_t>((i + 4) % 16);
    }

    for (std::size_t i = 0; i < waterData.water_body_ids.tile_count(); ++i) {
        waterData.water_body_ids.body_ids[i] = static_cast<WaterBodyID>((i + 5) % 1000);
        waterData.flow_directions.directions[i] = static_cast<FlowDirection>((i + 6) % 9);
    }

    TerrainGridSerializer serializer;

    // First serialization
    WriteBuffer buffer1;
    serializer.serialize(buffer1, grid, waterData, mapSeed);

    // Deserialize
    TerrainGrid loadedGrid;
    WaterData loadedWaterData;
    std::uint32_t loadedSeed = 0;

    ReadBuffer readBuf(buffer1.data(), buffer1.size());
    TerrainSerializeResult result = serializer.deserialize(readBuf, loadedGrid, loadedWaterData, loadedSeed);
    ASSERT_EQ(static_cast<int>(result), static_cast<int>(TerrainSerializeResult::Success));

    // Second serialization (of loaded data)
    WriteBuffer buffer2;
    serializer.serialize(buffer2, loadedGrid, loadedWaterData, loadedSeed);

    // Binary compare
    ASSERT_EQ(buffer1.size(), buffer2.size());
    ASSERT(std::memcmp(buffer1.data(), buffer2.data(), buffer1.size()) == 0);
}

// =============================================================================
// Test: Version field is written correctly
// =============================================================================
TEST(VersionFieldWrittenCorrectly) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);

    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, 0);

    // Version is first 2 bytes
    const std::uint8_t* data = buffer.data();
    std::uint16_t version = static_cast<std::uint16_t>(data[0]) | (static_cast<std::uint16_t>(data[1]) << 8);
    ASSERT_EQ(version, TERRAIN_GRID_FORMAT_VERSION);
}

// =============================================================================
// Test: Dimensions are written correctly
// =============================================================================
TEST(DimensionsWrittenCorrectly) {
    TerrainGrid grid(MapSize::Medium);
    WaterData waterData(MapSize::Medium);

    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, 0);

    // Width is bytes 2-3, height is bytes 4-5 (little-endian)
    const std::uint8_t* data = buffer.data();
    std::uint16_t width = static_cast<std::uint16_t>(data[2]) | (static_cast<std::uint16_t>(data[3]) << 8);
    std::uint16_t height = static_cast<std::uint16_t>(data[4]) | (static_cast<std::uint16_t>(data[5]) << 8);
    ASSERT_EQ(width, 256u);
    ASSERT_EQ(height, 256u);
}

// =============================================================================
// Test: Sea level is written correctly
// =============================================================================
TEST(SeaLevelWrittenCorrectly) {
    TerrainGrid grid(MapSize::Small, 15);  // Custom sea level
    WaterData waterData(MapSize::Small);

    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, 0);

    // Sea level is byte 6
    const std::uint8_t* data = buffer.data();
    ASSERT_EQ(data[6], 15u);
}

// =============================================================================
// Test: Map seed is written correctly
// =============================================================================
TEST(MapSeedWrittenCorrectly) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);
    const std::uint32_t mapSeed = 0xDEADBEEF;

    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, mapSeed);

    // Map seed is bytes 8-11 (little-endian)
    const std::uint8_t* data = buffer.data();
    std::uint32_t seed = static_cast<std::uint32_t>(data[8]) |
                         (static_cast<std::uint32_t>(data[9]) << 8) |
                         (static_cast<std::uint32_t>(data[10]) << 16) |
                         (static_cast<std::uint32_t>(data[11]) << 24);
    ASSERT_EQ(seed, mapSeed);
}

// =============================================================================
// Test: Invalid version rejection
// =============================================================================
TEST(InvalidVersionRejection) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);

    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, 0);

    // Corrupt version to unsupported value
    std::vector<std::uint8_t> corruptData(buffer.data(), buffer.data() + buffer.size());
    corruptData[0] = 99;  // Invalid version
    corruptData[1] = 0;

    TerrainGrid loadedGrid;
    WaterData loadedWaterData;
    std::uint32_t loadedSeed = 0;

    ReadBuffer readBuf(corruptData.data(), corruptData.size());
    TerrainSerializeResult result = serializer.deserialize(readBuf, loadedGrid, loadedWaterData, loadedSeed);
    ASSERT_EQ(static_cast<int>(result), static_cast<int>(TerrainSerializeResult::InvalidVersion));
}

// =============================================================================
// Test: Invalid dimensions rejection
// =============================================================================
TEST(InvalidDimensionsRejection) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);

    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, 0);

    // Corrupt width to invalid value
    std::vector<std::uint8_t> corruptData(buffer.data(), buffer.data() + buffer.size());
    corruptData[2] = 100;  // Invalid width (not 128, 256, or 512)
    corruptData[3] = 0;

    TerrainGrid loadedGrid;
    WaterData loadedWaterData;
    std::uint32_t loadedSeed = 0;

    ReadBuffer readBuf(corruptData.data(), corruptData.size());
    TerrainSerializeResult result = serializer.deserialize(readBuf, loadedGrid, loadedWaterData, loadedSeed);
    ASSERT_EQ(static_cast<int>(result), static_cast<int>(TerrainSerializeResult::InvalidDimensions));
}

// =============================================================================
// Test: Insufficient data rejection
// =============================================================================
TEST(InsufficientDataRejection) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);

    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, 0);

    // Truncate data
    std::vector<std::uint8_t> truncatedData(buffer.data(), buffer.data() + 100);  // Only 100 bytes

    TerrainGrid loadedGrid;
    WaterData loadedWaterData;
    std::uint32_t loadedSeed = 0;

    ReadBuffer readBuf(truncatedData.data(), truncatedData.size());
    TerrainSerializeResult result = serializer.deserialize(readBuf, loadedGrid, loadedWaterData, loadedSeed);
    ASSERT_EQ(static_cast<int>(result), static_cast<int>(TerrainSerializeResult::InsufficientData));
}

// =============================================================================
// Test: Header validation without full deserialization
// =============================================================================
TEST(HeaderValidation) {
    TerrainGrid grid(MapSize::Large, 10);
    WaterData waterData(MapSize::Large);
    const std::uint32_t mapSeed = 54321;

    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, mapSeed);

    // Validate header only
    TerrainGridHeader header{};
    ReadBuffer readBuf(buffer.data(), buffer.size());
    TerrainSerializeResult result = serializer.validateHeader(readBuf, header);

    ASSERT_EQ(static_cast<int>(result), static_cast<int>(TerrainSerializeResult::Success));
    ASSERT_EQ(header.version, TERRAIN_GRID_FORMAT_VERSION);
    ASSERT_EQ(header.width, 512u);
    ASSERT_EQ(header.height, 512u);
    ASSERT_EQ(header.sea_level, 10u);
    ASSERT_EQ(header.map_seed, mapSeed);
}

// =============================================================================
// Test: All map sizes serialize/deserialize correctly
// =============================================================================
TEST(AllMapSizes) {
    TerrainGridSerializer serializer;

    // Test all three map sizes
    MapSize sizes[] = { MapSize::Small, MapSize::Medium, MapSize::Large };
    std::uint16_t dims[] = { 128, 256, 512 };

    for (int i = 0; i < 3; ++i) {
        TerrainGrid grid(sizes[i]);
        WaterData waterData(sizes[i]);
        const std::uint32_t mapSeed = 1000 + i;

        // Populate some data
        grid.tiles[0].terrain_type = static_cast<std::uint8_t>(i);
        grid.tiles[0].elevation = static_cast<std::uint8_t>(i + 10);
        waterData.water_body_ids.body_ids[0] = static_cast<WaterBodyID>(i + 100);
        waterData.flow_directions.directions[0] = static_cast<FlowDirection>(i + 1);

        // Serialize
        WriteBuffer buffer;
        serializer.serialize(buffer, grid, waterData, mapSeed);

        // Verify size
        std::size_t expectedSize = TerrainGridSerializer::calculateSerializedSize(dims[i], dims[i]);
        ASSERT_EQ(buffer.size(), expectedSize);

        // Deserialize
        TerrainGrid loadedGrid;
        WaterData loadedWaterData;
        std::uint32_t loadedSeed = 0;

        ReadBuffer readBuf(buffer.data(), buffer.size());
        TerrainSerializeResult result = serializer.deserialize(readBuf, loadedGrid, loadedWaterData, loadedSeed);

        ASSERT_EQ(static_cast<int>(result), static_cast<int>(TerrainSerializeResult::Success));
        ASSERT_EQ(loadedGrid.width, dims[i]);
        ASSERT_EQ(loadedGrid.height, dims[i]);
        ASSERT_EQ(loadedGrid.tiles[0].terrain_type, grid.tiles[0].terrain_type);
        ASSERT_EQ(loadedGrid.tiles[0].elevation, grid.tiles[0].elevation);
        ASSERT_EQ(loadedWaterData.water_body_ids.body_ids[0], waterData.water_body_ids.body_ids[0]);
        ASSERT_EQ(static_cast<int>(loadedWaterData.flow_directions.directions[0]),
                  static_cast<int>(waterData.flow_directions.directions[0]));
        ASSERT_EQ(loadedSeed, mapSeed);
    }
}

// =============================================================================
// Test: FlowDirection values are preserved
// =============================================================================
TEST(FlowDirectionPreserved) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);

    // Set each tile to a different flow direction
    for (std::size_t i = 0; i < waterData.flow_directions.tile_count(); ++i) {
        waterData.flow_directions.directions[i] = static_cast<FlowDirection>(i % 9);
    }

    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, 0);

    TerrainGrid loadedGrid;
    WaterData loadedWaterData;
    std::uint32_t loadedSeed = 0;

    ReadBuffer readBuf(buffer.data(), buffer.size());
    TerrainSerializeResult result = serializer.deserialize(readBuf, loadedGrid, loadedWaterData, loadedSeed);

    ASSERT_EQ(static_cast<int>(result), static_cast<int>(TerrainSerializeResult::Success));

    for (std::size_t i = 0; i < waterData.flow_directions.tile_count(); ++i) {
        ASSERT_EQ(static_cast<int>(loadedWaterData.flow_directions.directions[i]),
                  static_cast<int>(waterData.flow_directions.directions[i]));
    }
}

// =============================================================================
// Test: Water body IDs up to MAX_WATER_BODY_ID are preserved
// =============================================================================
TEST(WaterBodyIdMaxValue) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);

    // Set some tiles to max value
    waterData.water_body_ids.body_ids[0] = MAX_WATER_BODY_ID;  // 65535
    waterData.water_body_ids.body_ids[1] = NO_WATER_BODY;      // 0
    waterData.water_body_ids.body_ids[2] = 32768;              // Mid value

    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, 0);

    TerrainGrid loadedGrid;
    WaterData loadedWaterData;
    std::uint32_t loadedSeed = 0;

    ReadBuffer readBuf(buffer.data(), buffer.size());
    TerrainSerializeResult result = serializer.deserialize(readBuf, loadedGrid, loadedWaterData, loadedSeed);

    ASSERT_EQ(static_cast<int>(result), static_cast<int>(TerrainSerializeResult::Success));
    ASSERT_EQ(loadedWaterData.water_body_ids.body_ids[0], MAX_WATER_BODY_ID);
    ASSERT_EQ(loadedWaterData.water_body_ids.body_ids[1], NO_WATER_BODY);
    ASSERT_EQ(loadedWaterData.water_body_ids.body_ids[2], 32768u);
}

// =============================================================================
// Test: Expected size formula: width * height * 7 + header
// =============================================================================
TEST(ExpectedSizeFormula) {
    // Formula from ticket: width * height * (4 + 2 + 1) bytes + header
    // 4 bytes = TerrainComponent
    // 2 bytes = WaterBodyID
    // 1 byte = FlowDirection
    // Header = 12 bytes

    // 512x512: 512 * 512 * 7 = 1,835,008 + 12 = 1,835,020 bytes
    std::size_t expected512 = 512 * 512 * 7 + 12;
    std::size_t actual512 = TerrainGridSerializer::calculateSerializedSize(512, 512);
    ASSERT_EQ(actual512, expected512);

    // Verify ~1MB uncompressed for 512x512 (as stated in ticket)
    ASSERT(actual512 > 1800000 && actual512 < 1900000);
}

// =============================================================================
// Test: Reserved byte is zeroed
// =============================================================================
TEST(ReservedByteZeroed) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);

    TerrainGridSerializer serializer;
    WriteBuffer buffer;
    serializer.serialize(buffer, grid, waterData, 0);

    // Reserved is byte 7
    const std::uint8_t* data = buffer.data();
    ASSERT_EQ(data[7], 0u);
}

// =============================================================================
// Test: TerrainGrid implements ISerializable interface (Criterion 1)
// =============================================================================
TEST(TerrainGridImplementsISerializable) {
    // Verify TerrainGrid can be used as ISerializable
    TerrainGrid grid(MapSize::Small, 12);

    // Populate with some test data
    for (std::size_t i = 0; i < grid.tile_count(); ++i) {
        grid.tiles[i].terrain_type = static_cast<std::uint8_t>(i % 10);
        grid.tiles[i].elevation = static_cast<std::uint8_t>((i * 2) % 32);
        grid.tiles[i].moisture = static_cast<std::uint8_t>(i % 256);
        grid.tiles[i].flags = static_cast<std::uint8_t>((i * 5) % 16);
    }

    // Use through ISerializable interface
    ISerializable* serializable = &grid;
    WriteBuffer buffer;
    serializable->serialize(buffer);

    // Verify version field is written correctly (first 2 bytes)
    const std::uint8_t* data = buffer.data();
    std::uint16_t version = static_cast<std::uint16_t>(data[0]) | (static_cast<std::uint16_t>(data[1]) << 8);
    ASSERT_EQ(version, TERRAIN_GRID_VERSION);

    // Verify dimensions are written (bytes 2-5)
    std::uint16_t width = static_cast<std::uint16_t>(data[2]) | (static_cast<std::uint16_t>(data[3]) << 8);
    std::uint16_t height = static_cast<std::uint16_t>(data[4]) | (static_cast<std::uint16_t>(data[5]) << 8);
    ASSERT_EQ(width, 128u);
    ASSERT_EQ(height, 128u);

    // Verify sea level (byte 6)
    ASSERT_EQ(data[6], 12u);

    // Deserialize through ISerializable interface
    TerrainGrid loadedGrid;
    ISerializable* loadableSerializable = &loadedGrid;
    ReadBuffer readBuf(buffer.data(), buffer.size());
    loadableSerializable->deserialize(readBuf);

    // Verify loaded data matches
    ASSERT_EQ(loadedGrid.width, grid.width);
    ASSERT_EQ(loadedGrid.height, grid.height);
    ASSERT_EQ(loadedGrid.sea_level, grid.sea_level);
    ASSERT_EQ(loadedGrid.tile_count(), grid.tile_count());

    for (std::size_t i = 0; i < grid.tile_count(); ++i) {
        ASSERT_EQ(loadedGrid.tiles[i].terrain_type, grid.tiles[i].terrain_type);
        ASSERT_EQ(loadedGrid.tiles[i].elevation, grid.tiles[i].elevation);
        ASSERT_EQ(loadedGrid.tiles[i].moisture, grid.tiles[i].moisture);
        ASSERT_EQ(loadedGrid.tiles[i].flags, grid.tiles[i].flags);
    }
}

// =============================================================================
// Test: TerrainGrid ISerializable version field works correctly
// =============================================================================
TEST(TerrainGridISerializableVersionField) {
    // Verify the static version accessor
    ASSERT_EQ(TerrainGrid::getFormatVersion(), TERRAIN_GRID_VERSION);
    ASSERT_EQ(TerrainGrid::getFormatVersion(), 1u);
}

// =============================================================================
// Test: TerrainGrid ISerializable round-trip preserves all data
// =============================================================================
TEST(TerrainGridISerializableRoundTrip) {
    // Test all map sizes through ISerializable interface
    MapSize sizes[] = { MapSize::Small, MapSize::Medium, MapSize::Large };

    for (int i = 0; i < 3; ++i) {
        TerrainGrid grid(sizes[i], static_cast<std::uint8_t>(5 + i));

        // Populate with deterministic data
        for (std::size_t j = 0; j < grid.tile_count(); ++j) {
            grid.tiles[j].terrain_type = static_cast<std::uint8_t>((j + i) % 10);
            grid.tiles[j].elevation = static_cast<std::uint8_t>((j + i * 3) % 32);
            grid.tiles[j].moisture = static_cast<std::uint8_t>((j + i * 7) % 256);
            grid.tiles[j].flags = static_cast<std::uint8_t>((j + i * 11) % 16);
        }

        // Serialize via ISerializable
        WriteBuffer buffer;
        grid.serialize(buffer);

        // Deserialize via ISerializable
        TerrainGrid loadedGrid;
        ReadBuffer readBuf(buffer.data(), buffer.size());
        loadedGrid.deserialize(readBuf);

        // Verify all data matches
        ASSERT_EQ(loadedGrid.width, grid.width);
        ASSERT_EQ(loadedGrid.height, grid.height);
        ASSERT_EQ(loadedGrid.sea_level, grid.sea_level);

        for (std::size_t j = 0; j < grid.tile_count(); ++j) {
            ASSERT_EQ(loadedGrid.tiles[j].terrain_type, grid.tiles[j].terrain_type);
            ASSERT_EQ(loadedGrid.tiles[j].elevation, grid.tiles[j].elevation);
            ASSERT_EQ(loadedGrid.tiles[j].moisture, grid.tiles[j].moisture);
            ASSERT_EQ(loadedGrid.tiles[j].flags, grid.tiles[j].flags);
        }
    }
}

// =============================================================================
// Test: TerrainGrid ISerializable handles invalid version gracefully
// =============================================================================
TEST(TerrainGridISerializableInvalidVersion) {
    TerrainGrid grid(MapSize::Small);

    // Serialize
    WriteBuffer buffer;
    grid.serialize(buffer);

    // Corrupt version to an unsupported value
    std::vector<std::uint8_t> corruptData(buffer.data(), buffer.data() + buffer.size());
    corruptData[0] = 99;  // Invalid version
    corruptData[1] = 0;

    // Deserialize should leave grid empty
    TerrainGrid loadedGrid;
    ReadBuffer readBuf(corruptData.data(), corruptData.size());
    loadedGrid.deserialize(readBuf);

    // Grid should be in empty state
    ASSERT_EQ(loadedGrid.width, 0u);
    ASSERT_EQ(loadedGrid.height, 0u);
    ASSERT(loadedGrid.tiles.empty());
}

// =============================================================================
// Main
// =============================================================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "TerrainGridSerializer Tests (3-036)" << std::endl;
    std::cout << "========================================" << std::endl;

    // Tests are auto-registered and run via static initialization

    std::cout << "========================================" << std::endl;
    std::cout << "Results: " << g_testsPassed << " passed, " << g_testsFailed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return g_testsFailed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
