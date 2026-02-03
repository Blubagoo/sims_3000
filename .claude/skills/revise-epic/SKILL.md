# Revise Epic

Surgically update an already-planned epic when canon files or technical decisions change, preserving stable ticket IDs and producing an audit trail.

## Usage

```
/revise-epic <epic_number> --trigger <decision_path|"canon-update"> [--agents <agent1,agent2,...>]
```

**Examples:**
```
/revise-epic 2 --trigger plans/decisions/3d-rendering-architecture.md
/revise-epic 5 --trigger canon-update
/revise-epic 1 --trigger plans/decisions/epic-1-lockfree-queue.md --agents systems-architect,engine-developer
```

## When to Use

Use this skill when:
- A technical decision changes something an already-planned epic depends on
- Canon files have been updated and existing tickets may be affected
- A new decision record introduces constraints that invalidate or modify planned work
- Another epic's revision report flags this epic as potentially impacted

**Do NOT use when:**
- The epic has no `tickets.md` yet — use `/plan-epic` instead
- You want to plan an epic from scratch — use `/plan-epic` instead

## Prerequisites

Before running, ensure:
1. `/docs/epics/epic-{N}/tickets.md` exists (if not, redirect to `/plan-epic`)
2. The `--trigger` path exists and is readable
3. Canon files exist in `/docs/canon/`
4. Agent profiles exist in `/agents/`

---

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| **Ticket ID stability** | Unchanged/modified tickets keep their original IDs (e.g., `2-003`). New tickets get the next available ID. |
| **Required `--trigger`** | Every revision must be traceable to a specific decision or canon update. No untriggered revisions. |
| **Skip-if-no-impact** | If all agents agree the change has no impact, short-circuit after Phase 2 with a brief report. |
| **Cascading awareness** | The revision report flags other epics that may need their own revision. |
| **Epic not yet planned** | If `tickets.md` doesn't exist, abort and redirect the user to `/plan-epic`. |

---

## Workflow Overview

```
┌─────────────────────────────────────────────────────────────────┐
│  1. CHANGE DETECTION                                            │
│     Read trigger doc + existing artifacts, compute change delta  │
├─────────────────────────────────────────────────────────────────┤
│  2. IMPACT ASSESSMENT (parallel agents)                         │
│     Agents receive delta + their previous analysis              │
│     Produce impact classification, not full re-analysis         │
├─────────────────────────────────────────────────────────────────┤
│  ── SHORT-CIRCUIT if all agents report NO_IMPACT ──             │
├─────────────────────────────────────────────────────────────────┤
│  3. TARGETED DISCUSSION                                         │
│     Only new questions from the delta                           │
│     Preserves existing resolved threads                         │
├─────────────────────────────────────────────────────────────────┤
│  4. TICKET REVISION                                             │
│     Classify tickets: UNCHANGED / MODIFIED / REMOVED / NEW      │
│     Preserve original IDs for unchanged/modified tickets        │
├─────────────────────────────────────────────────────────────────┤
│  5. CANON RE-VERIFICATION                                       │
│     Systems Architect reviews changed tickets against canon     │
├─────────────────────────────────────────────────────────────────┤
│  6. DECISION CAPTURE (if conflicts)                             │
│     Document any canon deviations as decisions                  │
├─────────────────────────────────────────────────────────────────┤
│  7. REVISION REPORT                                             │
│     Audit trail: what changed, why, and cascading impacts       │
└─────────────────────────────────────────────────────────────────┘
```

---

## Phase 1: Change Detection

Read the trigger document and existing epic artifacts to compute a structured change delta.

### Files to Read

```
# Trigger
{trigger_path}                              # The decision or canon change

# Existing Epic Artifacts
/docs/epics/epic-{N}/tickets.md             # Current ticket breakdown
/docs/epics/epic-{N}/canon-verification.md  # Previous verification
/docs/epics/epic-{N}/planning/              # All agent analysis files

# Canon (current state)
/docs/canon/CANON.md
/docs/canon/systems.yaml
/docs/canon/interfaces.yaml
/docs/canon/patterns.yaml
/docs/canon/terminology.yaml
```

