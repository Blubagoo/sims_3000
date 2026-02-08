/**
 * @file RailSystem.cpp
 * @brief RailSystem implementation for Epic 7 (Tickets E7-032, E7-033, E7-034)
 *
 * Implements the rail system with three tick phases:
 * 1. Power state updates from IEnergyProvider (fallback: all powered)
 * 2. Active state updates (powered + terminal adjacency check)
 * 3. Terminal coverage effects
 *
 * E7-033: Queries IEnergyProvider.is_powered_at() for each rail/terminal.
 *         Falls back to all-powered when no provider is set.
 * E7-034: Terminal placement validation and activation checks.
 */

#include <sims3000/transport/RailSystem.h>
#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace transport {

RailSystem::RailSystem(uint32_t map_width, uint32_t map_height)
    : map_width_(map_width)
    , map_height_(map_height)
    , next_entity_id_(1)
    , energy_provider_(nullptr)
{
}

// =============================================================================
// ISimulatable interface
// =============================================================================

void RailSystem::tick(float /*delta_time*/) {
    // Phase 1: Update power states from energy provider
    update_power_states();

    // Phase 2: Update active states (powered + terminal connection)
    update_active_states();

    // Phase 3: Calculate terminal coverage effects (stub)
    update_terminal_coverage();
}

// =============================================================================
// Rail management
// =============================================================================

uint32_t RailSystem::place_rail(int32_t x, int32_t y, RailType type, uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }

    // Bounds check
    if (x < 0 || y < 0 ||
        static_cast<uint32_t>(x) >= map_width_ ||
        static_cast<uint32_t>(y) >= map_height_) {
        return 0;
    }

    uint32_t id = next_entity_id_++;

    RailEntry entry{};
    entry.entity_id = id;
    entry.component.type = type;
    entry.component.is_powered = true;   // Stub: default powered
    entry.component.is_active = false;   // Will be set by active state update
    entry.x = x;
    entry.y = y;

    rails_[owner].push_back(entry);
    return id;
}

bool RailSystem::remove_rail(uint32_t entity_id, uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return false;
    }

    auto& player_rails = rails_[owner];
    auto it = std::find_if(player_rails.begin(), player_rails.end(),
        [entity_id](const RailEntry& e) { return e.entity_id == entity_id; });

    if (it == player_rails.end()) {
        return false;
    }

    // Swap-and-pop for O(1) removal
    *it = player_rails.back();
    player_rails.pop_back();
    return true;
}

// =============================================================================
// Terminal management
// =============================================================================

uint32_t RailSystem::place_terminal(int32_t x, int32_t y, TerminalType type, uint8_t owner) {
    if (!can_place_terminal(x, y, owner)) {
        return 0;
    }

    uint32_t id = next_entity_id_++;

    TerminalEntry entry{};
    entry.entity_id = id;
    entry.component.type = type;
    entry.component.is_powered = false;  // Will be set by tick phase 1
    entry.component.is_active = false;   // Will be set by tick phase 2
    entry.x = x;
    entry.y = y;

    terminals_[owner].push_back(entry);
    return id;
}

bool RailSystem::remove_terminal(uint32_t entity_id, uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return false;
    }

    auto& player_terminals = terminals_[owner];
    auto it = std::find_if(player_terminals.begin(), player_terminals.end(),
        [entity_id](const TerminalEntry& e) { return e.entity_id == entity_id; });

    if (it == player_terminals.end()) {
        return false;
    }

    // Swap-and-pop for O(1) removal
    *it = player_terminals.back();
    player_terminals.pop_back();
    return true;
}

// =============================================================================
// Power dependency
// =============================================================================

void RailSystem::set_energy_provider(const building::IEnergyProvider* provider) {
    energy_provider_ = provider;
}

// =============================================================================
// Queries
// =============================================================================

bool RailSystem::is_rail_powered(uint32_t entity_id) const {
    for (uint8_t owner = 0; owner < MAX_PLAYERS; ++owner) {
        for (const auto& entry : rails_[owner]) {
            if (entry.entity_id == entity_id) {
                return entry.component.is_powered;
            }
        }
    }
    return false;
}

bool RailSystem::is_terminal_active(uint32_t entity_id) const {
    for (uint8_t owner = 0; owner < MAX_PLAYERS; ++owner) {
        for (const auto& entry : terminals_[owner]) {
            if (entry.entity_id == entity_id) {
                return entry.component.is_active;
            }
        }
    }
    return false;
}

