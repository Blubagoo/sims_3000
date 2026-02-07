/**
 * @file WaterMesh.h
 * @brief Water surface mesh generation for terrain rendering.
 *
 * Generates separate water surface meshes for each water body (ocean, rivers,
 * lakes). Water meshes are NOT part of terrain chunks -- they are separate
 * semi-transparent layers rendered on top of terrain.
 *
 * Key features:
 * - Separate mesh per water body (using body IDs from ticket 3-005)
 * - Water surface at sea_level elevation (Y = sea_level * ELEVATION_HEIGHT)
 * - Per-vertex shore_factor (0.0-1.0) for shoreline glow effects
 * - Ocean: single mesh covering all DeepVoid tiles at map edges
 * - Rivers: mesh per river body following FlowChannel tiles
 * - Lakes: mesh per StillBasin body
 * - Water mesh vertices at tile corners (shared with terrain grid but separate buffer)
 *
 * Resource ownership:
 * - WaterMesh stores GPU buffer handles (vertex and index buffers)
 * - GPU memory is released via SDL_ReleaseGPUBuffer on cleanup
 * - WaterMeshes must be properly released before destruction
 *
 * @see WaterData.h for WaterBodyID and FlowDirection
 * @see WaterBodyGenerator.h for water body generation
 * @see TerrainChunk.h for ELEVATION_HEIGHT constant
 */

#ifndef SIMS3000_TERRAIN_WATERMESH_H
#define SIMS3000_TERRAIN_WATERMESH_H

#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/WaterData.h>
#include <sims3000/terrain/TerrainChunk.h>
#include <sims3000/render/GPUMesh.h>
#include <SDL3/SDL.h>
#include <cstdint>
#include <vector>
#include <type_traits>

namespace sims3000 {
namespace terrain {

/**
 * @struct WaterVertex
 * @brief GPU vertex format for water surface mesh rendering.
 *
 * Layout (24 bytes total, naturally aligned):
 * - position: vec3 (12 bytes, offset 0) - World-space position
 * - shore_factor: float (4 bytes, offset 12) - Shoreline proximity (0.0-1.0)
 * - water_body_id: uint16 (2 bytes, offset 16) - Water body identifier
 * - padding: 2 bytes (offset 18) - Alignment padding
 * - uv: vec2 (8 bytes, offset 20) - Texture coordinates for wave animation
 *
 * Note: Total is 28 bytes. Adjusted from spec to maintain natural alignment.
 *
 * Design notes:
 * - shore_factor: 1.0 at land-adjacent vertices, 0.0 at interior
 *   Used for shoreline glow/foam effects in shader
 * - water_body_id: Allows per-body tinting or effects in shader
 * - uv: For animated water texture scrolling
 */
struct WaterVertex {
    // Position in world space (12 bytes, offset 0)
    float position_x;  ///< X coordinate in world space
    float position_y;  ///< Y coordinate (sea level elevation)
    float position_z;  ///< Z coordinate in world space

    // Shore factor for shoreline glow (4 bytes, offset 12)
    float shore_factor;  ///< 0.0 = interior water, 1.0 = adjacent to land

    // Water body identification (4 bytes with padding, offset 16)
    std::uint16_t water_body_id;  ///< Water body ID (1-65535, 0 = invalid)
    std::uint8_t _padding[2];     ///< Alignment padding

    // Texture coordinates (8 bytes, offset 20)
    float uv_u;  ///< U texture coordinate
    float uv_v;  ///< V texture coordinate

    // =========================================================================
    // Constructors
    // =========================================================================

    /**
     * @brief Default constructor - zero-initializes all fields.
     */
    WaterVertex()
        : position_x(0.0f)
        , position_y(0.0f)
        , position_z(0.0f)
        , shore_factor(0.0f)
        , water_body_id(0)
        , _padding{0, 0}
        , uv_u(0.0f)
        , uv_v(0.0f)
    {}

    /**
     * @brief Full constructor for explicit initialization.
     *
     * @param px Position X
     * @param py Position Y (sea level)
     * @param pz Position Z
     * @param sf Shore factor (0.0-1.0)
     * @param body_id Water body ID
     * @param u UV coordinate U
     * @param v UV coordinate V
     */
    WaterVertex(float px, float py, float pz,
                float sf, std::uint16_t body_id,
                float u, float v)
        : position_x(px)
        , position_y(py)
        , position_z(pz)
        , shore_factor(sf)
        , water_body_id(body_id)
        , _padding{0, 0}
        , uv_u(u)
        , uv_v(v)
    {}

    // =========================================================================
    // Accessors
    // =========================================================================

    /**
     * @brief Set position from three floats.
     */
    void setPosition(float x, float y, float z) {
        position_x = x;
        position_y = y;
        position_z = z;
    }

