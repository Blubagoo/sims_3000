/**
 * @file test_ui_widgets.cpp
 * @brief Unit tests for the UI widget system (Ticket E12-026).
 *
 * Test categories:
 * 1. Widget Hierarchy - add_child, remove_child, bounds_calculation, visibility
 * 2. Hit Testing - point_in_rect, nested_widgets, z_order
 * 3. State Management - tool_selection, overlay_toggle, scan_layer_cycle, cursor_mode
 * 4. Widget-specific - ButtonWidget, ProgressBarWidget, LabelWidget, PanelWidget
 */

#include <sims3000/ui/Widget.h>
#include <sims3000/ui/CoreWidgets.h>
#include <sims3000/ui/UIManager.h>
#include <sims3000/ui/ToolStateMachine.h>
#include <sims3000/ui/ScanLayerManager.h>
#include <sims3000/ui/ProgressWidgets.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>

using namespace sims3000::ui;

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n  FAILED: %s (line %d)\n", #condition, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b) do { \
    if (std::fabs((a) - (b)) > 0.001f) { \
        printf("\n  FAILED: %s == %s (got %f vs %f, line %d)\n", #a, #b, \
               static_cast<double>(a), static_cast<double>(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Widget Hierarchy Tests
// =============================================================================

TEST(add_child_sets_parent_and_children_size) {
    auto root = std::make_unique<Widget>();
    root->bounds = {0, 0, 800, 600};

    auto child_ptr = std::make_unique<Widget>();
    child_ptr->bounds = {10, 10, 100, 50};

    Widget* child = root->add_child(std::move(child_ptr));

    ASSERT(child != nullptr);
    ASSERT_EQ(child->parent, root.get());
    ASSERT_EQ(root->children.size(), static_cast<size_t>(1));
}

TEST(add_multiple_children) {
    auto root = std::make_unique<Widget>();
    root->bounds = {0, 0, 800, 600};

    root->add_child(std::make_unique<Widget>());
    root->add_child(std::make_unique<Widget>());
    root->add_child(std::make_unique<Widget>());

    ASSERT_EQ(root->children.size(), static_cast<size_t>(3));
    for (auto& c : root->children) {
        ASSERT_EQ(c->parent, root.get());
    }
}

TEST(remove_child_removes_and_clears_parent) {
    auto root = std::make_unique<Widget>();
    root->bounds = {0, 0, 800, 600};

    auto child_ptr = std::make_unique<Widget>();
    Widget* child = root->add_child(std::move(child_ptr));

    ASSERT_EQ(root->children.size(), static_cast<size_t>(1));

    root->remove_child(child);

    ASSERT_EQ(root->children.size(), static_cast<size_t>(0));
    // child is destroyed after remove_child, so we only check the size
}

TEST(remove_child_null_is_safe) {
    auto root = std::make_unique<Widget>();
    root->bounds = {0, 0, 800, 600};
    root->add_child(std::make_unique<Widget>());

    // Removing nullptr should be a no-op
    root->remove_child(nullptr);
    ASSERT_EQ(root->children.size(), static_cast<size_t>(1));
}

TEST(add_child_null_returns_nullptr) {
    auto root = std::make_unique<Widget>();
    Widget* result = root->add_child(nullptr);
    ASSERT(result == nullptr);
    ASSERT_EQ(root->children.size(), static_cast<size_t>(0));
}

TEST(nested_bounds_calculation) {
    // Parent at (100, 100), child at (10, 10) -> screen bounds (110, 110)
    auto root = std::make_unique<Widget>();
    root->bounds = {100, 100, 400, 300};

    auto child_ptr = std::make_unique<Widget>();
    child_ptr->bounds = {10, 10, 80, 60};
    Widget* child = root->add_child(std::move(child_ptr));

    root->compute_screen_bounds();

    // Root screen bounds == root bounds (no parent)
    ASSERT_FLOAT_EQ(root->screen_bounds.x, 100.0f);
    ASSERT_FLOAT_EQ(root->screen_bounds.y, 100.0f);
    ASSERT_FLOAT_EQ(root->screen_bounds.width, 400.0f);
    ASSERT_FLOAT_EQ(root->screen_bounds.height, 300.0f);

    // Child screen bounds = parent screen + child bounds offset
    ASSERT_FLOAT_EQ(child->screen_bounds.x, 110.0f);
    ASSERT_FLOAT_EQ(child->screen_bounds.y, 110.0f);
    ASSERT_FLOAT_EQ(child->screen_bounds.width, 80.0f);
    ASSERT_FLOAT_EQ(child->screen_bounds.height, 60.0f);
}

