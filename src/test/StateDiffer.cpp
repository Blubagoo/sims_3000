/**
 * @file StateDiffer.cpp
 * @brief Implementation of StateDiffer for ECS state comparison.
 */

#include "sims3000/test/StateDiffer.h"
#include "sims3000/ecs/Components.h"
#include <glm/gtc/quaternion.hpp>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <unordered_set>

namespace sims3000 {

std::string StateDifference::toString() const {
    std::ostringstream oss;

    switch (type) {
        case DifferenceType::EntityMissing:
            oss << "Entity " << entityId << " missing from actual state";
            break;
        case DifferenceType::EntityExtra:
            oss << "Entity " << entityId << " exists in actual but not expected";
            break;
        case DifferenceType::ComponentMissing:
            oss << "Entity " << entityId << ": " << componentName
                << " missing from actual";
            break;
        case DifferenceType::ComponentExtra:
            oss << "Entity " << entityId << ": " << componentName
                << " exists in actual but not expected";
            break;
        case DifferenceType::ComponentValueDiffers:
            oss << "Entity " << entityId << ": " << componentName
                << " differs - expected: " << expectedValue
                << ", actual: " << actualValue;
            break;
    }

    return oss.str();
}

std::vector<StateDifference> StateDiffer::compare(
    const Registry& expected,
    const Registry& actual,
    const DiffOptions& options) const {

    std::vector<StateDifference> diffs;

    // Helper to check max differences
    auto checkMax = [&]() {
        return options.maxDifferences == 0 ||
               diffs.size() < options.maxDifferences;
    };

    // Compare entity existence
    auto& expectedRaw = const_cast<Registry&>(expected).raw();
    auto& actualRaw = const_cast<Registry&>(actual).raw();

    // Get all entity IDs from both registries
    std::unordered_set<EntityID> expectedEntities;
    std::unordered_set<EntityID> actualEntities;

    for (auto e : expectedRaw.storage<entt::entity>()) {
        expectedEntities.insert(static_cast<EntityID>(e));
    }

    for (auto e : actualRaw.storage<entt::entity>()) {
        actualEntities.insert(static_cast<EntityID>(e));
    }

    // Check for missing entities
    for (EntityID eid : expectedEntities) {
        if (actualEntities.find(eid) == actualEntities.end()) {
            if (!checkMax()) break;
            StateDifference diff;
            diff.type = DifferenceType::EntityMissing;
            diff.entityId = eid;
            diffs.push_back(diff);
        }
    }

    // Check for extra entities
    for (EntityID eid : actualEntities) {
        if (expectedEntities.find(eid) == expectedEntities.end()) {
            if (!checkMax()) break;
            StateDifference diff;
            diff.type = DifferenceType::EntityExtra;
            diff.entityId = eid;
            diffs.push_back(diff);
        }
    }

    // Compare components for entities that exist in both
    if (options.checkPosition && checkMax()) {
        auto posDiffs = comparePositions(expected, actual, options);
        for (auto& d : posDiffs) {
            if (!checkMax()) break;
            diffs.push_back(std::move(d));
        }
    }

    if (options.checkOwnership && checkMax()) {
        auto ownDiffs = compareOwnerships(expected, actual, options);
        for (auto& d : ownDiffs) {
            if (!checkMax()) break;
            diffs.push_back(std::move(d));
        }
    }

    if (options.checkBuilding && checkMax()) {
        auto bldDiffs = compareBuildings(expected, actual, options);
        for (auto& d : bldDiffs) {
            if (!checkMax()) break;
            diffs.push_back(std::move(d));
        }
    }

    if (options.checkTransform && checkMax()) {
        auto trnDiffs = compareTransforms(expected, actual, options);
        for (auto& d : trnDiffs) {
            if (!checkMax()) break;
            diffs.push_back(std::move(d));
        }
    }

    return diffs;
}

bool StateDiffer::statesMatch(
    const Registry& expected,
    const Registry& actual,
    const DiffOptions& options) const {

    return compare(expected, actual, options).empty();
}

std::pair<std::size_t, std::size_t> StateDiffer::getEntityCounts(
    const Registry& expected,
    const Registry& actual) const {

    return {expected.size(), actual.size()};
}

std::vector<StateDifference> StateDiffer::comparePositions(
    const Registry& expected,
    const Registry& actual,
    const DiffOptions& options) const {

    std::vector<StateDifference> diffs;
    auto& expectedRaw = const_cast<Registry&>(expected).raw();
    auto& actualRaw = const_cast<Registry&>(actual).raw();

    // Check positions in expected
    auto expectedView = expectedRaw.view<PositionComponent>();
    for (auto entity : expectedView) {
        EntityID eid = static_cast<EntityID>(entity);

        if (!actualRaw.valid(entity)) continue; // Entity comparison handled separately

        const auto& expPos = expectedView.get<PositionComponent>(entity);

        // Check if actual has the component
        const auto* actPos = actualRaw.try_get<PositionComponent>(entity);
        if (!actPos) {
            StateDifference diff;
            diff.type = DifferenceType::ComponentMissing;
            diff.entityId = eid;
            diff.componentName = "PositionComponent";
            diffs.push_back(diff);
            continue;
        }

        // Compare values
        if (expPos.pos.x != actPos->pos.x ||
            expPos.pos.y != actPos->pos.y ||
            expPos.elevation != actPos->elevation) {

            StateDifference diff;
            diff.type = DifferenceType::ComponentValueDiffers;
            diff.entityId = eid;
            diff.componentName = "PositionComponent";

            std::ostringstream exp, act;
            exp << "(" << expPos.pos.x << "," << expPos.pos.y
                << ",e=" << expPos.elevation << ")";
            act << "(" << actPos->pos.x << "," << actPos->pos.y
                << ",e=" << actPos->elevation << ")";
            diff.expectedValue = exp.str();
            diff.actualValue = act.str();
            diffs.push_back(diff);
        }
    }

    // Check for extra positions in actual
    auto actualView = actualRaw.view<PositionComponent>();
    for (auto entity : actualView) {
        if (!expectedRaw.valid(entity)) continue;
        if (!expectedRaw.all_of<PositionComponent>(entity)) {
            StateDifference diff;
            diff.type = DifferenceType::ComponentExtra;
            diff.entityId = static_cast<EntityID>(entity);
            diff.componentName = "PositionComponent";
            diffs.push_back(diff);
        }
    }

    return diffs;
}

std::vector<StateDifference> StateDiffer::compareOwnerships(
    const Registry& expected,
    const Registry& actual,
    const DiffOptions& options) const {

    std::vector<StateDifference> diffs;
    auto& expectedRaw = const_cast<Registry&>(expected).raw();
    auto& actualRaw = const_cast<Registry&>(actual).raw();

    auto expectedView = expectedRaw.view<OwnershipComponent>();
    for (auto entity : expectedView) {
        EntityID eid = static_cast<EntityID>(entity);

        if (!actualRaw.valid(entity)) continue;

        const auto& expOwn = expectedView.get<OwnershipComponent>(entity);
        const auto* actOwn = actualRaw.try_get<OwnershipComponent>(entity);

        if (!actOwn) {
            StateDifference diff;
            diff.type = DifferenceType::ComponentMissing;
            diff.entityId = eid;
            diff.componentName = "OwnershipComponent";
            diffs.push_back(diff);
            continue;
        }

        if (expOwn.owner != actOwn->owner ||
            expOwn.state != actOwn->state) {

            StateDifference diff;
            diff.type = DifferenceType::ComponentValueDiffers;
            diff.entityId = eid;
            diff.componentName = "OwnershipComponent";

            std::ostringstream exp, act;
            exp << "owner=" << static_cast<int>(expOwn.owner)
                << ",state=" << static_cast<int>(expOwn.state);
            act << "owner=" << static_cast<int>(actOwn->owner)
                << ",state=" << static_cast<int>(actOwn->state);
            diff.expectedValue = exp.str();
            diff.actualValue = act.str();
            diffs.push_back(diff);
        }
    }

    auto actualView = actualRaw.view<OwnershipComponent>();
    for (auto entity : actualView) {
        if (!expectedRaw.valid(entity)) continue;
        if (!expectedRaw.all_of<OwnershipComponent>(entity)) {
            StateDifference diff;
            diff.type = DifferenceType::ComponentExtra;
            diff.entityId = static_cast<EntityID>(entity);
            diff.componentName = "OwnershipComponent";
            diffs.push_back(diff);
        }
    }

    return diffs;
}

std::vector<StateDifference> StateDiffer::compareBuildings(
    const Registry& expected,
    const Registry& actual,
    const DiffOptions& options) const {

    std::vector<StateDifference> diffs;
    auto& expectedRaw = const_cast<Registry&>(expected).raw();
    auto& actualRaw = const_cast<Registry&>(actual).raw();

    auto expectedView = expectedRaw.view<BuildingComponent>();
    for (auto entity : expectedView) {
        EntityID eid = static_cast<EntityID>(entity);

        if (!actualRaw.valid(entity)) continue;

        const auto& expBld = expectedView.get<BuildingComponent>(entity);
        const auto* actBld = actualRaw.try_get<BuildingComponent>(entity);

        if (!actBld) {
            StateDifference diff;
            diff.type = DifferenceType::ComponentMissing;
            diff.entityId = eid;
            diff.componentName = "BuildingComponent";
            diffs.push_back(diff);
            continue;
        }

        if (expBld.buildingType != actBld->buildingType ||
            expBld.level != actBld->level ||
            expBld.health != actBld->health) {

            StateDifference diff;
            diff.type = DifferenceType::ComponentValueDiffers;
            diff.entityId = eid;
            diff.componentName = "BuildingComponent";

            std::ostringstream exp, act;
            exp << "type=" << expBld.buildingType
                << ",level=" << static_cast<int>(expBld.level)
                << ",health=" << static_cast<int>(expBld.health);
            act << "type=" << actBld->buildingType
                << ",level=" << static_cast<int>(actBld->level)
                << ",health=" << static_cast<int>(actBld->health);
            diff.expectedValue = exp.str();
            diff.actualValue = act.str();
            diffs.push_back(diff);
        }
    }

    auto actualView = actualRaw.view<BuildingComponent>();
    for (auto entity : actualView) {
        if (!expectedRaw.valid(entity)) continue;
        if (!expectedRaw.all_of<BuildingComponent>(entity)) {
            StateDifference diff;
            diff.type = DifferenceType::ComponentExtra;
            diff.entityId = static_cast<EntityID>(entity);
            diff.componentName = "BuildingComponent";
            diffs.push_back(diff);
        }
    }

    return diffs;
}

std::vector<StateDifference> StateDiffer::compareTransforms(
    const Registry& expected,
    const Registry& actual,
    const DiffOptions& options) const {

    std::vector<StateDifference> diffs;
    auto& expectedRaw = const_cast<Registry&>(expected).raw();
    auto& actualRaw = const_cast<Registry&>(actual).raw();

    auto expectedView = expectedRaw.view<TransformComponent>();
    for (auto entity : expectedView) {
        EntityID eid = static_cast<EntityID>(entity);

        if (!actualRaw.valid(entity)) continue;

        const auto& expTrn = expectedView.get<TransformComponent>(entity);
        const auto* actTrn = actualRaw.try_get<TransformComponent>(entity);

        if (!actTrn) {
            StateDifference diff;
            diff.type = DifferenceType::ComponentMissing;
            diff.entityId = eid;
            diff.componentName = "TransformComponent";
            diffs.push_back(diff);
            continue;
        }

        // Compare position components
        bool positionDiffers =
            !floatEqual(expTrn.position.x, actTrn->position.x, options.floatTolerance) ||
            !floatEqual(expTrn.position.y, actTrn->position.y, options.floatTolerance) ||
            !floatEqual(expTrn.position.z, actTrn->position.z, options.floatTolerance);

        // Compare rotation quaternion components
        bool rotationDiffers =
            !floatEqual(expTrn.rotation.w, actTrn->rotation.w, options.floatTolerance) ||
            !floatEqual(expTrn.rotation.x, actTrn->rotation.x, options.floatTolerance) ||
            !floatEqual(expTrn.rotation.y, actTrn->rotation.y, options.floatTolerance) ||
            !floatEqual(expTrn.rotation.z, actTrn->rotation.z, options.floatTolerance);

        // Compare scale components
        bool scaleDiffers =
            !floatEqual(expTrn.scale.x, actTrn->scale.x, options.floatTolerance) ||
            !floatEqual(expTrn.scale.y, actTrn->scale.y, options.floatTolerance) ||
            !floatEqual(expTrn.scale.z, actTrn->scale.z, options.floatTolerance);

        if (positionDiffers || rotationDiffers || scaleDiffers) {

            StateDifference diff;
            diff.type = DifferenceType::ComponentValueDiffers;
            diff.entityId = eid;
            diff.componentName = "TransformComponent";

            std::ostringstream exp, act;
            exp << std::fixed << std::setprecision(3)
                << "pos=(" << expTrn.position.x << ","
                << expTrn.position.y << "," << expTrn.position.z
                << "),rot=(" << expTrn.rotation.w << "," << expTrn.rotation.x << ","
                << expTrn.rotation.y << "," << expTrn.rotation.z
                << "),scale=(" << expTrn.scale.x << "," << expTrn.scale.y << ","
                << expTrn.scale.z << ")";
            act << std::fixed << std::setprecision(3)
                << "pos=(" << actTrn->position.x << ","
                << actTrn->position.y << "," << actTrn->position.z
                << "),rot=(" << actTrn->rotation.w << "," << actTrn->rotation.x << ","
                << actTrn->rotation.y << "," << actTrn->rotation.z
                << "),scale=(" << actTrn->scale.x << "," << actTrn->scale.y << ","
                << actTrn->scale.z << ")";
            diff.expectedValue = exp.str();
            diff.actualValue = act.str();
            diffs.push_back(diff);
        }
    }

    auto actualView = actualRaw.view<TransformComponent>();
    for (auto entity : actualView) {
        if (!expectedRaw.valid(entity)) continue;
        if (!expectedRaw.all_of<TransformComponent>(entity)) {
            StateDifference diff;
            diff.type = DifferenceType::ComponentExtra;
            diff.entityId = static_cast<EntityID>(entity);
            diff.componentName = "TransformComponent";
            diffs.push_back(diff);
        }
    }

    return diffs;
}

bool StateDiffer::floatEqual(float a, float b, float tolerance) const {
    return std::fabs(a - b) <= tolerance;
}

std::string summarizeDifferences(
    const std::vector<StateDifference>& diffs,
    std::size_t maxToShow) {

    if (diffs.empty()) {
        return "No differences found";
    }

    std::ostringstream oss;
    oss << diffs.size() << " difference(s) found:\n";

    std::size_t count = 0;
    for (const auto& diff : diffs) {
        if (maxToShow > 0 && count >= maxToShow) {
            oss << "  ... and " << (diffs.size() - count) << " more\n";
            break;
        }
        oss << "  - " << diff.toString() << "\n";
        count++;
    }

    return oss.str();
}

} // namespace sims3000
