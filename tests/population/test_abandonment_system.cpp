#include <sims3000/population/AbandonmentSystem.h>
#include <cstdio>
#include <cassert>

using namespace sims3000::population;

void test_should_start_abandonment() {
    printf("Testing should_start_abandonment...\n");

    // Test: Normal conditions - should NOT start
    assert(!should_start_abandonment(50.0f, 50.0f, true, true));
    printf("  [PASS] Normal conditions do not trigger abandonment\n");

    // Test: Low demand - should start
    assert(should_start_abandonment(-60.0f, 50.0f, true, true));
    printf("  [PASS] Low demand triggers abandonment\n");

    // Test: High disorder - should start
    assert(should_start_abandonment(50.0f, 250.0f, true, true));
    printf("  [PASS] High disorder triggers abandonment\n");

    // Test: No utilities - should start
    assert(should_start_abandonment(50.0f, 50.0f, false, false));
    printf("  [PASS] No utilities triggers abandonment\n");

    // Test: Has power but no fluid - should NOT start (has at least one utility)
    assert(!should_start_abandonment(50.0f, 50.0f, true, false));
    printf("  [PASS] Having power prevents utility-based abandonment\n");

    // Test: Has fluid but no power - should NOT start
    assert(!should_start_abandonment(50.0f, 50.0f, false, true));
    printf("  [PASS] Having fluid prevents utility-based abandonment\n");

    // Test: Edge case - exactly at demand threshold
    assert(!should_start_abandonment(-50.0f, 50.0f, true, true));
    printf("  [PASS] Exactly at demand threshold does not trigger\n");

    // Test: Edge case - exactly at disorder threshold
    assert(!should_start_abandonment(50.0f, 200.0f, true, true));
    printf("  [PASS] Exactly at disorder threshold does not trigger\n");
}

void test_update_abandonment() {
    printf("\nTesting update_abandonment...\n");

    // Test: Counter increments with bad conditions
    AbandonmentCandidate candidate = {1, 0, AbandonmentReason::None};
    bool should_abandon = update_abandonment(candidate, -60.0f, 50.0f, true, true);
    assert(candidate.ticks_negative == 1);
    assert(candidate.reason == AbandonmentReason::LowDemand);
    assert(!should_abandon);
    printf("  [PASS] Counter increments with bad conditions\n");

    // Test: Counter resets with good conditions
    candidate = {1, 50, AbandonmentReason::LowDemand};
    should_abandon = update_abandonment(candidate, 50.0f, 50.0f, true, true);
    assert(candidate.ticks_negative == 0);
    assert(candidate.reason == AbandonmentReason::None);
    assert(!should_abandon);
    printf("  [PASS] Counter resets with good conditions\n");

    // Test: Reaches threshold
    candidate = {1, 199, AbandonmentReason::LowDemand};
    should_abandon = update_abandonment(candidate, -60.0f, 50.0f, true, true);
    assert(candidate.ticks_negative == 200);
    assert(should_abandon);
    printf("  [PASS] Reaches threshold at 200 ticks\n");

    // Test: High disorder reason
    candidate = {1, 0, AbandonmentReason::None};
    update_abandonment(candidate, 50.0f, 250.0f, true, true);
    assert(candidate.reason == AbandonmentReason::HighDisorder);
    printf("  [PASS] High disorder reason set correctly\n");

    // Test: No utilities reason
    candidate = {1, 0, AbandonmentReason::None};
    update_abandonment(candidate, 50.0f, 50.0f, false, false);
    assert(candidate.reason == AbandonmentReason::NoUtilities);
    printf("  [PASS] No utilities reason set correctly\n");

    // Test: Combined reason (multiple conditions)
    candidate = {1, 0, AbandonmentReason::None};
    update_abandonment(candidate, -60.0f, 250.0f, true, true);
    assert(candidate.reason == AbandonmentReason::Combined);
    printf("  [PASS] Combined reason set for multiple conditions\n");

    // Test: Combined reason (all three conditions)
    candidate = {1, 0, AbandonmentReason::None};
    update_abandonment(candidate, -60.0f, 250.0f, false, false);
    assert(candidate.reason == AbandonmentReason::Combined);
    printf("  [PASS] Combined reason set for all conditions\n");
}

void test_get_abandonment_reason_text() {
    printf("\nTesting get_abandonment_reason_text...\n");

    assert(get_abandonment_reason_text(AbandonmentReason::None) != nullptr);
    assert(get_abandonment_reason_text(AbandonmentReason::LowDemand) != nullptr);
    assert(get_abandonment_reason_text(AbandonmentReason::HighDisorder) != nullptr);
    assert(get_abandonment_reason_text(AbandonmentReason::NoUtilities) != nullptr);
    assert(get_abandonment_reason_text(AbandonmentReason::Combined) != nullptr);

    printf("  [PASS] All reason texts are non-null\n");
    printf("  Reason texts:\n");
    printf("    None: %s\n", get_abandonment_reason_text(AbandonmentReason::None));
    printf("    LowDemand: %s\n", get_abandonment_reason_text(AbandonmentReason::LowDemand));
    printf("    HighDisorder: %s\n", get_abandonment_reason_text(AbandonmentReason::HighDisorder));
    printf("    NoUtilities: %s\n", get_abandonment_reason_text(AbandonmentReason::NoUtilities));
    printf("    Combined: %s\n", get_abandonment_reason_text(AbandonmentReason::Combined));
}

void test_abandonment_simulation() {
    printf("\nTesting full abandonment simulation...\n");

    AbandonmentCandidate candidate = {42, 0, AbandonmentReason::None};

    // Simulate 199 ticks of bad conditions
    for (uint32_t i = 0; i < 199; i++) {
        bool should_abandon = update_abandonment(candidate, -60.0f, 50.0f, true, true);
        assert(!should_abandon);
    }
    assert(candidate.ticks_negative == 199);
    printf("  [PASS] Building survives 199 ticks of bad conditions\n");

    // 200th tick should trigger abandonment
    bool should_abandon = update_abandonment(candidate, -60.0f, 50.0f, true, true);
    assert(should_abandon);
    assert(candidate.ticks_negative == 200);
    printf("  [PASS] Building abandons at 200 ticks\n");

    // Test recovery scenario
    candidate = {43, 150, AbandonmentReason::LowDemand};
    should_abandon = update_abandonment(candidate, 50.0f, 50.0f, true, true);
    assert(!should_abandon);
    assert(candidate.ticks_negative == 0);
    assert(candidate.reason == AbandonmentReason::None);
    printf("  [PASS] Building recovers when conditions improve\n");

    // Test oscillating conditions
    candidate = {44, 0, AbandonmentReason::None};
    for (int i = 0; i < 10; i++) {
        // Bad for 10 ticks
        for (int j = 0; j < 10; j++) {
            update_abandonment(candidate, -60.0f, 50.0f, true, true);
        }
        // Good for 1 tick (resets counter)
        update_abandonment(candidate, 50.0f, 50.0f, true, true);
    }
    assert(candidate.ticks_negative == 0);
    printf("  [PASS] Oscillating conditions reset counter\n");
}

int main() {
    printf("=== AbandonmentSystem Test Suite ===\n\n");

    test_should_start_abandonment();
    test_update_abandonment();
    test_get_abandonment_reason_text();
    test_abandonment_simulation();

    printf("\n=== All AbandonmentSystem tests passed! ===\n");
    return 0;
}
