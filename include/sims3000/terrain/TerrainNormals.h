/**
 * @file TerrainNormals.h
 * @brief Terrain normal calculation using central differences
 *
 * Computes per-vertex normals from heightmap elevation data using the
 * central differences method. Normals drive toon shader lighting bands:
 * - Flat terrain: Normals point straight up (0, 1, 0), full light
 * - Slopes: Normals tilt toward gradient direction, mid light
 * - Steep cliffs: Normals point mostly horizontal, deep shadow
 *
 * Central differences formula:
 *   nx = height(x-1, z) - height(x+1, z)
 *   nz = height(x, z-1) - height(x, z+1)
 *   ny = 2.0 * ELEVATION_HEIGHT
 *   normalize(nx, ny, nz)
 *
 * Boundary handling:
 * - At chunk edges: reads from TerrainGrid directly (not chunk-local data)
 * - At map edges: clamps or mirrors neighbor lookups to avoid out-of-bounds
 *
 * @see TerrainVertex for vertex format with normal fields
 * @see TerrainChunk for ELEVATION_HEIGHT constant
 */

#ifndef SIMS3000_TERRAIN_TERRAINNORMALS_H
#define SIMS3000_TERRAIN_TERRAINNORMALS_H

#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainChunk.h>
#include <cstdint>
#include <cmath>

namespace sims3000 {
namespace terrain {

/**
 * @brief Result of normal calculation as three float components.
 *
 * Stored as separate floats rather than a vector to match TerrainVertex layout.
 */
struct NormalResult {
    float nx;  ///< X component of normalized normal
    float ny;  ///< Y component of normalized normal
    float nz;  ///< Z component of normalized normal
};

/**
 * @brief Compute the terrain normal at a specific vertex position.
 *
 * Uses central differences to compute the gradient of the heightfield,
 * then normalizes the result. The ny component is scaled by 2.0 * ELEVATION_HEIGHT
 * to provide proper scaling between elevation units and world units.
 *
 * Boundary handling:
 * - At map edges, neighbor lookups are clamped to valid coordinates
 * - This produces correct normals at map boundaries (slopes toward interior)
 *
 * @param grid The terrain grid to sample elevation from.
 * @param vertex_x Vertex X coordinate (tile coordinate, can be at tile corner).
 * @param vertex_z Vertex Z coordinate (tile coordinate, can be at tile corner).
 * @return Normalized normal vector components.
 *
 * @note For vertices at tile corners, the coordinates match the tile origin.
 *       For example, vertex at (32, 32) is at the corner of tiles (31,31),
 *       (31,32), (32,31), and (32,32).
 */
NormalResult computeTerrainNormal(const TerrainGrid& grid,
                                   std::int32_t vertex_x,
                                   std::int32_t vertex_z);

/**
 * @brief Compute the terrain normal with explicit elevation sampling function.
 *
 * Template version that allows custom elevation sampling, useful for:
 * - Testing with mock grids
 * - Sampling with interpolation
 * - Using cached elevation data
 *
 * @tparam ElevationSampler Callable type: float(int32_t x, int32_t z)
 * @param sampler Elevation sampling function.
 * @param vertex_x Vertex X coordinate.
 * @param vertex_z Vertex Z coordinate.
 * @param map_width Map width in tiles (for boundary clamping).
 * @param map_height Map height in tiles (for boundary clamping).
 * @return Normalized normal vector components.
 */
template<typename ElevationSampler>
NormalResult computeTerrainNormalWithSampler(
    ElevationSampler&& sampler,
    std::int32_t vertex_x,
    std::int32_t vertex_z,
    std::int32_t map_width,
    std::int32_t map_height);

/**
 * @brief Sample elevation at a coordinate with boundary clamping.
 *
 * Returns the elevation at the given coordinate, clamping coordinates
 * to valid range [0, dimension-1] for boundary handling.
 *
 * @param grid The terrain grid.
 * @param x X coordinate (may be out of bounds).
 * @param z Z coordinate (may be out of bounds).
 * @return Elevation in world units (elevation_level * ELEVATION_HEIGHT).
 */
float sampleElevationClamped(const TerrainGrid& grid,
                              std::int32_t x,
                              std::int32_t z);

/**
 * @brief Check if a computed normal represents flat terrain.
 *
 * Flat terrain has normals pointing straight up (0, 1, 0).
 * Uses an epsilon for floating-point comparison.
 *
 * @param normal The computed normal.
 * @param epsilon Tolerance for comparison (default: 0.0001f).
 * @return true if normal is approximately (0, 1, 0).
 */
bool isNormalFlat(const NormalResult& normal, float epsilon = 0.0001f);

/**
 * @brief Calculate the slope angle from a normal vector.
 *
 * Returns the angle between the normal and the up vector (0, 1, 0)
 * in radians. Used for toon shader band selection:
 * - 0 radians: flat terrain (full light)
 * - ~0.25-0.5 radians: gentle slope (mid light)
 * - >0.5 radians: steep slope (shadow)
 *
 * @param normal The computed normal.
 * @return Slope angle in radians [0, PI/2].
 */
float calculateSlopeAngle(const NormalResult& normal);


// ============================================================================
// Template Implementation
// ============================================================================

template<typename ElevationSampler>
NormalResult computeTerrainNormalWithSampler(
    ElevationSampler&& sampler,
    std::int32_t vertex_x,
    std::int32_t vertex_z,
    std::int32_t map_width,
    std::int32_t map_height)
{
    // Clamp neighbor coordinates to valid range
    auto clamp = [](std::int32_t val, std::int32_t min_val, std::int32_t max_val) {
        if (val < min_val) return min_val;
        if (val > max_val) return max_val;
        return val;
    };

    // Sample neighbor elevations with clamping
    // Note: Vertices are at tile corners, so valid range is [0, map_size]
    // For elevation sampling, we clamp to tile range [0, map_size-1]
    std::int32_t x_minus = clamp(vertex_x - 1, 0, map_width - 1);
    std::int32_t x_plus = clamp(vertex_x + 1, 0, map_width - 1);
    std::int32_t z_minus = clamp(vertex_z - 1, 0, map_height - 1);
    std::int32_t z_plus = clamp(vertex_z + 1, 0, map_height - 1);

    // Clamp center coordinates for sampling
    std::int32_t x_center = clamp(vertex_x, 0, map_width - 1);
    std::int32_t z_center = clamp(vertex_z, 0, map_height - 1);

    // Sample elevations (in world units)
    float h_x_minus = sampler(x_minus, z_center);
    float h_x_plus = sampler(x_plus, z_center);
    float h_z_minus = sampler(x_center, z_minus);
    float h_z_plus = sampler(x_center, z_plus);

    // Central differences formula:
    // nx = h(x-1,z) - h(x+1,z)
    // nz = h(x,z-1) - h(x,z+1)
    // ny = 2.0 * ELEVATION_HEIGHT (scaling factor)
    float nx = h_x_minus - h_x_plus;
    float nz = h_z_minus - h_z_plus;
    float ny = 2.0f * ELEVATION_HEIGHT;

    // Normalize the result
    float length = std::sqrt(nx * nx + ny * ny + nz * nz);

    NormalResult result;
    if (length > 0.0f) {
        float inv_length = 1.0f / length;
        result.nx = nx * inv_length;
        result.ny = ny * inv_length;
        result.nz = nz * inv_length;
    } else {
        // Degenerate case: return up vector
        result.nx = 0.0f;
        result.ny = 1.0f;
        result.nz = 0.0f;
    }

    return result;
}

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINNORMALS_H
