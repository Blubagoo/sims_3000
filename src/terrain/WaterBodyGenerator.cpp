/**
 * @file WaterBodyGenerator.cpp
 * @brief Implementation of water body generation.
 *
 * Generates oceans (DeepVoid), rivers (FlowChannel), and lakes (StillBasin)
 * using elevation heightmap data. Assigns water body IDs, flow directions,
 * and updates terrain flags.
 *
 * Generation is single-threaded for deterministic output.
 */

#include "sims3000/terrain/WaterBodyGenerator.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <set>

namespace sims3000 {
namespace terrain {

// =============================================================================
// Helper Functions
// =============================================================================

bool WaterBodyGenerator::isWater(TerrainType type) {
    return type == TerrainType::DeepVoid ||
           type == TerrainType::FlowChannel ||
           type == TerrainType::StillBasin;
}

std::uint32_t WaterBodyGenerator::countWaterTiles(const TerrainGrid& grid) {
    std::uint32_t count = 0;
    for (std::size_t i = 0; i < grid.tile_count(); ++i) {
        if (isWater(grid.tiles[i].getTerrainType())) {
            ++count;
        }
    }
    return count;
}

float WaterBodyGenerator::calculateWaterCoverage(const TerrainGrid& grid) {
    std::uint32_t waterTiles = countWaterTiles(grid);
    return static_cast<float>(waterTiles) / static_cast<float>(grid.tile_count());
}

// 8-directional neighbor offsets
static constexpr std::int16_t DX[8] = { 0,  1, 1, 1, 0, -1, -1, -1 };
static constexpr std::int16_t DY[8] = { -1, -1, 0, 1, 1,  1,  0, -1 };

// Map direction index to FlowDirection enum
static constexpr FlowDirection DIR_MAP[8] = {
    FlowDirection::N,
    FlowDirection::NE,
    FlowDirection::E,
    FlowDirection::SE,
    FlowDirection::S,
    FlowDirection::SW,
    FlowDirection::W,
    FlowDirection::NW
};

FlowDirection WaterBodyGenerator::getDownhillDirection(
    const TerrainGrid& grid,
    std::uint16_t x,
    std::uint16_t y)
{
    std::uint8_t currentElev = grid.at(x, y).getElevation();
    std::uint8_t lowestElev = currentElev;
    int lowestDir = -1;

    for (int d = 0; d < 8; ++d) {
        std::int32_t nx = static_cast<std::int32_t>(x) + DX[d];
        std::int32_t ny = static_cast<std::int32_t>(y) + DY[d];

        if (!grid.in_bounds(nx, ny)) {
            // Map edge - always valid destination for rivers
            // Treat edge as lowest possible elevation
            if (lowestDir == -1) {
                lowestDir = d;
                lowestElev = 0;
            }
            continue;
        }

        std::uint8_t neighborElev = grid.at(nx, ny).getElevation();
        if (neighborElev < lowestElev) {
            lowestElev = neighborElev;
            lowestDir = d;
        }
    }

    if (lowestDir == -1 || lowestElev >= currentElev) {
        return FlowDirection::None;
    }

    return DIR_MAP[lowestDir];
}

// =============================================================================
// Ocean (DeepVoid) Generation
// =============================================================================

std::uint32_t WaterBodyGenerator::placeOcean(
    TerrainGrid& grid,
    const WaterBodyConfig& config)
{
    std::uint32_t count = 0;

    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            // Check if within ocean border
            bool nearEdge =
                x < config.oceanBorderWidth ||
                x >= grid.width - config.oceanBorderWidth ||
                y < config.oceanBorderWidth ||
                y >= grid.height - config.oceanBorderWidth;

            if (!nearEdge) {
                continue;
            }

            TerrainComponent& tile = grid.at(x, y);

            // Only convert tiles at or below sea level
            if (tile.getElevation() <= config.seaLevel) {
                // Don't overwrite existing water
                if (!isWater(tile.getTerrainType())) {
                    tile.setTerrainType(TerrainType::DeepVoid);
                    ++count;
                }
            }
        }
    }