### Compute Change Delta

Produce a structured delta describing what changed and what it could affect:

```yaml
change_delta:
  trigger: "{trigger_path}"
  trigger_type: decision | canon-update
  date: "YYYY-MM-DD"
  epic: N

  summary: |
    One-paragraph description of what changed.

  changes:
    - area: "systems | interfaces | patterns | terminology | architecture"
      what: "Description of specific change"
      before: "Previous state (quote from old doc or 'N/A' if new)"
      after: "New state (quote from trigger doc)"

  potentially_affected:
    systems:
      - SystemName1
      - SystemName2
    components:
      - ComponentName1
    interfaces:
      - IInterfaceName
    tickets:
      - "{N}-003"   # Tickets that reference affected systems/components
      - "{N}-007"

  scope_estimate: minimal | moderate | extensive
```

### Redirect Check

If `/docs/epics/epic-{N}/tickets.md` does not exist:

```
⚠ Epic {N} has no tickets.md. This epic has not been planned yet.
→ Use /plan-epic {N} instead.
```

Abort the revision workflow.

---

## Phase 2: Impact Assessment

Launch agents in parallel. Each agent receives the change delta and their previous analysis, then produces an impact classification — not a full re-analysis.

### Agent Prompt Template

```
You are assessing the IMPACT of a change on EPIC {N}: {Epic Name}

Agent Profile: Read and follow /agents/{agent-name}.md

CHANGE DELTA:
{Structured change delta from Phase 1}

YOUR PREVIOUS ANALYSIS:
Read: /docs/epics/epic-{N}/planning/{agent-name}-analysis.md

CURRENT TICKETS:
Read: /docs/epics/epic-{N}/tickets.md

CANON CONTEXT (read for reference):
- /docs/canon/CANON.md
- /docs/canon/systems.yaml
- /docs/canon/interfaces.yaml
- /docs/canon/patterns.yaml

YOUR TASK:
Assess the impact of this change from your perspective ({agent role}).
Do NOT re-analyze the entire epic. Focus only on what the delta changes.

For each item in your previous analysis, classify:
- UNAFFECTED — this change does not impact this item
- MODIFIED — this item needs updating due to the change
- INVALIDATED — this item is no longer valid
- NEW_CONCERN — the change introduces something not in your original analysis

OUTPUT FORMAT:
Write to: /docs/epics/epic-{N}/planning/{agent-name}-impact-analysis.md

Use this structure:
---
# {Agent Name} Impact Analysis: Epic {N}

## Trigger
{trigger_path}

## Overall Impact: NO_IMPACT | LOW | MEDIUM | HIGH

## Impact Summary
{1-2 sentences on what this change means for your domain}

## Item-by-Item Assessment

### UNAFFECTED
- Item: {description} — Reason: {why unchanged}

### MODIFIED
- Item: {description}
  - Previous: {what it was}
  - Revised: {what it should become}
  - Reason: {why the change requires this}

### INVALIDATED
- Item: {description}
  - Reason: {why no longer valid}
  - Recommendation: remove | replace with {alternative}

### NEW CONCERNS
- Concern: {description}
  - Reason: {why the change introduces this}
  - Recommendation: {what to do}

## Questions for Other Agents
(Only if the change raises NEW questions not already resolved)
- @{target-agent}: {question}?

## Affected Tickets
- {N}-003: MODIFY — {what needs changing}
- {N}-007: REMOVE — {why}
- NEW: {description of new ticket needed}
---
```

### Short-Circuit Check

After all agents complete Phase 2, check overall impact:

