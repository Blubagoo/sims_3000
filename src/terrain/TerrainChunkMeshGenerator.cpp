/**
 * @file TerrainChunkMeshGenerator.cpp
 * @brief Implementation of terrain chunk mesh generation.
 *
 * Generates GPU-ready vertex/index buffers for 32x32 tile terrain chunks.
 * Handles surface mesh generation, cliff face geometry, and incremental
 * chunk rebuilds with at most 1 chunk per frame to avoid GPU stalls.
 */

#include <sims3000/terrain/TerrainChunkMeshGenerator.h>
#include <sims3000/core/Logger.h>
#include <algorithm>
#include <chrono>
#include <cmath>

namespace sims3000 {
namespace terrain {

// ============================================================================
// Constructor / Initialization
// ============================================================================

TerrainChunkMeshGenerator::TerrainChunkMeshGenerator()
    : map_width_(0)
    , map_height_(0)
    , chunks_x_(0)
    , chunks_y_(0)
    , cliff_threshold_(DEFAULT_CLIFF_THRESHOLD)
    , skirt_height_(DEFAULT_SKIRT_HEIGHT)
    , initialized_(false)
    , rebuild_queue_()
    , temp_mesh_data_()
{
}

void TerrainChunkMeshGenerator::initialize(std::uint16_t map_width, std::uint16_t map_height) {
    map_width_ = map_width;
    map_height_ = map_height;
    chunks_x_ = (map_width + TILES_PER_CHUNK - 1) / TILES_PER_CHUNK;
    chunks_y_ = (map_height + TILES_PER_CHUNK - 1) / TILES_PER_CHUNK;
    initialized_ = true;

    // Pre-allocate temp buffer for typical chunk size
    // Surface: (32+1)^2 = 1089 vertices, 32*32*6 = 6144 indices
    // Cliff faces add extra geometry
    temp_mesh_data_.reserve(VERTICES_PER_CHUNK + 1024, INDICES_PER_CHUNK + 4096);
}

void TerrainChunkMeshGenerator::setCliffThreshold(std::uint8_t threshold) {
    cliff_threshold_ = threshold;
}

void TerrainChunkMeshGenerator::setSkirtHeight(float height) {
    // Clamp to valid range
    if (height < MIN_SKIRT_HEIGHT) {
        height = MIN_SKIRT_HEIGHT;
    } else if (height > MAX_SKIRT_HEIGHT) {
        height = MAX_SKIRT_HEIGHT;
    }
    skirt_height_ = height;
}

// ============================================================================
// Full Map Building
// ============================================================================

bool TerrainChunkMeshGenerator::buildAllChunks(
    SDL_GPUDevice* device,
    const TerrainGrid& grid,
    std::vector<TerrainChunk>& chunks
) {
    if (!initialized_) {
        LOG_ERROR("TerrainChunkMeshGenerator not initialized");
        return false;
    }

    if (device == nullptr) {
        LOG_ERROR("Null GPU device provided to buildAllChunks");
        return false;
    }

    std::uint32_t total_chunks = static_cast<std::uint32_t>(chunks_x_) * chunks_y_;
    if (chunks.size() != total_chunks) {
        LOG_ERROR("Chunk vector size mismatch: expected {}, got {}",
                  total_chunks, chunks.size());
        return false;
    }

    bool all_success = true;

    for (std::uint16_t cy = 0; cy < chunks_y_; ++cy) {
        for (std::uint16_t cx = 0; cx < chunks_x_; ++cx) {
            std::uint32_t chunk_index = static_cast<std::uint32_t>(cy) * chunks_x_ + cx;
            TerrainChunk& chunk = chunks[chunk_index];

            // Initialize chunk coordinates
            chunk.chunk_x = cx;
            chunk.chunk_y = cy;

            if (!rebuildChunk(device, grid, chunk)) {
                LOG_ERROR("Failed to build chunk ({}, {})", cx, cy);
                all_success = false;
            }
        }
    }

    return all_success;
}

// ============================================================================
// Incremental Updates
// ============================================================================

void TerrainChunkMeshGenerator::queueChunkRebuild(std::uint16_t chunk_x, std::uint16_t chunk_y) {
    // Check if already in queue to avoid duplicates
    auto coord = std::make_pair(chunk_x, chunk_y);
    for (const auto& queued : rebuild_queue_) {
        if (queued == coord) {
            return;  // Already queued
        }
    }
    rebuild_queue_.push_back(coord);
}

void TerrainChunkMeshGenerator::queueDirtyChunks(const ChunkDirtyTracker& tracker) {
    if (!tracker.isInitialized()) {
        return;
    }

    for (std::uint16_t cy = 0; cy < tracker.getChunksY(); ++cy) {
        for (std::uint16_t cx = 0; cx < tracker.getChunksX(); ++cx) {
            if (tracker.isChunkDirty(cx, cy)) {
                queueChunkRebuild(cx, cy);
            }
        }
    }
}

ChunkRebuildStats TerrainChunkMeshGenerator::updateDirtyChunks(
    SDL_GPUDevice* device,
    const TerrainGrid& grid,
    std::vector<TerrainChunk>& chunks,
    ChunkDirtyTracker& tracker
) {
    ChunkRebuildStats stats;
    stats.chunks_pending = static_cast<std::uint32_t>(rebuild_queue_.size());

    if (rebuild_queue_.empty()) {
        return stats;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Process at most MAX_CHUNKS_PER_FRAME chunks
    std::uint32_t chunks_to_process = std::min(
        static_cast<std::uint32_t>(rebuild_queue_.size()),
        MAX_CHUNKS_PER_FRAME
    );

    for (std::uint32_t i = 0; i < chunks_to_process; ++i) {
        auto coord = rebuild_queue_.front();
        rebuild_queue_.pop_front();

        std::uint16_t cx = coord.first;
        std::uint16_t cy = coord.second;

        // Validate chunk coordinates
        if (cx >= chunks_x_ || cy >= chunks_y_) {
            LOG_WARN("Invalid chunk coordinates in rebuild queue: ({}, {})", cx, cy);
            continue;
        }

        std::uint32_t chunk_index = static_cast<std::uint32_t>(cy) * chunks_x_ + cx;
        if (chunk_index >= chunks.size()) {
            LOG_WARN("Chunk index out of bounds: {}", chunk_index);
            continue;
        }

        TerrainChunk& chunk = chunks[chunk_index];

        // Generate mesh data
        if (generateChunkMesh(grid, cx, cy, temp_mesh_data_)) {
            // Upload to GPU
            if (uploadChunkMesh(device, temp_mesh_data_, chunk)) {
                stats.chunks_rebuilt++;
                stats.vertices_generated += static_cast<std::uint32_t>(temp_mesh_data_.vertices.size());
                stats.indices_generated += static_cast<std::uint32_t>(temp_mesh_data_.indices.size());

                // Clear dirty flag
                tracker.clearChunkDirty(cx, cy);
                chunk.clearDirty();
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    stats.rebuild_time_ms = static_cast<float>(duration.count()) / 1000.0f;
    stats.chunks_pending = static_cast<std::uint32_t>(rebuild_queue_.size());

    return stats;
}

// ============================================================================
// Single Chunk Operations
// ============================================================================

bool TerrainChunkMeshGenerator::generateChunkMesh(
    const TerrainGrid& grid,
    std::uint16_t chunk_x,
    std::uint16_t chunk_y,
    ChunkMeshData& out_data
) {
    out_data.clear();

    // Pre-allocate for expected size
    out_data.reserve(VERTICES_PER_CHUNK + 512, INDICES_PER_CHUNK + 2048);

    // Generate surface mesh
    generateSurfaceVertices(grid, chunk_x, chunk_y, out_data);
    generateSurfaceIndices(out_data);

    // Generate cliff faces for steep transitions
    std::uint32_t cliff_count = generateCliffFaces(grid, chunk_x, chunk_y, out_data);
    out_data.has_cliff_faces = (cliff_count > 0);

    return !out_data.vertices.empty();
}

bool TerrainChunkMeshGenerator::uploadChunkMesh(
    SDL_GPUDevice* device,
    const ChunkMeshData& mesh_data,
    TerrainChunk& chunk
) {
    if (device == nullptr) {
        LOG_ERROR("Null GPU device in uploadChunkMesh");
        return false;
    }

    if (mesh_data.vertices.empty() || mesh_data.indices.empty()) {
        LOG_WARN("Empty mesh data for chunk ({}, {})", chunk.chunk_x, chunk.chunk_y);
        return false;
    }

    // Calculate buffer sizes
    std::uint32_t vertex_count = static_cast<std::uint32_t>(mesh_data.vertices.size());
    std::uint32_t index_count = static_cast<std::uint32_t>(mesh_data.indices.size());
    std::uint32_t vertex_buffer_size = vertex_count * sizeof(TerrainVertex);
    std::uint32_t index_buffer_size = index_count * sizeof(std::uint32_t);

    // Release old buffers if they exist
    if (chunk.has_gpu_resources) {
        chunk.releaseGPUResources(device);
    }

    // Create vertex buffer
    SDL_GPUBufferCreateInfo vertex_buffer_info{};
    vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_buffer_info.size = vertex_buffer_size;
    vertex_buffer_info.props = 0;

    chunk.vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
    if (chunk.vertex_buffer == nullptr) {
        LOG_ERROR("Failed to create vertex buffer: {}", SDL_GetError());
        return false;
    }

    // Create index buffer
    SDL_GPUBufferCreateInfo index_buffer_info{};
    index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_buffer_info.size = index_buffer_size;
    index_buffer_info.props = 0;

    chunk.index_buffer = SDL_CreateGPUBuffer(device, &index_buffer_info);
    if (chunk.index_buffer == nullptr) {
        LOG_ERROR("Failed to create index buffer: {}", SDL_GetError());
        SDL_ReleaseGPUBuffer(device, chunk.vertex_buffer);
        chunk.vertex_buffer = nullptr;
        return false;
    }

    // Create transfer buffer for uploading data
    SDL_GPUTransferBufferCreateInfo transfer_info{};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = vertex_buffer_size + index_buffer_size;
    transfer_info.props = 0;

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (transfer_buffer == nullptr) {
        LOG_ERROR("Failed to create transfer buffer: {}", SDL_GetError());
        SDL_ReleaseGPUBuffer(device, chunk.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, chunk.index_buffer);
        chunk.vertex_buffer = nullptr;
        chunk.index_buffer = nullptr;
        return false;
    }

    // Map transfer buffer and copy data
    void* mapped = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (mapped == nullptr) {
        LOG_ERROR("Failed to map transfer buffer: {}", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        SDL_ReleaseGPUBuffer(device, chunk.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, chunk.index_buffer);
        chunk.vertex_buffer = nullptr;
        chunk.index_buffer = nullptr;
        return false;
    }

    // Copy vertex data
    std::memcpy(mapped, mesh_data.vertices.data(), vertex_buffer_size);

    // Copy index data (after vertices)
    std::memcpy(
        static_cast<std::uint8_t*>(mapped) + vertex_buffer_size,
        mesh_data.indices.data(),
        index_buffer_size
    );

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    // Create copy pass to upload data
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (cmd == nullptr) {
        LOG_ERROR("Failed to acquire command buffer: {}", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        SDL_ReleaseGPUBuffer(device, chunk.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, chunk.index_buffer);
        chunk.vertex_buffer = nullptr;
        chunk.index_buffer = nullptr;
        return false;
    }

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);
    if (copy_pass == nullptr) {
        LOG_ERROR("Failed to begin copy pass: {}", SDL_GetError());
        SDL_CancelGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        SDL_ReleaseGPUBuffer(device, chunk.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, chunk.index_buffer);
        chunk.vertex_buffer = nullptr;
        chunk.index_buffer = nullptr;
        return false;
    }

    // Upload vertex buffer
    SDL_GPUTransferBufferLocation vertex_src{};
    vertex_src.transfer_buffer = transfer_buffer;
    vertex_src.offset = 0;

    SDL_GPUBufferRegion vertex_dst{};
    vertex_dst.buffer = chunk.vertex_buffer;
    vertex_dst.offset = 0;
    vertex_dst.size = vertex_buffer_size;

    SDL_UploadToGPUBuffer(copy_pass, &vertex_src, &vertex_dst, false);

    // Upload index buffer
    SDL_GPUTransferBufferLocation index_src{};
    index_src.transfer_buffer = transfer_buffer;
    index_src.offset = vertex_buffer_size;

    SDL_GPUBufferRegion index_dst{};
    index_dst.buffer = chunk.index_buffer;
    index_dst.offset = 0;
    index_dst.size = index_buffer_size;

    SDL_UploadToGPUBuffer(copy_pass, &index_src, &index_dst, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);

    // Release transfer buffer
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    // Update chunk metadata
    chunk.vertex_count = vertex_count;
    chunk.index_count = index_count;
    chunk.has_gpu_resources = true;
    chunk.dirty = false;

    // Compute AABB from elevation data
    chunk.computeAABB(mesh_data.max_elevation);

    return true;
}

bool TerrainChunkMeshGenerator::rebuildChunk(
    SDL_GPUDevice* device,
    const TerrainGrid& grid,
    TerrainChunk& chunk
) {
    temp_mesh_data_.clear();

    if (!generateChunkMesh(grid, chunk.chunk_x, chunk.chunk_y, temp_mesh_data_)) {
        return false;
    }

    return uploadChunkMesh(device, temp_mesh_data_, chunk);
}

// ============================================================================
// Mesh Generation Helpers
// ============================================================================

void TerrainChunkMeshGenerator::generateSurfaceVertices(
    const TerrainGrid& grid,
    std::uint16_t chunk_x,
    std::uint16_t chunk_y,
    ChunkMeshData& out_data
) {
    // Calculate tile range for this chunk
    std::uint16_t tile_start_x = chunk_x * TILES_PER_CHUNK;
    std::uint16_t tile_start_y = chunk_y * TILES_PER_CHUNK;
    std::uint16_t tile_end_x = std::min<std::uint16_t>(
        tile_start_x + TILES_PER_CHUNK, grid.width);
    std::uint16_t tile_end_y = std::min<std::uint16_t>(
        tile_start_y + TILES_PER_CHUNK, grid.height);

    // We need vertices at tile corners: (tiles+1) x (tiles+1)
    std::uint16_t vertex_count_x = tile_end_x - tile_start_x + 1;
    std::uint16_t vertex_count_y = tile_end_y - tile_start_y + 1;

    out_data.max_elevation = 0;
    out_data.min_elevation = 255;

    // Generate vertices at each tile corner
    for (std::uint16_t vy = 0; vy < vertex_count_y; ++vy) {
        for (std::uint16_t vx = 0; vx < vertex_count_x; ++vx) {
            std::int32_t world_x = tile_start_x + vx;
            std::int32_t world_z = tile_start_y + vy;

            // Sample elevation at this vertex position
            // Vertices are at tile corners, so we sample the tile at (world_x, world_z)
            // For edge vertices, clamp to valid tile range
            std::int32_t sample_x = std::min<std::int32_t>(world_x, grid.width - 1);
            std::int32_t sample_z = std::min<std::int32_t>(world_z, grid.height - 1);

            const TerrainComponent& tile = grid.at(sample_x, sample_z);
            std::uint8_t elevation = tile.getElevation();
            std::uint8_t terrain_type = tile.terrain_type;

            // Track min/max elevation for AABB
            out_data.max_elevation = std::max(out_data.max_elevation, elevation);
            out_data.min_elevation = std::min(out_data.min_elevation, elevation);

            // Calculate world position
            float pos_x = static_cast<float>(world_x);
            float pos_y = static_cast<float>(elevation) * ELEVATION_HEIGHT;
            float pos_z = static_cast<float>(world_z);

            // Compute normal via central differences
            NormalResult normal = computeTerrainNormal(grid, world_x, world_z);

            // Calculate UV coordinates (normalized within chunk)
            float uv_u = static_cast<float>(vx) / static_cast<float>(TILES_PER_CHUNK);
            float uv_v = static_cast<float>(vy) / static_cast<float>(TILES_PER_CHUNK);

            // Create vertex
            TerrainVertex vertex(
                pos_x, pos_y, pos_z,
                normal.nx, normal.ny, normal.nz,
                terrain_type, elevation,
                uv_u, uv_v,
                static_cast<float>(world_x), static_cast<float>(world_z)
            );

            out_data.vertices.push_back(vertex);
        }
    }
}

void TerrainChunkMeshGenerator::generateSurfaceIndices(ChunkMeshData& out_data) {
    // Number of vertices per row
    std::uint32_t vertices_per_row = TILES_PER_CHUNK + 1;

    // Generate two triangles per tile (quad)
    for (std::uint16_t ty = 0; ty < TILES_PER_CHUNK; ++ty) {
        for (std::uint16_t tx = 0; tx < TILES_PER_CHUNK; ++tx) {
            // Get the four corners of this tile
            std::uint32_t top_left = getSurfaceVertexIndex(tx, ty);
            std::uint32_t top_right = getSurfaceVertexIndex(tx + 1, ty);
            std::uint32_t bottom_left = getSurfaceVertexIndex(tx, ty + 1);
            std::uint32_t bottom_right = getSurfaceVertexIndex(tx + 1, ty + 1);

            // Ensure indices are valid
            if (top_left >= out_data.vertices.size() ||
                top_right >= out_data.vertices.size() ||
                bottom_left >= out_data.vertices.size() ||
                bottom_right >= out_data.vertices.size()) {
                continue;  // Skip if vertex indices are out of bounds
            }

            // Triangle 1: top-left, bottom-left, bottom-right (CCW)
            out_data.indices.push_back(top_left);
            out_data.indices.push_back(bottom_left);
            out_data.indices.push_back(bottom_right);

            // Triangle 2: top-left, bottom-right, top-right (CCW)
            out_data.indices.push_back(top_left);
            out_data.indices.push_back(bottom_right);
            out_data.indices.push_back(top_right);
        }
    }
}

std::uint32_t TerrainChunkMeshGenerator::generateCliffFaces(
    const TerrainGrid& grid,
    std::uint16_t chunk_x,
    std::uint16_t chunk_y,
    ChunkMeshData& out_data
) {
    if (cliff_threshold_ == 0) {
        return 0;  // Cliff faces disabled
    }

    std::uint32_t cliff_count = 0;

    // Calculate tile range for this chunk
    std::uint16_t tile_start_x = chunk_x * TILES_PER_CHUNK;
    std::uint16_t tile_start_y = chunk_y * TILES_PER_CHUNK;
    std::uint16_t tile_end_x = std::min<std::uint16_t>(
        tile_start_x + TILES_PER_CHUNK, grid.width);
    std::uint16_t tile_end_y = std::min<std::uint16_t>(
        tile_start_y + TILES_PER_CHUNK, grid.height);

    // Check each tile's edges for cliff faces
    for (std::uint16_t ty = tile_start_y; ty < tile_end_y; ++ty) {
        for (std::uint16_t tx = tile_start_x; tx < tile_end_x; ++tx) {
            std::uint8_t elevation = grid.at(tx, ty).getElevation();
            std::uint8_t terrain_type = grid.at(tx, ty).terrain_type;

            // Check right edge (positive X direction)
            if (tx + 1 < grid.width) {
                std::uint8_t neighbor_elevation = grid.at(tx + 1, ty).getElevation();
                std::int16_t delta = static_cast<std::int16_t>(elevation) -
                                     static_cast<std::int16_t>(neighbor_elevation);

                if (std::abs(delta) >= cliff_threshold_) {
                    // Cliff face on right edge
                    std::uint8_t high = std::max(elevation, neighbor_elevation);
                    std::uint8_t low = std::min(elevation, neighbor_elevation);

                    // Determine normal direction (pointing away from higher tile)
                    float normal_x = (elevation > neighbor_elevation) ? 1.0f : -1.0f;

                    generateCliffFaceQuad(
                        static_cast<float>(tx + 1), static_cast<float>(ty),
                        static_cast<float>(tx + 1), static_cast<float>(ty + 1),
                        high, low, terrain_type,
                        normal_x, 0.0f,
                        out_data
                    );
                    cliff_count++;
                }
            }

            // Check bottom edge (positive Z direction)
            if (ty + 1 < grid.height) {
                std::uint8_t neighbor_elevation = grid.at(tx, ty + 1).getElevation();
                std::int16_t delta = static_cast<std::int16_t>(elevation) -
                                     static_cast<std::int16_t>(neighbor_elevation);

                if (std::abs(delta) >= cliff_threshold_) {
                    // Cliff face on bottom edge
                    std::uint8_t high = std::max(elevation, neighbor_elevation);
                    std::uint8_t low = std::min(elevation, neighbor_elevation);

                    // Determine normal direction (pointing away from higher tile)
                    float normal_z = (elevation > neighbor_elevation) ? 1.0f : -1.0f;

                    generateCliffFaceQuad(
                        static_cast<float>(tx), static_cast<float>(ty + 1),
                        static_cast<float>(tx + 1), static_cast<float>(ty + 1),
                        high, low, terrain_type,
                        0.0f, normal_z,
                        out_data
                    );
                    cliff_count++;
                }
            }
        }
    }

    return cliff_count;
}

void TerrainChunkMeshGenerator::generateCliffFaceQuad(
    float x1, float z1,
    float x2, float z2,
    std::uint8_t high_elevation,
    std::uint8_t low_elevation,
    std::uint8_t terrain_type,
    float normal_x, float normal_z,
    ChunkMeshData& out_data
) {
    // Calculate Y positions
    float high_y = static_cast<float>(high_elevation) * ELEVATION_HEIGHT;
    float low_y = static_cast<float>(low_elevation) * ELEVATION_HEIGHT;

    // Cliff face normals are horizontal (ny = 0) for distinct toon shader shadow bands
    float normal_y = 0.0f;

    // Base index for new vertices
    std::uint32_t base_index = static_cast<std::uint32_t>(out_data.vertices.size());

    // Create 4 vertices for the cliff face quad
    // UV coordinates: u = 0/1 along edge, v = 0 at top, 1 at bottom

    // Vertex 0: start, top
    TerrainVertex v0(
        x1, high_y, z1,
        normal_x, normal_y, normal_z,
        terrain_type, high_elevation,
        0.0f, 0.0f,
        x1, z1
    );
    out_data.vertices.push_back(v0);

    // Vertex 1: end, top
    TerrainVertex v1(
        x2, high_y, z2,
        normal_x, normal_y, normal_z,
        terrain_type, high_elevation,
        1.0f, 0.0f,
        x2, z2
    );
    out_data.vertices.push_back(v1);

    // Vertex 2: start, bottom
    TerrainVertex v2(
        x1, low_y, z1,
        normal_x, normal_y, normal_z,
        terrain_type, low_elevation,
        0.0f, 1.0f,
        x1, z1
    );
    out_data.vertices.push_back(v2);

    // Vertex 3: end, bottom
    TerrainVertex v3(
        x2, low_y, z2,
        normal_x, normal_y, normal_z,
        terrain_type, low_elevation,
        1.0f, 1.0f,
        x2, z2
    );
    out_data.vertices.push_back(v3);

    // Generate two triangles for the quad
    // Winding order depends on normal direction to ensure back-face culling works correctly
    if (normal_x > 0 || normal_z > 0) {
        // Normal pointing in positive direction
        // Triangle 1: 0, 2, 3 (CCW when viewed from positive side)
        out_data.indices.push_back(base_index + 0);
        out_data.indices.push_back(base_index + 2);
        out_data.indices.push_back(base_index + 3);

        // Triangle 2: 0, 3, 1
        out_data.indices.push_back(base_index + 0);
        out_data.indices.push_back(base_index + 3);
        out_data.indices.push_back(base_index + 1);
    } else {
        // Normal pointing in negative direction
        // Triangle 1: 0, 3, 2 (CCW when viewed from negative side)
        out_data.indices.push_back(base_index + 0);
        out_data.indices.push_back(base_index + 3);
        out_data.indices.push_back(base_index + 2);

        // Triangle 2: 0, 1, 3
        out_data.indices.push_back(base_index + 0);
        out_data.indices.push_back(base_index + 1);
        out_data.indices.push_back(base_index + 3);
    }
}

// ============================================================================
// LOD Mesh Generation (Ticket 3-032)
// ============================================================================

bool TerrainChunkMeshGenerator::generateLODMesh(
    const TerrainGrid& grid,
    std::uint16_t chunk_x,
    std::uint16_t chunk_y,
    std::uint8_t lod_level,
    ChunkMeshData& out_data
) {
    if (lod_level >= TERRAIN_LOD_LEVEL_COUNT) {
        LOG_ERROR("Invalid LOD level: {}", lod_level);
        return false;
    }

    out_data.clear();

    // Pre-allocate for expected size at this LOD level including skirt geometry
    std::uint32_t expected_vertices = getVertexCount(lod_level) + getTotalSkirtVertexCount(lod_level);
    std::uint32_t expected_indices = getIndexCount(lod_level) + getTotalSkirtIndexCount(lod_level);
    out_data.reserve(expected_vertices, expected_indices);

    // Generate LOD surface mesh (no cliff faces at lower LOD levels)
    generateLODSurfaceVertices(grid, chunk_x, chunk_y, lod_level, out_data);
    generateLODSurfaceIndices(lod_level, out_data);

    // Generate skirt geometry for LOD seam mitigation (Ticket 3-033)
    if (skirt_height_ > 0.0f) {
        generateLODSkirtGeometry(grid, chunk_x, chunk_y, lod_level, skirt_height_, out_data);
    }

    return !out_data.vertices.empty();
}

bool TerrainChunkMeshGenerator::generateAllLODMeshes(
    const TerrainGrid& grid,
    std::uint16_t chunk_x,
    std::uint16_t chunk_y,
    ChunkMeshData out_lod_data[TERRAIN_LOD_LEVEL_COUNT]
) {
    bool all_success = true;

    for (std::uint8_t level = 0; level < TERRAIN_LOD_LEVEL_COUNT; ++level) {
        if (!generateLODMesh(grid, chunk_x, chunk_y, level, out_lod_data[level])) {
            LOG_ERROR("Failed to generate LOD {} mesh for chunk ({}, {})",
                      level, chunk_x, chunk_y);
            all_success = false;
        }
    }

    return all_success;
}

bool TerrainChunkMeshGenerator::uploadLODMesh(
    SDL_GPUDevice* device,
    const ChunkMeshData& mesh_data,
    TerrainLODMesh& lod_mesh,
    std::uint8_t lod_level
) {
    if (device == nullptr) {
        LOG_ERROR("Null GPU device in uploadLODMesh");
        return false;
    }

    if (lod_level >= TERRAIN_LOD_LEVEL_COUNT) {
        LOG_ERROR("Invalid LOD level: {}", lod_level);
        return false;
    }

    if (mesh_data.vertices.empty() || mesh_data.indices.empty()) {
        LOG_WARN("Empty mesh data for LOD {} of chunk ({}, {})",
                 lod_level, lod_mesh.chunk_x, lod_mesh.chunk_y);
        return false;
    }

    TerrainLODLevel& level = lod_mesh.levels[lod_level];

    // Calculate buffer sizes
    std::uint32_t vertex_count = static_cast<std::uint32_t>(mesh_data.vertices.size());
    std::uint32_t index_count = static_cast<std::uint32_t>(mesh_data.indices.size());
    std::uint32_t vertex_buffer_size = vertex_count * sizeof(TerrainVertex);
    std::uint32_t index_buffer_size = index_count * sizeof(std::uint32_t);

    // Release old buffers if they exist
    if (level.vertex_buffer != nullptr) {
        SDL_ReleaseGPUBuffer(device, level.vertex_buffer);
        level.vertex_buffer = nullptr;
    }
    if (level.index_buffer != nullptr) {
        SDL_ReleaseGPUBuffer(device, level.index_buffer);
        level.index_buffer = nullptr;
    }

    // Create vertex buffer
    SDL_GPUBufferCreateInfo vertex_buffer_info{};
    vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_buffer_info.size = vertex_buffer_size;
    vertex_buffer_info.props = 0;

    level.vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
    if (level.vertex_buffer == nullptr) {
        LOG_ERROR("Failed to create LOD {} vertex buffer: {}", lod_level, SDL_GetError());
        return false;
    }

    // Create index buffer
    SDL_GPUBufferCreateInfo index_buffer_info{};
    index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_buffer_info.size = index_buffer_size;
    index_buffer_info.props = 0;

    level.index_buffer = SDL_CreateGPUBuffer(device, &index_buffer_info);
    if (level.index_buffer == nullptr) {
        LOG_ERROR("Failed to create LOD {} index buffer: {}", lod_level, SDL_GetError());
        SDL_ReleaseGPUBuffer(device, level.vertex_buffer);
        level.vertex_buffer = nullptr;
        return false;
    }

    // Create transfer buffer for uploading data
    SDL_GPUTransferBufferCreateInfo transfer_info{};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = vertex_buffer_size + index_buffer_size;
    transfer_info.props = 0;

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (transfer_buffer == nullptr) {
        LOG_ERROR("Failed to create transfer buffer for LOD {}: {}", lod_level, SDL_GetError());
        SDL_ReleaseGPUBuffer(device, level.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, level.index_buffer);
        level.vertex_buffer = nullptr;
        level.index_buffer = nullptr;
        return false;
    }

    // Map transfer buffer and copy data
    void* mapped = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (mapped == nullptr) {
        LOG_ERROR("Failed to map transfer buffer for LOD {}: {}", lod_level, SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        SDL_ReleaseGPUBuffer(device, level.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, level.index_buffer);
        level.vertex_buffer = nullptr;
        level.index_buffer = nullptr;
        return false;
    }

    // Copy vertex data
    std::memcpy(mapped, mesh_data.vertices.data(), vertex_buffer_size);

    // Copy index data (after vertices)
    std::memcpy(
        static_cast<std::uint8_t*>(mapped) + vertex_buffer_size,
        mesh_data.indices.data(),
        index_buffer_size
    );

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    // Create copy pass to upload data
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (cmd == nullptr) {
        LOG_ERROR("Failed to acquire command buffer for LOD {}: {}", lod_level, SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        SDL_ReleaseGPUBuffer(device, level.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, level.index_buffer);
        level.vertex_buffer = nullptr;
        level.index_buffer = nullptr;
        return false;
    }

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);
    if (copy_pass == nullptr) {
        LOG_ERROR("Failed to begin copy pass for LOD {}: {}", lod_level, SDL_GetError());
        SDL_CancelGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        SDL_ReleaseGPUBuffer(device, level.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, level.index_buffer);
        level.vertex_buffer = nullptr;
        level.index_buffer = nullptr;
        return false;
    }

    // Upload vertex buffer
    SDL_GPUTransferBufferLocation vertex_src{};
    vertex_src.transfer_buffer = transfer_buffer;
    vertex_src.offset = 0;

    SDL_GPUBufferRegion vertex_dst{};
    vertex_dst.buffer = level.vertex_buffer;
    vertex_dst.offset = 0;
    vertex_dst.size = vertex_buffer_size;

    SDL_UploadToGPUBuffer(copy_pass, &vertex_src, &vertex_dst, false);

    // Upload index buffer
    SDL_GPUTransferBufferLocation index_src{};
    index_src.transfer_buffer = transfer_buffer;
    index_src.offset = vertex_buffer_size;

    SDL_GPUBufferRegion index_dst{};
    index_dst.buffer = level.index_buffer;
    index_dst.offset = 0;
    index_dst.size = index_buffer_size;

    SDL_UploadToGPUBuffer(copy_pass, &index_src, &index_dst, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);

    // Release transfer buffer
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    // Update level metadata
    level.vertex_count = vertex_count;
    level.index_count = index_count;

    return true;
}

bool TerrainChunkMeshGenerator::rebuildAllLODLevels(
    SDL_GPUDevice* device,
    const TerrainGrid& grid,
    TerrainLODMesh& lod_mesh
) {
    if (!initialized_) {
        LOG_ERROR("TerrainChunkMeshGenerator not initialized");
        return false;
    }

    if (device == nullptr) {
        LOG_ERROR("Null GPU device in rebuildAllLODLevels");
        return false;
    }

    // Generate all LOD levels
    ChunkMeshData lod_data[TERRAIN_LOD_LEVEL_COUNT];
    if (!generateAllLODMeshes(grid, lod_mesh.chunk_x, lod_mesh.chunk_y, lod_data)) {
        return false;
    }

    // Upload all LOD levels
    bool all_success = true;
    for (std::uint8_t level = 0; level < TERRAIN_LOD_LEVEL_COUNT; ++level) {
        if (!uploadLODMesh(device, lod_data[level], lod_mesh, level)) {
            LOG_ERROR("Failed to upload LOD {} for chunk ({}, {})",
                      level, lod_mesh.chunk_x, lod_mesh.chunk_y);
            all_success = false;
        }
    }

    if (all_success) {
        // Compute AABB from LOD 0 data (most accurate)
        lod_mesh.aabb.min = glm::vec3(
            static_cast<float>(lod_mesh.chunk_x * TILES_PER_CHUNK),
            0.0f,
            static_cast<float>(lod_mesh.chunk_y * TILES_PER_CHUNK)
        );
        lod_mesh.aabb.max = glm::vec3(
            static_cast<float>((lod_mesh.chunk_x + 1) * TILES_PER_CHUNK),
            static_cast<float>(lod_data[0].max_elevation) * ELEVATION_HEIGHT,
            static_cast<float>((lod_mesh.chunk_y + 1) * TILES_PER_CHUNK)
        );
        lod_mesh.complete = true;
    }

    return all_success;
}

bool TerrainChunkMeshGenerator::buildAllLODMeshes(
    SDL_GPUDevice* device,
    const TerrainGrid& grid,
    std::vector<TerrainLODMesh>& lod_meshes
) {
    if (!initialized_) {
        LOG_ERROR("TerrainChunkMeshGenerator not initialized");
        return false;
    }

    if (device == nullptr) {
        LOG_ERROR("Null GPU device in buildAllLODMeshes");
        return false;
    }

    std::uint32_t total_chunks = static_cast<std::uint32_t>(chunks_x_) * chunks_y_;
    if (lod_meshes.size() != total_chunks) {
        LOG_ERROR("LOD mesh vector size mismatch: expected {}, got {}",
                  total_chunks, lod_meshes.size());
        return false;
    }

    bool all_success = true;

    for (std::uint16_t cy = 0; cy < chunks_y_; ++cy) {
        for (std::uint16_t cx = 0; cx < chunks_x_; ++cx) {
            std::uint32_t chunk_index = static_cast<std::uint32_t>(cy) * chunks_x_ + cx;
            TerrainLODMesh& lod_mesh = lod_meshes[chunk_index];

            // Initialize chunk coordinates
            lod_mesh.chunk_x = cx;
            lod_mesh.chunk_y = cy;

            if (!rebuildAllLODLevels(device, grid, lod_mesh)) {
                LOG_ERROR("Failed to build LOD mesh for chunk ({}, {})", cx, cy);
                all_success = false;
            }
        }
    }

    return all_success;
}

// ============================================================================
// LOD Mesh Generation Helpers
// ============================================================================

void TerrainChunkMeshGenerator::generateLODSurfaceVertices(
    const TerrainGrid& grid,
    std::uint16_t chunk_x,
    std::uint16_t chunk_y,
    std::uint8_t lod_level,
    ChunkMeshData& out_data
) {
    // Get LOD parameters
    std::uint8_t step = getLODStep(lod_level);
    std::uint32_t grid_size = getVertexGridSize(lod_level);

    // Calculate tile range for this chunk
    std::uint16_t tile_start_x = chunk_x * TILES_PER_CHUNK;
    std::uint16_t tile_start_y = chunk_y * TILES_PER_CHUNK;
    std::uint16_t tile_end_x = std::min<std::uint16_t>(
        tile_start_x + TILES_PER_CHUNK, grid.width);
    std::uint16_t tile_end_y = std::min<std::uint16_t>(
        tile_start_y + TILES_PER_CHUNK, grid.height);

    out_data.max_elevation = 0;
    out_data.min_elevation = 255;

    // Generate vertices at subsampled tile corners
    for (std::uint32_t vy = 0; vy < grid_size; ++vy) {
        for (std::uint32_t vx = 0; vx < grid_size; ++vx) {
            // Calculate world position from LOD grid position
            std::int32_t world_x = tile_start_x + static_cast<std::int32_t>(vx * step);
            std::int32_t world_z = tile_start_y + static_cast<std::int32_t>(vy * step);

            // Clamp to grid boundaries
            std::int32_t sample_x = std::min<std::int32_t>(world_x, grid.width - 1);
            std::int32_t sample_z = std::min<std::int32_t>(world_z, grid.height - 1);

            const TerrainComponent& tile = grid.at(sample_x, sample_z);
            std::uint8_t elevation = tile.getElevation();
            std::uint8_t terrain_type = tile.terrain_type;

            // Track min/max elevation for AABB
            out_data.max_elevation = std::max(out_data.max_elevation, elevation);
            out_data.min_elevation = std::min(out_data.min_elevation, elevation);

            // Calculate world position
            float pos_x = static_cast<float>(world_x);
            float pos_y = static_cast<float>(elevation) * ELEVATION_HEIGHT;
            float pos_z = static_cast<float>(world_z);

            // Compute normal via central differences
            // For LOD levels, use the step size for neighbor sampling
            NormalResult normal = computeTerrainNormalWithSampler(
                [&grid](std::int32_t x, std::int32_t z) -> float {
                    std::int32_t sx = std::max<std::int32_t>(0, std::min<std::int32_t>(x, grid.width - 1));
                    std::int32_t sz = std::max<std::int32_t>(0, std::min<std::int32_t>(z, grid.height - 1));
                    return static_cast<float>(grid.at(sx, sz).getElevation()) * ELEVATION_HEIGHT;
                },
                world_x, world_z,
                grid.width, grid.height
            );

            // Calculate UV coordinates (normalized within chunk)
            float uv_u = static_cast<float>(vx) / static_cast<float>(grid_size - 1);
            float uv_v = static_cast<float>(vy) / static_cast<float>(grid_size - 1);

            // Create vertex
            TerrainVertex vertex(
                pos_x, pos_y, pos_z,
                normal.nx, normal.ny, normal.nz,
                terrain_type, elevation,
                uv_u, uv_v,
                static_cast<float>(world_x), static_cast<float>(world_z)
            );

            out_data.vertices.push_back(vertex);
        }
    }
}

void TerrainChunkMeshGenerator::generateLODSurfaceIndices(
    std::uint8_t lod_level,
    ChunkMeshData& out_data
) {
    std::uint32_t grid_size = getVertexGridSize(lod_level);
    std::uint32_t tiles_per_side = getTilesPerSide(lod_level);

    // Generate two triangles per tile (quad)
    for (std::uint32_t ty = 0; ty < tiles_per_side; ++ty) {
        for (std::uint32_t tx = 0; tx < tiles_per_side; ++tx) {
            // Get the four corners of this tile in the LOD grid
            std::uint32_t top_left = getLODSurfaceVertexIndex(
                static_cast<std::uint16_t>(tx),
                static_cast<std::uint16_t>(ty),
                grid_size);
            std::uint32_t top_right = getLODSurfaceVertexIndex(
                static_cast<std::uint16_t>(tx + 1),
                static_cast<std::uint16_t>(ty),
                grid_size);
            std::uint32_t bottom_left = getLODSurfaceVertexIndex(
                static_cast<std::uint16_t>(tx),
                static_cast<std::uint16_t>(ty + 1),
                grid_size);
            std::uint32_t bottom_right = getLODSurfaceVertexIndex(
                static_cast<std::uint16_t>(tx + 1),
                static_cast<std::uint16_t>(ty + 1),
                grid_size);

            // Ensure indices are valid
            if (top_left >= out_data.vertices.size() ||
                top_right >= out_data.vertices.size() ||
                bottom_left >= out_data.vertices.size() ||
                bottom_right >= out_data.vertices.size()) {
                continue;  // Skip if vertex indices are out of bounds
            }

            // Triangle 1: top-left, bottom-left, bottom-right (CCW)
            out_data.indices.push_back(top_left);
            out_data.indices.push_back(bottom_left);
            out_data.indices.push_back(bottom_right);

            // Triangle 2: top-left, bottom-right, top-right (CCW)
            out_data.indices.push_back(top_left);
            out_data.indices.push_back(bottom_right);
            out_data.indices.push_back(top_right);
        }
    }
}

// ============================================================================
// Skirt Geometry Generation (Ticket 3-033 - LOD Seam Mitigation)
// ============================================================================

void TerrainChunkMeshGenerator::generateLODSkirtGeometry(
    const TerrainGrid& grid,
    std::uint16_t chunk_x,
    std::uint16_t chunk_y,
    std::uint8_t lod_level,
    float skirt_height,
    ChunkMeshData& out_data
) {
    // Generate skirt for all 4 edges
    // Edge 0 = North (Z min), Edge 1 = East (X max)
    // Edge 2 = South (Z max), Edge 3 = West (X min)
    for (std::uint8_t edge = 0; edge < 4; ++edge) {
        generateLODSkirtEdge(grid, chunk_x, chunk_y, lod_level, edge, skirt_height, out_data);
    }
}

void TerrainChunkMeshGenerator::generateLODSkirtEdge(
    const TerrainGrid& grid,
    std::uint16_t chunk_x,
    std::uint16_t chunk_y,
    std::uint8_t lod_level,
    std::uint8_t edge,
    float skirt_height,
    ChunkMeshData& out_data
) {
    // Get LOD parameters
    std::uint8_t step = getLODStep(lod_level);
    std::uint32_t grid_size = getVertexGridSize(lod_level);

    // Calculate tile range for this chunk
    std::uint16_t tile_start_x = chunk_x * TILES_PER_CHUNK;
    std::uint16_t tile_start_y = chunk_y * TILES_PER_CHUNK;

    // Record base index before adding skirt vertices
    std::uint32_t skirt_base_index = static_cast<std::uint32_t>(out_data.vertices.size());

    // Generate skirt vertices along the edge
    // Each skirt vertex is a copy of the surface edge vertex, extended downward
    for (std::uint32_t i = 0; i < grid_size; ++i) {
        // Determine world coordinates based on edge
        std::int32_t world_x, world_z;

        switch (edge) {
            case 0:  // North edge (Z min) - local_y = 0
                world_x = tile_start_x + static_cast<std::int32_t>(i * step);
                world_z = tile_start_y;
                break;
            case 1:  // East edge (X max) - local_x = grid_size - 1
                world_x = tile_start_x + TILES_PER_CHUNK;
                world_z = tile_start_y + static_cast<std::int32_t>(i * step);
                break;
            case 2:  // South edge (Z max) - local_y = grid_size - 1
                world_x = tile_start_x + static_cast<std::int32_t>(i * step);
                world_z = tile_start_y + TILES_PER_CHUNK;
                break;
            case 3:  // West edge (X min) - local_x = 0
            default:
                world_x = tile_start_x;
                world_z = tile_start_y + static_cast<std::int32_t>(i * step);
                break;
        }

        // Clamp to grid boundaries for sampling
        std::int32_t sample_x = std::min<std::int32_t>(world_x, grid.width - 1);
        std::int32_t sample_z = std::min<std::int32_t>(world_z, grid.height - 1);

        const TerrainComponent& tile = grid.at(sample_x, sample_z);
        std::uint8_t elevation = tile.getElevation();
        std::uint8_t terrain_type = tile.terrain_type;

        // Surface position
        float surface_y = static_cast<float>(elevation) * ELEVATION_HEIGHT;

        // Skirt position (extended downward)
        float skirt_y = surface_y - skirt_height;

        // Clamp skirt Y to ensure it doesn't go below 0
        if (skirt_y < 0.0f) {
            skirt_y = 0.0f;
        }

        // Compute normal - skirt normals point outward from the chunk edge
        // to ensure proper lighting and back-face culling
        float normal_x = 0.0f;
        float normal_y = 0.0f;
        float normal_z = 0.0f;

        switch (edge) {
            case 0:  // North edge - normal points -Z
                normal_z = -1.0f;
                break;
            case 1:  // East edge - normal points +X
                normal_x = 1.0f;
                break;
            case 2:  // South edge - normal points +Z
                normal_z = 1.0f;
                break;
            case 3:  // West edge - normal points -X
            default:
                normal_x = -1.0f;
                break;
        }

        // UV coordinates for skirt (consistent with edge)
        float uv_u, uv_v;
        if (edge == 0 || edge == 2) {
            // Horizontal edges - U varies, V is 0 (top) or 1 (bottom)
            uv_u = static_cast<float>(i) / static_cast<float>(grid_size - 1);
            uv_v = 1.0f;  // Skirts are at the bottom of the UV
        } else {
            // Vertical edges - V varies, U is 0 (left) or 1 (right)
            uv_u = 1.0f;  // Skirts are at the edge of the UV
            uv_v = static_cast<float>(i) / static_cast<float>(grid_size - 1);
        }

        // Create skirt vertex (extended downward from edge)
        TerrainVertex skirt_vertex(
            static_cast<float>(world_x), skirt_y, static_cast<float>(world_z),
            normal_x, normal_y, normal_z,
            terrain_type, elevation,
            uv_u, uv_v,
            static_cast<float>(world_x), static_cast<float>(world_z)
        );

        out_data.vertices.push_back(skirt_vertex);
    }

    // Generate skirt triangles connecting surface edge to skirt edge
    // Need to find the surface vertex indices for this edge
    for (std::uint32_t i = 0; i < grid_size - 1; ++i) {
        // Surface vertex indices on this edge
        std::uint32_t surface_i, surface_i_plus_1;

        switch (edge) {
            case 0:  // North edge - local_y = 0, local_x varies
                surface_i = getLODSurfaceVertexIndex(
                    static_cast<std::uint16_t>(i), 0, grid_size);
                surface_i_plus_1 = getLODSurfaceVertexIndex(
                    static_cast<std::uint16_t>(i + 1), 0, grid_size);
                break;
            case 1:  // East edge - local_x = grid_size - 1, local_y varies
                surface_i = getLODSurfaceVertexIndex(
                    static_cast<std::uint16_t>(grid_size - 1),
                    static_cast<std::uint16_t>(i), grid_size);
                surface_i_plus_1 = getLODSurfaceVertexIndex(
                    static_cast<std::uint16_t>(grid_size - 1),
                    static_cast<std::uint16_t>(i + 1), grid_size);
                break;
            case 2:  // South edge - local_y = grid_size - 1, local_x varies
                surface_i = getLODSurfaceVertexIndex(
                    static_cast<std::uint16_t>(i),
                    static_cast<std::uint16_t>(grid_size - 1), grid_size);
                surface_i_plus_1 = getLODSurfaceVertexIndex(
                    static_cast<std::uint16_t>(i + 1),
                    static_cast<std::uint16_t>(grid_size - 1), grid_size);
                break;
            case 3:  // West edge - local_x = 0, local_y varies
            default:
                surface_i = getLODSurfaceVertexIndex(
                    0, static_cast<std::uint16_t>(i), grid_size);
                surface_i_plus_1 = getLODSurfaceVertexIndex(
                    0, static_cast<std::uint16_t>(i + 1), grid_size);
                break;
        }

        // Skirt vertex indices (just added)
        std::uint32_t skirt_i = skirt_base_index + i;
        std::uint32_t skirt_i_plus_1 = skirt_base_index + i + 1;

        // Create two triangles for the skirt quad
        // Winding order is set so the skirt faces outward from the chunk
        // The skirt "hangs down" from the surface edge

        switch (edge) {
            case 0:  // North edge - faces outward (-Z), winding: surface-to-skirt left-to-right
                // Triangle 1: surface_i, skirt_i, skirt_i+1
                out_data.indices.push_back(surface_i);
                out_data.indices.push_back(skirt_i);
                out_data.indices.push_back(skirt_i_plus_1);
                // Triangle 2: surface_i, skirt_i+1, surface_i+1
                out_data.indices.push_back(surface_i);
                out_data.indices.push_back(skirt_i_plus_1);
                out_data.indices.push_back(surface_i_plus_1);
                break;

            case 1:  // East edge - faces outward (+X), winding: surface-to-skirt top-to-bottom
                // Triangle 1: surface_i, surface_i+1, skirt_i
                out_data.indices.push_back(surface_i);
                out_data.indices.push_back(surface_i_plus_1);
                out_data.indices.push_back(skirt_i);
                // Triangle 2: skirt_i, surface_i+1, skirt_i+1
                out_data.indices.push_back(skirt_i);
                out_data.indices.push_back(surface_i_plus_1);
                out_data.indices.push_back(skirt_i_plus_1);
                break;

            case 2:  // South edge - faces outward (+Z), winding: surface-to-skirt right-to-left
                // Triangle 1: surface_i, surface_i+1, skirt_i
                out_data.indices.push_back(surface_i);
                out_data.indices.push_back(surface_i_plus_1);
                out_data.indices.push_back(skirt_i);
                // Triangle 2: skirt_i, surface_i+1, skirt_i+1
                out_data.indices.push_back(skirt_i);
                out_data.indices.push_back(surface_i_plus_1);
                out_data.indices.push_back(skirt_i_plus_1);
                break;

            case 3:  // West edge - faces outward (-X), winding: surface-to-skirt bottom-to-top
            default:
                // Triangle 1: surface_i, skirt_i, skirt_i+1
                out_data.indices.push_back(surface_i);
                out_data.indices.push_back(skirt_i);
                out_data.indices.push_back(skirt_i_plus_1);
                // Triangle 2: surface_i, skirt_i+1, surface_i+1
                out_data.indices.push_back(surface_i);
                out_data.indices.push_back(skirt_i_plus_1);
                out_data.indices.push_back(surface_i_plus_1);
                break;
        }
    }
}

} // namespace terrain
} // namespace sims3000
