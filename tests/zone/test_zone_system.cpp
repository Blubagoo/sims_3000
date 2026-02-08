/**
 * @file test_zone_system.cpp
 * @brief Tests for ZoneSystem (ticket 4-008)
 */

#include <sims3000/zone/ZoneSystem.h>
#include <gtest/gtest.h>

using namespace sims3000::zone;

// ============================================================================
// ISimulatable Tests
// ============================================================================

TEST(ZoneSystemTest, PriorityIs30) {
    ZoneSystem system(nullptr, nullptr, 128);
    EXPECT_EQ(system.get_priority(), 30);
}

TEST(ZoneSystemTest, TickDoesNotCrash) {
    ZoneSystem system(nullptr, nullptr, 128);
    // Tick with various delta times - should not crash
    system.tick(0.05f);
    system.tick(0.0f);
    system.tick(1.0f);
}

TEST(ZoneSystemTest, ImplementsISimulatable) {
    ZoneSystem system(nullptr, nullptr, 128);
    ISimulatable* interface = &system;

    // Verify polymorphic calls work
    EXPECT_EQ(interface->get_priority(), 30);
    interface->tick(0.05f); // Should not crash
}

// ============================================================================
// Grid Initialization Tests
// ============================================================================

TEST(ZoneSystemTest, GridInitialized128) {
    ZoneSystem system(nullptr, nullptr, 128);
    const ZoneGrid& grid = system.get_grid();

    EXPECT_EQ(grid.getWidth(), 128);
    EXPECT_EQ(grid.getHeight(), 128);
    EXPECT_FALSE(grid.empty());
}

TEST(ZoneSystemTest, GridInitialized256) {
    ZoneSystem system(nullptr, nullptr, 256);
    const ZoneGrid& grid = system.get_grid();

    EXPECT_EQ(grid.getWidth(), 256);
    EXPECT_EQ(grid.getHeight(), 256);
    EXPECT_FALSE(grid.empty());
}

TEST(ZoneSystemTest, GridInitializedDefault256) {
    ZoneSystem system(nullptr, nullptr);
    const ZoneGrid& grid = system.get_grid();

    EXPECT_EQ(grid.getWidth(), 256);
    EXPECT_EQ(grid.getHeight(), 256);
}

// ============================================================================
// Zone Placement and Query Tests
// ============================================================================

TEST(ZoneSystemTest, PlaceZoneSuccess) {
    ZoneSystem system(nullptr, nullptr, 128);

    bool result = system.place_zone(10, 20, ZoneType::Habitation,
                                    ZoneDensity::LowDensity, 0, 1001);
    EXPECT_TRUE(result);
    EXPECT_TRUE(system.is_zoned(10, 20));
}

TEST(ZoneSystemTest, PlaceZoneDuplicate) {
    ZoneSystem system(nullptr, nullptr, 128);

    system.place_zone(10, 20, ZoneType::Habitation,
                      ZoneDensity::LowDensity, 0, 1001);

    // Second placement at same position should fail
    bool result = system.place_zone(10, 20, ZoneType::Exchange,
                                    ZoneDensity::HighDensity, 0, 1002);
    EXPECT_FALSE(result);
}

TEST(ZoneSystemTest, PlaceZoneOutOfBounds) {
    ZoneSystem system(nullptr, nullptr, 128);

    bool result = system.place_zone(-1, 0, ZoneType::Habitation,
                                    ZoneDensity::LowDensity, 0, 1001);
    EXPECT_FALSE(result);

    result = system.place_zone(0, 128, ZoneType::Habitation,
                               ZoneDensity::LowDensity, 0, 1002);
    EXPECT_FALSE(result);
}

TEST(ZoneSystemTest, IsZonedEmpty) {
    ZoneSystem system(nullptr, nullptr, 128);
    EXPECT_FALSE(system.is_zoned(10, 20));
}

TEST(ZoneSystemTest, GetZoneTypeSuccess) {
    ZoneSystem system(nullptr, nullptr, 128);
    system.place_zone(5, 5, ZoneType::Exchange,
                      ZoneDensity::HighDensity, 1, 100);

    ZoneType type;
    EXPECT_TRUE(system.get_zone_type(5, 5, type));
    EXPECT_EQ(type, ZoneType::Exchange);
}

TEST(ZoneSystemTest, GetZoneTypeEmpty) {
    ZoneSystem system(nullptr, nullptr, 128);

    ZoneType type;
    EXPECT_FALSE(system.get_zone_type(5, 5, type));
}

TEST(ZoneSystemTest, GetZoneDensitySuccess) {
    ZoneSystem system(nullptr, nullptr, 128);
    system.place_zone(5, 5, ZoneType::Fabrication,
                      ZoneDensity::HighDensity, 2, 200);

    ZoneDensity density;
    EXPECT_TRUE(system.get_zone_density(5, 5, density));
    EXPECT_EQ(density, ZoneDensity::HighDensity);
}

TEST(ZoneSystemTest, GetZoneDensityEmpty) {
    ZoneSystem system(nullptr, nullptr, 128);

    ZoneDensity density;
    EXPECT_FALSE(system.get_zone_density(5, 5, density));
}

// ============================================================================
// ZoneCounts Tracking Tests
// ============================================================================

