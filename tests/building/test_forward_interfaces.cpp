/**
 * @file test_forward_interfaces.cpp
 * @brief Compile-time API verification for forward dependency interfaces
 *
 * This test verifies that all six forward dependency interfaces have the
 * expected API. Since these are pure abstract interfaces, we test by creating
 * minimal concrete implementations and calling all methods.
 */

#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <gtest/gtest.h>

using namespace sims3000::building;

// ============================================================================
// Minimal Concrete Implementations for Compile Testing
// ============================================================================

class TestEnergyProvider : public IEnergyProvider {
public:
    bool is_powered(std::uint32_t entity_id) const override { return true; }
    bool is_powered_at(std::uint32_t x, std::uint32_t y, std::uint32_t player_id) const override { return true; }
};

class TestFluidProvider : public IFluidProvider {
public:
    bool has_fluid(std::uint32_t entity_id) const override { return true; }
    bool has_fluid_at(std::uint32_t x, std::uint32_t y, std::uint32_t player_id) const override { return true; }
};

class TestTransportProvider : public ITransportProvider {
public:
    bool is_road_accessible_at(std::uint32_t x, std::uint32_t y, std::uint32_t max_distance) const override { return true; }
    std::uint32_t get_nearest_road_distance(std::uint32_t x, std::uint32_t y) const override { return 0; }
};

class TestLandValueProvider : public ILandValueProvider {
public:
    float get_land_value(std::uint32_t x, std::uint32_t y) const override { return 50.0f; }
};

class TestDemandProvider : public IDemandProvider {
public:
    float get_demand(std::uint8_t zone_type, std::uint32_t player_id) const override { return 1.0f; }
};

class TestCreditProvider : public ICreditProvider {
public:
    bool deduct_credits(std::uint32_t player_id, std::int64_t amount) override { return true; }
    bool has_credits(std::uint32_t player_id, std::int64_t amount) const override { return true; }
};

// ============================================================================
// API Verification Tests
// ============================================================================

TEST(ForwardInterfacesTest, IEnergyProviderAPI) {
    TestEnergyProvider provider;

    // Verify all methods compile and are callable
    EXPECT_TRUE(provider.is_powered(123));
    EXPECT_TRUE(provider.is_powered_at(10, 20, 1));
}

TEST(ForwardInterfacesTest, IFluidProviderAPI) {
    TestFluidProvider provider;

    // Verify all methods compile and are callable
    EXPECT_TRUE(provider.has_fluid(123));
    EXPECT_TRUE(provider.has_fluid_at(10, 20, 1));
}

TEST(ForwardInterfacesTest, ITransportProviderAPI) {
    TestTransportProvider provider;

    // Verify all methods compile and are callable
    EXPECT_TRUE(provider.is_road_accessible_at(10, 20, 3));
    EXPECT_EQ(provider.get_nearest_road_distance(10, 20), 0u);
}

TEST(ForwardInterfacesTest, ILandValueProviderAPI) {
    TestLandValueProvider provider;

    // Verify method compiles and is callable
    EXPECT_FLOAT_EQ(provider.get_land_value(10, 20), 50.0f);
}

TEST(ForwardInterfacesTest, IDemandProviderAPI) {
    TestDemandProvider provider;

    // Verify method compiles and is callable
    EXPECT_FLOAT_EQ(provider.get_demand(0, 1), 1.0f);
}

TEST(ForwardInterfacesTest, ICreditProviderAPI) {
    TestCreditProvider provider;

    // Verify all methods compile and are callable
    EXPECT_TRUE(provider.deduct_credits(1, 1000));
    EXPECT_TRUE(provider.has_credits(1, 1000));
}

// ============================================================================
// Polymorphic Behavior Tests
// ============================================================================

TEST(ForwardInterfacesTest, PolymorphicEnergyProvider) {
    TestEnergyProvider concrete;
    IEnergyProvider* interface = &concrete;

    // Verify polymorphic calls work
    EXPECT_TRUE(interface->is_powered(123));
    EXPECT_TRUE(interface->is_powered_at(10, 20, 1));
}

TEST(ForwardInterfacesTest, PolymorphicFluidProvider) {
    TestFluidProvider concrete;
    IFluidProvider* interface = &concrete;

    // Verify polymorphic calls work
    EXPECT_TRUE(interface->has_fluid(123));
    EXPECT_TRUE(interface->has_fluid_at(10, 20, 1));
}

TEST(ForwardInterfacesTest, PolymorphicTransportProvider) {
    TestTransportProvider concrete;
    ITransportProvider* interface = &concrete;

    // Verify polymorphic calls work
    EXPECT_TRUE(interface->is_road_accessible_at(10, 20, 3));
    EXPECT_EQ(interface->get_nearest_road_distance(10, 20), 0u);
}

TEST(ForwardInterfacesTest, PolymorphicLandValueProvider) {
    TestLandValueProvider concrete;
    ILandValueProvider* interface = &concrete;

    // Verify polymorphic calls work
    EXPECT_FLOAT_EQ(interface->get_land_value(10, 20), 50.0f);
}

TEST(ForwardInterfacesTest, PolymorphicDemandProvider) {
    TestDemandProvider concrete;
    IDemandProvider* interface = &concrete;

    // Verify polymorphic calls work
    EXPECT_FLOAT_EQ(interface->get_demand(0, 1), 1.0f);
}

TEST(ForwardInterfacesTest, PolymorphicCreditProvider) {
    TestCreditProvider concrete;
    ICreditProvider* interface = &concrete;

    // Verify polymorphic calls work
    EXPECT_TRUE(interface->deduct_credits(1, 1000));
    EXPECT_TRUE(interface->has_credits(1, 1000));
}

// ============================================================================
// Destructor Tests
// ============================================================================

TEST(ForwardInterfacesTest, VirtualDestructors) {
    // Verify all interfaces have virtual destructors by deleting through base pointer
    {
        IEnergyProvider* p = new TestEnergyProvider();
        delete p; // Should not leak
    }
    {
        IFluidProvider* p = new TestFluidProvider();
        delete p; // Should not leak
    }
    {
        ITransportProvider* p = new TestTransportProvider();
        delete p; // Should not leak
    }
    {
        ILandValueProvider* p = new TestLandValueProvider();
        delete p; // Should not leak
    }
    {
        IDemandProvider* p = new TestDemandProvider();
        delete p; // Should not leak
    }
    {
        ICreditProvider* p = new TestCreditProvider();
        delete p; // Should not leak
    }
}
