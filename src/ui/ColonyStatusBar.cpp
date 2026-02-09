/**
 * @file ColonyStatusBar.cpp
 * @brief Implementation of ColonyStatusBar: top-of-screen colony info display.
 */

#include "sims3000/ui/ColonyStatusBar.h"

#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace ui {

// =========================================================================
// Construction
// =========================================================================

ColonyStatusBar::ColonyStatusBar() {
    // Default bounds: full width placeholder, BAR_HEIGHT at top of screen.
    // The actual width should be set by the caller to match viewport width.
    bounds = {0.0f, 0.0f, 1280.0f, BAR_HEIGHT};
}

// =========================================================================
// Data accessors
// =========================================================================

void ColonyStatusBar::set_data(const ColonyStatusData& data) {
    m_data = data;
}

const ColonyStatusData& ColonyStatusBar::get_data() const {
    return m_data;
}

// =========================================================================
// Rendering
// =========================================================================

void ColonyStatusBar::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    // 1. Dark semi-transparent background bar
    Color bg_fill   = {0.05f, 0.05f, 0.1f, 0.85f};
    Color bg_border = {0.2f, 0.3f, 0.4f, 0.9f};
    renderer->draw_rect(screen_bounds, bg_fill, bg_border);

    // Text color: light cyan-white for the alien/holographic aesthetic
    Color text_color = {0.8f, 0.95f, 1.0f, 1.0f};

    // Vertical center for text (small offset from top)
    float text_y = screen_bounds.y + 6.0f;

    // 2. Population at left
    std::string pop_text = "Pop: " + format_population(m_data.population) + " beings";
    float pop_x = screen_bounds.x + 12.0f;
    renderer->draw_text(pop_text, pop_x, text_y, FontSize::Small, text_color);

    // 3. Treasury at center
    std::string treasury_text = "Treasury: " + format_treasury(m_data.treasury_balance);
    float treasury_x = screen_bounds.x + screen_bounds.width * 0.35f;
    renderer->draw_text(treasury_text, treasury_x, text_y, FontSize::Small, text_color);

    // 4. Date at right-center
    std::string date_text = format_date(m_data.current_cycle, m_data.current_phase);
    float date_x = screen_bounds.x + screen_bounds.width * 0.65f;
    renderer->draw_text(date_text, date_x, text_y, FontSize::Small, text_color);

    // 5. Speed indicator at far right
    std::string speed_text = format_speed(m_data.paused, m_data.speed_multiplier);
    float speed_x = screen_bounds.x + screen_bounds.width - 80.0f;
    renderer->draw_text(speed_text, speed_x, text_y, FontSize::Small, text_color);

    // Render children
    Widget::render(renderer);
}

// =========================================================================
// Formatting helpers
// =========================================================================

std::string ColonyStatusBar::format_population(int64_t pop) {
    bool negative = pop < 0;
    if (negative) {
        pop = -pop;
    }

    std::string digits = std::to_string(pop);
    std::string result;
    result.reserve(digits.size() + digits.size() / 3 + 1);

    int count = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        if (count > 0 && count % 3 == 0) {
            result += ',';
        }
        result += digits[static_cast<size_t>(i)];
        ++count;
    }

    // Reverse to get correct order
    std::reverse(result.begin(), result.end());

    if (negative) {
        result.insert(result.begin(), '-');
    }

    return result;
}

std::string ColonyStatusBar::format_treasury(int64_t balance) {
    bool negative = balance < 0;
    int64_t abs_balance = negative ? -balance : balance;

    std::string digits = std::to_string(abs_balance);
    std::string result;
    result.reserve(digits.size() + digits.size() / 3 + 4);

    int count = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        if (count > 0 && count % 3 == 0) {
            result += ',';
        }
        result += digits[static_cast<size_t>(i)];
        ++count;
    }

    std::reverse(result.begin(), result.end());

    if (negative) {
        result.insert(result.begin(), '-');
    }

    result += " cr";
    return result;
}

std::string ColonyStatusBar::format_date(uint32_t cycle, uint32_t phase) {
    return "Cycle " + std::to_string(cycle) + ", Phase " + std::to_string(phase);
}

std::string ColonyStatusBar::format_speed(bool paused, int speed) {
    if (paused) {
        return "[PAUSED]";
    }

    switch (speed) {
        case 1:  return "[>]";
        case 2:  return "[>>]";
        case 3:  return "[>>>]";
        default: return "[>]";
    }
}

} // namespace ui
} // namespace sims3000
