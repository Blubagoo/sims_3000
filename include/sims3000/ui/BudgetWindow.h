/**
 * @file BudgetWindow.h
 * @brief Colony Treasury Panel (budget window) with tabbed display.
 *
 * Provides a tabbed modal window showing colony financial information:
 * revenue, expenditures, services funding levels, and credit advances.
 * Uses alien terminology throughout (tributes, credit advances, etc.).
 *
 * Usage:
 * @code
 *   auto budget = std::make_unique<BudgetWindow>();
 *
 *   BudgetData data;
 *   data.total_balance = 125000;
 *   data.total_revenue = 8500;
 *   data.revenue_items.push_back({"Habitation Tribute", 5000});
 *   data.revenue_items.push_back({"Exchange Tribute", 3500});
 *   budget->set_data(data);
 *
 *   BudgetCallbacks callbacks;
 *   callbacks.on_issue_bond = []{ issue_credit_advance(); };
 *   budget->set_callbacks(callbacks);
 *
 *   budget->visible = true;
 *   root->add_child(std::move(budget));
 * @endcode
 */

#ifndef SIMS3000_UI_BUDGETWINDOW_H
#define SIMS3000_UI_BUDGETWINDOW_H

#include "sims3000/ui/CoreWidgets.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace sims3000 {
namespace ui {

/**
 * @enum BudgetTab
 * @brief Tabs available in the Colony Treasury panel.
 */
enum class BudgetTab : uint8_t {
    Revenue        = 0,  ///< Tribute income breakdown
    Expenditure    = 1,  ///< Expense line items
    Services       = 2,  ///< Service funding levels
    CreditAdvances = 3   ///< Outstanding credit advances (bonds)
};

/**
 * @struct RevenueLineItem
 * @brief A single revenue/tribute source entry.
 */
struct RevenueLineItem {
    std::string label;       ///< Display name (e.g. "Habitation Tribute")
    int64_t amount = 0;      ///< Revenue amount in credits
};

/**
 * @struct ExpenseLineItem
 * @brief A single expenditure entry.
 */
struct ExpenseLineItem {
    std::string label;       ///< Display name (e.g. "Pathway Maintenance")
    int64_t amount = 0;      ///< Expense amount in credits
};

/**
 * @struct ServiceFundingEntry
 * @brief Funding level for a colony service.
 */
struct ServiceFundingEntry {
    std::string service_name;       ///< Service display name
    uint8_t funding_level = 100;    ///< Funding percentage (0-150%)
    int64_t cost_at_level = 0;      ///< Cost at current funding level
};

/**
 * @struct BondEntry
 * @brief A credit advance (bond) record.
 */
struct BondEntry {
    int64_t principal = 0;          ///< Original amount borrowed
    int64_t remaining = 0;          ///< Outstanding balance
    uint16_t interest_bps = 0;      ///< Interest rate in basis points (100 = 1%)
    uint16_t phases_remaining = 0;  ///< Number of phases until repaid
    bool is_emergency = false;      ///< Whether this was an emergency advance
};

/**
 * @struct BudgetData
 * @brief Complete colony financial data for the budget window display.
 *
 * Populated by the simulation and passed to BudgetWindow::set_data().
 */
struct BudgetData {
    /// Current colony treasury balance
    int64_t total_balance = 0;

    // -- Revenue tab ---------------------------------------------------------

    /// Individual tribute/revenue line items
    std::vector<RevenueLineItem> revenue_items;

    /// Sum of all revenue items
    int64_t total_revenue = 0;

    // -- Expenditure tab -----------------------------------------------------

    /// Individual expense line items
    std::vector<ExpenseLineItem> expense_items;

    /// Sum of all expense items
    int64_t total_expenses = 0;

    // -- Services tab --------------------------------------------------------

    /// Per-service funding entries
    std::vector<ServiceFundingEntry> service_entries;

    // -- Credit Advances tab -------------------------------------------------

    /// Outstanding credit advance records
    std::vector<BondEntry> bonds;

    /// Total outstanding debt across all credit advances
    int64_t total_debt = 0;

    /// Whether the colony is eligible to issue a new credit advance
    bool can_issue_bond = true;

    /// Maximum number of concurrent credit advances allowed
    int max_bonds = 5;
};

/**
 * @struct BudgetCallbacks
 * @brief Callbacks for user-initiated budget changes.
 *
 * Set via BudgetWindow::set_callbacks() to connect the UI to game logic.
 */
struct BudgetCallbacks {
    /// Called when a tribute rate slider is adjusted
    /// @param zone_type Zone type identifier
    /// @param new_rate  New tribute rate (0.0 - 1.0)
    std::function<void(uint8_t zone_type, float new_rate)> on_tribute_rate_changed;

    /// Called when a service funding level is adjusted
    /// @param service_type Service type identifier
    /// @param new_level    New funding percentage (0-150)
    std::function<void(uint8_t service_type, uint8_t new_level)> on_funding_changed;

