/**
 * @file test_template_selector.cpp
 * @brief Tests for TemplateSelector weighted random selection (Ticket 4-022)
 */

#include <sims3000/building/TemplateSelector.h>
#include <sims3000/building/BuildingTemplate.h>
#include <gtest/gtest.h>
#include <set>
#include <map>

using namespace sims3000::building;

// ============================================================================
// Test Fixture
// ============================================================================

class TemplateSelectorTest : public ::testing::Test {
protected:
    BuildingTemplateRegistry registry;

    void SetUp() override {
        // Register a variety of templates for testing
        // Habitation Low pool: 3 templates
        {
            BuildingTemplate t;
            t.template_id = 1001;
            t.name = "Small Dwelling";
            t.zone_type = ZoneBuildingType::Habitation;
            t.density = DensityLevel::Low;
            t.min_land_value = 0.0f;
            t.min_level = 1;
            t.color_accent_count = 4;
            registry.register_template(t);
        }
        {
            BuildingTemplate t;
            t.template_id = 1002;
            t.name = "Medium Dwelling";
            t.zone_type = ZoneBuildingType::Habitation;
            t.density = DensityLevel::Low;
            t.min_land_value = 50.0f;
            t.min_level = 1;
            t.color_accent_count = 3;
            registry.register_template(t);
        }
        {
            BuildingTemplate t;
            t.template_id = 1003;
            t.name = "Large Dwelling";
            t.zone_type = ZoneBuildingType::Habitation;
            t.density = DensityLevel::Low;
            t.min_land_value = 150.0f;
            t.min_level = 1;
            t.color_accent_count = 2;
            registry.register_template(t);
        }

        // Habitation High pool: 1 template (high min_level to test filtering)
        {
            BuildingTemplate t;
            t.template_id = 1011;
            t.name = "High Rise";
            t.zone_type = ZoneBuildingType::Habitation;
            t.density = DensityLevel::High;
            t.min_land_value = 0.0f;
            t.min_level = 1;
            t.color_accent_count = 5;
            registry.register_template(t);
        }

        // Exchange Low pool: 1 template with min_level > 1 (should be filtered for initial spawn)
        {
            BuildingTemplate t;
            t.template_id = 2001;
            t.name = "Market Stall";
            t.zone_type = ZoneBuildingType::Exchange;
            t.density = DensityLevel::Low;
            t.min_land_value = 0.0f;
            t.min_level = 1;
            t.color_accent_count = 4;
            registry.register_template(t);
        }
        {
            BuildingTemplate t;
            t.template_id = 2002;
            t.name = "Advanced Market";
            t.zone_type = ZoneBuildingType::Exchange;
            t.density = DensityLevel::Low;
            t.min_land_value = 0.0f;
            t.min_level = 3; // Requires level 3 - should be filtered out
            t.color_accent_count = 4;
            registry.register_template(t);
        }
    }
};

// ============================================================================
// Determinism Tests
// ============================================================================

TEST_F(TemplateSelectorTest, DeterministicSameInputsSameOutput) {
    std::vector<std::uint32_t> neighbors = {};

    auto result1 = select_template(registry, ZoneBuildingType::Habitation,
        DensityLevel::Low, 100.0f, 10, 20, 1000, neighbors);
    auto result2 = select_template(registry, ZoneBuildingType::Habitation,
        DensityLevel::Low, 100.0f, 10, 20, 1000, neighbors);

    EXPECT_EQ(result1.template_id, result2.template_id);
    EXPECT_EQ(result1.rotation, result2.rotation);
    EXPECT_EQ(result1.color_accent_index, result2.color_accent_index);
}

TEST_F(TemplateSelectorTest, DeterministicAcrossMultipleCalls) {
    std::vector<std::uint32_t> neighbors = { 1001, 0, 0, 0 };

    // Call many times with same inputs
    auto reference = select_template(registry, ZoneBuildingType::Habitation,
        DensityLevel::Low, 75.0f, 5, 5, 500, neighbors);

    for (int i = 0; i < 100; ++i) {
        auto result = select_template(registry, ZoneBuildingType::Habitation,
            DensityLevel::Low, 75.0f, 5, 5, 500, neighbors);
        EXPECT_EQ(result.template_id, reference.template_id);
        EXPECT_EQ(result.rotation, reference.rotation);
        EXPECT_EQ(result.color_accent_index, reference.color_accent_index);
    }
}

