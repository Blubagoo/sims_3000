#pragma once

#include "PathwayGrid.h"
#include <cstdint>
#include <vector>
#include <cmath>

// FlowSimulation: Aggregate flow model (NOT per-vehicle)
// Buildings have traffic_contribution, flow diffuses along pathway network.
// Congestion = flow / capacity per pathway tile.
// Uses iterative diffusion (3-5 iterations sufficient).
// Per systems.yaml TransportSystem traffic simulation

// Building traffic source
struct TrafficSource {
    int32_t x;
    int32_t y;
    uint16_t contribution;
};

class FlowSimulation {
public:
    static constexpr uint16_t DEFAULT_CAPACITY = 1000;
    static constexpr float DIFFUSION_RATE = 0.25f; // Flow spreads to neighbors each iteration

    FlowSimulation() = default;

    FlowSimulation(uint32_t width, uint32_t height)
        : width_(width), height_(height)
        , flow_(width * height, 0)
        , capacity_(width * height, DEFAULT_CAPACITY)
        , flow_buffer_(width * height, 0) {}

    void resize(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;
        flow_.assign(width * height, 0);
        capacity_.assign(width * height, DEFAULT_CAPACITY);
        flow_buffer_.assign(width * height, 0);
    }

    // Set capacity for a pathway tile
    void set_capacity(int32_t x, int32_t y, uint16_t cap) {
        if (in_bounds(x, y)) {
            capacity_[y * width_ + x] = cap;
        }
    }

    // Get flow at position
    uint32_t get_flow_at(int32_t x, int32_t y) const {
        if (!in_bounds(x, y)) return 0;
        return flow_[y * width_ + x];
    }

    // Get congestion at position (0.0 = free, 1.0+ = congested)
    float get_congestion_at(int32_t x, int32_t y) const {
        if (!in_bounds(x, y)) return 0.0f;
        size_t idx = y * width_ + x;
        if (capacity_[idx] == 0) return 0.0f;
        return static_cast<float>(flow_[idx]) / static_cast<float>(capacity_[idx]);
    }

    // Simulate traffic flow using iterative diffusion
    // sources: buildings generating traffic
    // pathways: the pathway grid (only diffuse along pathways)
    // iterations: number of diffusion passes (3-5 recommended)
    void simulate(const std::vector<TrafficSource>& sources,
                  const PathwayGrid& pathways,
                  int iterations = 5) {
        // Reset flow
        std::fill(flow_.begin(), flow_.end(), 0);

        // Inject traffic from sources to nearest pathway tiles
        inject_traffic(sources, pathways);

        // Diffusion iterations
        for (int iter = 0; iter < iterations; ++iter) {
            diffuse_flow(pathways);
        }
    }

    bool in_bounds(int32_t x, int32_t y) const {
        return x >= 0 && x < static_cast<int32_t>(width_) &&
               y >= 0 && y < static_cast<int32_t>(height_);
    }

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

    // Memory info
    size_t memory_bytes() const {
        return flow_.size() * sizeof(uint32_t) +
               capacity_.size() * sizeof(uint16_t) +
               flow_buffer_.size() * sizeof(uint32_t);
    }

private:
    // Inject traffic from buildings to their nearest pathway tiles
    void inject_traffic(const std::vector<TrafficSource>& sources,
                       const PathwayGrid& pathways) {
        // For each source, find nearest pathway and add contribution
        for (const auto& src : sources) {
            // Try to find a pathway within 3 tiles (Manhattan distance)
            int32_t best_x = -1, best_y = -1;
            int32_t best_dist = 4; // Max search distance + 1

            // Search in expanding squares
            for (int32_t dist = 0; dist < 4; ++dist) {
                for (int32_t dy = -dist; dy <= dist; ++dy) {
                    for (int32_t dx = -dist; dx <= dist; ++dx) {
                        if (std::abs(dx) + std::abs(dy) != dist) continue; // Manhattan distance

                        int32_t px = src.x + dx;
                        int32_t py = src.y + dy;

                        if (pathways.has_pathway(px, py)) {
                            best_x = px;
                            best_y = py;
                            best_dist = dist;
                            goto found;
                        }
                    }
                }
            }
            found:

            if (best_x >= 0 && best_y >= 0) {
                size_t idx = best_y * width_ + best_x;
                flow_[idx] += src.contribution;
            }
        }
    }

    // Single diffusion pass
    void diffuse_flow(const PathwayGrid& pathways) {
        // Copy current flow to buffer
        std::copy(flow_.begin(), flow_.end(), flow_buffer_.begin());

        // 4-connected neighbors
        static const int32_t dx[] = {0, 1, 0, -1};
        static const int32_t dy[] = {-1, 0, 1, 0};

        for (int32_t y = 0; y < static_cast<int32_t>(height_); ++y) {
            for (int32_t x = 0; x < static_cast<int32_t>(width_); ++x) {
                if (!pathways.has_pathway(x, y)) continue;

                size_t idx = y * width_ + x;
                uint32_t current_flow = flow_buffer_[idx];

                if (current_flow == 0) continue;

                // Count valid pathway neighbors
                int neighbor_count = 0;
                for (int i = 0; i < 4; ++i) {
                    int32_t nx = x + dx[i];
                    int32_t ny = y + dy[i];
                    if (pathways.has_pathway(nx, ny)) {
                        neighbor_count++;
                    }
                }

                if (neighbor_count == 0) continue;

                // Calculate flow to spread
                uint32_t flow_to_spread = static_cast<uint32_t>(
                    current_flow * DIFFUSION_RATE);
                uint32_t flow_per_neighbor = flow_to_spread / neighbor_count;

                if (flow_per_neighbor == 0) continue;

                // Spread to neighbors
                for (int i = 0; i < 4; ++i) {
                    int32_t nx = x + dx[i];
                    int32_t ny = y + dy[i];
                    if (pathways.has_pathway(nx, ny)) {
                        size_t nidx = ny * width_ + nx;
                        flow_[nidx] += flow_per_neighbor;
                    }
                }

                // Reduce current tile's flow
                flow_[idx] -= flow_per_neighbor * neighbor_count;
            }
        }
    }

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    std::vector<uint32_t> flow_;      // Current flow per tile
    std::vector<uint16_t> capacity_;  // Capacity per tile
    std::vector<uint32_t> flow_buffer_; // Double buffer for diffusion
};
