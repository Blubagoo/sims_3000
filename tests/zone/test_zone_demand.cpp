/**
 * @file test_zone_demand.cpp
 * @brief Tests for basic demand calculation / growth pressure (Ticket 4-016)
 *
 * Tests:
 * - Initial demand with no zones (should be base + factors)
 * - Demand with occupied zones (saturation reduces demand)
 * - Soft cap behavior
 * - Negative demand possible
 * - Clamping at -100/+100
 * - Configurable parameters
 * - Per-overseer independence
 * - get_zone_demand returns correct data
 */

#include <gtest/gtest.h>
#include <sims3000/zone/ZoneSystem.h>

namespace sims3000 {
namespace zone {
namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class ZoneDemandTest : public ::testing::Test {
protected:
    void SetUp() override {
        system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
    }

    /// Helper: place a zone and transition it to Occupied
    void place_occupied_zone(std::int32_t x, std::int32_t y,
                              ZoneType type, std::uint8_t player_id) {
        system->place_zone(x, y, type, ZoneDensity::LowDensity,
                           player_id, next_entity_id++);
        system->set_zone_state(x, y, ZoneState::Occupied);
    }

    /// Helper: tick to update demand
    void tick_once() {
        system->tick(0.016f);
    }

    std::unique_ptr<ZoneSystem> system;
    std::uint32_t next_entity_id = 100;
};

// ============================================================================
// Initial demand with no zones (base + factors)
// ============================================================================

TEST_F(ZoneDemandTest, InitialDemand_NoZones_DefaultConfig) {
    // Default DemandConfig:
    // hab: base=10, pop_hab=20, utility=10, tribute=0 => 10+20+10+0 = 40
    // exc: base=5, pop_exc=10, employment=0, utility=10, tribute=0 => 5+10+0+10+0 = 25
    // fab: base=5, pop_fab=10, employment=0, utility=10, tribute=0 => 5+10+0+10+0 = 25
    // No occupied zones => saturation = 0

    tick_once();

    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 40);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Exchange, 0), 25);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Fabrication, 0), 25);
}

TEST_F(ZoneDemandTest, InitialDemand_AllOverseers) {
    tick_once();

    // All overseers should have the same initial demand (no zones for any)
    for (std::uint8_t pid = 0; pid < MAX_OVERSEERS; ++pid) {
        EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, pid), 40)
            << "Overseer " << static_cast<int>(pid);
    }
}

// ============================================================================
// Demand with occupied zones (saturation reduces demand)
// ============================================================================

TEST_F(ZoneDemandTest, Saturation_ReducesDemand) {
    // Place 25 occupied habitation zones for player 0
    // Saturation = (25 * 100) / 50 = 50
    // Hab demand = 40 - 50 = -10
    for (int i = 0; i < 25; ++i) {
        place_occupied_zone(i, 0, ZoneType::Habitation, 0);
    }

    tick_once();

    std::int8_t hab_demand = system->get_demand_for_type(ZoneType::Habitation, 0);
    // With 25 occupied zones total:
    // saturation = (25 * 100) / 50 = 50
    // raw = 40 - 50 = -10
    EXPECT_EQ(hab_demand, -10);
}

TEST_F(ZoneDemandTest, Saturation_MoreZonesMoreSaturation) {
    // Place 50 occupied zones => saturation = 100
    // Hab demand = 40 - 100 = -60
    for (int i = 0; i < 50; ++i) {
        place_occupied_zone(i, 0, ZoneType::Habitation, 0);
    }

    tick_once();

    std::int8_t hab_demand = system->get_demand_for_type(ZoneType::Habitation, 0);
    // raw = 40 - 100 = -60 (within -80..80 so no soft cap)
    EXPECT_EQ(hab_demand, -60);
}

// ============================================================================
// Soft cap behavior
// ============================================================================

TEST_F(ZoneDemandTest, SoftCap_AppliedAboveThreshold) {
    // Set config so raw demand exceeds soft cap threshold
    DemandConfig cfg;
    cfg.habitation_base = 50;
    cfg.population_hab_factor = 50;
    cfg.utility_factor = 0;
    cfg.tribute_factor = 0;
    cfg.soft_cap_threshold = 80;
    cfg.target_zone_count = 50;
    system->set_demand_config(cfg);

    // Raw hab = 50 + 50 = 100 (above threshold 80)
    // Soft cap: 80 + (100 - 80) / 2 = 80 + 10 = 90
    tick_once();

    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 90);
}

