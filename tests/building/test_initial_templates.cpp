/**
 * @file test_initial_templates.cpp
 * @brief Tests for initial 30 building templates (ticket 4-023)
 */

#include <sims3000/building/InitialTemplates.h>
#include <sims3000/building/BuildingTemplate.h>
#include <gtest/gtest.h>

using namespace sims3000::building;

class InitialTemplatesTest : public ::testing::Test {
protected:
    void SetUp() override {
        register_initial_templates(registry);
    }

    BuildingTemplateRegistry registry;
};

// ============================================================================
// Total Count Tests
// ============================================================================

TEST_F(InitialTemplatesTest, ThirtyTemplatesRegistered) {
    EXPECT_EQ(registry.get_template_count(), 30u);
}

TEST_F(InitialTemplatesTest, TemplateIDsOneToThirty) {
    for (std::uint32_t id = 1; id <= 30; ++id) {
        EXPECT_TRUE(registry.has_template(id))
            << "Missing template ID: " << id;
    }
}

// ============================================================================
// Pool Distribution Tests (5 per pool)
// ============================================================================

TEST_F(InitialTemplatesTest, FivePerPoolHabitationLow) {
    auto templates = registry.get_templates_for_pool(
        ZoneBuildingType::Habitation, DensityLevel::Low);
    EXPECT_EQ(templates.size(), 5u);
}

TEST_F(InitialTemplatesTest, FivePerPoolHabitationHigh) {
    auto templates = registry.get_templates_for_pool(
        ZoneBuildingType::Habitation, DensityLevel::High);
    EXPECT_EQ(templates.size(), 5u);
}

TEST_F(InitialTemplatesTest, FivePerPoolExchangeLow) {
    auto templates = registry.get_templates_for_pool(
        ZoneBuildingType::Exchange, DensityLevel::Low);
    EXPECT_EQ(templates.size(), 5u);
}

TEST_F(InitialTemplatesTest, FivePerPoolExchangeHigh) {
    auto templates = registry.get_templates_for_pool(
        ZoneBuildingType::Exchange, DensityLevel::High);
    EXPECT_EQ(templates.size(), 5u);
}

TEST_F(InitialTemplatesTest, FivePerPoolFabricationLow) {
    auto templates = registry.get_templates_for_pool(
        ZoneBuildingType::Fabrication, DensityLevel::Low);
    EXPECT_EQ(templates.size(), 5u);
}

TEST_F(InitialTemplatesTest, FivePerPoolFabricationHigh) {
    auto templates = registry.get_templates_for_pool(
        ZoneBuildingType::Fabrication, DensityLevel::High);
    EXPECT_EQ(templates.size(), 5u);
}

// ============================================================================
// Template Name Tests
// ============================================================================

TEST_F(InitialTemplatesTest, HabitationLowNames) {
    EXPECT_EQ(registry.get_template(1).name, "dwelling-pod-alpha");
    EXPECT_EQ(registry.get_template(2).name, "dwelling-pod-beta");
    EXPECT_EQ(registry.get_template(3).name, "hab-cell-standard");
    EXPECT_EQ(registry.get_template(4).name, "hab-cell-compact");
    EXPECT_EQ(registry.get_template(5).name, "micro-dwelling");
}

TEST_F(InitialTemplatesTest, HabitationHighNames) {
    EXPECT_EQ(registry.get_template(6).name, "hab-spire-minor");
    EXPECT_EQ(registry.get_template(7).name, "hab-spire-major");
    EXPECT_EQ(registry.get_template(8).name, "hab-tower-standard");
    EXPECT_EQ(registry.get_template(9).name, "communal-nexus");
    EXPECT_EQ(registry.get_template(10).name, "hab-complex-alpha");
}

TEST_F(InitialTemplatesTest, FabricationLowNames) {
    EXPECT_EQ(registry.get_template(21).name, "fabricator-pod-alpha");
    EXPECT_EQ(registry.get_template(22).name, "fabricator-pod-beta");
    EXPECT_EQ(registry.get_template(23).name, "assembly-cell");
    EXPECT_EQ(registry.get_template(24).name, "forge-pod");
    EXPECT_EQ(registry.get_template(25).name, "workshop-node");
}

// ============================================================================
// Capacity Range Tests
// ============================================================================

TEST_F(InitialTemplatesTest, HabitationLowCapacityRange) {
    // Habitation low: 4-12
    for (std::uint32_t id = 1; id <= 5; ++id) {
        const auto& t = registry.get_template(id);
        EXPECT_GE(t.base_capacity, 4u) << "Template " << id << " capacity too low";
        EXPECT_LE(t.base_capacity, 12u) << "Template " << id << " capacity too high";
    }
}

TEST_F(InitialTemplatesTest, HabitationHighCapacityRange) {
    // Habitation high: 40-200
    for (std::uint32_t id = 6; id <= 10; ++id) {
        const auto& t = registry.get_template(id);
        EXPECT_GE(t.base_capacity, 40u) << "Template " << id << " capacity too low";
        EXPECT_LE(t.base_capacity, 200u) << "Template " << id << " capacity too high";
    }
}

TEST_F(InitialTemplatesTest, ExchangeLowCapacityRange) {
    // Exchange low: 2-6
    for (std::uint32_t id = 11; id <= 15; ++id) {
        const auto& t = registry.get_template(id);
        EXPECT_GE(t.base_capacity, 2u) << "Template " << id << " capacity too low";
        EXPECT_LE(t.base_capacity, 6u) << "Template " << id << " capacity too high";
    }
}

