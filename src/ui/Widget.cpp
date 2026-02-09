/**
 * @file Widget.cpp
 * @brief Widget base class implementation.
 */

#include "sims3000/ui/Widget.h"

#include <algorithm>

namespace sims3000 {
namespace ui {

// -- Rect ---------------------------------------------------------------

bool Rect::contains(float px, float py) const {
    return px >= x && px < x + width &&
           py >= y && py < y + height;
}

// -- Color --------------------------------------------------------------

Color Color::from_rgba8(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    constexpr float INV_255 = 1.0f / 255.0f;
    return Color{
        static_cast<float>(r) * INV_255,
        static_cast<float>(g) * INV_255,
        static_cast<float>(b) * INV_255,
        static_cast<float>(a) * INV_255
    };
}

// -- Widget lifecycle ----------------------------------------------------

void Widget::update(float delta_time) {
    for (auto& child : children) {
        if (child) {
            child->update(delta_time);
        }
    }
}

void Widget::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    for (auto& child : children) {
        if (child) {
            child->render(renderer);
        }
    }
}

// -- Hierarchy -----------------------------------------------------------

Widget* Widget::add_child(std::unique_ptr<Widget> child) {
    if (!child) {
        return nullptr;
    }

    child->parent = this;
    Widget* raw = child.get();
    children.push_back(std::move(child));
    return raw;
}

void Widget::remove_child(Widget* child) {
    if (!child) {
        return;
    }

    auto it = std::find_if(children.begin(), children.end(),
        [child](const std::unique_ptr<Widget>& ptr) {
            return ptr.get() == child;
        });

    if (it != children.end()) {
        (*it)->parent = nullptr;
        children.erase(it);
    }
}

Widget* Widget::find_child_at(float x, float y) {
    // Reverse iterate so higher-index (and typically higher z-order) children
    // are tested first, matching visual front-to-back order.
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto& child = *it;
        if (!child || !child->visible || !child->enabled) {
            continue;
        }

        // Recurse into this child first to find the deepest match
        Widget* deep = child->find_child_at(x, y);
        if (deep) {
            return deep;
        }

        // Check the child itself
        if (child->hit_test(x, y)) {
            return child.get();
        }
    }

    return nullptr;
}

// -- Screen bounds -------------------------------------------------------

void Widget::compute_screen_bounds() {
    if (parent) {
        screen_bounds.x = parent->screen_bounds.x + bounds.x;
        screen_bounds.y = parent->screen_bounds.y + bounds.y;
    } else {
        // Root widget: screen_bounds == bounds
        screen_bounds.x = bounds.x;
        screen_bounds.y = bounds.y;
    }

    screen_bounds.width = bounds.width;
    screen_bounds.height = bounds.height;

    for (auto& child : children) {
        if (child) {
            child->compute_screen_bounds();
        }
    }
}

// -- Hit testing ---------------------------------------------------------

bool Widget::hit_test(float x, float y) const {
    return visible && enabled && screen_bounds.contains(x, y);
}

// -- Mouse events (default no-ops) ----------------------------------------

void Widget::on_mouse_enter() {}
void Widget::on_mouse_leave() {}
void Widget::on_mouse_down(int /*button*/, float /*x*/, float /*y*/) {}
void Widget::on_mouse_up(int /*button*/, float /*x*/, float /*y*/) {}
void Widget::on_mouse_move(float /*x*/, float /*y*/) {}

} // namespace ui
} // namespace sims3000