```
If ALL agents report "Overall Impact: NO_IMPACT":
  → Skip to Phase 7 (Revision Report)
  → Report: "No impact detected. All agents confirm the change
     does not affect Epic {N} tickets."
  → No ticket modifications needed.
```

---

## Phase 3: Targeted Discussion

Only create a discussion if agents raised NEW questions in Phase 2. Preserve existing resolved threads.

### Discussion Document

```yaml
# /docs/discussions/epic-{N}-revision-{YYYY-MM-DD}.discussion.yaml
topic: "Epic {N} Revision: {Trigger Summary}"
created: "{date}"
status: active
trigger: "{trigger_path}"
participants:
  - {agent-list}

# Import only NEW questions from Phase 2 impact analyses
threads:
  - id: RQ001
    status: open
    author: "{agent-name}"
    target: "{target-agent}"
    timestamp: "{timestamp}"
    question: |
      {Question from impact analysis}
    answers: []
```

**Question ID Convention:** Use `RQ` prefix (Revision Question) + 3-digit number to distinguish from original planning questions (`Q001`).

### Run Answer Phase

Spawn each targeted agent to answer questions:

```
You are answering questions for EPIC {N} revision.

Agent Profile: Read and follow /agents/{agent-name}.md

CHANGE CONTEXT:
{Change delta from Phase 1}

Read the revision discussion document:
/docs/discussions/epic-{N}-revision-{YYYY-MM-DD}.discussion.yaml

Also read the original planning discussion for context:
/docs/discussions/epic-{N}-planning.discussion.yaml (if exists)

Find all threads where target: "{your-agent-name}" and status: open.

For each question:
1. Read the question carefully
2. Consider the change delta and your impact analysis
3. Write a clear, actionable answer
4. Update the thread status to "answered"
```

### Skip Condition

If no agents raised new questions in Phase 2, skip Phase 3 entirely.

---

## Phase 4: Ticket Revision

Classify every existing ticket and produce the updated `tickets.md`.

### Classification Categories

| Category | Meaning | Action |
|----------|---------|--------|
| **UNCHANGED** | Change does not affect this ticket | Keep as-is, retain original ID |
| **MODIFIED** | Ticket needs updating | Update content, retain original ID |
| **REMOVED** | Ticket is no longer needed | Remove from tickets.md |
| **NEW** | Change requires a new ticket | Add with next available ID |

### Ticket Revision Prompt

```
You are revising tickets for EPIC {N}: {Epic Name}

CHANGE DELTA:
{From Phase 1}

AGENT IMPACT ANALYSES:
Read all files in: /docs/epics/epic-{N}/planning/*-impact-analysis.md

RESOLVED DISCUSSION (if Phase 3 ran):
/docs/discussions/epic-{N}-revision-{YYYY-MM-DD}.discussion.yaml

CURRENT TICKETS:
/docs/epics/epic-{N}/tickets.md

CANON CONTEXT:
- /docs/canon/CANON.md
- /docs/canon/systems.yaml
- /docs/canon/interfaces.yaml
- /docs/canon/patterns.yaml

YOUR TASK:
Revise the ticket list based on impact analyses and discussion outcomes.

RULES:
1. UNCHANGED tickets: Copy exactly as-is. Do not modify content or ID.
2. MODIFIED tickets: Keep the original ticket ID ({N}-XXX). Update only
   the fields that changed. Add a revision note at the bottom:
   > **Revised {date}:** {what changed and why} (trigger: {trigger_path})
3. REMOVED tickets: Do not include in output. Document in removal log.
4. NEW tickets: Assign next sequential ID after the highest existing ID.
   Add a creation note:
   > **Added {date}:** {why this ticket is needed} (trigger: {trigger_path})

OUTPUT FORMAT:
Update: /docs/epics/epic-{N}/tickets.md

Add a revision header after the generation metadata:

## Revision History

| Date | Trigger | Tickets Modified | Tickets Removed | Tickets Added |
|------|---------|------------------|-----------------|---------------|
| {date} | {trigger_path} | {N}-003, {N}-007 | {N}-012 | {N}-015 |

Then include all tickets (UNCHANGED + MODIFIED + NEW) in dependency order.

ALSO OUTPUT a classification summary at the end of the file:

## Ticket Classification ({date})

| Ticket | Status | Notes |
|--------|--------|-------|
| {N}-001 | UNCHANGED | |
| {N}-002 | UNCHANGED | |
| {N}-003 | MODIFIED | Updated system boundary per {trigger} |
| {N}-007 | MODIFIED | New interface requirement |
| {N}-012 | REMOVED | Superseded by {N}-015 |
| {N}-015 | NEW | Covers 3D rendering pipeline |
```