TEST_F(InitialTemplatesTest, ExchangeHighCapacityRange) {
    // Exchange high: 20-80
    for (std::uint32_t id = 16; id <= 20; ++id) {
        const auto& t = registry.get_template(id);
        EXPECT_GE(t.base_capacity, 20u) << "Template " << id << " capacity too low";
        EXPECT_LE(t.base_capacity, 80u) << "Template " << id << " capacity too high";
    }
}

TEST_F(InitialTemplatesTest, FabricationLowCapacityRange) {
    // Fabrication low: 4-10
    for (std::uint32_t id = 21; id <= 25; ++id) {
        const auto& t = registry.get_template(id);
        EXPECT_GE(t.base_capacity, 4u) << "Template " << id << " capacity too low";
        EXPECT_LE(t.base_capacity, 10u) << "Template " << id << " capacity too high";
    }
}

TEST_F(InitialTemplatesTest, FabricationHighCapacityRange) {
    // Fabrication high: 30-120
    for (std::uint32_t id = 26; id <= 30; ++id) {
        const auto& t = registry.get_template(id);
        EXPECT_GE(t.base_capacity, 30u) << "Template " << id << " capacity too low";
        EXPECT_LE(t.base_capacity, 120u) << "Template " << id << " capacity too high";
    }
}

// ============================================================================
// Contamination Tests
// ============================================================================

TEST_F(InitialTemplatesTest, FabricationHasContamination) {
    // All fabrication templates (IDs 21-30) should have contamination > 0
    for (std::uint32_t id = 21; id <= 30; ++id) {
        const auto& t = registry.get_template(id);
        EXPECT_GT(t.contamination_output, 0u)
            << "Fabrication template " << id << " (" << t.name
            << ") should have contamination > 0";
    }
}

TEST_F(InitialTemplatesTest, NonFabricationNoContamination) {
    // Habitation and Exchange templates (IDs 1-20) should have 0 contamination
    for (std::uint32_t id = 1; id <= 20; ++id) {
        const auto& t = registry.get_template(id);
        EXPECT_EQ(t.contamination_output, 0u)
            << "Non-fabrication template " << id << " (" << t.name
            << ") should have 0 contamination";
    }
}

// ============================================================================
// Construction Ticks Tests
// ============================================================================

TEST_F(InitialTemplatesTest, LowDensityConstructionTicksRange) {
    // Low density: 40-80
    for (std::uint32_t id : {1u, 2u, 3u, 4u, 5u, 11u, 12u, 13u, 14u, 15u, 21u, 22u, 23u, 24u, 25u}) {
        const auto& t = registry.get_template(id);
        EXPECT_GE(t.construction_ticks, 40u) << "Template " << id << " ticks too low";
        EXPECT_LE(t.construction_ticks, 80u) << "Template " << id << " ticks too high";
    }
}

TEST_F(InitialTemplatesTest, HighDensityConstructionTicksRange) {
    // High density: 100-200
    for (std::uint32_t id : {6u, 7u, 8u, 9u, 10u, 16u, 17u, 18u, 19u, 20u, 26u, 27u, 28u, 29u, 30u}) {
        const auto& t = registry.get_template(id);
        EXPECT_GE(t.construction_ticks, 100u) << "Template " << id << " ticks too low";
        EXPECT_LE(t.construction_ticks, 200u) << "Template " << id << " ticks too high";
    }
}

// ============================================================================
// Footprint Tests
// ============================================================================

TEST_F(InitialTemplatesTest, LowDensityIs1x1) {
    for (std::uint32_t id : {1u, 2u, 3u, 4u, 5u, 11u, 12u, 13u, 14u, 15u, 21u, 22u, 23u, 24u, 25u}) {
        const auto& t = registry.get_template(id);
        EXPECT_EQ(t.footprint_w, 1u) << "Template " << id << " should be 1x1";
        EXPECT_EQ(t.footprint_h, 1u) << "Template " << id << " should be 1x1";
    }
}

TEST_F(InitialTemplatesTest, HighDensityHasAt2x2PerType) {
    // Each high-density type should have at least one 2x2
    bool hab_has_2x2 = false;
    bool exch_has_2x2 = false;
    bool fab_has_2x2 = false;

    for (std::uint32_t id = 6; id <= 10; ++id) {
        const auto& t = registry.get_template(id);
        if (t.footprint_w == 2 && t.footprint_h == 2) hab_has_2x2 = true;
    }
    for (std::uint32_t id = 16; id <= 20; ++id) {
        const auto& t = registry.get_template(id);
        if (t.footprint_w == 2 && t.footprint_h == 2) exch_has_2x2 = true;
    }
    for (std::uint32_t id = 26; id <= 30; ++id) {
        const auto& t = registry.get_template(id);
        if (t.footprint_w == 2 && t.footprint_h == 2) fab_has_2x2 = true;
    }

    EXPECT_TRUE(hab_has_2x2) << "Habitation high should have at least one 2x2";
    EXPECT_TRUE(exch_has_2x2) << "Exchange high should have at least one 2x2";
    EXPECT_TRUE(fab_has_2x2) << "Fabrication high should have at least one 2x2";
}

// ============================================================================
// Color Accent Tests
// ============================================================================

TEST_F(InitialTemplatesTest, AllHaveFourColorAccents) {
    for (std::uint32_t id = 1; id <= 30; ++id) {
        const auto& t = registry.get_template(id);
        EXPECT_EQ(t.color_accent_count, 4u)
            << "Template " << id << " should have 4 color accents";
    }
}