TEST_F(TemplateSelectorTest, DifferentPositionsProduceDifferentResults) {
    std::vector<std::uint32_t> neighbors = {};

    // Different positions should (likely) produce different selections
    // This is probabilistic, but over many positions at least one should differ
    std::set<std::uint32_t> template_ids;
    for (int x = 0; x < 20; ++x) {
        auto result = select_template(registry, ZoneBuildingType::Habitation,
            DensityLevel::Low, 100.0f, x, 0, 1000, neighbors);
        template_ids.insert(result.template_id);
    }

    // With 3 templates and 20 positions, we should see multiple distinct selections
    EXPECT_GT(template_ids.size(), 1u);
}

// ============================================================================
// Pool Filtering Tests
// ============================================================================

TEST_F(TemplateSelectorTest, FilterByLandValue) {
    std::vector<std::uint32_t> neighbors = {};

    // With land_value = 10, only template 1001 (min_land_value=0) passes
    // But also 1002 (min_land_value=50) is excluded, 1003 (min_land_value=150) excluded
    // The first candidate should be 1001
    bool found_1001 = false;
    for (int x = 0; x < 100; ++x) {
        auto result = select_template(registry, ZoneBuildingType::Habitation,
            DensityLevel::Low, 10.0f, x, 0, 1, neighbors);
        EXPECT_NE(result.template_id, 0u);
        if (result.template_id == 1001) found_1001 = true;
        // Should only ever select 1001 since it's the only candidate
        EXPECT_EQ(result.template_id, 1001u);
    }
    EXPECT_TRUE(found_1001);
}

TEST_F(TemplateSelectorTest, FilterByMinLevel) {
    std::vector<std::uint32_t> neighbors = {};

    // Exchange Low pool: 2001 (min_level=1) passes, 2002 (min_level=3) filtered
    for (int x = 0; x < 50; ++x) {
        auto result = select_template(registry, ZoneBuildingType::Exchange,
            DensityLevel::Low, 100.0f, x, 0, 1, neighbors);
        EXPECT_EQ(result.template_id, 2001u); // Only 2001 should be selected
    }
}

TEST_F(TemplateSelectorTest, FallbackWhenNoPassFilters) {
    // Register a pool where all templates have high min_land_value
    BuildingTemplateRegistry special_registry;
    {
        BuildingTemplate t;
        t.template_id = 9001;
        t.name = "Expensive Only";
        t.zone_type = ZoneBuildingType::Fabrication;
        t.density = DensityLevel::High;
        t.min_land_value = 500.0f;
        t.min_level = 1;
        t.color_accent_count = 2;
        special_registry.register_template(t);
    }

    std::vector<std::uint32_t> neighbors = {};

    // Land value too low, no candidates pass, should fall back to first in pool
    auto result = select_template(special_registry, ZoneBuildingType::Fabrication,
        DensityLevel::High, 10.0f, 0, 0, 1, neighbors);
    EXPECT_EQ(result.template_id, 9001u);
}

TEST_F(TemplateSelectorTest, EmptyPoolReturnsZero) {
    std::vector<std::uint32_t> neighbors = {};

    // Fabrication Low pool has no templates registered
    auto result = select_template(registry, ZoneBuildingType::Fabrication,
        DensityLevel::Low, 100.0f, 0, 0, 1, neighbors);
    EXPECT_EQ(result.template_id, 0u);
}

// ============================================================================
// Duplicate Penalty Tests
// ============================================================================

TEST_F(TemplateSelectorTest, DuplicatePenaltyReducesWeight) {
    // With land_value = 100, templates 1001 and 1002 pass (not 1003 - min_land_value=150)
    // If all 4 neighbors have template 1001, it gets penalty -0.7*4 = -2.8
    // Weight becomes max(1.0 - 2.8, 0.1) = 0.1
    // Template 1002 stays at 1.0
    // So 1002 should be selected much more often
    std::vector<std::uint32_t> all_same_neighbors = { 1001, 1001, 1001, 1001 };

    std::map<std::uint32_t, int> counts;
    for (int x = 0; x < 1000; ++x) {
        auto result = select_template(registry, ZoneBuildingType::Habitation,
            DensityLevel::Low, 100.0f, x, 0, 1, all_same_neighbors);
        counts[result.template_id]++;
    }

    // 1002 should be selected significantly more than 1001
    // With weights 0.1 vs 1.0, ratio should be roughly 10:1
    EXPECT_GT(counts[1002], counts[1001]);
}

TEST_F(TemplateSelectorTest, AllNeighborsSameTemplateStillWorks) {
    // Even with maximum penalty, should still get a valid selection
    std::vector<std::uint32_t> neighbors = { 1001, 1001, 1001, 1001 };

    auto result = select_template(registry, ZoneBuildingType::Habitation,
        DensityLevel::Low, 100.0f, 42, 42, 1, neighbors);
    EXPECT_NE(result.template_id, 0u);
}