---

## Phase 5: Canon Re-Verification

Systems Architect reviews changed and new tickets against canon. Unchanged tickets are not re-verified.

### Verification Prompt

```
You are the Systems Architect verifying revised Epic {N} tickets against canon.

Agent Profile: Read and follow /agents/systems-architect.md

CANON FILES (source of truth):
- /docs/canon/CANON.md
- /docs/canon/systems.yaml
- /docs/canon/interfaces.yaml
- /docs/canon/patterns.yaml

REVISED TICKETS:
/docs/epics/epic-{N}/tickets.md

PREVIOUS VERIFICATION:
/docs/epics/epic-{N}/canon-verification.md

TICKET CLASSIFICATION:
Read the "Ticket Classification" table at the end of tickets.md.

YOUR TASK:
Only verify tickets classified as MODIFIED or NEW.
UNCHANGED tickets were verified during original planning — skip them.

FOR EACH MODIFIED/NEW TICKET, CHECK:
1. Does it respect system boundaries from systems.yaml?
2. Does it implement required interfaces from interfaces.yaml?
3. Does it follow patterns from patterns.yaml?
4. Does terminology match terminology.yaml?
5. Does it account for multiplayer (core principle)?

OUTPUT:
Update: /docs/epics/epic-{N}/canon-verification.md

Preserve the original verification results for UNCHANGED tickets.
Add/update entries for MODIFIED and NEW tickets.

Format additions as:
---
## Revision Verification ({date})

### Trigger
{trigger_path}

### Verified Tickets

| Ticket | Status | Canon Compliant | Notes |
|--------|--------|-----------------|-------|
| {N}-003 | MODIFIED | Yes | Updated boundary now matches systems.yaml |
| {N}-007 | MODIFIED | Yes | Interface requirement correctly added |
| {N}-015 | NEW | No | See Conflict 1 below |

### New Conflicts (if any)

#### Conflict 1: {Ticket ID}
**Issue:** {What violates canon}
**Canon Reference:** {Which file/section}
**Resolution Options:**
A) Modify ticket to comply
B) Propose canon update (if canon is wrong)
---
```

---

## Phase 6: Decision Capture

Identical to the plan-epic workflow. For each conflict found in Phase 5, trigger the decision process.

### If Ticket Should Change

Update the ticket in `tickets.md` to comply with canon. Add a revision note:

```markdown
> **Revised {date}:** Fixed canon conflict — {description} (trigger: {trigger_path})
```

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
**Triggered By:** {trigger_path}

## Context
{Why this came up during revision}

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

## Phase 7: Revision Report

Produce an audit trail documenting what changed and why.

### Report Prompt

