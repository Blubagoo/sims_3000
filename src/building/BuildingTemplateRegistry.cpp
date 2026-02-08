/**
 * @file BuildingTemplateRegistry.cpp
 * @brief Implementation of BuildingTemplateRegistry
 */

#include <sims3000/building/BuildingTemplate.h>
#include <stdexcept>

namespace sims3000 {
namespace building {

void BuildingTemplateRegistry::register_template(const BuildingTemplate& tmpl) {
    // Validate template_id is non-zero
    if (tmpl.template_id == 0) {
        throw std::invalid_argument("BuildingTemplateRegistry::register_template: template_id cannot be 0");
    }

    // Check for duplicate template_id
    if (templates_.find(tmpl.template_id) != templates_.end()) {
        throw std::invalid_argument(
            "BuildingTemplateRegistry::register_template: template_id " +
            std::to_string(tmpl.template_id) + " already registered");
    }

    // Register template
    templates_[tmpl.template_id] = tmpl;

    // Update pool index
    TemplatePoolKey key{ tmpl.zone_type, tmpl.density };
    pool_index_[key].push_back(tmpl.template_id);
}

const BuildingTemplate& BuildingTemplateRegistry::get_template(std::uint32_t template_id) const {
    auto it = templates_.find(template_id);
    if (it == templates_.end()) {
        throw std::out_of_range(
            "BuildingTemplateRegistry::get_template: template_id " +
            std::to_string(template_id) + " not found");
    }
    return it->second;
}

std::vector<const BuildingTemplate*> BuildingTemplateRegistry::get_templates_for_pool(
    ZoneBuildingType zone_type,
    DensityLevel density) const {

    std::vector<const BuildingTemplate*> result;

    TemplatePoolKey key{ zone_type, density };
    auto it = pool_index_.find(key);

    if (it != pool_index_.end()) {
        // Reserve space for efficiency
        result.reserve(it->second.size());

        // Convert template_ids to pointers
        for (std::uint32_t template_id : it->second) {
            auto tmpl_it = templates_.find(template_id);
            if (tmpl_it != templates_.end()) {
                result.push_back(&tmpl_it->second);
            }
        }
    }

    return result;
}

std::size_t BuildingTemplateRegistry::get_pool_size(
    ZoneBuildingType zone_type,
    DensityLevel density) const {

    TemplatePoolKey key{ zone_type, density };
    auto it = pool_index_.find(key);

    if (it != pool_index_.end()) {
        return it->second.size();
    }

    return 0;
}

// ============================================================================
// IBuildingTemplateQuery Implementation
// ============================================================================

std::vector<const BuildingTemplate*> BuildingTemplateRegistry::get_templates_for_zone(
    ZoneBuildingType type,
    DensityLevel density) const {
    return get_templates_for_pool(type, density);
}

std::uint16_t BuildingTemplateRegistry::get_energy_required(std::uint32_t template_id) const {
    return get_template(template_id).energy_required;
}

std::uint16_t BuildingTemplateRegistry::get_fluid_required(std::uint32_t template_id) const {
    return get_template(template_id).fluid_required;
}

std::uint16_t BuildingTemplateRegistry::get_population_capacity(std::uint32_t template_id) const {
    return get_template(template_id).base_capacity;
}

} // namespace building
} // namespace sims3000
