/**
 * @file RadialMenu.cpp
 * @brief Implementation of the holographic radial menu.
 */

#include "sims3000/ui/RadialMenu.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace sims3000 {
namespace ui {

// =========================================================================
// Construction
// =========================================================================

RadialMenu::RadialMenu() {
    visible = false;
}

// =========================================================================
// Category setup
// =========================================================================

void RadialMenu::build_default_categories() {
    m_categories.clear();

    // BUILD category
    {
        RadialCategory cat;
        cat.label = "BUILD";
        cat.items = {
            {"HABITATION",   ToolType::ZoneHabitation, nullptr},
            {"EXCHANGE",     ToolType::ZoneExchange,   nullptr},
            {"FABRICATION",  ToolType::ZoneFabrication, nullptr},
            {"PATHWAY",      ToolType::Pathway,         nullptr},
            {"ENERGY",       ToolType::EnergyConduit,   nullptr},
            {"FLUID",        ToolType::FluidConduit,    nullptr}
        };
        m_categories.push_back(std::move(cat));
    }

    // MODIFY category
    {
        RadialCategory cat;
        cat.label = "MODIFY";
        cat.items = {
            {"BULLDOZE", ToolType::Bulldoze, nullptr},
            {"PURGE",    ToolType::Purge,    nullptr},
            {"GRADE",    ToolType::Grade,    nullptr}
        };
        m_categories.push_back(std::move(cat));
    }

    // INSPECT category
    {
        RadialCategory cat;
        cat.label = "INSPECT";
        cat.items = {
            {"PROBE", ToolType::Probe, nullptr}
        };
        m_categories.push_back(std::move(cat));
    }

    // VIEW category
    {
        RadialCategory cat;
        cat.label = "VIEW";
        cat.items = {
            {"SELECT", ToolType::Select, nullptr}
        };
        m_categories.push_back(std::move(cat));
    }
}

// =========================================================================
// Show / hide
// =========================================================================

void RadialMenu::show(float center_x, float center_y) {
    m_center_x = center_x;
    m_center_y = center_y;
    m_open = true;
    m_animation_progress = 0.0f;
    m_hovered_category = -1;
    m_hovered_item = -1;
    visible = true;
}

void RadialMenu::hide() {
    m_open = false;
    m_hovered_category = -1;
    m_hovered_item = -1;
    visible = false;
}

bool RadialMenu::is_open() const {
    return m_open;
}

void RadialMenu::set_tool_callback(ToolSelectCallback callback) {
    m_tool_callback = std::move(callback);
}

// =========================================================================
// Update
// =========================================================================

void RadialMenu::update(float delta_time) {
    if (m_open) {
        // Animate toward fully open (1.0)
        float blend = std::min(ANIMATION_SPEED * delta_time, 1.0f);
        m_animation_progress += (1.0f - m_animation_progress) * blend;
        m_animation_progress = std::min(m_animation_progress, 1.0f);
    } else {
        // Animate toward closed (0.0)
        float blend = std::min(ANIMATION_SPEED * delta_time, 1.0f);
        m_animation_progress -= m_animation_progress * blend;
        m_animation_progress = std::max(m_animation_progress, 0.0f);

        // Once fully closed, hide the widget
        if (m_animation_progress < 0.01f) {
            visible = false;
        }
    }

    Widget::update(delta_time);
}

// =========================================================================
// Render
// =========================================================================

void RadialMenu::render(UIRenderer* renderer) {
    if (!visible || m_animation_progress < 0.01f) {
        return;
    }

    const int num_categories = static_cast<int>(m_categories.size());
    if (num_categories == 0) {
        return;
    }

    const float scale = m_animation_progress;
    const float two_pi = static_cast<float>(2.0 * M_PI);
    const float mid_radius = (INNER_RADIUS + OUTER_RADIUS) * 0.5f * scale;

    // -- Inner ring: draw a segment rect and label for each category ------

    for (int i = 0; i < num_categories; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(num_categories)) * two_pi;
        // Offset by -PI/2 so category 0 is at the top
        angle -= static_cast<float>(M_PI * 0.5);

        float seg_x = m_center_x + mid_radius * std::cos(angle);
        float seg_y = m_center_y + mid_radius * std::sin(angle);

        // Segment rectangle (centered on the angular position)
        constexpr float seg_w = 60.0f;
        constexpr float seg_h = 24.0f;
        Rect seg_bounds = {
            seg_x - seg_w * 0.5f,
            seg_y - seg_h * 0.5f,
            seg_w,
            seg_h
        };

        // Highlight the hovered category
        Color fill;
        Color border;
        if (i == m_hovered_category) {
            fill = {0.0f, 0.8f, 0.8f, 0.9f * scale};
            border = {0.0f, 1.0f, 1.0f, 1.0f * scale};
        } else {
            fill = {0.1f, 0.1f, 0.15f, 0.8f * scale};
            border = {0.0f, 0.6f, 0.6f, 0.6f * scale};
        }

        renderer->draw_rect(seg_bounds, fill, border);

        // Category label
        Color label_color = {1.0f, 1.0f, 1.0f, 1.0f * scale};
        renderer->draw_text(m_categories[i].label,
                            seg_bounds.x + 4.0f,
                            seg_bounds.y + 4.0f,
                            FontSize::Small, label_color);
    }

