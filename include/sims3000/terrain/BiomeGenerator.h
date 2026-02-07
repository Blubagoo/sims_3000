/**
 * @file BiomeGenerator.h
 * @brief Alien biome distribution using noise-based placement with ecological rules.
 *
 * Places the four alien terrain types and biolume groves using noise-based
 * distribution with coherent clustering. Biome placement respects geographic logic:
 * - BiolumeGroves: Lowlands and along flow channel banks (~8-12% of map)
 * - PrismaFields: Exposed ridgelines and plateaus (~2-4% of map)
 * - SporeFlats: Transitional zones between substrate and groves (~3-5% of map)
 * - BlightMires: Lowlands, never blocking all expansion paths (~3-5% of map)
 * - EmberCrust: High elevation volcanic ridges (~3-6% of map)
 *
 * Features:
 * - Separate noise channels for each biome type for independent clustering
 * - Elevation-dependent placement rules
 * - Water proximity rules (BiolumeGroves near FlowChannels)
 * - Cluster-based placement (no single-tile scatter)
 * - Fully deterministic generation (same seed = same biomes)
 * - Preserves existing water tiles (DeepVoid, FlowChannel, StillBasin)
 *
 * @see ElevationGenerator.h for elevation data
 * @see ProceduralNoise.h for underlying noise functions
 * @see TerrainGrid.h for grid storage
 */

#ifndef SIMS3000_TERRAIN_BIOMEGENERATOR_H
#define SIMS3000_TERRAIN_BIOMEGENERATOR_H

#include <sims3000/terrain/ProceduralNoise.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <sims3000/terrain/WaterDistanceField.h>
#include <cstdint>

namespace sims3000 {
namespace terrain {

/**
 * @struct BiomeConfig
 * @brief Configuration for biome distribution generation.
 *
 * Controls noise parameters, elevation thresholds, and coverage targets
 * for each alien biome type. All values have sensible defaults that
 * produce visually distinct and ecologically plausible distributions.
 */
struct BiomeConfig {
    // =========================================================================
    // Global Noise Parameters
    // =========================================================================

    /**
     * @brief Base feature scale for biome noise.
     *
     * Controls the size of biome clusters. Lower values = larger clusters.
     * This is the base scale; individual biomes may modify it.
     */
    float baseFeatureScale = 0.015f;

    /**
     * @brief Number of noise octaves for biome sampling.
     *
     * More octaves add finer detail to cluster boundaries.
     */
    std::uint8_t octaves = 4;

    /**
     * @brief Persistence for noise octaves.
     *
     * Controls how quickly amplitude decreases at higher frequencies.
     */
    float persistence = 0.5f;

    // =========================================================================
    // Elevation Thresholds (0-31 range)
    // =========================================================================

    /**
     * @brief Maximum elevation for lowland biomes (BiolumeGrove, BlightMires).
     *
     * Tiles at or below this elevation are considered lowlands.
     */
    std::uint8_t lowlandMaxElevation = 10;

    /**
     * @brief Minimum elevation for highland biomes (PrismaFields, EmberCrust).
     *
     * Tiles at or above this elevation are considered highlands.
     */
    std::uint8_t highlandMinElevation = 18;

    /**
     * @brief Minimum elevation for EmberCrust (volcanic ridges).
     *
     * EmberCrust only appears at very high elevations.
     */
    std::uint8_t volcanicMinElevation = 22;

    /**
     * @brief Minimum elevation for ridge classification (PrismaFields).
     *
     * PrismaFields prefer ridgeline terrain.
     */
    std::uint8_t ridgeMinElevation = 20;

    // =========================================================================
    // Water Proximity Rules
    // =========================================================================

    /**
     * @brief Maximum distance from water for BiolumeGrove bank placement.
     *
     * BiolumeGroves along flow channel banks must be within this distance.
     */
    std::uint8_t groveWaterProximityMax = 3;

    /**
     * @brief Minimum distance from water for BlightMires.
     *
     * BlightMires should not be immediately adjacent to water.
     */
    std::uint8_t mireWaterProximityMin = 2;

    // =========================================================================
    // Coverage Targets (as percentages of non-water, non-ridge tiles)
    // =========================================================================

