/**
 * @file test_neighbor_relationship.cpp
 * @brief Unit tests for Neighbor Relationship Evolution (Epic 8, Ticket E8-034)
 *
 * Tests cover:
 * - Default initialization values
 * - Relationship status classification at all thresholds
 * - Max available tier mapping at all thresholds
 * - update_relationship with positive/negative points
 * - Clamping at min/max boundaries (-100, +100)
 * - record_trade updates trade history and relationship
 * - Tier recalculation after relationship changes
 * - Hostile relationship blocks all tiers
 * - Allied relationship unlocks Premium tier
 * - Edge cases and boundary values
 */

#include <sims3000/port/NeighborRelationship.h>
#include <sims3000/port/PortTypes.h>
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace sims3000::port;

// =============================================================================
// Test: Default initialization
// =============================================================================

void test_default_initialization() {
    printf("Testing default initialization...\n");

    NeighborRelationship rel{};
    assert(rel.neighbor_id == 0);
    assert(rel.relationship_value == 0);
    assert(rel.total_trades == 0);
    assert(rel.total_trade_volume == 0);
    assert(rel.max_available_tier == TradeAgreementType::Basic);

    printf("  PASS: Default initialization values are correct\n");
}

// =============================================================================
// Test: Custom initialization
// =============================================================================

void test_custom_initialization() {
    printf("Testing custom initialization...\n");

    NeighborRelationship rel{};
    rel.neighbor_id = 3;
    rel.relationship_value = 50;
    rel.total_trades = 10;
    rel.total_trade_volume = 50000;
    rel.max_available_tier = TradeAgreementType::Enhanced;

    assert(rel.neighbor_id == 3);
    assert(rel.relationship_value == 50);
    assert(rel.total_trades == 10);
    assert(rel.total_trade_volume == 50000);
    assert(rel.max_available_tier == TradeAgreementType::Enhanced);

    printf("  PASS: Custom initialization works correctly\n");
}

// =============================================================================
// Test: Relationship status classification
// =============================================================================

void test_relationship_status_hostile() {
    printf("Testing relationship status: Hostile...\n");

    assert(get_relationship_status(-100) == RelationshipStatus::Hostile);
    assert(get_relationship_status(-51) == RelationshipStatus::Hostile);
    assert(get_relationship_status(-75) == RelationshipStatus::Hostile);

    printf("  PASS: Hostile status for values < -50\n");
}

void test_relationship_status_cold() {
    printf("Testing relationship status: Cold...\n");

    assert(get_relationship_status(-50) == RelationshipStatus::Cold);
    assert(get_relationship_status(-1) == RelationshipStatus::Cold);
    assert(get_relationship_status(-25) == RelationshipStatus::Cold);

    printf("  PASS: Cold status for values -50 to -1\n");
}

void test_relationship_status_neutral() {
    printf("Testing relationship status: Neutral...\n");

    assert(get_relationship_status(0) == RelationshipStatus::Neutral);
    assert(get_relationship_status(24) == RelationshipStatus::Neutral);
    assert(get_relationship_status(12) == RelationshipStatus::Neutral);

    printf("  PASS: Neutral status for values 0 to 24\n");
}

void test_relationship_status_warm() {
    printf("Testing relationship status: Warm...\n");

    assert(get_relationship_status(25) == RelationshipStatus::Warm);
    assert(get_relationship_status(49) == RelationshipStatus::Warm);
    assert(get_relationship_status(37) == RelationshipStatus::Warm);

    printf("  PASS: Warm status for values 25 to 49\n");
}

void test_relationship_status_friendly() {
    printf("Testing relationship status: Friendly...\n");

    assert(get_relationship_status(50) == RelationshipStatus::Friendly);
    assert(get_relationship_status(79) == RelationshipStatus::Friendly);
    assert(get_relationship_status(65) == RelationshipStatus::Friendly);

    printf("  PASS: Friendly status for values 50 to 79\n");
}

