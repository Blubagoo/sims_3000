# Debugger Context

This file accumulates debugging knowledge for the Sims 3000 project. The Debug Engineer uses this as context when investigating issues. The Orchestrator updates this when encountering decisions or discoveries that would help debugging.

---

## Tricky Areas

*Areas of the codebase that are known to be complex or error-prone.*

<!-- Example:
- **ECS Entity Destruction**: Entities marked for destruction during iteration. Always use deferred deletion to avoid iterator invalidation.
-->

*No entries yet.*

---

## Common Bugs

*Patterns of bugs that have occurred and how to diagnose/fix them.*

<!-- Example:
- **Null component access**: Symptom is crash during system update. Check that entity is valid AND component exists before access.
-->

*No entries yet.*

---

## Key State to Inspect

*Important state variables to check when debugging specific systems.*

<!-- Example:
- **Budget.balance**: Check after every transaction. Negative values indicate debt.
- **Building.state**: Should progress UnderConstruction -> Operational. If stuck, check construction timer.
-->

*No entries yet.*

---

## System Relationships

*Non-obvious interactions between systems.*

<!-- Example:
- **Zone -> Building**: Building placement validates against ZoneGrid. If building won't place, check zone type and density first.
- **Population -> Economy**: Tax calculation depends on population count. Population changes trigger budget recalculation.
-->

*No entries yet.*

---

## Past Bugs

*Record of bugs that have been fixed, for reference.*

| Date | Bug | Root Cause | Fix |
|------|-----|------------|-----|
| - | - | - | - |

---

## Debug Tips

*General debugging tips specific to this codebase.*

<!-- Example:
- Enable DEBUG_ZONES macro to see zone operation logging
- Use dumpBuildingState() to inspect building data
-->

*No entries yet.*

---

## Notes for Future Debugging

*Anything else that might help when debugging.*

*No entries yet.*
