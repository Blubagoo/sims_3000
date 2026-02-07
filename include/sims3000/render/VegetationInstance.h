/**
 * @file VegetationInstance.h
 * @brief Vegetation instance placement generator for terrain rendering.
 *
 * Generates deterministic per-tile vegetation/decoration instance placement
 * from tile coordinates. Uses seeded PRNG (seed = hash(tile_x, tile_y, map_seed))
 * for position jitter, Y-axis rotation, and scale variation.
 *
 * This is part of RenderingSystem's responsibility per decision record
 * /plans/decisions/epic-3-vegetation-ownership.md:
 * - TerrainSystem owns tile-level vegetation designation (which tiles have vegetation)
 * - RenderingSystem generates per-tile visual instances deterministically
 *
 * Instance generation is deterministic: the same tile coordinates and map seed
 * will always produce the same instances. This eliminates the need to sync
 * per-tree positions over the network.
 *
 * @see /plans/decisions/epic-3-vegetation-ownership.md
 * @see TerrainTypeInfo.h for terrain type properties
 */

#ifndef SIMS3000_RENDER_VEGETATION_INSTANCE_H
#define SIMS3000_RENDER_VEGETATION_INSTANCE_H

#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainTypeInfo.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
#include <random>

namespace sims3000 {
namespace render {

/**
 * @enum VegetationModelType
 * @brief Model types for vegetation instances.
 *
 * Each vegetation terrain type maps to a specific model type.
 * Used to batch instances by model for efficient GPU instancing.
 */
enum class VegetationModelType : std::uint8_t {
    BiolumeTree = 0,     ///< Alien tree/fungus for BiolumeGrove terrain
    CrystalSpire = 1,    ///< Luminous crystal for PrismaFields terrain
    SporeEmitter = 2,    ///< Bioluminescent spore flora for SporeFlats terrain

    Count = 3            ///< Number of model types
};

/**
 * @struct VegetationInstance
 * @brief Per-instance data for vegetation rendering.
 *
 * Contains all data needed for GPU instancing of a single vegetation object.
 * Position is in world space (tile origin + jitter).
 */
struct VegetationInstance {
    glm::vec3 position;           ///< World position (tile center + jitter)
    float rotation_y;             ///< Y-axis rotation in radians (0 to 2*PI)
    float scale;                  ///< Uniform scale factor (0.8 to 1.2)
    VegetationModelType model_type;  ///< Which model to use
    std::uint8_t _padding[3];     ///< Padding for alignment
};

// Verify VegetationInstance size for predictable memory layout
static_assert(sizeof(VegetationInstance) == 24,
    "VegetationInstance should be 24 bytes for efficient storage");

/**
 * @struct ChunkInstances
 * @brief Collection of vegetation instances for a chunk.
 *
 * Stores all vegetation instances generated for a single chunk,
 * along with metadata for the chunk bounds.
 */
struct ChunkInstances {
    std::vector<VegetationInstance> instances;  ///< All instances in the chunk
    std::int32_t chunk_x = 0;                   ///< Chunk X coordinate
    std::int32_t chunk_y = 0;                   ///< Chunk Y coordinate
};

/**
 * @class VegetationPlacementGenerator
 * @brief Generates deterministic vegetation instances from terrain data.
 *
 * Uses a hash-based seeded PRNG to generate consistent placement:
 * - Same tile coordinates + map seed = same instances every time
 * - No network sync needed for per-tree positions
 *
 * Instance counts per terrain type:
 * - BiolumeGrove: 2-4 instances per tile
 * - PrismaFields: 1-3 instances per tile
 * - SporeFlats: 4-6 instances per tile (small instances)
 *
 * Cleared tiles (is_cleared flag set) produce no instances.
 *
 * Usage:
 * @code
 *   VegetationPlacementGenerator generator(map_seed, terrain_grid);
 *
 *   // Generate for a single chunk
 *   ChunkInstances chunk = generator.generateForChunk(chunk_x, chunk_y);
 *
 *   // Use instances for GPU instancing
 *   for (const auto& instance : chunk.instances) {
 *       addToInstanceBuffer(instance);
 *   }
 * @endcode
 */
class VegetationPlacementGenerator {
public:
    /// Chunk size in tiles (32x32)
    static constexpr std::int32_t CHUNK_SIZE = 32;