void test_relationship_status_allied() {
    printf("Testing relationship status: Allied...\n");

    assert(get_relationship_status(80) == RelationshipStatus::Allied);
    assert(get_relationship_status(100) == RelationshipStatus::Allied);
    assert(get_relationship_status(90) == RelationshipStatus::Allied);

    printf("  PASS: Allied status for values 80+\n");
}

// =============================================================================
// Test: Max available tier mapping
// =============================================================================

void test_max_tier_hostile() {
    printf("Testing max available tier: Hostile -> None...\n");

    assert(get_max_available_tier(-100) == TradeAgreementType::None);
    assert(get_max_available_tier(-51) == TradeAgreementType::None);
    assert(get_max_available_tier(-75) == TradeAgreementType::None);

    printf("  PASS: Hostile relationship allows no trade tiers\n");
}

void test_max_tier_cold() {
    printf("Testing max available tier: Cold -> Basic...\n");

    assert(get_max_available_tier(-50) == TradeAgreementType::Basic);
    assert(get_max_available_tier(-1) == TradeAgreementType::Basic);

    printf("  PASS: Cold relationship allows Basic tier\n");
}

void test_max_tier_neutral() {
    printf("Testing max available tier: Neutral -> Basic...\n");

    assert(get_max_available_tier(0) == TradeAgreementType::Basic);
    assert(get_max_available_tier(24) == TradeAgreementType::Basic);

    printf("  PASS: Neutral relationship allows Basic tier\n");
}

void test_max_tier_warm() {
    printf("Testing max available tier: Warm -> Enhanced...\n");

    assert(get_max_available_tier(25) == TradeAgreementType::Enhanced);
    assert(get_max_available_tier(49) == TradeAgreementType::Enhanced);

    printf("  PASS: Warm relationship allows Enhanced tier\n");
}

void test_max_tier_friendly() {
    printf("Testing max available tier: Friendly -> Enhanced...\n");

    assert(get_max_available_tier(50) == TradeAgreementType::Enhanced);
    assert(get_max_available_tier(79) == TradeAgreementType::Enhanced);

    printf("  PASS: Friendly relationship allows Enhanced tier\n");
}

void test_max_tier_allied() {
    printf("Testing max available tier: Allied -> Premium...\n");

    assert(get_max_available_tier(80) == TradeAgreementType::Premium);
    assert(get_max_available_tier(100) == TradeAgreementType::Premium);

    printf("  PASS: Allied relationship allows Premium tier\n");
}

// =============================================================================
// Test: update_relationship with positive points
// =============================================================================

void test_update_positive_points() {
    printf("Testing update_relationship with positive points...\n");

    NeighborRelationship rel{};
    rel.neighbor_id = 1;
    rel.relationship_value = 0;

    update_relationship(rel, 10);
    assert(rel.relationship_value == 10);
    assert(rel.max_available_tier == TradeAgreementType::Basic);  // Still Neutral

    update_relationship(rel, 15);
    assert(rel.relationship_value == 25);
    assert(rel.max_available_tier == TradeAgreementType::Enhanced);  // Now Warm

    update_relationship(rel, 55);
    assert(rel.relationship_value == 80);
    assert(rel.max_available_tier == TradeAgreementType::Premium);  // Now Allied

    printf("  PASS: Positive points increase relationship and update tier\n");
}

// =============================================================================
// Test: update_relationship with negative points
// =============================================================================

void test_update_negative_points() {
    printf("Testing update_relationship with negative points...\n");

    NeighborRelationship rel{};
    rel.neighbor_id = 2;
    rel.relationship_value = 50;  // Friendly
    rel.max_available_tier = TradeAgreementType::Enhanced;

    update_relationship(rel, -30);
    assert(rel.relationship_value == 20);
    assert(rel.max_available_tier == TradeAgreementType::Basic);  // Back to Neutral

    update_relationship(rel, -80);
    assert(rel.relationship_value == -60);
    assert(rel.max_available_tier == TradeAgreementType::None);  // Hostile

    printf("  PASS: Negative points decrease relationship and update tier\n");
}

