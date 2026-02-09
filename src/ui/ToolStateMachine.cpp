/**
 * @file ToolStateMachine.cpp
 * @brief Implementation of ToolStateMachine - tool transitions and cursor display.
 */

#include "sims3000/ui/ToolStateMachine.h"

#include <unordered_map>

namespace sims3000 {
namespace ui {

// ============================================================================
// Construction
// ============================================================================

ToolStateMachine::ToolStateMachine() {
    // Directly compute visual state for the default tool (Select).
    // Cannot use set_tool() here because the early-return guard would
    // skip update_visual_state() since m_current_tool is already Select.
    update_visual_state();
}

// ============================================================================
// Tool selection
// ============================================================================

void ToolStateMachine::set_tool(ToolType tool) {
    if (tool == m_current_tool) {
        return;
    }

    ToolType old_tool = m_current_tool;
    m_current_tool = tool;
    update_visual_state();

    if (m_on_change) {
        m_on_change(old_tool, m_current_tool);
    }
}

ToolType ToolStateMachine::get_tool() const {
    return m_current_tool;
}

// ============================================================================
// Visual state
// ============================================================================

const ToolVisualState& ToolStateMachine::get_visual_state() const {
    return m_visual_state;
}

void ToolStateMachine::set_placement_validity(PlacementValidity validity) {
    m_visual_state.placement_valid = validity;
}

// ============================================================================
// Cancel
// ============================================================================

void ToolStateMachine::cancel() {
    set_tool(ToolType::Select);
}

// ============================================================================
// Display names
// ============================================================================

const std::string& ToolStateMachine::get_tool_display_name() const {
    return display_name_for_tool(m_current_tool);
}

// ============================================================================
// Tool classification
// ============================================================================

bool ToolStateMachine::is_placement_tool() const {
    switch (m_current_tool) {
        case ToolType::ZoneHabitation:
        case ToolType::ZoneExchange:
        case ToolType::ZoneFabrication:
        case ToolType::Pathway:
        case ToolType::EnergyConduit:
        case ToolType::FluidConduit:
        case ToolType::Structure:
            return true;
        default:
            return false;
    }
}

bool ToolStateMachine::is_zone_tool() const {
    switch (m_current_tool) {
        case ToolType::ZoneHabitation:
        case ToolType::ZoneExchange:
        case ToolType::ZoneFabrication:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// Callback
// ============================================================================

void ToolStateMachine::set_on_tool_change(ToolChangeCallback callback) {
    m_on_change = std::move(callback);
}

// ============================================================================
// Internal state computation
// ============================================================================

void ToolStateMachine::update_visual_state() {
    m_visual_state.cursor_mode = cursor_mode_for_tool(m_current_tool);
    m_visual_state.cursor_color = color_for_tool(m_current_tool);
    m_visual_state.placement_valid = PlacementValidity::Unknown;
    m_visual_state.tooltip_text = display_name_for_tool(m_current_tool);
}

CursorMode ToolStateMachine::cursor_mode_for_tool(ToolType tool) const {
    switch (tool) {
        case ToolType::Select:
            return CursorMode::Arrow;
        case ToolType::ZoneHabitation:
        case ToolType::ZoneExchange:
        case ToolType::ZoneFabrication:
            return CursorMode::ZoneBrush;
        case ToolType::Pathway:
        case ToolType::EnergyConduit:
        case ToolType::FluidConduit:
            return CursorMode::LinePlacement;
        case ToolType::Structure:
            return CursorMode::Arrow;  // Will become ZoneBrush later
        case ToolType::Bulldoze:
            return CursorMode::Bulldoze;
        case ToolType::Probe:
            return CursorMode::Probe;
        case ToolType::Grade:
            return CursorMode::Grade;
        case ToolType::Purge:
            return CursorMode::Purge;
        default:
            return CursorMode::Arrow;
    }
}

Color ToolStateMachine::color_for_tool(ToolType tool) const {
    switch (tool) {
        case ToolType::ZoneHabitation:
            return {0.0f, 0.8f, 0.0f, 1.0f};      // Green
        case ToolType::ZoneExchange:
            return {0.0f, 0.4f, 0.8f, 1.0f};       // Blue
        case ToolType::ZoneFabrication:
            return {0.8f, 0.8f, 0.0f, 1.0f};       // Yellow
        case ToolType::Pathway:
            return {0.8f, 0.8f, 0.8f, 1.0f};       // White/grey
        case ToolType::EnergyConduit:
            return {0.0f, 0.9f, 0.9f, 1.0f};       // Cyan
        case ToolType::FluidConduit:
            return {0.2f, 0.4f, 0.9f, 1.0f};       // Blue
        case ToolType::Bulldoze:
            return {0.9f, 0.2f, 0.2f, 1.0f};       // Red
        case ToolType::Probe:
            return {1.0f, 1.0f, 1.0f, 1.0f};       // White
        default:
            return {1.0f, 1.0f, 1.0f, 1.0f};       // White (default)
    }
}

const std::string& ToolStateMachine::display_name_for_tool(ToolType tool) {
    // Static map ensures stable references - no dangling pointers
    static const std::unordered_map<uint8_t, std::string> names = {
        { static_cast<uint8_t>(ToolType::Select),           "Select" },
        { static_cast<uint8_t>(ToolType::ZoneHabitation),   "Habitation Zone" },
        { static_cast<uint8_t>(ToolType::ZoneExchange),     "Exchange Zone" },
        { static_cast<uint8_t>(ToolType::ZoneFabrication),  "Fabrication Zone" },
        { static_cast<uint8_t>(ToolType::Pathway),          "Pathway" },
        { static_cast<uint8_t>(ToolType::EnergyConduit),    "Energy Conduit" },
        { static_cast<uint8_t>(ToolType::FluidConduit),     "Fluid Conduit" },
        { static_cast<uint8_t>(ToolType::Structure),        "Structure" },
        { static_cast<uint8_t>(ToolType::Bulldoze),         "Bulldoze" },
        { static_cast<uint8_t>(ToolType::Purge),            "Purge Terrain" },
        { static_cast<uint8_t>(ToolType::Grade),            "Grade Terrain" },
        { static_cast<uint8_t>(ToolType::Probe),            "Probe Function" }
    };

    static const std::string unknown = "Unknown";

    auto it = names.find(static_cast<uint8_t>(tool));
    if (it != names.end()) {
        return it->second;
    }
    return unknown;
}

} // namespace ui
} // namespace sims3000
