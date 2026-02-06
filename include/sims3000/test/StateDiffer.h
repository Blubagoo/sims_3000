/**
 * @file StateDiffer.h
 * @brief ECS state comparison for testing.
 *
 * Compares two ECS registries and returns a list of differences.
 * Used by TestHarness for assert_state_match() verification.
 *
 * Compares:
 * - Entity existence (created/destroyed)
 * - Component presence (added/removed)
 * - Component values (modified)
 *
 * Usage:
 *   StateDiffer differ;
 *   auto diffs = differ.compare(serverRegistry, clientRegistry);
 *   if (!diffs.empty()) {
 *       for (const auto& diff : diffs) {
 *           std::cerr << diff.toString() << std::endl;
 *       }
 *   }
 *
 * Ownership: StateDiffer is stateless, diffs are value types.
 */

#ifndef SIMS3000_TEST_STATEDIFFER_H
#define SIMS3000_TEST_STATEDIFFER_H

#include "sims3000/ecs/Registry.h"
#include "sims3000/core/types.h"
#include <vector>
#include <string>
#include <functional>
#include <cstdint>

namespace sims3000 {

/**
 * @enum DifferenceType
 * @brief Type of difference between ECS states.
 */
enum class DifferenceType : std::uint8_t {
    EntityMissing,          // Entity exists in expected but not actual
    EntityExtra,            // Entity exists in actual but not expected
    ComponentMissing,       // Component exists in expected but not actual
    ComponentExtra,         // Component exists in actual but not expected
    ComponentValueDiffers   // Component values don't match
};

/**
 * @struct StateDifference
 * @brief Describes a single difference between ECS states.
 */
struct StateDifference {
    DifferenceType type;
    EntityID entityId = 0;
    std::string componentName;
    std::string expectedValue;
    std::string actualValue;

    /**
     * @brief Convert difference to human-readable string.
     */
    std::string toString() const;
};

/**
 * @struct DiffOptions
 * @brief Options for state comparison.
 */
struct DiffOptions {
    /// Tolerance for floating-point comparisons
    float floatTolerance = 0.001f;

    /// Whether to compare entity IDs exactly (false allows remapping)
    bool strictEntityIds = true;

    /// Maximum differences to report (0 = unlimited)
    std::size_t maxDifferences = 100;

    /// Component types to ignore during comparison
    std::vector<std::string> ignoreComponents;

    /// Whether to check PositionComponent
    bool checkPosition = true;

    /// Whether to check OwnershipComponent
    bool checkOwnership = true;

    /// Whether to check BuildingComponent
    bool checkBuilding = true;

    /// Whether to check TransformComponent
    bool checkTransform = true;

    /// Default options
    static DiffOptions defaults() { return DiffOptions{}; }
};

/**
 * @class StateDiffer
 * @brief Compares ECS registry states and reports differences.
 *
 * Example usage:
 * @code
 *   StateDiffer differ;
 *   DiffOptions opts;
 *   opts.floatTolerance = 0.01f;
 *
 *   auto diffs = differ.compare(expected, actual, opts);
 *   ASSERT_TRUE(diffs.empty()) << "States don't match: " << diffs[0].toString();
 * @endcode
 */
class StateDiffer {
public:
    StateDiffer() = default;

    /**
     * @brief Compare two registries and return differences.
     * @param expected The expected/reference state (e.g., server).
     * @param actual The actual state to verify (e.g., client).
     * @param options Comparison options.
     * @return List of differences found.
     */
    std::vector<StateDifference> compare(
        const Registry& expected,
        const Registry& actual,
        const DiffOptions& options = DiffOptions::defaults()) const;

    /**
     * @brief Compare specific component type on two registries.
     * @tparam T Component type to compare.
     * @param expected The expected registry.
     * @param actual The actual registry.
     * @param options Comparison options.
     * @return List of differences for this component type.
     */
    template<typename T>
    std::vector<StateDifference> compareComponent(
        const Registry& expected,
        const Registry& actual,
        const DiffOptions& options = DiffOptions::defaults()) const;

    /**
     * @brief Check if two states match exactly (no differences).
     * @param expected The expected state.
     * @param actual The actual state.
     * @param options Comparison options.
     * @return true if states match.
     */
    bool statesMatch(
        const Registry& expected,
        const Registry& actual,
        const DiffOptions& options = DiffOptions::defaults()) const;

    /**
     * @brief Get entity count difference.
     * @param expected The expected registry.
     * @param actual The actual registry.
     * @return (expected count, actual count)
     */
    std::pair<std::size_t, std::size_t> getEntityCounts(
        const Registry& expected,
        const Registry& actual) const;

private:
    /// Compare PositionComponent values
    std::vector<StateDifference> comparePositions(
        const Registry& expected,
        const Registry& actual,
        const DiffOptions& options) const;

    /// Compare OwnershipComponent values
    std::vector<StateDifference> compareOwnerships(
        const Registry& expected,
        const Registry& actual,
        const DiffOptions& options) const;

    /// Compare BuildingComponent values
    std::vector<StateDifference> compareBuildings(
        const Registry& expected,
        const Registry& actual,
        const DiffOptions& options) const;

    /// Compare TransformComponent values
    std::vector<StateDifference> compareTransforms(
        const Registry& expected,
        const Registry& actual,
        const DiffOptions& options) const;

    /// Helper to check if floats are equal within tolerance
    bool floatEqual(float a, float b, float tolerance) const;
};

/**
 * @brief Create a summary string from a list of differences.
 * @param diffs Differences to summarize.
 * @param maxToShow Maximum number of differences to include (0 = all).
 * @return Human-readable summary.
 */
std::string summarizeDifferences(
    const std::vector<StateDifference>& diffs,
    std::size_t maxToShow = 5);

} // namespace sims3000

#endif // SIMS3000_TEST_STATEDIFFER_H
