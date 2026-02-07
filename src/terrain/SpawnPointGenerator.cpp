/**
 * @file SpawnPointGenerator.cpp
 * @brief Implementation of multiplayer spawn point selection with terrain scoring.
 */

#include <sims3000/terrain/SpawnPointGenerator.h>
#include <cmath>
#include <algorithm>
#include <chrono>

namespace sims3000 {
namespace terrain {

namespace {

// Mathematical constants
constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;

// Maximum search iterations to prevent infinite loops
constexpr std::uint32_t MAX_SEARCH_ITERATIONS = 1000;

// Minimum tiles for valid scoring
constexpr std::uint32_t MIN_TILES_FOR_SCORING = 10;

} // anonymous namespace

SpawnPointResult SpawnPointGenerator::generate(
    const TerrainGrid& grid,
    const WaterDistanceField& waterDist,
    std::uint64_t seed,
    const SpawnConfig& config)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    SpawnPointResult result;
    result.isValid = false;
    result.isFair = false;
    result.minScore = 0.0f;
    result.maxScore = 0.0f;
    result.scoreDifference = 1.0f;

    // Validate player count
    if (config.playerCount < 2 || config.playerCount > 4) {
        auto endTime = std::chrono::high_resolution_clock::now();
        result.generationTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
        return result;
    }

    // Initialize RNG
    Xoshiro256 rng(seed);

    // Find candidate positions using rotational symmetry
    std::vector<GridPosition> candidates = findSymmetricCandidates(
        grid, waterDist, rng, config);

    if (candidates.size() < config.playerCount) {
        // Not enough valid candidates found
        auto endTime = std::chrono::high_resolution_clock::now();
        result.generationTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
        return result;
    }

    // Score and select best spawn positions
    result.spawns = selectBestSpawns(grid, waterDist, candidates, config);

    if (result.spawns.size() < config.playerCount) {
        auto endTime = std::chrono::high_resolution_clock::now();
        result.generationTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
        return result;
    }

    // Validate all positions meet placement rules
    result.isValid = true;
    for (const auto& spawn : result.spawns) {
        if (!isValidSpawnPosition(grid, waterDist, spawn.position, config)) {
            result.isValid = false;
            break;
        }
    }

    // Calculate score statistics
    result.minScore = result.spawns[0].score;
    result.maxScore = result.spawns[0].score;
    for (const auto& spawn : result.spawns) {
        result.minScore = std::min(result.minScore, spawn.score);
        result.maxScore = std::max(result.maxScore, spawn.score);
    }

    // Calculate score difference
    if (result.maxScore > 0.0f) {
        result.scoreDifference = (result.maxScore - result.minScore) / result.maxScore;
    } else {
        result.scoreDifference = 0.0f;
    }

    // Check fairness tolerance
    result.isFair = result.scoreDifference <= config.scoreTolerance;

