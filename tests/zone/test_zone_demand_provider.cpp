/**
 * @file test_zone_demand_provider.cpp
 * @brief Tests for demand factor extension points / IDemandProvider (Ticket 4-017)
 *
 * Tests:
 * - Default uses internal demand calculation
 * - External provider overrides internal demand
 * - Null provider falls back to internal calculation
 * - External provider results clamped to [-100, +100]
 * - Switching between providers
 */

#include <gtest/gtest.h>
#include <sims3000/zone/ZoneSystem.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/building/ForwardDependencyStubs.h>

namespace sims3000 {
namespace zone {
namespace {

// ============================================================================
// Custom IDemandProvider for testing
// ============================================================================

class TestDemandProvider : public building::IDemandProvider {
public:
    TestDemandProvider()
        : m_habitation(0.0f)
        , m_exchange(0.0f)
        , m_fabrication(0.0f)
    {}

    void set_demands(float hab, float exc, float fab) {
        m_habitation = hab;
        m_exchange = exc;
        m_fabrication = fab;
    }

    float get_demand(std::uint8_t zone_type, std::uint32_t player_id) const override {
        (void)player_id;
        switch (zone_type) {
            case 0: return m_habitation;  // Habitation
            case 1: return m_exchange;    // Exchange
            case 2: return m_fabrication; // Fabrication
            default: return 0.0f;
        }
    }

private:
    float m_habitation;
    float m_exchange;
    float m_fabrication;
};

// ============================================================================
// Test Fixture
// ============================================================================

class ZoneDemandProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
    }

    std::unique_ptr<ZoneSystem> system;
    TestDemandProvider test_provider;
};

// ============================================================================
// Default uses internal demand calculation
// ============================================================================

TEST_F(ZoneDemandProviderTest, DefaultUsesInternalDemand) {
    EXPECT_FALSE(system->has_external_demand_provider());

    // Tick to populate internal demand values
    system->tick(0.016f);

    // Internal demand should use the DemandConfig formula
    auto demand = system->get_zone_demand(0);
    // The exact values depend on DemandConfig defaults, but they should be non-zero
    // since default config has non-zero base pressures and factors
    // (habitation_base=10, population_hab_factor=20, utility_factor=10, etc.)
    // Raw = 10 + 20 + 10 + 0 = 40 (below soft cap 80), so demand should be 40
    EXPECT_EQ(demand.habitation_demand, 40);
}

TEST_F(ZoneDemandProviderTest, DefaultGetDemandForType_UsesInternal) {
    system->tick(0.016f);

    // Should use internal calculation
    std::int8_t hab = system->get_demand_for_type(ZoneType::Habitation, 0);
    EXPECT_EQ(hab, 40); // base(10) + pop_hab(20) + utility(10) + tribute(0) = 40
}

// ============================================================================
// External provider overrides internal demand
// ============================================================================

TEST_F(ZoneDemandProviderTest, ExternalProviderOverrides) {
    test_provider.set_demands(75.0f, -30.0f, 50.0f);
    system->set_external_demand_provider(&test_provider);

    EXPECT_TRUE(system->has_external_demand_provider());

    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 75);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Exchange, 0), -30);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Fabrication, 0), 50);
}

TEST_F(ZoneDemandProviderTest, ExternalProviderOverrides_GetZoneDemand) {
    test_provider.set_demands(42.0f, -15.0f, 88.0f);
    system->set_external_demand_provider(&test_provider);

    auto demand = system->get_zone_demand(0);
    EXPECT_EQ(demand.habitation_demand, 42);
    EXPECT_EQ(demand.exchange_demand, -15);
    EXPECT_EQ(demand.fabrication_demand, 88);
}

TEST_F(ZoneDemandProviderTest, ExternalProviderOverrides_DifferentPlayers) {
    // The test provider ignores player_id, but we verify the system passes it through
    test_provider.set_demands(10.0f, 20.0f, 30.0f);
    system->set_external_demand_provider(&test_provider);

    auto demand0 = system->get_zone_demand(0);
    auto demand1 = system->get_zone_demand(1);

    // Same values since test provider ignores player_id
    EXPECT_EQ(demand0.habitation_demand, 10);
    EXPECT_EQ(demand1.habitation_demand, 10);
}

TEST_F(ZoneDemandProviderTest, ExternalProviderOverrides_IgnoresInternalCalculation) {
    // Tick to populate internal values
    system->tick(0.016f);

    // Internal demand for habitation should be 40
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 40);

    // Set external provider with different values
    test_provider.set_demands(-50.0f, -50.0f, -50.0f);
    system->set_external_demand_provider(&test_provider);

    // Now should use external values, not internal
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), -50);
}

// ============================================================================
// Null provider falls back to internal calculation
// ============================================================================

