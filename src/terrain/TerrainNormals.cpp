/**
 * @file TerrainNormals.cpp
 * @brief Implementation of terrain normal calculation using central differences
 *
 * Implements the central differences algorithm for computing terrain normals
 * from heightmap data. Uses the TerrainGrid for elevation sampling and handles
 * boundary conditions at map edges through coordinate clamping.
 */

#include <sims3000/terrain/TerrainNormals.h>
#include <cmath>

namespace sims3000 {
namespace terrain {

float sampleElevationClamped(const TerrainGrid& grid,
                              std::int32_t x,
                              std::int32_t z)
{
    // Clamp coordinates to valid tile range
    if (x < 0) x = 0;
    if (z < 0) z = 0;
    if (x >= static_cast<std::int32_t>(grid.width)) {
        x = static_cast<std::int32_t>(grid.width) - 1;
    }
    if (z >= static_cast<std::int32_t>(grid.height)) {
        z = static_cast<std::int32_t>(grid.height) - 1;
    }

    // Get elevation from grid and convert to world units
    std::uint8_t elevation = grid.at(x, z).getElevation();
    return static_cast<float>(elevation) * ELEVATION_HEIGHT;
}

NormalResult computeTerrainNormal(const TerrainGrid& grid,
                                   std::int32_t vertex_x,
                                   std::int32_t vertex_z)
{
    // Create a sampler lambda that uses the grid
    auto sampler = [&grid](std::int32_t x, std::int32_t z) -> float {
        return sampleElevationClamped(grid, x, z);
    };

    return computeTerrainNormalWithSampler(
        sampler,
        vertex_x,
        vertex_z,
        static_cast<std::int32_t>(grid.width),
        static_cast<std::int32_t>(grid.height)
    );
}

bool isNormalFlat(const NormalResult& normal, float epsilon)
{
    // Flat terrain has normal pointing straight up: (0, 1, 0)
    // Check if nx and nz are near zero, and ny is near 1
    return std::abs(normal.nx) < epsilon &&
           std::abs(normal.ny - 1.0f) < epsilon &&
           std::abs(normal.nz) < epsilon;
}

float calculateSlopeAngle(const NormalResult& normal)
{
    // The angle between the normal and the up vector (0, 1, 0)
    // is acos(dot(normal, up)) = acos(ny)
    // Clamp ny to [-1, 1] to handle floating-point errors
    float ny_clamped = normal.ny;
    if (ny_clamped > 1.0f) ny_clamped = 1.0f;
    if (ny_clamped < -1.0f) ny_clamped = -1.0f;

    return std::acos(ny_clamped);
}

} // namespace terrain
} // namespace sims3000