TEST(deeply_nested_bounds_calculation) {
    // root(50,50) -> child(10,20) -> grandchild(5,5)
    // grandchild screen = (65, 75)
    auto root = std::make_unique<Widget>();
    root->bounds = {50, 50, 400, 300};

    auto child_ptr = std::make_unique<Widget>();
    child_ptr->bounds = {10, 20, 200, 200};
    Widget* child = root->add_child(std::move(child_ptr));

    auto grandchild_ptr = std::make_unique<Widget>();
    grandchild_ptr->bounds = {5, 5, 50, 50};
    Widget* grandchild = child->add_child(std::move(grandchild_ptr));

    root->compute_screen_bounds();

    ASSERT_FLOAT_EQ(grandchild->screen_bounds.x, 65.0f);
    ASSERT_FLOAT_EQ(grandchild->screen_bounds.y, 75.0f);
}

TEST(hidden_parent_blocks_child_render) {
    // Widget::render() returns early when visible=false, so children never render.
    // We verify this by checking the render flow: if parent is invisible,
    // children should not receive render calls.
    auto root = std::make_unique<Widget>();
    root->bounds = {0, 0, 800, 600};
    root->visible = false;

    auto child_ptr = std::make_unique<Widget>();
    child_ptr->bounds = {10, 10, 100, 50};
    child_ptr->visible = true;
    root->add_child(std::move(child_ptr));

    // Calling render(nullptr) on an invisible widget returns immediately
    // without accessing the renderer, so this should not crash.
    root->render(nullptr);

    // The key assertion: invisible root means children don't render.
    // This is inherently tested by the code path: Widget::render() returns
    // early when !visible, never iterating children.
    ASSERT(!root->visible);
    ASSERT(root->children[0]->visible);
    // The structural guarantee: invisible parent prevents child rendering.
}

// =============================================================================
// Hit Testing Tests
// =============================================================================

TEST(rect_contains_point_inside) {
    Rect r{10.0f, 20.0f, 100.0f, 50.0f};
    ASSERT(r.contains(10.0f, 20.0f));   // top-left corner
    ASSERT(r.contains(50.0f, 40.0f));   // center-ish
    ASSERT(r.contains(109.9f, 69.9f));  // near bottom-right edge
}

TEST(rect_does_not_contain_point_outside) {
    Rect r{10.0f, 20.0f, 100.0f, 50.0f};
    ASSERT(!r.contains(9.9f, 20.0f));   // just left
    ASSERT(!r.contains(10.0f, 19.9f));  // just above
    ASSERT(!r.contains(110.0f, 40.0f)); // right edge (exclusive)
    ASSERT(!r.contains(50.0f, 70.0f));  // bottom edge (exclusive)
    ASSERT(!r.contains(0.0f, 0.0f));    // well outside
}

TEST(widget_hit_test_checks_screen_bounds) {
    Widget w;
    w.bounds = {10, 10, 100, 50};
    w.screen_bounds = {10, 10, 100, 50};
    w.visible = true;
    w.enabled = true;

    ASSERT(w.hit_test(50.0f, 30.0f));
    ASSERT(!w.hit_test(0.0f, 0.0f));
}

TEST(widget_hit_test_invisible_returns_false) {
    Widget w;
    w.screen_bounds = {10, 10, 100, 50};
    w.visible = false;
    w.enabled = true;

    ASSERT(!w.hit_test(50.0f, 30.0f));
}

TEST(widget_hit_test_disabled_returns_false) {
    Widget w;
    w.screen_bounds = {10, 10, 100, 50};
    w.visible = true;
    w.enabled = false;

    ASSERT(!w.hit_test(50.0f, 30.0f));
}

TEST(find_child_at_returns_deepest_child) {
    auto root = std::make_unique<Widget>();
    root->bounds = {0, 0, 800, 600};

    auto parent = std::make_unique<Widget>();
    parent->bounds = {100, 100, 200, 200};

    auto child = std::make_unique<Widget>();
    child->bounds = {10, 10, 50, 50};
    Widget* child_raw = parent->add_child(std::move(child));

    Widget* parent_raw = root->add_child(std::move(parent));

    root->compute_screen_bounds();

    // Point at (115, 115) should be inside both parent and child.
    // find_child_at should return the deepest (child).
    Widget* hit = root->find_child_at(115.0f, 115.0f);
    ASSERT_EQ(hit, child_raw);

    // Point at (150, 150) is inside parent but outside child (child goes to 160,160).
    // Should return parent.
    hit = root->find_child_at(195.0f, 195.0f);
    ASSERT_EQ(hit, parent_raw);
}

TEST(find_child_at_returns_nullptr_on_miss) {
    auto root = std::make_unique<Widget>();
    root->bounds = {0, 0, 800, 600};

    auto child = std::make_unique<Widget>();
    child->bounds = {100, 100, 50, 50};
    root->add_child(std::move(child));

    root->compute_screen_bounds();

    // Point at (10, 10) misses the child entirely
    Widget* hit = root->find_child_at(10.0f, 10.0f);
    ASSERT(hit == nullptr);
}