```
You are documenting the revision of EPIC {N}: {Epic Name}

INPUTS:
- Change delta from Phase 1
- All impact analyses from Phase 2
- Discussion document from Phase 3 (if created)
- Ticket classification from Phase 4
- Verification results from Phase 5
- Any decisions from Phase 6

YOUR TASK:
Create a comprehensive revision report that serves as an audit trail.

OUTPUT:
Write to: /docs/epics/epic-{N}/revision-report-{YYYY-MM-DD}.md

Use this structure:
---
# Revision Report: Epic {N} — {date}

## Trigger
**Document:** {trigger_path}
**Type:** decision | canon-update
**Summary:** {one-line summary of the change}

## Impact Summary

| Agent | Impact Level | Key Finding |
|-------|-------------|-------------|
| systems-architect | HIGH | {summary} |
| game-designer | LOW | {summary} |
| {agent} | NO_IMPACT | No effect on this domain |

## Ticket Changes

### Modified Tickets
| Ticket | What Changed | Why |
|--------|-------------|-----|
| {N}-003 | Updated system boundary | {trigger} redefines SystemX scope |
| {N}-007 | Added interface requirement | New IRenderable contract |

### Removed Tickets
| Ticket | Original Purpose | Removal Reason |
|--------|-----------------|----------------|
| {N}-012 | Old rendering setup | Superseded by GPU pipeline approach |

### New Tickets
| Ticket | Purpose | Triggered By |
|--------|---------|-------------|
| {N}-015 | GPU pipeline initialization | 3D rendering decision |

### Unchanged Tickets
{N}-001, {N}-002, {N}-004, {N}-005, {N}-006, {N}-008, {N}-009, {N}-010, {N}-011, {N}-013, {N}-014

## Discussion Summary
{Summary of key questions and resolutions from Phase 3, or "No new questions raised."}

## Canon Verification
- Tickets verified: {count}
- Conflicts found: {count}
- Resolutions: {summary}

## Decisions Made
- {decision-slug}: {summary} (status: {status})
- Or: "No new decisions required."

## Cascading Impact

The following epics MAY need revision based on this change:

| Epic | Reason | Recommended Action |
|------|--------|--------------------|
| Epic 3 | Shares rendering interfaces | Run /revise-epic 3 --trigger {trigger_path} |
| Epic 12 | UI depends on rendering output | Review after Epic 2 revision finalizes |

Or: "No cascading impact detected."
---
```

---

## Default Agent Assignments

Same as `/plan-epic` — each epic uses its domain-relevant agents:

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

After running `/revise-epic N --trigger <path>`, these files are created or updated:

```
docs/epics/epic-{N}/
├── planning/
│   ├── {agent-name}-impact-analysis.md     # Per-agent impact assessment (NEW)
│   └── {agent-name}-analysis.md            # Original analysis (PRESERVED)
├── tickets.md                               # Updated ticket breakdown (UPDATED)
├── canon-verification.md                    # Updated verification (UPDATED)
└── revision-report-{YYYY-MM-DD}.md         # Audit trail (NEW)

docs/discussions/
└── epic-{N}-revision-{YYYY-MM-DD}.discussion.yaml  # Only if new questions arise (NEW)

plans/decisions/
└── epic-{N}-*.md                            # Only if new conflicts arise (NEW)
```

---

## Example: Revising Epic 2 After 3D Rendering Decision

```bash
/revise-epic 2 --trigger plans/decisions/3d-rendering-architecture.md
```

**Agents spawned:** systems-architect, graphics-engineer, game-designer

### Phase 1: Change Detection

The trigger document introduces a GPU-driven rendering pipeline replacing the originally planned CPU-side approach. Change delta identifies:

```yaml
change_delta:
  trigger: "plans/decisions/3d-rendering-architecture.md"
  trigger_type: decision
  date: "2026-01-28"
  epic: 2

  summary: |
    Decision adopts GPU-driven rendering pipeline with compute shaders
    for culling and draw call generation, replacing the previously planned
    CPU-side rendering approach.

  changes:
    - area: architecture
      what: "Rendering pipeline moves from CPU-driven to GPU-driven"
      before: "CPU submits draw calls directly"
      after: "GPU compute shaders handle culling and indirect draw"
    - area: interfaces
      what: "New IRenderable contract for GPU pipeline"
      before: "Simple draw() method"
      after: "GPU buffer registration + material binding"

  potentially_affected:
    systems:
      - RenderingSystem
      - TerrainRenderingSystem
    components:
      - RenderableComponent
      - MeshComponent
    interfaces:
      - IRenderable
    tickets:
      - "2-003"   # Rendering pipeline setup
      - "2-005"   # Material system
      - "2-007"   # Draw call batching

  scope_estimate: extensive
```

