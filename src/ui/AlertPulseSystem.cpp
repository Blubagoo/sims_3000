/**
 * @file AlertPulseSystem.cpp
 * @brief Implementation of AlertPulseSystem: priority-based notification stack.
 */

#include "sims3000/ui/AlertPulseSystem.h"

#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace ui {

// =========================================================================
// Construction
// =========================================================================

AlertPulseSystem::AlertPulseSystem() {
    // Default bounds: positioned at top-right area.
    // The actual position should be set by the caller to match viewport layout.
    bounds = {0.0f, 0.0f, NOTIFICATION_WIDTH, MAX_VISIBLE * (NOTIFICATION_HEIGHT + SPACING)};
}

// =========================================================================
// Alert management
// =========================================================================

void AlertPulseSystem::push_alert(const std::string& message, AlertPriority priority,
                                  float focus_x, float focus_y) {
    AlertNotification notification;
    notification.message = message;
    notification.priority = priority;
    notification.duration = get_priority_duration(priority);
    notification.time_remaining = notification.duration;
    notification.fade_alpha = 1.0f;
    notification.has_audio = get_priority_has_audio(priority);
    notification.dismissed = false;

    // Set focus position if valid coordinates were provided
    if (focus_x >= 0.0f && focus_y >= 0.0f) {
        notification.has_focus_position = true;
        notification.focus_x = focus_x;
        notification.focus_y = focus_y;
    } else {
        notification.has_focus_position = false;
        notification.focus_x = -1.0f;
        notification.focus_y = -1.0f;
    }

    // Push newest to front (FIFO: newest at front, oldest at back)
    m_notifications.push_front(std::move(notification));

    // Enforce maximum visible limit by removing oldest
    while (static_cast<int>(m_notifications.size()) > MAX_VISIBLE) {
        m_notifications.pop_back();
    }
}

// =========================================================================
// Update
// =========================================================================

void AlertPulseSystem::update(float delta_time) {
    if (!visible) {
        return;
    }

    // Tick down timers and compute fade alpha
    for (auto& notif : m_notifications) {
        if (notif.dismissed) {
            continue;
        }

        notif.time_remaining -= delta_time;

        // Compute fade alpha during the final FADE_DURATION seconds
        if (notif.time_remaining <= 0.0f) {
            notif.time_remaining = 0.0f;
            notif.fade_alpha = 0.0f;
            notif.dismissed = true;
        } else if (notif.time_remaining < FADE_DURATION) {
            notif.fade_alpha = notif.time_remaining / FADE_DURATION;
        } else {
            notif.fade_alpha = 1.0f;
        }
    }

    // Remove dismissed / expired notifications
    m_notifications.erase(
        std::remove_if(m_notifications.begin(), m_notifications.end(),
                       [](const AlertNotification& n) { return n.dismissed; }),
        m_notifications.end());

    // Update children
    Widget::update(delta_time);
}

// =========================================================================
// Rendering
// =========================================================================

void AlertPulseSystem::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    int count = std::min(static_cast<int>(m_notifications.size()), MAX_VISIBLE);

    for (int i = 0; i < count; ++i) {
        const AlertNotification& notif = m_notifications[static_cast<size_t>(i)];

        Rect rect = get_notification_rect(i);

        // Get priority color and apply fade alpha
        Color base_color = get_priority_color(notif.priority);
        Color fill_color = {base_color.r, base_color.g, base_color.b, 0.85f * notif.fade_alpha};

        // Border is slightly brighter than the fill
        Color border_color = {
            std::min(base_color.r + 0.2f, 1.0f),
            std::min(base_color.g + 0.2f, 1.0f),
            std::min(base_color.b + 0.2f, 1.0f),
            0.95f * notif.fade_alpha
        };

        renderer->draw_rect(rect, fill_color, border_color);

        // Draw message text centered vertically within the notification
        Color text_color = {1.0f, 1.0f, 1.0f, notif.fade_alpha};
        float text_x = rect.x + 10.0f;
        float text_y = rect.y + (NOTIFICATION_HEIGHT - 14.0f) * 0.5f;
        renderer->draw_text(notif.message, text_x, text_y, FontSize::Small, text_color);
    }

    // Render children
    Widget::render(renderer);
}

// =========================================================================
// Mouse events
// =========================================================================

void AlertPulseSystem::on_mouse_down(int button, float x, float y) {
    if (button != 0) {
        return;
    }

    int count = std::min(static_cast<int>(m_notifications.size()), MAX_VISIBLE);

    for (int i = 0; i < count; ++i) {
        Rect rect = get_notification_rect(i);
        if (rect.contains(x, y)) {
            AlertNotification& notif = m_notifications[static_cast<size_t>(i)];

            // If the notification has a focus position and we have a callback, invoke it
            if (notif.has_focus_position && m_focus_callback) {
                m_focus_callback(notif.focus_x, notif.focus_y);
            }

            // Dismiss the notification
            notif.dismissed = true;
            return;
        }
    }
}

// =========================================================================
// Callbacks
// =========================================================================

void AlertPulseSystem::set_focus_callback(std::function<void(float x, float y)> callback) {
    m_focus_callback = std::move(callback);
}

// =========================================================================
// Query
// =========================================================================

int AlertPulseSystem::get_active_count() const {
    int count = 0;
    for (const auto& notif : m_notifications) {
        if (!notif.dismissed) {
            ++count;
        }
    }
    return count;
}

// =========================================================================
// Priority helpers
// =========================================================================

Color AlertPulseSystem::get_priority_color(AlertPriority priority) {
    switch (priority) {
        case AlertPriority::Critical:
            return {0.8f, 0.1f, 0.1f, 1.0f};   // Red pulse
        case AlertPriority::Warning:
            return {0.8f, 0.6f, 0.0f, 1.0f};   // Amber glow
        case AlertPriority::Info:
            return {0.0f, 0.7f, 0.8f, 1.0f};   // Cyan flash
        default:
            return {0.0f, 0.7f, 0.8f, 1.0f};   // Default to Info color
    }
}

float AlertPulseSystem::get_priority_duration(AlertPriority priority) {
    switch (priority) {
        case AlertPriority::Critical:
            return 8.0f;
        case AlertPriority::Warning:
            return 5.0f;
        case AlertPriority::Info:
            return 3.0f;
        default:
            return 3.0f;
    }
}

bool AlertPulseSystem::get_priority_has_audio(AlertPriority priority) {
    switch (priority) {
        case AlertPriority::Critical:
            return true;   // Alert chime
        case AlertPriority::Warning:
            return true;   // Soft tone
        case AlertPriority::Info:
            return false;  // No audio
        default:
            return false;
    }
}

// =========================================================================
// Layout helpers
// =========================================================================

Rect AlertPulseSystem::get_notification_rect(int index) const {
    // Notifications stack from the top of the widget's screen_bounds
    float y_offset = static_cast<float>(index) * (NOTIFICATION_HEIGHT + SPACING);
    return {
        screen_bounds.x,
        screen_bounds.y + y_offset,
        NOTIFICATION_WIDTH,
        NOTIFICATION_HEIGHT
    };
}

} // namespace ui
} // namespace sims3000
