/**
 * @file NeighborGeneration.h
 * @brief NPC neighbor generation for Epic 8 (Ticket E8-015)
 *
 * Generates AI-controlled neighbor settlements at game start based on
 * map edge connections. Each neighbor is assigned to a map edge that has
 * at least one external connection, receives a unique name from a themed
 * pool, and has randomized economic factors for trade simulation.
 *
 * Neighbor data is trivially serializable for save/load support.
 */

#pragma once

#include <sims3000/port/ExternalConnectionComponent.h>
#include <sims3000/port/PortTypes.h>
#include <cstdint>
#include <string>
#include <vector>

namespace sims3000 {
namespace port {

/// Maximum number of NPC neighbors (one per map edge)
constexpr uint8_t MAX_NEIGHBORS = 4;

/// Number of names in the neighbor name pool
constexpr uint8_t NEIGHBOR_NAME_POOL_SIZE = 8;

/// Minimum demand/supply factor value
constexpr float NEIGHBOR_FACTOR_MIN = 0.5f;

/// Maximum demand/supply factor value
constexpr float NEIGHBOR_FACTOR_MAX = 1.5f;

/**
 * @struct NeighborData
 * @brief Data for a single NPC neighbor settlement.
 *
 * Each neighbor is associated with a map edge and has unique economic
 * characteristics that influence trade flows.
 */
struct NeighborData {
    uint8_t neighbor_id;                    ///< Unique neighbor ID (1-4)
    MapEdge edge;                           ///< Which map edge this neighbor is on
    std::string name;                       ///< Settlement name (e.g., "Nexus Prime")
    float demand_factor;                    ///< Demand multiplier (0.5-1.5)
    float supply_factor;                    ///< Supply multiplier (0.5-1.5)
    TradeAgreementType relationship;        ///< Initial trade relationship
};

/**
 * @brief Get the themed name pool for NPC neighbors.
 *
 * Returns a fixed pool of 8 names. The generation function selects
 * from this pool deterministically based on the RNG seed.
 *
 * @return Pointer to the static name pool array of size NEIGHBOR_NAME_POOL_SIZE.
 */
const std::string* get_neighbor_name_pool();

/**
 * @brief Generate NPC neighbors based on map edge connections.
 *
 * Scans the provided edge connections to determine which map edges have
 * active external connections. For each edge with at least one connection,
 * generates one neighbor settlement with:
 * - A unique name from the themed name pool (deterministic, seeded)
 * - Randomized demand_factor in [0.5, 1.5] (deterministic, seeded)
 * - Randomized supply_factor in [0.5, 1.5] (deterministic, seeded)
 * - Initial relationship set to TradeAgreementType::None
 *
 * Produces 0-4 neighbors depending on how many edges have connections.
 * The same seed always produces the same output for save/load determinism.
 *
 * @param edge_connections Vector of external connections detected on map edges.
 * @param seed RNG seed for deterministic generation.
 * @return Vector of generated NeighborData (0 to MAX_NEIGHBORS entries).
 */
std::vector<NeighborData> generate_neighbors(
    const std::vector<ExternalConnectionComponent>& edge_connections,
    uint32_t seed);

} // namespace port
} // namespace sims3000
