/**
 * @file BuildingTemplate.h
 * @brief Building template data structure and registry (Epic 4, ticket 4-021)
 *
 * Defines BuildingTemplate struct and BuildingTemplateRegistry class.
 * Templates describe building archetypes with construction parameters, resource
 * requirements, and visual properties. Registry organizes templates by
 * TemplatePoolKey (zone_type + density) for fast lookup during building spawn.
 *
 * @see /docs/epics/epic-4/tickets.md (ticket 4-021)
 * @see /docs/building-template-briefs.yaml (template content)
 */

#ifndef SIMS3000_BUILDING_BUILDINGTEMPLATE_H
#define SIMS3000_BUILDING_BUILDINGTEMPLATE_H

#include <sims3000/building/BuildingTypes.h>
#include <sims3000/building/IBuildingTemplateQuery.h>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace sims3000 {
namespace building {

/**
 * @enum ModelSource
 * @brief Source of 3D model for rendering.
 */
enum class ModelSource : std::uint8_t {
    Procedural = 0,  ///< Runtime-generated geometry
    Asset = 1        ///< Loaded from .glb file
};

/**
 * @struct BuildingTemplate
 * @brief Complete building archetype definition.
 *
 * Describes all properties of a building type: construction parameters,
 * resource requirements, capacity, visual properties, and selection criteria.
 * Immutable after loading.
 */
struct BuildingTemplate {
    /// Unique template identifier (1-based, 0 reserved)
    std::uint32_t template_id = 0;

    /// Human-readable name (canonical alien terminology)
    std::string name;

    /// Zone type this template belongs to
    ZoneBuildingType zone_type = ZoneBuildingType::Habitation;

    /// Density level this template is for
    DensityLevel density = DensityLevel::Low;

    /// Model source (Procedural or Asset)
    ModelSource model_source = ModelSource::Procedural;

    /// Path to .glb file (empty if model_source = Procedural)
    std::string model_path;

    /// Footprint width in sectors
    std::uint8_t footprint_w = 1;

    /// Footprint height in sectors
    std::uint8_t footprint_h = 1;

    /// Construction cost in credits
    std::uint32_t construction_cost = 100;

    /// Construction duration in ticks (2-10 seconds real time)
    std::uint16_t construction_ticks = 40;

    /// Minimum land value to spawn (0-255)
    float min_land_value = 0.0f;

    /// Minimum building level required (for upgrades)
    std::uint8_t min_level = 1;

    /// Maximum building level supported (for upgrades)
    std::uint8_t max_level = 1;

    /// Base capacity (occupants or production)
    std::uint16_t base_capacity = 10;

    /// Energy required per tick
    std::uint16_t energy_required = 10;

    /// Fluid (water) required per tick
    std::uint16_t fluid_required = 10;

    /// Contamination output per tick (0 for habitation/exchange)
    std::uint16_t contamination_output = 0;

    /// Number of color accent variants
    std::uint8_t color_accent_count = 4;

    /// Selection weight for weighted random (base 1.0)
    float selection_weight = 1.0f;
};

/**
 * @struct TemplatePoolKey
 * @brief Key for template pool lookup (zone_type + density).
 *
 * Used as key in unordered_map for O(1) template pool retrieval.
 * Equality and hash operators provided.
 */
struct TemplatePoolKey {
    ZoneBuildingType zone_type;
    DensityLevel density;

    /// Equality operator for unordered_map key
    bool operator==(const TemplatePoolKey& other) const {
        return zone_type == other.zone_type && density == other.density;
    }
};

} // namespace building
} // namespace sims3000

// Hash specialization for TemplatePoolKey
namespace std {
    template<>
    struct hash<sims3000::building::TemplatePoolKey> {
        std::size_t operator()(const sims3000::building::TemplatePoolKey& key) const {
            // Combine zone_type (0-2) and density (0-1) into single hash
            // zone_type uses lower 8 bits, density uses next 8 bits
            std::size_t h = static_cast<std::size_t>(key.zone_type);
            h |= (static_cast<std::size_t>(key.density) << 8);
            return h;
        }
    };
}

