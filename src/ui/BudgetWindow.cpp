/**
 * @file BudgetWindow.cpp
 * @brief Implementation of the Colony Treasury Panel (budget window).
 *
 * Renders a tabbed financial overview with four views:
 * - Revenue:        Tribute income breakdown with per-source line items
 * - Expenditure:    Expense line items with totals
 * - Services:       Per-service funding levels and costs
 * - Credit Advances: Outstanding bonds with principal, interest, and phases
 */

#include "sims3000/ui/BudgetWindow.h"

#include <string>

namespace sims3000 {
namespace ui {

// =========================================================================
// Construction
// =========================================================================

BudgetWindow::BudgetWindow() {
    title = "COLONY TREASURY";
    closable = true;
    draggable = true;

    bounds = {0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT};
    visible = false;
}

// =========================================================================
// Public interface
// =========================================================================

void BudgetWindow::set_data(const BudgetData& data) {
    m_data = data;
}

void BudgetWindow::set_callbacks(const BudgetCallbacks& callbacks) {
    m_callbacks = callbacks;
}

BudgetTab BudgetWindow::get_active_tab() const {
    return m_active_tab;
}

void BudgetWindow::set_active_tab(BudgetTab tab) {
    m_active_tab = tab;
}

// =========================================================================
// Rendering
// =========================================================================

void BudgetWindow::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    // Draw the panel chrome (background, title bar, border)
    renderer->draw_panel(screen_bounds, title, closable);

    // Content area starts below the title bar
    Rect content = get_content_bounds();
    float y = content.y + 4.0f;
    float left_margin = content.x + 8.0f;

    // Balance header line
    std::string balance_text = "Balance: " + format_credits(m_data.total_balance);
    Color balance_color = (m_data.total_balance >= 0)
                              ? POSITIVE_COLOR()
                              : NEGATIVE_COLOR();
    renderer->draw_text(balance_text, left_margin, y,
                        FontSize::Large, balance_color);
    y += LINE_HEIGHT + 6.0f;

    // Separator below balance
    Rect sep_rect = {content.x + 4.0f, y, content.width - 8.0f, 1.0f};
    renderer->draw_rect(sep_rect, SEPARATOR_COLOR(), SEPARATOR_COLOR());
    y += 4.0f;

    // Tab buttons
    render_tabs(renderer, y);
    y += TAB_HEIGHT + 4.0f;

    // Separator below tabs
    Rect tab_sep = {content.x + 4.0f, y, content.width - 8.0f, 1.0f};
    renderer->draw_rect(tab_sep, SEPARATOR_COLOR(), SEPARATOR_COLOR());
    y += 6.0f;

    // Active tab content
    switch (m_active_tab) {
        case BudgetTab::Revenue:
            render_revenue_tab(renderer, y);
            break;
        case BudgetTab::Expenditure:
            render_expenditure_tab(renderer, y);
            break;
        case BudgetTab::Services:
            render_services_tab(renderer, y);
            break;
        case BudgetTab::CreditAdvances:
            render_bonds_tab(renderer, y);
            break;
    }

