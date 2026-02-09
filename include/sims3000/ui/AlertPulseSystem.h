/**
 * @file AlertPulseSystem.h
 * @brief Alert pulse notification system for game events.
 *
 * Displays priority-based notifications in a stacked panel at the top-right
 * of the screen.  Three priority levels (Critical, Warning, Info) have
 * distinct colors, durations, and optional audio cues.  Notifications are
 * queued FIFO with a maximum of 4 visible at once; oldest are pushed out
 * when the queue overflows.
 *
 * Features:
 * - Three priority tiers with distinct colors and durations
 * - Auto-dismiss after configurable lifetime
 * - Fade-out animation during the final 0.5 seconds
 * - Click to dismiss any notification
 * - Click to focus the camera on a location (for position-based alerts)
 * - Focus callback for camera integration
 *
 * Usage:
 * @code
 *   auto alerts = std::make_unique<AlertPulseSystem>();
 *   alerts->bounds = {980, 40, NOTIFICATION_WIDTH, 400};
 *   alerts->set_focus_callback([&cam](float x, float y) {
 *       cam.center_on(x, y);
 *   });
 *
 *   alerts->push_alert("Energy grid overloaded!", AlertPriority::Critical, 128.0f, 64.0f);
 *   alerts->push_alert("Low funds", AlertPriority::Warning);
 *
 *   // Per-frame:
 *   alerts->update(delta_time);
 *   alerts->render(renderer);
 * @endcode
 *
 * Thread safety: not thread-safe. Call from the main/render thread only.
 */

#ifndef SIMS3000_UI_ALERTPULSESYSTEM_H
#define SIMS3000_UI_ALERTPULSESYSTEM_H

#include "sims3000/ui/Widget.h"
#include "sims3000/ui/UIRenderer.h"
#include "sims3000/ui/UIManager.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <string>

namespace sims3000 {
namespace ui {

/**
 * @struct AlertNotification
 * @brief Individual notification entry with lifetime, fade, and optional position.
 *
 * Each notification carries its own timer and fade state.  When time_remaining
 * drops below FADE_DURATION the notification begins to fade out.  A dismissed
 * notification is removed on the next update tick.
 */
struct AlertNotification {
    std::string message;                             ///< Alert display text
    AlertPriority priority = AlertPriority::Info;     ///< Severity level
    float duration = 3.0f;                           ///< Original lifetime (seconds)
    float time_remaining = 3.0f;                     ///< Seconds until auto-dismiss
    float fade_alpha = 1.0f;                         ///< Current opacity (0.0 - 1.0)
    bool has_focus_position = false;                 ///< Whether this alert has a map focus target
    float focus_x = -1.0f;                           ///< Map X to focus camera on click
    float focus_y = -1.0f;                           ///< Map Y to focus camera on click
    bool has_audio = false;                          ///< Whether an audio cue should play
    bool dismissed = false;                          ///< Marked for removal
};

/**
 * @class AlertPulseSystem
 * @brief Widget that manages and renders a stack of alert notifications.
 *
 * Notifications are rendered as colored rectangles stacked vertically from
 * the top of the widget's screen_bounds.  Up to MAX_VISIBLE notifications
 * are displayed; when the queue exceeds that limit the oldest notification
 * is discarded.
 *
 * Clicking a notification dismisses it.  If the notification has a focus
 * position and a focus callback has been registered, the callback fires
 * before the notification is dismissed.
 */
class AlertPulseSystem : public Widget {
public:
    AlertPulseSystem();

    // -- Constants -----------------------------------------------------------

    /// Maximum number of notifications visible at once
    static constexpr int MAX_VISIBLE = 4;

    /// Width of each notification rectangle (pixels)
    static constexpr float NOTIFICATION_WIDTH = 300.0f;

    /// Height of each notification rectangle (pixels)
    static constexpr float NOTIFICATION_HEIGHT = 50.0f;

    /// Vertical spacing between stacked notifications (pixels)
    static constexpr float SPACING = 5.0f;