    // -- Outer ring: draw sub-items for the hovered category --------------

    if (m_hovered_category >= 0 && m_hovered_category < num_categories) {
        const auto& items = m_categories[m_hovered_category].items;
        const int num_items = static_cast<int>(items.size());

        if (num_items > 0) {
            // Spread items around the angular position of the hovered category
            float cat_angle = (static_cast<float>(m_hovered_category) /
                               static_cast<float>(num_categories)) * two_pi;
            cat_angle -= static_cast<float>(M_PI * 0.5);

            // Angular span for sub-items: distribute within one category sector
            float sector_span = two_pi / static_cast<float>(num_categories);
            // Offset the start so items are centered around the category angle
            float start_angle = cat_angle - sector_span * 0.5f;
            float item_step = (num_items > 1)
                ? sector_span / static_cast<float>(num_items)
                : 0.0f;

            float sub_radius = (OUTER_RADIUS + SUB_RING_RADIUS) * 0.5f * scale;

            for (int j = 0; j < num_items; ++j) {
                float item_angle = start_angle + item_step * (static_cast<float>(j) + 0.5f);

                float item_x = m_center_x + sub_radius * std::cos(item_angle);
                float item_y = m_center_y + sub_radius * std::sin(item_angle);

                constexpr float item_w = 80.0f;
                constexpr float item_h = 22.0f;
                Rect item_bounds = {
                    item_x - item_w * 0.5f,
                    item_y - item_h * 0.5f,
                    item_w,
                    item_h
                };

                Color item_fill;
                Color item_border;
                if (j == m_hovered_item) {
                    item_fill = {0.0f, 0.9f, 0.6f, 0.9f * scale};
                    item_border = {0.0f, 1.0f, 0.8f, 1.0f * scale};
                } else {
                    item_fill = {0.05f, 0.12f, 0.15f, 0.8f * scale};
                    item_border = {0.0f, 0.5f, 0.5f, 0.6f * scale};
                }

                renderer->draw_rect(item_bounds, item_fill, item_border);

                Color item_label_color = {1.0f, 1.0f, 1.0f, 1.0f * scale};
                renderer->draw_text(items[j].label,
                                    item_bounds.x + 4.0f,
                                    item_bounds.y + 3.0f,
                                    FontSize::Small, item_label_color);
            }
        }
    }

    // -- Center indicator (dead zone) -------------------------------------
    {
        constexpr float indicator_size = 16.0f;
        Rect center_rect = {
            m_center_x - indicator_size * 0.5f,
            m_center_y - indicator_size * 0.5f,
            indicator_size,
            indicator_size
        };
        Color center_fill = {0.0f, 0.6f, 0.6f, 0.5f * scale};
        Color center_border = {0.0f, 0.8f, 0.8f, 0.8f * scale};
        renderer->draw_rect(center_rect, center_fill, center_border);
    }

    // Do not call Widget::render -- radial menu has no child widgets
}

// =========================================================================
// Hit testing
// =========================================================================

bool RadialMenu::hit_test(float x, float y) const {
    if (!visible || !m_open) {
        return false;
    }
    return distance_from_center(x, y) < SUB_RING_RADIUS;
}

// =========================================================================
// Mouse events
// =========================================================================