    return count;
}

// =============================================================================
// River (FlowChannel) Generation
// =============================================================================

std::uint32_t WaterBodyGenerator::carveRiver(
    TerrainGrid& grid,
    WaterData& waterData,
    std::uint16_t startX,
    std::uint16_t startY,
    Xoshiro256& rng,
    const WaterBodyConfig& config,
    int depth)
{
    std::uint32_t tilesCarved = 0;
    std::uint16_t x = startX;
    std::uint16_t y = startY;

    // Track visited tiles to avoid loops
    std::set<std::uint32_t> visited;

    while (true) {
        // Check bounds
        if (!grid.in_bounds(x, y)) {
            break;  // Reached map edge - success
        }

        std::uint32_t key = static_cast<std::uint32_t>(y) * grid.width + x;
        if (visited.count(key) > 0) {
            break;  // Loop detected
        }
        visited.insert(key);

        TerrainComponent& tile = grid.at(x, y);

        // Stop if we hit existing water
        if (isWater(tile.getTerrainType())) {
            break;
        }

        // Get direction to lowest neighbor
        FlowDirection flowDir = getDownhillDirection(grid, x, y);

        // If no downhill direction and not at edge, we're stuck in a depression
        if (flowDir == FlowDirection::None) {
            break;
        }

        // Convert tile to river
        tile.setTerrainType(TerrainType::FlowChannel);
        waterData.set_flow_direction(x, y, flowDir);
        ++tilesCarved;

        // Widen river if configured
        if (config.riverWidth > 1) {
            // Add tiles perpendicular to flow direction
            for (int w = 1; w < config.riverWidth; ++w) {
                // Perpendicular directions
                std::int8_t perpDX = getFlowDirectionDY(flowDir);  // Rotated 90 degrees
                std::int8_t perpDY = -getFlowDirectionDX(flowDir);

                for (int side = -1; side <= 1; side += 2) {
                    std::int32_t wx = static_cast<std::int32_t>(x) + perpDX * side * w;
                    std::int32_t wy = static_cast<std::int32_t>(y) + perpDY * side * w;

                    if (grid.in_bounds(wx, wy)) {
                        TerrainComponent& wideTile = grid.at(wx, wy);
                        if (!isWater(wideTile.getTerrainType())) {
                            wideTile.setTerrainType(TerrainType::FlowChannel);
                            waterData.set_flow_direction(
                                static_cast<std::uint16_t>(wx),
                                static_cast<std::uint16_t>(wy),
                                flowDir);
                            ++tilesCarved;
                        }
                    }
                }
            }
        }

        // Check for tributary spawn (only for main rivers, not tributaries)
        if (depth == 0 && tilesCarved > 5) {
            float roll = rng.nextFloat();
            if (roll < config.tributaryProbability) {
                // Try to spawn a tributary perpendicular to current direction
                std::int8_t tribDX = getFlowDirectionDY(flowDir);
                std::int8_t tribDY = -getFlowDirectionDX(flowDir);

                // Pick a side randomly
                if (rng.nextUint32(2) == 0) {
                    tribDX = -tribDX;
                    tribDY = -tribDY;
                }

                // Find a starting point for tributary
                std::int32_t tribX = static_cast<std::int32_t>(x) + tribDX * 3;
                std::int32_t tribY = static_cast<std::int32_t>(y) + tribDY * 3;

                if (grid.in_bounds(tribX, tribY)) {
                    // Walk uphill to find higher starting point
                    for (int i = 0; i < 10; ++i) {
                        std::int32_t nextX = tribX + tribDX;
                        std::int32_t nextY = tribY + tribDY;

                        if (!grid.in_bounds(nextX, nextY)) break;

                        if (grid.at(nextX, nextY).getElevation() >
                            grid.at(tribX, tribY).getElevation()) {
                            tribX = nextX;
                            tribY = nextY;
                        } else {
                            break;
                        }
                    }

                    // Carve tributary (depth + 1 prevents recursive tributaries)
                    carveRiver(grid, waterData,
                        static_cast<std::uint16_t>(tribX),
                        static_cast<std::uint16_t>(tribY),
                        rng, config, depth + 1);
                }
            }
        }

        // Move to next tile
        std::int32_t nextX = static_cast<std::int32_t>(x) + getFlowDirectionDX(flowDir);
        std::int32_t nextY = static_cast<std::int32_t>(y) + getFlowDirectionDY(flowDir);

        if (!grid.in_bounds(nextX, nextY)) {
            break;  // Reached map edge
        }

        x = static_cast<std::uint16_t>(nextX);
        y = static_cast<std::uint16_t>(nextY);
    }

    return tilesCarved;
}

