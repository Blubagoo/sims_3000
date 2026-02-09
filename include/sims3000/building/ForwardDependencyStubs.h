/**
 * @file ForwardDependencyStubs.h
 * @brief Stub implementations of all 8 forward dependency interfaces
 *
 * Provides permissive default implementations for testing and development
 * before real systems are available. Each stub has a debug_restrictive mode
 * that returns false/0/negative values to test failure paths.
 *
 * Stubs:
 * - StubEnergyProvider: is_powered() -> true
 * - StubFluidProvider: has_fluid() -> true
 * - StubTransportProvider: is_road_accessible_at() -> true
 * - StubPortProvider: get_port_capacity() -> 0, has_operational_port() -> false
 * - StubServiceQueryable: get_coverage() -> 0.0f, get_effectiveness() -> 0.0f
 * - StubLandValueProvider: get_land_value() -> 50.0f
 * - StubDemandProvider: get_demand() -> 1.0f
 * - StubCreditProvider: deduct_credits() -> true, has_credits() -> true
 *
 * @see ForwardDependencyInterfaces.h
 * @see /docs/epics/epic-4/tickets.md (ticket 4-020)
 */

#ifndef SIMS3000_BUILDING_FORWARDDEPENDENCYSTUBS_H
#define SIMS3000_BUILDING_FORWARDDEPENDENCYSTUBS_H

#include <sims3000/building/ForwardDependencyInterfaces.h>

namespace sims3000 {
namespace building {

/**
 * @class StubEnergyProvider
 * @brief Permissive energy provider stub.
 *
 * Default: all entities/positions are powered.
 * Debug restrictive: nothing is powered.
 */
class StubEnergyProvider : public IEnergyProvider {
public:
    StubEnergyProvider() : m_restrictive(false) {}

    bool is_powered(std::uint32_t entity_id) const override {
        (void)entity_id;
        return !m_restrictive;
    }

    bool is_powered_at(std::uint32_t x, std::uint32_t y, std::uint32_t player_id) const override {
        (void)x; (void)y; (void)player_id;
        return !m_restrictive;
    }

    void set_debug_restrictive(bool restrictive) { m_restrictive = restrictive; }
    bool is_debug_restrictive() const { return m_restrictive; }

private:
    bool m_restrictive;
};

/**
 * @class StubFluidProvider
 * @brief Permissive fluid provider stub.
 *
 * Default: all entities/positions have fluid.
 * Debug restrictive: nothing has fluid.
 */
class StubFluidProvider : public IFluidProvider {
public:
    StubFluidProvider() : m_restrictive(false) {}

    bool has_fluid(std::uint32_t entity_id) const override {
        (void)entity_id;
        return !m_restrictive;
    }

    bool has_fluid_at(std::uint32_t x, std::uint32_t y, std::uint32_t player_id) const override {
        (void)x; (void)y; (void)player_id;
        return !m_restrictive;
    }

    void set_debug_restrictive(bool restrictive) { m_restrictive = restrictive; }
    bool is_debug_restrictive() const { return m_restrictive; }

private:
    bool m_restrictive;
};

/**
 * @class StubTransportProvider
 * @brief Permissive transport provider stub.
 *
 * Default: all positions are road-accessible, connected, distance 0, no congestion.
 * Debug restrictive: nothing is accessible, distance 255, fully congested.
 *
 * Implements all ITransportProvider methods including Epic 7 extensions.
 */
class StubTransportProvider : public ITransportProvider {
public:
    StubTransportProvider() : m_restrictive(false) {}

    // Original methods (Epic 4)
    bool is_road_accessible_at(std::uint32_t x, std::uint32_t y, std::uint32_t max_distance) const override {
        (void)x; (void)y; (void)max_distance;
        return !m_restrictive;
    }

    std::uint32_t get_nearest_road_distance(std::uint32_t x, std::uint32_t y) const override {
        (void)x; (void)y;
        return m_restrictive ? 255 : 0;
    }

    // Extended methods (Epic 7, Ticket E7-016)
    bool is_road_accessible(EntityID entity) const override {
        (void)entity;
        return !m_restrictive;
    }

    bool is_connected_to_network(std::int32_t x, std::int32_t y) const override {
        (void)x; (void)y;
        return !m_restrictive;
    }

    bool are_connected(std::int32_t x1, std::int32_t y1,
                       std::int32_t x2, std::int32_t y2) const override {
        (void)x1; (void)y1; (void)x2; (void)y2;
        return !m_restrictive;
    }

    float get_congestion_at(std::int32_t x, std::int32_t y) const override {
        (void)x; (void)y;
        return m_restrictive ? 1.0f : 0.0f;
    }

    std::uint32_t get_traffic_volume_at(std::int32_t x, std::int32_t y) const override {
        (void)x; (void)y;
        return m_restrictive ? 1000 : 0;
    }

    std::uint16_t get_network_id_at(std::int32_t x, std::int32_t y) const override {
        (void)x; (void)y;
        return m_restrictive ? 0 : 1;
    }

    void set_debug_restrictive(bool restrictive) { m_restrictive = restrictive; }
    bool is_debug_restrictive() const { return m_restrictive; }

private:
    bool m_restrictive;
};

/**
 * @class StubPortProvider
 * @brief Port provider stub with safe defaults.
 *
 * Default: no ports, no capacity, no connections, no trade income.
 * Debug restrictive: same as default (ports are opt-in infrastructure).
 */
class StubPortProvider : public IPortProvider {
public:
    StubPortProvider() : m_restrictive(false) {}