// =============================================================================
// Test: Clamping at maximum (+100)
// =============================================================================

void test_clamp_max() {
    printf("Testing clamping at maximum (+100)...\n");

    NeighborRelationship rel{};
    rel.neighbor_id = 1;
    rel.relationship_value = 90;

    update_relationship(rel, 50);
    assert(rel.relationship_value == 100);  // Clamped to max
    assert(rel.max_available_tier == TradeAgreementType::Premium);

    // Adding more still stays at 100
    update_relationship(rel, 100);
    assert(rel.relationship_value == 100);

    printf("  PASS: Relationship clamped at +100\n");
}

// =============================================================================
// Test: Clamping at minimum (-100)
// =============================================================================

void test_clamp_min() {
    printf("Testing clamping at minimum (-100)...\n");

    NeighborRelationship rel{};
    rel.neighbor_id = 1;
    rel.relationship_value = -80;

    update_relationship(rel, -50);
    assert(rel.relationship_value == -100);  // Clamped to min
    assert(rel.max_available_tier == TradeAgreementType::None);

    // Subtracting more still stays at -100
    update_relationship(rel, -200);
    assert(rel.relationship_value == -100);

    printf("  PASS: Relationship clamped at -100\n");
}

// =============================================================================
// Test: record_trade updates history and relationship
// =============================================================================

void test_record_trade_basic() {
    printf("Testing record_trade basic functionality...\n");

    NeighborRelationship rel{};
    rel.neighbor_id = 1;
    rel.relationship_value = 0;

    record_trade(rel, 1000, 5);

    assert(rel.total_trades == 1);
    assert(rel.total_trade_volume == 1000);
    assert(rel.relationship_value == 5);

    record_trade(rel, 2000, 5);

    assert(rel.total_trades == 2);
    assert(rel.total_trade_volume == 3000);
    assert(rel.relationship_value == 10);

    printf("  PASS: record_trade updates trade count, volume, and relationship\n");
}

// =============================================================================
// Test: record_trade cumulative volume
// =============================================================================

void test_record_trade_cumulative() {
    printf("Testing record_trade cumulative trade volume...\n");

    NeighborRelationship rel{};
    rel.neighbor_id = 2;

    for (int i = 0; i < 100; ++i) {
        record_trade(rel, 500, 1);
    }

    assert(rel.total_trades == 100);
    assert(rel.total_trade_volume == 50000);
    assert(rel.relationship_value == 100);  // Clamped at max

    printf("  PASS: Cumulative trade tracking works correctly\n");
}

// =============================================================================
// Test: record_trade with negative relationship points
// =============================================================================

void test_record_trade_negative_points() {
    printf("Testing record_trade with negative relationship points...\n");

    NeighborRelationship rel{};
    rel.neighbor_id = 3;
    rel.relationship_value = 10;

    // A bad trade deal can reduce relationship
    record_trade(rel, 500, -20);

    assert(rel.total_trades == 1);
    assert(rel.total_trade_volume == 500);
    assert(rel.relationship_value == -10);
    assert(rel.max_available_tier == TradeAgreementType::Basic);  // Cold

    printf("  PASS: Negative points from trade reduce relationship\n");
}

// =============================================================================
// Test: Tier evolution from Neutral to Allied through trades
// =============================================================================

void test_tier_evolution_through_trades() {
    printf("Testing tier evolution from Neutral to Allied through trades...\n");

    NeighborRelationship rel{};
    rel.neighbor_id = 1;
    rel.relationship_value = 0;

    // Start at Neutral -> Basic tier
    assert(rel.max_available_tier == TradeAgreementType::Basic);

    // Trade enough to reach Warm (25+)
    for (int i = 0; i < 5; ++i) {
        record_trade(rel, 1000, 5);
    }
    assert(rel.relationship_value == 25);
    assert(rel.max_available_tier == TradeAgreementType::Enhanced);

    // Trade enough to reach Friendly (50+)
    for (int i = 0; i < 5; ++i) {
        record_trade(rel, 1000, 5);
    }
    assert(rel.relationship_value == 50);
    assert(rel.max_available_tier == TradeAgreementType::Enhanced);

    // Trade enough to reach Allied (80+)
    for (int i = 0; i < 6; ++i) {
        record_trade(rel, 1000, 5);
    }
    assert(rel.relationship_value == 80);
    assert(rel.max_available_tier == TradeAgreementType::Premium);

    printf("  PASS: Tier evolves correctly through trade history\n");
}

