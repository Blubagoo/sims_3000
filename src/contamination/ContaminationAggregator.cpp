/**
 * @file ContaminationAggregator.cpp
 * @brief Implementation of ContaminationAggregator (Ticket E10-082).
 *
 * @see IContaminationSource.h for interface documentation.
 * @see E10-082
 */

#include <sims3000/contamination/IContaminationSource.h>
#include <algorithm>

namespace sims3000 {
namespace contamination {

void ContaminationAggregator::register_source(IContaminationSource* source) {
    if (source) {
        m_sources.push_back(source);
    }
}

void ContaminationAggregator::unregister_source(IContaminationSource* source) {
    auto it = std::find(m_sources.begin(), m_sources.end(), source);
    if (it != m_sources.end()) {
        m_sources.erase(it);
    }
}

void ContaminationAggregator::apply_all_sources(ContaminationGrid& grid) {
    std::vector<ContaminationSourceEntry> entries;

    for (const IContaminationSource* src : m_sources) {
        src->get_contamination_sources(entries);
    }

    for (const ContaminationSourceEntry& entry : entries) {
        uint8_t amount = entry.output > 255
            ? static_cast<uint8_t>(255)
            : static_cast<uint8_t>(entry.output);
        grid.add_contamination(entry.x, entry.y, amount,
                               static_cast<uint8_t>(entry.type));
    }
}

size_t ContaminationAggregator::get_source_count() const {
    return m_sources.size();
}

} // namespace contamination
} // namespace sims3000
