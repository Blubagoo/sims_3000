# Plan Epic

Orchestrate multiple agents to collaboratively plan an epic, producing a ticket breakdown with dependencies.

## Usage

```
/plan-epic <epic_number> [--agents <agent1,agent2,...>]
```

**Examples:**
```
/plan-epic 5                              # Uses default agents for Epic 5
/plan-epic 5 --agents systems-architect,game-designer,qa-engineer
```

## When to Use

Use this skill when:
- Starting work on a new epic
- Re-planning an epic after major canon changes
- Need detailed ticket breakdown before implementation

## Prerequisites

Before running, ensure:
1. Canon files exist in `/docs/canon/`
2. Agent profiles exist in `/agents/`
3. Discussion directory exists at `/docs/discussions/`

---

## Workflow Overview

```
┌─────────────────────────────────────────────────────────────────┐
│  1. DIGEST CANON                                                │
│     Read canon files, extract epic scope and systems            │
├─────────────────────────────────────────────────────────────────┤
│  2. SPAWN PLANNING AGENTS (parallel)                            │
│     Each agent analyzes epic from their perspective             │
├─────────────────────────────────────────────────────────────────┤
│  3. DISCUSSION PHASE                                            │
│     Agents ask/answer questions via discussion doc              │
├─────────────────────────────────────────────────────────────────┤
│  4. TICKET SYNTHESIS                                            │
│     Combine agent outputs into unified ticket list              │
├─────────────────────────────────────────────────────────────────┤
│  5. CANON VERIFICATION                                          │
│     Systems Architect reviews tickets against canon             │
├─────────────────────────────────────────────────────────────────┤
│  6. DECISION CAPTURE (if conflicts)                             │
│     Document any canon deviations as decisions                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Phase 1: Digest Canon

Read and extract relevant information:

### Files to Read
```
/docs/canon/CANON.md           # Core principles
/docs/canon/systems.yaml       # Epic systems and boundaries
/docs/canon/interfaces.yaml    # Contracts this epic must implement
/docs/canon/patterns.yaml      # How to build
/docs/canon/terminology.yaml   # Naming conventions
```

### Extract for Epic N
```yaml
epic_info:
  number: N
  name: "Epic Name"
  phase: X
  systems:
    - SystemName1
    - SystemName2
  components:
    - ComponentName1
    - ComponentName2
  dependencies:
    depends_on:
      - Epic M (reason)
    required_by:
      - Epic P (reason)
  interfaces:
    implements:
      - IInterfaceName
    consumes:
      - IOtherInterface
```

---

## Phase 2: Spawn Planning Agents

Launch specified agents in parallel. Each agent produces analysis from their perspective.

### Agent Prompt Template

```
You are planning EPIC {N}: {Epic Name}

Agent Profile: Read and follow /agents/{agent-name}.md

CANON CONTEXT (read these first):
- /docs/canon/CANON.md
- /docs/canon/systems.yaml (focus on epic_{N} section)
- /docs/canon/interfaces.yaml
- /docs/canon/patterns.yaml

EPIC SCOPE:
{Extracted epic info from Phase 1}

YOUR TASK:
Analyze this epic from your perspective ({agent role}).
Identify:
1. Key work items needed
2. Questions for other agents (will go to discussion doc)
3. Risks or concerns
4. Dependencies you see

OUTPUT FORMAT:
Write your analysis to: /docs/epics/epic-{N}/planning/{agent-name}-analysis.md

Use this structure:
---
# {Agent Name} Analysis: Epic {N}

## Key Work Items
- [ ] Item 1: Description
- [ ] Item 2: Description

## Questions for Other Agents
(These will be added to discussion doc)
- @{target-agent}: Question text?

## Risks & Concerns
- Risk 1
- Risk 2