    /**
     * @brief Target coverage for BiolumeGrove (~8-12%).
     */
    float groveTargetCoverage = 0.10f;

    /**
     * @brief Target coverage for PrismaFields (~2-4%).
     */
    float prismaTargetCoverage = 0.03f;

    /**
     * @brief Target coverage for SporeFlats (~3-5%).
     */
    float sporeTargetCoverage = 0.04f;

    /**
     * @brief Target coverage for BlightMires (~3-5%).
     */
    float mireTargetCoverage = 0.04f;

    /**
     * @brief Target coverage for EmberCrust (~3-6%).
     */
    float emberTargetCoverage = 0.045f;

    // =========================================================================
    // Noise Thresholds (determines biome placement probability)
    // =========================================================================

    /**
     * @brief Noise threshold for BiolumeGrove (lower = more coverage).
     */
    float groveNoiseThreshold = 0.55f;

    /**
     * @brief Noise threshold for PrismaFields (higher = rarer).
     */
    float prismaNoiseThreshold = 0.72f;

    /**
     * @brief Noise threshold for SporeFlats.
     */
    float sporeNoiseThreshold = 0.58f;

    /**
     * @brief Noise threshold for BlightMires.
     */
    float mireNoiseThreshold = 0.60f;

    /**
     * @brief Noise threshold for EmberCrust.
     */
    float emberNoiseThreshold = 0.55f;

    // =========================================================================
    // Seed Offsets (for independent noise channels)
    // =========================================================================

    /**
     * @brief Seed offset for BiolumeGrove noise.
     */
    std::int32_t groveSeedOffset = 1000;

    /**
     * @brief Seed offset for PrismaFields noise.
     */
    std::int32_t prismaSeedOffset = 2000;

    /**
     * @brief Seed offset for SporeFlats noise.
     */
    std::int32_t sporeSeedOffset = 3000;

    /**
     * @brief Seed offset for BlightMires noise.
     */
    std::int32_t mireSeedOffset = 4000;

    /**
     * @brief Seed offset for EmberCrust noise.
     */
    std::int32_t emberSeedOffset = 5000;

    // =========================================================================
    // Cluster Size Parameters
    // =========================================================================

    /**
     * @brief Minimum cluster radius for biomes (in tiles).
     *
     * Prevents single-tile scatter by requiring minimum cluster sizes.
     */
    std::uint8_t minClusterRadius = 2;

    /**
     * @brief Feature scale multiplier for cluster coherence noise.
     *
     * Lower values create more coherent (larger) clusters.
     */
    float clusterCoherenceScale = 0.5f;

    // =========================================================================
    // BlightMire Expansion Safety
    // =========================================================================

    /**
     * @brief Minimum distance between BlightMire patches (in tiles).
     *
     * Prevents BlightMires from blocking all expansion paths.
     * This creates gaps that players can navigate through.
     */
    std::uint8_t mireMinPatchDistance = 8;

    /**
     * @brief Maximum BlightMire patches per map edge.
     *
     * Limits BlightMire coverage near map edges to ensure expansion paths.
     */
    std::uint8_t mireMaxPatchesPerEdge = 2;

    // =========================================================================
    // Factory Methods
    // =========================================================================

    /**
     * @brief Default configuration for standard biome distribution.
     */
    static BiomeConfig defaultConfig() {
        return BiomeConfig{};
    }

    /**
     * @brief Configuration for lush maps with more vegetation.
     */
    static BiomeConfig lush() {
        BiomeConfig cfg;
        cfg.groveTargetCoverage = 0.14f;
        cfg.groveNoiseThreshold = 0.48f;
        cfg.sporeTargetCoverage = 0.06f;
        cfg.sporeNoiseThreshold = 0.52f;
        cfg.mireTargetCoverage = 0.03f;
        cfg.mireNoiseThreshold = 0.65f;
        return cfg;
    }

    /**
     * @brief Configuration for harsh/volcanic maps.
     */
    static BiomeConfig volcanic() {
        BiomeConfig cfg;
        cfg.emberTargetCoverage = 0.08f;
        cfg.emberNoiseThreshold = 0.45f;
        cfg.volcanicMinElevation = 18;
        cfg.groveTargetCoverage = 0.06f;
        cfg.groveNoiseThreshold = 0.62f;
        cfg.mireTargetCoverage = 0.05f;
        return cfg;
    }

