/**
 * @file UIManager.cpp
 * @brief Implementation of UIManager core and state management.
 */

#include "sims3000/ui/UIManager.h"

#include <algorithm>

namespace sims3000 {
namespace ui {

// ============================================================================
// Construction / Destruction
// ============================================================================

UIManager::UIManager()
    : m_root(std::make_unique<Widget>())
    , m_state()
    , m_skin(UISkin::create_legacy())
{
    // Root widget covers a default 1920x1080 viewport.
    // Can be resized later when the actual window size is known.
    m_root->bounds = { 0.0f, 0.0f, 1920.0f, 1080.0f };

    m_state.current_mode = UIMode::Legacy;
}

UIManager::~UIManager() = default;

// ============================================================================
// Lifecycle
// ============================================================================

void UIManager::update(float delta_time) {
    m_root->compute_screen_bounds();
    m_root->update(delta_time);
    update_alerts(delta_time);
}

void UIManager::render(UIRenderer* renderer) {
    if (renderer) {
        m_root->render(renderer);
    }
}

// ============================================================================
// Widget tree
// ============================================================================

Widget* UIManager::get_root() const {
    return m_root.get();
}

// ============================================================================
// State access
// ============================================================================

UIState& UIManager::get_state() {
    return m_state;
}

const UIState& UIManager::get_state() const {
    return m_state;
}

// ============================================================================
// Mode switching
// ============================================================================

void UIManager::set_mode(UIMode mode) {
    m_state.current_mode = mode;

    // Apply the matching built-in skin
    if (mode == UIMode::Legacy) {
        m_skin = UISkin::create_legacy();
    } else {
        m_skin = UISkin::create_holo();
    }
}

UIMode UIManager::get_mode() const {
    return m_state.current_mode;
}

// ============================================================================
// Tool management
// ============================================================================

void UIManager::set_tool(ToolType tool) {
    m_state.current_tool = tool;
}

ToolType UIManager::get_tool() const {
    return m_state.current_tool;
}

// ============================================================================
// Overlay management
// ============================================================================

void UIManager::set_overlay(OverlayType overlay) {
    m_state.current_overlay = overlay;
}

OverlayType UIManager::get_overlay() const {
    return m_state.current_overlay;
}

void UIManager::cycle_overlay() {
    // Cycle: None -> Disorder -> Contamination -> SectorValue ->
    //        EnergyCoverage -> FluidCoverage -> ServiceCoverage -> Traffic -> None
    switch (m_state.current_overlay) {
        case OverlayType::None:
            m_state.current_overlay = OverlayType::Disorder;
            break;
        case OverlayType::Disorder:
            m_state.current_overlay = OverlayType::Contamination;
            break;
        case OverlayType::Contamination:
            m_state.current_overlay = OverlayType::SectorValue;
            break;
        case OverlayType::SectorValue:
            m_state.current_overlay = OverlayType::EnergyCoverage;
            break;
        case OverlayType::EnergyCoverage:
            m_state.current_overlay = OverlayType::FluidCoverage;
            break;
        case OverlayType::FluidCoverage:
            m_state.current_overlay = OverlayType::ServiceCoverage;
            break;
        case OverlayType::ServiceCoverage:
            m_state.current_overlay = OverlayType::Traffic;
            break;
        case OverlayType::Traffic:
            m_state.current_overlay = OverlayType::None;
            break;
        default:
            m_state.current_overlay = OverlayType::None;
            break;
    }
}

// ============================================================================
// Alerts
// ============================================================================

void UIManager::push_alert(const std::string& message, AlertPriority priority,
                            std::optional<GridPosition> location) {
    AlertPulse alert;
    alert.message = message;
    alert.priority = priority;
    alert.time_remaining = 3.0f;
    alert.location = location;

    // Add to front so newest alerts appear first
    m_state.active_alerts.push_front(std::move(alert));

    // Cap the deque at MAX_VISIBLE_ALERTS
    while (static_cast<int>(m_state.active_alerts.size()) > UIState::MAX_VISIBLE_ALERTS) {
        m_state.active_alerts.pop_back();
    }
}

void UIManager::update_alerts(float delta_time) {
    // Tick down all alert timers
    for (auto& alert : m_state.active_alerts) {
        alert.time_remaining -= delta_time;
    }

    // Remove expired alerts (time_remaining <= 0)
    m_state.active_alerts.erase(
        std::remove_if(m_state.active_alerts.begin(),
                       m_state.active_alerts.end(),
                       [](const AlertPulse& a) {
                           return a.time_remaining <= 0.0f;
                       }),
        m_state.active_alerts.end());
}

// ============================================================================
// Skin
// ============================================================================

void UIManager::set_skin(const UISkin& skin) {
    m_skin = skin;
    m_state.current_mode = skin.mode;
}

const UISkin& UIManager::get_skin() const {
    return m_skin;
}

// ============================================================================
// Panel toggle
// ============================================================================

void UIManager::toggle_budget_panel() {
    m_state.budget_panel_open = !m_state.budget_panel_open;
}

// ============================================================================
// Selection
// ============================================================================

void UIManager::select_entity(EntityID entity) {
    m_state.selected_entity = entity;
}

void UIManager::clear_selection() {
    m_state.selected_entity = std::nullopt;
}

void UIManager::set_query_position(GridPosition pos) {
    m_state.query_position = pos;
}

void UIManager::clear_query_position() {
    m_state.query_position = std::nullopt;
}

} // namespace ui
} // namespace sims3000
