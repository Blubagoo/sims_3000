/**
 * @file test_debris_component.cpp
 * @brief Unit tests for DebrisComponent (Epic 4, Ticket 4-005)
 */

#include <sims3000/building/BuildingComponents.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::building;

void test_debris_component_size() {
    printf("Testing DebrisComponent size...\n");

    assert(sizeof(DebrisComponent) == 8);
    assert(std::is_trivially_copyable<DebrisComponent>::value);

    printf("  PASS: DebrisComponent is 8 bytes and trivially copyable\n");
}

void test_default_initialization() {
    printf("Testing default initialization...\n");

    DebrisComponent debris;
    assert(debris.original_template_id == 0);
    assert(debris.clear_timer == DebrisComponent::DEFAULT_CLEAR_TIMER);
    assert(debris.footprint_w == 1);
    assert(debris.footprint_h == 1);
    assert(DebrisComponent::DEFAULT_CLEAR_TIMER == 60);

    printf("  PASS: Default initialization works correctly\n");
}

void test_custom_initialization() {
    printf("Testing custom initialization...\n");

    DebrisComponent debris(12345, 2, 3, 120);
    assert(debris.original_template_id == 12345);
    assert(debris.clear_timer == 120);
    assert(debris.footprint_w == 2);
    assert(debris.footprint_h == 3);

    // Test with default timer
    DebrisComponent debris2(999, 4, 4);
    assert(debris2.original_template_id == 999);
    assert(debris2.clear_timer == DebrisComponent::DEFAULT_CLEAR_TIMER);
    assert(debris2.footprint_w == 4);
    assert(debris2.footprint_h == 4);

    printf("  PASS: Custom initialization works correctly\n");
}

void test_timer_mechanics() {
    printf("Testing timer mechanics...\n");

    DebrisComponent debris(123, 1, 1, 3);
    assert(!debris.isExpired());

    debris.tick(); // 3 -> 2
    assert(!debris.isExpired());

    debris.tick(); // 2 -> 1
    assert(!debris.isExpired());

    debris.tick(); // 1 -> 0
    assert(debris.isExpired());

    // Tick again - should stay at 0
    debris.tick();
    assert(debris.isExpired());
    assert(debris.clear_timer == 0);

    printf("  PASS: Timer mechanics work correctly\n");
}

int main() {
    printf("=== DebrisComponent Unit Tests (Epic 4, Ticket 4-005) ===\n\n");

    test_debris_component_size();
    test_default_initialization();
    test_custom_initialization();
    test_timer_mechanics();

    printf("\n=== All DebrisComponent Tests Passed ===\n");
    return 0;
}
