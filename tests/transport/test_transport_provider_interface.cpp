/**
 * @file test_transport_provider_interface.cpp
 * @brief Unit tests for ITransportProvider interface and StubTransportProvider (Epic 7, Ticket E7-016)
 *
 * Standalone tests using assert macros (no test framework).
 * Tests the extended ITransportProvider interface and its stub implementation.
 */

#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/building/ForwardDependencyStubs.h>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000::building;

// ============================================================================
// Helper: Concrete implementation for testing interface defaults
// ============================================================================

/**
 * Minimal concrete implementation that only implements the original
 * pure virtual methods. Used to verify that the new Epic 7 methods
 * have working default implementations.
 */
class MinimalTransportProvider : public ITransportProvider {
public:
    bool is_road_accessible_at(std::uint32_t x, std::uint32_t y, std::uint32_t max_distance) const override {
        (void)x; (void)y; (void)max_distance;
        return true;
    }

    std::uint32_t get_nearest_road_distance(std::uint32_t x, std::uint32_t y) const override {
        (void)x; (void)y;
        return 0;
    }
};

// ============================================================================
// ITransportProvider Default Implementation Tests
// ============================================================================

void test_interface_default_is_road_accessible() {
    printf("Testing ITransportProvider default is_road_accessible()...\n");

    MinimalTransportProvider provider;
    ITransportProvider* iface = &provider;

    // Default returns true (permissive)
    assert(iface->is_road_accessible(0) == true);
    assert(iface->is_road_accessible(12345) == true);
    assert(iface->is_road_accessible(UINT32_MAX) == true);

    printf("  PASS: default is_road_accessible() returns true\n");
}

void test_interface_default_is_connected_to_network() {
    printf("Testing ITransportProvider default is_connected_to_network()...\n");

    MinimalTransportProvider provider;
    ITransportProvider* iface = &provider;

    // Default returns true (permissive)
    assert(iface->is_connected_to_network(0, 0) == true);
    assert(iface->is_connected_to_network(100, 200) == true);
    assert(iface->is_connected_to_network(-1, -1) == true);

    printf("  PASS: default is_connected_to_network() returns true\n");
}

void test_interface_default_are_connected() {
    printf("Testing ITransportProvider default are_connected()...\n");

    MinimalTransportProvider provider;
    ITransportProvider* iface = &provider;

    // Default returns true (permissive)
    assert(iface->are_connected(0, 0, 10, 10) == true);
    assert(iface->are_connected(-5, -5, 5, 5) == true);

    printf("  PASS: default are_connected() returns true\n");
}

void test_interface_default_get_congestion_at() {
    printf("Testing ITransportProvider default get_congestion_at()...\n");

    MinimalTransportProvider provider;
    ITransportProvider* iface = &provider;

    // Default returns 0.0f (no congestion)
    assert(fabsf(iface->get_congestion_at(0, 0) - 0.0f) < 0.001f);
    assert(fabsf(iface->get_congestion_at(50, 50) - 0.0f) < 0.001f);

    printf("  PASS: default get_congestion_at() returns 0.0f\n");
}

void test_interface_default_get_traffic_volume_at() {
    printf("Testing ITransportProvider default get_traffic_volume_at()...\n");

    MinimalTransportProvider provider;
    ITransportProvider* iface = &provider;

    // Default returns 0 (no traffic)
    assert(iface->get_traffic_volume_at(0, 0) == 0);
    assert(iface->get_traffic_volume_at(100, 200) == 0);

    printf("  PASS: default get_traffic_volume_at() returns 0\n");
}

void test_interface_default_get_network_id_at() {
    printf("Testing ITransportProvider default get_network_id_at()...\n");

    MinimalTransportProvider provider;
    ITransportProvider* iface = &provider;

    // Default returns 0 (not in any network)
    assert(iface->get_network_id_at(0, 0) == 0);
    assert(iface->get_network_id_at(100, 200) == 0);

    printf("  PASS: default get_network_id_at() returns 0\n");
}

// ============================================================================
// StubTransportProvider Permissive Mode Tests
// ============================================================================

void test_stub_permissive_original_methods() {
    printf("Testing StubTransportProvider permissive original methods...\n");

    StubTransportProvider stub;

    assert(stub.is_road_accessible_at(10, 20, 3) == true);
    assert(stub.get_nearest_road_distance(10, 20) == 0);
    assert(stub.is_debug_restrictive() == false);

    printf("  PASS: Stub permissive original methods work\n");
}

