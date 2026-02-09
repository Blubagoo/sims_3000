/**
 * @file test_service_queryable.cpp
 * @brief Unit tests for IServiceQueryable interface and StubServiceQueryable
 *        (Epic 9, Ticket E9-004)
 *
 * Tests cover:
 * - IServiceQueryable interface via StubServiceQueryable
 * - Stub defaults return 0.0f (not 0.5f) for all methods
 * - Virtual destructor / polymorphic usage
 * - All service type values via uint8_t casting
 * - Debug restrictive mode (same as default for opt-in infrastructure)
 */

#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/building/ForwardDependencyStubs.h>
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace sims3000::building;

// Helper for float comparison
static bool float_eq(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

// =============================================================================
// Stub default tests
// =============================================================================

void test_stub_get_coverage_returns_zero() {
    printf("Testing StubServiceQueryable::get_coverage returns 0.0f...\n");

    StubServiceQueryable stub;
    for (uint8_t st = 0; st < 4; ++st) {
        float cov = stub.get_coverage(st, 0);
        assert(float_eq(cov, 0.0f));
    }

    // Different player IDs
    assert(float_eq(stub.get_coverage(0, 0), 0.0f));
    assert(float_eq(stub.get_coverage(0, 1), 0.0f));
    assert(float_eq(stub.get_coverage(0, 3), 0.0f));

    printf("  PASS: get_coverage returns 0.0f for all inputs\n");
}

void test_stub_get_coverage_at_returns_zero() {
    printf("Testing StubServiceQueryable::get_coverage_at returns 0.0f...\n");

    StubServiceQueryable stub;
    assert(float_eq(stub.get_coverage_at(0, 0, 0, 0), 0.0f));
    assert(float_eq(stub.get_coverage_at(1, 50, 50, 0), 0.0f));
    assert(float_eq(stub.get_coverage_at(2, 127, 255, 1), 0.0f));
    assert(float_eq(stub.get_coverage_at(3, -1, -1, 3), 0.0f));

    printf("  PASS: get_coverage_at returns 0.0f for all inputs\n");
}

void test_stub_get_effectiveness_returns_zero() {
    printf("Testing StubServiceQueryable::get_effectiveness returns 0.0f...\n");

    StubServiceQueryable stub;
    for (uint8_t st = 0; st < 4; ++st) {
        float eff = stub.get_effectiveness(st, 0);
        assert(float_eq(eff, 0.0f));
    }

    // Different player IDs
    assert(float_eq(stub.get_effectiveness(0, 0), 0.0f));
    assert(float_eq(stub.get_effectiveness(1, 1), 0.0f));
    assert(float_eq(stub.get_effectiveness(3, 3), 0.0f));

    printf("  PASS: get_effectiveness returns 0.0f for all inputs\n");
}

// =============================================================================
// Polymorphic usage via base pointer
// =============================================================================

void test_interface_via_base_pointer() {
    printf("Testing IServiceQueryable via base pointer...\n");

    StubServiceQueryable stub;
    IServiceQueryable* iface = &stub;

    assert(float_eq(iface->get_coverage(0, 0), 0.0f));
    assert(float_eq(iface->get_coverage_at(0, 10, 10, 0), 0.0f));
    assert(float_eq(iface->get_effectiveness(0, 0), 0.0f));

    printf("  PASS: Interface works via base pointer\n");
}

// =============================================================================
// Debug restrictive mode
// =============================================================================

void test_debug_restrictive_mode() {
    printf("Testing StubServiceQueryable debug restrictive mode...\n");

    StubServiceQueryable stub;
    assert(!stub.is_debug_restrictive());

    stub.set_debug_restrictive(true);
    assert(stub.is_debug_restrictive());

    // For opt-in infrastructure, restrictive == default (both return 0.0f)
    assert(float_eq(stub.get_coverage(0, 0), 0.0f));
    assert(float_eq(stub.get_coverage_at(0, 0, 0, 0), 0.0f));
    assert(float_eq(stub.get_effectiveness(0, 0), 0.0f));

    stub.set_debug_restrictive(false);
    assert(!stub.is_debug_restrictive());

    printf("  PASS: Debug restrictive mode works correctly\n");
}

// =============================================================================
// Virtual destructor test
// =============================================================================

void test_virtual_destructor() {
    printf("Testing IServiceQueryable virtual destructor...\n");

    // Allocate via base pointer, delete via base - must not leak
    IServiceQueryable* iface = new StubServiceQueryable();
    assert(float_eq(iface->get_coverage(0, 0), 0.0f));
    delete iface;

    printf("  PASS: Virtual destructor works correctly\n");
}

// =============================================================================
// Not 0.5f verification (explicit per acceptance criteria)
// =============================================================================

void test_defaults_not_half() {
    printf("Testing stub defaults are 0.0f, NOT 0.5f...\n");

    StubServiceQueryable stub;

    // Acceptance criteria: Stub fallback returns 0.0f (not 0.5f)
    assert(!float_eq(stub.get_coverage(0, 0), 0.5f));
    assert(!float_eq(stub.get_coverage_at(0, 0, 0, 0), 0.5f));
    assert(!float_eq(stub.get_effectiveness(0, 0), 0.5f));

    assert(float_eq(stub.get_coverage(0, 0), 0.0f));
    assert(float_eq(stub.get_coverage_at(0, 0, 0, 0), 0.0f));
    assert(float_eq(stub.get_effectiveness(0, 0), 0.0f));

    printf("  PASS: Defaults are 0.0f, not 0.5f\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== IServiceQueryable & StubServiceQueryable Tests (E9-004) ===\n\n");

    test_stub_get_coverage_returns_zero();
    test_stub_get_coverage_at_returns_zero();
    test_stub_get_effectiveness_returns_zero();
    test_interface_via_base_pointer();
    test_debug_restrictive_mode();
    test_virtual_destructor();
    test_defaults_not_half();

    printf("\n=== All IServiceQueryable Tests Passed ===\n");
    return 0;
}
