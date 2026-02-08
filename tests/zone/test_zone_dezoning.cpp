/**
 * @file test_zone_dezoning.cpp
 * @brief Tests for de-zoning implementation (Ticket 4-013)
 *
 * Tests:
 * - Dezone empty (Designated) zones
 * - Dezone stalled zones
 * - Dezone occupied zones emits DemolitionRequestEvent
 * - Rectangular dezone
 * - Ownership check
 * - Events emitted correctly
 * - ZoneCounts decremented
 * - finalize_zone_removal works
 */

#include <gtest/gtest.h>
#include <sims3000/zone/ZoneSystem.h>

namespace sims3000 {
namespace zone {
namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class ZoneDezoningTest : public ::testing::Test {
protected:
    void SetUp() override {
        system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
    }

    /// Helper to place a zone and return the entity_id
    std::uint32_t place_at(std::int32_t x, std::int32_t y,
                            ZoneType type = ZoneType::Habitation,
                            ZoneDensity density = ZoneDensity::LowDensity,
                            std::uint8_t player_id = 0,
                            std::uint32_t entity_id = 0) {
        static std::uint32_t next_id = 100;
        if (entity_id == 0) entity_id = next_id++;
        system->place_zone(x, y, type, density, player_id, entity_id);
        return entity_id;
    }

    std::unique_ptr<ZoneSystem> system;
};

// ============================================================================
// Dezone Designated zones tests
// ============================================================================

TEST_F(ZoneDezoningTest, DezoneDesignatedZone_SingleCell) {
    place_at(10, 10);
    ASSERT_TRUE(system->is_zoned(10, 10));

    auto result = system->remove_zones(10, 10, 1, 1, 0);
    EXPECT_TRUE(result.any_removed);
    EXPECT_EQ(result.removed_count, 1u);
    EXPECT_EQ(result.skipped_count, 0u);
    EXPECT_EQ(result.demolition_requested_count, 0u);

    // Zone should be gone
    EXPECT_FALSE(system->is_zoned(10, 10));
}

TEST_F(ZoneDezoningTest, DezoneDesignatedZone_EmitsEvent) {
    std::uint32_t eid = place_at(10, 10, ZoneType::Exchange, ZoneDensity::HighDensity, 1);

    system->remove_zones(10, 10, 1, 1, 1);

    const auto& events = system->get_pending_undesignated_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].entity_id, eid);
    EXPECT_EQ(events[0].grid_x, 10);
    EXPECT_EQ(events[0].grid_y, 10);
    EXPECT_EQ(events[0].zone_type, ZoneType::Exchange);
    EXPECT_EQ(events[0].owner_id, 1);
}

// ============================================================================
// Dezone Stalled zones tests
// ============================================================================

TEST_F(ZoneDezoningTest, DezoneStalledZone) {
    place_at(15, 15);
    // Transition to Stalled
    ASSERT_TRUE(system->set_zone_state(15, 15, ZoneState::Stalled));

    ZoneState state;
    ASSERT_TRUE(system->get_zone_state(15, 15, state));
    ASSERT_EQ(state, ZoneState::Stalled);

    auto result = system->remove_zones(15, 15, 1, 1, 0);
    EXPECT_TRUE(result.any_removed);
    EXPECT_EQ(result.removed_count, 1u);
    EXPECT_EQ(result.demolition_requested_count, 0u);

    EXPECT_FALSE(system->is_zoned(15, 15));
}

// ============================================================================
// Dezone Occupied zones tests (emits DemolitionRequestEvent)
// ============================================================================

TEST_F(ZoneDezoningTest, DezoneOccupiedZone_EmitsDemolitionRequest) {
    std::uint32_t eid = place_at(20, 20);
    // Transition to Occupied
    ASSERT_TRUE(system->set_zone_state(20, 20, ZoneState::Occupied));

    auto result = system->remove_zones(20, 20, 1, 1, 0);

    // Zone should NOT be removed yet
    EXPECT_TRUE(system->is_zoned(20, 20));
    EXPECT_EQ(result.removed_count, 0u);
    EXPECT_EQ(result.demolition_requested_count, 1u);
    EXPECT_FALSE(result.any_removed);

    // No undesignated events
    EXPECT_EQ(system->get_pending_undesignated_events().size(), 0u);

    // Demolition request event emitted
    const auto& demo_events = system->get_pending_demolition_events();
    ASSERT_EQ(demo_events.size(), 1u);
    EXPECT_EQ(demo_events[0].grid_x, 20);
    EXPECT_EQ(demo_events[0].grid_y, 20);
    EXPECT_EQ(demo_events[0].requesting_entity_id, eid);
}

// ============================================================================
// Rectangular dezone tests
// ============================================================================

