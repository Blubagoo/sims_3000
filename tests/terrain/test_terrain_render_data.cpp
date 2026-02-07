/**
 * @file test_terrain_render_data.cpp
 * @brief Tests for ITerrainRenderData interface (Ticket 3-016)
 *
 * Tests the data contract between TerrainSystem and RenderingSystem:
 * - Interface method signatures
 * - Mock implementation for verification
 * - Integration with TerrainGrid, TerrainTypeInfo, ChunkDirtyTracker, WaterData
 */

#include <sims3000/terrain/ITerrainRenderData.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainTypeInfo.h>
#include <sims3000/terrain/ChunkDirtyTracker.h>
#include <sims3000/terrain/WaterData.h>

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cmath>

using namespace sims3000::terrain;

// =============================================================================
// Mock Implementation for Testing
// =============================================================================

/**
 * @class MockTerrainRenderData
 * @brief Test implementation of ITerrainRenderData interface.
 *
 * Implements all interface methods using real terrain data structures.
 * This verifies the interface design works with actual dependencies.
 */
class MockTerrainRenderData : public ITerrainRenderData {
public:
    MockTerrainRenderData(MapSize size)
        : grid_(size)
        , water_data_(size)
        , chunk_tracker_(static_cast<std::uint16_t>(size),
                         static_cast<std::uint16_t>(size))
    {
        // Initialize with some default data
        grid_.fill_type(TerrainType::Substrate);
    }

    // Grid access
    const TerrainGrid& get_grid() const override {
        return grid_;
    }

    // Type info lookup
    const TerrainTypeInfo& get_type_info(TerrainType type) const override {
        return getTerrainInfo(type);
    }

    // Dirty chunk tracking
    bool is_chunk_dirty(std::uint32_t chunk_x, std::uint32_t chunk_y) const override {
        return chunk_tracker_.isChunkDirty(
            static_cast<std::uint16_t>(chunk_x),
            static_cast<std::uint16_t>(chunk_y)
        );
    }

    void clear_chunk_dirty(std::uint32_t chunk_x, std::uint32_t chunk_y) override {
        chunk_tracker_.clearChunkDirty(
            static_cast<std::uint16_t>(chunk_x),
            static_cast<std::uint16_t>(chunk_y)
        );
    }

    std::uint32_t get_chunk_size() const override {
        return TERRAIN_CHUNK_SIZE;
    }

    // Water body queries
    WaterBodyID get_water_body_id(std::int32_t x, std::int32_t y) const override {
        if (!water_data_.in_bounds(x, y)) {
            return NO_WATER_BODY;
        }
        return water_data_.get_water_body_id(x, y);
    }

    // Flow direction queries
    FlowDirection get_flow_direction(std::int32_t x, std::int32_t y) const override {
        if (!water_data_.in_bounds(x, y)) {
            return FlowDirection::None;
        }
        return water_data_.get_flow_direction(x, y);
    }

    // Map metadata
    std::uint32_t get_map_width() const override {
        return grid_.width;
    }

    std::uint32_t get_map_height() const override {
        return grid_.height;
    }

    std::uint32_t get_chunks_x() const override {
        return chunk_tracker_.getChunksX();
    }

    std::uint32_t get_chunks_y() const override {
        return chunk_tracker_.getChunksY();
    }

    // Test helpers (not part of interface)
    TerrainGrid& get_mutable_grid() { return grid_; }
    WaterData& get_mutable_water_data() { return water_data_; }
    ChunkDirtyTracker& get_mutable_tracker() { return chunk_tracker_; }

private:
    TerrainGrid grid_;
    WaterData water_data_;
    mutable ChunkDirtyTracker chunk_tracker_;  // mutable for clear_chunk_dirty
};

// =============================================================================
// Test Utilities
// =============================================================================

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("  Running test_%s... ", #name); \
    tests_run++; \
    test_##name(); \
    tests_passed++; \
    printf("PASSED\n"); \
} while(0)

// =============================================================================
// Tests: Interface Existence and Method Signatures
// =============================================================================

TEST(interface_exists) {
    // Verify ITerrainRenderData can be instantiated through mock
    MockTerrainRenderData mock(MapSize::Small);
    ITerrainRenderData* interface_ptr = &mock;

    // Verify we can call methods through interface pointer
    assert(interface_ptr != nullptr);
    (void)interface_ptr->get_grid();
    (void)interface_ptr->get_type_info(TerrainType::Substrate);
}

