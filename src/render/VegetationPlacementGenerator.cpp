/**
 * @file VegetationPlacementGenerator.cpp
 * @brief Implementation of vegetation instance placement generator.
 *
 * @see VegetationInstance.h
 */

#include <sims3000/render/VegetationInstance.h>
#include <cmath>

namespace sims3000 {
namespace render {

// FNV-1a constants for 64-bit hash
static constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
static constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;

// Pi constant
static constexpr float PI = 3.14159265358979323846f;
static constexpr float TWO_PI = 2.0f * PI;

VegetationPlacementGenerator::VegetationPlacementGenerator(
    std::uint64_t map_seed,
    const terrain::TerrainGrid& terrain)
    : m_mapSeed(map_seed)
    , m_terrain(terrain)
{
}

std::uint64_t VegetationPlacementGenerator::computeTileSeed(
    std::int32_t tile_x,
    std::int32_t tile_y) const
{
    // FNV-1a hash combining map seed and tile coordinates
    // This produces a unique, deterministic seed for each tile
    std::uint64_t hash = FNV_OFFSET_BASIS;

    // Mix in map seed (8 bytes)
    const std::uint8_t* seed_bytes = reinterpret_cast<const std::uint8_t*>(&m_mapSeed);
    for (std::size_t i = 0; i < 8; ++i) {
        hash ^= seed_bytes[i];
        hash *= FNV_PRIME;
    }

    // Mix in tile_x (4 bytes)
    const std::uint8_t* x_bytes = reinterpret_cast<const std::uint8_t*>(&tile_x);
    for (std::size_t i = 0; i < 4; ++i) {
        hash ^= x_bytes[i];
        hash *= FNV_PRIME;
    }

    // Mix in tile_y (4 bytes)
    const std::uint8_t* y_bytes = reinterpret_cast<const std::uint8_t*>(&tile_y);
    for (std::size_t i = 0; i < 4; ++i) {
        hash ^= y_bytes[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

bool VegetationPlacementGenerator::hasVegetation(terrain::TerrainType type)
{
    switch (type) {
        case terrain::TerrainType::BiolumeGrove:
        case terrain::TerrainType::PrismaFields:
        case terrain::TerrainType::SporeFlats:
            return true;
        default:
            return false;
    }
}

VegetationModelType VegetationPlacementGenerator::getModelType(terrain::TerrainType type)
{
    switch (type) {
        case terrain::TerrainType::BiolumeGrove:
            return VegetationModelType::BiolumeTree;
        case terrain::TerrainType::PrismaFields:
            return VegetationModelType::CrystalSpire;
        case terrain::TerrainType::SporeFlats:
            return VegetationModelType::SporeEmitter;
        default:
            return VegetationModelType::BiolumeTree;
    }
}

void VegetationPlacementGenerator::getInstanceCountRange(
    terrain::TerrainType type,
    std::uint8_t& min_instances,
    std::uint8_t& max_instances)
{
    switch (type) {
        case terrain::TerrainType::BiolumeGrove:
            min_instances = BIOLUME_GROVE_MIN_INSTANCES;
            max_instances = BIOLUME_GROVE_MAX_INSTANCES;
            break;
        case terrain::TerrainType::PrismaFields:
            min_instances = PRISMA_FIELDS_MIN_INSTANCES;
            max_instances = PRISMA_FIELDS_MAX_INSTANCES;
            break;
        case terrain::TerrainType::SporeFlats:
            min_instances = SPORE_FLATS_MIN_INSTANCES;
            max_instances = SPORE_FLATS_MAX_INSTANCES;
            break;
        default:
            min_instances = 0;
            max_instances = 0;
            break;
    }
}

void VegetationPlacementGenerator::generateForTile(
    std::int32_t tile_x,
    std::int32_t tile_y,
    std::vector<VegetationInstance>& instances) const
{
    // Bounds check
    if (!m_terrain.in_bounds(tile_x, tile_y)) {
        return;
    }

    const terrain::TerrainComponent& tile = m_terrain.at(tile_x, tile_y);
    terrain::TerrainType type = tile.getTerrainType();

    // Skip non-vegetation terrain
    if (!hasVegetation(type)) {
        return;
    }

    // Skip cleared tiles
    if (tile.isCleared()) {
        return;
    }

    // Get instance count range for this terrain type
    std::uint8_t min_count, max_count;
    getInstanceCountRange(type, min_count, max_count);

    // Seed PRNG with deterministic tile seed
    std::uint64_t tile_seed = computeTileSeed(tile_x, tile_y);
    std::mt19937_64 rng(tile_seed);

    // Distributions for random values
    // Note: MSVC doesn't support uniform_int_distribution<uint8_t>, so use int
    std::uniform_int_distribution<int> count_dist(min_count, max_count);
    std::uniform_real_distribution<float> jitter_dist(-JITTER_RANGE, JITTER_RANGE);
    std::uniform_real_distribution<float> rotation_dist(0.0f, TWO_PI);
    std::uniform_real_distribution<float> scale_dist(MIN_SCALE, MAX_SCALE);

    // Determine how many instances to generate for this tile
    int instance_count = count_dist(rng);

    // Get model type for this terrain
    VegetationModelType model = getModelType(type);

    // Tile center in world coordinates (tiles are 1 unit, origin at top-left)
    float tile_center_x = static_cast<float>(tile_x) + 0.5f;
    float tile_center_y = static_cast<float>(tile_y) + 0.5f;

    // Get tile elevation for Y position
    float elevation = static_cast<float>(tile.getElevation());

    // Generate instances
    for (int i = 0; i < instance_count; ++i) {
        VegetationInstance instance;

        // Position: tile center + random jitter
        float jitter_x = jitter_dist(rng);
        float jitter_z = jitter_dist(rng);  // Z is the grid Y in 3D space

        instance.position = glm::vec3(
            tile_center_x + jitter_x,
            elevation,
            tile_center_y + jitter_z
        );

        // Rotation: random Y-axis rotation
        instance.rotation_y = rotation_dist(rng);

        // Scale: random variation within range
        instance.scale = scale_dist(rng);

        // Model type
        instance.model_type = model;

        instances.push_back(instance);
    }
}

ChunkInstances VegetationPlacementGenerator::generateForChunk(
    std::int32_t chunk_x,
    std::int32_t chunk_y) const
{
    ChunkInstances result;
    result.chunk_x = chunk_x;
    result.chunk_y = chunk_y;

    // Reserve estimated capacity (average ~3-4 instances per vegetation tile,
    // assuming ~50% vegetation coverage = ~512 instances per chunk)
    result.instances.reserve(512);

    // Calculate tile range for this chunk
    std::int32_t start_tile_x = chunk_x * CHUNK_SIZE;
    std::int32_t start_tile_y = chunk_y * CHUNK_SIZE;
    std::int32_t end_tile_x = start_tile_x + CHUNK_SIZE;
    std::int32_t end_tile_y = start_tile_y + CHUNK_SIZE;

    // Clamp to terrain bounds
    if (start_tile_x < 0) start_tile_x = 0;
    if (start_tile_y < 0) start_tile_y = 0;
    if (end_tile_x > static_cast<std::int32_t>(m_terrain.width)) {
        end_tile_x = static_cast<std::int32_t>(m_terrain.width);
    }
    if (end_tile_y > static_cast<std::int32_t>(m_terrain.height)) {
        end_tile_y = static_cast<std::int32_t>(m_terrain.height);
    }

    // Early out if chunk is completely out of bounds
    if (start_tile_x >= end_tile_x || start_tile_y >= end_tile_y) {
        return result;
    }

    // Generate instances for each tile in the chunk
    for (std::int32_t y = start_tile_y; y < end_tile_y; ++y) {
        for (std::int32_t x = start_tile_x; x < end_tile_x; ++x) {
            generateForTile(x, y, result.instances);
        }
    }

    return result;
}

} // namespace render
} // namespace sims3000
