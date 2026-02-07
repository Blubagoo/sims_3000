/**
 * @file test_terrain_modification_visual_pipeline.cpp
 * @brief Unit tests for TerrainModificationVisualPipeline.
 *
 * Tests the visual update pipeline for terrain modifications:
 * - Event handling and chunk dirty marking
 * - Rate-limited chunk mesh rebuilds
 * - Vegetation instance regeneration
 * - Water mesh regeneration triggers
 * - Query methods for pending updates
 */

#include <sims3000/terrain/TerrainModificationVisualPipeline.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainChunk.h>
#include <sims3000/terrain/WaterData.h>
#include <sims3000/terrain/WaterMesh.h>
#include <iostream>
#include <cassert>
#include <cmath>

using namespace sims3000::terrain;
using namespace sims3000::render;

// ============================================================================
// Test Helpers
// ============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, msg) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << msg << " (line " << __LINE__ << ")" << std::endl; \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define TEST_PASS(name) \
    do { \
        std::cout << "PASS: " << name << std::endl; \
        tests_passed++; \
    } while (0)

// ============================================================================
// Test: Default Construction
// ============================================================================

void test_default_construction() {
    TerrainModificationVisualPipeline pipeline;

    TEST_ASSERT(!pipeline.isInitialized(), "Pipeline should not be initialized after default construction");
    TEST_ASSERT(!pipeline.hasPendingUpdates(), "Should have no pending updates");
    TEST_ASSERT(pipeline.getPendingTerrainChunks() == 0, "Should have 0 pending terrain chunks");
    TEST_ASSERT(pipeline.getPendingVegetationChunks() == 0, "Should have 0 pending vegetation chunks");
    TEST_ASSERT(pipeline.getPendingWaterBodies() == 0, "Should have 0 pending water bodies");

    TEST_PASS("default_construction");
}

// ============================================================================
// Test: Initialization Without GPU (CPU-only tests)
// ============================================================================

void test_grid_rect_to_chunks_calculation() {
    // Test that GridRect affects correct chunks
    // A 256x256 map has 8x8 = 64 chunks (32 tiles per chunk)

    TerrainGrid grid(MapSize::Medium);  // 256x256
    WaterData water_data(MapSize::Medium);

    // Initialize grid with flat terrain
    for (std::size_t i = 0; i < grid.tiles.size(); ++i) {
        grid.tiles[i].terrain_type = static_cast<std::uint8_t>(TerrainType::Substrate);
        grid.tiles[i].setElevation(10);
    }

    // Test single tile modification in first chunk (tile 0,0)
    GridRect rect1 = GridRect::singleTile(0, 0);
    TEST_ASSERT(rect1.x == 0, "Rect1 x should be 0");
    TEST_ASSERT(rect1.y == 0, "Rect1 y should be 0");
    TEST_ASSERT(rect1.width == 1, "Rect1 width should be 1");
    TEST_ASSERT(rect1.height == 1, "Rect1 height should be 1");

    // Test tile at chunk boundary (tile 31,31 is last tile of chunk 0,0)
    GridRect rect2 = GridRect::singleTile(31, 31);
    TEST_ASSERT(rect2.contains(31, 31), "Rect2 should contain (31,31)");

    // Test tile at second chunk (tile 32,0 is first tile of chunk 1,0)
    GridRect rect3 = GridRect::singleTile(32, 0);
    TEST_ASSERT(rect3.x == 32, "Rect3 x should be 32");

    // Test multi-chunk spanning rect
    GridRect rect4 = GridRect::fromCorners(30, 30, 34, 34);
    TEST_ASSERT(rect4.width == 4, "Rect4 width should be 4");
    TEST_ASSERT(rect4.height == 4, "Rect4 height should be 4");

    TEST_PASS("grid_rect_to_chunks_calculation");
}

// ============================================================================
// Test: Event Processing (Without GPU)
// ============================================================================

