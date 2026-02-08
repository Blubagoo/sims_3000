/**
 * @file test_zone_state_management.cpp
 * @brief Tests for zone state management (Ticket 4-015)
 *
 * Tests:
 * - All 4 valid transitions
 * - Invalid transition rejection
 * - Event emission for each transition
 * - ZoneCounts updates on transitions
 * - Stalled zones not counted as supply
 */

#include <gtest/gtest.h>
#include <sims3000/zone/ZoneSystem.h>

namespace sims3000 {
namespace zone {
namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class ZoneStateManagementTest : public ::testing::Test {
protected:
    void SetUp() override {
        system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
    }

    /// Helper: place a zone and return its entity_id
    std::uint32_t place_test_zone(std::int32_t x, std::int32_t y,
                                   ZoneType type = ZoneType::Habitation,
                                   std::uint8_t player_id = 0) {
        std::uint32_t entity_id = next_entity_id++;
        system->place_zone(x, y, type, ZoneDensity::LowDensity, player_id, entity_id);
        return entity_id;
    }

    std::unique_ptr<ZoneSystem> system;
    std::uint32_t next_entity_id = 100;
};

// ============================================================================
// Valid Transition Tests
// ============================================================================

TEST_F(ZoneStateManagementTest, ValidTransition_DesignatedToOccupied) {
    place_test_zone(10, 10);
    // Zone starts as Designated
    EXPECT_TRUE(system->set_zone_state(10, 10, ZoneState::Occupied));
}

TEST_F(ZoneStateManagementTest, ValidTransition_OccupiedToDesignated) {
    place_test_zone(10, 10);
    system->set_zone_state(10, 10, ZoneState::Occupied);
    system->clear_pending_state_events();

    EXPECT_TRUE(system->set_zone_state(10, 10, ZoneState::Designated));
}

TEST_F(ZoneStateManagementTest, ValidTransition_DesignatedToStalled) {
    place_test_zone(10, 10);
    EXPECT_TRUE(system->set_zone_state(10, 10, ZoneState::Stalled));
}

TEST_F(ZoneStateManagementTest, ValidTransition_StalledToDesignated) {
    place_test_zone(10, 10);
    system->set_zone_state(10, 10, ZoneState::Stalled);
    system->clear_pending_state_events();

    EXPECT_TRUE(system->set_zone_state(10, 10, ZoneState::Designated));
}

// ============================================================================
// Invalid Transition Tests
// ============================================================================

TEST_F(ZoneStateManagementTest, InvalidTransition_OccupiedToStalled) {
    place_test_zone(10, 10);
    system->set_zone_state(10, 10, ZoneState::Occupied);
    system->clear_pending_state_events();

    EXPECT_FALSE(system->set_zone_state(10, 10, ZoneState::Stalled));
}

TEST_F(ZoneStateManagementTest, InvalidTransition_StalledToOccupied) {
    place_test_zone(10, 10);
    system->set_zone_state(10, 10, ZoneState::Stalled);
    system->clear_pending_state_events();

    EXPECT_FALSE(system->set_zone_state(10, 10, ZoneState::Occupied));
}

TEST_F(ZoneStateManagementTest, InvalidTransition_SameState) {
    place_test_zone(10, 10);
    // Zone is Designated, try to set to Designated again
    EXPECT_FALSE(system->set_zone_state(10, 10, ZoneState::Designated));
}

TEST_F(ZoneStateManagementTest, InvalidTransition_NoZoneAtPosition) {
    // No zone placed at (10, 10)
    EXPECT_FALSE(system->set_zone_state(10, 10, ZoneState::Occupied));
}

TEST_F(ZoneStateManagementTest, InvalidTransition_OutOfBounds) {
    EXPECT_FALSE(system->set_zone_state(-1, -1, ZoneState::Occupied));
}

// ============================================================================
// Event Emission Tests
// ============================================================================

TEST_F(ZoneStateManagementTest, EventEmission_DesignatedToOccupied) {
    std::uint32_t eid = place_test_zone(10, 10);
    system->clear_pending_state_events();

    system->set_zone_state(10, 10, ZoneState::Occupied);

    const auto& events = system->get_pending_state_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].entity_id, eid);
    EXPECT_EQ(events[0].grid_x, 10);
    EXPECT_EQ(events[0].grid_y, 10);
    EXPECT_EQ(events[0].old_state, ZoneState::Designated);
    EXPECT_EQ(events[0].new_state, ZoneState::Occupied);
}

