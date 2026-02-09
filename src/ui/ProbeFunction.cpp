/**
 * @file ProbeFunction.cpp
 * @brief Implementation of the probe query tool.
 *
 * Manages a list of IProbeQueryProvider instances and aggregates their
 * responses into a single TileQueryResult when the player probes a tile.
 */

#include "sims3000/ui/ProbeFunction.h"

#include <algorithm>

namespace sims3000 {
namespace ui {

// =========================================================================
// Construction
// =========================================================================

ProbeFunction::ProbeFunction() = default;

// =========================================================================
// Provider registration
// =========================================================================

void ProbeFunction::register_provider(IProbeQueryProvider* provider) {
    if (!provider) {
        return;
    }

    // Avoid duplicate registration
    auto it = std::find(m_providers.begin(), m_providers.end(), provider);
    if (it == m_providers.end()) {
        m_providers.push_back(provider);
    }
}

void ProbeFunction::unregister_provider(IProbeQueryProvider* provider) {
    auto it = std::find(m_providers.begin(), m_providers.end(), provider);
    if (it != m_providers.end()) {
        m_providers.erase(it);
    }
}

// =========================================================================
// Query execution
// =========================================================================

TileQueryResult ProbeFunction::query(GridPosition pos) const {
    TileQueryResult result;
    result.position = pos;

    for (const auto* provider : m_providers) {
        provider->fill_query(pos, result);
    }

    return result;
}

void ProbeFunction::query_and_display(GridPosition pos, DataReadoutPanel& panel) const {
    TileQueryResult result = query(pos);
    panel.show_query(result);
}

// =========================================================================
// Accessors
// =========================================================================

size_t ProbeFunction::provider_count() const {
    return m_providers.size();
}

} // namespace ui
} // namespace sims3000