TEST_F(ZoneDezoningTest, RectangularDezone) {
    // Place a 3x3 grid of zones
    for (std::int32_t y = 10; y < 13; ++y) {
        for (std::int32_t x = 10; x < 13; ++x) {
            place_at(x, y);
        }
    }

    auto result = system->remove_zones(10, 10, 3, 3, 0);
    EXPECT_TRUE(result.any_removed);
    EXPECT_EQ(result.removed_count, 9u);
    EXPECT_EQ(result.skipped_count, 0u);

    // All zones should be gone
    for (std::int32_t y = 10; y < 13; ++y) {
        for (std::int32_t x = 10; x < 13; ++x) {
            EXPECT_FALSE(system->is_zoned(x, y))
                << "Expected no zone at (" << x << ", " << y << ")";
        }
    }
}

TEST_F(ZoneDezoningTest, RectangularDezone_MixedStates) {
    // Place 3 zones: designated, stalled, occupied
    place_at(10, 10);  // Will be Designated
    place_at(11, 10);  // Will be Stalled
    place_at(12, 10);  // Will be Occupied

    ASSERT_TRUE(system->set_zone_state(11, 10, ZoneState::Stalled));
    ASSERT_TRUE(system->set_zone_state(12, 10, ZoneState::Occupied));

    auto result = system->remove_zones(10, 10, 3, 1, 0);

    EXPECT_EQ(result.removed_count, 2u);    // Designated + Stalled
    EXPECT_EQ(result.demolition_requested_count, 1u); // Occupied
    EXPECT_EQ(result.skipped_count, 0u);

    EXPECT_FALSE(system->is_zoned(10, 10));  // Removed
    EXPECT_FALSE(system->is_zoned(11, 10));  // Removed
    EXPECT_TRUE(system->is_zoned(12, 10));   // Still there (occupied, pending demolition)
}

TEST_F(ZoneDezoningTest, RectangularDezone_SomeEmpty) {
    // Only place zones at some positions
    place_at(10, 10);
    place_at(12, 10);

    auto result = system->remove_zones(10, 10, 3, 1, 0);

    EXPECT_TRUE(result.any_removed);
    EXPECT_EQ(result.removed_count, 2u);
    EXPECT_EQ(result.skipped_count, 1u); // Cell (11,10) had no zone
}

// ============================================================================
// Ownership check tests
// ============================================================================

TEST_F(ZoneDezoningTest, OwnershipCheck_WrongOwner) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);

    // Player 1 tries to dezone player 0's zone
    auto result = system->remove_zones(10, 10, 1, 1, 1);

    EXPECT_FALSE(result.any_removed);
    EXPECT_EQ(result.removed_count, 0u);
    EXPECT_EQ(result.skipped_count, 1u);

    // Zone should still be there
    EXPECT_TRUE(system->is_zoned(10, 10));
}

TEST_F(ZoneDezoningTest, OwnershipCheck_CorrectOwner) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 2);

    auto result = system->remove_zones(10, 10, 1, 1, 2);
    EXPECT_TRUE(result.any_removed);
    EXPECT_EQ(result.removed_count, 1u);
}

TEST_F(ZoneDezoningTest, OwnershipCheck_MixedOwners) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    place_at(11, 10, ZoneType::Exchange, ZoneDensity::HighDensity, 1);
    place_at(12, 10, ZoneType::Fabrication, ZoneDensity::LowDensity, 0);

    auto result = system->remove_zones(10, 10, 3, 1, 0);

    EXPECT_TRUE(result.any_removed);
    EXPECT_EQ(result.removed_count, 2u);  // (10,10) and (12,10) owned by player 0
    EXPECT_EQ(result.skipped_count, 1u);  // (11,10) owned by player 1
}

// ============================================================================
// Events emitted tests
// ============================================================================

TEST_F(ZoneDezoningTest, UndesignatedEventsEmitted) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    place_at(11, 10, ZoneType::Exchange, ZoneDensity::HighDensity, 0);

    system->remove_zones(10, 10, 2, 1, 0);

    const auto& events = system->get_pending_undesignated_events();
    EXPECT_EQ(events.size(), 2u);
}

TEST_F(ZoneDezoningTest, EventsCleared) {
    place_at(10, 10);
    system->remove_zones(10, 10, 1, 1, 0);
    EXPECT_EQ(system->get_pending_undesignated_events().size(), 1u);

    system->clear_pending_undesignated_events();
    EXPECT_EQ(system->get_pending_undesignated_events().size(), 0u);
}

TEST_F(ZoneDezoningTest, DemolitionEventsCleared) {
    place_at(10, 10);
    system->set_zone_state(10, 10, ZoneState::Occupied);
    system->remove_zones(10, 10, 1, 1, 0);

    EXPECT_EQ(system->get_pending_demolition_events().size(), 1u);
    system->clear_pending_demolition_events();
    EXPECT_EQ(system->get_pending_demolition_events().size(), 0u);
}

// ============================================================================
// ZoneCounts decremented tests
// ============================================================================

TEST_F(ZoneDezoningTest, ZoneCountsDecremented_Designated) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    place_at(11, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);

    EXPECT_EQ(system->get_zone_count(0, ZoneType::Habitation), 2u);
    EXPECT_EQ(system->get_zone_counts(0).total, 2u);
    EXPECT_EQ(system->get_zone_counts(0).designated_total, 2u);
    EXPECT_EQ(system->get_zone_counts(0).low_density_total, 2u);

    system->remove_zones(10, 10, 1, 1, 0);

    EXPECT_EQ(system->get_zone_count(0, ZoneType::Habitation), 1u);
    EXPECT_EQ(system->get_zone_counts(0).total, 1u);
    EXPECT_EQ(system->get_zone_counts(0).designated_total, 1u);
    EXPECT_EQ(system->get_zone_counts(0).low_density_total, 1u);
}