TEST_F(ZoneStateManagementTest, EventEmission_OccupiedToDesignated) {
    std::uint32_t eid = place_test_zone(20, 20);
    system->set_zone_state(20, 20, ZoneState::Occupied);
    system->clear_pending_state_events();

    system->set_zone_state(20, 20, ZoneState::Designated);

    const auto& events = system->get_pending_state_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].entity_id, eid);
    EXPECT_EQ(events[0].old_state, ZoneState::Occupied);
    EXPECT_EQ(events[0].new_state, ZoneState::Designated);
}

TEST_F(ZoneStateManagementTest, EventEmission_DesignatedToStalled) {
    std::uint32_t eid = place_test_zone(30, 30);
    system->clear_pending_state_events();

    system->set_zone_state(30, 30, ZoneState::Stalled);

    const auto& events = system->get_pending_state_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].entity_id, eid);
    EXPECT_EQ(events[0].old_state, ZoneState::Designated);
    EXPECT_EQ(events[0].new_state, ZoneState::Stalled);
}

TEST_F(ZoneStateManagementTest, EventEmission_StalledToDesignated) {
    std::uint32_t eid = place_test_zone(40, 40);
    system->set_zone_state(40, 40, ZoneState::Stalled);
    system->clear_pending_state_events();

    system->set_zone_state(40, 40, ZoneState::Designated);

    const auto& events = system->get_pending_state_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].entity_id, eid);
    EXPECT_EQ(events[0].old_state, ZoneState::Stalled);
    EXPECT_EQ(events[0].new_state, ZoneState::Designated);
}

TEST_F(ZoneStateManagementTest, EventEmission_NoEventOnInvalidTransition) {
    place_test_zone(10, 10);
    system->set_zone_state(10, 10, ZoneState::Occupied);
    system->clear_pending_state_events();

    // Occupied -> Stalled is invalid
    system->set_zone_state(10, 10, ZoneState::Stalled);

    const auto& events = system->get_pending_state_events();
    EXPECT_EQ(events.size(), 0u);
}

TEST_F(ZoneStateManagementTest, EventEmission_MultipleEvents) {
    place_test_zone(10, 10);
    place_test_zone(20, 20);
    system->clear_pending_state_events();

    system->set_zone_state(10, 10, ZoneState::Occupied);
    system->set_zone_state(20, 20, ZoneState::Stalled);

    const auto& events = system->get_pending_state_events();
    EXPECT_EQ(events.size(), 2u);
}

TEST_F(ZoneStateManagementTest, ClearPendingEvents) {
    place_test_zone(10, 10);
    system->set_zone_state(10, 10, ZoneState::Occupied);

    EXPECT_GT(system->get_pending_state_events().size(), 0u);
    system->clear_pending_state_events();
    EXPECT_EQ(system->get_pending_state_events().size(), 0u);
}

// ============================================================================
// ZoneCounts Update Tests
// ============================================================================

TEST_F(ZoneStateManagementTest, Counts_DesignatedToOccupied) {
    place_test_zone(10, 10, ZoneType::Habitation, 0);
    const ZoneCounts& counts = system->get_zone_counts(0);
    EXPECT_EQ(counts.designated_total, 1u);
    EXPECT_EQ(counts.occupied_total, 0u);

    system->set_zone_state(10, 10, ZoneState::Occupied);
    EXPECT_EQ(counts.designated_total, 0u);
    EXPECT_EQ(counts.occupied_total, 1u);
}

TEST_F(ZoneStateManagementTest, Counts_OccupiedToDesignated) {
    place_test_zone(10, 10, ZoneType::Exchange, 0);
    system->set_zone_state(10, 10, ZoneState::Occupied);
    EXPECT_EQ(system->get_zone_counts(0).occupied_total, 1u);

    system->set_zone_state(10, 10, ZoneState::Designated);
    EXPECT_EQ(system->get_zone_counts(0).designated_total, 1u);
    EXPECT_EQ(system->get_zone_counts(0).occupied_total, 0u);
}