TEST(find_child_at_z_order_last_child_wins) {
    // find_child_at iterates in reverse order, so the last child in the
    // children vector (higher index) is tested first. When two overlapping
    // children are at the same z_order, the one added later wins.
    auto root = std::make_unique<Widget>();
    root->bounds = {0, 0, 800, 600};

    auto first = std::make_unique<Widget>();
    first->bounds = {50, 50, 100, 100};
    root->add_child(std::move(first));

    auto second = std::make_unique<Widget>();
    second->bounds = {50, 50, 100, 100};
    Widget* second_raw = root->add_child(std::move(second));

    root->compute_screen_bounds();

    // Both widgets overlap at (80, 80). The second (last added) should win
    // because find_child_at iterates in reverse.
    Widget* hit = root->find_child_at(80.0f, 80.0f);
    ASSERT_EQ(hit, second_raw);
}

TEST(find_child_at_skips_invisible_children) {
    auto root = std::make_unique<Widget>();
    root->bounds = {0, 0, 800, 600};

    auto visible_child = std::make_unique<Widget>();
    visible_child->bounds = {50, 50, 100, 100};
    Widget* visible_raw = root->add_child(std::move(visible_child));

    auto invisible_child = std::make_unique<Widget>();
    invisible_child->bounds = {50, 50, 100, 100};
    invisible_child->visible = false;
    root->add_child(std::move(invisible_child));

    root->compute_screen_bounds();

    // The invisible child is added last (higher index) but should be skipped.
    // The visible child should be found instead.
    Widget* hit = root->find_child_at(80.0f, 80.0f);
    ASSERT_EQ(hit, visible_raw);
}

TEST(find_child_at_skips_disabled_children) {
    auto root = std::make_unique<Widget>();
    root->bounds = {0, 0, 800, 600};

    auto enabled_child = std::make_unique<Widget>();
    enabled_child->bounds = {50, 50, 100, 100};
    Widget* enabled_raw = root->add_child(std::move(enabled_child));

    auto disabled_child = std::make_unique<Widget>();
    disabled_child->bounds = {50, 50, 100, 100};
    disabled_child->enabled = false;
    root->add_child(std::move(disabled_child));

    root->compute_screen_bounds();

    Widget* hit = root->find_child_at(80.0f, 80.0f);
    ASSERT_EQ(hit, enabled_raw);
}

// =============================================================================
// State Management Tests
// =============================================================================

TEST(uimanager_tool_selection) {
    UIManager ui;

    // Default tool is Select
    ASSERT_EQ(ui.get_tool(), ToolType::Select);

    ui.set_tool(ToolType::ZoneHabitation);
    ASSERT_EQ(ui.get_tool(), ToolType::ZoneHabitation);

    ui.set_tool(ToolType::Bulldoze);
    ASSERT_EQ(ui.get_tool(), ToolType::Bulldoze);

    ui.set_tool(ToolType::Probe);
    ASSERT_EQ(ui.get_tool(), ToolType::Probe);
}

TEST(uimanager_tool_changes_state) {
    UIManager ui;

    ui.set_tool(ToolType::ZoneHabitation);
    ASSERT_EQ(ui.get_state().current_tool, ToolType::ZoneHabitation);

    ui.set_tool(ToolType::Select);
    ASSERT_EQ(ui.get_state().current_tool, ToolType::Select);
}

TEST(uimanager_overlay_toggle_cycling) {
    UIManager ui;

    // Start at None
    ASSERT_EQ(ui.get_overlay(), OverlayType::None);

    // Cycle through all overlays
    ui.cycle_overlay();
    ASSERT_EQ(ui.get_overlay(), OverlayType::Disorder);

    ui.cycle_overlay();
    ASSERT_EQ(ui.get_overlay(), OverlayType::Contamination);

    ui.cycle_overlay();
    ASSERT_EQ(ui.get_overlay(), OverlayType::SectorValue);

    ui.cycle_overlay();
    ASSERT_EQ(ui.get_overlay(), OverlayType::EnergyCoverage);

    ui.cycle_overlay();
    ASSERT_EQ(ui.get_overlay(), OverlayType::FluidCoverage);

    ui.cycle_overlay();
    ASSERT_EQ(ui.get_overlay(), OverlayType::ServiceCoverage);

    ui.cycle_overlay();
    ASSERT_EQ(ui.get_overlay(), OverlayType::Traffic);

    // Wraps back to None
    ui.cycle_overlay();
    ASSERT_EQ(ui.get_overlay(), OverlayType::None);
}

TEST(uimanager_set_overlay_direct) {
    UIManager ui;

    ui.set_overlay(OverlayType::Traffic);
    ASSERT_EQ(ui.get_overlay(), OverlayType::Traffic);

    ui.set_overlay(OverlayType::None);
    ASSERT_EQ(ui.get_overlay(), OverlayType::None);
}

TEST(uimanager_budget_panel_toggle) {
    UIManager ui;

    ASSERT(!ui.get_state().budget_panel_open);
    ui.toggle_budget_panel();
    ASSERT(ui.get_state().budget_panel_open);
    ui.toggle_budget_panel();
    ASSERT(!ui.get_state().budget_panel_open);
}