void test_event_marks_chunks_dirty() {
    // Test that TerrainModifiedEvent correctly marks chunks as dirty

    ChunkDirtyTracker tracker(256, 256);

    // Initially no chunks should be dirty
    TEST_ASSERT(!tracker.hasAnyDirty(), "No chunks should be dirty initially");

    // Create an event for a single tile modification
    TerrainModifiedEvent event1(GridRect::singleTile(16, 16), ModificationType::Leveled);

    // Process the event
    std::uint32_t dirty_count = tracker.processEvent(event1);

    TEST_ASSERT(dirty_count == 1, "Should mark 1 chunk dirty");
    TEST_ASSERT(tracker.isChunkDirty(0, 0), "Chunk (0,0) should be dirty");
    TEST_ASSERT(!tracker.isChunkDirty(1, 0), "Chunk (1,0) should not be dirty");

    // Clear and test multi-chunk spanning event
    tracker.clearAllDirty();

    // Event spanning tiles (30,30) to (34,34) affects chunks (0,0) and (1,1)
    TerrainModifiedEvent event2(GridRect::fromCorners(30, 30, 66, 66), ModificationType::Cleared);
    dirty_count = tracker.processEvent(event2);

    TEST_ASSERT(dirty_count >= 4, "Should mark at least 4 chunks dirty");
    TEST_ASSERT(tracker.isChunkDirty(0, 0), "Chunk (0,0) should be dirty");
    TEST_ASSERT(tracker.isChunkDirty(1, 1), "Chunk (1,1) should be dirty");
    TEST_ASSERT(tracker.isChunkDirty(2, 2), "Chunk (2,2) should be dirty");

    TEST_PASS("event_marks_chunks_dirty");
}

// ============================================================================
// Test: Modification Types
// ============================================================================

void test_modification_types() {
    // Verify all modification types are valid

    TEST_ASSERT(isValidModificationType(static_cast<std::uint8_t>(ModificationType::Cleared)), "Cleared should be valid");
    TEST_ASSERT(isValidModificationType(static_cast<std::uint8_t>(ModificationType::Leveled)), "Leveled should be valid");
    TEST_ASSERT(isValidModificationType(static_cast<std::uint8_t>(ModificationType::Terraformed)), "Terraformed should be valid");
    TEST_ASSERT(isValidModificationType(static_cast<std::uint8_t>(ModificationType::Generated)), "Generated should be valid");
    TEST_ASSERT(isValidModificationType(static_cast<std::uint8_t>(ModificationType::SeaLevelChanged)), "SeaLevelChanged should be valid");
    TEST_ASSERT(!isValidModificationType(5), "5 should not be valid");
    TEST_ASSERT(!isValidModificationType(255), "255 should not be valid");

    TEST_PASS("modification_types");
}

// ============================================================================
// Test: Chunk Dirty Tracker Integration
// ============================================================================

void test_chunk_dirty_tracker_tile_to_chunk_conversion() {
    ChunkDirtyTracker tracker(256, 256);

    // Test tile (0,0) -> chunk (0,0)
    tracker.markTileDirty(0, 0);
    TEST_ASSERT(tracker.isChunkDirty(0, 0), "Tile (0,0) should make chunk (0,0) dirty");
    tracker.clearAllDirty();

    // Test tile (31,31) -> chunk (0,0) (last tile of first chunk)
    tracker.markTileDirty(31, 31);
    TEST_ASSERT(tracker.isChunkDirty(0, 0), "Tile (31,31) should make chunk (0,0) dirty");
    tracker.clearAllDirty();

    // Test tile (32,0) -> chunk (1,0) (first tile of second chunk in X)
    tracker.markTileDirty(32, 0);
    TEST_ASSERT(tracker.isChunkDirty(1, 0), "Tile (32,0) should make chunk (1,0) dirty");
    TEST_ASSERT(!tracker.isChunkDirty(0, 0), "Chunk (0,0) should not be dirty");
    tracker.clearAllDirty();

    // Test tile (0,32) -> chunk (0,1) (first tile of second chunk in Y)
    tracker.markTileDirty(0, 32);
    TEST_ASSERT(tracker.isChunkDirty(0, 1), "Tile (0,32) should make chunk (0,1) dirty");
    TEST_ASSERT(!tracker.isChunkDirty(0, 0), "Chunk (0,0) should not be dirty");

    TEST_PASS("chunk_dirty_tracker_tile_to_chunk_conversion");
}

// ============================================================================
// Test: Clear Operation Updates Chunk
// ============================================================================

