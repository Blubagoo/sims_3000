# Orchestrator Decision Flow

How the top-level orchestrator processes prompts and decides which agents to spawn.

**Sources of Truth:**
- **Requirements:** ClickUp task definition
- **Codebase State:** Explorer agent investigation
- **Domain Rules:** Agent profiles in `/agents/`

---

## Example Prompt: "Add road tool to utility bar"

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         ORCHESTRATOR RECEIVES PROMPT                         │
│                      "Add road tool to utility bar"                          │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  STEP 1: PARSE REQUEST + FETCH TASK                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│  Keywords identified:                                                        │
│    • "road tool" → tool for placing roads                                    │
│    • "utility bar" → UI toolbar element                                      │
│                                                                              │
│  ClickUp task says:                                                          │
│    • Scope: Full road placement system                                       │
│    • Interaction: Click-drag to place roads                                  │
│    • Acceptance: Roads connect, render, affect traffic                       │
│                                                                              │
│  Domains touched:                                                            │
│    • UI (toolbar, button, tool selection)                                    │
│    • Infrastructure (road placement, road network)                           │
│    • Graphics (road rendering, placement preview)                            │
│    • ECS (road entities/components)                                          │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  STEP 2: INVESTIGATE CODEBASE (spawn Explorer)                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  Task Tool → Explorer Agent (PARALLEL where possible):                       │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │  "Does a utility bar exist? Find UI toolbar implementation."          │  │
│  │  "What ECS components exist? Check /src/ecs/components/"              │  │
│  │  "What rendering systems exist? Find tilemap/sprite renderers."       │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
│  Explorer returns:                                                           │
│    ✓ Utility bar exists at /src/ui/Toolbar.cpp                              │
│    ✓ ECS has PositionComponent, SpriteComponent - no road components yet    │
│    ✓ TilemapRenderer exists, can be extended for roads                      │
│                                                                              │
│  NOW I KNOW what exists and what needs to be created.                        │
│  No user questions needed - I investigated myself.                           │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  STEP 3: MAP DOMAINS → PROFILES                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  Based on investigation + agent profiles:                                    │
│                                                                              │
│  ┌─────────────────┐     ┌──────────────────────────────────────────────┐   │
│  │   UI Developer  │────▶│ Add button to existing Toolbar.cpp           │   │
│  └─────────────────┘     └──────────────────────────────────────────────┘   │
│                                                                              │
│  ┌─────────────────┐     ┌──────────────────────────────────────────────┐   │
│  │ Infrastructure  │────▶│ Road placement logic, network connectivity   │   │
│  │   Engineer      │     │                                              │   │
│  └─────────────────┘     └──────────────────────────────────────────────┘   │
│                                                                              │
│  ┌─────────────────┐     ┌──────────────────────────────────────────────┐   │
│  │    Graphics     │────▶│ Extend TilemapRenderer for road tiles        │   │
│  │    Engineer     │     │                                              │   │
│  └─────────────────┘     └──────────────────────────────────────────────┘   │
│                                                                              │
│  ┌─────────────────┐     ┌──────────────────────────────────────────────┐   │
│  │  ECS Architect  │────▶│ Create RoadComponent, RoadNetworkComponent   │   │
│  └─────────────────┘     └──────────────────────────────────────────────┘   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  STEP 4: IDENTIFY DEPENDENCIES                                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│                    ┌─────────────────┐                                       │
│                    │  ECS: Road      │  ◄─── MUST EXIST FIRST                │
│                    │  Components     │       (others depend on this)         │
│                    └────────┬────────┘                                       │
│                             │                                                │
│           ┌─────────────────┼─────────────────┐                              │
│           │                 │                 │                              │
│           ▼                 ▼                 ▼                              │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐                 │
│  │ Infrastructure: │ │ Graphics: Road  │ │ UI: Toolbar     │                 │
│  │ Road Placement  │ │ Rendering       │ │ Button          │                 │
│  └────────┬────────┘ └────────┬────────┘ └────────┬────────┘                 │
│           │                   │                   │                          │
│           │      PARALLEL     │      PARALLEL     │                          │
│           │  (no deps between │  (no deps between │                          │
│           │      each other)  │      each other)  │                          │
│           └───────────────────┼───────────────────┘                          │
│                               │                                              │
│                               ▼                                              │
│                    ┌─────────────────┐                                       │
│                    │ Integration:    │  ◄─── FINAL STEP                      │
│                    │ Wire it all     │       (connect UI → placement         │
│                    │ together        │        → rendering)                   │
│                    └─────────────────┘                                       │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  STEP 5: SPAWN IMPLEMENTATION AGENTS                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  WAVE 1 (Sequential - foundation):                                           │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │  Task Tool → ECS Architect                                            │  │
│  │  "Create RoadComponent, RoadSegment, RoadNetwork components"          │  │
│  │  Profile: /agents/ecs-architect.md                                    │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                              │                                               │
│                              ▼ (wait for completion, verify)                 │
│                                                                              │
│  WAVE 2 (PARALLEL - all in single message):                                  │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │  Task Tool → Infrastructure Engineer          ┐                       │  │
│  │  "Implement road placement system"            │                       │  │
│  │  Profile: /agents/infrastructure-engineer.md  │                       │  │
│  ├───────────────────────────────────────────────┤  SINGLE MESSAGE       │  │
│  │  Task Tool → Graphics Engineer                │  3 TASK CALLS         │  │
│  │  "Extend TilemapRenderer for road tiles"      │                       │  │
│  │  Profile: /agents/graphics-engineer.md        │                       │  │
│  ├───────────────────────────────────────────────┤                       │  │
│  │  Task Tool → UI Developer                     │                       │  │
│  │  "Add road tool button to Toolbar.cpp"        │                       │  │
│  │  Profile: /agents/ui-developer.md             ┘                       │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                              │                                               │
│                              ▼ (wait for all, verify each)                   │
│                                                                              │
│  WAVE 3 (Sequential - integration):                                          │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │  Task Tool → UI Developer or Engine Developer                         │  │
│  │  "Wire toolbar button to road placement system"                       │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  STEP 6: VERIFY QUALITY GATES                                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  After each wave:                                                            │
│    ☐ Compilation check (spawn fix agent if fails)                           │
│    ☐ Test run (if applicable)                                               │
│    ☐ Pattern compliance                                                     │
│                                                                              │
│  Final verification:                                                         │
│    ☐ Click road tool → enters road placement mode                           │
│    ☐ Drag on map → shows road preview                                       │
│    ☐ Release → road placed and rendered                                     │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  STEP 7: REPORT COMPLETION                                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  "Road tool implemented:                                                     │
│   - Button added to utility bar                                             │
│   - Click-drag placement working                                            │
│   - Roads render with proper tiles                                          │
│   - Network connectivity tracked"                                           │
│                                                                              │
│  Update ClickUp task status → Done                                          │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Decision Reference

| Decision | How Orchestrator Decides |
|----------|--------------------------|
| Requirements/scope | Read ClickUp task - that's the source of truth |
| What exists? | Spawn Explorer agent to investigate codebase |
| Which profiles? | Match domains to profile tables |
| Parallel vs sequential? | Check for data/API dependencies between tasks |
| When to ask user? | **ONLY** for true conflicts: ClickUp task contradicts codebase reality, or requirement is genuinely ambiguous after investigation |
| When to spawn fix agent? | Compilation fails, tests fail, pattern violations |

---

## Anti-Pattern: Don't Ask What You Can Investigate

```
BAD:  "Does the utility bar exist?"          → User answers
GOOD: Spawn Explorer → "Find utility bar"    → I know the answer

BAD:  "What's the scope of road tool?"       → User answers
GOOD: Read ClickUp task                      → Scope is defined there

BAD:  "Any visual requirements?"             → User answers
GOOD: Read ClickUp task + check existing UI  → Follow established patterns
```

The orchestrator should be autonomous. Investigate first, act on findings.

---

## See Also

- [Top-Level Agent Profile](/agents/top-level-agent.md)
- [Agent Architecture Decision](/plans/decisions/agent-architecture.md)