std::uint8_t WaterBodyGenerator::placeRivers(
    TerrainGrid& grid,
    WaterData& waterData,
    Xoshiro256& rng,
    const WaterBodyConfig& config)
{
    // Find candidate source points (high elevation, not near edge, not water)
    std::vector<std::pair<std::uint16_t, std::uint16_t>> sources;

    std::uint16_t margin = static_cast<std::uint16_t>(config.oceanBorderWidth + 5);

    for (std::uint16_t y = margin; y < grid.height - margin; ++y) {
        for (std::uint16_t x = margin; x < grid.width - margin; ++x) {
            const TerrainComponent& tile = grid.at(x, y);

            if (tile.getElevation() >= config.riverSourceMinElevation &&
                !isWater(tile.getTerrainType())) {
                sources.push_back({x, y});
            }
        }
    }

    if (sources.empty()) {
        // No high points found - force at least one river from highest point
        std::uint16_t highX = grid.width / 2;
        std::uint16_t highY = grid.height / 2;
        std::uint8_t highElev = 0;

        for (std::uint16_t y = margin; y < grid.height - margin; ++y) {
            for (std::uint16_t x = margin; x < grid.width - margin; ++x) {
                const TerrainComponent& tile = grid.at(x, y);
                if (!isWater(tile.getTerrainType()) && tile.getElevation() > highElev) {
                    highElev = tile.getElevation();
                    highX = x;
                    highY = y;
                }
            }
        }

        if (highElev > config.seaLevel) {
            sources.push_back({highX, highY});
        }
    }

    if (sources.empty()) {
        return 0;  // No valid source points
    }

    // Shuffle sources for randomness
    for (std::size_t i = sources.size() - 1; i > 0; --i) {
        std::size_t j = rng.nextUint32(static_cast<std::uint32_t>(i + 1));
        std::swap(sources[i], sources[j]);
    }

    // Generate rivers
    std::uint8_t riversCreated = 0;
    std::uint8_t targetRivers = config.minRiverCount +
        static_cast<std::uint8_t>(rng.nextUint32(
            config.maxRiverCount - config.minRiverCount + 1));

    for (const auto& source : sources) {
        if (riversCreated >= targetRivers) {
            break;
        }

        // Check if water coverage limit reached
        if (calculateWaterCoverage(grid) >= config.maxWaterCoverage) {
            break;
        }

        // Don't start if source is now water (from previous river)
        if (isWater(grid.at(source.first, source.second).getTerrainType())) {
            continue;
        }

        std::uint32_t tilesCarved = carveRiver(
            grid, waterData, source.first, source.second, rng, config, 0);

        if (tilesCarved > 5) {  // Minimum viable river length
            ++riversCreated;
        }
    }

    // Ensure at least minRiverCount if possible
    while (riversCreated < config.minRiverCount && !sources.empty()) {
        // Try additional sources
        for (const auto& source : sources) {
            if (riversCreated >= config.minRiverCount) {
                break;
            }

            if (isWater(grid.at(source.first, source.second).getTerrainType())) {
                continue;
            }

            std::uint32_t tilesCarved = carveRiver(
                grid, waterData, source.first, source.second, rng, config, 0);

            if (tilesCarved > 0) {
                ++riversCreated;
            }
        }
        break;  // Only try once more
    }

    return riversCreated;
}

