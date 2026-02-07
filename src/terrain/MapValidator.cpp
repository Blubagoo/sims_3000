/**
 * @file MapValidator.cpp
 * @brief Implementation of post-generation map validation.
 */

#include <sims3000/terrain/MapValidator.h>
#include <sims3000/terrain/SpawnPointGenerator.h>
#include <chrono>
#include <cmath>

namespace sims3000 {
namespace terrain {

ValidationResult MapValidator::validate(
    const TerrainGrid& grid,
    const WaterDistanceField& waterDist,
    std::uint64_t seed,
    const ValidationConfig& config)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    ValidationResult result = {};
    result.totalTiles = static_cast<std::uint32_t>(grid.tile_count());

    // Count terrain types first (used by multiple checks)
    countTerrainTypes(grid,
        result.substrateCount,
        result.ridgeCount,
        result.waterCount,
        result.biomeCount);

    result.landTiles = result.totalTiles - result.waterCount;

    // 1. Check buildable area
    result.buildableAreaPassed = checkBuildableArea(
        grid,
        config.minBuildablePercent,
        result.buildableAreaPercent);

    // 2. Check river exists
    if (config.requireRiver) {
        result.riverExistsPassed = checkRiverExists(grid, result.riverTileCount);
    } else {
        result.riverExistsPassed = true;
        result.riverTileCount = 0;
        // Count anyway for info
        for (std::size_t i = 0; i < grid.tiles.size(); ++i) {
            if (grid.tiles[i].getTerrainType() == TerrainType::FlowChannel) {
                result.riverTileCount++;
            }
        }
    }

    // 3. Check coastline continuity
    if (config.checkCoastlineContinuity) {
        result.coastlineContinuityPassed = checkCoastlineContinuity(
            grid, result.coastlineGapCount);
    } else {
        result.coastlineContinuityPassed = true;
        result.coastlineGapCount = 0;
    }

    // 4. Check terrain anomalies
    if (config.checkTerrainAnomalies) {
        result.terrainAnomaliesPassed = checkTerrainAnomalies(
            grid, result.anomalyCount);
    } else {
        result.terrainAnomaliesPassed = true;
        result.anomalyCount = 0;
    }

    // 5. Check terrain distribution
    if (config.checkTerrainDistribution) {
        result.terrainDistributionPassed = checkTerrainDistribution(
            grid,
            config.substrateMinPercent,
            config.substrateMaxPercent,
            result.substratePercent);
    } else {
        result.terrainDistributionPassed = true;
        result.substratePercent = result.landTiles > 0
            ? static_cast<float>(result.substrateCount) / static_cast<float>(result.landTiles)
            : 0.0f;
    }

    // 6. Check spawn point quality
    if (config.checkSpawnPoints && config.playerCount >= 2) {
        result.spawnPointsPassed = checkSpawnPointQuality(
            grid, waterDist, seed,
            config.playerCount,
            config.minSpawnPointScore,
            result.minSpawnScore);
    } else {
        result.spawnPointsPassed = true;
        result.minSpawnScore = 1.0f;
    }

    // Calculate aggregate validity
    result.isValid =
        result.buildableAreaPassed &&
        result.riverExistsPassed &&
        result.coastlineContinuityPassed &&
        result.terrainAnomaliesPassed &&
        result.terrainDistributionPassed &&
        result.spawnPointsPassed;

    // Calculate aggregate score
    result.aggregateScore = calculateAggregateScore(result);

    auto endTime = std::chrono::high_resolution_clock::now();
    result.validationTimeMs = std::chrono::duration<float, std::milli>(
        endTime - startTime).count();