TEST(get_grid_returns_const_reference) {
    MockTerrainRenderData mock(MapSize::Small);
    const ITerrainRenderData& iface = mock;

    const TerrainGrid& grid = iface.get_grid();

    // Verify grid properties
    assert(grid.width == 128);
    assert(grid.height == 128);
    assert(!grid.empty());
}

TEST(get_type_info_returns_valid_info) {
    MockTerrainRenderData mock(MapSize::Small);
    const ITerrainRenderData& iface = mock;

    // Test each terrain type
    for (std::uint8_t i = 0; i < TERRAIN_TYPE_COUNT; ++i) {
        TerrainType type = static_cast<TerrainType>(i);
        const TerrainTypeInfo& info = iface.get_type_info(type);

        // Verify emissive intensity is in valid range
        assert(info.emissive_intensity >= 0.0f);
        assert(info.emissive_intensity <= 1.0f);

        // Verify emissive color is normalized
        assert(info.emissive_color.x >= 0.0f && info.emissive_color.x <= 1.0f);
        assert(info.emissive_color.y >= 0.0f && info.emissive_color.y <= 1.0f);
        assert(info.emissive_color.z >= 0.0f && info.emissive_color.z <= 1.0f);
    }
}

// =============================================================================
// Tests: Dirty Chunk Tracking
// =============================================================================

TEST(is_chunk_dirty_default_false) {
    MockTerrainRenderData mock(MapSize::Small);
    const ITerrainRenderData& iface = mock;

    // All chunks should start clean after initialization
    for (std::uint32_t y = 0; y < iface.get_chunks_y(); ++y) {
        for (std::uint32_t x = 0; x < iface.get_chunks_x(); ++x) {
            assert(!iface.is_chunk_dirty(x, y));
        }
    }
}

TEST(is_chunk_dirty_out_of_bounds_returns_false) {
    MockTerrainRenderData mock(MapSize::Small);
    const ITerrainRenderData& iface = mock;

    // Out of bounds should return false, not crash
    assert(!iface.is_chunk_dirty(100, 100));
    assert(!iface.is_chunk_dirty(1000, 0));
    assert(!iface.is_chunk_dirty(0, 1000));
}

TEST(clear_chunk_dirty_works) {
    MockTerrainRenderData mock(MapSize::Small);
    ITerrainRenderData& iface = mock;

    // Mark a chunk dirty
    mock.get_mutable_tracker().markChunkDirty(1, 2);
    assert(iface.is_chunk_dirty(1, 2));

    // Clear it
    iface.clear_chunk_dirty(1, 2);
    assert(!iface.is_chunk_dirty(1, 2));
}

TEST(get_chunk_size_returns_32) {
    MockTerrainRenderData mock(MapSize::Small);
    const ITerrainRenderData& iface = mock;

    assert(iface.get_chunk_size() == 32);
    assert(iface.get_chunk_size() == TERRAIN_CHUNK_SIZE);
}

// =============================================================================
// Tests: Water Body Queries
// =============================================================================

TEST(get_water_body_id_default_no_water) {
    MockTerrainRenderData mock(MapSize::Small);
    const ITerrainRenderData& iface = mock;

    // All tiles should start with no water body
    assert(iface.get_water_body_id(0, 0) == NO_WATER_BODY);
    assert(iface.get_water_body_id(64, 64) == NO_WATER_BODY);
}

TEST(get_water_body_id_out_of_bounds_returns_zero) {
    MockTerrainRenderData mock(MapSize::Small);
    const ITerrainRenderData& iface = mock;

    // Out of bounds should return NO_WATER_BODY (0)
    assert(iface.get_water_body_id(-1, 0) == NO_WATER_BODY);
    assert(iface.get_water_body_id(0, -1) == NO_WATER_BODY);
    assert(iface.get_water_body_id(200, 0) == NO_WATER_BODY);
    assert(iface.get_water_body_id(0, 200) == NO_WATER_BODY);
}

TEST(get_water_body_id_returns_set_value) {
    MockTerrainRenderData mock(MapSize::Small);

    // Set a water body ID
    mock.get_mutable_water_data().set_water_body_id(10, 20, 42);

    const ITerrainRenderData& iface = mock;
    assert(iface.get_water_body_id(10, 20) == 42);
}

// =============================================================================
// Tests: Flow Direction Queries
// =============================================================================

TEST(get_flow_direction_default_none) {
    MockTerrainRenderData mock(MapSize::Small);
    const ITerrainRenderData& iface = mock;

    // All tiles should start with no flow direction
    assert(iface.get_flow_direction(0, 0) == FlowDirection::None);
    assert(iface.get_flow_direction(64, 64) == FlowDirection::None);
}

