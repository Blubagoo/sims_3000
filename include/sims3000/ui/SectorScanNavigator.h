/**
 * @file SectorScanNavigator.h
 * @brief Smooth camera pan/interpolation logic for sector scan click navigation.
 *
 * Provides smooth camera panning when the user clicks on the sector scan
 * (minimap) to navigate to a world position. Uses ease-out interpolation
 * for a natural deceleration feel. Works in both Legacy and Holo UI modes.
 *
 * Designed to be wired up as the NavigateCallback on a SectorScan widget:
 *
 * Usage:
 * @code
 *   SectorScanNavigator navigator;
 *   navigator.set_camera_position(cam_x, cam_y);
 *
 *   minimap->set_navigate_callback([&navigator](float wx, float wy) {
 *       navigator.navigate_to(wx, wy);
 *   });
 *
 *   // In game loop:
 *   navigator.update(delta_time);
 *   float cx, cy;
 *   navigator.get_camera_position(cx, cy);
 * @endcode
 */

#ifndef SIMS3000_UI_SECTORSCANNAVIGATOR_H
#define SIMS3000_UI_SECTORSCANNAVIGATOR_H

namespace sims3000 {
namespace ui {

/**
 * @class SectorScanNavigator
 * @brief Smooth camera pan controller for minimap click-to-navigate.
 *
 * Stores the current camera position and a target position, interpolating
 * between them over PAN_DURATION seconds using ease-out for smooth
 * deceleration. The camera position can be read each frame and applied
 * to the actual camera system.
 */
class SectorScanNavigator {
public:
    SectorScanNavigator();

    /// Duration of the smooth pan transition in seconds.
    static constexpr float PAN_DURATION = 0.5f;

    /**
     * Begin a smooth pan to the given world position.
     * Records the current position as the pan start and resets the
     * interpolation timer.
     * @param world_x Target X in world tile coordinates
     * @param world_y Target Y in world tile coordinates
     */
    void navigate_to(float world_x, float world_y);

    /**
     * Advance the interpolation by delta_time seconds.
     * Call once per frame. When not navigating this is a no-op.
     * @param delta_time Seconds since the last frame
     */
    void update(float delta_time);

    /**
     * Get the current (interpolated) camera position.
     * @param[out] x Current camera X
     * @param[out] y Current camera Y
     */
    void get_camera_position(float& x, float& y) const;

    /**
     * Set the camera position immediately (no animation).
     * Also cancels any in-progress navigation.
     * @param x Camera X in world tile coordinates
     * @param y Camera Y in world tile coordinates
     */
    void set_camera_position(float x, float y);

    /**
     * @return true while a pan transition is in progress.
     */
    bool is_navigating() const;

private:
    /**
     * Ease-out interpolation function (quadratic).
     * @param t Normalized time in [0, 1]
     * @return Eased value in [0, 1], decelerating toward 1
     */
    static float ease_out(float t);

    float m_current_x = 0.0f;   ///< Current interpolated camera X
    float m_current_y = 0.0f;   ///< Current interpolated camera Y

    float m_start_x = 0.0f;     ///< Pan start X
    float m_start_y = 0.0f;     ///< Pan start Y

    float m_target_x = 0.0f;    ///< Pan target X
    float m_target_y = 0.0f;    ///< Pan target Y

    float m_elapsed = 0.0f;     ///< Time elapsed since pan started
    bool m_navigating = false;  ///< Whether a pan is in progress
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_SECTORSCANNAVIGATOR_H
