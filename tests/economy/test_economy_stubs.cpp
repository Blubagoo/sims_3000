/**
 * @file test_economy_stubs.cpp
 * @brief Unit tests for IEconomyQueryable and StubEconomyQueryable (E10-113)
 */

#include "sims3000/economy/IEconomyQueryable.h"
#include "sims3000/economy/StubEconomyQueryable.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>

using namespace sims3000::economy;

void test_stub_tribute_rate() {
    printf("Testing StubEconomyQueryable tribute rates...\n");

    StubEconomyQueryable stub;

    // All zone types should return 7.0f
    assert(std::fabs(stub.get_tribute_rate(0) - 7.0f) < 0.001f);
    assert(std::fabs(stub.get_tribute_rate(1) - 7.0f) < 0.001f);
    assert(std::fabs(stub.get_tribute_rate(2) - 7.0f) < 0.001f);

    // Even arbitrary zone type values
    assert(std::fabs(stub.get_tribute_rate(255) - 7.0f) < 0.001f);

    printf("  PASS: get_tribute_rate returns 7.0f for all zone types\n");
}

void test_stub_average_tribute_rate() {
    printf("Testing StubEconomyQueryable average tribute rate...\n");

    StubEconomyQueryable stub;

    assert(std::fabs(stub.get_average_tribute_rate() - 7.0f) < 0.001f);

    printf("  PASS: get_average_tribute_rate returns 7.0f\n");
}

void test_interface_via_base_pointer() {
    printf("Testing IEconomyQueryable via base pointer...\n");

    std::unique_ptr<IEconomyQueryable> economy =
        std::make_unique<StubEconomyQueryable>();

    // Should work polymorphically through the interface
    assert(std::fabs(economy->get_tribute_rate(0) - 7.0f) < 0.001f);
    assert(std::fabs(economy->get_tribute_rate(1) - 7.0f) < 0.001f);
    assert(std::fabs(economy->get_tribute_rate(2) - 7.0f) < 0.001f);
    assert(std::fabs(economy->get_average_tribute_rate() - 7.0f) < 0.001f);

    printf("  PASS: IEconomyQueryable works via base pointer\n");
}

void test_interface_raw_pointer() {
    printf("Testing IEconomyQueryable via raw base pointer...\n");

    StubEconomyQueryable stub;
    IEconomyQueryable* base = &stub;

    assert(std::fabs(base->get_tribute_rate(0) - 7.0f) < 0.001f);
    assert(std::fabs(base->get_average_tribute_rate() - 7.0f) < 0.001f);

    printf("  PASS: IEconomyQueryable works via raw base pointer\n");
}

int main() {
    printf("=== Economy Stubs Unit Tests (E10-113) ===\n\n");

    test_stub_tribute_rate();
    test_stub_average_tribute_rate();
    test_interface_via_base_pointer();
    test_interface_raw_pointer();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
