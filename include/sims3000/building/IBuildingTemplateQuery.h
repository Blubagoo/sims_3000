/**
 * @file IBuildingTemplateQuery.h
 * @brief IBuildingTemplateQuery pure abstract interface (ticket 4-037)
 *
 * Defines a read-only query interface for building template data.
 * BuildingTemplateRegistry implements this interface.
 *
 * @see BuildingTemplate.h
 * @see /docs/epics/epic-4/tickets.md (ticket 4-037)
 */

#ifndef SIMS3000_BUILDING_IBUILDINGTEMPLATEQUERY_H
#define SIMS3000_BUILDING_IBUILDINGTEMPLATEQUERY_H

#include <sims3000/building/BuildingTypes.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace building {

// Forward declaration
struct BuildingTemplate;

/**
 * @interface IBuildingTemplateQuery
 * @brief Read-only query interface for building template data.
 *
 * Provides methods to look up template properties by ID or by zone/density pool.
 * Implemented by BuildingTemplateRegistry.
 */
class IBuildingTemplateQuery {
public:
    virtual ~IBuildingTemplateQuery() = default;

    /**
     * @brief Get template by ID.
     * @param template_id The template ID to look up.
     * @return Const reference to template.
     * @throws std::out_of_range if template_id not found.
     */
    virtual const BuildingTemplate& get_template(std::uint32_t template_id) const = 0;

    /**
     * @brief Get all templates for a zone type and density combination.
     * @param type Zone building type.
     * @param density Density level.
     * @return Vector of const pointers to matching templates.
     */
    virtual std::vector<const BuildingTemplate*> get_templates_for_zone(
        ZoneBuildingType type, DensityLevel density) const = 0;

    /**
     * @brief Get energy required for a template.
     * @param template_id The template ID.
     * @return Energy required per tick.
     * @throws std::out_of_range if template_id not found.
     */
    virtual std::uint16_t get_energy_required(std::uint32_t template_id) const = 0;

    /**
     * @brief Get fluid required for a template.
     * @param template_id The template ID.
     * @return Fluid required per tick.
     * @throws std::out_of_range if template_id not found.
     */
    virtual std::uint16_t get_fluid_required(std::uint32_t template_id) const = 0;

    /**
     * @brief Get population capacity for a template.
     * @param template_id The template ID.
     * @return Base population capacity.
     * @throws std::out_of_range if template_id not found.
     */
    virtual std::uint16_t get_population_capacity(std::uint32_t template_id) const = 0;
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_IBUILDINGTEMPLATEQUERY_H