void test_clear_operation_marks_dirty() {
    ChunkDirtyTracker tracker(256, 256);

    // Clear operation at tile (100, 100) affects chunk (3, 3)
    TerrainModifiedEvent clear_event(
        GridRect::fromCorners(100, 100, 110, 110),
        ModificationType::Cleared
    );

    std::uint32_t dirty_count = tracker.processEvent(clear_event);

    TEST_ASSERT(dirty_count >= 1, "Clear should mark at least 1 chunk dirty");
    TEST_ASSERT(tracker.isChunkDirty(3, 3), "Chunk (3,3) should be dirty after clear");

    TEST_PASS("clear_operation_marks_dirty");
}

// ============================================================================
// Test: Grade Operation Updates Chunk
// ============================================================================

void test_grade_operation_marks_dirty() {
    ChunkDirtyTracker tracker(256, 256);

    // Grade (elevation change) operation at tile (50, 50) affects chunk (1, 1)
    TerrainModifiedEvent grade_event(
        GridRect::fromCorners(50, 50, 55, 55),
        ModificationType::Leveled
    );

    std::uint32_t dirty_count = tracker.processEvent(grade_event);

    TEST_ASSERT(dirty_count >= 1, "Grade should mark at least 1 chunk dirty");
    TEST_ASSERT(tracker.isChunkDirty(1, 1), "Chunk (1,1) should be dirty after grade");

    TEST_PASS("grade_operation_marks_dirty");
}

// ============================================================================
// Test: Terraform Operation Updates Chunk
// ============================================================================

void test_terraform_operation_marks_dirty() {
    ChunkDirtyTracker tracker(256, 256);

    // Terraform (terrain type change) at tile (200, 200) affects chunk (6, 6)
    TerrainModifiedEvent terraform_event(
        GridRect::fromCorners(200, 200, 205, 205),
        ModificationType::Terraformed
    );

    std::uint32_t dirty_count = tracker.processEvent(terraform_event);

    TEST_ASSERT(dirty_count >= 1, "Terraform should mark at least 1 chunk dirty");
    TEST_ASSERT(tracker.isChunkDirty(6, 6), "Chunk (6,6) should be dirty after terraform");

    TEST_PASS("terraform_operation_marks_dirty");
}

// ============================================================================
// Test: VisualUpdateStats Structure
// ============================================================================

void test_visual_update_stats() {
    VisualUpdateStats stats;

    // Verify default initialization
    TEST_ASSERT(stats.terrain_chunks_rebuilt == 0, "Default terrain_chunks_rebuilt should be 0");
    TEST_ASSERT(stats.terrain_chunks_pending == 0, "Default terrain_chunks_pending should be 0");
    TEST_ASSERT(stats.vegetation_chunks_updated == 0, "Default vegetation_chunks_updated should be 0");
    TEST_ASSERT(stats.vegetation_chunks_pending == 0, "Default vegetation_chunks_pending should be 0");
    TEST_ASSERT(stats.water_bodies_updated == 0, "Default water_bodies_updated should be 0");
    TEST_ASSERT(stats.water_bodies_pending == 0, "Default water_bodies_pending should be 0");
    TEST_ASSERT(stats.update_time_ms == 0.0f, "Default update_time_ms should be 0");

    TEST_PASS("visual_update_stats");
}

// ============================================================================
// Test: Rate Limiting Constants
// ============================================================================

void test_rate_limiting_constants() {
    // Verify rate limiting constants are reasonable
    TEST_ASSERT(TerrainModificationVisualPipeline::MAX_TERRAIN_CHUNKS_PER_FRAME == 1,
                "MAX_TERRAIN_CHUNKS_PER_FRAME should be 1 to avoid GPU stalls");
    TEST_ASSERT(TerrainModificationVisualPipeline::MAX_VEGETATION_CHUNKS_PER_FRAME >= 1,
                "MAX_VEGETATION_CHUNKS_PER_FRAME should be at least 1");
    TEST_ASSERT(TerrainModificationVisualPipeline::MAX_WATER_BODIES_PER_FRAME >= 1,
                "MAX_WATER_BODIES_PER_FRAME should be at least 1");

    TEST_PASS("rate_limiting_constants");
}

// ============================================================================
// Test: Queue Deduplication
// ============================================================================

