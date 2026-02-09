/**
 * @file test_demand_provider_extended.cpp
 * @brief Tests for IDemandProvider extension methods (E10-041)
 *
 * Validates:
 * - get_demand_cap() default returns 0
 * - has_positive_demand() default delegates to get_demand()
 * - StubDemandProvider overrides in permissive and restrictive modes
 */

#include <cassert>
#include <cstdio>
#include <cmath>

#include "sims3000/building/ForwardDependencyInterfaces.h"
#include "sims3000/building/ForwardDependencyStubs.h"

using namespace sims3000::building;

// --------------------------------------------------------------------------
// Concrete implementation for testing defaults
// --------------------------------------------------------------------------

/// Minimal IDemandProvider that only implements get_demand() (the pure virtual).
/// Used to verify the default implementations of get_demand_cap and
/// has_positive_demand work correctly when not overridden.
class MinimalDemandProvider : public IDemandProvider {
public:
    explicit MinimalDemandProvider(float demand_value)
        : m_demand(demand_value) {}

    float get_demand(std::uint8_t /*zone_type*/, std::uint32_t /*player_id*/) const override {
        return m_demand;
    }

private:
    float m_demand;
};

// --------------------------------------------------------------------------
// Test: get_demand_cap default returns 0
// --------------------------------------------------------------------------
static void test_get_demand_cap_default_returns_zero() {
    MinimalDemandProvider provider(5.0f);
    std::uint32_t cap = provider.get_demand_cap(0, 0);
    assert(cap == 0 && "Default get_demand_cap should return 0");

    // Test with different zone/player values
    cap = provider.get_demand_cap(1, 3);
    assert(cap == 0 && "Default get_demand_cap should return 0 for any zone/player");

    std::printf("  PASS: get_demand_cap default returns 0\n");
}

// --------------------------------------------------------------------------
// Test: has_positive_demand default delegates to get_demand
// --------------------------------------------------------------------------
static void test_has_positive_demand_default_delegates() {
    // Positive demand -> has_positive_demand should return true
    {
        MinimalDemandProvider provider(10.0f);
        assert(provider.has_positive_demand(0, 0) == true);
    }

    // Negative demand -> has_positive_demand should return false
    {
        MinimalDemandProvider provider(-5.0f);
        assert(provider.has_positive_demand(0, 0) == false);
    }

    // Zero demand -> has_positive_demand should return false (> 0, not >= 0)
    {
        MinimalDemandProvider provider(0.0f);
        assert(provider.has_positive_demand(0, 0) == false);
    }

    // Small positive demand
    {
        MinimalDemandProvider provider(0.001f);
        assert(provider.has_positive_demand(0, 0) == true);
    }

    std::printf("  PASS: has_positive_demand default delegates to get_demand\n");
}

// --------------------------------------------------------------------------
// Test: StubDemandProvider overrides (permissive mode)
// --------------------------------------------------------------------------
static void test_stub_permissive_mode() {
    StubDemandProvider stub;

    // get_demand: permissive returns 1.0f
    float demand = stub.get_demand(0, 0);
    assert(std::fabs(demand - 1.0f) < 0.001f && "Permissive get_demand should return 1.0f");

    // get_demand_cap: permissive returns 10000
    std::uint32_t cap = stub.get_demand_cap(0, 0);
    assert(cap == 10000 && "Permissive get_demand_cap should return 10000");

    // has_positive_demand: permissive returns true
    bool positive = stub.has_positive_demand(0, 0);
    assert(positive == true && "Permissive has_positive_demand should return true");

    // Test across different zone types
    for (std::uint8_t zone = 0; zone < 3; ++zone) {
        assert(stub.get_demand_cap(zone, 0) == 10000);
        assert(stub.has_positive_demand(zone, 0) == true);
    }

    std::printf("  PASS: StubDemandProvider permissive mode\n");
}

// --------------------------------------------------------------------------
// Test: StubDemandProvider overrides (restrictive mode)
// --------------------------------------------------------------------------
static void test_stub_restrictive_mode() {
    StubDemandProvider stub;
    stub.set_debug_restrictive(true);

    // get_demand: restrictive returns -1.0f
    float demand = stub.get_demand(0, 0);
    assert(std::fabs(demand - (-1.0f)) < 0.001f && "Restrictive get_demand should return -1.0f");

    // get_demand_cap: restrictive returns 0
    std::uint32_t cap = stub.get_demand_cap(0, 0);
    assert(cap == 0 && "Restrictive get_demand_cap should return 0");

    // has_positive_demand: restrictive returns false
    bool positive = stub.has_positive_demand(0, 0);
    assert(positive == false && "Restrictive has_positive_demand should return false");

    std::printf("  PASS: StubDemandProvider restrictive mode\n");
}

// --------------------------------------------------------------------------
// Test: StubDemandProvider mode toggling
// --------------------------------------------------------------------------
static void test_stub_mode_toggle() {
    StubDemandProvider stub;

    // Start permissive
    assert(stub.has_positive_demand(0, 0) == true);
    assert(stub.get_demand_cap(0, 0) == 10000);

    // Switch to restrictive
    stub.set_debug_restrictive(true);
    assert(stub.has_positive_demand(0, 0) == false);
    assert(stub.get_demand_cap(0, 0) == 0);

    // Switch back to permissive
    stub.set_debug_restrictive(false);
    assert(stub.has_positive_demand(0, 0) == true);
    assert(stub.get_demand_cap(0, 0) == 10000);

    std::printf("  PASS: StubDemandProvider mode toggle\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== IDemandProvider Extended Tests (E10-041) ===\n");

    test_get_demand_cap_default_returns_zero();
    test_has_positive_demand_default_delegates();
    test_stub_permissive_mode();
    test_stub_restrictive_mode();
    test_stub_mode_toggle();

    std::printf("All IDemandProvider extended tests passed.\n");
    return 0;
}
