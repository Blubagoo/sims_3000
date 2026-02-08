/**
 * @file NetworkGraph.cpp
 * @brief Implementation of NetworkGraph for transport network (Epic 7, Ticket E7-008)
 *
 * @see NetworkGraph.h
 */

#include <sims3000/transport/NetworkGraph.h>
#include <sims3000/transport/PathwayGrid.h>
#include <queue>
#include <algorithm>

namespace sims3000 {
namespace transport {

void NetworkGraph::clear() {
    nodes_.clear();
    position_to_node_.clear();
    next_network_id_ = 1;
}

uint16_t NetworkGraph::add_node(const GridPosition& pos) {
    // Check if node already exists at this position
    auto it = position_to_node_.find(pos);
    if (it != position_to_node_.end()) {
        return it->second;
    }

    uint16_t index = static_cast<uint16_t>(nodes_.size());
    NetworkNode node;
    node.position = pos;
    node.network_id = 0;
    nodes_.push_back(node);
    position_to_node_[pos] = index;
    return index;
}

void NetworkGraph::add_edge(uint16_t node_a, uint16_t node_b) {
    if (node_a >= nodes_.size() || node_b >= nodes_.size()) {
        return;
    }
    if (node_a == node_b) {
        return;
    }

    // Check if edge already exists (avoid duplicates)
    auto& neighbors_a = nodes_[node_a].neighbor_indices;
    if (std::find(neighbors_a.begin(), neighbors_a.end(), node_b) == neighbors_a.end()) {
        neighbors_a.push_back(node_b);
    }

    auto& neighbors_b = nodes_[node_b].neighbor_indices;
    if (std::find(neighbors_b.begin(), neighbors_b.end(), node_a) == neighbors_b.end()) {
        neighbors_b.push_back(node_a);
    }
}

bool NetworkGraph::is_connected(const GridPosition& a, const GridPosition& b) const {
    auto it_a = position_to_node_.find(a);
    auto it_b = position_to_node_.find(b);

    if (it_a == position_to_node_.end() || it_b == position_to_node_.end()) {
        return false;
    }

    uint16_t id_a = nodes_[it_a->second].network_id;
    uint16_t id_b = nodes_[it_b->second].network_id;

    // Both must have non-zero network_id and they must match
    return id_a != 0 && id_a == id_b;
}

uint16_t NetworkGraph::get_network_id(const GridPosition& pos) const {
    auto it = position_to_node_.find(pos);
    if (it == position_to_node_.end()) {
        return 0;
    }
    return nodes_[it->second].network_id;
}

uint16_t NetworkGraph::get_node_index(const GridPosition& pos) const {
    auto it = position_to_node_.find(pos);
    if (it == position_to_node_.end()) {
        return UINT16_MAX;
    }
    return it->second;
}

size_t NetworkGraph::node_count() const {
    return nodes_.size();
}

const NetworkNode& NetworkGraph::get_node(uint16_t index) const {
    return nodes_[index];
}

void NetworkGraph::assign_network_ids() {
    // Reset all network IDs
    for (auto& node : nodes_) {
        node.network_id = 0;
    }
    next_network_id_ = 1;

    // BFS over all nodes, assigning connected component IDs
    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].network_id != 0) {
            continue; // Already visited
        }

        uint16_t current_id = next_network_id_++;

        // BFS from this node
        std::queue<uint16_t> frontier;
        frontier.push(static_cast<uint16_t>(i));
        nodes_[i].network_id = current_id;

        while (!frontier.empty()) {
            uint16_t current = frontier.front();
            frontier.pop();

            for (uint16_t neighbor : nodes_[current].neighbor_indices) {
                if (nodes_[neighbor].network_id == 0) {
                    nodes_[neighbor].network_id = current_id;
                    frontier.push(neighbor);
                }
            }
        }
    }
}

std::vector<GridPosition> NetworkGraph::get_network_positions(uint16_t network_id) const {
    std::vector<GridPosition> positions;
    for (const auto& node : nodes_) {
        if (node.network_id == network_id) {
            positions.push_back(node.position);
        }
    }
    return positions;
}

uint16_t NetworkGraph::get_network_count() const {
    // next_network_id_ starts at 1 and increments for each component.
    // After assign_network_ids(), it points one past the last assigned ID.
    // If no nodes exist, next_network_id_ is 1, so count is 0.
    return (next_network_id_ > 1) ? static_cast<uint16_t>(next_network_id_ - 1) : 0;
}

void NetworkGraph::rebuild_from_grid(const PathwayGrid& grid) {
    // 1. Clear existing graph
    clear();

    // 2. Scan PathwayGrid for all pathway tiles and create nodes
    const int32_t w = static_cast<int32_t>(grid.width());
    const int32_t h = static_cast<int32_t>(grid.height());

    for (int32_t y = 0; y < h; ++y) {
        for (int32_t x = 0; x < w; ++x) {
            if (grid.has_pathway(x, y)) {
                add_node(GridPosition{x, y});
            }
        }
    }

    // 3. Connect adjacent pathway tiles (N/S/E/W)
    //    Cross-ownership per CCR-002: no owner check when connecting
    static const int32_t dx[] = { 1, 0 };
    static const int32_t dy[] = { 0, 1 };

    for (int32_t y = 0; y < h; ++y) {
        for (int32_t x = 0; x < w; ++x) {
            if (!grid.has_pathway(x, y)) {
                continue;
            }

            uint16_t node_a = get_node_index(GridPosition{x, y});

            // Only check East and South to avoid duplicate edges
            for (int d = 0; d < 2; ++d) {
                int32_t nx = x + dx[d];
                int32_t ny = y + dy[d];

                if (nx < 0 || ny < 0
                    || static_cast<uint32_t>(nx) >= grid.width()
                    || static_cast<uint32_t>(ny) >= grid.height()) {
                    continue;
                }

                if (grid.has_pathway(nx, ny)) {
                    uint16_t node_b = get_node_index(GridPosition{nx, ny});
                    add_edge(node_a, node_b);
                }
            }
        }
    }

    // 4. Assign connected component network IDs
    assign_network_ids();
}

} // namespace transport
} // namespace sims3000
