/**
 * @file test_terrain_vertex.cpp
 * @brief Unit tests for TerrainVertex struct (Ticket 3-023)
 *
 * Tests:
 * - TerrainVertex size and layout verification
 * - Default construction
 * - Full constructor
 * - Accessor methods
 * - SDL_GPU vertex attribute configuration
 */

#include <sims3000/terrain/TerrainVertex.h>
#include <cstdio>
#include <cstring>
#include <cstdint>

using namespace sims3000::terrain;

// Test counter
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            tests_passed++; \
            printf("[PASS] %s\n", message); \
        } else { \
            tests_failed++; \
            printf("[FAIL] %s\n", message); \
        } \
    } while (0)

#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) == (expected)) { \
            tests_passed++; \
            printf("[PASS] %s\n", message); \
        } else { \
            tests_failed++; \
            printf("[FAIL] %s (expected %d, got %d)\n", message, (int)(expected), (int)(actual)); \
        } \
    } while (0)

#define TEST_ASSERT_FLOAT_EQ(actual, expected, message) \
    do { \
        float diff = (actual) - (expected); \
        if (diff < 0.0001f && diff > -0.0001f) { \
            tests_passed++; \
            printf("[PASS] %s\n", message); \
        } else { \
            tests_failed++; \
            printf("[FAIL] %s (expected %.4f, got %.4f)\n", message, (float)(expected), (float)(actual)); \
        } \
    } while (0)

// ============================================================================
// Test: TerrainVertex size is exactly 44 bytes
// ============================================================================
void test_vertex_size() {
    printf("\n--- Test: TerrainVertex Size ---\n");

    // Size breakdown:
    // - position: 12 bytes (3 floats)
    // - normal: 12 bytes (3 floats)
    // - terrain_type: 1 byte
    // - elevation: 1 byte
    // - padding: 2 bytes
    // - uv: 8 bytes (2 floats)
    // - tile_coord: 8 bytes (2 floats)
    // Total: 44 bytes with natural alignment
    TEST_ASSERT_EQ(sizeof(TerrainVertex), 44, "TerrainVertex should be 44 bytes");
}

// ============================================================================
// Test: TerrainVertex memory layout (offsets)
// ============================================================================
void test_vertex_layout() {
    printf("\n--- Test: TerrainVertex Layout ---\n");

    // Position at offset 0
    TEST_ASSERT_EQ(offsetof(TerrainVertex, position_x), 0, "position_x at offset 0");
    TEST_ASSERT_EQ(offsetof(TerrainVertex, position_y), 4, "position_y at offset 4");
    TEST_ASSERT_EQ(offsetof(TerrainVertex, position_z), 8, "position_z at offset 8");

    // Normal at offset 12
    TEST_ASSERT_EQ(offsetof(TerrainVertex, normal_x), 12, "normal_x at offset 12");
    TEST_ASSERT_EQ(offsetof(TerrainVertex, normal_y), 16, "normal_y at offset 16");
    TEST_ASSERT_EQ(offsetof(TerrainVertex, normal_z), 20, "normal_z at offset 20");

    // Terrain type and elevation at offset 24
    TEST_ASSERT_EQ(offsetof(TerrainVertex, terrain_type), 24, "terrain_type at offset 24");
    TEST_ASSERT_EQ(offsetof(TerrainVertex, elevation), 25, "elevation at offset 25");

    // UV at offset 28 (after 2 bytes data + 2 bytes padding)
    TEST_ASSERT_EQ(offsetof(TerrainVertex, uv_u), 28, "uv_u at offset 28");
    TEST_ASSERT_EQ(offsetof(TerrainVertex, uv_v), 32, "uv_v at offset 32");

    // Tile coordinates at offset 36
    TEST_ASSERT_EQ(offsetof(TerrainVertex, tile_coord_x), 36, "tile_coord_x at offset 36");
    TEST_ASSERT_EQ(offsetof(TerrainVertex, tile_coord_y), 40, "tile_coord_y at offset 40");
}

