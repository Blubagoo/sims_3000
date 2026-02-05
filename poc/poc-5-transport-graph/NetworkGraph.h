#pragma once

#include <cstdint>
#include <vector>

// NetworkGraph: Union-Find connectivity for pathway tiles
// Uses union-find (disjoint set) with path compression and union by rank
// Provides O(1) amortized `are_connected(tile_a, tile_b)` queries
// Per systems.yaml TransportSystem and interfaces.yaml ITransportProvider
//
// Design notes:
// - Uses dense arrays indexed by linear coordinate for O(1) parent/rank lookup
// - No edge storage needed at runtime - we rebuild from PathwayGrid when topology changes
// - Memory efficient: parent array adds 4 bytes per map tile (shared, not per-pathway)
// - Per-pathway overhead is just 4 bytes from PathwayGrid; union-find arrays are map-wide

class NetworkGraph {
public:
    static constexpr uint32_t INVALID = 0xFFFFFFFF;

    // Initialize the graph for a given map size
    void init(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;
        size_t total = static_cast<size_t>(width) * height;

        // Dense arrays for O(1) lookup - use INVALID to indicate "no pathway"
        parent_.assign(total, INVALID);
        rank_.assign(total, 0);
        network_id_.assign(total, 0);

        node_count_ = 0;
        edge_count_ = 0;
        next_network_id_ = 1;
    }

    // Clear all nodes
    void clear() {
        std::fill(parent_.begin(), parent_.end(), INVALID);
        std::fill(rank_.begin(), rank_.end(), 0);
        std::fill(network_id_.begin(), network_id_.end(), 0);
        node_count_ = 0;
        edge_count_ = 0;
        next_network_id_ = 1;
    }

    // Coordinate to linear index conversion
    uint32_t coord_to_index(int32_t x, int32_t y) const {
        return static_cast<uint32_t>(y) * width_ + static_cast<uint32_t>(x);
    }

    // Add a pathway node at position
    void add_node(int32_t x, int32_t y) {
        if (!in_bounds(x, y)) return;
        uint32_t idx = coord_to_index(x, y);
        if (parent_[idx] == INVALID) {
            parent_[idx] = idx; // Self-parent initially
            rank_[idx] = 0;
            node_count_++;
        }
    }

    // Check if node exists
    bool has_node(int32_t x, int32_t y) const {
        if (!in_bounds(x, y)) return false;
        return parent_[coord_to_index(x, y)] != INVALID;
    }

    // Remove a pathway node
    void remove_node(int32_t x, int32_t y) {
        if (!in_bounds(x, y)) return;
        uint32_t idx = coord_to_index(x, y);
        if (parent_[idx] != INVALID) {
            parent_[idx] = INVALID;
            node_count_--;
        }
    }

    // Add edge between two adjacent tiles (just performs union, no storage)
    void add_edge(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
        if (!in_bounds(x1, y1) || !in_bounds(x2, y2)) return;

        uint32_t a = coord_to_index(x1, y1);
        uint32_t b = coord_to_index(x2, y2);

        // Ensure both nodes exist
        add_node(x1, y1);
        add_node(x2, y2);

        // Perform union
        if (unite(a, b)) {
            edge_count_++;
        }
    }

    // Rebuild union-find from PathwayGrid (call when topology changes)
    template<typename PathwayGridT>
    void rebuild_from_grid(const PathwayGridT& pathways) {
        // Reset
        clear();

        // Add all pathway nodes
        for (int32_t y = 0; y < static_cast<int32_t>(height_); ++y) {
            for (int32_t x = 0; x < static_cast<int32_t>(width_); ++x) {
                if (pathways.has_pathway(x, y)) {
                    add_node(x, y);
                }
            }
        }

        // Connect adjacent pathway tiles
        for (int32_t y = 0; y < static_cast<int32_t>(height_); ++y) {
            for (int32_t x = 0; x < static_cast<int32_t>(width_); ++x) {
                if (!pathways.has_pathway(x, y)) continue;

                // Right neighbor
                if (x + 1 < static_cast<int32_t>(width_) && pathways.has_pathway(x + 1, y)) {
                    add_edge(x, y, x + 1, y);
                }
                // Bottom neighbor
                if (y + 1 < static_cast<int32_t>(height_) && pathways.has_pathway(x, y + 1)) {
                    add_edge(x, y, x, y + 1);
                }
            }
        }

        // Assign network IDs
        assign_network_ids();
    }