    /**
     * @brief Set UV coordinates.
     */
    void setUV(float u, float v) {
        uv_u = u;
        uv_v = v;
    }
};

// Verify WaterVertex is exactly 28 bytes (naturally aligned)
static_assert(sizeof(WaterVertex) == 28,
    "WaterVertex must be exactly 28 bytes for GPU buffer compatibility");

// Verify WaterVertex is trivially copyable for GPU upload
static_assert(std::is_trivially_copyable<WaterVertex>::value,
    "WaterVertex must be trivially copyable for GPU upload");

// Verify standard layout for predictable memory layout
static_assert(std::is_standard_layout<WaterVertex>::value,
    "WaterVertex must be standard layout for GPU buffer binding");

/**
 * @brief Get the SDL_GPUVertexBufferDescription for WaterVertex.
 *
 * Provides the vertex buffer description needed for pipeline creation.
 * Uses per-vertex input rate (not instanced).
 *
 * @param slot The binding slot for the vertex buffer.
 * @return SDL_GPUVertexBufferDescription for WaterVertex.
 */
inline SDL_GPUVertexBufferDescription getWaterVertexBufferDescription(std::uint32_t slot = 0) {
    SDL_GPUVertexBufferDescription desc{};
    desc.slot = slot;
    desc.pitch = sizeof(WaterVertex);
    desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    desc.instance_step_rate = 0;
    return desc;
}

/**
 * @brief Get the SDL_GPUVertexAttribute array for WaterVertex.
 *
 * Defines the vertex attribute layout for shader input:
 * - Location 0: position (vec3)
 * - Location 1: shore_factor (float)
 * - Location 2: water_body_id (uint16, shader receives as uint)
 * - Location 3: uv (vec2)
 *
 * @param slot The buffer slot these attributes bind to.
 * @param out_attributes Output array (must have room for 4 elements).
 * @param out_count Output: number of attributes written (4).
 */
inline void getWaterVertexAttributes(std::uint32_t slot,
                                     SDL_GPUVertexAttribute* out_attributes,
                                     std::uint32_t* out_count) {
    // Position (vec3 at offset 0)
    out_attributes[0].location = 0;
    out_attributes[0].buffer_slot = slot;
    out_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    out_attributes[0].offset = offsetof(WaterVertex, position_x);

    // Shore factor (float at offset 12)
    out_attributes[1].location = 1;
    out_attributes[1].buffer_slot = slot;
    out_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    out_attributes[1].offset = offsetof(WaterVertex, shore_factor);

    // Water body ID (uint16 at offset 16)
    out_attributes[2].location = 2;
    out_attributes[2].buffer_slot = slot;
    out_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_USHORT2;  // uint16 x2 for alignment
    out_attributes[2].offset = offsetof(WaterVertex, water_body_id);

    // UV (vec2 at offset 20)
    out_attributes[3].location = 3;
    out_attributes[3].buffer_slot = slot;
    out_attributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    out_attributes[3].offset = offsetof(WaterVertex, uv_u);

    *out_count = 4;
}

/// Number of vertex attributes in WaterVertex
constexpr std::uint32_t WATER_VERTEX_ATTRIBUTE_COUNT = 4;

/**
 * @enum WaterBodyType
 * @brief Classification of water body type for rendering.
 */
enum class WaterBodyType : std::uint8_t {
    Ocean = 0,   ///< DeepVoid tiles (ocean/deep water)
    River = 1,   ///< FlowChannel tiles (rivers)
    Lake = 2     ///< StillBasin tiles (lakes)
};

/**
 * @struct WaterMesh
 * @brief CPU-side data structure for a water body's surface mesh.
 *
 * Manages the GPU resources and state for rendering a single water body.
 * Each water body (ocean, river segment, lake) gets its own WaterMesh.
 *
 * Lifecycle:
 * 1. Create WaterMesh with water body info
 * 2. Generate mesh data via WaterMeshGenerator
 * 3. Upload to GPU via uploadToGPU()
 * 4. Render water surface
 * 5. Release GPU resources via releaseGPUResources()
 *
 * Thread safety:
 * - Mesh data is accessed from the main thread only
 */
struct WaterMesh {
    // =========================================================================
    // Water Body Identity
    // =========================================================================

    WaterBodyID body_id;          ///< Water body identifier (1-65535)
    WaterBodyType body_type;      ///< Type of water body (ocean/river/lake)

    // =========================================================================
    // GPU Resources
    // =========================================================================

    SDL_GPUBuffer* vertex_buffer;  ///< GPU vertex buffer
    SDL_GPUBuffer* index_buffer;   ///< GPU index buffer

    // =========================================================================
    // Mesh Metadata
    // =========================================================================

