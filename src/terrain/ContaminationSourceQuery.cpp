/**
 * @file ContaminationSourceQuery.cpp
 * @brief Implementation of terrain contamination source queries
 */

#include <sims3000/terrain/ContaminationSourceQuery.h>
#include <sims3000/terrain/TerrainTypeInfo.h>

namespace sims3000 {
namespace terrain {

ContaminationSourceQuery::ContaminationSourceQuery(const TerrainGrid& grid)
    : m_grid(grid)
    , m_sources()
    , m_cache_valid(false)
{
}

const std::vector<ContaminationSource>& ContaminationSourceQuery::get_terrain_contamination_sources() {
    if (!m_cache_valid) {
        rebuild_cache();
    }
    return m_sources;
}

void ContaminationSourceQuery::on_terrain_modified(const TerrainModifiedEvent& event) {
    // Terraformed or Generated events always invalidate the cache
    // because terrain types may have changed
    if (event.modification_type == ModificationType::Terraformed ||
        event.modification_type == ModificationType::Generated) {
        invalidate_cache();
        return;
    }

    // For other modification types (Cleared, Leveled, SeaLevelChanged),
    // we need to check if the affected area contains any BlightMires tiles.
    // However, since clearing/leveling doesn't change terrain type and
    // sea level changes don't affect BlightMires generation, we can skip
    // invalidation for these events.
    //
    // Note: If future modifications can affect contamination generation
    // without changing terrain type, this logic should be updated.
}

void ContaminationSourceQuery::invalidate_cache() {
    m_cache_valid = false;
    // Don't clear sources vector - it will be repopulated on rebuild
}

void ContaminationSourceQuery::rebuild_cache() {
    m_sources.clear();

    // Early exit if grid is empty
    if (m_grid.empty()) {
        m_cache_valid = true;
        return;
    }

    const std::uint16_t width = m_grid.width;
    const std::uint16_t height = m_grid.height;

    // Reserve space for potential sources (estimate based on typical map density)
    // BlightMires is relatively rare, so a small initial reserve is appropriate
    m_sources.reserve(64);

    // Scan entire grid for contamination-producing tiles
    for (std::uint16_t y = 0; y < height; ++y) {
        for (std::uint16_t x = 0; x < width; ++x) {
            const TerrainComponent& tile = m_grid.at(x, y);
            const TerrainType type = tile.getTerrainType();

            // Check if this terrain type generates contamination
            if (generatesContamination(type)) {
                ContaminationSource source;
                source.position.x = static_cast<std::int16_t>(x);
                source.position.y = static_cast<std::int16_t>(y);
                source.contamination_per_tick = getTerrainInfo(type).contamination_per_tick;
                source.source_type = type;

                m_sources.push_back(source);
            }
        }
    }

    // Shrink to fit to avoid excess memory usage
    m_sources.shrink_to_fit();

    m_cache_valid = true;
}

} // namespace terrain
} // namespace sims3000
