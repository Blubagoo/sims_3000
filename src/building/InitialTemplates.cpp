/**
 * @file InitialTemplates.cpp
 * @brief Implementation of register_initial_templates (ticket 4-023)
 *
 * Registers 30 building templates: 5 per bucket across 6 buckets.
 * Uses canonical alien terminology for template names.
 */

#include <sims3000/building/InitialTemplates.h>
#include <sims3000/building/BuildingTemplate.h>

namespace sims3000 {
namespace building {

void register_initial_templates(BuildingTemplateRegistry& registry) {

    // ========================================================================
    // Habitation Low Density (IDs 1-5)
    // 1x1 footprint, construction_ticks 40-80, capacity 4-12
    // ========================================================================

    {
        BuildingTemplate t;
        t.template_id = 1;
        t.name = "dwelling-pod-alpha";
        t.zone_type = ZoneBuildingType::Habitation;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 40;
        t.construction_cost = 100;
        t.base_capacity = 4;
        t.energy_required = 5;
        t.fluid_required = 3;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 2;
        t.name = "dwelling-pod-beta";
        t.zone_type = ZoneBuildingType::Habitation;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 50;
        t.construction_cost = 120;
        t.base_capacity = 6;
        t.energy_required = 6;
        t.fluid_required = 4;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 3;
        t.name = "hab-cell-standard";
        t.zone_type = ZoneBuildingType::Habitation;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 60;
        t.construction_cost = 150;
        t.base_capacity = 8;
        t.energy_required = 8;
        t.fluid_required = 5;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 4;
        t.name = "hab-cell-compact";
        t.zone_type = ZoneBuildingType::Habitation;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 70;
        t.construction_cost = 180;
        t.base_capacity = 10;
        t.energy_required = 9;
        t.fluid_required = 6;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 5;
        t.name = "micro-dwelling";
        t.zone_type = ZoneBuildingType::Habitation;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 80;
        t.construction_cost = 200;
        t.base_capacity = 12;
        t.energy_required = 10;
        t.fluid_required = 7;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }

    // ========================================================================
    // Habitation High Density (IDs 6-10)
    // Mostly 1x1, at least one 2x2, construction_ticks 100-200, capacity 40-200
    // ========================================================================

    {
        BuildingTemplate t;
        t.template_id = 6;
        t.name = "hab-spire-minor";
        t.zone_type = ZoneBuildingType::Habitation;
        t.density = DensityLevel::High;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 100;
        t.construction_cost = 500;
        t.base_capacity = 40;
        t.energy_required = 25;
        t.fluid_required = 20;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 7;
        t.name = "hab-spire-major";
        t.zone_type = ZoneBuildingType::Habitation;
        t.density = DensityLevel::High;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 130;
        t.construction_cost = 700;
        t.base_capacity = 80;
        t.energy_required = 40;
        t.fluid_required = 30;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 8;
        t.name = "hab-tower-standard";
        t.zone_type = ZoneBuildingType::Habitation;
        t.density = DensityLevel::High;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 150;
        t.construction_cost = 900;
        t.base_capacity = 120;
        t.energy_required = 50;
        t.fluid_required = 40;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 9;
        t.name = "communal-nexus";
        t.zone_type = ZoneBuildingType::Habitation;
        t.density = DensityLevel::High;
        t.footprint_w = 2; t.footprint_h = 2;  // 2x2 footprint
        t.construction_ticks = 180;
        t.construction_cost = 1200;
        t.base_capacity = 160;
        t.energy_required = 70;
        t.fluid_required = 55;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 0.8f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 10;
        t.name = "hab-complex-alpha";
        t.zone_type = ZoneBuildingType::Habitation;
        t.density = DensityLevel::High;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 200;
        t.construction_cost = 1500;
        t.base_capacity = 200;
        t.energy_required = 80;
        t.fluid_required = 60;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 0.6f;
        registry.register_template(t);
    }

    // ========================================================================
    // Exchange Low Density (IDs 11-15)
    // 1x1 footprint, construction_ticks 40-80, capacity 2-6
    // ========================================================================

    {
        BuildingTemplate t;
        t.template_id = 11;
        t.name = "market-pod-alpha";
        t.zone_type = ZoneBuildingType::Exchange;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 40;
        t.construction_cost = 120;
        t.base_capacity = 2;
        t.energy_required = 4;
        t.fluid_required = 2;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 12;
        t.name = "market-pod-beta";
        t.zone_type = ZoneBuildingType::Exchange;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 50;
        t.construction_cost = 140;
        t.base_capacity = 3;
        t.energy_required = 5;
        t.fluid_required = 3;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 13;
        t.name = "trade-cell-standard";
        t.zone_type = ZoneBuildingType::Exchange;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 60;
        t.construction_cost = 160;
        t.base_capacity = 4;
        t.energy_required = 6;
        t.fluid_required = 4;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 14;
        t.name = "barter-node";
        t.zone_type = ZoneBuildingType::Exchange;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 70;
        t.construction_cost = 180;
        t.base_capacity = 5;
        t.energy_required = 7;
        t.fluid_required = 4;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 15;
        t.name = "exchange-kiosk";
        t.zone_type = ZoneBuildingType::Exchange;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 80;
        t.construction_cost = 200;
        t.base_capacity = 6;
        t.energy_required = 8;
        t.fluid_required = 5;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }

    // ========================================================================
    // Exchange High Density (IDs 16-20)
    // Mostly 1x1, at least one 2x2, construction_ticks 100-200, capacity 20-80
    // ========================================================================

    {
        BuildingTemplate t;
        t.template_id = 16;
        t.name = "exchange-tower-alpha";
        t.zone_type = ZoneBuildingType::Exchange;
        t.density = DensityLevel::High;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 100;
        t.construction_cost = 600;
        t.base_capacity = 20;
        t.energy_required = 20;
        t.fluid_required = 15;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 17;
        t.name = "exchange-tower-beta";
        t.zone_type = ZoneBuildingType::Exchange;
        t.density = DensityLevel::High;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 130;
        t.construction_cost = 800;
        t.base_capacity = 35;
        t.energy_required = 30;
        t.fluid_required = 22;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 18;
        t.name = "trade-nexus";
        t.zone_type = ZoneBuildingType::Exchange;
        t.density = DensityLevel::High;
        t.footprint_w = 2; t.footprint_h = 2;  // 2x2 footprint
        t.construction_ticks = 160;
        t.construction_cost = 1100;
        t.base_capacity = 50;
        t.energy_required = 45;
        t.fluid_required = 35;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 0.8f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 19;
        t.name = "commerce-spire";
        t.zone_type = ZoneBuildingType::Exchange;
        t.density = DensityLevel::High;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 180;
        t.construction_cost = 1300;
        t.base_capacity = 65;
        t.energy_required = 55;
        t.fluid_required = 42;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 0.7f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 20;
        t.name = "exchange-complex";
        t.zone_type = ZoneBuildingType::Exchange;
        t.density = DensityLevel::High;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 200;
        t.construction_cost = 1600;
        t.base_capacity = 80;
        t.energy_required = 65;
        t.fluid_required = 50;
        t.contamination_output = 0;
        t.color_accent_count = 4;
        t.selection_weight = 0.6f;
        registry.register_template(t);
    }

    // ========================================================================
    // Fabrication Low Density (IDs 21-25)
    // 1x1 footprint, construction_ticks 40-80, capacity 4-10
    // All have contamination_output > 0
    // ========================================================================

    {
        BuildingTemplate t;
        t.template_id = 21;
        t.name = "fabricator-pod-alpha";
        t.zone_type = ZoneBuildingType::Fabrication;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 40;
        t.construction_cost = 130;
        t.base_capacity = 4;
        t.energy_required = 8;
        t.fluid_required = 3;
        t.contamination_output = 5;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 22;
        t.name = "fabricator-pod-beta";
        t.zone_type = ZoneBuildingType::Fabrication;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 50;
        t.construction_cost = 160;
        t.base_capacity = 5;
        t.energy_required = 10;
        t.fluid_required = 4;
        t.contamination_output = 7;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 23;
        t.name = "assembly-cell";
        t.zone_type = ZoneBuildingType::Fabrication;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 60;
        t.construction_cost = 190;
        t.base_capacity = 7;
        t.energy_required = 12;
        t.fluid_required = 5;
        t.contamination_output = 8;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 24;
        t.name = "forge-pod";
        t.zone_type = ZoneBuildingType::Fabrication;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 70;
        t.construction_cost = 220;
        t.base_capacity = 8;
        t.energy_required = 14;
        t.fluid_required = 6;
        t.contamination_output = 10;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 25;
        t.name = "workshop-node";
        t.zone_type = ZoneBuildingType::Fabrication;
        t.density = DensityLevel::Low;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 80;
        t.construction_cost = 250;
        t.base_capacity = 10;
        t.energy_required = 16;
        t.fluid_required = 7;
        t.contamination_output = 12;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }

    // ========================================================================
    // Fabrication High Density (IDs 26-30)
    // Mostly 1x1, at least one 2x2, construction_ticks 100-200, capacity 30-120
    // All have contamination_output > 0
    // ========================================================================

    {
        BuildingTemplate t;
        t.template_id = 26;
        t.name = "fabrication-tower-alpha";
        t.zone_type = ZoneBuildingType::Fabrication;
        t.density = DensityLevel::High;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 100;
        t.construction_cost = 650;
        t.base_capacity = 30;
        t.energy_required = 35;
        t.fluid_required = 20;
        t.contamination_output = 15;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 27;
        t.name = "fabrication-tower-beta";
        t.zone_type = ZoneBuildingType::Fabrication;
        t.density = DensityLevel::High;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 130;
        t.construction_cost = 850;
        t.base_capacity = 50;
        t.energy_required = 50;
        t.fluid_required = 30;
        t.contamination_output = 20;
        t.color_accent_count = 4;
        t.selection_weight = 1.0f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 28;
        t.name = "forge-spire";
        t.zone_type = ZoneBuildingType::Fabrication;
        t.density = DensityLevel::High;
        t.footprint_w = 2; t.footprint_h = 2;  // 2x2 footprint
        t.construction_ticks = 160;
        t.construction_cost = 1150;
        t.base_capacity = 80;
        t.energy_required = 70;
        t.fluid_required = 45;
        t.contamination_output = 30;
        t.color_accent_count = 4;
        t.selection_weight = 0.8f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 29;
        t.name = "assembly-complex";
        t.zone_type = ZoneBuildingType::Fabrication;
        t.density = DensityLevel::High;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 180;
        t.construction_cost = 1400;
        t.base_capacity = 100;
        t.energy_required = 80;
        t.fluid_required = 55;
        t.contamination_output = 35;
        t.color_accent_count = 4;
        t.selection_weight = 0.7f;
        registry.register_template(t);
    }
    {
        BuildingTemplate t;
        t.template_id = 30;
        t.name = "factory-nexus";
        t.zone_type = ZoneBuildingType::Fabrication;
        t.density = DensityLevel::High;
        t.footprint_w = 1; t.footprint_h = 1;
        t.construction_ticks = 200;
        t.construction_cost = 1700;
        t.base_capacity = 120;
        t.energy_required = 90;
        t.fluid_required = 65;
        t.contamination_output = 40;
        t.color_accent_count = 4;
        t.selection_weight = 0.6f;
        registry.register_template(t);
    }
}

} // namespace building
} // namespace sims3000
