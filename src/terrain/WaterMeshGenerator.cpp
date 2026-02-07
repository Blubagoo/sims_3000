/**
 * @file WaterMeshGenerator.cpp
 * @brief Implementation of water surface mesh generation.
 */

#include <sims3000/terrain/WaterMesh.h>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

namespace sims3000 {
namespace terrain {

// ============================================================================
// Helper Functions
// ============================================================================

bool WaterMeshGenerator::isWater(TerrainType type) {
    return type == TerrainType::DeepVoid ||
           type == TerrainType::FlowChannel ||
           type == TerrainType::StillBasin;
}

WaterBodyType WaterMeshGenerator::getWaterBodyType(TerrainType type) {
    switch (type) {
        case TerrainType::DeepVoid:
            return WaterBodyType::Ocean;
        case TerrainType::FlowChannel:
            return WaterBodyType::River;
        case TerrainType::StillBasin:
            return WaterBodyType::Lake;
        default:
            return WaterBodyType::Ocean;  // Fallback
    }
}

float WaterMeshGenerator::calculateShoreFactor(
    const TerrainGrid& grid,
    std::int32_t vx,
    std::int32_t vy,
    const WaterData& waterData,
    WaterBodyID body_id
) {
    // A vertex at corner (vx, vy) is adjacent to up to 4 tiles:
    // - (vx-1, vy-1) top-left
    // - (vx, vy-1)   top-right
    // - (vx-1, vy)   bottom-left
    // - (vx, vy)     bottom-right
    //
    // Shore factor is 1.0 if any adjacent tile is:
    // 1. Land (not water)
    // 2. A different water body
    // 3. Out of bounds (edge of map)

    // Offsets for the 4 adjacent tiles
    static const std::int32_t offsets[4][2] = {
        {-1, -1},  // top-left
        { 0, -1},  // top-right
        {-1,  0},  // bottom-left
        { 0,  0}   // bottom-right
    };

    int adjacent_same_body = 0;
    int total_valid_tiles = 0;

    for (int i = 0; i < 4; ++i) {
        std::int32_t tx = vx + offsets[i][0];
        std::int32_t ty = vy + offsets[i][1];

        if (!grid.in_bounds(tx, ty)) {
            // Out of bounds counts as land (edge of map)
            continue;
        }

        total_valid_tiles++;

        const TerrainComponent& tile = grid.at(tx, ty);
        if (!isWater(tile.getTerrainType())) {
            // Land tile - this is a shore vertex
            return 1.0f;
        }

        // Check if same water body
        WaterBodyID tile_body = waterData.get_water_body_id(tx, ty);
        if (tile_body == body_id) {
            adjacent_same_body++;
        } else {
            // Different water body - treat as edge
            return 1.0f;
        }
    }

    // If we have fewer than 4 valid adjacent tiles (at map edge), it's a shore
    if (total_valid_tiles < 4) {
        return 1.0f;
    }

    // Interior vertex - all 4 adjacent tiles are same water body
    return 0.0f;
}

std::vector<std::pair<std::uint16_t, std::uint16_t>> WaterMeshGenerator::collectBodyTiles(
    const WaterData& waterData,
    WaterBodyID body_id
) {
    std::vector<std::pair<std::uint16_t, std::uint16_t>> tiles;

    std::uint16_t width = waterData.water_body_ids.width;
    std::uint16_t height = waterData.water_body_ids.height;

    for (std::uint16_t y = 0; y < height; ++y) {
        for (std::uint16_t x = 0; x < width; ++x) {
            if (waterData.get_water_body_id(x, y) == body_id) {
                tiles.emplace_back(x, y);
            }
        }
    }

    return tiles;
}

WaterMesh WaterMeshGenerator::generateBodyMesh(
    const TerrainGrid& grid,
    const WaterData& waterData,
    WaterBodyID body_id,
    const std::vector<std::pair<std::uint16_t, std::uint16_t>>& tiles
) {
    // Determine water body type from first tile
    WaterBodyType body_type = WaterBodyType::Ocean;
    if (!tiles.empty()) {
        const auto& first_tile = tiles[0];
        TerrainType terrain_type = grid.at(first_tile.first, first_tile.second).getTerrainType();
        body_type = getWaterBodyType(terrain_type);
    }

    WaterMesh mesh(body_id, body_type);

    if (tiles.empty()) {
        return mesh;
    }

    // Calculate water surface Y position (sea level * ELEVATION_HEIGHT)
    float water_y = static_cast<float>(grid.sea_level) * ELEVATION_HEIGHT;

    // Build a set of tile coordinates for quick lookup
    std::unordered_set<std::uint32_t> tile_set;
    for (const auto& tile : tiles) {
        std::uint32_t key = (static_cast<std::uint32_t>(tile.second) << 16) | tile.first;
        tile_set.insert(key);
    }

    // Find bounding box of tiles
    std::uint16_t min_x = tiles[0].first;
    std::uint16_t max_x = tiles[0].first;
    std::uint16_t min_y = tiles[0].second;
    std::uint16_t max_y = tiles[0].second;

    for (const auto& tile : tiles) {
        min_x = std::min(min_x, tile.first);
        max_x = std::max(max_x, tile.first);
        min_y = std::min(min_y, tile.second);
        max_y = std::max(max_y, tile.second);
    }

    // Create a map from vertex coordinate to vertex index
    // Vertices are at tile corners: (vx, vy) where vx in [min_x, max_x+1], vy in [min_y, max_y+1]
    std::uint16_t vertex_width = max_x - min_x + 2;  // +2 because corners
    std::uint16_t vertex_height = max_y - min_y + 2;

    // Generate vertices for all corners in the bounding box
    // We'll only use vertices that are adjacent to at least one tile in this body
    std::unordered_map<std::uint32_t, std::uint32_t> vertex_map;  // (vy << 16 | vx) -> vertex index

    auto getVertexKey = [](std::uint16_t vx, std::uint16_t vy) -> std::uint32_t {
        return (static_cast<std::uint32_t>(vy) << 16) | vx;
    };

    auto getTileKey = [](std::uint16_t tx, std::uint16_t ty) -> std::uint32_t {
        return (static_cast<std::uint32_t>(ty) << 16) | tx;
    };

    // For each tile in the body, generate its 4 corner vertices and quad
    for (const auto& tile : tiles) {
        std::uint16_t tx = tile.first;
        std::uint16_t ty = tile.second;

        // 4 corners of this tile
        // Top-left: (tx, ty)
        // Top-right: (tx+1, ty)
        // Bottom-left: (tx, ty+1)
        // Bottom-right: (tx+1, ty+1)
        std::uint16_t corners[4][2] = {
            {tx, ty},
            {static_cast<std::uint16_t>(tx + 1), ty},
            {tx, static_cast<std::uint16_t>(ty + 1)},
            {static_cast<std::uint16_t>(tx + 1), static_cast<std::uint16_t>(ty + 1)}
        };

        std::uint32_t vertex_indices[4];

        for (int i = 0; i < 4; ++i) {
            std::uint16_t vx = corners[i][0];
            std::uint16_t vy = corners[i][1];
            std::uint32_t key = getVertexKey(vx, vy);

            auto it = vertex_map.find(key);
            if (it != vertex_map.end()) {
                vertex_indices[i] = it->second;
            } else {
                // Create new vertex
                WaterVertex vert;
                vert.position_x = static_cast<float>(vx);
                vert.position_y = water_y;
                vert.position_z = static_cast<float>(vy);

                vert.shore_factor = calculateShoreFactor(grid, vx, vy, waterData, body_id);
                vert.water_body_id = body_id;

                // UV coordinates based on world position (for tiling water texture)
                vert.uv_u = static_cast<float>(vx);
                vert.uv_v = static_cast<float>(vy);

                std::uint32_t idx = static_cast<std::uint32_t>(mesh.vertices.size());
                mesh.vertices.push_back(vert);
                vertex_map[key] = idx;
                vertex_indices[i] = idx;
            }
        }

        // Generate 2 triangles for the quad
        // Triangle 1: top-left, bottom-left, bottom-right
        mesh.indices.push_back(vertex_indices[0]);  // TL
        mesh.indices.push_back(vertex_indices[2]);  // BL
        mesh.indices.push_back(vertex_indices[3]);  // BR

        // Triangle 2: top-left, bottom-right, top-right
        mesh.indices.push_back(vertex_indices[0]);  // TL
        mesh.indices.push_back(vertex_indices[3]);  // BR
        mesh.indices.push_back(vertex_indices[1]);  // TR
    }

    // Update counts
    mesh.vertex_count = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.index_count = static_cast<std::uint32_t>(mesh.indices.size());

    // Compute AABB
    if (!mesh.vertices.empty()) {
        mesh.aabb = AABB::empty();
        for (const auto& vert : mesh.vertices) {
            mesh.aabb.expand(glm::vec3(vert.position_x, vert.position_y, vert.position_z));
        }
    }

    return mesh;
}

// ============================================================================
// Public API
// ============================================================================

WaterMeshGenerationResult WaterMeshGenerator::generate(
    const TerrainGrid& grid,
    const WaterData& waterData
) {
    auto start_time = std::chrono::high_resolution_clock::now();

    WaterMeshGenerationResult result;
    result.total_vertex_count = 0;
    result.total_index_count = 0;
    result.ocean_mesh_count = 0;
    result.river_mesh_count = 0;
    result.lake_mesh_count = 0;

    if (grid.empty() || waterData.empty()) {
        result.generation_time_ms = 0.0f;
        return result;
    }

    // Find all unique water body IDs
    std::unordered_set<WaterBodyID> body_ids;
    std::uint16_t width = waterData.water_body_ids.width;
    std::uint16_t height = waterData.water_body_ids.height;

    for (std::uint16_t y = 0; y < height; ++y) {
        for (std::uint16_t x = 0; x < width; ++x) {
            WaterBodyID id = waterData.get_water_body_id(x, y);
            if (id != NO_WATER_BODY) {
                body_ids.insert(id);
            }
        }
    }

    // Generate mesh for each water body
    for (WaterBodyID body_id : body_ids) {
        std::vector<std::pair<std::uint16_t, std::uint16_t>> tiles = collectBodyTiles(waterData, body_id);

        if (tiles.empty()) {
            continue;
        }

        WaterMesh mesh = generateBodyMesh(grid, waterData, body_id, tiles);

        if (!mesh.isEmpty()) {
            result.total_vertex_count += mesh.vertex_count;
            result.total_index_count += mesh.index_count;

            switch (mesh.body_type) {
                case WaterBodyType::Ocean:
                    result.ocean_mesh_count++;
                    break;
                case WaterBodyType::River:
                    result.river_mesh_count++;
                    break;
                case WaterBodyType::Lake:
                    result.lake_mesh_count++;
                    break;
            }

            result.meshes.push_back(std::move(mesh));
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    result.generation_time_ms = static_cast<float>(duration.count()) / 1000.0f;

    return result;
}

bool WaterMeshGenerator::regenerateBody(
    const TerrainGrid& grid,
    const WaterData& waterData,
    WaterBodyID body_id,
    WaterMesh& out_mesh
) {
    if (body_id == NO_WATER_BODY) {
        return false;
    }

    std::vector<std::pair<std::uint16_t, std::uint16_t>> tiles = collectBodyTiles(waterData, body_id);

    if (tiles.empty()) {
        return false;
    }

    out_mesh = generateBodyMesh(grid, waterData, body_id, tiles);
    return !out_mesh.isEmpty();
}

} // namespace terrain
} // namespace sims3000