    /**
     * @brief Configuration for crystalline maps with more PrismaFields.
     */
    static BiomeConfig crystalline() {
        BiomeConfig cfg;
        cfg.prismaTargetCoverage = 0.06f;
        cfg.prismaNoiseThreshold = 0.58f;
        cfg.highlandMinElevation = 15;
        cfg.groveTargetCoverage = 0.08f;
        return cfg;
    }
};

// Verify BiomeConfig is trivially copyable for serialization
static_assert(std::is_trivially_copyable<BiomeConfig>::value,
    "BiomeConfig must be trivially copyable");

/**
 * @struct BiomeResult
 * @brief Statistics from biome generation.
 *
 * Provides information about the generated biome distribution for
 * debugging and verification purposes.
 */
struct BiomeResult {
    // Tile counts by biome type
    std::uint32_t groveCount;       ///< BiolumeGrove tile count
    std::uint32_t prismaCount;      ///< PrismaFields tile count
    std::uint32_t sporeCount;       ///< SporeFlats tile count
    std::uint32_t mireCount;        ///< BlightMires tile count
    std::uint32_t emberCount;       ///< EmberCrust tile count
    std::uint32_t substrateCount;   ///< Remaining Substrate tiles
    std::uint32_t ridgeCount;       ///< Ridge tiles (not modified)
    std::uint32_t waterCount;       ///< Water tiles (not modified)

    // Coverage percentages (of total land tiles)
    float groveCoverage;            ///< BiolumeGrove coverage %
    float prismaCoverage;           ///< PrismaFields coverage %
    float sporeCoverage;            ///< SporeFlats coverage %
    float mireCoverage;             ///< BlightMires coverage %
    float emberCoverage;            ///< EmberCrust coverage %
    float substrateCoverage;        ///< Substrate coverage %

    std::uint32_t totalTiles;       ///< Total tiles processed
    std::uint32_t landTiles;        ///< Non-water tiles
    float generationTimeMs;         ///< Time taken to generate (milliseconds)

    /**
     * @brief Check if at least one BlightMire patch exists.
     *
     * Acceptance criterion: Every map must have at least one blight mire patch.
     */
    bool hasBlightMirePatch() const {
        return mireCount > 0;
    }

    /**
     * @brief Check if PrismaFields is the rarest special terrain.
     *
     * Acceptance criterion: PrismaFields should be the rarest.
     */
    bool isPrismaRarest() const {
        return prismaCount <= groveCount &&
               prismaCount <= sporeCount &&
               prismaCount <= mireCount &&
               prismaCount <= emberCount;
    }
};

/**
 * @class BiomeGenerator
 * @brief Generates alien biome distribution using noise-based placement.
 *
 * Uses separate noise channels for each biome type with elevation-dependent
 * and proximity-dependent placement rules. Biomes form coherent clusters,
 * not random scatter.
 *
 * Usage:
 * @code
 * TerrainGrid grid(MapSize::Medium);
 * ElevationGenerator::generate(grid, seed);  // Generate elevation first
 * WaterDistanceField waterDist(MapSize::Medium);
 * waterDist.compute(grid);  // Compute water distances
 * BiomeConfig config = BiomeConfig::defaultConfig();
 * BiomeResult result = BiomeGenerator::generate(grid, waterDist, seed, config);
 * @endcode
 *
 * Thread Safety:
 * - generate() is NOT thread-safe (modifies grid)
 *
 * @note Generation is single-threaded for deterministic output.
 * @note Must be called AFTER ElevationGenerator::generate().
 * @note Must be called AFTER WaterDistanceField::compute().
 */
class BiomeGenerator {
public:
    /**
     * @brief Generate biome distribution for the entire grid.
     *
     * Places alien biomes using noise-based clustering with elevation
     * and proximity rules. Preserves water tiles and existing Ridge tiles.
     *
     * Order of biome placement (to ensure proper layering):
     * 1. EmberCrust (highest elevation requirement)
     * 2. PrismaFields (ridge requirement)
     * 3. BiolumeGrove (lowlands and water banks)
     * 4. BlightMires (lowlands, with expansion safety)
     * 5. SporeFlats (transitional zones)
     *
     * @param grid The terrain grid with elevation data already set.
     * @param waterDist Pre-computed water distance field.
     * @param seed Random seed for deterministic generation.
     * @param config Configuration parameters for biome distribution.
     * @return Statistics about the generated biome distribution.
     *
     * @note Grid must have elevation data from ElevationGenerator.
     * @note WaterDistanceField must be computed before calling this.
     */
    static BiomeResult generate(
        TerrainGrid& grid,
        const WaterDistanceField& waterDist,
        std::uint64_t seed,
        const BiomeConfig& config = BiomeConfig::defaultConfig()
    );