    std::uint32_t vertex_count;    ///< Number of vertices in vertex_buffer
    std::uint32_t index_count;     ///< Number of indices in index_buffer

    // =========================================================================
    // CPU-side data (for mesh generation)
    // =========================================================================

    std::vector<WaterVertex> vertices;  ///< CPU-side vertex data
    std::vector<std::uint32_t> indices; ///< CPU-side index data

    // =========================================================================
    // Bounding Volume
    // =========================================================================

    AABB aabb;  ///< Axis-aligned bounding box for frustum culling

    // =========================================================================
    // State Flags
    // =========================================================================

    bool dirty;             ///< Needs GPU buffer update
    bool has_gpu_resources; ///< GPU buffers have been created

    // =========================================================================
    // Constructors
    // =========================================================================

    /**
     * @brief Default constructor - creates an uninitialized water mesh.
     */
    WaterMesh()
        : body_id(NO_WATER_BODY)
        , body_type(WaterBodyType::Ocean)
        , vertex_buffer(nullptr)
        , index_buffer(nullptr)
        , vertex_count(0)
        , index_count(0)
        , vertices()
        , indices()
        , aabb()
        , dirty(true)
        , has_gpu_resources(false)
    {}

    /**
     * @brief Construct a water mesh for a specific water body.
     *
     * @param id Water body ID.
     * @param type Type of water body.
     */
    WaterMesh(WaterBodyID id, WaterBodyType type)
        : body_id(id)
        , body_type(type)
        , vertex_buffer(nullptr)
        , index_buffer(nullptr)
        , vertex_count(0)
        , index_count(0)
        , vertices()
        , indices()
        , aabb()
        , dirty(true)
        , has_gpu_resources(false)
    {}

    // Non-copyable (owns GPU resources)
    WaterMesh(const WaterMesh&) = delete;
    WaterMesh& operator=(const WaterMesh&) = delete;

    // Movable
    WaterMesh(WaterMesh&& other) noexcept
        : body_id(other.body_id)
        , body_type(other.body_type)
        , vertex_buffer(other.vertex_buffer)
        , index_buffer(other.index_buffer)
        , vertex_count(other.vertex_count)
        , index_count(other.index_count)
        , vertices(std::move(other.vertices))
        , indices(std::move(other.indices))
        , aabb(other.aabb)
        , dirty(other.dirty)
        , has_gpu_resources(other.has_gpu_resources)
    {
        other.vertex_buffer = nullptr;
        other.index_buffer = nullptr;
        other.has_gpu_resources = false;
    }

    WaterMesh& operator=(WaterMesh&& other) noexcept {
        if (this != &other) {
            body_id = other.body_id;
            body_type = other.body_type;
            vertex_buffer = other.vertex_buffer;
            index_buffer = other.index_buffer;
            vertex_count = other.vertex_count;
            index_count = other.index_count;
            vertices = std::move(other.vertices);
            indices = std::move(other.indices);
            aabb = other.aabb;
            dirty = other.dirty;
            has_gpu_resources = other.has_gpu_resources;

            other.vertex_buffer = nullptr;
            other.index_buffer = nullptr;
            other.has_gpu_resources = false;
        }
        return *this;
    }

    // =========================================================================
    // State Methods
    // =========================================================================

    /**
     * @brief Mark the mesh as needing GPU buffer update.
     */
    void markDirty() {
        dirty = true;
    }

    /**
     * @brief Clear the dirty flag (after successful GPU upload).
     */
    void clearDirty() {
        dirty = false;
    }

    /**
     * @brief Check if mesh needs GPU buffer update.
     */
    bool isDirty() const {
        return dirty;
    }

    /**
     * @brief Check if mesh has valid GPU buffers.
     */
    bool hasGPUResources() const {
        return has_gpu_resources &&
               vertex_buffer != nullptr &&
               index_buffer != nullptr;
    }

    /**
     * @brief Check if mesh is renderable.
     */
    bool isRenderable() const {
        return hasGPUResources() && !dirty && index_count > 0;
    }

    /**
     * @brief Check if mesh has any water tiles.
     */
    bool isEmpty() const {
        return indices.empty();
    }

    // =========================================================================
    // GPU Resource Management
    // =========================================================================

    /**
     * @brief Release GPU resources.
     *
     * Must be called before mesh destruction if GPU resources were created.
     *
     * @param device The SDL GPU device that owns the buffers.
     */
    void releaseGPUResources(SDL_GPUDevice* device) {
        if (vertex_buffer != nullptr) {
            SDL_ReleaseGPUBuffer(device, vertex_buffer);
            vertex_buffer = nullptr;
        }
        if (index_buffer != nullptr) {
            SDL_ReleaseGPUBuffer(device, index_buffer);
            index_buffer = nullptr;
        }
        vertex_count = 0;
        index_count = 0;
        has_gpu_resources = false;
        dirty = true;
    }

