/**
 * @file NeighborGeneration.cpp
 * @brief NPC neighbor generation implementation (Epic 8, Ticket E8-015)
 *
 * Implements deterministic neighbor generation using a simple LCG
 * (Linear Congruential Generator) for cross-platform reproducibility.
 * No dependency on std::random or platform-specific RNG.
 */

#include <sims3000/port/NeighborGeneration.h>
#include <algorithm>
#include <cstring>

namespace sims3000 {
namespace port {

// =============================================================================
// Name Pool
// =============================================================================

static const std::string s_name_pool[NEIGHBOR_NAME_POOL_SIZE] = {
    "Settlement Alpha",
    "Nexus Prime",
    "Forge Delta",
    "Haven Epsilon",
    "Citadel Omega",
    "Outpost Sigma",
    "Colony Zeta",
    "Bastion Theta"
};

const std::string* get_neighbor_name_pool() {
    return s_name_pool;
}

// =============================================================================
// Deterministic LCG (Linear Congruential Generator)
// =============================================================================

/**
 * @brief Simple LCG for deterministic pseudo-random generation.
 *
 * Uses the Numerical Recipes LCG constants for reasonable distribution.
 * Returns values in [0, 2^32 - 1]. State is updated in place.
 */
static uint32_t lcg_next(uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    return state;
}

/**
 * @brief Generate a float in [min_val, max_val] from LCG state.
 */
static float lcg_float_range(uint32_t& state, float min_val, float max_val) {
    uint32_t raw = lcg_next(state);
    // Convert to [0, 1] then scale to [min_val, max_val]
    float t = static_cast<float>(raw) / static_cast<float>(0xFFFFFFFFu);
    return min_val + t * (max_val - min_val);
}

// =============================================================================
// Neighbor Generation
// =============================================================================

std::vector<NeighborData> generate_neighbors(
    const std::vector<ExternalConnectionComponent>& edge_connections,
    uint32_t seed)
{
    // Determine which edges have at least one connection
    bool edge_has_connection[MAP_EDGE_COUNT] = { false, false, false, false };
    for (const auto& conn : edge_connections) {
        uint8_t edge_idx = static_cast<uint8_t>(conn.edge_side);
        if (edge_idx < MAP_EDGE_COUNT) {
            edge_has_connection[edge_idx] = true;
        }
    }

    // Collect edges that have connections (ordered N, E, S, W)
    std::vector<MapEdge> active_edges;
    active_edges.reserve(MAP_EDGE_COUNT);
    for (uint8_t i = 0; i < MAP_EDGE_COUNT; ++i) {
        if (edge_has_connection[i]) {
            active_edges.push_back(static_cast<MapEdge>(i));
        }
    }

    if (active_edges.empty()) {
        return {};
    }

    // Initialize RNG state from seed
    uint32_t rng_state = seed;

    // Build a shuffled copy of name indices for unique name assignment
    uint8_t name_indices[NEIGHBOR_NAME_POOL_SIZE];
    for (uint8_t i = 0; i < NEIGHBOR_NAME_POOL_SIZE; ++i) {
        name_indices[i] = i;
    }

    // Fisher-Yates shuffle of name indices using deterministic RNG
    for (uint8_t i = NEIGHBOR_NAME_POOL_SIZE - 1; i > 0; --i) {
        uint32_t r = lcg_next(rng_state);
        uint8_t j = static_cast<uint8_t>(r % (i + 1));
        uint8_t tmp = name_indices[i];
        name_indices[i] = name_indices[j];
        name_indices[j] = tmp;
    }

    // Generate one neighbor per active edge
    std::vector<NeighborData> result;
    result.reserve(active_edges.size());

    uint8_t neighbor_id = 1;
    uint8_t name_slot = 0;

    for (const MapEdge edge : active_edges) {
        NeighborData neighbor;
        neighbor.neighbor_id = neighbor_id;
        neighbor.edge = edge;
        neighbor.name = s_name_pool[name_indices[name_slot % NEIGHBOR_NAME_POOL_SIZE]];
        neighbor.demand_factor = lcg_float_range(rng_state, NEIGHBOR_FACTOR_MIN, NEIGHBOR_FACTOR_MAX);
        neighbor.supply_factor = lcg_float_range(rng_state, NEIGHBOR_FACTOR_MIN, NEIGHBOR_FACTOR_MAX);
        neighbor.relationship = TradeAgreementType::None;

        result.push_back(std::move(neighbor));

        ++neighbor_id;
        ++name_slot;
    }

    return result;
}

} // namespace port
} // namespace sims3000
