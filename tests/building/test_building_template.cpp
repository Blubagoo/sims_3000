/**
 * @file test_building_template.cpp
 * @brief Tests for BuildingTemplate and BuildingTemplateRegistry
 */

#include <sims3000/building/BuildingTemplate.h>
#include <gtest/gtest.h>
#include <stdexcept>

using namespace sims3000::building;

// ============================================================================
// BuildingTemplate Tests
// ============================================================================

TEST(BuildingTemplateTest, DefaultInitialization) {
    BuildingTemplate tmpl;

    // Verify default values
    EXPECT_EQ(tmpl.template_id, 0u);
    EXPECT_TRUE(tmpl.name.empty());
    EXPECT_EQ(tmpl.zone_type, ZoneBuildingType::Habitation);
    EXPECT_EQ(tmpl.density, DensityLevel::Low);
    EXPECT_EQ(tmpl.model_source, ModelSource::Procedural);
    EXPECT_TRUE(tmpl.model_path.empty());
    EXPECT_EQ(tmpl.footprint_w, 1u);
    EXPECT_EQ(tmpl.footprint_h, 1u);
    EXPECT_EQ(tmpl.construction_cost, 100u);
    EXPECT_EQ(tmpl.construction_ticks, 40u);
    EXPECT_FLOAT_EQ(tmpl.min_land_value, 0.0f);
    EXPECT_EQ(tmpl.min_level, 1u);
    EXPECT_EQ(tmpl.max_level, 1u);
    EXPECT_EQ(tmpl.base_capacity, 10u);
    EXPECT_EQ(tmpl.energy_required, 10u);
    EXPECT_EQ(tmpl.fluid_required, 10u);
    EXPECT_EQ(tmpl.contamination_output, 0u);
    EXPECT_EQ(tmpl.color_accent_count, 4u);
    EXPECT_FLOAT_EQ(tmpl.selection_weight, 1.0f);
}

TEST(BuildingTemplateTest, CustomInitialization) {
    BuildingTemplate tmpl;
    tmpl.template_id = 1001;
    tmpl.name = "Test Dwelling";
    tmpl.zone_type = ZoneBuildingType::Habitation;
    tmpl.density = DensityLevel::Low;
    tmpl.model_source = ModelSource::Asset;
    tmpl.model_path = "assets/models/dwelling.glb";
    tmpl.footprint_w = 1;
    tmpl.footprint_h = 1;
    tmpl.construction_cost = 500;
    tmpl.construction_ticks = 60;
    tmpl.min_land_value = 50.0f;
    tmpl.min_level = 1;
    tmpl.max_level = 3;
    tmpl.base_capacity = 8;
    tmpl.energy_required = 15;
    tmpl.fluid_required = 12;
    tmpl.contamination_output = 0;
    tmpl.color_accent_count = 3;
    tmpl.selection_weight = 1.2f;

    // Verify all fields
    EXPECT_EQ(tmpl.template_id, 1001u);
    EXPECT_EQ(tmpl.name, "Test Dwelling");
    EXPECT_EQ(tmpl.zone_type, ZoneBuildingType::Habitation);
    EXPECT_EQ(tmpl.density, DensityLevel::Low);
    EXPECT_EQ(tmpl.model_source, ModelSource::Asset);
    EXPECT_EQ(tmpl.model_path, "assets/models/dwelling.glb");
    EXPECT_EQ(tmpl.footprint_w, 1u);
    EXPECT_EQ(tmpl.footprint_h, 1u);
    EXPECT_EQ(tmpl.construction_cost, 500u);
    EXPECT_EQ(tmpl.construction_ticks, 60u);
    EXPECT_FLOAT_EQ(tmpl.min_land_value, 50.0f);
    EXPECT_EQ(tmpl.min_level, 1u);
    EXPECT_EQ(tmpl.max_level, 3u);
    EXPECT_EQ(tmpl.base_capacity, 8u);
    EXPECT_EQ(tmpl.energy_required, 15u);
    EXPECT_EQ(tmpl.fluid_required, 12u);
    EXPECT_EQ(tmpl.contamination_output, 0u);
    EXPECT_EQ(tmpl.color_accent_count, 3u);
    EXPECT_FLOAT_EQ(tmpl.selection_weight, 1.2f);
}

// ============================================================================
// TemplatePoolKey Tests
// ============================================================================

TEST(TemplatePoolKeyTest, Equality) {
    TemplatePoolKey key1{ ZoneBuildingType::Habitation, DensityLevel::Low };
    TemplatePoolKey key2{ ZoneBuildingType::Habitation, DensityLevel::Low };
    TemplatePoolKey key3{ ZoneBuildingType::Exchange, DensityLevel::Low };
    TemplatePoolKey key4{ ZoneBuildingType::Habitation, DensityLevel::High };

    // Same zone_type and density
    EXPECT_TRUE(key1 == key2);

    // Different zone_type
    EXPECT_FALSE(key1 == key3);

    // Different density
    EXPECT_FALSE(key1 == key4);
}