    // Port state queries
    std::uint32_t get_port_capacity(std::uint8_t port_type, std::uint8_t owner) const override {
        (void)port_type; (void)owner;
        return 0;
    }

    float get_port_utilization(std::uint8_t port_type, std::uint8_t owner) const override {
        (void)port_type; (void)owner;
        return 0.0f;
    }

    bool has_operational_port(std::uint8_t port_type, std::uint8_t owner) const override {
        (void)port_type; (void)owner;
        return false;
    }

    std::uint32_t get_port_count(std::uint8_t port_type, std::uint8_t owner) const override {
        (void)port_type; (void)owner;
        return 0;
    }

    // Demand bonus queries
    float get_global_demand_bonus(std::uint8_t zone_type, std::uint8_t owner) const override {
        (void)zone_type; (void)owner;
        return 0.0f;
    }

    float get_local_demand_bonus(std::uint8_t zone_type, std::int32_t x, std::int32_t y, std::uint8_t owner) const override {
        (void)zone_type; (void)x; (void)y; (void)owner;
        return 0.0f;
    }

    // External connection queries
    std::uint32_t get_external_connection_count(std::uint8_t owner) const override {
        (void)owner;
        return 0;
    }

    bool is_connected_to_edge(std::uint8_t edge, std::uint8_t owner) const override {
        (void)edge; (void)owner;
        return false;
    }

    // Trade income queries
    std::int64_t get_trade_income(std::uint8_t owner) const override {
        (void)owner;
        return 0;
    }

    void set_debug_restrictive(bool restrictive) { m_restrictive = restrictive; }
    bool is_debug_restrictive() const { return m_restrictive; }

private:
    bool m_restrictive;
};

/**
 * @class StubServiceQueryable
 * @brief Service queryable stub with safe defaults.
 *
 * Default: no coverage, no effectiveness (0.0f).
 * Services are opt-in infrastructure, so the safe default is 0.0f
 * (not 0.5f) when no service buildings exist.
 * Debug restrictive: same as default (services are opt-in).
 */
class StubServiceQueryable : public IServiceQueryable {
public:
    StubServiceQueryable() : m_restrictive(false) {}

    float get_coverage(std::uint8_t service_type, std::uint8_t player_id) const override {
        (void)service_type; (void)player_id;
        return 0.0f;
    }

    float get_coverage_at(std::uint8_t service_type, std::int32_t x, std::int32_t y, std::uint8_t player_id) const override {
        (void)service_type; (void)x; (void)y; (void)player_id;
        return 0.0f;
    }

    float get_effectiveness(std::uint8_t service_type, std::uint8_t player_id) const override {
        (void)service_type; (void)player_id;
        return 0.0f;
    }

    void set_debug_restrictive(bool restrictive) { m_restrictive = restrictive; }
    bool is_debug_restrictive() const { return m_restrictive; }

private:
    bool m_restrictive;
};

/**
 * @class StubLandValueProvider
 * @brief Permissive land value provider stub.
 *
 * Default: land value 50.0f (neutral).
 * Debug restrictive: land value 0.0f (no value).
 */
class StubLandValueProvider : public ILandValueProvider {
public:
    StubLandValueProvider() : m_restrictive(false) {}

    float get_land_value(std::uint32_t x, std::uint32_t y) const override {
        (void)x; (void)y;
        return m_restrictive ? 0.0f : 50.0f;
    }

    void set_debug_restrictive(bool restrictive) { m_restrictive = restrictive; }
    bool is_debug_restrictive() const { return m_restrictive; }

private:
    bool m_restrictive;
};

/**
 * @class StubDemandProvider
 * @brief Permissive demand provider stub.
 *
 * Default: demand 1.0f (positive growth).
 * Debug restrictive: demand -1.0f (negative/shrinking).
 */
class StubDemandProvider : public IDemandProvider {
public:
    StubDemandProvider() : m_restrictive(false) {}

    float get_demand(std::uint8_t zone_type, std::uint32_t player_id) const override {
        (void)zone_type; (void)player_id;
        return m_restrictive ? -1.0f : 1.0f;
    }

    void set_debug_restrictive(bool restrictive) { m_restrictive = restrictive; }
    bool is_debug_restrictive() const { return m_restrictive; }

private:
    bool m_restrictive;
};

/**
 * @class StubCreditProvider
 * @brief Permissive credit provider stub.
 *
 * Default: deductions always succeed, credits always available.
 * Debug restrictive: deductions always fail, no credits.
 */
class StubCreditProvider : public ICreditProvider {
public:
    StubCreditProvider() : m_restrictive(false) {}

    bool deduct_credits(std::uint32_t player_id, std::int64_t amount) override {
        (void)player_id; (void)amount;
        return !m_restrictive;
    }

    bool has_credits(std::uint32_t player_id, std::int64_t amount) const override {
        (void)player_id; (void)amount;
        return !m_restrictive;
    }

    void set_debug_restrictive(bool restrictive) { m_restrictive = restrictive; }
    bool is_debug_restrictive() const { return m_restrictive; }

private:
    bool m_restrictive;
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_FORWARDDEPENDENCYSTUBS_H