// =============================================================================
// Test: Tier downgrade from Allied to Hostile
// =============================================================================

void test_tier_downgrade() {
    printf("Testing tier downgrade from Allied to Hostile...\n");

    NeighborRelationship rel{};
    rel.neighbor_id = 1;
    rel.relationship_value = 90;
    rel.max_available_tier = TradeAgreementType::Premium;

    // Drop to Friendly
    update_relationship(rel, -20);
    assert(rel.relationship_value == 70);
    assert(rel.max_available_tier == TradeAgreementType::Enhanced);

    // Drop to Warm
    update_relationship(rel, -30);
    assert(rel.relationship_value == 40);
    assert(rel.max_available_tier == TradeAgreementType::Enhanced);

    // Drop to Neutral
    update_relationship(rel, -20);
    assert(rel.relationship_value == 20);
    assert(rel.max_available_tier == TradeAgreementType::Basic);

    // Drop to Cold
    update_relationship(rel, -30);
    assert(rel.relationship_value == -10);
    assert(rel.max_available_tier == TradeAgreementType::Basic);

    // Drop to Hostile
    update_relationship(rel, -50);
    assert(rel.relationship_value == -60);
    assert(rel.max_available_tier == TradeAgreementType::None);

    printf("  PASS: Tier downgrades correctly through all levels\n");
}

// =============================================================================
// Test: Boundary values for relationship status
// =============================================================================

void test_boundary_values() {
    printf("Testing exact boundary values...\n");

    // Hostile/Cold boundary at -50
    assert(get_relationship_status(-51) == RelationshipStatus::Hostile);
    assert(get_relationship_status(-50) == RelationshipStatus::Cold);

    // Cold/Neutral boundary at 0
    assert(get_relationship_status(-1) == RelationshipStatus::Cold);
    assert(get_relationship_status(0) == RelationshipStatus::Neutral);

    // Neutral/Warm boundary at 25
    assert(get_relationship_status(24) == RelationshipStatus::Neutral);
    assert(get_relationship_status(25) == RelationshipStatus::Warm);

    // Warm/Friendly boundary at 50
    assert(get_relationship_status(49) == RelationshipStatus::Warm);
    assert(get_relationship_status(50) == RelationshipStatus::Friendly);

    // Friendly/Allied boundary at 80
    assert(get_relationship_status(79) == RelationshipStatus::Friendly);
    assert(get_relationship_status(80) == RelationshipStatus::Allied);

    printf("  PASS: All boundary values classified correctly\n");
}

// =============================================================================
// Test: Boundary values for max tier
// =============================================================================

void test_tier_boundary_values() {
    printf("Testing exact boundary values for max tier...\n");

    // Hostile/Cold boundary: None -> Basic at -50
    assert(get_max_available_tier(-51) == TradeAgreementType::None);
    assert(get_max_available_tier(-50) == TradeAgreementType::Basic);

    // Cold+Neutral/Warm boundary: Basic -> Enhanced at 25
    assert(get_max_available_tier(24) == TradeAgreementType::Basic);
    assert(get_max_available_tier(25) == TradeAgreementType::Enhanced);

    // Friendly/Allied boundary: Enhanced -> Premium at 80
    assert(get_max_available_tier(79) == TradeAgreementType::Enhanced);
    assert(get_max_available_tier(80) == TradeAgreementType::Premium);

    printf("  PASS: All tier boundaries work correctly\n");
}