TEST_F(ZoneStateManagementTest, Counts_DesignatedToStalled) {
    place_test_zone(10, 10, ZoneType::Fabrication, 0);
    system->set_zone_state(10, 10, ZoneState::Stalled);

    const ZoneCounts& counts = system->get_zone_counts(0);
    EXPECT_EQ(counts.designated_total, 0u);
    EXPECT_EQ(counts.stalled_total, 1u);
}

TEST_F(ZoneStateManagementTest, Counts_StalledToDesignated) {
    place_test_zone(10, 10, ZoneType::Habitation, 0);
    system->set_zone_state(10, 10, ZoneState::Stalled);
    system->set_zone_state(10, 10, ZoneState::Designated);

    const ZoneCounts& counts = system->get_zone_counts(0);
    EXPECT_EQ(counts.designated_total, 1u);
    EXPECT_EQ(counts.stalled_total, 0u);
}

TEST_F(ZoneStateManagementTest, Counts_NoChangeOnInvalidTransition) {
    place_test_zone(10, 10, ZoneType::Habitation, 0);
    system->set_zone_state(10, 10, ZoneState::Occupied);

    const ZoneCounts& counts = system->get_zone_counts(0);
    EXPECT_EQ(counts.occupied_total, 1u);
    EXPECT_EQ(counts.stalled_total, 0u);

    // Invalid transition: Occupied -> Stalled
    system->set_zone_state(10, 10, ZoneState::Stalled);

    // Counts should not change
    EXPECT_EQ(counts.occupied_total, 1u);
    EXPECT_EQ(counts.stalled_total, 0u);
}

TEST_F(ZoneStateManagementTest, Counts_TotalUnchangedDuringTransitions) {
    place_test_zone(10, 10, ZoneType::Habitation, 0);
    const ZoneCounts& counts = system->get_zone_counts(0);
    EXPECT_EQ(counts.total, 1u);

    system->set_zone_state(10, 10, ZoneState::Occupied);
    EXPECT_EQ(counts.total, 1u);

    system->set_zone_state(10, 10, ZoneState::Designated);
    EXPECT_EQ(counts.total, 1u);

    system->set_zone_state(10, 10, ZoneState::Stalled);
    EXPECT_EQ(counts.total, 1u);
}

// ============================================================================
// Stalled zones not counted as supply
// ============================================================================

TEST_F(ZoneStateManagementTest, StalledNotCountedAsOccupied) {
    place_test_zone(10, 10, ZoneType::Habitation, 0);
    place_test_zone(20, 20, ZoneType::Habitation, 0);

    // Make one occupied and one stalled
    system->set_zone_state(10, 10, ZoneState::Occupied);
    system->set_zone_state(20, 20, ZoneState::Stalled);

    const ZoneCounts& counts = system->get_zone_counts(0);
    EXPECT_EQ(counts.occupied_total, 1u);
    EXPECT_EQ(counts.stalled_total, 1u);
    EXPECT_EQ(counts.designated_total, 0u);
    // Stalled should not contribute to occupied count (supply)
    EXPECT_NE(counts.occupied_total, counts.occupied_total + counts.stalled_total);
}

// ============================================================================
// Multi-overseer state management
// ============================================================================

TEST_F(ZoneStateManagementTest, MultiOverseer_IndependentCounts) {
    place_test_zone(10, 10, ZoneType::Habitation, 0);
    place_test_zone(20, 20, ZoneType::Habitation, 1);

    system->set_zone_state(10, 10, ZoneState::Occupied);
    system->set_zone_state(20, 20, ZoneState::Stalled);

    EXPECT_EQ(system->get_zone_counts(0).occupied_total, 1u);
    EXPECT_EQ(system->get_zone_counts(0).stalled_total, 0u);
    EXPECT_EQ(system->get_zone_counts(1).occupied_total, 0u);
    EXPECT_EQ(system->get_zone_counts(1).stalled_total, 1u);
}

} // anonymous namespace
} // namespace zone
} // namespace sims3000