void test_stub_permissive_extended_methods() {
    printf("Testing StubTransportProvider permissive extended methods...\n");

    StubTransportProvider stub;

    assert(stub.is_road_accessible(42) == true);
    assert(stub.is_connected_to_network(5, 5) == true);
    assert(stub.are_connected(0, 0, 10, 10) == true);
    assert(fabsf(stub.get_congestion_at(5, 5) - 0.0f) < 0.001f);
    assert(stub.get_traffic_volume_at(5, 5) == 0);
    assert(stub.get_network_id_at(5, 5) == 1);

    printf("  PASS: Stub permissive extended methods work\n");
}

// ============================================================================
// StubTransportProvider Restrictive Mode Tests
// ============================================================================

void test_stub_restrictive_original_methods() {
    printf("Testing StubTransportProvider restrictive original methods...\n");

    StubTransportProvider stub;
    stub.set_debug_restrictive(true);

    assert(stub.is_road_accessible_at(10, 20, 3) == false);
    assert(stub.get_nearest_road_distance(10, 20) == 255);
    assert(stub.is_debug_restrictive() == true);

    printf("  PASS: Stub restrictive original methods work\n");
}

void test_stub_restrictive_extended_methods() {
    printf("Testing StubTransportProvider restrictive extended methods...\n");

    StubTransportProvider stub;
    stub.set_debug_restrictive(true);

    assert(stub.is_road_accessible(42) == false);
    assert(stub.is_connected_to_network(5, 5) == false);
    assert(stub.are_connected(0, 0, 10, 10) == false);
    assert(fabsf(stub.get_congestion_at(5, 5) - 1.0f) < 0.001f);
    assert(stub.get_traffic_volume_at(5, 5) == 1000);
    assert(stub.get_network_id_at(5, 5) == 0);

    printf("  PASS: Stub restrictive extended methods work\n");
}

// ============================================================================
// StubTransportProvider Toggle Mode Test
// ============================================================================

void test_stub_toggle_mode() {
    printf("Testing StubTransportProvider mode toggle...\n");

    StubTransportProvider stub;

    // Start permissive
    assert(stub.is_road_accessible(1) == true);
    assert(stub.is_connected_to_network(0, 0) == true);
    assert(stub.get_network_id_at(0, 0) == 1);

    // Switch to restrictive
    stub.set_debug_restrictive(true);
    assert(stub.is_road_accessible(1) == false);
    assert(stub.is_connected_to_network(0, 0) == false);
    assert(stub.get_network_id_at(0, 0) == 0);

    // Switch back to permissive
    stub.set_debug_restrictive(false);
    assert(stub.is_road_accessible(1) == true);
    assert(stub.is_connected_to_network(0, 0) == true);
    assert(stub.get_network_id_at(0, 0) == 1);

    printf("  PASS: Mode toggle works correctly\n");
}

// ============================================================================
// Polymorphic Usage Test
// ============================================================================

void test_stub_polymorphic_usage() {
    printf("Testing StubTransportProvider through ITransportProvider pointer...\n");

    StubTransportProvider stub;
    ITransportProvider* iface = &stub;

    // Original methods
    assert(iface->is_road_accessible_at(10, 20, 3) == true);
    assert(iface->get_nearest_road_distance(10, 20) == 0);

    // Extended methods
    assert(iface->is_road_accessible(42) == true);
    assert(iface->is_connected_to_network(5, 5) == true);
    assert(iface->are_connected(0, 0, 10, 10) == true);
    assert(fabsf(iface->get_congestion_at(5, 5) - 0.0f) < 0.001f);
    assert(iface->get_traffic_volume_at(5, 5) == 0);
    assert(iface->get_network_id_at(5, 5) == 1);

    printf("  PASS: Polymorphic usage through ITransportProvider* works\n");
}

void test_interface_virtual_destructor() {
    printf("Testing ITransportProvider virtual destructor via delete...\n");

    ITransportProvider* p = new StubTransportProvider();
    delete p; // Should not leak or crash

    printf("  PASS: Virtual destructor works correctly\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== ITransportProvider Interface Tests (Epic 7, Ticket E7-016) ===\n\n");

    // Interface default implementation tests
    test_interface_default_is_road_accessible();
    test_interface_default_is_connected_to_network();
    test_interface_default_are_connected();
    test_interface_default_get_congestion_at();
    test_interface_default_get_traffic_volume_at();
    test_interface_default_get_network_id_at();

    // StubTransportProvider permissive mode
    test_stub_permissive_original_methods();
    test_stub_permissive_extended_methods();

    // StubTransportProvider restrictive mode
    test_stub_restrictive_original_methods();
    test_stub_restrictive_extended_methods();

    // Toggle
    test_stub_toggle_mode();

    // Polymorphic usage
    test_stub_polymorphic_usage();
    test_interface_virtual_destructor();

    printf("\n=== All ITransportProvider Interface Tests Passed ===\n");
    return 0;
}
