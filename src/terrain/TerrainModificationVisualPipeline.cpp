/**
 * @file TerrainModificationVisualPipeline.cpp
 * @brief Implementation of terrain modification visual update pipeline.
 *
 * Coordinates visual updates when terrain is modified, including:
 * - Terrain chunk mesh rebuilding
 * - Vegetation instance regeneration
 * - Water mesh regeneration
 */

#include <sims3000/terrain/TerrainModificationVisualPipeline.h>
#include <sims3000/core/Logger.h>
#include <algorithm>
#include <chrono>

namespace sims3000 {
namespace terrain {

// ============================================================================
// Constructor / Destructor
// ============================================================================

TerrainModificationVisualPipeline::TerrainModificationVisualPipeline()
    : initialized_(false)
    , device_(nullptr)
    , grid_(nullptr)
    , water_data_(nullptr)
    , chunks_(nullptr)
    , water_meshes_(nullptr)
    , mesh_generator_()
    , dirty_tracker_()
    , map_seed_(0)
    , pending_vegetation_chunks_()
    , pending_water_bodies_()
    , vegetation_cache_()
    , chunks_x_(0)
    , chunks_y_(0)
{
}

TerrainModificationVisualPipeline::~TerrainModificationVisualPipeline() {
    // Note: We don't own the external references, so nothing to clean up
}

// ============================================================================
// Initialization
// ============================================================================

bool TerrainModificationVisualPipeline::initialize(
    SDL_GPUDevice* device,
    TerrainGrid& grid,
    WaterData& water_data,
    std::vector<TerrainChunk>& chunks,
    std::vector<WaterMesh>& water_meshes,
    std::uint64_t map_seed
) {
    if (device == nullptr) {
        LOG_ERROR("Null GPU device provided to TerrainModificationVisualPipeline");
        return false;
    }

    if (grid.empty()) {
        LOG_ERROR("Empty terrain grid provided to TerrainModificationVisualPipeline");
        return false;
    }

    device_ = device;
    grid_ = &grid;
    water_data_ = &water_data;
    chunks_ = &chunks;
    water_meshes_ = &water_meshes;
    map_seed_ = map_seed;

    // Initialize mesh generator
    mesh_generator_.initialize(grid.width, grid.height);

    // Initialize dirty tracker
    dirty_tracker_.initialize(grid.width, grid.height);

    // Calculate chunk dimensions
    chunks_x_ = (grid.width + TILES_PER_CHUNK - 1) / TILES_PER_CHUNK;
    chunks_y_ = (grid.height + TILES_PER_CHUNK - 1) / TILES_PER_CHUNK;

    // Initialize vegetation cache (one entry per chunk)
    std::uint32_t total_chunks = static_cast<std::uint32_t>(chunks_x_) * chunks_y_;
    vegetation_cache_.resize(total_chunks);

    // Initialize chunk coordinates in vegetation cache
    for (std::uint16_t cy = 0; cy < chunks_y_; ++cy) {
        for (std::uint16_t cx = 0; cx < chunks_x_; ++cx) {
            std::uint32_t idx = getChunkIndex(cx, cy);
            vegetation_cache_[idx].chunk_x = static_cast<std::int32_t>(cx);
            vegetation_cache_[idx].chunk_y = static_cast<std::int32_t>(cy);
        }
    }

    initialized_ = true;
    LOG_INFO("TerrainModificationVisualPipeline initialized: {}x{} map, {}x{} chunks",
             grid.width, grid.height, chunks_x_, chunks_y_);

    return true;
}

// ============================================================================
// Event Handling
// ============================================================================

void TerrainModificationVisualPipeline::onTerrainModified(const TerrainModifiedEvent& event) {
    if (!initialized_) {
        LOG_WARN("TerrainModificationVisualPipeline: onTerrainModified called before initialize");
        return;
    }

    if (event.affected_area.isEmpty()) {
        return;  // No tiles affected
    }

    // Mark affected chunks as dirty in the tracker
    std::uint32_t dirty_count = dirty_tracker_.processEvent(event);

    // Calculate affected chunk range
    std::uint16_t min_cx, min_cy, max_cx, max_cy;
    getAffectedChunks(event.affected_area, min_cx, min_cy, max_cx, max_cy);

    // Queue terrain chunks for rebuild (via dirty tracker)
    mesh_generator_.queueDirtyChunks(dirty_tracker_);

    // Queue vegetation chunks for regeneration
    for (std::uint16_t cy = min_cy; cy <= max_cy; ++cy) {
        for (std::uint16_t cx = min_cx; cx <= max_cx; ++cx) {
            auto coord = std::make_pair(cx, cy);

            // Check if already queued
            bool already_queued = false;
            for (const auto& queued : pending_vegetation_chunks_) {
                if (queued == coord) {
                    already_queued = true;
                    break;
                }
            }

            if (!already_queued) {
                pending_vegetation_chunks_.push_back(coord);
            }
        }
    }

    // Queue water bodies for regeneration if modification can affect water
    if (canAffectWater(event.modification_type)) {
        std::unordered_set<WaterBodyID> affected_bodies = findAffectedWaterBodies(event.affected_area);

        for (WaterBodyID body_id : affected_bodies) {
            // Check if already queued
            bool already_queued = false;
            for (const auto& queued : pending_water_bodies_) {
                if (queued == body_id) {
                    already_queued = true;
                    break;
                }
            }

            if (!already_queued) {
                pending_water_bodies_.push_back(body_id);
            }
        }
    }

    LOG_DEBUG("TerrainModified: type={}, area=({},{}){}x{}, {} chunks dirty, {} veg pending, {} water pending",
              static_cast<int>(event.modification_type),
              event.affected_area.x, event.affected_area.y,
              event.affected_area.width, event.affected_area.height,
              dirty_count,
              pending_vegetation_chunks_.size(),
              pending_water_bodies_.size());
}

// ============================================================================
// Per-Frame Update
// ============================================================================

VisualUpdateStats TerrainModificationVisualPipeline::update(SDL_GPUDevice* device, float delta_time) {
    VisualUpdateStats stats;

    if (!initialized_) {
        return stats;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Update terrain chunk meshes (rate limited to 1 per frame)
    updateTerrainChunks(device, stats);

    // Update vegetation instances (rate limited to 2 per frame)
    updateVegetation(stats);

    // Update water body meshes (rate limited to 1 per frame)
    updateWaterBodies(device, stats);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    stats.update_time_ms = static_cast<float>(duration.count()) / 1000.0f;

    return stats;
}

void TerrainModificationVisualPipeline::updateTerrainChunks(SDL_GPUDevice* device, VisualUpdateStats& stats) {
    // Use the mesh generator's update method which handles rate limiting
    ChunkRebuildStats rebuild_stats = mesh_generator_.updateDirtyChunks(
        device,
        *grid_,
        *chunks_,
        dirty_tracker_
    );

    stats.terrain_chunks_rebuilt = rebuild_stats.chunks_rebuilt;
    stats.terrain_chunks_pending = rebuild_stats.chunks_pending;
}

void TerrainModificationVisualPipeline::updateVegetation(VisualUpdateStats& stats) {
    std::uint32_t processed = 0;

    while (!pending_vegetation_chunks_.empty() && processed < MAX_VEGETATION_CHUNKS_PER_FRAME) {
        auto coord = pending_vegetation_chunks_.front();
        pending_vegetation_chunks_.pop_front();

        std::uint16_t cx = coord.first;
        std::uint16_t cy = coord.second;

        // Validate coordinates
        if (cx >= chunks_x_ || cy >= chunks_y_) {
            continue;
        }

        // Generate vegetation instances for this chunk
        render::VegetationPlacementGenerator generator(map_seed_, *grid_);
        render::ChunkInstances instances = generator.generateForChunk(
            static_cast<std::int32_t>(cx),
            static_cast<std::int32_t>(cy)
        );

        // Store in cache
        std::uint32_t idx = getChunkIndex(cx, cy);
        vegetation_cache_[idx] = std::move(instances);

        processed++;
        stats.vegetation_chunks_updated++;
    }

    stats.vegetation_chunks_pending = static_cast<std::uint32_t>(pending_vegetation_chunks_.size());
}

void TerrainModificationVisualPipeline::updateWaterBodies(SDL_GPUDevice* device, VisualUpdateStats& stats) {
    std::uint32_t processed = 0;

    while (!pending_water_bodies_.empty() && processed < MAX_WATER_BODIES_PER_FRAME) {
        WaterBodyID body_id = pending_water_bodies_.front();
        pending_water_bodies_.pop_front();

        if (body_id == NO_WATER_BODY) {
            continue;
        }

        // Find the water mesh for this body
        WaterMesh* mesh = nullptr;
        for (auto& wm : *water_meshes_) {
            if (wm.body_id == body_id) {
                mesh = &wm;
                break;
            }
        }

        if (mesh != nullptr) {
            // Release old GPU resources
            if (mesh->hasGPUResources()) {
                mesh->releaseGPUResources(device);
            }

            // Regenerate mesh
            bool success = WaterMeshGenerator::regenerateBody(
                *grid_,
                *water_data_,
                body_id,
                *mesh
            );

            if (success) {
                // Mark as needing GPU upload (done elsewhere in render system)
                mesh->markDirty();
                processed++;
                stats.water_bodies_updated++;
            }
        }
    }

    stats.water_bodies_pending = static_cast<std::uint32_t>(pending_water_bodies_.size());
}

// ============================================================================
// Query Methods
// ============================================================================

bool TerrainModificationVisualPipeline::hasPendingUpdates() const {
    if (!initialized_) {
        return false;
    }

    return mesh_generator_.hasPendingRebuilds() ||
           !pending_vegetation_chunks_.empty() ||
           !pending_water_bodies_.empty();
}

std::uint32_t TerrainModificationVisualPipeline::getPendingTerrainChunks() const {
    if (!initialized_) {
        return 0;
    }
    return mesh_generator_.getPendingRebuildCount();
}

std::uint32_t TerrainModificationVisualPipeline::getPendingVegetationChunks() const {
    return static_cast<std::uint32_t>(pending_vegetation_chunks_.size());
}

std::uint32_t TerrainModificationVisualPipeline::getPendingWaterBodies() const {
    return static_cast<std::uint32_t>(pending_water_bodies_.size());
}

// ============================================================================
// Vegetation Instance Access
// ============================================================================

const render::ChunkInstances& TerrainModificationVisualPipeline::getVegetationInstances(
    std::int32_t chunk_x,
    std::int32_t chunk_y
) {
    static render::ChunkInstances empty_instances;

    if (!initialized_) {
        return empty_instances;
    }

    if (chunk_x < 0 || chunk_x >= static_cast<std::int32_t>(chunks_x_) ||
        chunk_y < 0 || chunk_y >= static_cast<std::int32_t>(chunks_y_)) {
        return empty_instances;
    }

    std::uint32_t idx = getChunkIndex(
        static_cast<std::uint16_t>(chunk_x),
        static_cast<std::uint16_t>(chunk_y)
    );

    // If cache is empty for this chunk, generate on demand
    if (vegetation_cache_[idx].instances.empty()) {
        render::VegetationPlacementGenerator generator(map_seed_, *grid_);
        vegetation_cache_[idx] = generator.generateForChunk(chunk_x, chunk_y);
    }

    return vegetation_cache_[idx];
}

// ============================================================================
// Manual Control
// ============================================================================

void TerrainModificationVisualPipeline::forceRebuildAll(SDL_GPUDevice* device) {
    if (!initialized_) {
        return;
    }

    LOG_INFO("Force rebuilding all terrain visual data...");

    // Rebuild all terrain chunks
    mesh_generator_.buildAllChunks(device, *grid_, *chunks_);

    // Regenerate all vegetation
    render::VegetationPlacementGenerator generator(map_seed_, *grid_);
    for (std::uint16_t cy = 0; cy < chunks_y_; ++cy) {
        for (std::uint16_t cx = 0; cx < chunks_x_; ++cx) {
            std::uint32_t idx = getChunkIndex(cx, cy);
            vegetation_cache_[idx] = generator.generateForChunk(
                static_cast<std::int32_t>(cx),
                static_cast<std::int32_t>(cy)
            );
        }
    }

    // Regenerate all water meshes
    WaterMeshGenerationResult result = WaterMeshGenerator::generate(*grid_, *water_data_);

    // Replace water meshes (releasing old GPU resources first)
    for (auto& mesh : *water_meshes_) {
        if (mesh.hasGPUResources()) {
            mesh.releaseGPUResources(device);
        }
    }
    *water_meshes_ = std::move(result.meshes);

    // Clear all pending updates
    clearPendingUpdates();

    LOG_INFO("Force rebuild complete");
}

void TerrainModificationVisualPipeline::clearPendingUpdates() {
    pending_vegetation_chunks_.clear();
    pending_water_bodies_.clear();
    dirty_tracker_.clearAllDirty();

    // Clear the mesh generator's queue by creating a new one
    mesh_generator_ = TerrainChunkMeshGenerator();
    if (initialized_ && grid_ != nullptr) {
        mesh_generator_.initialize(grid_->width, grid_->height);
    }
}

// ============================================================================
// Internal Helper Methods
// ============================================================================

void TerrainModificationVisualPipeline::getAffectedChunks(
    const GridRect& rect,
    std::uint16_t& out_min_cx,
    std::uint16_t& out_min_cy,
    std::uint16_t& out_max_cx,
    std::uint16_t& out_max_cy
) const {
    // Convert tile coordinates to chunk coordinates
    std::int32_t min_tile_x = rect.x;
    std::int32_t min_tile_y = rect.y;
    std::int32_t max_tile_x = rect.right() - 1;
    std::int32_t max_tile_y = rect.bottom() - 1;

    // Clamp to valid range
    if (min_tile_x < 0) min_tile_x = 0;
    if (min_tile_y < 0) min_tile_y = 0;
    if (max_tile_x < 0) max_tile_x = 0;
    if (max_tile_y < 0) max_tile_y = 0;

    // Convert to chunk coordinates
    out_min_cx = static_cast<std::uint16_t>(min_tile_x / TILES_PER_CHUNK);
    out_min_cy = static_cast<std::uint16_t>(min_tile_y / TILES_PER_CHUNK);
    out_max_cx = static_cast<std::uint16_t>(max_tile_x / TILES_PER_CHUNK);
    out_max_cy = static_cast<std::uint16_t>(max_tile_y / TILES_PER_CHUNK);

    // Clamp to valid chunk range
    if (out_max_cx >= chunks_x_) out_max_cx = chunks_x_ - 1;
    if (out_max_cy >= chunks_y_) out_max_cy = chunks_y_ - 1;
}

bool TerrainModificationVisualPipeline::canAffectWater(ModificationType type) const {
    switch (type) {
        case ModificationType::Terraformed:
            // Terrain type changes can create or remove water
            return true;
        case ModificationType::SeaLevelChanged:
            // Sea level changes affect all water
            return true;
        case ModificationType::Generated:
            // Initial generation includes water
            return true;
        case ModificationType::Cleared:
        case ModificationType::Leveled:
            // These don't typically affect water boundaries
            return false;
        default:
            return false;
    }
}

std::unordered_set<WaterBodyID> TerrainModificationVisualPipeline::findAffectedWaterBodies(
    const GridRect& rect
) const {
    std::unordered_set<WaterBodyID> bodies;

    if (water_data_ == nullptr) {
        return bodies;
    }

    // Check each tile in the affected area
    std::int16_t end_x = rect.right();
    std::int16_t end_y = rect.bottom();

    for (std::int16_t y = rect.y; y < end_y; ++y) {
        for (std::int16_t x = rect.x; x < end_x; ++x) {
            WaterBodyID body_id = water_data_->get_water_body_id(x, y);
            if (body_id != NO_WATER_BODY) {
                bodies.insert(body_id);
            }
        }
    }

    // Also check adjacent tiles (water boundaries)
    // Expand the search by 1 tile in each direction
    for (std::int16_t y = rect.y - 1; y <= end_y; ++y) {
        for (std::int16_t x = rect.x - 1; x <= end_x; ++x) {
            WaterBodyID body_id = water_data_->get_water_body_id(x, y);
            if (body_id != NO_WATER_BODY) {
                bodies.insert(body_id);
            }
        }
    }

    return bodies;
}

std::uint32_t TerrainModificationVisualPipeline::getChunkIndex(
    std::uint16_t chunk_x,
    std::uint16_t chunk_y
) const {
    return static_cast<std::uint32_t>(chunk_y) * chunks_x_ + chunk_x;
}

} // namespace terrain
} // namespace sims3000