uint8_t RailSystem::get_terminal_coverage_radius(uint32_t entity_id) const {
    for (uint8_t owner = 0; owner < MAX_PLAYERS; ++owner) {
        for (const auto& entry : terminals_[owner]) {
            if (entry.entity_id == entity_id) {
                return entry.component.coverage_radius;
            }
        }
    }
    return 0;
}

// =============================================================================
// Coverage queries (E7-035)
// =============================================================================

bool RailSystem::is_in_terminal_coverage(int32_t x, int32_t y, uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return false;
    }

    for (const auto& terminal : terminals_[owner]) {
        if (!terminal.component.is_active) {
            continue;
        }

        int32_t dx = std::abs(x - terminal.x);
        int32_t dy = std::abs(y - terminal.y);
        uint32_t dist = static_cast<uint32_t>(dx + dy);

        if (dist <= terminal.component.coverage_radius) {
            return true;
        }
    }

    return false;
}

uint8_t RailSystem::get_traffic_reduction_at(int32_t x, int32_t y, uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }

    uint8_t max_reduction = 0;

    for (const auto& terminal : terminals_[owner]) {
        if (!terminal.component.is_active) {
            continue;
        }

        int32_t dx = std::abs(x - terminal.x);
        int32_t dy = std::abs(y - terminal.y);
        uint32_t dist = static_cast<uint32_t>(dx + dy);
        uint8_t radius = terminal.component.coverage_radius;

        if (dist > radius) {
            continue;
        }

        // Linear falloff: 50% at distance 0 (terminal), 0% at radius edge
        // reduction = 50 * (1 - dist / radius)
        // = 50 * (radius - dist) / radius
        uint8_t reduction = 0;
        if (radius > 0) {
            reduction = static_cast<uint8_t>((50u * (radius - dist)) / radius);
        } else {
            // radius == 0, distance must be 0 to reach here
            reduction = 50;
        }

        if (reduction > max_reduction) {
            max_reduction = reduction;
        }
    }

    return max_reduction;
}

uint8_t RailSystem::calculate_traffic_reduction(int32_t x, int32_t y, uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }

    uint8_t max_reduction = 0;

    for (const auto& terminal : terminals_[owner]) {
        if (!terminal.component.is_active) {
            continue;
        }

        int32_t dist = std::abs(x - terminal.x) + std::abs(y - terminal.y);
        uint8_t radius = terminal.component.coverage_radius;

        if (static_cast<uint32_t>(dist) > radius) {
            continue;
        }

        // Linear falloff: 50% at distance 0 (terminal), 0% at radius edge
        uint8_t reduction = 0;
        if (radius > 0) {
            reduction = static_cast<uint8_t>((50u * (radius - static_cast<uint32_t>(dist))) / radius);
        } else {
            reduction = 50;
        }

        if (reduction > max_reduction) {
            max_reduction = reduction;
        }
    }

    return max_reduction;
}

// =============================================================================
// State queries
// =============================================================================

uint32_t RailSystem::get_rail_count(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }
    return static_cast<uint32_t>(rails_[owner].size());
}

uint32_t RailSystem::get_terminal_count(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }
    return static_cast<uint32_t>(terminals_[owner].size());
}

// =============================================================================
// Internal tick phases
// =============================================================================

void RailSystem::update_power_states() {
    // Phase 1: Update power states from energy provider
    // If no energy provider is set, default all rails/terminals to powered (fallback).
    // Otherwise, query IEnergyProvider::is_powered_at() for each entity's position.
    for (uint8_t owner = 0; owner < MAX_PLAYERS; ++owner) {
        for (auto& entry : rails_[owner]) {
            if (energy_provider_) {
                entry.component.is_powered = energy_provider_->is_powered_at(
                    static_cast<uint32_t>(entry.x),
                    static_cast<uint32_t>(entry.y),
                    static_cast<uint32_t>(owner));
            } else {
                entry.component.is_powered = true;  // Fallback: all powered
            }
        }
        for (auto& entry : terminals_[owner]) {
            if (energy_provider_) {
                entry.component.is_powered = energy_provider_->is_powered_at(
                    static_cast<uint32_t>(entry.x),
                    static_cast<uint32_t>(entry.y),
                    static_cast<uint32_t>(owner));
            } else {
                entry.component.is_powered = true;  // Fallback: all powered
            }
        }
    }
}

