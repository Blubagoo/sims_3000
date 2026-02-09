/**
 * @file UIManager.h
 * @brief Core UI system manager with state tracking and widget tree ownership.
 *
 * UIManager is the central hub for the UI system. It owns the root widget,
 * manages tool/overlay/alert state, and drives per-frame update and render.
 *
 * Key responsibilities:
 * - Owns the widget tree (root widget and all descendants)
 * - Tracks complete UI state (tool selection, overlays, alerts, panels)
 * - Provides mode switching between Legacy and Holo visual styles
 * - Manages alert notification lifecycle (push, tick-down, expire)
 *
 * Usage:
 * @code
 *   UIManager ui;
 *   ui.set_mode(UIMode::Holo);
 *   ui.set_tool(ToolType::ZoneHabitation);
 *   ui.push_alert("Energy grid overloaded!", AlertPriority::Critical);
 *
 *   // Per-frame:
 *   ui.update(delta_time);
 *   ui.render(renderer);
 * @endcode
 *
 * Thread safety: not thread-safe. Call from the main/render thread only.
 */

#ifndef SIMS3000_UI_UIMANAGER_H
#define SIMS3000_UI_UIMANAGER_H

#include "sims3000/ui/Widget.h"
#include "sims3000/ui/UISkin.h"
#include "sims3000/ui/UIRenderer.h"
#include "sims3000/core/types.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>

namespace sims3000 {
namespace ui {

// Forward declarations
class Widget;

/// Tool types available to the player (Overseer).
enum class ToolType : uint8_t {
    Select = 0,          ///< Default pointer / selection tool
    ZoneHabitation,      ///< Place habitation (residential) zones
    ZoneExchange,        ///< Place exchange (commercial) zones
    ZoneFabrication,     ///< Place fabrication (industrial) zones
    Pathway,             ///< Place pathway (road) infrastructure
    EnergyConduit,       ///< Place energy conduit infrastructure
    FluidConduit,        ///< Place fluid conduit infrastructure
    Structure,           ///< Place civic structures
    Bulldoze,            ///< Demolish structures
    Purge,               ///< Remove zones (de-zone)
    Grade,               ///< Terraform / grade terrain
    Probe                ///< Query / inspect tile info
};

/// Infrastructure placement types.
enum class InfraType : uint8_t {
    None = 0,            ///< No infrastructure selected
    Pathway,             ///< Road / pathway
    EnergyConduit,       ///< Power line
    FluidConduit         ///< Water pipe
};

/// Overlay types for scan layers.
enum class OverlayType : uint8_t {
    None = 0,            ///< No overlay active
    Disorder,            ///< Crime / disorder heat map
    Contamination,       ///< Pollution / contamination heat map
    SectorValue,         ///< Land value heat map
    EnergyCoverage,      ///< Power coverage map
    FluidCoverage,       ///< Water coverage map
    ServiceCoverage,     ///< Service radius map
    Traffic              ///< Traffic density map
};

/// Alert priority levels.
enum class AlertPriority : uint8_t {
    Info = 0,            ///< Informational notification
    Warning = 1,         ///< Warning requiring attention
    Critical = 2         ///< Critical alert requiring immediate action
};

/**
 * @struct AlertPulse
 * @brief Individual alert notification with auto-expiry.
 *
 * Alerts are displayed in a notification area and automatically expire
 * after their time_remaining reaches zero.
 */
struct AlertPulse {
    std::string message;                       ///< Alert text
    AlertPriority priority = AlertPriority::Info; ///< Severity level
    float time_remaining = 3.0f;               ///< Seconds until expiry
    std::optional<GridPosition> location;      ///< Optional map location
};

/// Sentinel value for zone_brush_type indicating no zone brush is active.
constexpr uint8_t ZONE_BRUSH_NONE = 0xFF;

/**
 * @struct UIState
 * @brief Complete snapshot of current UI state.
 *
 * All mutable UI state is collected here for easy serialization,
 * debugging, and access by the widget tree and game systems.
 */
struct UIState {
    // -- Mode ----------------------------------------------------------------
    UIMode current_mode = UIMode::Legacy;

    // -- Tools ---------------------------------------------------------------
    ToolType current_tool = ToolType::Select;

