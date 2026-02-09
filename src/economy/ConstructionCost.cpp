/**
 * @file ConstructionCost.cpp
 * @brief Implementation of construction cost deduction (Ticket E11-020)
 */

#include "sims3000/economy/ConstructionCost.h"

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// check_construction_cost
// ---------------------------------------------------------------------------
ConstructionCostResult check_construction_cost(const TreasuryState& treasury, int64_t cost) {
    ConstructionCostResult result;
    result.cost = cost;
    result.can_afford = (treasury.balance >= cost);
    result.balance_after = treasury.balance - cost;
    return result;
}

// ---------------------------------------------------------------------------
// deduct_construction_cost
// ---------------------------------------------------------------------------
bool deduct_construction_cost(TreasuryState& treasury, int64_t cost) {
    if (treasury.balance < cost) {
        return false;
    }
    treasury.balance -= cost;
    return true;
}

} // namespace economy
} // namespace sims3000