TEST_F(ZoneDemandTest, SoftCap_NotAppliedBelowThreshold) {
    DemandConfig cfg;
    cfg.habitation_base = 30;
    cfg.population_hab_factor = 20;
    cfg.utility_factor = 0;
    cfg.tribute_factor = 0;
    cfg.soft_cap_threshold = 80;
    cfg.target_zone_count = 50;
    system->set_demand_config(cfg);

    // Raw hab = 30 + 20 = 50 (below threshold 80)
    tick_once();

    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 50);
}

TEST_F(ZoneDemandTest, SoftCap_NegativeDirection) {
    DemandConfig cfg;
    cfg.habitation_base = -50;
    cfg.population_hab_factor = -50;
    cfg.utility_factor = 0;
    cfg.tribute_factor = 0;
    cfg.soft_cap_threshold = 80;
    cfg.target_zone_count = 50;
    system->set_demand_config(cfg);

    // Raw hab = -50 + -50 = -100 (below -threshold = -80)
    // Soft cap: -80 + (-100 + 80) / 2 = -80 + (-10) = -90
    tick_once();

    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), -90);
}

// ============================================================================
// Clamping at -100/+100
// ============================================================================

TEST_F(ZoneDemandTest, Clamping_PositiveMax) {
    DemandConfig cfg;
    cfg.habitation_base = 100;
    cfg.population_hab_factor = 100;
    cfg.utility_factor = 100;
    cfg.tribute_factor = 0;
    cfg.soft_cap_threshold = 127; // Set high so soft cap doesn't interfere
    cfg.target_zone_count = 50;
    system->set_demand_config(cfg);

    tick_once();

    // Raw = 100 + 100 + 100 = 300, clamped to 100
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 100);
}

TEST_F(ZoneDemandTest, Clamping_NegativeMax) {
    DemandConfig cfg;
    cfg.habitation_base = -100;
    cfg.population_hab_factor = -100;
    cfg.utility_factor = 0;
    cfg.tribute_factor = 0;
    cfg.soft_cap_threshold = 127; // Set high so soft cap doesn't interfere
    cfg.target_zone_count = 50;
    system->set_demand_config(cfg);

    tick_once();

    // Raw = -100 + -100 = -200, clamped to -100
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), -100);
}

// ============================================================================
// Negative demand is possible
// ============================================================================

TEST_F(ZoneDemandTest, NegativeDemand_WithHighSaturation) {
    // Default config: hab demand = 40 with no zones
    // Place enough occupied zones to push saturation above 40
    // saturation = (occupied * 100) / 50
    // Need occupied > 20 for saturation > 40

    for (int i = 0; i < 30; ++i) {
        place_occupied_zone(i, 0, ZoneType::Habitation, 0);
    }

    tick_once();

    // saturation = (30 * 100) / 50 = 60
    // raw = 40 - 60 = -20
    std::int8_t hab_demand = system->get_demand_for_type(ZoneType::Habitation, 0);
    EXPECT_LT(hab_demand, 0);
    EXPECT_EQ(hab_demand, -20);
}

// ============================================================================
// Configurable parameters
// ============================================================================

TEST_F(ZoneDemandTest, ConfigurableParams_CustomBase) {
    DemandConfig cfg;
    cfg.habitation_base = 20;
    cfg.exchange_base = 15;
    cfg.fabrication_base = 10;
    cfg.population_hab_factor = 0;
    cfg.population_exc_factor = 0;
    cfg.population_fab_factor = 0;
    cfg.employment_factor = 0;
    cfg.utility_factor = 0;
    cfg.tribute_factor = 0;
    cfg.target_zone_count = 50;
    cfg.soft_cap_threshold = 80;
    system->set_demand_config(cfg);

    tick_once();

    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 20);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Exchange, 0), 15);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Fabrication, 0), 10);
}

TEST_F(ZoneDemandTest, ConfigurableParams_TargetZoneCount) {
    DemandConfig cfg;
    cfg.habitation_base = 0;
    cfg.population_hab_factor = 0;
    cfg.population_exc_factor = 0;
    cfg.population_fab_factor = 0;
    cfg.employment_factor = 0;
    cfg.utility_factor = 0;
    cfg.tribute_factor = 0;
    cfg.target_zone_count = 10; // Much smaller target
    cfg.soft_cap_threshold = 127;
    system->set_demand_config(cfg);

    // Place 10 occupied zones => saturation = (10*100)/10 = 100
    // demand = 0 + 0 - 100 = -100
    for (int i = 0; i < 10; ++i) {
        place_occupied_zone(i, 0, ZoneType::Habitation, 0);
    }

    tick_once();

    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), -100);
}

