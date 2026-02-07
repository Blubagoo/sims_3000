/**
 * @file TerrainVertex.h
 * @brief GPU vertex format for terrain mesh rendering
 *
 * Defines the TerrainVertex struct used for terrain mesh generation and
 * GPU buffer binding. Each vertex contains position, normal, terrain type,
 * elevation, texture coordinates, and tile coordinates for shader effects.
 *
 * Memory layout is optimized for GPU upload:
 * - 44 bytes total (naturally aligned)
 * - 4-byte aligned for GPU compatibility
 * - Compatible with SDL_GPU buffer binding
 *
 * Terrain type is stored per-vertex to enable smooth blending between
 * terrain types at tile boundaries and for shader-based color lookup.
 *
 * @see TerrainChunk for chunk-level mesh management
 * @see ChunkDirtyTracker for chunk update tracking
 */

#ifndef SIMS3000_TERRAIN_TERRAINVERTEX_H
#define SIMS3000_TERRAIN_TERRAINVERTEX_H

#include <cstdint>
#include <type_traits>
#include <SDL3/SDL.h>

namespace sims3000 {
namespace terrain {

/**
 * @struct TerrainVertex
 * @brief GPU vertex format for terrain mesh rendering.
 *
 * Layout (44 bytes total, naturally aligned):
 * - position: vec3 (12 bytes, offset 0) - World-space position
 * - normal: vec3 (12 bytes, offset 12) - Surface normal for lighting
 * - terrain_type: uint8 (1 byte, offset 24) - TerrainType enum value for color lookup
 * - elevation: uint8 (1 byte, offset 25) - Height level (0-31) for effects
 * - padding: 2 bytes (offset 26) - Alignment padding
 * - uv: vec2 (8 bytes, offset 28) - Texture coordinates
 * - tile_coord: vec2 (8 bytes, offset 36) - Tile position for shader effects
 *
 * Total: 44 bytes with natural alignment (all floats 4-byte aligned).
 *
 * Design notes and intentional deviations from original spec:
 * - terrain_type: Uses uint8_t instead of uint32. Only 10 terrain types exist (0-9),
 *   so uint8_t is sufficient and provides better cache efficiency. GPU vertex
 *   attributes typically pad to 4 bytes anyway.
 * - elevation: Additional uint8_t field (not in original spec) added for shader use.
 *   Allows elevation-based effects without recalculating from position.
 * - tile_coord: Added per SA verification - identifies tile position for certain
 *   shader effects (e.g., tile-based color variation, grid alignment).
 *
 * This struct is designed for SDL_GPU buffer binding and must maintain
 * exact size and alignment for cross-platform compatibility.
 */
struct TerrainVertex {
    // Position in world space (12 bytes, offset 0)
    float position_x;  ///< X coordinate in world space
    float position_y;  ///< Y coordinate in world space
    float position_z;  ///< Z coordinate (elevation) in world space

    // Surface normal for lighting (12 bytes, offset 12)
    float normal_x;    ///< Normal X component
    float normal_y;    ///< Normal Y component
    float normal_z;    ///< Normal Z component

    // Terrain data for shader lookup (4 bytes with padding, offset 24)
    std::uint8_t terrain_type;  ///< TerrainType enum value (0-9)
    std::uint8_t elevation;     ///< Height level (0-31)
    std::uint8_t _padding[2];   ///< Alignment padding

    // Texture coordinates (8 bytes, offset 28)
    float uv_u;        ///< U texture coordinate
    float uv_v;        ///< V texture coordinate

    // Tile coordinates (8 bytes, offset 36)
    float tile_coord_x;  ///< Tile X position (0 to map_width-1)
    float tile_coord_y;  ///< Tile Y position (0 to map_height-1)

    // =========================================================================
    // Constructors
    // =========================================================================

    /**
     * @brief Default constructor - zero-initializes all fields.
     */
    TerrainVertex()
        : position_x(0.0f)
        , position_y(0.0f)
        , position_z(0.0f)
        , normal_x(0.0f)
        , normal_y(1.0f)  // Default up-facing normal
        , normal_z(0.0f)
        , terrain_type(0)
        , elevation(0)
        , _padding{0, 0}
        , uv_u(0.0f)
        , uv_v(0.0f)
        , tile_coord_x(0.0f)
        , tile_coord_y(0.0f)
    {}

    /**
     * @brief Full constructor for explicit initialization.
     *
     * @param px Position X
     * @param py Position Y
     * @param pz Position Z (elevation)
     * @param nx Normal X
     * @param ny Normal Y
     * @param nz Normal Z
     * @param type TerrainType enum value
     * @param elev Elevation level (0-31)
     * @param u Texture U coordinate
     * @param v Texture V coordinate
     * @param tile_x Tile X coordinate
     * @param tile_y Tile Y coordinate
     */
    TerrainVertex(float px, float py, float pz,
                  float nx, float ny, float nz,
                  std::uint8_t type, std::uint8_t elev,
                  float u, float v,
                  float tile_x = 0.0f, float tile_y = 0.0f)
        : position_x(px)
        , position_y(py)
        , position_z(pz)
        , normal_x(nx)
        , normal_y(ny)
        , normal_z(nz)
        , terrain_type(type)
        , elevation(elev)
        , _padding{0, 0}
        , uv_u(u)
        , uv_v(v)
        , tile_coord_x(tile_x)
        , tile_coord_y(tile_y)
    {}