TEST_F(ZoneDezoningTest, ZoneCountsDecremented_Stalled) {
    place_at(10, 10, ZoneType::Exchange, ZoneDensity::HighDensity, 0);
    system->set_zone_state(10, 10, ZoneState::Stalled);

    EXPECT_EQ(system->get_zone_counts(0).stalled_total, 1u);
    EXPECT_EQ(system->get_zone_counts(0).exchange_total, 1u);

    system->remove_zones(10, 10, 1, 1, 0);

    EXPECT_EQ(system->get_zone_counts(0).stalled_total, 0u);
    EXPECT_EQ(system->get_zone_counts(0).exchange_total, 0u);
    EXPECT_EQ(system->get_zone_counts(0).total, 0u);
}

TEST_F(ZoneDezoningTest, ZoneCountsNotDecremented_OccupiedZone) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    system->set_zone_state(10, 10, ZoneState::Occupied);

    std::uint32_t total_before = system->get_zone_counts(0).total;
    system->remove_zones(10, 10, 1, 1, 0);

    // Counts should NOT change for occupied zones (not removed yet)
    EXPECT_EQ(system->get_zone_counts(0).total, total_before);
}

// ============================================================================
// finalize_zone_removal tests
// ============================================================================

TEST_F(ZoneDezoningTest, FinalizeZoneRemoval_RemovesZone) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    system->set_zone_state(10, 10, ZoneState::Occupied);

    ASSERT_TRUE(system->is_zoned(10, 10));

    bool removed = system->finalize_zone_removal(10, 10);
    EXPECT_TRUE(removed);
    EXPECT_FALSE(system->is_zoned(10, 10));
}

TEST_F(ZoneDezoningTest, FinalizeZoneRemoval_DecrementsCounts) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    system->set_zone_state(10, 10, ZoneState::Occupied);

    EXPECT_EQ(system->get_zone_counts(0).total, 1u);
    EXPECT_EQ(system->get_zone_counts(0).occupied_total, 1u);

    system->finalize_zone_removal(10, 10);

    EXPECT_EQ(system->get_zone_counts(0).total, 0u);
    EXPECT_EQ(system->get_zone_counts(0).occupied_total, 0u);
}

TEST_F(ZoneDezoningTest, FinalizeZoneRemoval_EmitsUndesignatedEvent) {
    place_at(10, 10, ZoneType::Exchange, ZoneDensity::HighDensity, 1);

    system->finalize_zone_removal(10, 10);

    const auto& events = system->get_pending_undesignated_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].grid_x, 10);
    EXPECT_EQ(events[0].grid_y, 10);
    EXPECT_EQ(events[0].zone_type, ZoneType::Exchange);
    EXPECT_EQ(events[0].owner_id, 1);
}

TEST_F(ZoneDezoningTest, FinalizeZoneRemoval_NoZone_ReturnsFalse) {
    bool removed = system->finalize_zone_removal(10, 10);
    EXPECT_FALSE(removed);
}

TEST_F(ZoneDezoningTest, FinalizeZoneRemoval_FullDemolitionFlow) {
    // Simulate the full dezone-occupied flow:
    // 1. Place and make occupied
    place_at(10, 10, ZoneType::Fabrication, ZoneDensity::LowDensity, 0);
    system->set_zone_state(10, 10, ZoneState::Occupied);

    // 2. remove_zones emits DemolitionRequestEvent
    auto result = system->remove_zones(10, 10, 1, 1, 0);
    EXPECT_EQ(result.demolition_requested_count, 1u);
    EXPECT_TRUE(system->is_zoned(10, 10));

    system->clear_pending_demolition_events();

    // 3. BuildingSystem handles demolition, calls finalize
    bool finalized = system->finalize_zone_removal(10, 10);
    EXPECT_TRUE(finalized);
    EXPECT_FALSE(system->is_zoned(10, 10));
    EXPECT_EQ(system->get_zone_counts(0).total, 0u);
}

// ============================================================================
// Edge cases
// ============================================================================

TEST_F(ZoneDezoningTest, DezoneEmptyArea) {
    auto result = system->remove_zones(10, 10, 3, 3, 0);
    EXPECT_FALSE(result.any_removed);
    EXPECT_EQ(result.removed_count, 0u);
    EXPECT_EQ(result.skipped_count, 9u);
}

TEST_F(ZoneDezoningTest, DezoneOutOfBounds) {
    auto result = system->remove_zones(128, 128, 1, 1, 0);
    EXPECT_FALSE(result.any_removed);
    EXPECT_EQ(result.removed_count, 0u);
    EXPECT_EQ(result.skipped_count, 1u);
}

} // anonymous namespace
} // namespace zone
} // namespace sims3000
