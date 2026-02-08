/**
 * @file NetworkGraph.h
 * @brief NetworkGraph node/edge structures for transport network (Epic 7, Ticket E7-008)
 *
 * Provides a grid-based graph representation for the road/transport network.
 * Nodes represent road tiles at grid positions, edges represent bidirectional
 * connections between adjacent road tiles. Connected component IDs are assigned
 * via BFS to enable O(1) connectivity queries.
 *
 * @see /docs/epics/epic-7/tickets.md (ticket E7-008)
 */

#pragma once
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace sims3000 {
namespace transport {

class PathwayGrid;  // forward declare (E7-009)

/**
 * @struct GridPosition
 * @brief 2D integer grid coordinate for transport network nodes.
 */
struct GridPosition {
    int32_t x = 0;
    int32_t y = 0;

    bool operator==(const GridPosition& other) const { return x == other.x && y == other.y; }
    bool operator!=(const GridPosition& other) const { return !(*this == other); }
};

/**
 * @struct GridPositionHash
 * @brief Hash functor for GridPosition, packs x/y into a single 64-bit value.
 */
struct GridPositionHash {
    size_t operator()(const GridPosition& pos) const {
        return std::hash<int64_t>()(static_cast<int64_t>(pos.x) << 32 | static_cast<uint32_t>(pos.y));
    }
};

/**
 * @struct NetworkNode
 * @brief A node in the transport network graph.
 *
 * Stores the grid position, indices of neighboring nodes, and the
 * connected component network_id (assigned by assign_network_ids).
 */
struct NetworkNode {
    GridPosition position;
    std::vector<uint16_t> neighbor_indices;
    uint16_t network_id = 0;
};

/**
 * @class NetworkGraph
 * @brief Graph representation of the transport (road) network.
 *
 * Manages nodes and edges for the road network. Supports:
 * - Adding/removing nodes and edges
 * - Connected component assignment via BFS
 * - O(1) connectivity queries via network_id comparison
 */
class NetworkGraph {
public:
    NetworkGraph() = default;

    // =========================================================================
    // Graph management
    // =========================================================================

    /**
     * @brief Clear all nodes, edges, and position mappings.
     */
    void clear();

    /**
     * @brief Add a node at the given grid position.
     * @param pos The grid position for the new node.
     * @return Index of the newly added node.
     */
    uint16_t add_node(const GridPosition& pos);

    /**
     * @brief Add a bidirectional edge between two nodes.
     * @param node_a Index of the first node.
     * @param node_b Index of the second node.
     */
    void add_edge(uint16_t node_a, uint16_t node_b);

    // =========================================================================
    // Queries
    // =========================================================================

    /**
     * @brief Check if two grid positions are in the same connected component.
     * @param a First grid position.
     * @param b Second grid position.
     * @return true if both positions exist and share the same non-zero network_id.
     */
    bool is_connected(const GridPosition& a, const GridPosition& b) const;

    /**
     * @brief Get the network_id for a grid position.
     * @param pos The grid position to query.
     * @return The network_id (0 if position not found or not yet assigned).
     */
    uint16_t get_network_id(const GridPosition& pos) const;

    /**
     * @brief Get the node index for a grid position.
     * @param pos The grid position to query.
     * @return The node index, or UINT16_MAX if not found.
     */
    uint16_t get_node_index(const GridPosition& pos) const;

    /**
     * @brief Get the total number of nodes in the graph.
     * @return Number of nodes.
     */
    size_t node_count() const;

    /**
     * @brief Get a const reference to a node by index.
     * @param index The node index.
     * @return Const reference to the NetworkNode.
     */
    const NetworkNode& get_node(uint16_t index) const;

    // =========================================================================
    // Network ID assignment
    // =========================================================================

    /**
     * @brief Assign connected component IDs to all nodes via BFS.
     *
     * Resets all network_ids to 0, then performs BFS from each unvisited
     * node, assigning incrementing network_ids to each connected component.
     */
    void assign_network_ids();

    /**
     * @brief Get all grid positions belonging to a specific network.
     * @param network_id The network ID to query.
     * @return Vector of GridPositions in that network (empty if none found).
     */
    std::vector<GridPosition> get_network_positions(uint16_t network_id) const;

    /**
     * @brief Get the total number of distinct connected component networks.
     * @return Number of networks (each with a unique non-zero network_id).
     */
    uint16_t get_network_count() const;

    // =========================================================================
    // Grid rebuild (Ticket E7-009)
    // =========================================================================

    /**
     * @brief Rebuild the entire graph from a PathwayGrid.
     *
     * Algorithm:
     * 1. Clear existing graph
     * 2. Scan PathwayGrid for all pathway tiles
     * 3. Create a node for each pathway tile
     * 4. Connect adjacent pathway tiles (N/S/E/W) - cross-ownership per CCR-002
     * 5. Call assign_network_ids() to label connected components
     *
     * Cross-ownership: No owner check when connecting adjacent tiles.
     * Two pathway tiles owned by different players are connected if adjacent.
     *
     * @param grid The PathwayGrid to rebuild from.
     */
    void rebuild_from_grid(const PathwayGrid& grid);

private:
    std::vector<NetworkNode> nodes_;
    std::unordered_map<GridPosition, uint16_t, GridPositionHash> position_to_node_;
    uint16_t next_network_id_ = 1;
};

} // namespace transport
} // namespace sims3000