    /// Called when the player requests a new credit advance
    std::function<void()> on_issue_bond;
};

/**
 * @class BudgetWindow
 * @brief Colony Treasury panel with tabbed revenue/expense/service/bond views.
 *
 * A modal panel that displays the colony's financial state across four tabs:
 * Revenue, Expenditure, Services, and Credit Advances.  The window renders
 * tab buttons along the top of the content area, with the active tab's
 * content displayed below.
 *
 * The panel is hidden by default; set visible = true to show it.
 * Bounds are initialized centered at WINDOW_WIDTH x WINDOW_HEIGHT.
 */
class BudgetWindow : public PanelWidget {
public:
    BudgetWindow();

    /**
     * Set the budget data to display.
     * @param data Complete financial data snapshot from the simulation
     */
    void set_data(const BudgetData& data);

    /**
     * Set callbacks for user-initiated budget changes.
     * @param callbacks Struct of callback functions
     */
    void set_callbacks(const BudgetCallbacks& callbacks);

    /**
     * Get the currently active tab.
     * @return The active BudgetTab enum value
     */
    BudgetTab get_active_tab() const;

    /**
     * Set the active tab.
     * @param tab The tab to switch to
     */
    void set_active_tab(BudgetTab tab);

    void render(UIRenderer* renderer) override;

    // -- Layout constants ----------------------------------------------------

    /// Default window width in pixels
    static constexpr float WINDOW_WIDTH = 500.0f;

    /// Default window height in pixels
    static constexpr float WINDOW_HEIGHT = 400.0f;

    /// Height of the tab button row in pixels
    static constexpr float TAB_HEIGHT = 32.0f;

    /// Height of each text line in the content area
    static constexpr float LINE_HEIGHT = 22.0f;

private:
    /// Current financial data
    BudgetData m_data;

    /// User action callbacks
    BudgetCallbacks m_callbacks;

    /// Currently selected tab
    BudgetTab m_active_tab = BudgetTab::Revenue;

    // -- Tab rendering -------------------------------------------------------

    /**
     * Render the row of tab buttons.
     * @param renderer The UI renderer
     * @param y        Y position of the tab row top edge
     */
    void render_tabs(UIRenderer* renderer, float y);

    /**
     * Render the Revenue tab content.
     * @param renderer The UI renderer
     * @param y        Y position to start rendering content
     */
    void render_revenue_tab(UIRenderer* renderer, float y);

    /**
     * Render the Expenditure tab content.
     * @param renderer The UI renderer
     * @param y        Y position to start rendering content
     */
    void render_expenditure_tab(UIRenderer* renderer, float y);

    /**
     * Render the Services tab content.
     * @param renderer The UI renderer
     * @param y        Y position to start rendering content
     */
    void render_services_tab(UIRenderer* renderer, float y);

    /**
     * Render the Credit Advances tab content.
     * @param renderer The UI renderer
     * @param y        Y position to start rendering content
     */
    void render_bonds_tab(UIRenderer* renderer, float y);

    /**
     * Format a credit amount with thousands separators and " cr" suffix.
     * @param amount The credit amount (may be negative)
     * @return Formatted string (e.g. "1,250,000 cr" or "-500 cr")
     */
    static std::string format_credits(int64_t amount);

    // -- Colors --------------------------------------------------------------

    /// Color for section headers and labels
    static constexpr Color HEADER_COLOR() { return {0.7f, 0.8f, 1.0f, 1.0f}; }

    /// Color for normal body text
    static constexpr Color TEXT_COLOR() { return {1.0f, 1.0f, 1.0f, 1.0f}; }

    /// Color for dimmed/secondary text
    static constexpr Color DIM_TEXT_COLOR() { return {0.6f, 0.6f, 0.7f, 1.0f}; }

    /// Color for positive values (revenue, surplus)
    static constexpr Color POSITIVE_COLOR() { return {0.0f, 0.8f, 0.0f, 1.0f}; }

    /// Color for negative values (expenses, debt)
    static constexpr Color NEGATIVE_COLOR() { return {0.8f, 0.2f, 0.2f, 1.0f}; }

    /// Color for active/selected tab background
    static constexpr Color ACTIVE_TAB_COLOR() { return {0.25f, 0.3f, 0.4f, 1.0f}; }

    /// Color for inactive tab background
    static constexpr Color INACTIVE_TAB_COLOR() { return {0.15f, 0.18f, 0.25f, 1.0f}; }

    /// Color for tab border
    static constexpr Color TAB_BORDER_COLOR() { return {0.4f, 0.45f, 0.55f, 1.0f}; }

    /// Color for separator lines
    static constexpr Color SEPARATOR_COLOR() { return {0.3f, 0.35f, 0.45f, 0.8f}; }

    /// Color for emergency/warning indicators
    static constexpr Color WARNING_COLOR() { return {0.8f, 0.8f, 0.0f, 1.0f}; }
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_BUDGETWINDOW_H