// ============================================================================
// Test: TerrainVertex default construction
// ============================================================================
void test_default_construction() {
    printf("\n--- Test: Default Construction ---\n");

    TerrainVertex v;

    // Position should be (0, 0, 0)
    TEST_ASSERT_FLOAT_EQ(v.position_x, 0.0f, "Default position_x is 0");
    TEST_ASSERT_FLOAT_EQ(v.position_y, 0.0f, "Default position_y is 0");
    TEST_ASSERT_FLOAT_EQ(v.position_z, 0.0f, "Default position_z is 0");

    // Normal should be (0, 1, 0) - up-facing
    TEST_ASSERT_FLOAT_EQ(v.normal_x, 0.0f, "Default normal_x is 0");
    TEST_ASSERT_FLOAT_EQ(v.normal_y, 1.0f, "Default normal_y is 1 (up)");
    TEST_ASSERT_FLOAT_EQ(v.normal_z, 0.0f, "Default normal_z is 0");

    // Terrain data should be 0
    TEST_ASSERT_EQ(v.terrain_type, 0, "Default terrain_type is 0");
    TEST_ASSERT_EQ(v.elevation, 0, "Default elevation is 0");

    // UV should be (0, 0)
    TEST_ASSERT_FLOAT_EQ(v.uv_u, 0.0f, "Default uv_u is 0");
    TEST_ASSERT_FLOAT_EQ(v.uv_v, 0.0f, "Default uv_v is 0");

    // Tile coord should be (0, 0)
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_x, 0.0f, "Default tile_coord_x is 0");
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_y, 0.0f, "Default tile_coord_y is 0");
}

// ============================================================================
// Test: TerrainVertex full constructor
// ============================================================================
void test_full_construction() {
    printf("\n--- Test: Full Constructor ---\n");

    TerrainVertex v(
        1.0f, 2.0f, 3.0f,       // position
        0.0f, 1.0f, 0.0f,       // normal
        5, 15,                   // terrain_type, elevation
        0.25f, 0.75f            // uv
    );

    // Position
    TEST_ASSERT_FLOAT_EQ(v.position_x, 1.0f, "Constructor position_x");
    TEST_ASSERT_FLOAT_EQ(v.position_y, 2.0f, "Constructor position_y");
    TEST_ASSERT_FLOAT_EQ(v.position_z, 3.0f, "Constructor position_z");

    // Normal
    TEST_ASSERT_FLOAT_EQ(v.normal_x, 0.0f, "Constructor normal_x");
    TEST_ASSERT_FLOAT_EQ(v.normal_y, 1.0f, "Constructor normal_y");
    TEST_ASSERT_FLOAT_EQ(v.normal_z, 0.0f, "Constructor normal_z");

    // Terrain data
    TEST_ASSERT_EQ(v.terrain_type, 5, "Constructor terrain_type");
    TEST_ASSERT_EQ(v.elevation, 15, "Constructor elevation");

    // UV
    TEST_ASSERT_FLOAT_EQ(v.uv_u, 0.25f, "Constructor uv_u");
    TEST_ASSERT_FLOAT_EQ(v.uv_v, 0.75f, "Constructor uv_v");

    // Tile coord (using defaults since not specified in test constructor)
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_x, 0.0f, "Constructor tile_coord_x default");
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_y, 0.0f, "Constructor tile_coord_y default");

    // Test with explicit tile coords
    TerrainVertex v2(
        1.0f, 2.0f, 3.0f,       // position
        0.0f, 1.0f, 0.0f,       // normal
        5, 15,                   // terrain_type, elevation
        0.25f, 0.75f,           // uv
        10.0f, 20.0f            // tile_coord
    );
    TEST_ASSERT_FLOAT_EQ(v2.tile_coord_x, 10.0f, "Constructor tile_coord_x explicit");
    TEST_ASSERT_FLOAT_EQ(v2.tile_coord_y, 20.0f, "Constructor tile_coord_y explicit");
}