    auto endTime = std::chrono::high_resolution_clock::now();
    result.generationTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    return result;
}

MapSpawnData SpawnPointGenerator::toMapSpawnData(
    const SpawnPointResult& result,
    std::uint64_t seed)
{
    MapSpawnData data;
    data.generationSeed = seed;
    data.playerCount = static_cast<std::uint8_t>(
        std::min(result.spawns.size(), static_cast<std::size_t>(MapSpawnData::MAX_PLAYERS)));

    for (std::uint8_t i = 0; i < data.playerCount; ++i) {
        data.spawnPositions[i] = result.spawns[i].position;
        data.spawnScores[i] = result.spawns[i].score;
    }

    // Zero out unused slots
    for (std::uint8_t i = data.playerCount; i < MapSpawnData::MAX_PLAYERS; ++i) {
        data.spawnPositions[i] = GridPosition{0, 0};
        data.spawnScores[i] = 0.0f;
    }

    return data;
}

float SpawnPointGenerator::calculateTerrainScore(
    const TerrainGrid& grid,
    const WaterDistanceField& waterDist,
    GridPosition pos,
    const SpawnConfig& config)
{
    // Fluid access score (closer = better)
    float fluidDist = static_cast<float>(waterDist.get_water_distance(pos.x, pos.y));
    float fluidScore = 1.0f - std::min(fluidDist / static_cast<float>(config.fluidAccessMaxDistance), 1.0f);

    // Buildable area score
    std::uint32_t buildableCount = countBuildableTiles(grid, pos, config.scoringRadius);
    std::uint32_t totalTilesInRadius = static_cast<std::uint32_t>(
        PI * config.scoringRadius * config.scoringRadius);
    totalTilesInRadius = std::max(totalTilesInRadius, MIN_TILES_FOR_SCORING);
    float buildableScore = static_cast<float>(buildableCount) / static_cast<float>(totalTilesInRadius);
    buildableScore = std::min(buildableScore, 1.0f);

    // Special terrain score
    std::uint32_t specialCount = countSpecialTerrain(grid, pos, config.scoringRadius);
    float specialScore = std::min(static_cast<float>(specialCount) / 20.0f, 1.0f);

    // Contamination exposure (farther = better)
    float contaminationDist = findContaminationDistance(grid, pos, config.scoringRadius * 2);
    float contaminationScore = std::min(contaminationDist / static_cast<float>(config.blightMireMinDistance * 2), 1.0f);

    // Elevation advantage
    float avgElevation = calculateAvgElevation(grid, pos, config.scoringRadius);
    float elevationScore = avgElevation / static_cast<float>(TerrainComponent::MAX_ELEVATION);

    // Calculate weighted total
    float totalScore =
        config.weightFluidAccess * fluidScore +
        config.weightBuildableArea * buildableScore +
        config.weightSpecialTerrain * specialScore +
        config.weightContaminationExposure * contaminationScore +
        config.weightElevationAdvantage * elevationScore;

    return std::clamp(totalScore, 0.0f, 1.0f);
}

bool SpawnPointGenerator::isValidSpawnPosition(
    const TerrainGrid& grid,
    const WaterDistanceField& waterDist,
    GridPosition pos,
    const SpawnConfig& config)
{
    // Check bounds
    if (!grid.in_bounds(pos.x, pos.y)) {
        return false;
    }

    // Check edge margin
    if (pos.x < config.edgeMargin || pos.y < config.edgeMargin ||
        pos.x >= static_cast<std::int16_t>(grid.width) - config.edgeMargin ||
        pos.y >= static_cast<std::int16_t>(grid.height) - config.edgeMargin) {
        return false;
    }

    // Check spawn position is on or adjacent to substrate
    const TerrainComponent& centerTile = grid.at(pos.x, pos.y);
    if (!isBuildable(centerTile.getTerrainType())) {
        // Check if adjacent to buildable
        bool hasAdjacentBuildable = false;
        for (std::int16_t dy = -1; dy <= 1 && !hasAdjacentBuildable; ++dy) {
            for (std::int16_t dx = -1; dx <= 1 && !hasAdjacentBuildable; ++dx) {
                if (dx == 0 && dy == 0) continue;
                std::int16_t nx = pos.x + dx;
                std::int16_t ny = pos.y + dy;
                if (grid.in_bounds(nx, ny)) {
                    if (isBuildable(grid.at(nx, ny).getTerrainType())) {
                        hasAdjacentBuildable = true;
                    }
                }
            }
        }
        if (!hasAdjacentBuildable) {
            return false;
        }
    }

    // Check minimum buildable radius
    if (!checkBuildableRadius(grid, pos, config.minBuildableRadius)) {
        return false;
    }

    // Check fluid access
    std::uint8_t fluidDist = waterDist.get_water_distance(pos.x, pos.y);
    if (fluidDist > config.fluidAccessMaxDistance) {
        return false;
    }

    // Check blight mire distance
    float contaminationDist = findContaminationDistance(grid, pos, config.blightMireMinDistance + 5);
    if (contaminationDist < static_cast<float>(config.blightMireMinDistance)) {
        return false;
    }

    return true;
}

float SpawnPointGenerator::getSymmetryAngle(std::uint8_t playerCount) {
    switch (playerCount) {
        case 2: return 180.0f;
        case 3: return 120.0f;
        case 4: return 90.0f;
        default: return 180.0f;
    }
}

float SpawnPointGenerator::calculateDistance(GridPosition a, GridPosition b) {
    float dx = static_cast<float>(a.x - b.x);
    float dy = static_cast<float>(a.y - b.y);
    return std::sqrt(dx * dx + dy * dy);
}

std::vector<GridPosition> SpawnPointGenerator::findSymmetricCandidates(
    const TerrainGrid& grid,
    const WaterDistanceField& waterDist,
    Xoshiro256& rng,
    const SpawnConfig& config)
{
    std::vector<GridPosition> candidates;

    // Calculate map center
    float centerX = static_cast<float>(grid.width) / 2.0f;
    float centerY = static_cast<float>(grid.height) / 2.0f;

    // Calculate spawn radius from center
    float minDim = static_cast<float>(std::min(grid.width, grid.height));
    float spawnRadius = minDim * config.spawnRadiusFraction;

    // Get symmetry angle
    float symmetryAngle = getSymmetryAngle(config.playerCount);

    // Random starting angle for variety
    float startAngle = rng.nextFloat() * 360.0f;

    // Search for valid positions at each symmetric angle
    for (std::uint8_t playerIdx = 0; playerIdx < config.playerCount; ++playerIdx) {
        float targetAngle = startAngle + playerIdx * symmetryAngle;

        // Convert to radians
        float angleRad = targetAngle * DEG_TO_RAD;

        // Calculate base position
        float baseX = centerX + spawnRadius * std::cos(angleRad);
        float baseY = centerY + spawnRadius * std::sin(angleRad);

        // Search around base position for valid spawn
        bool found = false;
        GridPosition bestPos{0, 0};
        float bestScore = -1.0f;

        // Spiral search around base position
        for (std::uint32_t searchRadius = 0;
             searchRadius < config.scoringRadius && !found;
             ++searchRadius) {
            // Search in a ring at this radius
            for (float searchAngle = 0.0f; searchAngle < 360.0f; searchAngle += 15.0f) {
                float searchRad = searchAngle * DEG_TO_RAD;
                std::int16_t testX = static_cast<std::int16_t>(
                    baseX + searchRadius * std::cos(searchRad));
                std::int16_t testY = static_cast<std::int16_t>(
                    baseY + searchRadius * std::sin(searchRad));

                GridPosition testPos{testX, testY};

                if (isValidSpawnPosition(grid, waterDist, testPos, config)) {
                    float score = calculateTerrainScore(grid, waterDist, testPos, config);
                    if (score > bestScore) {
                        bestScore = score;
                        bestPos = testPos;
                        found = true;
                    }
                }
            }
        }

        // Try with radius variation if not found
        if (!found) {
            for (float radiusMult = 0.8f; radiusMult <= 1.2f && !found; radiusMult += 0.1f) {
                float testRadius = spawnRadius * radiusMult;
                float testX = centerX + testRadius * std::cos(angleRad);
                float testY = centerY + testRadius * std::sin(angleRad);

                GridPosition testPos{
                    static_cast<std::int16_t>(testX),
                    static_cast<std::int16_t>(testY)
                };

                if (isValidSpawnPosition(grid, waterDist, testPos, config)) {
                    bestPos = testPos;
                    found = true;
                }
            }
        }

        if (found) {
            candidates.push_back(bestPos);
        }
    }

    return candidates;
}

std::vector<SpawnPoint> SpawnPointGenerator::selectBestSpawns(
    const TerrainGrid& grid,
    const WaterDistanceField& waterDist,
    const std::vector<GridPosition>& candidates,
    const SpawnConfig& config)
{
    std::vector<SpawnPoint> spawns;
    spawns.reserve(candidates.size());

    std::uint8_t playerIdx = 0;
    for (const auto& pos : candidates) {
        if (playerIdx >= config.playerCount) break;

        SpawnPoint spawn;
        spawn.position = pos;
        spawn.playerIndex = playerIdx;

        // Calculate detailed scores
        spawn.fluidDistance = static_cast<float>(waterDist.get_water_distance(pos.x, pos.y));
        spawn.contaminationDistance = findContaminationDistance(grid, pos, config.scoringRadius * 2);
        spawn.avgElevation = calculateAvgElevation(grid, pos, config.scoringRadius);

        std::uint32_t buildableCount = countBuildableTiles(grid, pos, config.scoringRadius);
        std::uint32_t totalTilesInRadius = static_cast<std::uint32_t>(
            PI * config.scoringRadius * config.scoringRadius);
        totalTilesInRadius = std::max(totalTilesInRadius, MIN_TILES_FOR_SCORING);
        spawn.buildableAreaFraction = static_cast<float>(buildableCount) /
                                      static_cast<float>(totalTilesInRadius);
        spawn.buildableAreaFraction = std::min(spawn.buildableAreaFraction, 1.0f);

        // Calculate total score
        spawn.score = calculateTerrainScore(grid, waterDist, pos, config);

        spawns.push_back(spawn);
        ++playerIdx;
    }

    return spawns;
}

bool SpawnPointGenerator::checkBuildableRadius(
    const TerrainGrid& grid,
    GridPosition center,
    std::uint8_t radius)
{
    std::uint32_t buildableCount = 0;
    std::uint32_t totalChecked = 0;
    float radiusSq = static_cast<float>(radius * radius);

    for (std::int16_t dy = -static_cast<std::int16_t>(radius);
         dy <= static_cast<std::int16_t>(radius); ++dy) {
        for (std::int16_t dx = -static_cast<std::int16_t>(radius);
             dx <= static_cast<std::int16_t>(radius); ++dx) {
            float distSq = static_cast<float>(dx * dx + dy * dy);
            if (distSq > radiusSq) continue;

            std::int16_t nx = center.x + dx;
            std::int16_t ny = center.y + dy;

            if (!grid.in_bounds(nx, ny)) {
                // Out of bounds counts as not buildable
                ++totalChecked;
                continue;
            }

            const TerrainComponent& tile = grid.at(nx, ny);
            if (isBuildable(tile.getTerrainType()) || tile.isCleared()) {
                ++buildableCount;
            }
            ++totalChecked;
        }
    }

    // Require at least 80% buildable for spawn to be valid
    if (totalChecked == 0) return false;
    float buildableRatio = static_cast<float>(buildableCount) / static_cast<float>(totalChecked);
    return buildableRatio >= 0.8f;
}

float SpawnPointGenerator::findContaminationDistance(
    const TerrainGrid& grid,
    GridPosition pos,
    std::uint8_t searchRadius)
{
    float minDist = static_cast<float>(searchRadius + 1);

    for (std::int16_t dy = -static_cast<std::int16_t>(searchRadius);
         dy <= static_cast<std::int16_t>(searchRadius); ++dy) {
        for (std::int16_t dx = -static_cast<std::int16_t>(searchRadius);
             dx <= static_cast<std::int16_t>(searchRadius); ++dx) {
            std::int16_t nx = pos.x + dx;
            std::int16_t ny = pos.y + dy;

            if (!grid.in_bounds(nx, ny)) continue;

            const TerrainComponent& tile = grid.at(nx, ny);
            if (tile.getTerrainType() == TerrainType::BlightMires) {
                float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                minDist = std::min(minDist, dist);
            }
        }
    }

    return minDist;
}

std::uint32_t SpawnPointGenerator::countSpecialTerrain(
    const TerrainGrid& grid,
    GridPosition center,
    std::uint8_t radius)
{
    std::uint32_t count = 0;
    float radiusSq = static_cast<float>(radius * radius);

    for (std::int16_t dy = -static_cast<std::int16_t>(radius);
         dy <= static_cast<std::int16_t>(radius); ++dy) {
        for (std::int16_t dx = -static_cast<std::int16_t>(radius);
             dx <= static_cast<std::int16_t>(radius); ++dx) {
            float distSq = static_cast<float>(dx * dx + dy * dy);
            if (distSq > radiusSq) continue;

            std::int16_t nx = center.x + dx;
            std::int16_t ny = center.y + dy;

            if (!grid.in_bounds(nx, ny)) continue;

            if (isSpecialTerrain(grid.at(nx, ny).getTerrainType())) {
                ++count;
            }
        }
    }

    return count;
}

float SpawnPointGenerator::calculateAvgElevation(
    const TerrainGrid& grid,
    GridPosition center,
    std::uint8_t radius)
{
    std::uint32_t totalElevation = 0;
    std::uint32_t count = 0;
    float radiusSq = static_cast<float>(radius * radius);

    for (std::int16_t dy = -static_cast<std::int16_t>(radius);
         dy <= static_cast<std::int16_t>(radius); ++dy) {
        for (std::int16_t dx = -static_cast<std::int16_t>(radius);
             dx <= static_cast<std::int16_t>(radius); ++dx) {
            float distSq = static_cast<float>(dx * dx + dy * dy);
            if (distSq > radiusSq) continue;

            std::int16_t nx = center.x + dx;
            std::int16_t ny = center.y + dy;

            if (!grid.in_bounds(nx, ny)) continue;

            totalElevation += grid.at(nx, ny).getElevation();
            ++count;
        }
    }

    if (count == 0) return 0.0f;
    return static_cast<float>(totalElevation) / static_cast<float>(count);
}

std::uint32_t SpawnPointGenerator::countBuildableTiles(
    const TerrainGrid& grid,
    GridPosition center,
    std::uint8_t radius)
{
    std::uint32_t count = 0;
    float radiusSq = static_cast<float>(radius * radius);

    for (std::int16_t dy = -static_cast<std::int16_t>(radius);
         dy <= static_cast<std::int16_t>(radius); ++dy) {
        for (std::int16_t dx = -static_cast<std::int16_t>(radius);
             dx <= static_cast<std::int16_t>(radius); ++dx) {
            float distSq = static_cast<float>(dx * dx + dy * dy);
            if (distSq > radiusSq) continue;

            std::int16_t nx = center.x + dx;
            std::int16_t ny = center.y + dy;

            if (!grid.in_bounds(nx, ny)) continue;

            const TerrainComponent& tile = grid.at(nx, ny);
            if (isBuildable(tile.getTerrainType()) || tile.isCleared()) {
                ++count;
            }
        }
    }

    return count;
}

bool SpawnPointGenerator::isBuildable(TerrainType type) {
    switch (type) {
        case TerrainType::Substrate:
        case TerrainType::Ridge:
            return true;

        // Clearable types (buildable after clearing)
        case TerrainType::BiolumeGrove:
        case TerrainType::PrismaFields:
        case TerrainType::SporeFlats:
        case TerrainType::EmberCrust:
            return true;

        // Not buildable
        case TerrainType::DeepVoid:
        case TerrainType::FlowChannel:
        case TerrainType::StillBasin:
        case TerrainType::BlightMires:
            return false;

        default:
            return false;
    }
}

bool SpawnPointGenerator::isSpecialTerrain(TerrainType type) {
    switch (type) {
        case TerrainType::PrismaFields:  // Crystal bonus
        case TerrainType::BiolumeGrove:  // Forest value
        case TerrainType::SporeFlats:    // Harmony bonus
            return true;
        default:
            return false;
    }
}

GridPosition SpawnPointGenerator::rotateAroundCenter(
    GridPosition pos,
    float angleDegrees,
    std::uint16_t mapWidth,
    std::uint16_t mapHeight)
{
    float centerX = static_cast<float>(mapWidth) / 2.0f;
    float centerY = static_cast<float>(mapHeight) / 2.0f;

    // Translate to origin
    float relX = static_cast<float>(pos.x) - centerX;
    float relY = static_cast<float>(pos.y) - centerY;

    // Rotate
    float angleRad = angleDegrees * DEG_TO_RAD;
    float cosA = std::cos(angleRad);
    float sinA = std::sin(angleRad);

    float rotX = relX * cosA - relY * sinA;
    float rotY = relX * sinA + relY * cosA;

    // Translate back
    return GridPosition{
        static_cast<std::int16_t>(rotX + centerX),
        static_cast<std::int16_t>(rotY + centerY)
    };
}

} // namespace terrain
} // namespace sims3000
