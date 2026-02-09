/**
 * @file DisorderGeneration.cpp
 * @brief Implementation of disorder generation from buildings (Ticket E10-073).
 *
 * @see DisorderGeneration.h for interface documentation.
 * @see E10-073
 */

#include <sims3000/disorder/DisorderGeneration.h>
#include <algorithm>

namespace sims3000 {
namespace disorder {

uint8_t calculate_disorder_amount(const DisorderSource& source) {
    // Validate zone type index
    if (source.zone_type >= DISORDER_CONFIG_COUNT) {
        return 0;
    }

    const DisorderGenerationConfig& config = DISORDER_CONFIGS[source.zone_type];

    // Base generation + population-scaled component
    float generation = static_cast<float>(config.base_generation)
        + (static_cast<float>(config.base_generation) * config.population_multiplier * source.occupancy_ratio);

    // Land value modifier: low land value increases disorder
    float land_value_mod = config.land_value_modifier * (1.0f - static_cast<float>(source.land_value) / 255.0f);
    generation += generation * land_value_mod;

    // Clamp to [0, 255]
    float clamped = std::max(0.0f, std::min(generation, 255.0f));
    return static_cast<uint8_t>(clamped);
}

void apply_disorder_generation(DisorderGrid& grid, const DisorderSource& source) {
    uint8_t amount = calculate_disorder_amount(source);
    grid.add_disorder(source.x, source.y, amount);
}

} // namespace disorder
} // namespace sims3000