TEST_F(ZoneDemandProviderTest, NullProviderFallsBackToInternal) {
    // Set then clear external provider
    test_provider.set_demands(99.0f, 99.0f, 99.0f);
    system->set_external_demand_provider(&test_provider);
    EXPECT_TRUE(system->has_external_demand_provider());

    system->set_external_demand_provider(nullptr);
    EXPECT_FALSE(system->has_external_demand_provider());

    // Should use internal demand again
    system->tick(0.016f);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 40);
}

TEST_F(ZoneDemandProviderTest, NullProviderFallsBackToInternal_GetZoneDemand) {
    system->set_external_demand_provider(nullptr);
    system->tick(0.016f);

    auto demand = system->get_zone_demand(0);
    EXPECT_EQ(demand.habitation_demand, 40);
}

// ============================================================================
// External provider results clamped to [-100, +100]
// ============================================================================

TEST_F(ZoneDemandProviderTest, ClampedToPositive100) {
    test_provider.set_demands(200.0f, 150.0f, 999.0f);
    system->set_external_demand_provider(&test_provider);

    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 100);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Exchange, 0), 100);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Fabrication, 0), 100);
}

TEST_F(ZoneDemandProviderTest, ClampedToNegative100) {
    test_provider.set_demands(-200.0f, -150.0f, -999.0f);
    system->set_external_demand_provider(&test_provider);

    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), -100);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Exchange, 0), -100);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Fabrication, 0), -100);
}

TEST_F(ZoneDemandProviderTest, ExactBoundaryValues) {
    test_provider.set_demands(100.0f, -100.0f, 0.0f);
    system->set_external_demand_provider(&test_provider);

    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 100);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Exchange, 0), -100);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Fabrication, 0), 0);
}

TEST_F(ZoneDemandProviderTest, ClampedValues_InGetZoneDemand) {
    test_provider.set_demands(500.0f, -500.0f, 100.0f);
    system->set_external_demand_provider(&test_provider);

    auto demand = system->get_zone_demand(0);
    EXPECT_EQ(demand.habitation_demand, 100);
    EXPECT_EQ(demand.exchange_demand, -100);
    EXPECT_EQ(demand.fabrication_demand, 100);
}

// ============================================================================
// Switching between providers
// ============================================================================

TEST_F(ZoneDemandProviderTest, SwitchFromInternalToExternal) {
    system->tick(0.016f);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 40);

    test_provider.set_demands(77.0f, 77.0f, 77.0f);
    system->set_external_demand_provider(&test_provider);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 77);
}

TEST_F(ZoneDemandProviderTest, SwitchFromExternalToInternal) {
    test_provider.set_demands(77.0f, 77.0f, 77.0f);
    system->set_external_demand_provider(&test_provider);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 77);

    system->set_external_demand_provider(nullptr);
    system->tick(0.016f);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 40);
}

TEST_F(ZoneDemandProviderTest, SwitchBetweenTwoExternalProviders) {
    TestDemandProvider provider_a;
    provider_a.set_demands(10.0f, 20.0f, 30.0f);

    TestDemandProvider provider_b;
    provider_b.set_demands(60.0f, 70.0f, 80.0f);

    system->set_external_demand_provider(&provider_a);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 10);

    system->set_external_demand_provider(&provider_b);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 60);
}

// ============================================================================
// StubDemandProvider integration
// ============================================================================

TEST_F(ZoneDemandProviderTest, StubDemandProvider_Permissive) {
    building::StubDemandProvider stub;
    system->set_external_demand_provider(&stub);

    // Stub returns 1.0f by default
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), 1);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Exchange, 0), 1);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Fabrication, 0), 1);
}

TEST_F(ZoneDemandProviderTest, StubDemandProvider_Restrictive) {
    building::StubDemandProvider stub;
    stub.set_debug_restrictive(true);
    system->set_external_demand_provider(&stub);

    // Stub returns -1.0f when restrictive
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, 0), -1);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Exchange, 0), -1);
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Fabrication, 0), -1);
}

// ============================================================================
// Invalid player_id edge cases
// ============================================================================

TEST_F(ZoneDemandProviderTest, InvalidPlayerId_ReturnsZero) {
    test_provider.set_demands(50.0f, 50.0f, 50.0f);
    system->set_external_demand_provider(&test_provider);

    // Invalid player_id should return 0 regardless of provider
    EXPECT_EQ(system->get_demand_for_type(ZoneType::Habitation, MAX_OVERSEERS), 0);
}

TEST_F(ZoneDemandProviderTest, InvalidPlayerId_GetZoneDemand_ReturnsDefault) {
    test_provider.set_demands(50.0f, 50.0f, 50.0f);
    system->set_external_demand_provider(&test_provider);

    auto demand = system->get_zone_demand(MAX_OVERSEERS);
    EXPECT_EQ(demand.habitation_demand, 0);
    EXPECT_EQ(demand.exchange_demand, 0);
    EXPECT_EQ(demand.fabrication_demand, 0);
}

} // anonymous namespace
} // namespace zone
} // namespace sims3000