TEST_F(ZoneDemandTest, ConfigurableParams_GetConfig) {
    DemandConfig cfg;
    cfg.habitation_base = 42;
    system->set_demand_config(cfg);

    const DemandConfig& retrieved = system->get_demand_config();
    EXPECT_EQ(retrieved.habitation_base, 42);
}

// ============================================================================
// Per-overseer independence
// ============================================================================

TEST_F(ZoneDemandTest, PerOverseer_Independent) {
    // Overseer 0 has occupied zones, overseer 1 does not
    for (int i = 0; i < 25; ++i) {
        place_occupied_zone(i, 0, ZoneType::Habitation, 0);
    }

    tick_once();

    std::int8_t demand_p0 = system->get_demand_for_type(ZoneType::Habitation, 0);
    std::int8_t demand_p1 = system->get_demand_for_type(ZoneType::Habitation, 1);

    // Player 0: saturation = (25*100)/50 = 50, demand = 40 - 50 = -10
    EXPECT_EQ(demand_p0, -10);
    // Player 1: no zones, demand = 40
    EXPECT_EQ(demand_p1, 40);
}

TEST_F(ZoneDemandTest, PerOverseer_DifferentZoneCounts) {
    // Overseer 0: 10 zones
    for (int i = 0; i < 10; ++i) {
        place_occupied_zone(i, 0, ZoneType::Exchange, 0);
    }
    // Overseer 2: 30 zones
    for (int i = 0; i < 30; ++i) {
        place_occupied_zone(i, 1, ZoneType::Exchange, 2);
    }

    tick_once();

    // Overseer 0: saturation = (10*100)/50 = 20, exc demand = 25 - 20 = 5
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Exchange, 0), 5);
    // Overseer 2: saturation = (30*100)/50 = 60, exc demand = 25 - 60 = -35
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Exchange, 2), -35);
}

// ============================================================================
// get_zone_demand returns correct data
// ============================================================================

TEST_F(ZoneDemandTest, GetZoneDemand_ReturnsCorrectData) {
    tick_once();

    ZoneDemandData demand = system->get_zone_demand(0);
    EXPECT_EQ(demand.habitation_demand, 40);
    EXPECT_EQ(demand.exchange_demand, 25);
    EXPECT_EQ(demand.fabrication_demand, 25);
}

TEST_F(ZoneDemandTest, GetZoneDemand_InvalidOverseer) {
    tick_once();

    ZoneDemandData demand = system->get_zone_demand(MAX_OVERSEERS);
    EXPECT_EQ(demand.habitation_demand, 0);
    EXPECT_EQ(demand.exchange_demand, 0);
    EXPECT_EQ(demand.fabrication_demand, 0);
}

TEST_F(ZoneDemandTest, GetZoneDemand_MatchesGetDemandForType) {
    for (int i = 0; i < 15; ++i) {
        place_occupied_zone(i, 0, ZoneType::Habitation, 0);
    }

    tick_once();

    ZoneDemandData demand = system->get_zone_demand(0);
    EXPECT_EQ(demand.habitation_demand,
              system->get_demand_for_type(ZoneType::Habitation, 0));
    EXPECT_EQ(demand.exchange_demand,
              system->get_demand_for_type(ZoneType::Exchange, 0));
    EXPECT_EQ(demand.fabrication_demand,
              system->get_demand_for_type(ZoneType::Fabrication, 0));
}

// ============================================================================
// get_demand_for_type invalid overseer
// ============================================================================

TEST_F(ZoneDemandTest, GetDemandForType_InvalidOverseer) {
    tick_once();
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, MAX_OVERSEERS), 0);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 255), 0);
}

// ============================================================================
// Demand updates on tick
// ============================================================================

TEST_F(ZoneDemandTest, DemandUpdatesOnTick) {
    // Before any tick, demand should be 0 (initial state)
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 0);

    // After tick, demand should be calculated
    tick_once();
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 40);
}

// ============================================================================
// Stalled zones do not count as occupied for saturation
// ============================================================================

TEST_F(ZoneDemandTest, StalledZones_NotCountedAsSupply) {
    // Place zones and stall them (not occupied)
    for (int i = 0; i < 25; ++i) {
        system->place_zone(i, 0, ZoneType::Habitation, ZoneDensity::LowDensity,
                           0, next_entity_id++);
        system->set_zone_state(i, 0, ZoneState::Stalled);
    }

    tick_once();

    // Stalled zones are not occupied, so occupied_total = 0, saturation = 0
    // Demand should be the same as with no zones
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 40);
}

} // anonymous namespace
} // namespace zone
} // namespace sims3000