TEST(ZoneSystemTest, ZoneCountsInitializedToZero) {
    ZoneSystem system(nullptr, nullptr, 128);

    for (std::uint8_t pid = 0; pid < 5; ++pid) {
        const ZoneCounts& counts = system.get_zone_counts(pid);
        EXPECT_EQ(counts.total, 0u);
        EXPECT_EQ(counts.habitation_total, 0u);
        EXPECT_EQ(counts.exchange_total, 0u);
        EXPECT_EQ(counts.fabrication_total, 0u);
        EXPECT_EQ(counts.low_density_total, 0u);
        EXPECT_EQ(counts.high_density_total, 0u);
        EXPECT_EQ(counts.designated_total, 0u);
        EXPECT_EQ(counts.occupied_total, 0u);
        EXPECT_EQ(counts.stalled_total, 0u);
    }
}

TEST(ZoneSystemTest, ZoneCountsTrackByType) {
    ZoneSystem system(nullptr, nullptr, 128);

    system.place_zone(0, 0, ZoneType::Habitation, ZoneDensity::LowDensity, 0, 1);
    system.place_zone(1, 0, ZoneType::Habitation, ZoneDensity::LowDensity, 0, 2);
    system.place_zone(2, 0, ZoneType::Exchange, ZoneDensity::LowDensity, 0, 3);
    system.place_zone(3, 0, ZoneType::Fabrication, ZoneDensity::HighDensity, 0, 4);

    EXPECT_EQ(system.get_zone_count(0, ZoneType::Habitation), 2u);
    EXPECT_EQ(system.get_zone_count(0, ZoneType::Exchange), 1u);
    EXPECT_EQ(system.get_zone_count(0, ZoneType::Fabrication), 1u);

    const ZoneCounts& counts = system.get_zone_counts(0);
    EXPECT_EQ(counts.total, 4u);
    EXPECT_EQ(counts.low_density_total, 3u);
    EXPECT_EQ(counts.high_density_total, 1u);
    EXPECT_EQ(counts.designated_total, 4u);
}

TEST(ZoneSystemTest, ZoneCountsTrackPerPlayer) {
    ZoneSystem system(nullptr, nullptr, 128);

    system.place_zone(0, 0, ZoneType::Habitation, ZoneDensity::LowDensity, 0, 1);
    system.place_zone(1, 0, ZoneType::Exchange, ZoneDensity::LowDensity, 1, 2);
    system.place_zone(2, 0, ZoneType::Fabrication, ZoneDensity::HighDensity, 2, 3);

    EXPECT_EQ(system.get_zone_count(0, ZoneType::Habitation), 1u);
    EXPECT_EQ(system.get_zone_count(0, ZoneType::Exchange), 0u);
    EXPECT_EQ(system.get_zone_count(1, ZoneType::Exchange), 1u);
    EXPECT_EQ(system.get_zone_count(2, ZoneType::Fabrication), 1u);
}

TEST(ZoneSystemTest, GetZoneCountInvalidPlayer) {
    ZoneSystem system(nullptr, nullptr, 128);
    // Player ID >= MAX_OVERSEERS should return 0
    EXPECT_EQ(system.get_zone_count(5, ZoneType::Habitation), 0u);
    EXPECT_EQ(system.get_zone_count(255, ZoneType::Exchange), 0u);
}

// ============================================================================
// Demand Stub Tests
// ============================================================================

TEST(ZoneSystemTest, DemandStubReturnsZero) {
    ZoneSystem system(nullptr, nullptr, 128);

    EXPECT_EQ(system.get_demand_for_type(ZoneType::Habitation, 0), 0);
    EXPECT_EQ(system.get_demand_for_type(ZoneType::Exchange, 1), 0);
    EXPECT_EQ(system.get_demand_for_type(ZoneType::Fabrication, 2), 0);
}

// ============================================================================
// Zone State Tests
// ============================================================================

TEST(ZoneSystemTest, SetZoneState) {
    ZoneSystem system(nullptr, nullptr, 128);
    system.place_zone(5, 5, ZoneType::Habitation,
                      ZoneDensity::LowDensity, 0, 100);

    // Initial state should be Designated
    const ZoneCounts& counts = system.get_zone_counts(0);
    EXPECT_EQ(counts.designated_total, 1u);
    EXPECT_EQ(counts.occupied_total, 0u);

    // Transition to Occupied
    bool result = system.set_zone_state(5, 5, ZoneState::Occupied);
    EXPECT_TRUE(result);
    EXPECT_EQ(counts.designated_total, 0u);
    EXPECT_EQ(counts.occupied_total, 1u);

    // Transition back to Designated (Occupied -> Designated is valid)
    result = system.set_zone_state(5, 5, ZoneState::Designated);
    EXPECT_TRUE(result);
    EXPECT_EQ(counts.occupied_total, 0u);
    EXPECT_EQ(counts.designated_total, 1u);

    // Transition to Stalled (Designated -> Stalled is valid)
    result = system.set_zone_state(5, 5, ZoneState::Stalled);
    EXPECT_TRUE(result);
    EXPECT_EQ(counts.designated_total, 0u);
    EXPECT_EQ(counts.stalled_total, 1u);
}

TEST(ZoneSystemTest, SetZoneStateOnEmpty) {
    ZoneSystem system(nullptr, nullptr, 128);

    // No zone at position
    bool result = system.set_zone_state(5, 5, ZoneState::Occupied);
    EXPECT_FALSE(result);
}
