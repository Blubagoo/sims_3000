/**
 * @file CreditAdvance.h
 * @brief Bond/credit advance data structures (Ticket E11-003)
 *
 * Defines CreditAdvance (individual bond instance), BondConfig
 * (bond template presets), and BondType enum for the financial system.
 */

#ifndef SIMS3000_ECONOMY_CREDITADVANCE_H
#define SIMS3000_ECONOMY_CREDITADVANCE_H

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace economy {

/**
 * @struct CreditAdvance
 * @brief Represents an active bond/credit advance taken by a player.
 *
 * Tracks principal, remaining balance, interest rate (in basis points),
 * term length, and whether it was issued as an emergency bond.
 *
 * Target size: ~24 bytes.
 */
struct CreditAdvance {
    int64_t principal = 0;                        ///< Original loan amount
    int64_t remaining_principal = 0;              ///< Remaining balance
    uint16_t interest_rate_basis_points = 0;      ///< e.g., 750 = 7.5%
    uint16_t term_phases = 0;                     ///< Total term in phases
    uint16_t phases_remaining = 0;                ///< Phases left to repay
    bool is_emergency = false;                    ///< Emergency bond flag
};

static_assert(sizeof(CreditAdvance) <= 32, "CreditAdvance should be approximately 24 bytes");
static_assert(std::is_trivially_copyable<CreditAdvance>::value, "CreditAdvance must be trivially copyable");

/**
 * @struct BondConfig
 * @brief Template for bond presets (small, standard, large, emergency).
 *
 * Used as constexpr configuration for the available bond types.
 */
struct BondConfig {
    int64_t principal;              ///< Loan amount
    uint16_t interest_rate;         ///< Interest rate in basis points
    uint16_t term_phases;           ///< Repayment term in phases
    bool is_emergency;              ///< Whether this is an emergency bond
};

/// Small bond: 5000 credits, 5.0% interest, 12-phase term
constexpr BondConfig BOND_SMALL     = {5000,   500,  12, false};
/// Standard bond: 25000 credits, 7.5% interest, 24-phase term
constexpr BondConfig BOND_STANDARD  = {25000,  750,  24, false};
/// Large bond: 100000 credits, 10.0% interest, 48-phase term
constexpr BondConfig BOND_LARGE     = {100000, 1000, 48, false};
/// Emergency bond: 25000 credits, 15.0% interest, 12-phase term
constexpr BondConfig BOND_EMERGENCY = {25000,  1500, 12, true};

/// Maximum number of active bonds per player
constexpr int MAX_BONDS_PER_PLAYER = 5;

/**
 * @enum BondType
 * @brief Enumeration of available bond types.
 */
enum class BondType : uint8_t {
    Small = 0,       ///< Small bond (5000 credits)
    Standard = 1,    ///< Standard bond (25000 credits)
    Large = 2,       ///< Large bond (100000 credits)
    Emergency = 3    ///< Emergency bond (25000 credits, high interest)
};

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_CREDITADVANCE_H