TEST(get_flow_direction_out_of_bounds_returns_none) {
    MockTerrainRenderData mock(MapSize::Small);
    const ITerrainRenderData& iface = mock;

    // Out of bounds should return FlowDirection::None
    assert(iface.get_flow_direction(-1, 0) == FlowDirection::None);
    assert(iface.get_flow_direction(0, -1) == FlowDirection::None);
    assert(iface.get_flow_direction(200, 0) == FlowDirection::None);
    assert(iface.get_flow_direction(0, 200) == FlowDirection::None);
}

TEST(get_flow_direction_returns_set_value) {
    MockTerrainRenderData mock(MapSize::Small);

    // Set flow directions
    mock.get_mutable_water_data().set_flow_direction(5, 10, FlowDirection::E);
    mock.get_mutable_water_data().set_flow_direction(6, 10, FlowDirection::SE);

    const ITerrainRenderData& iface = mock;
    assert(iface.get_flow_direction(5, 10) == FlowDirection::E);
    assert(iface.get_flow_direction(6, 10) == FlowDirection::SE);
}

// =============================================================================
// Tests: Map Metadata
// =============================================================================

TEST(map_dimensions_small) {
    MockTerrainRenderData mock(MapSize::Small);
    const ITerrainRenderData& iface = mock;

    assert(iface.get_map_width() == 128);
    assert(iface.get_map_height() == 128);
    assert(iface.get_chunks_x() == 4);   // 128 / 32 = 4
    assert(iface.get_chunks_y() == 4);
}

TEST(map_dimensions_medium) {
    MockTerrainRenderData mock(MapSize::Medium);
    const ITerrainRenderData& iface = mock;

    assert(iface.get_map_width() == 256);
    assert(iface.get_map_height() == 256);
    assert(iface.get_chunks_x() == 8);   // 256 / 32 = 8
    assert(iface.get_chunks_y() == 8);
}

TEST(map_dimensions_large) {
    MockTerrainRenderData mock(MapSize::Large);
    const ITerrainRenderData& iface = mock;

    assert(iface.get_map_width() == 512);
    assert(iface.get_map_height() == 512);
    assert(iface.get_chunks_x() == 16);  // 512 / 32 = 16
    assert(iface.get_chunks_y() == 16);
}

// =============================================================================
// Tests: Integration - RenderingSystem Usage Pattern
// =============================================================================

TEST(integration_rendering_system_pattern) {
    // Simulate how RenderingSystem would use the interface
    MockTerrainRenderData mock(MapSize::Small);

    // Set up some terrain data
    mock.get_mutable_grid().at(50, 60).setTerrainType(TerrainType::FlowChannel);
    mock.get_mutable_grid().at(50, 60).setElevation(5);
    mock.get_mutable_water_data().set_water_body_id(50, 60, 1);
    mock.get_mutable_water_data().set_flow_direction(50, 60, FlowDirection::S);
    mock.get_mutable_tracker().markChunkDirty(1, 1);

    // Access through interface (as RenderingSystem would)
    ITerrainRenderData& iface = mock;

    // Check for dirty chunks
    std::uint32_t dirty_count = 0;
    for (std::uint32_t cy = 0; cy < iface.get_chunks_y(); ++cy) {
        for (std::uint32_t cx = 0; cx < iface.get_chunks_x(); ++cx) {
            if (iface.is_chunk_dirty(cx, cy)) {
                dirty_count++;

                // Simulate mesh rebuild...
                // Access grid data for the chunk
                const TerrainGrid& grid = iface.get_grid();
                std::uint32_t start_x = cx * iface.get_chunk_size();
                std::uint32_t start_y = cy * iface.get_chunk_size();
                std::uint32_t end_x = start_x + iface.get_chunk_size();
                std::uint32_t end_y = start_y + iface.get_chunk_size();

                for (std::uint32_t y = start_y; y < end_y && y < iface.get_map_height(); ++y) {
                    for (std::uint32_t x = start_x; x < end_x && x < iface.get_map_width(); ++x) {
                        const TerrainComponent& tile = grid.at(x, y);
                        TerrainType type = tile.getTerrainType();
                        const TerrainTypeInfo& info = iface.get_type_info(type);

                        // Use emissive properties for mesh generation
                        (void)info.emissive_color;
                        (void)info.emissive_intensity;

                        // Check water properties
                        WaterBodyID body_id = iface.get_water_body_id(x, y);
                        FlowDirection flow = iface.get_flow_direction(x, y);
                        (void)body_id;
                        (void)flow;
                    }
                }

                // Clear dirty flag after rebuild
                iface.clear_chunk_dirty(cx, cy);
            }
        }
    }

    assert(dirty_count == 1);  // We marked one chunk dirty

    // Verify chunk is no longer dirty
    assert(!iface.is_chunk_dirty(1, 1));
}