// =============================================================================
// Test: Relationship status to string conversion
// =============================================================================

void test_status_to_string() {
    printf("Testing relationship status string conversion...\n");

    assert(strcmp(relationship_status_to_string(RelationshipStatus::Hostile), "Hostile") == 0);
    assert(strcmp(relationship_status_to_string(RelationshipStatus::Cold), "Cold") == 0);
    assert(strcmp(relationship_status_to_string(RelationshipStatus::Neutral), "Neutral") == 0);
    assert(strcmp(relationship_status_to_string(RelationshipStatus::Warm), "Warm") == 0);
    assert(strcmp(relationship_status_to_string(RelationshipStatus::Friendly), "Friendly") == 0);
    assert(strcmp(relationship_status_to_string(RelationshipStatus::Allied), "Allied") == 0);

    printf("  PASS: All status strings are correct\n");
}

// =============================================================================
// Test: Zero points update
// =============================================================================

void test_zero_points_update() {
    printf("Testing update_relationship with zero points...\n");

    NeighborRelationship rel{};
    rel.neighbor_id = 1;
    rel.relationship_value = 42;
    rel.max_available_tier = TradeAgreementType::Enhanced;

    update_relationship(rel, 0);
    assert(rel.relationship_value == 42);
    assert(rel.max_available_tier == TradeAgreementType::Enhanced);

    printf("  PASS: Zero points leaves relationship unchanged\n");
}

// =============================================================================
// Test: Large trade volumes (int64_t)
// =============================================================================

void test_large_trade_volumes() {
    printf("Testing large trade volumes (int64_t range)...\n");

    NeighborRelationship rel{};
    rel.neighbor_id = 1;

    int64_t large_volume = 1000000000LL;  // 1 billion credits
    record_trade(rel, large_volume, 1);

    assert(rel.total_trades == 1);
    assert(rel.total_trade_volume == large_volume);

    record_trade(rel, large_volume, 1);

    assert(rel.total_trades == 2);
    assert(rel.total_trade_volume == 2 * large_volume);

    printf("  PASS: Large trade volumes tracked correctly\n");
}

// =============================================================================
// Test: Multiple neighbors independent
// =============================================================================

void test_independent_neighbors() {
    printf("Testing multiple neighbors are independent...\n");

    NeighborRelationship rel1{};
    rel1.neighbor_id = 1;

    NeighborRelationship rel2{};
    rel2.neighbor_id = 2;

    record_trade(rel1, 1000, 30);
    record_trade(rel2, 500, -60);

    assert(rel1.relationship_value == 30);
    assert(rel1.total_trades == 1);
    assert(rel1.max_available_tier == TradeAgreementType::Enhanced);

    assert(rel2.relationship_value == -60);
    assert(rel2.total_trades == 1);
    assert(rel2.max_available_tier == TradeAgreementType::None);

    printf("  PASS: Neighbor relationships are independent\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Neighbor Relationship Evolution Tests (Epic 8, Ticket E8-034) ===\n\n");

    test_default_initialization();
    test_custom_initialization();
    test_relationship_status_hostile();
    test_relationship_status_cold();
    test_relationship_status_neutral();
    test_relationship_status_warm();
    test_relationship_status_friendly();
    test_relationship_status_allied();
    test_max_tier_hostile();
    test_max_tier_cold();
    test_max_tier_neutral();
    test_max_tier_warm();
    test_max_tier_friendly();
    test_max_tier_allied();
    test_update_positive_points();
    test_update_negative_points();
    test_clamp_max();
    test_clamp_min();
    test_record_trade_basic();
    test_record_trade_cumulative();
    test_record_trade_negative_points();
    test_tier_evolution_through_trades();
    test_tier_downgrade();
    test_boundary_values();
    test_tier_boundary_values();
    test_status_to_string();
    test_zero_points_update();
    test_large_trade_volumes();
    test_independent_neighbors();

    printf("\n=== All Neighbor Relationship Evolution Tests Passed ===\n");
    return 0;
}