void test_queue_deduplication() {
    ChunkDirtyTracker tracker(256, 256);

    // Process same event twice
    TerrainModifiedEvent event(GridRect::singleTile(16, 16), ModificationType::Cleared);

    tracker.processEvent(event);
    std::uint32_t count1 = tracker.countDirty();

    tracker.processEvent(event);
    std::uint32_t count2 = tracker.countDirty();

    // Same chunk should only be marked once
    TEST_ASSERT(count1 == count2, "Duplicate events should not increase dirty count");
    TEST_ASSERT(count1 == 1, "Should have exactly 1 dirty chunk");

    TEST_PASS("queue_deduplication");
}

// ============================================================================
// Test: Get Next Dirty Chunk
// ============================================================================

void test_get_next_dirty_chunk() {
    ChunkDirtyTracker tracker(256, 256);

    // Initially no dirty chunks
    std::uint16_t cx, cy;
    TEST_ASSERT(!tracker.getNextDirty(cx, cy), "Should return false when no dirty chunks");

    // Mark a chunk dirty
    tracker.markChunkDirty(3, 5);

    TEST_ASSERT(tracker.getNextDirty(cx, cy), "Should return true when dirty chunks exist");
    TEST_ASSERT(cx == 3, "Dirty chunk X should be 3");
    TEST_ASSERT(cy == 5, "Dirty chunk Y should be 5");

    // Clear and verify
    tracker.clearChunkDirty(3, 5);
    TEST_ASSERT(!tracker.getNextDirty(cx, cy), "Should return false after clearing");

    TEST_PASS("get_next_dirty_chunk");
}

// ============================================================================
// Test: Terrain Event Size Constraints
// ============================================================================

void test_terrain_event_size_constraints() {
    // Verify TerrainModifiedEvent is trivially copyable and correct size
    static_assert(std::is_trivially_copyable<TerrainModifiedEvent>::value,
                  "TerrainModifiedEvent must be trivially copyable");
    static_assert(sizeof(TerrainModifiedEvent) == 12,
                  "TerrainModifiedEvent must be 12 bytes");

    // Verify GridRect size
    static_assert(std::is_trivially_copyable<GridRect>::value,
                  "GridRect must be trivially copyable");
    static_assert(sizeof(GridRect) == 8, "GridRect must be 8 bytes");

    TEST_PASS("terrain_event_size_constraints");
}

// ============================================================================
// Test: Multiple Concurrent Events
// ============================================================================

void test_multiple_concurrent_events() {
    ChunkDirtyTracker tracker(256, 256);

    // Fire multiple events affecting different chunks
    TerrainModifiedEvent event1(GridRect::singleTile(10, 10), ModificationType::Cleared);
    TerrainModifiedEvent event2(GridRect::singleTile(100, 100), ModificationType::Leveled);
    TerrainModifiedEvent event3(GridRect::singleTile(200, 200), ModificationType::Terraformed);

    tracker.processEvent(event1);
    tracker.processEvent(event2);
    tracker.processEvent(event3);

    TEST_ASSERT(tracker.countDirty() == 3, "Should have 3 dirty chunks");
    TEST_ASSERT(tracker.isChunkDirty(0, 0), "Chunk (0,0) should be dirty");
    TEST_ASSERT(tracker.isChunkDirty(3, 3), "Chunk (3,3) should be dirty");
    TEST_ASSERT(tracker.isChunkDirty(6, 6), "Chunk (6,6) should be dirty");

    TEST_PASS("multiple_concurrent_events");
}

// ============================================================================
// Test: Large Area Modification
// ============================================================================

void test_large_area_modification() {
    ChunkDirtyTracker tracker(256, 256);

    // Modify a large area spanning multiple chunks
    // 0,0 to 96,96 should affect chunks (0,0), (0,1), (0,2), (1,0), (1,1), (1,2), (2,0), (2,1), (2,2) = 9 chunks
    TerrainModifiedEvent large_event(
        GridRect::fromCorners(0, 0, 96, 96),
        ModificationType::Generated
    );

    std::uint32_t dirty_count = tracker.processEvent(large_event);

    TEST_ASSERT(dirty_count == 9, "Should mark 9 chunks dirty (3x3 area)");
    TEST_ASSERT(tracker.countDirty() == 9, "Should have 9 dirty chunks total");

    // Verify corners
    TEST_ASSERT(tracker.isChunkDirty(0, 0), "Chunk (0,0) should be dirty");
    TEST_ASSERT(tracker.isChunkDirty(2, 0), "Chunk (2,0) should be dirty");
    TEST_ASSERT(tracker.isChunkDirty(0, 2), "Chunk (0,2) should be dirty");
    TEST_ASSERT(tracker.isChunkDirty(2, 2), "Chunk (2,2) should be dirty");

    TEST_PASS("large_area_modification");
}