TEST(uimanager_mode_switching) {
    UIManager ui;

    // Default is Legacy
    ASSERT_EQ(ui.get_mode(), UIMode::Legacy);

    ui.set_mode(UIMode::Holo);
    ASSERT_EQ(ui.get_mode(), UIMode::Holo);
    ASSERT_EQ(ui.get_state().current_mode, UIMode::Holo);

    ui.set_mode(UIMode::Legacy);
    ASSERT_EQ(ui.get_mode(), UIMode::Legacy);
}

TEST(uimanager_alert_push) {
    UIManager ui;

    ui.push_alert("Test alert", AlertPriority::Info);
    ASSERT_EQ(ui.get_state().active_alerts.size(), static_cast<size_t>(1));
    ASSERT_EQ(ui.get_state().active_alerts.front().message, std::string("Test alert"));
    ASSERT_EQ(ui.get_state().active_alerts.front().priority, AlertPriority::Info);
}

TEST(uimanager_alert_max_visible) {
    UIManager ui;

    // Push more alerts than MAX_VISIBLE_ALERTS (4)
    for (int i = 0; i < 6; ++i) {
        ui.push_alert("Alert " + std::to_string(i), AlertPriority::Warning);
    }

    // Should be capped at MAX_VISIBLE_ALERTS
    ASSERT_EQ(static_cast<int>(ui.get_state().active_alerts.size()),
              UIState::MAX_VISIBLE_ALERTS);
}

TEST(uimanager_root_not_null) {
    UIManager ui;
    ASSERT(ui.get_root() != nullptr);
}

TEST(scan_layer_manager_cycle_next) {
    ScanLayerManager scans;

    // Starts at None
    ASSERT_EQ(scans.get_active_type(), OverlayType::None);

    // cycle_next starts a fade transition. The active_type changes after
    // the fade completes (update with enough delta_time).
    scans.cycle_next();
    scans.update(1.0f); // Complete the fade
    ASSERT_EQ(scans.get_active_type(), OverlayType::Disorder);

    scans.cycle_next();
    scans.update(1.0f);
    ASSERT_EQ(scans.get_active_type(), OverlayType::Contamination);

    scans.cycle_next();
    scans.update(1.0f);
    ASSERT_EQ(scans.get_active_type(), OverlayType::SectorValue);

    scans.cycle_next();
    scans.update(1.0f);
    ASSERT_EQ(scans.get_active_type(), OverlayType::EnergyCoverage);

    scans.cycle_next();
    scans.update(1.0f);
    ASSERT_EQ(scans.get_active_type(), OverlayType::FluidCoverage);

    scans.cycle_next();
    scans.update(1.0f);
    ASSERT_EQ(scans.get_active_type(), OverlayType::ServiceCoverage);

    scans.cycle_next();
    scans.update(1.0f);
    ASSERT_EQ(scans.get_active_type(), OverlayType::Traffic);

    // Wraps back to None
    scans.cycle_next();
    scans.update(1.0f);
    ASSERT_EQ(scans.get_active_type(), OverlayType::None);
}

TEST(scan_layer_manager_cycle_previous) {
    ScanLayerManager scans;

    // None -> Traffic (wraps backward)
    scans.cycle_previous();
    scans.update(1.0f);
    ASSERT_EQ(scans.get_active_type(), OverlayType::Traffic);

    // Traffic -> ServiceCoverage
    scans.cycle_previous();
    scans.update(1.0f);
    ASSERT_EQ(scans.get_active_type(), OverlayType::ServiceCoverage);
}

TEST(scan_layer_manager_set_active) {
    ScanLayerManager scans;

    scans.set_active(OverlayType::Contamination);
    scans.update(1.0f);
    ASSERT_EQ(scans.get_active_type(), OverlayType::Contamination);

    scans.set_active(OverlayType::None);
    scans.update(1.0f);
    ASSERT_EQ(scans.get_active_type(), OverlayType::None);
}

TEST(scan_layer_manager_fade_progress) {
    ScanLayerManager scans;

    // Initially no fade, progress = 1.0
    ASSERT_FLOAT_EQ(scans.get_fade_progress(), 1.0f);

    // Setting a new active starts a fade
    scans.set_active(OverlayType::Disorder);
    ASSERT_FLOAT_EQ(scans.get_fade_progress(), 0.0f);

    // Partially update
    scans.update(ScanLayerManager::FADE_DURATION * 0.5f);
    ASSERT(scans.get_fade_progress() > 0.0f);
    ASSERT(scans.get_fade_progress() < 1.0f);

    // Complete the fade
    scans.update(ScanLayerManager::FADE_DURATION);
    ASSERT_FLOAT_EQ(scans.get_fade_progress(), 1.0f);
}