    /**
     * @brief Clear CPU-side mesh data (after GPU upload to save memory).
     */
    void clearCPUData() {
        vertices.clear();
        vertices.shrink_to_fit();
        indices.clear();
        indices.shrink_to_fit();
    }
};

/**
 * @struct WaterMeshGenerationResult
 * @brief Result of water mesh generation.
 */
struct WaterMeshGenerationResult {
    std::vector<WaterMesh> meshes;      ///< Generated water meshes (one per body)
    std::uint32_t total_vertex_count;   ///< Total vertices across all meshes
    std::uint32_t total_index_count;    ///< Total indices across all meshes
    std::uint16_t ocean_mesh_count;     ///< Number of ocean meshes
    std::uint16_t river_mesh_count;     ///< Number of river meshes
    std::uint16_t lake_mesh_count;      ///< Number of lake meshes
    float generation_time_ms;           ///< Time taken to generate (milliseconds)
};

/**
 * @class WaterMeshGenerator
 * @brief Generates water surface meshes from terrain and water data.
 *
 * Creates separate meshes for each water body, with per-vertex shore_factor
 * for shoreline glow effects. Vertices are placed at tile corners and
 * quads are generated only for water tiles within each body.
 *
 * Usage:
 * @code
 * TerrainGrid grid(MapSize::Medium);
 * WaterData waterData(MapSize::Medium);
 * // ... generate terrain and water bodies ...
 *
 * WaterMeshGenerationResult result = WaterMeshGenerator::generate(grid, waterData);
 * for (auto& mesh : result.meshes) {
 *     // Upload to GPU and render
 * }
 * @endcode
 */
class WaterMeshGenerator {
public:
    /**
     * @brief Generate water surface meshes for all water bodies.
     *
     * Creates one mesh per water body with:
     * - Vertices at tile corners (shared with terrain grid but separate buffer)
     * - Flat plane at sea_level elevation
     * - Per-vertex shore_factor (1.0 at land-adjacent, 0.0 at interior)
     * - Indexed quads only for water tiles within each body
     *
     * @param grid The terrain grid (for terrain types and sea level).
     * @param waterData Water body IDs and flow directions.
     * @return Generated meshes and statistics.
     */
    static WaterMeshGenerationResult generate(
        const TerrainGrid& grid,
        const WaterData& waterData
    );

    /**
     * @brief Regenerate a single water body's mesh.
     *
     * Used when sea level changes or terrain is modified near water.
     *
     * @param grid The terrain grid.
     * @param waterData Water body data.
     * @param body_id The water body to regenerate.
     * @param out_mesh Output mesh to fill.
     * @return true if mesh was generated, false if body_id not found.
     */
    static bool regenerateBody(
        const TerrainGrid& grid,
        const WaterData& waterData,
        WaterBodyID body_id,
        WaterMesh& out_mesh
    );

private:
    /**
     * @brief Check if a terrain type is water.
     */
    static bool isWater(TerrainType type);

    /**
     * @brief Get the water body type for a terrain type.
     */
    static WaterBodyType getWaterBodyType(TerrainType type);

    /**
     * @brief Calculate shore_factor for a vertex at tile corner (vx, vy).
     *
     * Shore factor is 1.0 if any of the 4 adjacent tiles is land.
     *
     * @param grid The terrain grid.
     * @param vx Vertex X coordinate (tile corner).
     * @param vy Vertex Y coordinate (tile corner).
     * @param waterData Water body data for checking body membership.
     * @param body_id The water body we're generating mesh for.
     * @return Shore factor (0.0 = interior, 1.0 = shoreline).
     */
    static float calculateShoreFactor(
        const TerrainGrid& grid,
        std::int32_t vx,
        std::int32_t vy,
        const WaterData& waterData,
        WaterBodyID body_id
    );

    /**
     * @brief Collect all tiles belonging to a water body.
     *
     * @param waterData Water body data.
     * @param body_id The body to collect tiles for.
     * @return Vector of (x, y) tile coordinates.
     */
    static std::vector<std::pair<std::uint16_t, std::uint16_t>> collectBodyTiles(
        const WaterData& waterData,
        WaterBodyID body_id
    );

    /**
     * @brief Generate mesh for a single water body.
     *
     * @param grid The terrain grid.
     * @param waterData Water body data.
     * @param body_id The water body ID.
     * @param tiles Tiles belonging to this body.
     * @return Generated water mesh.
     */
    static WaterMesh generateBodyMesh(
        const TerrainGrid& grid,
        const WaterData& waterData,
        WaterBodyID body_id,
        const std::vector<std::pair<std::uint16_t, std::uint16_t>>& tiles
    );
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_WATERMESH_H