namespace sims3000 {
namespace building {

/**
 * @class BuildingTemplateRegistry
 * @brief Registry of all building templates organized by pool.
 *
 * Loads and stores all BuildingTemplate instances. Provides fast lookup by:
 * - template_id: O(1) direct lookup
 * - zone_type + density pool: O(1) pool retrieval
 *
 * Registry is loaded at startup and immutable during gameplay.
 * Thread-safe for read access.
 */
class BuildingTemplateRegistry : public IBuildingTemplateQuery {
public:
    /// Constructor (empty registry)
    BuildingTemplateRegistry() = default;

    /// Virtual destructor for IBuildingTemplateQuery
    ~BuildingTemplateRegistry() override = default;

    /// Non-copyable (registry is singleton-like)
    BuildingTemplateRegistry(const BuildingTemplateRegistry&) = delete;
    BuildingTemplateRegistry& operator=(const BuildingTemplateRegistry&) = delete;

    /// Movable for initialization flexibility
    BuildingTemplateRegistry(BuildingTemplateRegistry&&) = default;
    BuildingTemplateRegistry& operator=(BuildingTemplateRegistry&&) = default;

    /**
     * @brief Register a template (for initial setup or testing).
     *
     * Adds template to registry and updates pool index.
     * Throws if template_id already exists.
     *
     * @param tmpl The template to register.
     */
    void register_template(const BuildingTemplate& tmpl);

    /**
     * @brief Get template by ID (implements IBuildingTemplateQuery).
     *
     * @param template_id The template ID to look up.
     * @return const reference to template.
     * @throws std::out_of_range if template_id not found.
     */
    const BuildingTemplate& get_template(std::uint32_t template_id) const override;

    /**
     * @brief Get all templates for a pool (zone_type + density).
     *
     * Returns pointers to all templates in the specified pool.
     * Pointers remain valid for the lifetime of the registry.
     *
     * @param zone_type Zone building type.
     * @param density Density level.
     * @return Vector of pointers to templates in pool (may be empty).
     */
    std::vector<const BuildingTemplate*> get_templates_for_pool(
        ZoneBuildingType zone_type,
        DensityLevel density) const;

    // =========================================================================
    // IBuildingTemplateQuery Implementation
    // =========================================================================

    /**
     * @brief Get templates for zone type and density (implements IBuildingTemplateQuery).
     * Delegates to get_templates_for_pool.
     */
    std::vector<const BuildingTemplate*> get_templates_for_zone(
        ZoneBuildingType type, DensityLevel density) const override;

    /**
     * @brief Get energy required for a template (implements IBuildingTemplateQuery).
     */
    std::uint16_t get_energy_required(std::uint32_t template_id) const override;

    /**
     * @brief Get fluid required for a template (implements IBuildingTemplateQuery).
     */
    std::uint16_t get_fluid_required(std::uint32_t template_id) const override;

    /**
     * @brief Get population capacity for a template (implements IBuildingTemplateQuery).
     */
    std::uint16_t get_population_capacity(std::uint32_t template_id) const override;

    /**
     * @brief Get total template count.
     * @return Number of registered templates.
     */
    std::size_t get_template_count() const {
        return templates_.size();
    }

    /**
     * @brief Check if template exists.
     * @param template_id The template ID to check.
     * @return true if template exists, false otherwise.
     */
    bool has_template(std::uint32_t template_id) const {
        return templates_.find(template_id) != templates_.end();
    }

    /**
     * @brief Get pool size (for validation).
     * @param zone_type Zone building type.
     * @param density Density level.
     * @return Number of templates in pool.
     */
    std::size_t get_pool_size(ZoneBuildingType zone_type, DensityLevel density) const;

    /**
     * @brief Clear all templates (for testing).
     */
    void clear() {
        templates_.clear();
        pool_index_.clear();
    }

private:
    /// Template storage: template_id -> BuildingTemplate
    std::unordered_map<std::uint32_t, BuildingTemplate> templates_;

    /// Pool index: (zone_type, density) -> vector of template_ids
    std::unordered_map<TemplatePoolKey, std::vector<std::uint32_t>> pool_index_;
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGTEMPLATE_H