// =============================================================================
// Lake (StillBasin) Generation
// =============================================================================

std::vector<std::pair<std::uint16_t, std::uint16_t>> WaterBodyGenerator::findDepressions(
    const TerrainGrid& grid,
    const WaterBodyConfig& config)
{
    std::vector<std::pair<std::uint16_t, std::uint16_t>> depressions;

    // Margin from edges
    std::uint16_t margin = static_cast<std::uint16_t>(config.oceanBorderWidth + 3);

    for (std::uint16_t y = margin; y < grid.height - margin; ++y) {
        for (std::uint16_t x = margin; x < grid.width - margin; ++x) {
            const TerrainComponent& tile = grid.at(x, y);

            // Skip water tiles
            if (isWater(tile.getTerrainType())) {
                continue;
            }

            std::uint8_t centerElev = tile.getElevation();

            // Check if this is a local minimum
            bool isMinimum = true;
            std::uint8_t minNeighborElev = 255;

            for (int d = 0; d < 8; ++d) {
                std::int32_t nx = static_cast<std::int32_t>(x) + DX[d];
                std::int32_t ny = static_cast<std::int32_t>(y) + DY[d];

                if (!grid.in_bounds(nx, ny)) {
                    isMinimum = false;
                    break;
                }

                std::uint8_t neighborElev = grid.at(nx, ny).getElevation();
                if (neighborElev < centerElev) {
                    isMinimum = false;
                    break;
                }
                if (neighborElev < minNeighborElev) {
                    minNeighborElev = neighborElev;
                }
            }

            if (isMinimum) {
                // Check depression depth
                std::uint8_t rimElev = minNeighborElev;
                if (rimElev >= centerElev + config.minDepressionDepth) {
                    depressions.push_back({x, y});
                }
            }
        }
    }

    return depressions;
}

std::uint32_t WaterBodyGenerator::fillLake(
    TerrainGrid& grid,
    std::uint16_t centerX,
    std::uint16_t centerY,
    const WaterBodyConfig& config)
{
    std::uint32_t tilesFilled = 0;

    // Get center elevation and determine fill level
    std::uint8_t centerElev = grid.at(centerX, centerY).getElevation();

    // Find rim elevation (lowest surrounding peak)
    std::uint8_t rimElev = 255;
    for (int radius = 1; radius <= config.maxLakeRadius; ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (std::abs(dx) != radius && std::abs(dy) != radius) {
                    continue;  // Only check perimeter
                }

                std::int32_t nx = static_cast<std::int32_t>(centerX) + dx;
                std::int32_t ny = static_cast<std::int32_t>(centerY) + dy;

                if (grid.in_bounds(nx, ny)) {
                    std::uint8_t elev = grid.at(nx, ny).getElevation();
                    if (elev < rimElev) {
                        rimElev = elev;
                    }
                }
            }
        }
    }

    // Determine fill level
    std::uint8_t fillLevel = config.fillToRim ? rimElev : config.seaLevel;
    fillLevel = std::max(fillLevel, centerElev);

    // Flood fill from center
    std::deque<std::pair<std::uint16_t, std::uint16_t>> queue;
    std::set<std::uint32_t> visited;

    queue.push_back({centerX, centerY});

    while (!queue.empty()) {
        auto [x, y] = queue.front();
        queue.pop_front();

        std::uint32_t key = static_cast<std::uint32_t>(y) * grid.width + x;
        if (visited.count(key) > 0) {
            continue;
        }
        visited.insert(key);

        // Check distance from center
        int dx = static_cast<int>(x) - static_cast<int>(centerX);
        int dy = static_cast<int>(y) - static_cast<int>(centerY);
        if (dx * dx + dy * dy > config.maxLakeRadius * config.maxLakeRadius) {
            continue;
        }

        TerrainComponent& tile = grid.at(x, y);

        // Skip if already water
        if (isWater(tile.getTerrainType())) {
            continue;
        }

        // Only fill tiles at or below fill level
        if (tile.getElevation() > fillLevel) {
            continue;
        }

        // Convert to lake
        tile.setTerrainType(TerrainType::StillBasin);
        ++tilesFilled;

        // Add 4-connected neighbors to queue
        if (x > 0) queue.push_back({static_cast<std::uint16_t>(x - 1), y});
        if (x < grid.width - 1) queue.push_back({static_cast<std::uint16_t>(x + 1), y});
        if (y > 0) queue.push_back({x, static_cast<std::uint16_t>(y - 1)});
        if (y < grid.height - 1) queue.push_back({x, static_cast<std::uint16_t>(y + 1)});
    }

    return tilesFilled;
}