// ============================================================================
// Test: TerrainVertex accessor methods
// ============================================================================
void test_accessors() {
    printf("\n--- Test: Accessor Methods ---\n");

    TerrainVertex v;

    // setPosition
    v.setPosition(10.0f, 20.0f, 30.0f);
    TEST_ASSERT_FLOAT_EQ(v.position_x, 10.0f, "setPosition x");
    TEST_ASSERT_FLOAT_EQ(v.position_y, 20.0f, "setPosition y");
    TEST_ASSERT_FLOAT_EQ(v.position_z, 30.0f, "setPosition z");

    // setNormal
    v.setNormal(0.5f, 0.5f, 0.707f);
    TEST_ASSERT_FLOAT_EQ(v.normal_x, 0.5f, "setNormal x");
    TEST_ASSERT_FLOAT_EQ(v.normal_y, 0.5f, "setNormal y");
    TEST_ASSERT_FLOAT_EQ(v.normal_z, 0.707f, "setNormal z");

    // setNormalUp
    v.setNormalUp();
    TEST_ASSERT_FLOAT_EQ(v.normal_x, 0.0f, "setNormalUp x");
    TEST_ASSERT_FLOAT_EQ(v.normal_y, 1.0f, "setNormalUp y");
    TEST_ASSERT_FLOAT_EQ(v.normal_z, 0.0f, "setNormalUp z");

    // setUV
    v.setUV(0.123f, 0.456f);
    TEST_ASSERT_FLOAT_EQ(v.uv_u, 0.123f, "setUV u");
    TEST_ASSERT_FLOAT_EQ(v.uv_v, 0.456f, "setUV v");

    // setTileCoord
    v.setTileCoord(100.0f, 200.0f);
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_x, 100.0f, "setTileCoord x");
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_y, 200.0f, "setTileCoord y");
}

// ============================================================================
// Test: TerrainVertex is trivially copyable
// ============================================================================
void test_trivially_copyable() {
    printf("\n--- Test: Trivially Copyable ---\n");

    TerrainVertex v1(1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 0.0f, 7, 25, 0.5f, 0.5f, 10.0f, 20.0f);
    TerrainVertex v2;

    // Copy using memcpy (as GPU upload would)
    std::memcpy(&v2, &v1, sizeof(TerrainVertex));

    TEST_ASSERT_FLOAT_EQ(v2.position_x, 1.0f, "memcpy preserves position_x");
    TEST_ASSERT_FLOAT_EQ(v2.position_y, 2.0f, "memcpy preserves position_y");
    TEST_ASSERT_FLOAT_EQ(v2.position_z, 3.0f, "memcpy preserves position_z");
    TEST_ASSERT_EQ(v2.terrain_type, 7, "memcpy preserves terrain_type");
    TEST_ASSERT_EQ(v2.elevation, 25, "memcpy preserves elevation");
    TEST_ASSERT_FLOAT_EQ(v2.uv_u, 0.5f, "memcpy preserves uv_u");
    TEST_ASSERT_FLOAT_EQ(v2.uv_v, 0.5f, "memcpy preserves uv_v");
    TEST_ASSERT_FLOAT_EQ(v2.tile_coord_x, 10.0f, "memcpy preserves tile_coord_x");
    TEST_ASSERT_FLOAT_EQ(v2.tile_coord_y, 20.0f, "memcpy preserves tile_coord_y");
}

// ============================================================================
// Test: Vertex buffer description
// ============================================================================
void test_vertex_buffer_description() {
    printf("\n--- Test: Vertex Buffer Description ---\n");

    SDL_GPUVertexBufferDescription desc = getTerrainVertexBufferDescription(0);

    TEST_ASSERT_EQ(desc.slot, 0, "Buffer slot is 0");
    TEST_ASSERT_EQ(desc.pitch, 44, "Buffer pitch is 44 bytes");
    TEST_ASSERT_EQ(desc.input_rate, SDL_GPU_VERTEXINPUTRATE_VERTEX, "Input rate is per-vertex");
    TEST_ASSERT_EQ(desc.instance_step_rate, 0, "Instance step rate is 0");

    // Test with different slot
    SDL_GPUVertexBufferDescription desc2 = getTerrainVertexBufferDescription(1);
    TEST_ASSERT_EQ(desc2.slot, 1, "Buffer slot can be changed");
}