    return result;
}

bool MapValidator::checkBuildableArea(
    const TerrainGrid& grid,
    float minPercent,
    float& outPercent)
{
    std::uint32_t buildableCount = 0;
    std::uint32_t totalTiles = static_cast<std::uint32_t>(grid.tile_count());

    for (std::size_t i = 0; i < grid.tiles.size(); ++i) {
        if (isBuildable(grid.tiles[i].getTerrainType())) {
            ++buildableCount;
        }
    }

    outPercent = totalTiles > 0
        ? static_cast<float>(buildableCount) / static_cast<float>(totalTiles)
        : 0.0f;

    return outPercent >= minPercent;
}

bool MapValidator::checkRiverExists(
    const TerrainGrid& grid,
    std::uint32_t& outRiverTileCount)
{
    outRiverTileCount = 0;

    for (std::size_t i = 0; i < grid.tiles.size(); ++i) {
        if (grid.tiles[i].getTerrainType() == TerrainType::FlowChannel) {
            ++outRiverTileCount;
        }
    }

    return outRiverTileCount > 0;
}

bool MapValidator::checkCoastlineContinuity(
    const TerrainGrid& grid,
    std::uint32_t& outGapCount)
{
    outGapCount = 0;

    // First check if any deep void exists
    bool hasDeepVoid = false;
    for (std::size_t i = 0; i < grid.tiles.size() && !hasDeepVoid; ++i) {
        if (grid.tiles[i].getTerrainType() == TerrainType::DeepVoid) {
            hasDeepVoid = true;
        }
    }

    if (!hasDeepVoid) {
        // No ocean, no gaps possible
        return true;
    }

    // Check for single-tile gaps: non-ocean tiles surrounded by ocean
    for (std::uint16_t y = 1; y < grid.height - 1; ++y) {
        for (std::uint16_t x = 1; x < grid.width - 1; ++x) {
            TerrainType centerType = grid.at(x, y).getTerrainType();

            // Skip if center is ocean
            if (centerType == TerrainType::DeepVoid) {
                continue;
            }

            // Check if all 8 neighbors are deep void
            bool allNeighborsOcean = true;
            for (int dy = -1; dy <= 1 && allNeighborsOcean; ++dy) {
                for (int dx = -1; dx <= 1 && allNeighborsOcean; ++dx) {
                    if (dx == 0 && dy == 0) continue;

                    std::int32_t nx = static_cast<std::int32_t>(x) + dx;
                    std::int32_t ny = static_cast<std::int32_t>(y) + dy;

                    if (grid.at(nx, ny).getTerrainType() != TerrainType::DeepVoid) {
                        allNeighborsOcean = false;
                    }
                }
            }

            if (allNeighborsOcean) {
                ++outGapCount;
            }
        }
    }

    return outGapCount == 0;
}

bool MapValidator::checkTerrainAnomalies(
    const TerrainGrid& grid,
    std::uint32_t& outAnomalyCount)
{
    outAnomalyCount = 0;

    // Check interior tiles (exclude edges)
    for (std::uint16_t y = 1; y < grid.height - 1; ++y) {
        for (std::uint16_t x = 1; x < grid.width - 1; ++x) {
            if (isSingleTileAnomaly(grid, x, y)) {
                ++outAnomalyCount;
            }
        }
    }

    return outAnomalyCount == 0;
}

bool MapValidator::checkTerrainDistribution(
    const TerrainGrid& grid,
    float minSubstrate,
    float maxSubstrate,
    float& outSubstratePercent)
{
    std::uint32_t substrateCount = 0;
    std::uint32_t landCount = 0;

    for (std::size_t i = 0; i < grid.tiles.size(); ++i) {
        TerrainType type = grid.tiles[i].getTerrainType();

        if (!isWater(type)) {
            ++landCount;
            if (type == TerrainType::Substrate) {
                ++substrateCount;
            }
        }
    }

    outSubstratePercent = landCount > 0
        ? static_cast<float>(substrateCount) / static_cast<float>(landCount)
        : 0.0f;

    return outSubstratePercent >= minSubstrate && outSubstratePercent <= maxSubstrate;
}

bool MapValidator::checkSpawnPointQuality(
    const TerrainGrid& grid,
    const WaterDistanceField& waterDist,
    std::uint64_t seed,
    std::uint8_t playerCount,
    float minScore,
    float& outMinScore)
{
    // Generate spawn points with default config
    SpawnConfig spawnConfig = SpawnConfig::defaultConfig(playerCount);
    SpawnPointResult spawnResult = SpawnPointGenerator::generate(
        grid, waterDist, seed, spawnConfig);

    if (spawnResult.spawns.empty()) {
        outMinScore = 0.0f;
        return false;
    }

    outMinScore = spawnResult.minScore;

    // Check that all spawn points meet minimum score
    for (const auto& spawn : spawnResult.spawns) {
        if (spawn.score < minScore) {
            return false;
        }
    }

    return true;
}

float MapValidator::calculateAggregateScore(const ValidationResult& result)
{
    // Weight each check contribution to aggregate score
    const float buildableWeight = 0.25f;
    const float riverWeight = 0.15f;
    const float coastlineWeight = 0.10f;
    const float anomalyWeight = 0.10f;
    const float distributionWeight = 0.20f;
    const float spawnWeight = 0.20f;

    float score = 0.0f;

    // Buildable area: score based on how much above minimum
    // Perfect (100%) = 1.0, at minimum = 0.5, below = 0.0
    if (result.buildableAreaPercent >= 0.50f) {
        float excess = (result.buildableAreaPercent - 0.50f) / 0.50f;
        score += buildableWeight * (0.5f + 0.5f * std::min(1.0f, excess));
    }

    // River: binary pass/fail
    if (result.riverExistsPassed) {
        // Bonus for more river tiles (up to 5% of map)
        float riverCoverage = static_cast<float>(result.riverTileCount) /
            static_cast<float>(result.totalTiles);
        float riverScore = std::min(1.0f, riverCoverage / 0.05f);
        score += riverWeight * (0.7f + 0.3f * riverScore);
    }

    // Coastline: binary pass/fail
    if (result.coastlineContinuityPassed) {
        score += coastlineWeight;
    }

    // Anomalies: score inversely proportional to count
    if (result.anomalyCount == 0) {
        score += anomalyWeight;
    } else {
        // Degraded score for small number of anomalies
        float anomalyPenalty = std::min(1.0f, static_cast<float>(result.anomalyCount) / 10.0f);
        score += anomalyWeight * (1.0f - anomalyPenalty);
    }

    // Distribution: score based on distance from target center (40%)
    if (result.landTiles > 0) {
        float targetCenter = 0.40f;
        float distance = std::abs(result.substratePercent - targetCenter);
        float tolerance = 0.05f; // 35-45% range
        if (distance <= tolerance) {
            score += distributionWeight * (1.0f - distance / tolerance);
        }
    }

    // Spawn points: score based on minimum spawn score
    score += spawnWeight * result.minSpawnScore;

    return std::min(1.0f, std::max(0.0f, score));
}

bool MapValidator::isBuildable(TerrainType type)
{
    switch (type) {
        case TerrainType::Substrate:
        case TerrainType::Ridge:
        case TerrainType::BiolumeGrove:
        case TerrainType::PrismaFields:
        case TerrainType::SporeFlats:
        case TerrainType::EmberCrust:
            return true;

        case TerrainType::DeepVoid:
        case TerrainType::FlowChannel:
        case TerrainType::StillBasin:
        case TerrainType::BlightMires:
            return false;

        default:
            return false;
    }
}

bool MapValidator::isWater(TerrainType type)
{
    return type == TerrainType::DeepVoid ||
           type == TerrainType::FlowChannel ||
           type == TerrainType::StillBasin;
}

void MapValidator::countTerrainTypes(
    const TerrainGrid& grid,
    std::uint32_t& outSubstrate,
    std::uint32_t& outRidge,
    std::uint32_t& outWater,
    std::uint32_t& outBiome)
{
    outSubstrate = 0;
    outRidge = 0;
    outWater = 0;
    outBiome = 0;

    for (std::size_t i = 0; i < grid.tiles.size(); ++i) {
        TerrainType type = grid.tiles[i].getTerrainType();

        switch (type) {
            case TerrainType::Substrate:
                ++outSubstrate;
                break;

            case TerrainType::Ridge:
                ++outRidge;
                break;

            case TerrainType::DeepVoid:
            case TerrainType::FlowChannel:
            case TerrainType::StillBasin:
                ++outWater;
                break;

            case TerrainType::BiolumeGrove:
            case TerrainType::PrismaFields:
            case TerrainType::SporeFlats:
            case TerrainType::BlightMires:
            case TerrainType::EmberCrust:
                ++outBiome;
                break;

            default:
                break;
        }
    }
}

bool MapValidator::isSingleTileAnomaly(
    const TerrainGrid& grid,
    std::uint16_t x,
    std::uint16_t y)
{
    TerrainType centerType = grid.at(x, y).getTerrainType();

    // Get the type of the first neighbor (top-left)
    TerrainType firstNeighborType = grid.at(x - 1, y - 1).getTerrainType();

    // If center is same as neighbor, not an anomaly
    if (centerType == firstNeighborType) {
        return false;
    }

    // Check if ALL 8 neighbors are the same type (and different from center)
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;

            std::int32_t nx = static_cast<std::int32_t>(x) + dx;
            std::int32_t ny = static_cast<std::int32_t>(y) + dy;

            TerrainType neighborType = grid.at(nx, ny).getTerrainType();

            // If any neighbor differs from the first neighbor,
            // center is not completely surrounded by one type
            if (neighborType != firstNeighborType) {
                return false;
            }
        }
    }

    // Center type differs from all neighbors, and all neighbors are the same type
    return true;
}

} // namespace terrain
} // namespace sims3000
