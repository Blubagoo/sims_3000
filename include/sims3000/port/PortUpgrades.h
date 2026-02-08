/**
 * @file PortUpgrades.h
 * @brief Port infrastructure upgrade config and validation (Epic 8, Ticket E8-032)
 *
 * Allows players to invest credits in port infrastructure upgrades.
 * Each upgrade level increases the trade multiplier applied to port throughput.
 *
 * | Upgrade Level       | Cost       | Trade Multiplier | Requires Rail |
 * |---------------------|------------|------------------|---------------|
 * | Basic               | 0 (default)| 1.0x             | No            |
 * | Upgraded Terminals  | 50,000 cr  | 1.2x             | No            |
 * | Advanced Logistics  | 100,000 cr | 1.4x             | Yes           |
 * | Premium Hub         | 200,000 cr | 1.6x             | Yes (full)    |
 *
 * Header-only implementation (pure logic, no external dependencies beyond PortTypes.h).
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace port {

/**
 * @enum PortUpgradeLevel
 * @brief Infrastructure upgrade tiers for port facilities.
 *
 * Each level provides an increasing trade multiplier. Higher levels
 * require additional investment and infrastructure (rail connections).
 */
enum class PortUpgradeLevel : uint8_t {
    Basic = 0,              ///< Default level, no investment required
    UpgradedTerminals = 1,  ///< Improved terminals, 1.2x trade multiplier
    AdvancedLogistics = 2,  ///< Advanced logistics, 1.4x trade multiplier (requires rail)
    PremiumHub = 3          ///< Premium hub, 1.6x trade multiplier (requires full rail)
};

/// Total number of upgrade levels
constexpr uint8_t PORT_UPGRADE_LEVEL_COUNT = 4;

/// Maximum upgrade level
constexpr uint8_t MAX_PORT_UPGRADE_LEVEL = 3;

/**
 * @struct PortUpgradeConfig
 * @brief Configuration for a single port upgrade level.
 *
 * Contains the cost, trade multiplier, and rail requirement
 * for each upgrade tier.
 */
struct PortUpgradeConfig {
    int64_t cost;               ///< Credit cost to reach this level
    float trade_multiplier;     ///< Multiplier applied to trade throughput
    bool requires_rail;         ///< Whether rail connection is required
};

/**
 * @brief Get the configuration for a specific upgrade level.
 *
 * Returns the cost, trade multiplier, and rail requirement
 * for the given upgrade level.
 *
 * @param level The upgrade level to query.
 * @return PortUpgradeConfig with cost, multiplier, and rail requirement.
 */
inline PortUpgradeConfig get_upgrade_config(PortUpgradeLevel level) {
    switch (level) {
        case PortUpgradeLevel::Basic:
            return {0, 1.0f, false};
        case PortUpgradeLevel::UpgradedTerminals:
            return {50000, 1.2f, false};
        case PortUpgradeLevel::AdvancedLogistics:
            return {100000, 1.4f, true};
        case PortUpgradeLevel::PremiumHub:
            return {200000, 1.6f, true};
        default:
            // Invalid level, return Basic config as safe default
            return {0, 1.0f, false};
    }
}

/**
 * @brief Get the name string for an upgrade level.
 *
 * @param level The upgrade level.
 * @return Null-terminated string name, or "Unknown" for invalid levels.
 */
inline const char* upgrade_level_name(PortUpgradeLevel level) {
    switch (level) {
        case PortUpgradeLevel::Basic:              return "Basic";
        case PortUpgradeLevel::UpgradedTerminals:  return "Upgraded Terminals";
        case PortUpgradeLevel::AdvancedLogistics:   return "Advanced Logistics";
        case PortUpgradeLevel::PremiumHub:          return "Premium Hub";
        default:                                   return "Unknown";
    }
}

/**
 * @brief Check whether a port can be upgraded from current to target level.
 *
 * Validates:
 * 1. Target level is higher than current level (no downgrades).
 * 2. Target level is a valid upgrade level (0-3).
 * 3. Treasury has sufficient credits to cover the upgrade cost.
 * 4. Rail requirement is met if the target level requires it.
 *
 * @param current The port's current upgrade level.
 * @param target The desired upgrade level.
 * @param treasury Available credits in the player's treasury.
 * @param has_rail Whether the port has a rail connection.
 * @return true if the upgrade is allowed, false otherwise.
 */
inline bool can_upgrade_port(PortUpgradeLevel current, PortUpgradeLevel target,
                              int64_t treasury, bool has_rail) {
    // Cannot downgrade or stay at same level
    if (static_cast<uint8_t>(target) <= static_cast<uint8_t>(current)) {
        return false;
    }

    // Target must be valid
    if (static_cast<uint8_t>(target) > MAX_PORT_UPGRADE_LEVEL) {
        return false;
    }

    PortUpgradeConfig config = get_upgrade_config(target);

    // Check treasury
    if (treasury < config.cost) {
        return false;
    }

    // Check rail requirement
    if (config.requires_rail && !has_rail) {
        return false;
    }

    return true;
}

/**
 * @brief Get the trade multiplier for the given upgrade level.
 *
 * Convenience function that extracts just the trade multiplier
 * from the upgrade config.
 *
 * @param level The current upgrade level.
 * @return Trade multiplier (1.0 to 1.6).
 */
inline float get_trade_multiplier(PortUpgradeLevel level) {
    return get_upgrade_config(level).trade_multiplier;
}

/**
 * @brief Calculate the cost to upgrade from current to target level.
 *
 * Returns the cost of the target level (not the difference between levels).
 * Returns 0 if the upgrade is not valid (target <= current).
 *
 * @param current Current upgrade level.
 * @param target Desired upgrade level.
 * @return Cost in credits, or 0 if invalid upgrade.
 */
inline int64_t get_upgrade_cost(PortUpgradeLevel current, PortUpgradeLevel target) {
    if (static_cast<uint8_t>(target) <= static_cast<uint8_t>(current)) {
        return 0;
    }
    if (static_cast<uint8_t>(target) > MAX_PORT_UPGRADE_LEVEL) {
        return 0;
    }
    return get_upgrade_config(target).cost;
}

} // namespace port
} // namespace sims3000