// ============================================================================
// Test: Vertex attributes
// ============================================================================
void test_vertex_attributes() {
    printf("\n--- Test: Vertex Attributes ---\n");

    SDL_GPUVertexAttribute attrs[5];
    std::uint32_t count = 0;

    getTerrainVertexAttributes(0, attrs, &count);

    TEST_ASSERT_EQ(count, 5, "Should have 5 vertex attributes");
    TEST_ASSERT_EQ(count, TERRAIN_VERTEX_ATTRIBUTE_COUNT, "Count matches constant");

    // Attribute 0: position
    TEST_ASSERT_EQ(attrs[0].location, 0, "Position at location 0");
    TEST_ASSERT_EQ(attrs[0].buffer_slot, 0, "Position uses slot 0");
    TEST_ASSERT_EQ(attrs[0].format, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, "Position is float3");
    TEST_ASSERT_EQ(attrs[0].offset, 0, "Position offset is 0");

    // Attribute 1: normal
    TEST_ASSERT_EQ(attrs[1].location, 1, "Normal at location 1");
    TEST_ASSERT_EQ(attrs[1].format, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, "Normal is float3");
    TEST_ASSERT_EQ(attrs[1].offset, 12, "Normal offset is 12");

    // Attribute 2: terrain data (type + elevation)
    TEST_ASSERT_EQ(attrs[2].location, 2, "Terrain data at location 2");
    TEST_ASSERT_EQ(attrs[2].format, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE2, "Terrain data is ubyte2");
    TEST_ASSERT_EQ(attrs[2].offset, 24, "Terrain data offset is 24");

    // Attribute 3: UV
    TEST_ASSERT_EQ(attrs[3].location, 3, "UV at location 3");
    TEST_ASSERT_EQ(attrs[3].format, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, "UV is float2");
    TEST_ASSERT_EQ(attrs[3].offset, 28, "UV offset is 28");

    // Attribute 4: tile_coord
    TEST_ASSERT_EQ(attrs[4].location, 4, "Tile coord at location 4");
    TEST_ASSERT_EQ(attrs[4].format, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, "Tile coord is float2");
    TEST_ASSERT_EQ(attrs[4].offset, 36, "Tile coord offset is 36");
}