// ============================================================================
// Test: Edge Cases - Map Boundaries
// ============================================================================

void test_map_boundary_modifications() {
    ChunkDirtyTracker tracker(256, 256);  // 8x8 chunks

    // Test modification at map edge
    TerrainModifiedEvent edge_event(
        GridRect::fromCorners(250, 250, 256, 256),
        ModificationType::Cleared
    );

    std::uint32_t dirty_count = tracker.processEvent(edge_event);

    TEST_ASSERT(dirty_count >= 1, "Edge modification should mark at least 1 chunk dirty");
    TEST_ASSERT(tracker.isChunkDirty(7, 7), "Chunk (7,7) should be dirty");

    // Verify no out-of-bounds chunks are marked
    TEST_ASSERT(!tracker.isChunkDirty(8, 7), "Chunk (8,7) should not exist/be dirty");
    TEST_ASSERT(!tracker.isChunkDirty(7, 8), "Chunk (7,8) should not exist/be dirty");

    TEST_PASS("map_boundary_modifications");
}

// ============================================================================
// Test: Sea Level Change Event
// ============================================================================

void test_sea_level_change_event() {
    ChunkDirtyTracker tracker(256, 256);

    // Sea level change typically affects entire map
    // For testing, we use a full-map rect
    TerrainModifiedEvent sea_level_event(
        GridRect::fromCorners(0, 0, 256, 256),
        ModificationType::SeaLevelChanged
    );

    std::uint32_t dirty_count = tracker.processEvent(sea_level_event);

    // 256/32 = 8 chunks in each direction = 64 total chunks
    TEST_ASSERT(dirty_count == 64, "Sea level change should mark all 64 chunks dirty");

    TEST_PASS("sea_level_change_event");
}

// ============================================================================
// Test: Vegetation Chunk Instances Structure
// ============================================================================

void test_vegetation_chunk_instances() {
    ChunkInstances instances;
    instances.chunk_x = 5;
    instances.chunk_y = 7;

    TEST_ASSERT(instances.chunk_x == 5, "chunk_x should be 5");
    TEST_ASSERT(instances.chunk_y == 7, "chunk_y should be 7");
    TEST_ASSERT(instances.instances.empty(), "instances should be empty initially");

    // Add a test instance
    VegetationInstance vi;
    vi.position = glm::vec3(160.5f, 10.0f, 224.5f);
    vi.rotation_y = 1.5f;
    vi.scale = 1.0f;
    vi.model_type = VegetationModelType::BiolumeTree;

    instances.instances.push_back(vi);

    TEST_ASSERT(instances.instances.size() == 1, "Should have 1 instance");

    TEST_PASS("vegetation_chunk_instances");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== TerrainModificationVisualPipeline Tests ===" << std::endl;
    std::cout << std::endl;

    // Run tests (CPU-only, no GPU required)
    test_default_construction();
    test_grid_rect_to_chunks_calculation();
    test_event_marks_chunks_dirty();
    test_modification_types();
    test_chunk_dirty_tracker_tile_to_chunk_conversion();
    test_clear_operation_marks_dirty();
    test_grade_operation_marks_dirty();
    test_terraform_operation_marks_dirty();
    test_visual_update_stats();
    test_rate_limiting_constants();
    test_queue_deduplication();
    test_get_next_dirty_chunk();
    test_terrain_event_size_constraints();
    test_multiple_concurrent_events();
    test_large_area_modification();
    test_map_boundary_modifications();
    test_sea_level_change_event();
    test_vegetation_chunk_instances();

    // Summary
    std::cout << std::endl;
    std::cout << "=== Summary ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    return (tests_failed == 0) ? 0 : 1;
}