    /// Scale variation range: base scale * [MIN_SCALE, MAX_SCALE]
    static constexpr float MIN_SCALE = 0.8f;
    static constexpr float MAX_SCALE = 1.2f;

    /// Position jitter range: [-JITTER_RANGE, JITTER_RANGE] within tile
    /// Value of 0.4 keeps instances well within tile boundaries
    static constexpr float JITTER_RANGE = 0.4f;

    /// Instance counts per terrain type
    static constexpr std::uint8_t BIOLUME_GROVE_MIN_INSTANCES = 2;
    static constexpr std::uint8_t BIOLUME_GROVE_MAX_INSTANCES = 4;
    static constexpr std::uint8_t PRISMA_FIELDS_MIN_INSTANCES = 1;
    static constexpr std::uint8_t PRISMA_FIELDS_MAX_INSTANCES = 3;
    static constexpr std::uint8_t SPORE_FLATS_MIN_INSTANCES = 4;
    static constexpr std::uint8_t SPORE_FLATS_MAX_INSTANCES = 6;

    /**
     * Create a vegetation placement generator.
     *
     * @param map_seed Global map seed for deterministic generation
     * @param terrain Reference to the terrain grid (must outlive generator)
     */
    VegetationPlacementGenerator(std::uint64_t map_seed, const terrain::TerrainGrid& terrain);

    /**
     * Generate vegetation instances for a single chunk.
     *
     * Iterates through all tiles in the chunk and generates instances
     * for vegetation terrain types that are not cleared.
     *
     * @param chunk_x Chunk X coordinate (in chunk units, not tiles)
     * @param chunk_y Chunk Y coordinate (in chunk units, not tiles)
     * @return ChunkInstances containing all generated instances
     *
     * @note Performance target: < 0.5ms for 32x32 chunk
     */
    ChunkInstances generateForChunk(std::int32_t chunk_x, std::int32_t chunk_y) const;

    /**
     * Generate vegetation instances for a single tile.
     *
     * @param tile_x Tile X coordinate
     * @param tile_y Tile Y coordinate
     * @param[out] instances Output vector to append instances to
     *
     * @note Does not clear the output vector; appends to existing content
     */
    void generateForTile(
        std::int32_t tile_x,
        std::int32_t tile_y,
        std::vector<VegetationInstance>& instances) const;

    /**
     * Check if a terrain type produces vegetation instances.
     *
     * @param type Terrain type to check
     * @return true if the terrain type has vegetation
     */
    static bool hasVegetation(terrain::TerrainType type);

    /**
     * Get the model type for a terrain type.
     *
     * @param type Terrain type
     * @return Corresponding VegetationModelType
     *
     * @note Returns BiolumeTree for non-vegetation terrain (caller should
     *       check hasVegetation first)
     */
    static VegetationModelType getModelType(terrain::TerrainType type);

    /**
     * Get instance count range for a terrain type.
     *
     * @param type Terrain type
     * @param[out] min_instances Minimum instances per tile
     * @param[out] max_instances Maximum instances per tile
     */
    static void getInstanceCountRange(
        terrain::TerrainType type,
        std::uint8_t& min_instances,
        std::uint8_t& max_instances);

    /**
     * Get the map seed.
     * @return Current map seed
     */
    std::uint64_t getMapSeed() const { return m_mapSeed; }

    /**
     * Get the chunk size in tiles.
     * @return Chunk size (32)
     */
    static constexpr std::int32_t getChunkSize() { return CHUNK_SIZE; }

private:
    /**
     * Compute a deterministic seed for a tile.
     *
     * Uses FNV-1a hash to combine tile coordinates and map seed
     * into a unique seed for that tile's PRNG.
     *
     * @param tile_x Tile X coordinate
     * @param tile_y Tile Y coordinate
     * @return Unique seed for this tile
     */
    std::uint64_t computeTileSeed(std::int32_t tile_x, std::int32_t tile_y) const;

    std::uint64_t m_mapSeed;                      ///< Global map seed
    const terrain::TerrainGrid& m_terrain;        ///< Reference to terrain data
};

} // namespace render
} // namespace sims3000

#endif // SIMS3000_RENDER_VEGETATION_INSTANCE_H