## Dependencies
- Depends on: ...
- Blocks: ...
---
```

---

## Phase 3: Discussion Phase

### Create Discussion Document

After agents produce initial analysis, create a discussion doc:

```yaml
# /docs/discussions/epic-{N}-planning.discussion.yaml
topic: "Epic {N} Planning: {Epic Name}"
created: "{date}"
status: active
participants:
  - systems-architect
  - game-designer
  - {other agents}

threads: []
```

### Populate Questions

Extract questions from each agent's analysis (lines starting with `@agent-name:`) and add as threads:

```yaml
threads:
  - id: Q001
    status: open
    author: "systems-architect"
    target: "game-designer"
    timestamp: "{timestamp}"
    question: |
      {Question extracted from analysis}
    answers: []
```

### Run Answer Phase

Spawn each agent again to answer questions directed at them:

```
You are answering questions for EPIC {N} planning.

Agent Profile: Read and follow /agents/{agent-name}.md

Read the discussion document:
/docs/discussions/epic-{N}-planning.discussion.yaml

Find all threads where target: "{your-agent-name}" and status: open.

For each question:
1. Read the question carefully
2. Consider your domain expertise
3. Write a clear, actionable answer
4. Update the thread status to "answered"

Also read other agents' answers - if you have follow-ups, add new questions.
```

### Iterate Until Resolved

Repeat answer phase until all threads are `answered` or `resolved`.

---

## Phase 4: Ticket Synthesis

Combine all agent analyses and resolved discussions into a unified ticket list.

### Output File
```
/docs/epics/epic-{N}/tickets.md
```

### Ticket Format

```markdown
# Epic {N}: {Epic Name} - Tickets

Generated: {date}
Canon Version: {version from CANON.md}
Planning Agents: {list}

## Overview

{Brief summary of epic scope}

## Tickets

### Ticket {N}-001: {Title}

**Type:** feature | task | research | infrastructure
**System:** {SystemName}
**Estimated Scope:** S | M | L | XL

**Description:**
{What needs to be done}

**Acceptance Criteria:**
- [ ] Criterion 1
- [ ] Criterion 2

**Dependencies:**
- Blocked by: {N}-000 (reason)
- Blocks: {N}-002 (reason)

**Agent Notes:**
- Systems Architect: {relevant notes}
- Game Designer: {relevant notes}

---

### Ticket {N}-002: {Title}
...
```

### Ticket Ordering

1. Infrastructure/setup tickets first
2. Core functionality
3. Integration points
4. Polish/testing
5. Documentation

---

## Phase 5: Canon Verification

Systems Architect reviews all tickets against canon.

### Verification Prompt

```
You are the Systems Architect verifying Epic {N} tickets against canon.

Agent Profile: Read and follow /agents/systems-architect.md

CANON FILES (source of truth):
- /docs/canon/CANON.md
- /docs/canon/systems.yaml
- /docs/canon/interfaces.yaml
- /docs/canon/patterns.yaml

TICKETS TO VERIFY:
/docs/epics/epic-{N}/tickets.md

FOR EACH TICKET, CHECK:
1. Does it respect system boundaries from systems.yaml?
2. Does it implement required interfaces from interfaces.yaml?
3. Does it follow patterns from patterns.yaml?
4. Does terminology match terminology.yaml?
5. Does it account for multiplayer (core principle)?

OUTPUT:
Write verification report to: /docs/epics/epic-{N}/canon-verification.md

Format:
---
# Canon Verification: Epic {N}

## Summary
- Total tickets: X
- Canon-compliant: Y
- Conflicts found: Z

## Conflicts

### Conflict 1: {Ticket ID}
**Issue:** {What violates canon}
**Canon Reference:** {Which file/section}
**Resolution Options:**
A) Modify ticket to comply
B) Propose canon update (if canon is wrong)

### Conflict 2: ...

## Recommendations
...
---
```

---

## Phase 6: Decision Capture

For each conflict, trigger decision process.

### If Ticket Should Change

Update the ticket in `tickets.md` to comply with canon.

### If Canon Should Change

Create a decision record:

```
/plans/decisions/epic-{N}-{decision-slug}.md
```

Use standard decision format:
```markdown
# Decision: {Title}