// ============================================================================
// Test: TerrainVertex array for GPU upload simulation
// ============================================================================
void test_vertex_array() {
    printf("\n--- Test: Vertex Array (GPU Upload Simulation) ---\n");

    // Simulate a small vertex buffer
    TerrainVertex vertices[4];

    // Initialize as a quad (4 corners) with tile coordinates
    vertices[0] = TerrainVertex(0.0f, 0.0f, 0.0f, 0, 1, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
    vertices[1] = TerrainVertex(1.0f, 0.0f, 0.0f, 0, 1, 0, 0, 0, 1.0f, 0.0f, 1.0f, 0.0f);
    vertices[2] = TerrainVertex(0.0f, 0.0f, 1.0f, 0, 1, 0, 0, 0, 0.0f, 1.0f, 0.0f, 1.0f);
    vertices[3] = TerrainVertex(1.0f, 0.0f, 1.0f, 0, 1, 0, 0, 0, 1.0f, 1.0f, 1.0f, 1.0f);

    // Verify array size (4 vertices * 44 bytes = 176 bytes)
    TEST_ASSERT_EQ(sizeof(vertices), 176, "4 vertices = 176 bytes");

    // Verify each vertex is at correct offset
    auto* base = reinterpret_cast<std::uint8_t*>(vertices);
    auto* v0 = reinterpret_cast<std::uint8_t*>(&vertices[0]);
    auto* v1 = reinterpret_cast<std::uint8_t*>(&vertices[1]);
    auto* v2 = reinterpret_cast<std::uint8_t*>(&vertices[2]);
    auto* v3 = reinterpret_cast<std::uint8_t*>(&vertices[3]);

    TEST_ASSERT_EQ(v0 - base, 0, "Vertex 0 at offset 0");
    TEST_ASSERT_EQ(v1 - base, 44, "Vertex 1 at offset 44");
    TEST_ASSERT_EQ(v2 - base, 88, "Vertex 2 at offset 88");
    TEST_ASSERT_EQ(v3 - base, 132, "Vertex 3 at offset 132");
}

// ============================================================================
// Test: Terrain type values (0-9 range)
// ============================================================================
void test_terrain_type_range() {
    printf("\n--- Test: Terrain Type Range ---\n");

    TerrainVertex v;

    // Test all valid terrain types (0-9)
    for (std::uint8_t i = 0; i < 10; ++i) {
        v.terrain_type = i;
        char msg[64];
        snprintf(msg, sizeof(msg), "terrain_type %d stored correctly", i);
        TEST_ASSERT_EQ(v.terrain_type, i, msg);
    }

    // Test max uint8 value (edge case)
    v.terrain_type = 255;
    TEST_ASSERT_EQ(v.terrain_type, 255, "terrain_type can store max uint8");
}

// ============================================================================
// Test: Elevation values (0-31 range)
// ============================================================================
void test_elevation_range() {
    printf("\n--- Test: Elevation Range ---\n");

    TerrainVertex v;

    // Test valid elevation range (0-31)
    for (std::uint8_t i = 0; i <= 31; ++i) {
        v.elevation = i;
        char msg[64];
        snprintf(msg, sizeof(msg), "elevation %d stored correctly", i);
        TEST_ASSERT_EQ(v.elevation, i, msg);
    }

    // Test max uint8 value (edge case)
    v.elevation = 255;
    TEST_ASSERT_EQ(v.elevation, 255, "elevation can store max uint8");
}

// ============================================================================
// Test: Tile coordinate values
// ============================================================================
void test_tile_coord() {
    printf("\n--- Test: Tile Coordinate ---\n");

    TerrainVertex v;

    // Test various tile coordinate values
    v.setTileCoord(0.0f, 0.0f);
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_x, 0.0f, "tile_coord_x = 0");
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_y, 0.0f, "tile_coord_y = 0");

    v.setTileCoord(127.0f, 127.0f);
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_x, 127.0f, "tile_coord_x = 127");
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_y, 127.0f, "tile_coord_y = 127");

    v.setTileCoord(255.0f, 255.0f);
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_x, 255.0f, "tile_coord_x = 255");
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_y, 255.0f, "tile_coord_y = 255");

    v.setTileCoord(511.0f, 511.0f);
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_x, 511.0f, "tile_coord_x = 511 (max map size)");
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_y, 511.0f, "tile_coord_y = 511 (max map size)");

    // Test fractional values (edge of tile)
    v.setTileCoord(10.5f, 20.5f);
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_x, 10.5f, "tile_coord_x = 10.5");
    TEST_ASSERT_FLOAT_EQ(v.tile_coord_y, 20.5f, "tile_coord_y = 20.5");
}

// ============================================================================
// Main
// ============================================================================
int main() {
    printf("===========================================\n");
    printf("TerrainVertex Unit Tests (Ticket 3-023)\n");
    printf("===========================================\n");

    test_vertex_size();
    test_vertex_layout();
    test_default_construction();
    test_full_construction();
    test_accessors();
    test_trivially_copyable();
    test_vertex_buffer_description();
    test_vertex_attributes();
    test_vertex_array();
    test_terrain_type_range();
    test_elevation_range();
    test_tile_coord();

    printf("\n===========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("===========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