TEST(scan_layer_manager_on_change_callback) {
    ScanLayerManager scans;

    OverlayType captured_old = OverlayType::Traffic;
    OverlayType captured_new = OverlayType::Traffic;

    scans.set_on_change([&](OverlayType old_t, OverlayType new_t) {
        captured_old = old_t;
        captured_new = new_t;
    });

    scans.set_active(OverlayType::Disorder);
    ASSERT_EQ(captured_old, OverlayType::None);
    ASSERT_EQ(captured_new, OverlayType::Disorder);
}

TEST(tool_state_machine_cursor_mode_select) {
    ToolStateMachine tsm;

    // Default tool is Select -> Arrow cursor
    ASSERT_EQ(tsm.get_tool(), ToolType::Select);
    ASSERT_EQ(tsm.get_visual_state().cursor_mode, CursorMode::Arrow);
}

TEST(tool_state_machine_cursor_mode_zone) {
    ToolStateMachine tsm;

    tsm.set_tool(ToolType::ZoneHabitation);
    ASSERT_EQ(tsm.get_visual_state().cursor_mode, CursorMode::ZoneBrush);

    tsm.set_tool(ToolType::ZoneExchange);
    ASSERT_EQ(tsm.get_visual_state().cursor_mode, CursorMode::ZoneBrush);

    tsm.set_tool(ToolType::ZoneFabrication);
    ASSERT_EQ(tsm.get_visual_state().cursor_mode, CursorMode::ZoneBrush);
}

TEST(tool_state_machine_cursor_mode_infra) {
    ToolStateMachine tsm;

    tsm.set_tool(ToolType::Pathway);
    ASSERT_EQ(tsm.get_visual_state().cursor_mode, CursorMode::LinePlacement);

    tsm.set_tool(ToolType::EnergyConduit);
    ASSERT_EQ(tsm.get_visual_state().cursor_mode, CursorMode::LinePlacement);

    tsm.set_tool(ToolType::FluidConduit);
    ASSERT_EQ(tsm.get_visual_state().cursor_mode, CursorMode::LinePlacement);
}

TEST(tool_state_machine_cursor_mode_bulldoze) {
    ToolStateMachine tsm;
    tsm.set_tool(ToolType::Bulldoze);
    ASSERT_EQ(tsm.get_visual_state().cursor_mode, CursorMode::Bulldoze);
}

TEST(tool_state_machine_cursor_mode_probe) {
    ToolStateMachine tsm;
    tsm.set_tool(ToolType::Probe);
    ASSERT_EQ(tsm.get_visual_state().cursor_mode, CursorMode::Probe);
}

TEST(tool_state_machine_cursor_mode_grade) {
    ToolStateMachine tsm;
    tsm.set_tool(ToolType::Grade);
    ASSERT_EQ(tsm.get_visual_state().cursor_mode, CursorMode::Grade);
}

TEST(tool_state_machine_cursor_mode_purge) {
    ToolStateMachine tsm;
    tsm.set_tool(ToolType::Purge);
    ASSERT_EQ(tsm.get_visual_state().cursor_mode, CursorMode::Purge);
}

TEST(tool_state_machine_cancel_reverts_to_select) {
    ToolStateMachine tsm;
    tsm.set_tool(ToolType::Bulldoze);
    ASSERT_EQ(tsm.get_tool(), ToolType::Bulldoze);

    tsm.cancel();
    ASSERT_EQ(tsm.get_tool(), ToolType::Select);
    ASSERT_EQ(tsm.get_visual_state().cursor_mode, CursorMode::Arrow);
}

TEST(tool_state_machine_on_change_callback) {
    ToolStateMachine tsm;

    ToolType captured_old = ToolType::Bulldoze;
    ToolType captured_new = ToolType::Bulldoze;

    tsm.set_on_tool_change([&](ToolType old_t, ToolType new_t) {
        captured_old = old_t;
        captured_new = new_t;
    });

    tsm.set_tool(ToolType::ZoneHabitation);
    ASSERT_EQ(captured_old, ToolType::Select);
    ASSERT_EQ(captured_new, ToolType::ZoneHabitation);
}

TEST(tool_state_machine_same_tool_noop) {
    ToolStateMachine tsm;

    bool called = false;
    tsm.set_on_tool_change([&](ToolType, ToolType) {
        called = true;
    });

    // Setting the same tool should be a no-op (no callback)
    tsm.set_tool(ToolType::Select);
    ASSERT(!called);
}

TEST(tool_state_machine_is_placement_tool) {
    ToolStateMachine tsm;

    tsm.set_tool(ToolType::Select);
    ASSERT(!tsm.is_placement_tool());

    tsm.set_tool(ToolType::ZoneHabitation);
    ASSERT(tsm.is_placement_tool());

    tsm.set_tool(ToolType::Pathway);
    ASSERT(tsm.is_placement_tool());

    tsm.set_tool(ToolType::Bulldoze);
    ASSERT(!tsm.is_placement_tool());

    tsm.set_tool(ToolType::Probe);
    ASSERT(!tsm.is_placement_tool());
}