void RailSystem::update_active_states() {
    // Phase 2: Update active states
    // A rail is active if it is powered AND has terminal connection (terminal_adjacent)
    // A terminal is active if it is powered AND has adjacent rail track
    for (uint8_t owner = 0; owner < MAX_PLAYERS; ++owner) {
        for (auto& entry : rails_[owner]) {
            entry.component.is_active = entry.component.is_powered
                                     && entry.component.is_terminal_adjacent;
        }
        for (auto& entry : terminals_[owner]) {
            entry.component.is_active = check_terminal_activation(entry);
        }
    }
}

void RailSystem::update_terminal_coverage() {
    // Phase 3: Calculate terminal coverage effects
    // Stub: No coverage effects yet
    // Future implementation will mark rails as terminal-adjacent
    // based on proximity to active terminals

    // Mark rails as terminal-adjacent if within coverage radius
    for (uint8_t owner = 0; owner < MAX_PLAYERS; ++owner) {
        // Reset all terminal adjacency flags
        for (auto& rail : rails_[owner]) {
            rail.component.is_terminal_adjacent = false;
        }

        // Check each terminal's coverage radius against each rail
        for (const auto& terminal : terminals_[owner]) {
            if (!terminal.component.is_active) {
                continue;
            }

            uint8_t radius = terminal.component.coverage_radius;

            for (auto& rail : rails_[owner]) {
                int32_t dx = rail.x - terminal.x;
                int32_t dy = rail.y - terminal.y;
                // Manhattan distance for coverage check
                uint32_t dist = static_cast<uint32_t>(std::abs(dx) + std::abs(dy));
                if (dist <= radius) {
                    rail.component.is_terminal_adjacent = true;
                }
            }
        }
    }
}

// =============================================================================
// Terminal placement validation (E7-034)
// =============================================================================

bool RailSystem::can_place_terminal(int32_t x, int32_t y, uint8_t owner) const {
    // Check 1: Valid owner
    if (owner >= MAX_PLAYERS) {
        return false;
    }

    // Check 2: Bounds check
    if (x < 0 || y < 0 ||
        static_cast<uint32_t>(x) >= map_width_ ||
        static_cast<uint32_t>(y) >= map_height_) {
        return false;
    }

    // Check 3: Not already occupied by another terminal (any player)
    if (has_terminal_at(x, y)) {
        return false;
    }

    // Check 4: Adjacent rail track exists (N/S/E/W)
    if (!has_adjacent_rail(x, y)) {
        return false;
    }

    // Power check is NOT performed at placement time (deferred to tick)
    return true;
}

bool RailSystem::check_terminal_activation(const TerminalEntry& terminal) const {
    // Terminal activation requires:
    // 1. is_powered = true (already set by Phase 1)
    if (!terminal.component.is_powered) {
        return false;
    }

    // 2. Adjacent rail track exists (N/S/E/W)
    if (!has_adjacent_rail(terminal.x, terminal.y)) {
        return false;
    }

    return true;
}

// =============================================================================
// Internal helpers
// =============================================================================

bool RailSystem::has_rail_at(int32_t x, int32_t y) const {
    for (uint8_t owner = 0; owner < MAX_PLAYERS; ++owner) {
        for (const auto& entry : rails_[owner]) {
            if (entry.x == x && entry.y == y) {
                return true;
            }
        }
    }
    return false;
}

bool RailSystem::has_adjacent_rail(int32_t x, int32_t y) const {
    // Check N/S/E/W cardinal directions for any rail entity
    static const int32_t dx[] = { 0, 0, -1, 1 };
    static const int32_t dy[] = { -1, 1, 0, 0 };

    for (int i = 0; i < 4; ++i) {
        if (has_rail_at(x + dx[i], y + dy[i])) {
            return true;
        }
    }
    return false;
}

bool RailSystem::has_terminal_at(int32_t x, int32_t y) const {
    for (uint8_t owner = 0; owner < MAX_PLAYERS; ++owner) {
        for (const auto& entry : terminals_[owner]) {
            if (entry.x == x && entry.y == y) {
                return true;
            }
        }
    }
    return false;
}

} // namespace transport
} // namespace sims3000
