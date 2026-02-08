/**
 * @file TemplateSelector.cpp
 * @brief Implementation of weighted random template selection algorithm
 */

#include <sims3000/building/TemplateSelector.h>
#include <random>
#include <algorithm>
#include <numeric>

namespace sims3000 {
namespace building {

TemplateSelectionResult select_template(
    const BuildingTemplateRegistry& registry,
    ZoneBuildingType zone_type,
    DensityLevel density,
    float land_value,
    std::int32_t tile_x,
    std::int32_t tile_y,
    std::uint64_t sim_tick,
    const std::vector<std::uint32_t>& neighbor_template_ids)
{
    TemplateSelectionResult result;

    // Step 1: Get candidate pool from registry
    auto pool = registry.get_templates_for_pool(zone_type, density);
    if (pool.empty()) {
        return result; // No templates available, template_id = 0
    }

    // Step 2 & 3: Filter by min_land_value and min_level
    std::vector<const BuildingTemplate*> candidates;
    candidates.reserve(pool.size());

    for (const auto* tmpl : pool) {
        if (tmpl->min_land_value <= land_value && tmpl->min_level <= 1) {
            candidates.push_back(tmpl);
        }
    }

    // Fallback: If no candidates pass filtering, use first template in pool
    if (candidates.empty()) {
        candidates.push_back(pool[0]);
    }

    // Step 4: Weight candidates
    std::vector<float> weights(candidates.size());
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        float weight = 1.0f;

        // Land value bonus: +0.5 if land_value > 100
        if (land_value > 100.0f) {
            weight += 0.5f;
        }

        // Duplicate penalty: -0.7 per neighbor match (orthogonal only)
        for (std::uint32_t neighbor_id : neighbor_template_ids) {
            if (neighbor_id != 0 && neighbor_id == candidates[i]->template_id) {
                weight -= 0.7f;
            }
        }

        // Minimum weight: 0.1
        if (weight < 0.1f) {
            weight = 0.1f;
        }

        weights[i] = weight;
    }

    // Step 5: Deterministic seeded PRNG
    // seed = tile_x * 73856093 ^ tile_y * 19349663 ^ sim_tick * 83492791
    std::uint64_t seed_val = static_cast<std::uint64_t>(static_cast<std::uint32_t>(tile_x)) * 73856093ULL;
    seed_val ^= static_cast<std::uint64_t>(static_cast<std::uint32_t>(tile_y)) * 19349663ULL;
    seed_val ^= sim_tick * 83492791ULL;

    std::minstd_rand rng(static_cast<std::uint32_t>(seed_val & 0xFFFFFFFF));

    // Weighted random selection
    float total_weight = 0.0f;
    for (float w : weights) {
        total_weight += w;
    }

    // Generate a random value in [0, total_weight)
    std::uniform_real_distribution<float> dist(0.0f, total_weight);
    float roll = dist(rng);

    std::size_t selected_index = 0;
    float cumulative = 0.0f;
    for (std::size_t i = 0; i < weights.size(); ++i) {
        cumulative += weights[i];
        if (roll < cumulative) {
            selected_index = i;
            break;
        }
        // If we reach the last element without breaking, select it
        selected_index = i;
    }

    const BuildingTemplate* selected = candidates[selected_index];
    result.template_id = selected->template_id;

    // Variation output (NO scale per CCR-010)
    result.rotation = static_cast<std::uint8_t>(rng() % 4);

    if (selected->color_accent_count > 0) {
        result.color_accent_index = static_cast<std::uint8_t>(rng() % selected->color_accent_count);
    } else {
        result.color_accent_index = 0;
    }

    return result;
}

} // namespace building
} // namespace sims3000