TEST(integration_terrain_tile_read_access) {
    MockTerrainRenderData mock(MapSize::Small);

    // Set up various terrain types
    mock.get_mutable_grid().at(10, 10).setTerrainType(TerrainType::BiolumeGrove);
    mock.get_mutable_grid().at(10, 10).setElevation(15);
    mock.get_mutable_grid().at(20, 20).setTerrainType(TerrainType::PrismaFields);
    mock.get_mutable_grid().at(20, 20).setElevation(8);

    const ITerrainRenderData& iface = mock;

    // Read terrain through interface
    const TerrainGrid& grid = iface.get_grid();

    // Verify BiolumeGrove tile
    const TerrainComponent& grove_tile = grid.at(10, 10);
    assert(grove_tile.getTerrainType() == TerrainType::BiolumeGrove);
    assert(grove_tile.getElevation() == 15);

    const TerrainTypeInfo& grove_info = iface.get_type_info(TerrainType::BiolumeGrove);
    assert(grove_info.emissive_intensity > 0.0f);  // BiolumeGrove has glow

    // Verify PrismaFields tile
    const TerrainComponent& crystal_tile = grid.at(20, 20);
    assert(crystal_tile.getTerrainType() == TerrainType::PrismaFields);

    const TerrainTypeInfo& crystal_info = iface.get_type_info(TerrainType::PrismaFields);
    assert(crystal_info.emissive_intensity == 0.60f);  // Max terrain glow
}

TEST(const_correctness_enforced) {
    MockTerrainRenderData mock(MapSize::Small);
    const ITerrainRenderData& const_iface = mock;

    // All these should compile (const access)
    (void)const_iface.get_grid();
    (void)const_iface.get_type_info(TerrainType::Substrate);
    (void)const_iface.is_chunk_dirty(0, 0);
    (void)const_iface.get_chunk_size();
    (void)const_iface.get_water_body_id(0, 0);
    (void)const_iface.get_flow_direction(0, 0);
    (void)const_iface.get_map_width();
    (void)const_iface.get_map_height();
    (void)const_iface.get_chunks_x();
    (void)const_iface.get_chunks_y();

    // Note: clear_chunk_dirty is NOT const - requires mutable access
    // This is intentional design per the interface spec
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    printf("Running ITerrainRenderData tests (Ticket 3-016)...\n\n");

    printf("Interface Existence and Method Signatures:\n");
    RUN_TEST(interface_exists);
    RUN_TEST(get_grid_returns_const_reference);
    RUN_TEST(get_type_info_returns_valid_info);

    printf("\nDirty Chunk Tracking:\n");
    RUN_TEST(is_chunk_dirty_default_false);
    RUN_TEST(is_chunk_dirty_out_of_bounds_returns_false);
    RUN_TEST(clear_chunk_dirty_works);
    RUN_TEST(get_chunk_size_returns_32);

    printf("\nWater Body Queries:\n");
    RUN_TEST(get_water_body_id_default_no_water);
    RUN_TEST(get_water_body_id_out_of_bounds_returns_zero);
    RUN_TEST(get_water_body_id_returns_set_value);

    printf("\nFlow Direction Queries:\n");
    RUN_TEST(get_flow_direction_default_none);
    RUN_TEST(get_flow_direction_out_of_bounds_returns_none);
    RUN_TEST(get_flow_direction_returns_set_value);

    printf("\nMap Metadata:\n");
    RUN_TEST(map_dimensions_small);
    RUN_TEST(map_dimensions_medium);
    RUN_TEST(map_dimensions_large);

    printf("\nIntegration Tests:\n");
    RUN_TEST(integration_rendering_system_pattern);
    RUN_TEST(integration_terrain_tile_read_access);
    RUN_TEST(const_correctness_enforced);

    printf("\n========================================\n");
    printf("Tests run: %d, Tests passed: %d\n", tests_run, tests_passed);

    if (tests_passed == tests_run) {
        printf("ALL TESTS PASSED!\n");
        return 0;
    } else {
        printf("SOME TESTS FAILED!\n");
        return 1;
    }
}