void RadialMenu::on_mouse_move(float x, float y) {
    if (!m_open) {
        return;
    }

    m_hovered_category = get_category_at(x, y);
    m_hovered_item = -1;

    if (m_hovered_category >= 0) {
        m_hovered_item = get_item_at(x, y);
    }
}

void RadialMenu::on_mouse_up(int button, float /*x*/, float /*y*/) {
    if (!m_open) {
        return;
    }

    // Accept left or right mouse button release
    if (button != 0 && button != 1) {
        return;
    }

    if (m_hovered_category >= 0 && m_hovered_item >= 0) {
        const auto& item = m_categories[m_hovered_category].items[m_hovered_item];

        // Fire the item's own callback if set
        if (item.on_select) {
            item.on_select();
        }

        // Notify through the tool callback
        if (m_tool_callback) {
            m_tool_callback(item.tool);
        }

        hide();
    } else {
        // Released outside any item -- close the menu without selecting
        hide();
    }
}

// =========================================================================
// Geometry helpers
// =========================================================================

float RadialMenu::distance_from_center(float x, float y) const {
    float dx = x - m_center_x;
    float dy = y - m_center_y;
    return std::sqrt(dx * dx + dy * dy);
}

float RadialMenu::angle_from_center(float x, float y) const {
    return std::atan2(y - m_center_y, x - m_center_x);
}

int RadialMenu::get_category_at(float x, float y) const {
    float dist = distance_from_center(x, y);
    float scaled_inner = INNER_RADIUS * m_animation_progress;
    float scaled_outer = OUTER_RADIUS * m_animation_progress;

    if (dist < scaled_inner || dist > scaled_outer) {
        return -1;
    }

    const int num_categories = static_cast<int>(m_categories.size());
    if (num_categories == 0) {
        return -1;
    }

    // Compute angle, offset by +PI/2 so category 0 is at the top
    float angle = angle_from_center(x, y) + static_cast<float>(M_PI * 0.5);

    // Normalize to [0, 2*PI)
    const float two_pi = static_cast<float>(2.0 * M_PI);
    while (angle < 0.0f) {
        angle += two_pi;
    }
    while (angle >= two_pi) {
        angle -= two_pi;
    }

    float sector_size = two_pi / static_cast<float>(num_categories);
    int index = static_cast<int>(angle / sector_size);

    if (index < 0 || index >= num_categories) {
        return -1;
    }

    return index;
}

int RadialMenu::get_item_at(float x, float y) const {
    if (m_hovered_category < 0) {
        return -1;
    }

    float dist = distance_from_center(x, y);
    float scaled_outer = OUTER_RADIUS * m_animation_progress;
    float scaled_sub = SUB_RING_RADIUS * m_animation_progress;

    if (dist < scaled_outer || dist > scaled_sub) {
        return -1;
    }

    const int num_categories = static_cast<int>(m_categories.size());
    const auto& items = m_categories[m_hovered_category].items;
    const int num_items = static_cast<int>(items.size());

    if (num_items == 0) {
        return -1;
    }

    const float two_pi = static_cast<float>(2.0 * M_PI);
    float sector_span = two_pi / static_cast<float>(num_categories);

    // Category center angle (with top-offset)
    float cat_angle = (static_cast<float>(m_hovered_category) /
                       static_cast<float>(num_categories)) * two_pi;

    // Compute mouse angle relative to category sector start
    float angle = angle_from_center(x, y) + static_cast<float>(M_PI * 0.5);
    while (angle < 0.0f) {
        angle += two_pi;
    }
    while (angle >= two_pi) {
        angle -= two_pi;
    }

    // Angle relative to the start of the category sector
    float sector_start = cat_angle - sector_span * 0.5f;
    while (sector_start < 0.0f) {
        sector_start += two_pi;
    }

    float rel_angle = angle - sector_start;
    while (rel_angle < 0.0f) {
        rel_angle += two_pi;
    }
    while (rel_angle >= two_pi) {
        rel_angle -= two_pi;
    }

    // Only accept if within the sector span
    if (rel_angle > sector_span) {
        return -1;
    }

    float item_step = sector_span / static_cast<float>(num_items);
    int index = static_cast<int>(rel_angle / item_step);

    if (index < 0 || index >= num_items) {
        return -1;
    }

    return index;
}

} // namespace ui
} // namespace sims3000