**Date:** {date}
**Status:** proposed | accepted | rejected
**Epic:** {N}
**Ticket:** {ticket ID}

## Context
{Why this came up}

## Canon Conflict
**Current Canon:** {what canon says}
**Proposed Change:** {what we want instead}

## Options
1. {Option A}
2. {Option B}

## Decision
{Which option and why}

## Consequences
- Canon files to update: {list}
- Other epics affected: {list}
```

After decision is accepted, use `/update-canon` skill to update canon files.

---

## Default Agent Assignments

Each epic has recommended planning agents based on its domain:

| Epic | Name | Default Agents |
|------|------|----------------|
| 0 | Foundation | systems-architect, engine-developer |
| 1 | Multiplayer | systems-architect, engine-developer, qa-engineer |
| 2 | Rendering | systems-architect, graphics-engineer, game-designer |
| 3 | Terrain | systems-architect, graphics-engineer, game-designer |
| 4 | Zoning & Building | systems-architect, game-designer, zone-engineer, building-engineer |
| 5 | Power | systems-architect, game-designer, services-engineer |
| 6 | Water | systems-architect, game-designer, services-engineer |
| 7 | Transportation | systems-architect, game-designer, infrastructure-engineer |
| 8 | Ports | systems-architect, game-designer, economy-engineer |
| 9 | Services | systems-architect, game-designer, services-engineer, qa-engineer |
| 10 | Simulation | systems-architect, game-designer, population-engineer, economy-engineer |
| 11 | Financial | systems-architect, game-designer, economy-engineer |
| 12 | UI | systems-architect, game-designer, ui-developer |
| 13 | Disasters | systems-architect, game-designer, services-engineer |
| 14 | Progression | systems-architect, game-designer |
| 15 | Audio | systems-architect, audio-engineer, game-designer |
| 16 | Persistence | systems-architect, data-engineer, qa-engineer |
| 17 | Polish | systems-architect, game-designer, qa-engineer |

**Note:** `systems-architect` and `game-designer` are included in ALL epics as they provide cross-cutting perspective.

---

## Output Artifacts

After running `/plan-epic N`, these files are created:

```
docs/epics/epic-{N}/
├── planning/
│   ├── systems-architect-analysis.md
│   ├── game-designer-analysis.md
│   └── {other-agent}-analysis.md
├── tickets.md                    # Final ticket breakdown
└── canon-verification.md         # Verification report

docs/discussions/
└── epic-{N}-planning.discussion.yaml

plans/decisions/
└── epic-{N}-*.md                 # Any decisions made (if conflicts)
```

---

## Example: Planning Epic 5 (Power)

```bash
/plan-epic 5
```

**Agents spawned:** systems-architect, game-designer, services-engineer

**Phase 1 extracts:**
- Systems: EnergySystem
- Components: EnergyComponent, EnergyProducerComponent, EnergyConduitComponent
- Dependencies: BuildingSystem (Epic 4), TerrainSystem (Epic 3)
- Interfaces: ISimulatable, IPowerProvider

**Phase 2 produces:**
- `systems-architect-analysis.md` - System boundaries, data flow, multiplayer sync
- `game-designer-analysis.md` - Player experience, brownout tension, alien naming
- `services-engineer-analysis.md` - Technical implementation, coverage algorithms

**Phase 3 resolves:**
- Q: "Should power failure destroy buildings?" → A: "No, pause them"
- Q: "Pool model vs flow simulation?" → A: "Pool model per canon"

**Phase 4 produces:**
- 12 tickets covering nexus placement, coverage calculation, brownout UI, etc.

**Phase 5 finds:**
- 1 conflict: Ticket referenced "power lines" but canon uses "energy conduits"
- Resolution: Updated ticket terminology

**Final output:**
- `/docs/epics/epic-5/tickets.md` with 12 canon-compliant tickets