### Phase 2: Impact Assessment

- **Systems Architect** — Impact: HIGH. System boundaries shift; RenderingSystem now owns GPU buffer management. 3 tickets affected.
- **Graphics Engineer** — Impact: HIGH. Pipeline architecture fundamentally changes. Recommends 2 new tickets for compute shader setup and indirect draw buffer.
- **Game Designer** — Impact: LOW. Visual output goals unchanged. Only concern: ensure LOD transitions remain smooth under GPU-driven approach.

All agents report impact → proceed (no short-circuit).

### Phase 3: Targeted Discussion

Graphics Engineer asks Systems Architect:
- RQ001: "Should GPU buffer management be part of RenderingSystem or a separate GPUResourceSystem?"
- Answer: "Keep in RenderingSystem per single-responsibility — it's rendering-specific GPU resources."

Game Designer asks Graphics Engineer:
- RQ002: "Will GPU-driven culling maintain the same LOD distance thresholds?"
- Answer: "Yes, thresholds move to a GPU uniform buffer but values stay the same."

### Phase 4: Ticket Revision

| Ticket | Status | Notes |
|--------|--------|-------|
| 2-001 | UNCHANGED | Window/context setup unaffected |
| 2-002 | UNCHANGED | Camera system unaffected |
| 2-003 | MODIFIED | Pipeline setup now includes compute shader init |
| 2-004 | UNCHANGED | Lighting model unaffected |
| 2-005 | MODIFIED | Material system adds GPU buffer binding |
| 2-006 | UNCHANGED | Texture loading unaffected |
| 2-007 | REMOVED | CPU draw call batching superseded by GPU indirect draw |
| 2-008 | UNCHANGED | Debug rendering unaffected |
| 2-009 | NEW | Compute shader culling pipeline |
| 2-010 | NEW | Indirect draw buffer management |

### Phase 5: Canon Re-Verification

- 2-003 (MODIFIED): Canon compliant after update
- 2-005 (MODIFIED): Canon compliant after update
- 2-009 (NEW): Canon compliant — follows patterns.yaml GPU resource rules
- 2-010 (NEW): Conflict — terminology uses "draw buffer" but canon says "render command buffer"
  - Resolution: Update ticket terminology to match canon

### Phase 6: Decision Capture

No new canon-level decisions needed. Terminology conflict resolved by updating ticket.

### Phase 7: Revision Report

```markdown
# Revision Report: Epic 2 — 2026-01-28

## Trigger
**Document:** plans/decisions/3d-rendering-architecture.md
**Type:** decision
**Summary:** Adopt GPU-driven rendering pipeline with compute shaders

## Impact Summary
| Agent | Impact Level | Key Finding |
|-------|-------------|-------------|
| systems-architect | HIGH | RenderingSystem boundary expands to include GPU buffer management |
| graphics-engineer | HIGH | Pipeline architecture fundamentally changes |
| game-designer | LOW | Visual goals unchanged, LOD thresholds preserved |

## Ticket Changes
- Modified: 2-003, 2-005
- Removed: 2-007
- Added: 2-009, 2-010
- Unchanged: 2-001, 2-002, 2-004, 2-006, 2-008

## Cascading Impact
| Epic | Reason | Recommended Action |
|------|--------|--------------------|
| Epic 3 | TerrainRenderingSystem uses IRenderable | Run /revise-epic 3 --trigger plans/decisions/3d-rendering-architecture.md |
| Epic 12 | UI rendering may share GPU pipeline | Review after Epic 2 finalizes |
```