TEST(TemplatePoolKeyTest, Hashing) {
    TemplatePoolKey key1{ ZoneBuildingType::Habitation, DensityLevel::Low };
    TemplatePoolKey key2{ ZoneBuildingType::Habitation, DensityLevel::Low };
    TemplatePoolKey key3{ ZoneBuildingType::Exchange, DensityLevel::Low };

    std::hash<TemplatePoolKey> hasher;

    // Same keys produce same hash
    EXPECT_EQ(hasher(key1), hasher(key2));

    // Different keys likely produce different hashes (not guaranteed but probable)
    // This is more of a sanity check
    EXPECT_NE(hasher(key1), hasher(key3));
}

// ============================================================================
// BuildingTemplateRegistry Tests
// ============================================================================

TEST(BuildingTemplateRegistryTest, EmptyRegistry) {
    BuildingTemplateRegistry registry;

    EXPECT_EQ(registry.get_template_count(), 0u);
    EXPECT_FALSE(registry.has_template(1));

    // Empty pool lookup
    auto templates = registry.get_templates_for_pool(
        ZoneBuildingType::Habitation, DensityLevel::Low);
    EXPECT_TRUE(templates.empty());

    // Pool size of empty pool
    EXPECT_EQ(registry.get_pool_size(ZoneBuildingType::Habitation, DensityLevel::Low), 0u);
}

TEST(BuildingTemplateRegistryTest, RegisterSingleTemplate) {
    BuildingTemplateRegistry registry;

    BuildingTemplate tmpl;
    tmpl.template_id = 1001;
    tmpl.name = "Test Dwelling";
    tmpl.zone_type = ZoneBuildingType::Habitation;
    tmpl.density = DensityLevel::Low;

    registry.register_template(tmpl);

    EXPECT_EQ(registry.get_template_count(), 1u);
    EXPECT_TRUE(registry.has_template(1001));
    EXPECT_FALSE(registry.has_template(1002));

    // Retrieve by ID
    const auto& retrieved = registry.get_template(1001);
    EXPECT_EQ(retrieved.template_id, 1001u);
    EXPECT_EQ(retrieved.name, "Test Dwelling");
}

TEST(BuildingTemplateRegistryTest, RegisterMultipleTemplates) {
    BuildingTemplateRegistry registry;

    // Register 3 templates in same pool
    for (uint32_t i = 1001; i <= 1003; ++i) {
        BuildingTemplate tmpl;
        tmpl.template_id = i;
        tmpl.name = "Template " + std::to_string(i);
        tmpl.zone_type = ZoneBuildingType::Habitation;
        tmpl.density = DensityLevel::Low;
        registry.register_template(tmpl);
    }

    EXPECT_EQ(registry.get_template_count(), 3u);

    // All templates should be retrievable
    for (uint32_t i = 1001; i <= 1003; ++i) {
        EXPECT_TRUE(registry.has_template(i));
        const auto& tmpl = registry.get_template(i);
        EXPECT_EQ(tmpl.template_id, i);
    }
}

TEST(BuildingTemplateRegistryTest, GetTemplatesForPool) {
    BuildingTemplateRegistry registry;

    // Register templates in different pools
    {
        BuildingTemplate tmpl;
        tmpl.template_id = 1001;
        tmpl.zone_type = ZoneBuildingType::Habitation;
        tmpl.density = DensityLevel::Low;
        registry.register_template(tmpl);
    }
    {
        BuildingTemplate tmpl;
        tmpl.template_id = 1002;
        tmpl.zone_type = ZoneBuildingType::Habitation;
        tmpl.density = DensityLevel::Low;
        registry.register_template(tmpl);
    }
    {
        BuildingTemplate tmpl;
        tmpl.template_id = 1011;
        tmpl.zone_type = ZoneBuildingType::Habitation;
        tmpl.density = DensityLevel::High;
        registry.register_template(tmpl);
    }
    {
        BuildingTemplate tmpl;
        tmpl.template_id = 2001;
        tmpl.zone_type = ZoneBuildingType::Exchange;
        tmpl.density = DensityLevel::Low;
        registry.register_template(tmpl);
    }

    // Query Habitation Low pool
    auto hab_low = registry.get_templates_for_pool(
        ZoneBuildingType::Habitation, DensityLevel::Low);
    EXPECT_EQ(hab_low.size(), 2u);
    EXPECT_EQ(registry.get_pool_size(ZoneBuildingType::Habitation, DensityLevel::Low), 2u);

    // Query Habitation High pool
    auto hab_high = registry.get_templates_for_pool(
        ZoneBuildingType::Habitation, DensityLevel::High);
    EXPECT_EQ(hab_high.size(), 1u);
    EXPECT_EQ(registry.get_pool_size(ZoneBuildingType::Habitation, DensityLevel::High), 1u);

    // Query Exchange Low pool
    auto exch_low = registry.get_templates_for_pool(
        ZoneBuildingType::Exchange, DensityLevel::Low);
    EXPECT_EQ(exch_low.size(), 1u);
    EXPECT_EQ(registry.get_pool_size(ZoneBuildingType::Exchange, DensityLevel::Low), 1u);

    // Query empty pool
    auto fab_high = registry.get_templates_for_pool(
        ZoneBuildingType::Fabrication, DensityLevel::High);
    EXPECT_TRUE(fab_high.empty());
    EXPECT_EQ(registry.get_pool_size(ZoneBuildingType::Fabrication, DensityLevel::High), 0u);
}

