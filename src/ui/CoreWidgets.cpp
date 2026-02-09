/**
 * @file CoreWidgets.cpp
 * @brief Implementation of core widget types: Button, Label, Panel, Icon.
 */

#include "sims3000/ui/CoreWidgets.h"

namespace sims3000 {
namespace ui {

// =========================================================================
// ButtonWidget
// =========================================================================

void ButtonWidget::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    ButtonState state = get_visual_state();

    if (icon != INVALID_TEXTURE) {
        renderer->draw_icon_button(screen_bounds, icon, state);
    } else {
        renderer->draw_button(screen_bounds, text, state);
    }

    // Render children
    Widget::render(renderer);
}

void ButtonWidget::on_mouse_down(int button, float /*x*/, float /*y*/) {
    if (button == 0) {
        set_pressed(true);
    }
}

void ButtonWidget::on_mouse_up(int button, float /*x*/, float /*y*/) {
    if (button == 0) {
        if (is_pressed() && on_click) {
            on_click();
        }
        set_pressed(false);
    }
}

void ButtonWidget::on_mouse_enter() {
    set_hovered(true);
}

void ButtonWidget::on_mouse_leave() {
    set_hovered(false);
    set_pressed(false);
}

ButtonState ButtonWidget::get_visual_state() const {
    if (!enabled) {
        return ButtonState::Disabled;
    }
    if (is_pressed()) {
        return ButtonState::Pressed;
    }
    if (is_hovered()) {
        return ButtonState::Hovered;
    }
    return ButtonState::Normal;
}

// =========================================================================
// LabelWidget
// =========================================================================

void LabelWidget::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    float text_x = screen_bounds.x;

    switch (alignment) {
        case TextAlignment::Left:
            text_x = screen_bounds.x;
            break;
        case TextAlignment::Center:
            text_x = screen_bounds.x + screen_bounds.width * 0.5f;
            break;
        case TextAlignment::Right:
            text_x = screen_bounds.x + screen_bounds.width;
            break;
    }

    renderer->draw_text(text, text_x, screen_bounds.y, font_size, text_color);

    // Render children
    Widget::render(renderer);
}

// =========================================================================
// PanelWidget
// =========================================================================

void PanelWidget::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    renderer->draw_panel(screen_bounds, title, closable);

    // Render children
    Widget::render(renderer);
}

Rect PanelWidget::get_content_bounds() const {
    return Rect{
        screen_bounds.x,
        screen_bounds.y + TITLE_BAR_HEIGHT,
        screen_bounds.width,
        screen_bounds.height - TITLE_BAR_HEIGHT
    };
}

// =========================================================================
// IconWidget
// =========================================================================

void IconWidget::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    if (texture != INVALID_TEXTURE) {
        renderer->draw_icon(screen_bounds, texture, tint);
    }

    // Render children
    Widget::render(renderer);
}

} // namespace ui
} // namespace sims3000
