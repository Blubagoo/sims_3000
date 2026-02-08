/**
 * @file test_building_template_query.cpp
 * @brief Tests for IBuildingTemplateQuery interface (ticket 4-037)
 */

#include <sims3000/building/IBuildingTemplateQuery.h>
#include <sims3000/building/BuildingTemplate.h>
#include <sims3000/building/InitialTemplates.h>
#include <gtest/gtest.h>
#include <stdexcept>

using namespace sims3000::building;

class BuildingTemplateQueryTest : public ::testing::Test {
protected:
    void SetUp() override {
        register_initial_templates(registry);
    }

    BuildingTemplateRegistry registry;
};

// ============================================================================
// Polymorphic Usage Tests
// ============================================================================

TEST_F(BuildingTemplateQueryTest, PolymorphicGetTemplate) {
    IBuildingTemplateQuery* query = &registry;

    const BuildingTemplate& tmpl = query->get_template(1);
    EXPECT_EQ(tmpl.template_id, 1u);
    EXPECT_EQ(tmpl.name, "dwelling-pod-alpha");
}

TEST_F(BuildingTemplateQueryTest, PolymorphicGetTemplatesForZone) {
    IBuildingTemplateQuery* query = &registry;

    auto templates = query->get_templates_for_zone(
        ZoneBuildingType::Habitation, DensityLevel::Low);
    EXPECT_EQ(templates.size(), 5u);
}

// ============================================================================
// get_template Tests
// ============================================================================

TEST_F(BuildingTemplateQueryTest, GetTemplateValidID) {
    const BuildingTemplate& tmpl = registry.get_template(15);
    EXPECT_EQ(tmpl.template_id, 15u);
    EXPECT_EQ(tmpl.name, "exchange-kiosk");
}

TEST_F(BuildingTemplateQueryTest, GetTemplateInvalidID) {
    EXPECT_THROW(registry.get_template(9999), std::out_of_range);
}

// ============================================================================
// get_templates_for_zone Tests
// ============================================================================

TEST_F(BuildingTemplateQueryTest, GetTemplatesForZoneHabitationLow) {
    auto templates = registry.get_templates_for_zone(
        ZoneBuildingType::Habitation, DensityLevel::Low);
    EXPECT_EQ(templates.size(), 5u);

    // Verify all returned templates are Habitation Low
    for (const auto* t : templates) {
        EXPECT_EQ(t->zone_type, ZoneBuildingType::Habitation);
        EXPECT_EQ(t->density, DensityLevel::Low);
    }
}

TEST_F(BuildingTemplateQueryTest, GetTemplatesForZoneFabricationHigh) {
    auto templates = registry.get_templates_for_zone(
        ZoneBuildingType::Fabrication, DensityLevel::High);
    EXPECT_EQ(templates.size(), 5u);

    for (const auto* t : templates) {
        EXPECT_EQ(t->zone_type, ZoneBuildingType::Fabrication);
        EXPECT_EQ(t->density, DensityLevel::High);
    }
}

// ============================================================================
// get_energy_required Tests
// ============================================================================

TEST_F(BuildingTemplateQueryTest, GetEnergyRequiredValid) {
    std::uint16_t energy = registry.get_energy_required(1);
    EXPECT_EQ(energy, registry.get_template(1).energy_required);
    EXPECT_GT(energy, 0u);
}

TEST_F(BuildingTemplateQueryTest, GetEnergyRequiredInvalid) {
    EXPECT_THROW(registry.get_energy_required(9999), std::out_of_range);
}

// ============================================================================
// get_fluid_required Tests
// ============================================================================

TEST_F(BuildingTemplateQueryTest, GetFluidRequiredValid) {
    std::uint16_t fluid = registry.get_fluid_required(11);
    EXPECT_EQ(fluid, registry.get_template(11).fluid_required);
    EXPECT_GT(fluid, 0u);
}

TEST_F(BuildingTemplateQueryTest, GetFluidRequiredInvalid) {
    EXPECT_THROW(registry.get_fluid_required(9999), std::out_of_range);
}

// ============================================================================
// get_population_capacity Tests
// ============================================================================

TEST_F(BuildingTemplateQueryTest, GetPopulationCapacityValid) {
    std::uint16_t cap = registry.get_population_capacity(6);
    EXPECT_EQ(cap, registry.get_template(6).base_capacity);
    EXPECT_GE(cap, 40u);  // Habitation high min
}

TEST_F(BuildingTemplateQueryTest, GetPopulationCapacityInvalid) {
    EXPECT_THROW(registry.get_population_capacity(9999), std::out_of_range);
}

// ============================================================================
// Consistency Tests
// ============================================================================

TEST_F(BuildingTemplateQueryTest, QueryMethodsConsistentWithDirectAccess) {
    // For every template, verify query methods match direct access
    for (std::uint32_t id = 1; id <= 30; ++id) {
        const auto& tmpl = registry.get_template(id);

        EXPECT_EQ(registry.get_energy_required(id), tmpl.energy_required)
            << "Energy mismatch for template " << id;
        EXPECT_EQ(registry.get_fluid_required(id), tmpl.fluid_required)
            << "Fluid mismatch for template " << id;
        EXPECT_EQ(registry.get_population_capacity(id), tmpl.base_capacity)
            << "Capacity mismatch for template " << id;
    }
}

TEST_F(BuildingTemplateQueryTest, GetTemplatesForZoneMatchesPool) {
    // Verify get_templates_for_zone returns same as get_templates_for_pool
    for (auto type : { ZoneBuildingType::Habitation,
                       ZoneBuildingType::Exchange,
                       ZoneBuildingType::Fabrication }) {
        for (auto density : { DensityLevel::Low, DensityLevel::High }) {
            auto zone_result = registry.get_templates_for_zone(type, density);
            auto pool_result = registry.get_templates_for_pool(type, density);
            EXPECT_EQ(zone_result.size(), pool_result.size());
        }
    }
}