    /**
     * @brief Check if a tile is eligible for biome placement.
     *
     * Tiles that are water or already have a biome assigned are not eligible.
     *
     * @param type Current terrain type of the tile.
     * @return true if the tile can have a biome placed on it.
     */
    static bool isEligibleForBiome(TerrainType type);

    /**
     * @brief Sample biome noise value at a specific coordinate.
     *
     * Returns a normalized value (0.0 to 1.0) for the specified biome
     * noise channel.
     *
     * @param noise Pre-initialized SimplexNoise generator.
     * @param x X coordinate in grid space.
     * @param y Y coordinate in grid space.
     * @param config Configuration parameters.
     * @param seedOffset Seed offset for this biome's noise channel.
     * @return Normalized noise value (0.0 to 1.0).
     */
    static float sampleBiomeNoise(
        const SimplexNoise& noise,
        float x,
        float y,
        const BiomeConfig& config,
        std::int32_t seedOffset
    );

    /**
     * @brief Check if a tile meets elevation requirements for EmberCrust.
     */
    static bool isEmberElevation(std::uint8_t elevation, const BiomeConfig& config);

    /**
     * @brief Check if a tile meets elevation requirements for PrismaFields.
     */
    static bool isPrismaElevation(std::uint8_t elevation, const BiomeConfig& config);

    /**
     * @brief Check if a tile meets elevation requirements for lowland biomes.
     */
    static bool isLowlandElevation(std::uint8_t elevation, const BiomeConfig& config);

    /**
     * @brief Check if a tile is near water (for BiolumeGrove bank placement).
     */
    static bool isNearWater(
        const WaterDistanceField& waterDist,
        std::uint16_t x,
        std::uint16_t y,
        const BiomeConfig& config
    );

private:
    /**
     * @brief Place EmberCrust biome on eligible tiles.
     */
    static std::uint32_t placeEmberCrust(
        TerrainGrid& grid,
        const SimplexNoise& noise,
        const BiomeConfig& config
    );

    /**
     * @brief Place PrismaFields biome on eligible tiles.
     */
    static std::uint32_t placePrismaFields(
        TerrainGrid& grid,
        const SimplexNoise& noise,
        const BiomeConfig& config
    );

    /**
     * @brief Place BiolumeGrove biome on eligible tiles.
     */
    static std::uint32_t placeBiolumeGrove(
        TerrainGrid& grid,
        const WaterDistanceField& waterDist,
        const SimplexNoise& noise,
        const BiomeConfig& config
    );

    /**
     * @brief Place BlightMires biome on eligible tiles.
     */
    static std::uint32_t placeBlightMires(
        TerrainGrid& grid,
        const WaterDistanceField& waterDist,
        const SimplexNoise& noise,
        const BiomeConfig& config
    );

    /**
     * @brief Place SporeFlats biome on eligible tiles.
     */
    static std::uint32_t placeSporeFlats(
        TerrainGrid& grid,
        const SimplexNoise& noise,
        const BiomeConfig& config
    );

    /**
     * @brief Check if tile is adjacent to a specific terrain type.
     *
     * Used for transitional zone detection (SporeFlats).
     */
    static bool isAdjacentToType(
        const TerrainGrid& grid,
        std::uint16_t x,
        std::uint16_t y,
        TerrainType type
    );

    /**
     * @brief Check if tile is in a transitional zone.
     *
     * Transitional zones are areas between substrate and groves.
     */
    static bool isTransitionalZone(
        const TerrainGrid& grid,
        std::uint16_t x,
        std::uint16_t y
    );
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_BIOMEGENERATOR_H