    // =========================================================================
    // Position accessors
    // =========================================================================

    /**
     * @brief Set position from three floats.
     */
    void setPosition(float x, float y, float z) {
        position_x = x;
        position_y = y;
        position_z = z;
    }

    // =========================================================================
    // Normal accessors
    // =========================================================================

    /**
     * @brief Set normal from three floats.
     */
    void setNormal(float x, float y, float z) {
        normal_x = x;
        normal_y = y;
        normal_z = z;
    }

    /**
     * @brief Set normal to up-facing (0, 1, 0).
     */
    void setNormalUp() {
        normal_x = 0.0f;
        normal_y = 1.0f;
        normal_z = 0.0f;
    }

    // =========================================================================
    // UV accessors
    // =========================================================================

    /**
     * @brief Set texture coordinates.
     */
    void setUV(float u, float v) {
        uv_u = u;
        uv_v = v;
    }

    // =========================================================================
    // Tile coordinate accessors
    // =========================================================================

    /**
     * @brief Set tile coordinates.
     */
    void setTileCoord(float x, float y) {
        tile_coord_x = x;
        tile_coord_y = y;
    }
};

// Verify TerrainVertex is exactly 44 bytes (naturally aligned)
static_assert(sizeof(TerrainVertex) == 44,
    "TerrainVertex must be exactly 44 bytes for GPU buffer compatibility");

// Verify TerrainVertex is trivially copyable for GPU upload
static_assert(std::is_trivially_copyable<TerrainVertex>::value,
    "TerrainVertex must be trivially copyable for GPU upload");

// Verify standard layout for predictable memory layout
static_assert(std::is_standard_layout<TerrainVertex>::value,
    "TerrainVertex must be standard layout for GPU buffer binding");

/**
 * @brief Get the SDL_GPUVertexBufferDescription for TerrainVertex.
 *
 * Provides the vertex buffer description needed for pipeline creation.
 * Uses per-vertex input rate (not instanced).
 *
 * @param slot The binding slot for the vertex buffer.
 * @return SDL_GPUVertexBufferDescription for TerrainVertex.
 */
inline SDL_GPUVertexBufferDescription getTerrainVertexBufferDescription(std::uint32_t slot = 0) {
    SDL_GPUVertexBufferDescription desc{};
    desc.slot = slot;
    desc.pitch = sizeof(TerrainVertex);
    desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    desc.instance_step_rate = 0;
    return desc;
}

/**
 * @brief Get the SDL_GPUVertexAttribute array for TerrainVertex.
 *
 * Defines the vertex attribute layout for shader input:
 * - Location 0: position (vec3)
 * - Location 1: normal (vec3)
 * - Location 2: terrain_type + elevation (uint8x2 -> shader unpacks)
 * - Location 3: uv (vec2)
 * - Location 4: tile_coord (vec2)
 *
 * @param slot The buffer slot these attributes bind to.
 * @param out_attributes Output array (must have room for 5 elements).
 * @param out_count Output: number of attributes written (5).
 */
inline void getTerrainVertexAttributes(std::uint32_t slot,
                                       SDL_GPUVertexAttribute* out_attributes,
                                       std::uint32_t* out_count) {
    // Position (vec3 at offset 0)
    out_attributes[0].location = 0;
    out_attributes[0].buffer_slot = slot;
    out_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    out_attributes[0].offset = offsetof(TerrainVertex, position_x);

    // Normal (vec3 at offset 12)
    out_attributes[1].location = 1;
    out_attributes[1].buffer_slot = slot;
    out_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    out_attributes[1].offset = offsetof(TerrainVertex, normal_x);

    // Terrain type and elevation (2 x uint8, packed)
    // Use UBYTE2 format - shader will receive as uvec2 or can unpack
    out_attributes[2].location = 2;
    out_attributes[2].buffer_slot = slot;
    out_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE2;
    out_attributes[2].offset = offsetof(TerrainVertex, terrain_type);

    // UV (vec2 at offset 28)
    out_attributes[3].location = 3;
    out_attributes[3].buffer_slot = slot;
    out_attributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    out_attributes[3].offset = offsetof(TerrainVertex, uv_u);

    // Tile coordinate (vec2 at offset 36)
    out_attributes[4].location = 4;
    out_attributes[4].buffer_slot = slot;
    out_attributes[4].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    out_attributes[4].offset = offsetof(TerrainVertex, tile_coord_x);

    *out_count = 5;
}

/// Number of vertex attributes in TerrainVertex
constexpr std::uint32_t TERRAIN_VERTEX_ATTRIBUTE_COUNT = 5;

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINVERTEX_H