    // Render children (close button, etc.)
    Widget::render(renderer);
}

// =========================================================================
// Tab bar
// =========================================================================

void BudgetWindow::render_tabs(UIRenderer* renderer, float y) {
    Rect content = get_content_bounds();
    float left = content.x + 4.0f;
    float available_width = content.width - 8.0f;

    // Tab labels using alien terminology
    const char* tab_labels[] = {
        "Revenue",
        "Expenditure",
        "Services",
        "Credit Advances"
    };
    constexpr int TAB_COUNT = 4;

    float tab_width = available_width / static_cast<float>(TAB_COUNT);

    for (int i = 0; i < TAB_COUNT; ++i) {
        Rect tab_rect = {
            left + static_cast<float>(i) * tab_width,
            y,
            tab_width,
            TAB_HEIGHT
        };

        BudgetTab this_tab = static_cast<BudgetTab>(i);
        bool is_active = (this_tab == m_active_tab);

        Color fill = is_active ? ACTIVE_TAB_COLOR() : INACTIVE_TAB_COLOR();
        renderer->draw_rect(tab_rect, fill, TAB_BORDER_COLOR());

        // Center the label text vertically and horizontally within the tab
        float text_x = tab_rect.x + 6.0f;
        float text_y = tab_rect.y + (TAB_HEIGHT - 14.0f) * 0.5f;

        Color text_color = is_active ? TEXT_COLOR() : DIM_TEXT_COLOR();
        renderer->draw_text(tab_labels[i], text_x, text_y,
                            FontSize::Small, text_color);
    }
}

// =========================================================================
// Revenue tab
// =========================================================================

void BudgetWindow::render_revenue_tab(UIRenderer* renderer, float y) {
    Rect content = get_content_bounds();
    float left_margin = content.x + 12.0f;
    float amount_x = content.x + content.width - 140.0f;

    // Section header
    renderer->draw_text("TRIBUTE INCOME", left_margin, y,
                        FontSize::Normal, HEADER_COLOR());
    y += LINE_HEIGHT + 2.0f;

    // Revenue line items
    for (const auto& item : m_data.revenue_items) {
        renderer->draw_text(item.label, left_margin + 8.0f, y,
                            FontSize::Small, TEXT_COLOR());

        std::string amount_text = format_credits(item.amount);
        Color amount_color = (item.amount >= 0)
                                 ? POSITIVE_COLOR()
                                 : NEGATIVE_COLOR();
        renderer->draw_text(amount_text, amount_x, y,
                            FontSize::Small, amount_color);
        y += LINE_HEIGHT;
    }

    if (m_data.revenue_items.empty()) {
        renderer->draw_text("No tribute sources", left_margin + 8.0f, y,
                            FontSize::Small, DIM_TEXT_COLOR());
        y += LINE_HEIGHT;
    }

    // Separator
    y += 4.0f;
    Rect sep = {content.x + 8.0f, y, content.width - 16.0f, 1.0f};
    renderer->draw_rect(sep, SEPARATOR_COLOR(), SEPARATOR_COLOR());
    y += 6.0f;

    // Total revenue
    renderer->draw_text("Total Revenue:", left_margin, y,
                        FontSize::Normal, HEADER_COLOR());
    std::string total_text = format_credits(m_data.total_revenue);
    Color total_color = (m_data.total_revenue >= 0)
                            ? POSITIVE_COLOR()
                            : NEGATIVE_COLOR();
    renderer->draw_text(total_text, amount_x, y,
                        FontSize::Normal, total_color);
}

// =========================================================================
// Expenditure tab
// =========================================================================

void BudgetWindow::render_expenditure_tab(UIRenderer* renderer, float y) {
    Rect content = get_content_bounds();
    float left_margin = content.x + 12.0f;
    float amount_x = content.x + content.width - 140.0f;

    // Section header
    renderer->draw_text("COLONY EXPENDITURES", left_margin, y,
                        FontSize::Normal, HEADER_COLOR());
    y += LINE_HEIGHT + 2.0f;

    // Expense line items
    for (const auto& item : m_data.expense_items) {
        renderer->draw_text(item.label, left_margin + 8.0f, y,
                            FontSize::Small, TEXT_COLOR());

        std::string amount_text = format_credits(item.amount);
        renderer->draw_text(amount_text, amount_x, y,
                            FontSize::Small, NEGATIVE_COLOR());
        y += LINE_HEIGHT;
    }

    if (m_data.expense_items.empty()) {
        renderer->draw_text("No expenditures", left_margin + 8.0f, y,
                            FontSize::Small, DIM_TEXT_COLOR());
        y += LINE_HEIGHT;
    }

    // Separator
    y += 4.0f;
    Rect sep = {content.x + 8.0f, y, content.width - 16.0f, 1.0f};
    renderer->draw_rect(sep, SEPARATOR_COLOR(), SEPARATOR_COLOR());
    y += 6.0f;

    // Total expenses
    renderer->draw_text("Total Expenses:", left_margin, y,
                        FontSize::Normal, HEADER_COLOR());
    std::string total_text = format_credits(m_data.total_expenses);
    renderer->draw_text(total_text, amount_x, y,
                        FontSize::Normal, NEGATIVE_COLOR());
    y += LINE_HEIGHT + 2.0f;

    // Net income line
    int64_t net = m_data.total_revenue - m_data.total_expenses;
    renderer->draw_text("Net Income:", left_margin, y,
                        FontSize::Normal, HEADER_COLOR());
    std::string net_text = format_credits(net);
    Color net_color = (net >= 0) ? POSITIVE_COLOR() : NEGATIVE_COLOR();
    renderer->draw_text(net_text, amount_x, y,
                        FontSize::Normal, net_color);
}

// =========================================================================
// Services tab
// =========================================================================

void BudgetWindow::render_services_tab(UIRenderer* renderer, float y) {
    Rect content = get_content_bounds();
    float left_margin = content.x + 12.0f;
    float level_x = content.x + content.width - 200.0f;
    float cost_x = content.x + content.width - 100.0f;

    // Column headers
    renderer->draw_text("SERVICE", left_margin, y,
                        FontSize::Small, HEADER_COLOR());
    renderer->draw_text("FUNDING", level_x, y,
                        FontSize::Small, HEADER_COLOR());
    renderer->draw_text("COST", cost_x, y,
                        FontSize::Small, HEADER_COLOR());
    y += LINE_HEIGHT + 2.0f;

    // Separator under headers
    Rect header_sep = {content.x + 8.0f, y, content.width - 16.0f, 1.0f};
    renderer->draw_rect(header_sep, SEPARATOR_COLOR(), SEPARATOR_COLOR());
    y += 4.0f;

    // Service entries
    for (const auto& entry : m_data.service_entries) {
        renderer->draw_text(entry.service_name, left_margin + 4.0f, y,
                            FontSize::Small, TEXT_COLOR());

        // Funding level percentage with color coding
        std::string level_text = std::to_string(static_cast<int>(entry.funding_level)) + "%";
        Color level_color = TEXT_COLOR();
        if (entry.funding_level < 50) {
            level_color = NEGATIVE_COLOR();
        } else if (entry.funding_level < 80) {
            level_color = WARNING_COLOR();
        }
        renderer->draw_text(level_text, level_x, y,
                            FontSize::Small, level_color);

        // Cost at current funding level
        std::string cost_text = format_credits(entry.cost_at_level);
        renderer->draw_text(cost_text, cost_x, y,
                            FontSize::Small, NEGATIVE_COLOR());
        y += LINE_HEIGHT;
    }

    if (m_data.service_entries.empty()) {
        renderer->draw_text("No active services", left_margin + 4.0f, y,
                            FontSize::Small, DIM_TEXT_COLOR());
        y += LINE_HEIGHT;
    }

    // Total service costs
    y += 4.0f;
    Rect sep = {content.x + 8.0f, y, content.width - 16.0f, 1.0f};
    renderer->draw_rect(sep, SEPARATOR_COLOR(), SEPARATOR_COLOR());
    y += 6.0f;

    int64_t total_service_cost = 0;
    for (const auto& entry : m_data.service_entries) {
        total_service_cost += entry.cost_at_level;
    }

    renderer->draw_text("Total Service Cost:", left_margin, y,
                        FontSize::Normal, HEADER_COLOR());
    std::string total_text = format_credits(total_service_cost);
    renderer->draw_text(total_text, cost_x, y,
                        FontSize::Normal, NEGATIVE_COLOR());
}

// =========================================================================
// Credit Advances (bonds) tab
// =========================================================================

void BudgetWindow::render_bonds_tab(UIRenderer* renderer, float y) {
    Rect content = get_content_bounds();
    float left_margin = content.x + 12.0f;
    float amount_x = content.x + content.width - 140.0f;

    // Section header
    renderer->draw_text("CREDIT ADVANCES", left_margin, y,
                        FontSize::Normal, HEADER_COLOR());
    y += LINE_HEIGHT + 2.0f;

    // Bond entries
    if (m_data.bonds.empty()) {
        renderer->draw_text("No outstanding credit advances", left_margin + 4.0f, y,
                            FontSize::Small, DIM_TEXT_COLOR());
        y += LINE_HEIGHT;
    } else {
        for (size_t i = 0; i < m_data.bonds.size(); ++i) {
            const auto& bond = m_data.bonds[i];

            // Bond label (with emergency marker)
            std::string bond_label = "Advance #" + std::to_string(i + 1);
            if (bond.is_emergency) {
                bond_label += " [EMERGENCY]";
            }
            Color label_color = bond.is_emergency
                                    ? WARNING_COLOR()
                                    : TEXT_COLOR();
            renderer->draw_text(bond_label, left_margin + 4.0f, y,
                                FontSize::Small, label_color);
            y += LINE_HEIGHT;

            // Principal
            std::string principal_text = "  Principal: " + format_credits(bond.principal);
            renderer->draw_text(principal_text, left_margin + 8.0f, y,
                                FontSize::Small, DIM_TEXT_COLOR());
            y += LINE_HEIGHT;

            // Remaining balance
            std::string remaining_text = "  Remaining: " + format_credits(bond.remaining);
            renderer->draw_text(remaining_text, left_margin + 8.0f, y,
                                FontSize::Small, NEGATIVE_COLOR());
            y += LINE_HEIGHT;

            // Interest rate (convert basis points to percentage)
            float interest_pct = static_cast<float>(bond.interest_bps) / 100.0f;
            // Format to one decimal place
            int int_part = static_cast<int>(interest_pct);
            int frac_part = static_cast<int>((interest_pct - static_cast<float>(int_part)) * 10.0f + 0.5f);
            std::string interest_text = "  Interest: "
                                        + std::to_string(int_part) + "."
                                        + std::to_string(frac_part) + "%";
            renderer->draw_text(interest_text, left_margin + 8.0f, y,
                                FontSize::Small, DIM_TEXT_COLOR());
            y += LINE_HEIGHT;

            // Phases remaining
            std::string phases_text = "  Phases left: "
                                      + std::to_string(static_cast<int>(bond.phases_remaining));
            renderer->draw_text(phases_text, left_margin + 8.0f, y,
                                FontSize::Small, DIM_TEXT_COLOR());
            y += LINE_HEIGHT + 2.0f;
        }
    }

    // Separator
    y += 4.0f;
    Rect sep = {content.x + 8.0f, y, content.width - 16.0f, 1.0f};
    renderer->draw_rect(sep, SEPARATOR_COLOR(), SEPARATOR_COLOR());
    y += 6.0f;

    // Total debt
    renderer->draw_text("Total Debt:", left_margin, y,
                        FontSize::Normal, HEADER_COLOR());
    std::string debt_text = format_credits(m_data.total_debt);
    Color debt_color = (m_data.total_debt > 0)
                           ? NEGATIVE_COLOR()
                           : POSITIVE_COLOR();
    renderer->draw_text(debt_text, amount_x, y,
                        FontSize::Normal, debt_color);
    y += LINE_HEIGHT + 8.0f;

    // Issue Credit Advance button
    if (m_data.can_issue_bond
        && static_cast<int>(m_data.bonds.size()) < m_data.max_bonds) {
        Rect button_rect = {
            content.x + content.width * 0.5f - 80.0f,
            y,
            160.0f,
            28.0f
        };
        renderer->draw_button(button_rect, "Issue Credit Advance",
                              ButtonState::Normal);
    } else {
        // Show disabled state or limit reached message
        std::string limit_text = "Advance limit reached ("
                                 + std::to_string(m_data.bonds.size())
                                 + "/" + std::to_string(m_data.max_bonds) + ")";
        renderer->draw_text(limit_text, left_margin, y,
                            FontSize::Small, DIM_TEXT_COLOR());
    }
}

// =========================================================================
// Utility: format credits with thousands separators
// =========================================================================

std::string BudgetWindow::format_credits(int64_t amount) {
    bool negative = (amount < 0);
    // Use unsigned to handle INT64_MIN safely
    uint64_t abs_val;
    if (negative) {
        // Avoid undefined behavior with INT64_MIN
        abs_val = static_cast<uint64_t>(-(amount + 1)) + 1u;
    } else {
        abs_val = static_cast<uint64_t>(amount);
    }

    // Convert to string without separators first
    std::string digits = std::to_string(abs_val);

    // Insert thousands separators (commas)
    std::string result;
    int count = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        if (count > 0 && count % 3 == 0) {
            result = ',' + result;
        }
        result = digits[static_cast<size_t>(i)] + result;
        ++count;
    }

    if (negative) {
        result = '-' + result;
    }

    result += " cr";
    return result;
}

} // namespace ui
} // namespace sims3000