    /// Duration of the fade-out animation at end of lifetime (seconds)
    static constexpr float FADE_DURATION = 0.5f;

    // -- Lifecycle -----------------------------------------------------------

    /**
     * Push a new alert notification into the queue.
     *
     * The notification's duration is determined by its priority level:
     * - Critical: 8 seconds (has audio cue)
     * - Warning:  5 seconds (has audio cue)
     * - Info:     3 seconds (no audio)
     *
     * If the queue exceeds MAX_VISIBLE, the oldest notification is removed.
     *
     * @param message  Alert text to display
     * @param priority Severity level (Info, Warning, Critical)
     * @param focus_x  Optional map X to focus on click (-1 = no focus)
     * @param focus_y  Optional map Y to focus on click (-1 = no focus)
     */
    void push_alert(const std::string& message, AlertPriority priority,
                    float focus_x = -1.0f, float focus_y = -1.0f);

    /**
     * Update all notification timers and fade states.
     *
     * Decrements time_remaining for each notification, computes fade_alpha
     * during the final FADE_DURATION seconds, and removes expired or
     * dismissed notifications.
     *
     * @param delta_time Seconds since last frame
     */
    void update(float delta_time) override;

    /**
     * Render up to MAX_VISIBLE notifications stacked vertically.
     *
     * Each notification is drawn as a colored rectangle (priority-based
     * color) with the alert message text.  Opacity is modulated by
     * fade_alpha for the fade-out animation.
     *
     * @param renderer The UI renderer to draw with
     */
    void render(UIRenderer* renderer) override;

    // -- Mouse events --------------------------------------------------------

    /**
     * Handle mouse click on a notification.
     *
     * If the click hits a notification with a focus position and a focus
     * callback is registered, the callback is invoked.  The notification
     * is then dismissed.
     *
     * @param button Mouse button (0 = left)
     * @param x      Screen X of the click
     * @param y      Screen Y of the click
     */
    void on_mouse_down(int button, float x, float y) override;

    // -- Callbacks -----------------------------------------------------------

    /**
     * Set the callback invoked when clicking a notification with a focus
     * position.  Typically used to move the game camera.
     *
     * @param callback Function taking (focus_x, focus_y) world coordinates
     */
    void set_focus_callback(std::function<void(float x, float y)> callback);

    // -- Query ---------------------------------------------------------------

    /**
     * Get the number of currently active (non-dismissed) notifications.
     * @return Count of visible notifications
     */
    int get_active_count() const;

private:
    /// Active notification queue (front = newest)
    std::deque<AlertNotification> m_notifications;

    /// Optional callback for camera focus on click
    std::function<void(float x, float y)> m_focus_callback;

    /**
     * Get the display color for a given priority level.
     * - Critical: red   {0.8, 0.1, 0.1}
     * - Warning:  amber {0.8, 0.6, 0.0}
     * - Info:     cyan  {0.0, 0.7, 0.8}
     *
     * @param priority The alert priority level
     * @return Base color for that priority
     */
    static Color get_priority_color(AlertPriority priority);

    /**
     * Get the default duration (seconds) for a given priority level.
     * - Critical: 8.0s
     * - Warning:  5.0s
     * - Info:     3.0s
     *
     * @param priority The alert priority level
     * @return Duration in seconds
     */
    static float get_priority_duration(AlertPriority priority);

    /**
     * Determine whether a priority level has an audio cue.
     * - Critical: true
     * - Warning:  true
     * - Info:     false
     *
     * @param priority The alert priority level
     * @return true if the priority level triggers an audio cue
     */
    static bool get_priority_has_audio(AlertPriority priority);

    /**
     * Compute the screen-space rectangle for notification at a given index
     * in the visible stack (0 = top-most).
     *
     * @param index Position in the visible stack (0-based)
     * @return Screen-space rectangle for that notification slot
     */
    Rect get_notification_rect(int index) const;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_ALERTPULSESYSTEM_H
