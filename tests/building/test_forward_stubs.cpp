/**
 * @file test_forward_stubs.cpp
 * @brief Tests for forward dependency stub implementations (ticket 4-020)
 */

#include <sims3000/building/ForwardDependencyStubs.h>
#include <gtest/gtest.h>

using namespace sims3000::building;

// ============================================================================
// StubEnergyProvider Tests
// ============================================================================

TEST(ForwardStubsTest, EnergyProviderPermissiveDefaults) {
    StubEnergyProvider stub;

    EXPECT_TRUE(stub.is_powered(123));
    EXPECT_TRUE(stub.is_powered_at(10, 20, 1));
    EXPECT_FALSE(stub.is_debug_restrictive());
}

TEST(ForwardStubsTest, EnergyProviderRestrictiveMode) {
    StubEnergyProvider stub;
    stub.set_debug_restrictive(true);

    EXPECT_FALSE(stub.is_powered(123));
    EXPECT_FALSE(stub.is_powered_at(10, 20, 1));
    EXPECT_TRUE(stub.is_debug_restrictive());

    // Toggle back
    stub.set_debug_restrictive(false);
    EXPECT_TRUE(stub.is_powered(123));
    EXPECT_TRUE(stub.is_powered_at(10, 20, 1));
}

// ============================================================================
// StubFluidProvider Tests
// ============================================================================

TEST(ForwardStubsTest, FluidProviderPermissiveDefaults) {
    StubFluidProvider stub;

    EXPECT_TRUE(stub.has_fluid(123));
    EXPECT_TRUE(stub.has_fluid_at(10, 20, 1));
    EXPECT_FALSE(stub.is_debug_restrictive());
}

TEST(ForwardStubsTest, FluidProviderRestrictiveMode) {
    StubFluidProvider stub;
    stub.set_debug_restrictive(true);

    EXPECT_FALSE(stub.has_fluid(123));
    EXPECT_FALSE(stub.has_fluid_at(10, 20, 1));
    EXPECT_TRUE(stub.is_debug_restrictive());
}

// ============================================================================
// StubTransportProvider Tests
// ============================================================================

TEST(ForwardStubsTest, TransportProviderPermissiveDefaults) {
    StubTransportProvider stub;

    EXPECT_TRUE(stub.is_road_accessible_at(10, 20, 3));
    EXPECT_EQ(stub.get_nearest_road_distance(10, 20), 0u);
    EXPECT_FALSE(stub.is_debug_restrictive());
}

TEST(ForwardStubsTest, TransportProviderRestrictiveMode) {
    StubTransportProvider stub;
    stub.set_debug_restrictive(true);

    EXPECT_FALSE(stub.is_road_accessible_at(10, 20, 3));
    EXPECT_EQ(stub.get_nearest_road_distance(10, 20), 255u);
    EXPECT_TRUE(stub.is_debug_restrictive());
}

// ============================================================================
// StubLandValueProvider Tests
// ============================================================================

TEST(ForwardStubsTest, LandValueProviderPermissiveDefaults) {
    StubLandValueProvider stub;

    EXPECT_FLOAT_EQ(stub.get_land_value(10, 20), 50.0f);
    EXPECT_FALSE(stub.is_debug_restrictive());
}

TEST(ForwardStubsTest, LandValueProviderRestrictiveMode) {
    StubLandValueProvider stub;
    stub.set_debug_restrictive(true);

    EXPECT_FLOAT_EQ(stub.get_land_value(10, 20), 0.0f);
    EXPECT_TRUE(stub.is_debug_restrictive());
}

// ============================================================================
// StubDemandProvider Tests
// ============================================================================

TEST(ForwardStubsTest, DemandProviderPermissiveDefaults) {
    StubDemandProvider stub;

    EXPECT_FLOAT_EQ(stub.get_demand(0, 1), 1.0f);
    EXPECT_FLOAT_EQ(stub.get_demand(1, 2), 1.0f);
    EXPECT_FLOAT_EQ(stub.get_demand(2, 3), 1.0f);
    EXPECT_FALSE(stub.is_debug_restrictive());
}

TEST(ForwardStubsTest, DemandProviderRestrictiveMode) {
    StubDemandProvider stub;
    stub.set_debug_restrictive(true);

    EXPECT_FLOAT_EQ(stub.get_demand(0, 1), -1.0f);
    EXPECT_TRUE(stub.is_debug_restrictive());
}

// ============================================================================
// StubCreditProvider Tests
// ============================================================================

TEST(ForwardStubsTest, CreditProviderPermissiveDefaults) {
    StubCreditProvider stub;

    EXPECT_TRUE(stub.deduct_credits(1, 1000));
    EXPECT_TRUE(stub.has_credits(1, 1000));
    EXPECT_FALSE(stub.is_debug_restrictive());
}

TEST(ForwardStubsTest, CreditProviderRestrictiveMode) {
    StubCreditProvider stub;
    stub.set_debug_restrictive(true);

    EXPECT_FALSE(stub.deduct_credits(1, 1000));
    EXPECT_FALSE(stub.has_credits(1, 1000));
    EXPECT_TRUE(stub.is_debug_restrictive());
}

// ============================================================================
// Polymorphic Usage Tests
// ============================================================================

TEST(ForwardStubsTest, PolymorphicEnergyProvider) {
    StubEnergyProvider stub;
    IEnergyProvider* iface = &stub;

    EXPECT_TRUE(iface->is_powered(42));
    EXPECT_TRUE(iface->is_powered_at(0, 0, 0));
}

TEST(ForwardStubsTest, PolymorphicFluidProvider) {
    StubFluidProvider stub;
    IFluidProvider* iface = &stub;

    EXPECT_TRUE(iface->has_fluid(42));
    EXPECT_TRUE(iface->has_fluid_at(0, 0, 0));
}

TEST(ForwardStubsTest, PolymorphicTransportProvider) {
    StubTransportProvider stub;
    ITransportProvider* iface = &stub;

    EXPECT_TRUE(iface->is_road_accessible_at(0, 0, 3));
    EXPECT_EQ(iface->get_nearest_road_distance(0, 0), 0u);
}

TEST(ForwardStubsTest, PolymorphicLandValueProvider) {
    StubLandValueProvider stub;
    ILandValueProvider* iface = &stub;

    EXPECT_FLOAT_EQ(iface->get_land_value(0, 0), 50.0f);
}

TEST(ForwardStubsTest, PolymorphicDemandProvider) {
    StubDemandProvider stub;
    IDemandProvider* iface = &stub;

    EXPECT_FLOAT_EQ(iface->get_demand(0, 1), 1.0f);
}

TEST(ForwardStubsTest, PolymorphicCreditProvider) {
    StubCreditProvider stub;
    ICreditProvider* iface = &stub;

    EXPECT_TRUE(iface->deduct_credits(1, 500));
    EXPECT_TRUE(iface->has_credits(1, 500));
}
