/**
 * @file ForwardDependencyStubs.h
 * @brief Stub implementations of all 6 forward dependency interfaces
 *
 * Provides permissive default implementations for testing and development
 * before real systems are available. Each stub has a debug_restrictive mode
 * that returns false/0/negative values to test failure paths.
 *
 * Stubs:
 * - StubEnergyProvider: is_powered() -> true
 * - StubFluidProvider: has_fluid() -> true
 * - StubTransportProvider: is_road_accessible_at() -> true
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
 * Default: all positions are road-accessible, distance 0.
 * Debug restrictive: nothing is accessible, distance 255.
 */
class StubTransportProvider : public ITransportProvider {
public:
    StubTransportProvider() : m_restrictive(false) {}

    bool is_road_accessible_at(std::uint32_t x, std::uint32_t y, std::uint32_t max_distance) const override {
        (void)x; (void)y; (void)max_distance;
        return !m_restrictive;
    }

    std::uint32_t get_nearest_road_distance(std::uint32_t x, std::uint32_t y) const override {
        (void)x; (void)y;
        return m_restrictive ? 255 : 0;
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