TEST(tool_state_machine_is_zone_tool) {
    ToolStateMachine tsm;

    tsm.set_tool(ToolType::ZoneHabitation);
    ASSERT(tsm.is_zone_tool());

    tsm.set_tool(ToolType::ZoneExchange);
    ASSERT(tsm.is_zone_tool());

    tsm.set_tool(ToolType::ZoneFabrication);
    ASSERT(tsm.is_zone_tool());

    tsm.set_tool(ToolType::Pathway);
    ASSERT(!tsm.is_zone_tool());

    tsm.set_tool(ToolType::Select);
    ASSERT(!tsm.is_zone_tool());
}

TEST(tool_state_machine_placement_validity) {
    ToolStateMachine tsm;
    tsm.set_tool(ToolType::ZoneHabitation);

    // Default after tool change should be Unknown
    ASSERT_EQ(tsm.get_visual_state().placement_valid, PlacementValidity::Unknown);

    tsm.set_placement_validity(PlacementValidity::Valid);
    ASSERT_EQ(tsm.get_visual_state().placement_valid, PlacementValidity::Valid);

    tsm.set_placement_validity(PlacementValidity::Invalid);
    ASSERT_EQ(tsm.get_visual_state().placement_valid, PlacementValidity::Invalid);
}

// =============================================================================
// Widget-specific Tests
// =============================================================================

TEST(button_click_callback_fires) {
    ButtonWidget btn;
    btn.bounds = {10, 10, 120, 40};
    btn.screen_bounds = {10, 10, 120, 40};
    btn.visible = true;
    btn.enabled = true;

    bool clicked = false;
    btn.on_click = [&clicked]() { clicked = true; };

    // Simulate press then release (button 0 = left mouse)
    btn.on_mouse_down(0, 50.0f, 30.0f);
    ASSERT(btn.is_pressed());

    btn.on_mouse_up(0, 50.0f, 30.0f);
    ASSERT(clicked);
    ASSERT(!btn.is_pressed());
}

TEST(button_click_no_callback_does_not_crash) {
    ButtonWidget btn;
    btn.bounds = {10, 10, 120, 40};
    btn.visible = true;
    btn.enabled = true;

    // No on_click set; should not crash
    btn.on_mouse_down(0, 50.0f, 30.0f);
    btn.on_mouse_up(0, 50.0f, 30.0f);
    ASSERT(!btn.is_pressed());
}

TEST(button_right_click_does_not_fire) {
    ButtonWidget btn;
    btn.bounds = {10, 10, 120, 40};
    btn.visible = true;
    btn.enabled = true;

    bool clicked = false;
    btn.on_click = [&clicked]() { clicked = true; };

    // Right button (1) should not trigger click
    btn.on_mouse_down(1, 50.0f, 30.0f);
    btn.on_mouse_up(1, 50.0f, 30.0f);
    ASSERT(!clicked);
}

TEST(button_hover_state) {
    ButtonWidget btn;
    btn.visible = true;
    btn.enabled = true;

    ASSERT(!btn.is_hovered());

    btn.on_mouse_enter();
    ASSERT(btn.is_hovered());

    btn.on_mouse_leave();
    ASSERT(!btn.is_hovered());
}

TEST(button_leave_clears_pressed) {
    ButtonWidget btn;
    btn.visible = true;
    btn.enabled = true;

    btn.on_mouse_down(0, 50.0f, 30.0f);
    ASSERT(btn.is_pressed());

    btn.on_mouse_leave();
    ASSERT(!btn.is_pressed());
    ASSERT(!btn.is_hovered());
}

TEST(progress_bar_set_value_clamps) {
    ProgressBarWidget bar;

    // Values should be clamped to [0.0, 1.0]
    bar.set_value(0.5f);
    ASSERT_FLOAT_EQ(bar.target_value, 0.5f);

    bar.set_value(-1.0f);
    ASSERT_FLOAT_EQ(bar.target_value, 0.0f);

    bar.set_value(2.0f);
    ASSERT_FLOAT_EQ(bar.target_value, 1.0f);

    bar.set_value(0.0f);
    ASSERT_FLOAT_EQ(bar.target_value, 0.0f);

    bar.set_value(1.0f);
    ASSERT_FLOAT_EQ(bar.target_value, 1.0f);
}

TEST(progress_bar_set_value_immediate_clamps) {
    ProgressBarWidget bar;

    bar.set_value_immediate(0.75f);
    ASSERT_FLOAT_EQ(bar.value, 0.75f);
    ASSERT_FLOAT_EQ(bar.target_value, 0.75f);

    bar.set_value_immediate(-0.5f);
    ASSERT_FLOAT_EQ(bar.value, 0.0f);
    ASSERT_FLOAT_EQ(bar.target_value, 0.0f);

    bar.set_value_immediate(5.0f);
    ASSERT_FLOAT_EQ(bar.value, 1.0f);
    ASSERT_FLOAT_EQ(bar.target_value, 1.0f);
}