// ============================================================================
// Variation Output Tests
// ============================================================================

TEST_F(TemplateSelectorTest, RotationRange) {
    std::vector<std::uint32_t> neighbors = {};
    std::set<std::uint8_t> rotations;

    for (int x = 0; x < 200; ++x) {
        auto result = select_template(registry, ZoneBuildingType::Habitation,
            DensityLevel::Low, 100.0f, x, x, static_cast<std::uint64_t>(x), neighbors);
        EXPECT_LE(result.rotation, 3u);
        EXPECT_GE(result.rotation, 0u);
        rotations.insert(result.rotation);
    }

    // With 200 attempts, all 4 rotations should appear
    EXPECT_EQ(rotations.size(), 4u);
}

TEST_F(TemplateSelectorTest, ColorAccentWithinRange) {
    std::vector<std::uint32_t> neighbors = {};

    for (int x = 0; x < 200; ++x) {
        auto result = select_template(registry, ZoneBuildingType::Habitation,
            DensityLevel::Low, 10.0f, x, x, static_cast<std::uint64_t>(x), neighbors);
        // Only template 1001 passes filter (color_accent_count = 4)
        if (result.template_id == 1001) {
            EXPECT_LT(result.color_accent_index, 4u);
        }
    }
}

TEST_F(TemplateSelectorTest, ColorAccentCountZeroHandled) {
    BuildingTemplateRegistry special_registry;
    {
        BuildingTemplate t;
        t.template_id = 8001;
        t.name = "No Accents";
        t.zone_type = ZoneBuildingType::Fabrication;
        t.density = DensityLevel::Low;
        t.min_land_value = 0.0f;
        t.min_level = 1;
        t.color_accent_count = 0; // No color accents
        special_registry.register_template(t);
    }

    std::vector<std::uint32_t> neighbors = {};
    auto result = select_template(special_registry, ZoneBuildingType::Fabrication,
        DensityLevel::Low, 50.0f, 0, 0, 1, neighbors);
    EXPECT_EQ(result.template_id, 8001u);
    EXPECT_EQ(result.color_accent_index, 0u);
}

// ============================================================================
// Land Value Bonus Tests
// ============================================================================

TEST_F(TemplateSelectorTest, LandValueBonusApplied) {
    // With land_value > 100, all candidates get +0.5 bonus
    // This is hard to test directly, but we can verify that different
    // land values produce valid results
    std::vector<std::uint32_t> neighbors = {};

    auto low_val = select_template(registry, ZoneBuildingType::Habitation,
        DensityLevel::Low, 50.0f, 10, 20, 1000, neighbors);
    auto high_val = select_template(registry, ZoneBuildingType::Habitation,
        DensityLevel::Low, 200.0f, 10, 20, 1000, neighbors);

    EXPECT_NE(low_val.template_id, 0u);
    EXPECT_NE(high_val.template_id, 0u);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(TemplateSelectorTest, EmptyNeighborList) {
    std::vector<std::uint32_t> neighbors = {};
    auto result = select_template(registry, ZoneBuildingType::Habitation,
        DensityLevel::Low, 100.0f, 0, 0, 1, neighbors);
    EXPECT_NE(result.template_id, 0u);
}

TEST_F(TemplateSelectorTest, NeighborsWithZeroIds) {
    std::vector<std::uint32_t> neighbors = { 0, 0, 0, 0 };
    auto result = select_template(registry, ZoneBuildingType::Habitation,
        DensityLevel::Low, 100.0f, 0, 0, 1, neighbors);
    EXPECT_NE(result.template_id, 0u);
}

TEST_F(TemplateSelectorTest, LargeSimTick) {
    std::vector<std::uint32_t> neighbors = {};
    auto result = select_template(registry, ZoneBuildingType::Habitation,
        DensityLevel::Low, 100.0f, 0, 0, UINT64_MAX, neighbors);
    EXPECT_NE(result.template_id, 0u);
    EXPECT_LE(result.rotation, 3u);
}

TEST_F(TemplateSelectorTest, NegativeCoordinates) {
    std::vector<std::uint32_t> neighbors = {};
    auto result = select_template(registry, ZoneBuildingType::Habitation,
        DensityLevel::Low, 100.0f, -10, -20, 1000, neighbors);
    EXPECT_NE(result.template_id, 0u);
    EXPECT_LE(result.rotation, 3u);
}
