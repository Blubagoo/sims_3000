/**
 * @file PortSystem.cpp
 * @brief Implementation of PortSystem orchestrator (Epic 8, Tickets E8-006, E8-018, E8-020)
 *
 * E8-006: Core PortSystem stub with IPortProvider interface.
 * E8-018: DemandSystem integration - replaced stub demand bonus methods with
 *         real implementations delegating to calculate_global_demand_bonus()
 *         and calculate_local_demand_bonus() from DemandBonus.h.
 * E8-020: EconomySystem integration - wired calculate_trade_income() tick phase
 *         to use real TradeIncome functions. Added get_trade_income_breakdown(),
 *         trade agreement management, and cached breakdowns per player.
 *
 * Port state queries (capacity, utilization, has_operational, count) now
 * compute values from the tracked m_ports collection.
 *
 * Tick phases:
 * 1. Update port states (stub - will scan ECS entities)
 * 2. Update external connections (stub)
 * 3. Calculate trade income (wired to TradeIncome.h functions, E8-020)
 * 4. Cache demand bonuses (no-op, computed on query)
 *
 * @see PortSystem.h for class documentation.
 * @see DemandBonus.h for demand bonus calculation functions.
 */

#include <sims3000/port/PortSystem.h>
#include <sims3000/port/DemandBonus.h>
#include <sims3000/port/TradeIncome.h>
#include <algorithm>