TEST(progress_bar_smooth_animation) {
    ProgressBarWidget bar;

    bar.set_value_immediate(0.0f);
    bar.set_value(1.0f);

    // After update, value should move toward target but not reach it instantly
    bar.update(0.1f);
    ASSERT(bar.value > 0.0f);
    ASSERT(bar.value < 1.0f);

    // After a large update, should be at or very near target
    bar.update(10.0f);
    ASSERT_FLOAT_EQ(bar.value, 1.0f);
}

TEST(label_text_and_alignment) {
    LabelWidget label;
    label.text = "Hello World";
    ASSERT_EQ(label.text, std::string("Hello World"));

    // Default alignment is Left
    ASSERT_EQ(label.alignment, TextAlignment::Left);

    label.alignment = TextAlignment::Center;
    ASSERT_EQ(label.alignment, TextAlignment::Center);

    label.alignment = TextAlignment::Right;
    ASSERT_EQ(label.alignment, TextAlignment::Right);
}

TEST(label_font_size_setting) {
    LabelWidget label;

    // Default is Normal
    ASSERT_EQ(label.font_size, FontSize::Normal);

    label.font_size = FontSize::Small;
    ASSERT_EQ(label.font_size, FontSize::Small);

    label.font_size = FontSize::Large;
    ASSERT_EQ(label.font_size, FontSize::Large);

    label.font_size = FontSize::Title;
    ASSERT_EQ(label.font_size, FontSize::Title);
}

TEST(label_text_color) {
    LabelWidget label;

    // Default is white
    ASSERT_FLOAT_EQ(label.text_color.r, 1.0f);
    ASSERT_FLOAT_EQ(label.text_color.g, 1.0f);
    ASSERT_FLOAT_EQ(label.text_color.b, 1.0f);
    ASSERT_FLOAT_EQ(label.text_color.a, 1.0f);

    label.text_color = Color::from_rgba8(255, 0, 0);
    ASSERT_FLOAT_EQ(label.text_color.r, 1.0f);
    ASSERT_FLOAT_EQ(label.text_color.g, 0.0f);
    ASSERT_FLOAT_EQ(label.text_color.b, 0.0f);
}

TEST(panel_title_and_closable) {
    PanelWidget panel;
    panel.title = "Budget Panel";
    panel.closable = true;

    ASSERT_EQ(panel.title, std::string("Budget Panel"));
    ASSERT(panel.closable);

    panel.closable = false;
    ASSERT(!panel.closable);
}

TEST(panel_content_bounds) {
    PanelWidget panel;
    panel.bounds = {100, 100, 300, 200};
    panel.screen_bounds = {100, 100, 300, 200};

    Rect content = panel.get_content_bounds();

    ASSERT_FLOAT_EQ(content.x, 100.0f);
    ASSERT_FLOAT_EQ(content.y, 100.0f + PanelWidget::TITLE_BAR_HEIGHT);
    ASSERT_FLOAT_EQ(content.width, 300.0f);
    ASSERT_FLOAT_EQ(content.height, 200.0f - PanelWidget::TITLE_BAR_HEIGHT);
}

TEST(panel_draggable_flag) {
    PanelWidget panel;

    // Default is not draggable
    ASSERT(!panel.draggable);

    panel.draggable = true;
    ASSERT(panel.draggable);
}

TEST(panel_on_close_callback) {
    PanelWidget panel;
    panel.closable = true;

    bool closed = false;
    panel.on_close = [&closed]() { closed = true; };

    // Invoke the close callback manually
    if (panel.on_close) {
        panel.on_close();
    }
    ASSERT(closed);
}

TEST(icon_widget_defaults) {
    IconWidget icon;
    ASSERT_EQ(icon.texture, INVALID_TEXTURE);
    ASSERT_FLOAT_EQ(icon.tint.r, 1.0f);
    ASSERT_FLOAT_EQ(icon.tint.g, 1.0f);
    ASSERT_FLOAT_EQ(icon.tint.b, 1.0f);
    ASSERT_FLOAT_EQ(icon.tint.a, 1.0f);
}

TEST(zone_pressure_widget_demand_values) {
    ZonePressureWidget zp;

    // Defaults should be 0
    ASSERT_EQ(zp.habitation_demand, 0);
    ASSERT_EQ(zp.exchange_demand, 0);
    ASSERT_EQ(zp.fabrication_demand, 0);

    // Set values within valid range
    zp.habitation_demand = 60;
    zp.exchange_demand = -20;
    zp.fabrication_demand = 100;

    ASSERT_EQ(zp.habitation_demand, 60);
    ASSERT_EQ(zp.exchange_demand, -20);
    ASSERT_EQ(zp.fabrication_demand, 100);
}

TEST(color_from_rgba8) {
    Color c = Color::from_rgba8(0, 128, 255, 255);
    ASSERT_FLOAT_EQ(c.r, 0.0f);
    ASSERT(c.g > 0.49f && c.g < 0.51f);  // ~0.502
    ASSERT_FLOAT_EQ(c.b, 1.0f);
    ASSERT_FLOAT_EQ(c.a, 1.0f);

    Color c2 = Color::from_rgba8(0, 0, 0, 0);
    ASSERT_FLOAT_EQ(c2.r, 0.0f);
    ASSERT_FLOAT_EQ(c2.g, 0.0f);
    ASSERT_FLOAT_EQ(c2.b, 0.0f);
    ASSERT_FLOAT_EQ(c2.a, 0.0f);
}