    /// Zone brush type as raw uint8_t (maps to zone::ZoneType values).
    /// Use ZONE_BRUSH_NONE (0xFF) for "no zone selected".
    uint8_t zone_brush_type = ZONE_BRUSH_NONE;

    InfraType infra_brush_type = InfraType::None;

    // -- Panels --------------------------------------------------------------
    bool budget_panel_open = false;
    std::optional<EntityID> selected_entity;
    std::optional<GridPosition> query_position;

    // -- Overlays ------------------------------------------------------------
    OverlayType current_overlay = OverlayType::None;
    float overlay_opacity = 0.7f;

    // -- Notifications -------------------------------------------------------
    std::deque<AlertPulse> active_alerts;
    static constexpr int MAX_VISIBLE_ALERTS = 4;
};

/**
 * @class UIManager
 * @brief Main UI system manager.
 *
 * Owns the widget tree root, drives update/render, and provides the
 * primary API for tool selection, overlay toggling, alert management,
 * and mode switching.
 */
class UIManager {
public:
    UIManager();
    ~UIManager();

    // -- Lifecycle -----------------------------------------------------------

    /**
     * Update all widgets and alert timers.
     * @param delta_time Seconds since last frame
     */
    void update(float delta_time);

    /**
     * Render the entire widget tree.
     * @param renderer The UI renderer to draw with
     */
    void render(UIRenderer* renderer);

    // -- Widget tree ---------------------------------------------------------

    /**
     * Get the root widget of the UI tree.
     * @return Non-owning pointer to the root widget (never null)
     */
    Widget* get_root() const;

    // -- State access --------------------------------------------------------

    /// Mutable access to the UI state.
    UIState& get_state();

    /// Read-only access to the UI state.
    const UIState& get_state() const;

    // -- Mode switching ------------------------------------------------------

    /**
     * Switch the visual mode and apply the corresponding skin.
     * @param mode The new UIMode (Legacy or Holo)
     */
    void set_mode(UIMode mode);

    /// Get the current visual mode.
    UIMode get_mode() const;

    // -- Tool management -----------------------------------------------------

    /**
     * Set the active tool.
     * @param tool The tool to activate
     */
    void set_tool(ToolType tool);

    /// Get the currently active tool.
    ToolType get_tool() const;

    // -- Overlay management --------------------------------------------------

    /**
     * Set the active overlay layer.
     * @param overlay The overlay to display (None to disable)
     */
    void set_overlay(OverlayType overlay);

    /// Get the currently active overlay.
    OverlayType get_overlay() const;

    /**
     * Cycle through overlays in order:
     * None -> Disorder -> Contamination -> SectorValue ->
     * EnergyCoverage -> FluidCoverage -> ServiceCoverage -> Traffic -> None
     */
    void cycle_overlay();

    // -- Alerts --------------------------------------------------------------

    /**
     * Push a new alert notification.
     *
     * The alert is added to the front of the deque. If the number of
     * active alerts exceeds MAX_VISIBLE_ALERTS, the oldest is removed.
     *
     * @param message   Alert text to display
     * @param priority  Severity level
     * @param location  Optional map position to jump to on click
     */
    void push_alert(const std::string& message, AlertPriority priority,
                    std::optional<GridPosition> location = std::nullopt);

    // -- Skin ----------------------------------------------------------------

    /**
     * Set a custom skin.
     * @param skin The skin to apply
     */
    void set_skin(const UISkin& skin);

    /// Get the current skin (read-only).
    const UISkin& get_skin() const;

    // -- Panel toggle --------------------------------------------------------

    /// Toggle the budget panel open/closed.
    void toggle_budget_panel();

    // -- Selection -----------------------------------------------------------

    /**
     * Select an entity for the info panel.
     * @param entity The entity ID to select
     */
    void select_entity(EntityID entity);

    /// Clear the current entity selection.
    void clear_selection();

    /**
     * Set the query position for the probe tool.
     * @param pos Grid position to query
     */
    void set_query_position(GridPosition pos);

    /// Clear the query position.
    void clear_query_position();

private:
    std::unique_ptr<Widget> m_root;
    UIState m_state;
    UISkin m_skin;

    /**
     * Tick down alert timers and remove expired alerts.
     * @param delta_time Seconds since last frame
     */
    void update_alerts(float delta_time);
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_UIMANAGER_H