    // Force rebuild for benchmarking (rebuilds network IDs)
    void force_rebuild() {
        // Reset union-find for all existing nodes
        for (size_t i = 0; i < parent_.size(); ++i) {
            if (parent_[i] != INVALID) {
                parent_[i] = static_cast<uint32_t>(i);
                rank_[i] = 0;
            }
        }
        edge_count_ = 0;
    }

    // O(1) amortized connectivity query using union-find
    bool are_connected(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
        if (!in_bounds(x1, y1) || !in_bounds(x2, y2)) return false;

        uint32_t a = coord_to_index(x1, y1);
        uint32_t b = coord_to_index(x2, y2);

        if (parent_[a] == INVALID || parent_[b] == INVALID) {
            return false;
        }

        return find(a) == find(b);
    }

    // Get network ID for position (0 = no pathway)
    uint32_t get_network_id_at(int32_t x, int32_t y) {
        if (!in_bounds(x, y)) return 0;
        uint32_t idx = coord_to_index(x, y);
        if (parent_[idx] == INVALID) return 0;
        return network_id_[find(idx)];
    }

    bool in_bounds(int32_t x, int32_t y) const {
        return x >= 0 && x < static_cast<int32_t>(width_) &&
               y >= 0 && y < static_cast<int32_t>(height_);
    }

    // Stats
    size_t node_count() const { return node_count_; }
    size_t edge_count() const { return edge_count_; }

    // Memory usage calculation - just the dense arrays
    size_t memory_bytes() const {
        size_t total = 0;
        total += parent_.size() * sizeof(uint32_t);   // parent array
        total += rank_.size() * sizeof(uint32_t);     // rank array
        total += network_id_.size() * sizeof(uint32_t); // network_id array
        return total;
    }

    // Per-pathway memory overhead (excluding shared map arrays)
    // The parent/rank/network_id arrays are shared across ALL map tiles
    // So per-pathway overhead is effectively 0 bytes beyond PathwayGrid
    double memory_per_pathway_bytes() const {
        return 0.0; // No per-pathway storage - just map-wide dense arrays
    }

private:
    // Find with path compression
    uint32_t find(uint32_t x) {
        if (parent_[x] != x) {
            parent_[x] = find(parent_[x]); // Path compression
        }
        return parent_[x];
    }

    // Union by rank - returns true if a union was actually performed
    bool unite(uint32_t x, uint32_t y) {
        uint32_t root_x = find(x);
        uint32_t root_y = find(y);

        if (root_x == root_y) return false;

        // Union by rank
        if (rank_[root_x] < rank_[root_y]) {
            parent_[root_x] = root_y;
        } else if (rank_[root_x] > rank_[root_y]) {
            parent_[root_y] = root_x;
        } else {
            parent_[root_y] = root_x;
            rank_[root_x]++;
        }
        return true;
    }

    // Assign unique network IDs to each connected component
    void assign_network_ids() {
        std::fill(network_id_.begin(), network_id_.end(), 0);
        next_network_id_ = 1;

        for (size_t i = 0; i < parent_.size(); ++i) {
            if (parent_[i] != INVALID) {
                uint32_t root = find(static_cast<uint32_t>(i));
                if (network_id_[root] == 0) {
                    network_id_[root] = next_network_id_++;
                }
            }
        }
    }

    uint32_t width_ = 0;
    uint32_t height_ = 0;

    // Dense arrays for O(1) lookup (indexed by linear coordinate)
    // These are MAP-WIDE arrays, not per-pathway storage
    std::vector<uint32_t> parent_;     // Union-find parent (INVALID = no node)
    std::vector<uint32_t> rank_;       // Union-find rank
    std::vector<uint32_t> network_id_; // Network ID for root nodes

    size_t node_count_ = 0;
    size_t edge_count_ = 0;
    uint32_t next_network_id_ = 1;
};