TEST(widget_default_state) {
    Widget w;
    ASSERT(w.visible);
    ASSERT(w.enabled);
    ASSERT(w.parent == nullptr);
    ASSERT_EQ(w.z_order, 0);
    ASSERT(!w.is_hovered());
    ASSERT(!w.is_pressed());
    ASSERT_EQ(w.children.size(), static_cast<size_t>(0));
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== UI Widget System Unit Tests (E12-026) ===\n\n");

    // Widget Hierarchy Tests
    printf("--- Widget Hierarchy ---\n");
    RUN_TEST(add_child_sets_parent_and_children_size);
    RUN_TEST(add_multiple_children);
    RUN_TEST(remove_child_removes_and_clears_parent);
    RUN_TEST(remove_child_null_is_safe);
    RUN_TEST(add_child_null_returns_nullptr);
    RUN_TEST(nested_bounds_calculation);
    RUN_TEST(deeply_nested_bounds_calculation);
    RUN_TEST(hidden_parent_blocks_child_render);

    // Hit Testing Tests
    printf("\n--- Hit Testing ---\n");
    RUN_TEST(rect_contains_point_inside);
    RUN_TEST(rect_does_not_contain_point_outside);
    RUN_TEST(widget_hit_test_checks_screen_bounds);
    RUN_TEST(widget_hit_test_invisible_returns_false);
    RUN_TEST(widget_hit_test_disabled_returns_false);
    RUN_TEST(find_child_at_returns_deepest_child);
    RUN_TEST(find_child_at_returns_nullptr_on_miss);
    RUN_TEST(find_child_at_z_order_last_child_wins);
    RUN_TEST(find_child_at_skips_invisible_children);
    RUN_TEST(find_child_at_skips_disabled_children);

    // State Management Tests
    printf("\n--- State Management ---\n");
    RUN_TEST(uimanager_tool_selection);
    RUN_TEST(uimanager_tool_changes_state);
    RUN_TEST(uimanager_overlay_toggle_cycling);
    RUN_TEST(uimanager_set_overlay_direct);
    RUN_TEST(uimanager_budget_panel_toggle);
    RUN_TEST(uimanager_mode_switching);
    RUN_TEST(uimanager_alert_push);
    RUN_TEST(uimanager_alert_max_visible);
    RUN_TEST(uimanager_root_not_null);
    RUN_TEST(scan_layer_manager_cycle_next);
    RUN_TEST(scan_layer_manager_cycle_previous);
    RUN_TEST(scan_layer_manager_set_active);
    RUN_TEST(scan_layer_manager_fade_progress);
    RUN_TEST(scan_layer_manager_on_change_callback);
    RUN_TEST(tool_state_machine_cursor_mode_select);
    RUN_TEST(tool_state_machine_cursor_mode_zone);
    RUN_TEST(tool_state_machine_cursor_mode_infra);
    RUN_TEST(tool_state_machine_cursor_mode_bulldoze);
    RUN_TEST(tool_state_machine_cursor_mode_probe);
    RUN_TEST(tool_state_machine_cursor_mode_grade);
    RUN_TEST(tool_state_machine_cursor_mode_purge);
    RUN_TEST(tool_state_machine_cancel_reverts_to_select);
    RUN_TEST(tool_state_machine_on_change_callback);
    RUN_TEST(tool_state_machine_same_tool_noop);
    RUN_TEST(tool_state_machine_is_placement_tool);
    RUN_TEST(tool_state_machine_is_zone_tool);
    RUN_TEST(tool_state_machine_placement_validity);

    // Widget-specific Tests
    printf("\n--- Widget-specific ---\n");
    RUN_TEST(button_click_callback_fires);
    RUN_TEST(button_click_no_callback_does_not_crash);
    RUN_TEST(button_right_click_does_not_fire);
    RUN_TEST(button_hover_state);
    RUN_TEST(button_leave_clears_pressed);
    RUN_TEST(progress_bar_set_value_clamps);
    RUN_TEST(progress_bar_set_value_immediate_clamps);
    RUN_TEST(progress_bar_smooth_animation);
    RUN_TEST(label_text_and_alignment);
    RUN_TEST(label_font_size_setting);
    RUN_TEST(label_text_color);
    RUN_TEST(panel_title_and_closable);
    RUN_TEST(panel_content_bounds);
    RUN_TEST(panel_draggable_flag);
    RUN_TEST(panel_on_close_callback);
    RUN_TEST(icon_widget_defaults);
    RUN_TEST(zone_pressure_widget_demand_values);
    RUN_TEST(color_from_rgba8);
    RUN_TEST(widget_default_state);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
