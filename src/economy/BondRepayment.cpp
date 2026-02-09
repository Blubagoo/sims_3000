/**
 * @file BondRepayment.cpp
 * @brief Implementation of detailed bond repayment processing (Ticket E11-017)
 */

#include "sims3000/economy/BondRepayment.h"
#include <algorithm>

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

struct SinglePayment {
    int64_t principal_payment;
    int64_t interest_payment;
    int64_t total;
    bool will_mature;
};

SinglePayment calc_single_payment(const CreditAdvance& bond) {
    SinglePayment pay;

    if (bond.term_phases == 0) {
        pay.principal_payment = 0;
        pay.interest_payment = 0;
        pay.total = 0;
        pay.will_mature = true;
        return pay;
    }

    pay.principal_payment = bond.principal / bond.term_phases;
    pay.interest_payment = (bond.remaining_principal
                           * static_cast<int64_t>(bond.interest_rate_basis_points))
                          / (10000LL * 12LL);
    pay.total = pay.principal_payment + pay.interest_payment;
    pay.will_mature = (bond.phases_remaining <= 1);

    return pay;
}

/**
 * @brief Estimate total interest paid over the life of a bond.
 *
 * For a matured bond, we calculate what was paid in total by summing
 * the interest from each phase. Since we only have the current state,
 * we compute: total_interest = sum over phases of
 *   (remaining_at_phase * rate) / (10000 * 12)
 * where remaining_at_phase decreases by principal_payment each phase.
 *
 * principal_payment = principal / term_phases
 * At phase i (0-indexed), remaining = principal - i * principal_payment
 * interest_i = (remaining * rate) / (10000 * 12)
 */
int64_t estimate_total_interest(const CreditAdvance& bond) {
    if (bond.term_phases == 0) return 0;

    int64_t pp = bond.principal / bond.term_phases;
    int64_t total_interest = 0;

    for (uint16_t i = 0; i < bond.term_phases; ++i) {
        int64_t remaining = bond.principal - static_cast<int64_t>(i) * pp;
        if (remaining < 0) remaining = 0;
        int64_t interest = (remaining * static_cast<int64_t>(bond.interest_rate_basis_points))
                          / (10000LL * 12LL);
        total_interest += interest;
    }

    return total_interest;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// calculate_detailed_bond_payments (pure, const)
// ---------------------------------------------------------------------------

DetailedBondPaymentResult calculate_detailed_bond_payments(
    const std::vector<CreditAdvance>& bonds, uint8_t player_id) {

    DetailedBondPaymentResult result;
    result.total_principal_paid = 0;
    result.total_interest_paid = 0;
    result.total_payment = 0;

    for (size_t i = 0; i < bonds.size(); ++i) {
        auto pay = calc_single_payment(bonds[i]);

        DetailedBondPayment detail;
        detail.bond_index = i;
        detail.principal_payment = pay.principal_payment;
        detail.interest_payment = pay.interest_payment;
        detail.total_payment = pay.total;
        detail.is_final_payment = pay.will_mature;

        result.payments.push_back(detail);

        result.total_principal_paid += pay.principal_payment;
        result.total_interest_paid += pay.interest_payment;
        result.total_payment += pay.total;

        if (pay.will_mature) {
            BondPaidOffEvent evt;
            evt.player_id = player_id;
            evt.principal = bonds[i].principal;
            evt.total_interest_paid = estimate_total_interest(bonds[i]);
            evt.was_emergency = bonds[i].is_emergency;
            result.matured_events.push_back(evt);
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// process_detailed_bond_payments (mutating)
// ---------------------------------------------------------------------------

DetailedBondPaymentResult process_detailed_bond_payments(
    std::vector<CreditAdvance>& bonds, uint8_t player_id) {

    // First calculate (captures indices before removal)
    DetailedBondPaymentResult result;
    result.total_principal_paid = 0;
    result.total_interest_paid = 0;
    result.total_payment = 0;

    for (size_t i = 0; i < bonds.size(); ++i) {
        auto pay = calc_single_payment(bonds[i]);

        DetailedBondPayment detail;
        detail.bond_index = i;
        detail.principal_payment = pay.principal_payment;
        detail.interest_payment = pay.interest_payment;
        detail.total_payment = pay.total;
        detail.is_final_payment = pay.will_mature;

        result.payments.push_back(detail);

        result.total_principal_paid += pay.principal_payment;
        result.total_interest_paid += pay.interest_payment;
        result.total_payment += pay.total;

        if (pay.will_mature) {
            BondPaidOffEvent evt;
            evt.player_id = player_id;
            evt.principal = bonds[i].principal;
            evt.total_interest_paid = estimate_total_interest(bonds[i]);
            evt.was_emergency = bonds[i].is_emergency;
            result.matured_events.push_back(evt);
        }
    }

    // Now mutate: deduct principal and decrement phases
    for (auto& bond : bonds) {
        if (bond.term_phases == 0) continue;

        int64_t pp = bond.principal / bond.term_phases;
        bond.remaining_principal -= pp;
        if (bond.phases_remaining > 0) {
            --bond.phases_remaining;
        }
    }

    // Remove matured bonds (phases_remaining == 0)
    auto new_end = std::remove_if(bonds.begin(), bonds.end(),
        [](const CreditAdvance& b) { return b.phases_remaining == 0; });
    bonds.erase(new_end, bonds.end());

    return result;
}

// ---------------------------------------------------------------------------
// get_total_debt
// ---------------------------------------------------------------------------

int64_t get_total_debt(const std::vector<CreditAdvance>& bonds) {
    int64_t total = 0;
    for (const auto& bond : bonds) {
        total += bond.remaining_principal;
    }
    return total;
}

} // namespace economy
} // namespace sims3000