TEST(BuildingTemplateRegistryTest, PoolLookupReturnsValidPointers) {
    BuildingTemplateRegistry registry;

    BuildingTemplate tmpl;
    tmpl.template_id = 1001;
    tmpl.name = "Test Template";
    tmpl.zone_type = ZoneBuildingType::Habitation;
    tmpl.density = DensityLevel::Low;
    registry.register_template(tmpl);

    auto templates = registry.get_templates_for_pool(
        ZoneBuildingType::Habitation, DensityLevel::Low);

    ASSERT_EQ(templates.size(), 1u);
    ASSERT_NE(templates[0], nullptr);
    EXPECT_EQ(templates[0]->template_id, 1001u);
    EXPECT_EQ(templates[0]->name, "Test Template");
}

TEST(BuildingTemplateRegistryTest, RegisterDuplicateID) {
    BuildingTemplateRegistry registry;

    BuildingTemplate tmpl1;
    tmpl1.template_id = 1001;
    tmpl1.name = "First";
    registry.register_template(tmpl1);

    // Try to register another with same ID
    BuildingTemplate tmpl2;
    tmpl2.template_id = 1001;
    tmpl2.name = "Second";

    EXPECT_THROW(registry.register_template(tmpl2), std::invalid_argument);

    // Original should still be there
    EXPECT_EQ(registry.get_template_count(), 1u);
    EXPECT_EQ(registry.get_template(1001).name, "First");
}

TEST(BuildingTemplateRegistryTest, RegisterZeroID) {
    BuildingTemplateRegistry registry;

    BuildingTemplate tmpl;
    tmpl.template_id = 0; // Invalid ID

    EXPECT_THROW(registry.register_template(tmpl), std::invalid_argument);
    EXPECT_EQ(registry.get_template_count(), 0u);
}

TEST(BuildingTemplateRegistryTest, GetNonexistentTemplate) {
    BuildingTemplateRegistry registry;

    EXPECT_THROW(registry.get_template(9999), std::out_of_range);
}

TEST(BuildingTemplateRegistryTest, ClearRegistry) {
    BuildingTemplateRegistry registry;

    // Register some templates
    for (uint32_t i = 1001; i <= 1005; ++i) {
        BuildingTemplate tmpl;
        tmpl.template_id = i;
        tmpl.zone_type = ZoneBuildingType::Habitation;
        tmpl.density = DensityLevel::Low;
        registry.register_template(tmpl);
    }

    EXPECT_EQ(registry.get_template_count(), 5u);

    // Clear
    registry.clear();

    EXPECT_EQ(registry.get_template_count(), 0u);
    EXPECT_FALSE(registry.has_template(1001));

    auto templates = registry.get_templates_for_pool(
        ZoneBuildingType::Habitation, DensityLevel::Low);
    EXPECT_TRUE(templates.empty());
}

TEST(BuildingTemplateRegistryTest, MultiplePoolsIndependent) {
    BuildingTemplateRegistry registry;

    // Register 5 templates per pool for all 6 pools
    uint32_t template_id = 1000;
    for (auto zone_type : { ZoneBuildingType::Habitation,
                            ZoneBuildingType::Exchange,
                            ZoneBuildingType::Fabrication }) {
        for (auto density : { DensityLevel::Low, DensityLevel::High }) {
            for (int i = 0; i < 5; ++i) {
                ++template_id;
                BuildingTemplate tmpl;
                tmpl.template_id = template_id;
                tmpl.zone_type = zone_type;
                tmpl.density = density;
                registry.register_template(tmpl);
            }
        }
    }

    // Verify total count
    EXPECT_EQ(registry.get_template_count(), 30u);

    // Verify each pool has 5 templates
    for (auto zone_type : { ZoneBuildingType::Habitation,
                            ZoneBuildingType::Exchange,
                            ZoneBuildingType::Fabrication }) {
        for (auto density : { DensityLevel::Low, DensityLevel::High }) {
            auto templates = registry.get_templates_for_pool(zone_type, density);
            EXPECT_EQ(templates.size(), 5u)
                << "Pool size mismatch for zone_type="
                << static_cast<int>(zone_type)
                << " density=" << static_cast<int>(density);
        }
    }
}

TEST(BuildingTemplateRegistryTest, ModelSourceEnumValues) {
    BuildingTemplate tmpl;

    tmpl.model_source = ModelSource::Procedural;
    EXPECT_EQ(static_cast<std::uint8_t>(tmpl.model_source), 0);

    tmpl.model_source = ModelSource::Asset;
    EXPECT_EQ(static_cast<std::uint8_t>(tmpl.model_source), 1);
}