std::uint8_t WaterBodyGenerator::placeLakes(
    TerrainGrid& grid,
    Xoshiro256& rng,
    const WaterBodyConfig& config)
{
    if (config.maxLakeCount == 0) {
        return 0;
    }

    // Find depression candidates
    auto depressions = findDepressions(grid, config);

    if (depressions.empty()) {
        return 0;
    }

    // Shuffle for randomness
    for (std::size_t i = depressions.size() - 1; i > 0; --i) {
        std::size_t j = rng.nextUint32(static_cast<std::uint32_t>(i + 1));
        std::swap(depressions[i], depressions[j]);
    }

    std::uint8_t lakesCreated = 0;

    for (const auto& depression : depressions) {
        if (lakesCreated >= config.maxLakeCount) {
            break;
        }

        // Check if water coverage limit reached
        if (calculateWaterCoverage(grid) >= config.maxWaterCoverage) {
            break;
        }

        // Skip if depression is now water (from previous lake)
        if (isWater(grid.at(depression.first, depression.second).getTerrainType())) {
            continue;
        }

        std::uint32_t tilesFilled = fillLake(
            grid, depression.first, depression.second, config);

        if (tilesFilled > 0) {
            ++lakesCreated;
        }
    }

    return lakesCreated;
}

// =============================================================================
// Water Body ID Assignment
// =============================================================================

std::uint16_t WaterBodyGenerator::assignWaterBodyIDs(
    const TerrainGrid& grid,
    WaterData& waterData)
{
    // Clear existing IDs
    waterData.water_body_ids.clear();

    std::uint16_t nextBodyID = 1;  // 0 = NO_WATER_BODY
    std::vector<bool> visited(grid.tile_count(), false);

    // Flood-fill each contiguous water region
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            std::size_t idx = grid.index_of(x, y);

            if (visited[idx]) {
                continue;
            }

            if (!isWater(grid.tiles[idx].getTerrainType())) {
                continue;
            }

            // Found an unvisited water tile - flood fill
            WaterBodyID bodyID = nextBodyID++;

            std::deque<std::pair<std::uint16_t, std::uint16_t>> queue;
            queue.push_back({x, y});

            while (!queue.empty()) {
                auto [fx, fy] = queue.front();
                queue.pop_front();

                std::size_t fidx = grid.index_of(fx, fy);
                if (visited[fidx]) {
                    continue;
                }

                if (!isWater(grid.tiles[fidx].getTerrainType())) {
                    continue;
                }

                visited[fidx] = true;
                waterData.set_water_body_id(fx, fy, bodyID);

                // Add 4-connected neighbors
                if (fx > 0) queue.push_back({static_cast<std::uint16_t>(fx - 1), fy});
                if (fx < grid.width - 1) queue.push_back({static_cast<std::uint16_t>(fx + 1), fy});
                if (fy > 0) queue.push_back({fx, static_cast<std::uint16_t>(fy - 1)});
                if (fy < grid.height - 1) queue.push_back({fx, static_cast<std::uint16_t>(fy + 1)});
            }
        }
    }

    return static_cast<std::uint16_t>(nextBodyID - 1);  // Return count of bodies
}