namespace sims3000 {
namespace port {

// =============================================================================
// Construction
// =============================================================================

PortSystem::PortSystem(std::int32_t map_width, std::int32_t map_height)
    : m_map_width(map_width)
    , m_map_height(map_height)
{
    for (std::uint8_t i = 0; i <= MAX_PLAYERS; ++i) {
        m_cached_trade_income[i] = 0;
    }
}

// =============================================================================
// ISimulatable interface
// =============================================================================

void PortSystem::tick(float delta_time) {
    (void)delta_time;

    // Execute tick phases
    update_port_states();
    update_external_connections();
    calculate_trade_income();
    cache_demand_bonuses();
}

// =============================================================================
// Port Data Management
// =============================================================================

void PortSystem::add_port(const PortData& port) {
    m_ports.push_back(port);
}

void PortSystem::remove_port(std::uint8_t owner, std::int32_t x, std::int32_t y) {
    m_ports.erase(
        std::remove_if(m_ports.begin(), m_ports.end(),
            [owner, x, y](const PortData& p) {
                return p.owner == owner && p.x == x && p.y == y;
            }),
        m_ports.end());
}

void PortSystem::clear_ports() {
    m_ports.clear();
}

void PortSystem::set_cached_trade_income(std::uint8_t owner, std::int64_t income) {
    if (owner <= MAX_PLAYERS) {
        m_cached_trade_income[owner] = income;
    }
}

// =============================================================================
// Trade Income Breakdown (E8-020)
// =============================================================================

TradeIncomeBreakdown PortSystem::get_trade_income_breakdown(std::uint8_t owner) const {
    if (owner <= MAX_PLAYERS) {
        return m_cached_breakdowns[owner];
    }
    return TradeIncomeBreakdown{};
}

// =============================================================================
// Trade Agreement Management (E8-020)
// =============================================================================

void PortSystem::add_trade_agreement(const TradeAgreementComponent& agreement) {
    m_agreements.push_back(agreement);
}

void PortSystem::clear_trade_agreements() {
    m_agreements.clear();
}

// =============================================================================
// IPortProvider Implementation
// =============================================================================

std::uint32_t PortSystem::get_port_capacity(std::uint8_t port_type, std::uint8_t owner) const {
    std::uint32_t total = 0;
    PortType pt = static_cast<PortType>(port_type);
    for (const auto& p : m_ports) {
        if (p.port_type == pt && p.owner == owner && p.is_operational) {
            total += p.capacity;
        }
    }
    return total;
}

float PortSystem::get_port_utilization(std::uint8_t port_type, std::uint8_t owner) const {
    // Utilization is computed as a weighted average across ports of this type.
    // For now, return 0.0 if no ports (future tickets may integrate real utilization).
    std::uint32_t total_capacity = 0;
    PortType pt = static_cast<PortType>(port_type);
    for (const auto& p : m_ports) {
        if (p.port_type == pt && p.owner == owner && p.is_operational) {
            total_capacity += p.capacity;
        }
    }
    if (total_capacity == 0) {
        return 0.0f;
    }
    // Utilization tracking requires external usage data; return 0 until fully wired
    return 0.0f;
}

bool PortSystem::has_operational_port(std::uint8_t port_type, std::uint8_t owner) const {
    PortType pt = static_cast<PortType>(port_type);
    for (const auto& p : m_ports) {
        if (p.port_type == pt && p.owner == owner && p.is_operational) {
            return true;
        }
    }
    return false;
}

std::uint32_t PortSystem::get_port_count(std::uint8_t port_type, std::uint8_t owner) const {
    std::uint32_t count = 0;
    PortType pt = static_cast<PortType>(port_type);
    for (const auto& p : m_ports) {
        if (p.port_type == pt && p.owner == owner) {
            ++count;
        }
    }
    return count;
}

float PortSystem::get_global_demand_bonus(std::uint8_t zone_type, std::uint8_t owner) const {
    return calculate_global_demand_bonus(zone_type, owner, m_ports);
}

float PortSystem::get_local_demand_bonus(std::uint8_t zone_type, std::int32_t x, std::int32_t y, std::uint8_t owner) const {
    return calculate_local_demand_bonus(zone_type, x, y, owner, m_ports);
}

std::uint32_t PortSystem::get_external_connection_count(std::uint8_t owner) const {
    (void)owner;
    return 0;
}

bool PortSystem::is_connected_to_edge(std::uint8_t edge, std::uint8_t owner) const {
    (void)edge; (void)owner;
    return false;
}

std::int64_t PortSystem::get_trade_income(std::uint8_t owner) const {
    if (owner <= MAX_PLAYERS) {
        return m_cached_trade_income[owner];
    }
    return 0;
}

// =============================================================================
// Tick Phases
// =============================================================================

void PortSystem::update_port_states() {
    // Stub: Will scan PortComponent entities and update operational status
}

void PortSystem::update_external_connections() {
    // Stub: Will scan ExternalConnectionComponent entities and update active state
}

void PortSystem::calculate_trade_income() {
    // E8-020: Calculate trade income for each player using real TradeIncome functions
    // E8-021: Also record income history for UI display
    for (std::uint8_t owner = 0; owner <= MAX_PLAYERS; ++owner) {
        TradeIncomeBreakdown breakdown = sims3000::port::calculate_trade_income(
            owner, m_ports, m_agreements);
        m_cached_breakdowns[owner] = breakdown;
        m_cached_trade_income[owner] = breakdown.total;

        // Record income in circular history buffer (E8-021)
        m_income_history[owner][m_history_index[owner]] = breakdown.total;
        m_history_index[owner] = (m_history_index[owner] + 1) % INCOME_HISTORY_SIZE;
        m_history_initialized[owner] = true;
    }
}

void PortSystem::cache_demand_bonuses() {
    // Demand bonuses are now computed on-demand in get_global_demand_bonus()
    // and get_local_demand_bonus() by delegating to DemandBonus.h functions.
    // No caching needed for current implementation.
}

// =============================================================================
// Trade Income UI Data (E8-021)
// =============================================================================

TradeIncomeUIData PortSystem::get_trade_income_ui_data(std::uint8_t owner) const {
    TradeIncomeUIData ui_data;

    // Get aggregate breakdown
    ui_data.breakdown = get_trade_income_breakdown(owner);

    // Get trade multiplier for income calculation
    float trade_multiplier = get_trade_multiplier(owner, m_agreements);

    // Build per-port income details
    for (const auto& port : m_ports) {
        if (port.owner != owner) continue;

        PortIncomeDetail detail;
        detail.entity_id = 0;  // Entity ID not tracked in PortData; will be wired in ECS integration
        detail.port_type = port.port_type;
        detail.capacity = port.capacity;

        if (port.is_operational && port.capacity > 0) {
            float utilization = estimate_port_utilization(port);
            float income_rate = get_income_rate(port.port_type);
            float base_income = static_cast<float>(port.capacity) *
                                utilization * income_rate * DEFAULT_EXTERNAL_DEMAND_FACTOR;
            detail.income = static_cast<int64_t>(base_income * trade_multiplier);
            detail.utilization = static_cast<uint8_t>(utilization * 100.0f);
        } else {
            detail.income = 0;
            detail.utilization = 0;
        }

        ui_data.port_details.push_back(detail);
    }

    // Copy income history (reorder from circular buffer to chronological)
    if (owner <= MAX_PLAYERS) {
        std::size_t idx = m_history_index[owner];
        for (std::size_t i = 0; i < INCOME_HISTORY_SIZE; ++i) {
            ui_data.income_history[i] = m_income_history[owner][(idx + i) % INCOME_HISTORY_SIZE];
        }
    }

    return ui_data;
}

// =============================================================================
// Port Zone Management (E8-030)
// =============================================================================

void PortSystem::set_port_zone(std::uint8_t owner, std::int32_t x, std::int32_t y,
                               const PortZoneComponent& zone) {
    // Update existing entry if found
    for (auto& entry : m_port_zones) {
        if (entry.owner == owner && entry.x == x && entry.y == y) {
            entry.zone = zone;
            return;
        }
    }
    // Otherwise add new entry
    m_port_zones.push_back({owner, x, y, zone});
}

bool PortSystem::get_port_zone(std::uint8_t owner, std::int32_t x, std::int32_t y,
                               PortZoneComponent& out_zone) const {
    for (const auto& entry : m_port_zones) {
        if (entry.owner == owner && entry.x == x && entry.y == y) {
            out_zone = entry.zone;
            return true;
        }
    }
    return false;
}

// =============================================================================
// Port Render Data (E8-030)
// =============================================================================

std::vector<PortRenderData> PortSystem::get_port_render_data(std::uint8_t owner) const {
    std::vector<PortRenderData> result;

    for (const auto& port : m_ports) {
        if (port.owner != owner) continue;

        PortRenderData rd;
        rd.x = port.x;
        rd.y = port.y;
        rd.port_type = port.port_type;
        rd.is_operational = port.is_operational;

        // Look up zone data for development level and infrastructure details
        PortZoneComponent zone;
        if (get_port_zone(owner, port.x, port.y, zone)) {
            rd.zone_level = zone.zone_level;
            rd.width = zone.zone_tiles > 0 ? zone.zone_tiles : 1;
            rd.height = 1;  // Default; actual dimensions come from zone geometry

            // Aero port: runway data
            if (port.port_type == PortType::Aero) {
                rd.runway_x = zone.runway_area.x;
                rd.runway_y = zone.runway_area.y;
                rd.runway_w = zone.runway_area.width;
                rd.runway_h = zone.runway_area.height;
                rd.dock_count = 0;
            }
            // Aqua port: dock data
            else if (port.port_type == PortType::Aqua) {
                rd.dock_count = zone.dock_count;
                rd.runway_x = 0;
                rd.runway_y = 0;
                rd.runway_w = 0;
                rd.runway_h = 0;
            }
        } else {
            // No zone data; use minimal defaults
            rd.zone_level = 0;
            rd.width = 1;
            rd.height = 1;
            rd.dock_count = 0;
        }

        // Calculate boundary flags based on map edge proximity
        rd.boundary_flags = 0;
        if (port.y == 0) rd.boundary_flags |= BOUNDARY_NORTH;
        if (port.y >= m_map_height - 1) rd.boundary_flags |= BOUNDARY_SOUTH;
        if (port.x >= m_map_width - 1) rd.boundary_flags |= BOUNDARY_EAST;
        if (port.x == 0) rd.boundary_flags |= BOUNDARY_WEST;

        result.push_back(rd);
    }

    return result;
}

} // namespace port
} // namespace sims3000
