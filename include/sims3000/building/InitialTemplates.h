/**
 * @file InitialTemplates.h
 * @brief Registration of 30 initial building templates (ticket 4-023)
 *
 * Provides register_initial_templates() which registers 30 building templates
 * organized into 6 pools (3 zone types x 2 density levels), 5 per pool.
 *
 * Template naming uses canonical alien terminology per /docs/canon/terminology.yaml
 *
 * @see BuildingTemplate.h
 * @see /docs/epics/epic-4/tickets.md (ticket 4-023)
 */

#ifndef SIMS3000_BUILDING_INITIALTEMPLATES_H
#define SIMS3000_BUILDING_INITIALTEMPLATES_H

namespace sims3000 {
namespace building {

// Forward declaration
class BuildingTemplateRegistry;

/**
 * @brief Register all 30 initial building templates.
 *
 * Registers 5 templates per pool across all 6 pools:
 * - Habitation Low (IDs 1-5)
 * - Habitation High (IDs 6-10)
 * - Exchange Low (IDs 11-15)
 * - Exchange High (IDs 16-20)
 * - Fabrication Low (IDs 21-25)
 * - Fabrication High (IDs 26-30)
 *
 * @param registry The registry to register templates in.
 */
void register_initial_templates(BuildingTemplateRegistry& registry);

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_INITIALTEMPLATES_H