// =============================================================================
// Flag Updates
// =============================================================================

std::uint32_t WaterBodyGenerator::setUnderwaterFlags(TerrainGrid& grid) {
    std::uint32_t count = 0;

    for (std::size_t i = 0; i < grid.tile_count(); ++i) {
        if (isWater(grid.tiles[i].getTerrainType())) {
            grid.tiles[i].setUnderwater(true);
            ++count;
        } else {
            grid.tiles[i].setUnderwater(false);
        }
    }

    return count;
}

std::uint32_t WaterBodyGenerator::setCoastalFlags(TerrainGrid& grid) {
    std::uint32_t count = 0;

    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            TerrainComponent& tile = grid.at(x, y);

            // Skip water tiles
            if (isWater(tile.getTerrainType())) {
                tile.setCoastal(false);
                continue;
            }

            // Check 8 neighbors for water
            bool adjacentToWater = false;
            for (int d = 0; d < 8; ++d) {
                std::int32_t nx = static_cast<std::int32_t>(x) + DX[d];
                std::int32_t ny = static_cast<std::int32_t>(y) + DY[d];

                if (!grid.in_bounds(nx, ny)) {
                    continue;
                }

                if (isWater(grid.at(nx, ny).getTerrainType())) {
                    adjacentToWater = true;
                    break;
                }
            }

            tile.setCoastal(adjacentToWater);
            if (adjacentToWater) {
                ++count;
            }
        }
    }

    return count;
}

// =============================================================================
// Main Generation Function
// =============================================================================

WaterBodyResult WaterBodyGenerator::generate(
    TerrainGrid& grid,
    WaterData& waterData,
    WaterDistanceField& distanceField,
    std::uint64_t seed,
    const WaterBodyConfig& config)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    WaterBodyResult result{};
    result.totalTiles = static_cast<std::uint32_t>(grid.tile_count());

    // Initialize data structures if needed
    if (waterData.empty()) {
        MapSize mapSize = static_cast<MapSize>(grid.width);
        waterData.initialize(mapSize);
    }

    // 1. Place ocean along map edges
    result.oceanTileCount = placeOcean(grid, config);

    // 2. Initialize RNG for rivers and lakes
    Xoshiro256 rng(seed);

    // 3. Generate rivers
    result.riverCount = placeRivers(grid, waterData, rng, config);

    // Count river tiles (separate from ocean)
    for (std::size_t i = 0; i < grid.tile_count(); ++i) {
        if (grid.tiles[i].getTerrainType() == TerrainType::FlowChannel) {
            ++result.riverTileCount;
        }
    }

    // 4. Place lakes
    result.lakeCount = placeLakes(grid, rng, config);

    // Count lake tiles
    for (std::size_t i = 0; i < grid.tile_count(); ++i) {
        if (grid.tiles[i].getTerrainType() == TerrainType::StillBasin) {
            ++result.lakeTileCount;
        }
    }

    // 5. Assign water body IDs
    result.waterBodyCount = assignWaterBodyIDs(grid, waterData);

    // 6. Set underwater flags
    setUnderwaterFlags(grid);

    // 7. Set coastal flags
    result.coastalTileCount = setCoastalFlags(grid);

    // 8. Compute water distance field
    distanceField.compute(grid);

    // Calculate totals
    result.totalWaterTiles = result.oceanTileCount + result.riverTileCount + result.lakeTileCount;
    result.waterCoverage = static_cast<float>(result.totalWaterTiles) /
                           static_cast<float>(result.totalTiles);

    // Calculate generation time
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime);
    result.generationTimeMs = static_cast<float>(duration.count()) / 1000.0f;

    return result;
}

} // namespace terrain
} // namespace sims3000
