# Do Epic

Execute an epic's tickets with proper agent delegation, SA verification, and ClickUp integration.

## Usage

```
/do-epic <epic_number>
```

**Examples:**
```
/do-epic 1                    # Execute Epic 1
/do-epic 2 --dry-run          # Show execution plan without running
```

## When to Use

Use this skill when:
- Starting implementation of a planned epic
- Resuming work on a partially-completed epic
- All epic tickets have been planned via `/plan-epic`

## Prerequisites

Before running, ensure:
1. Epic tickets exist at `/docs/epics/epic-{N}/tickets.md`
2. ClickUp has corresponding tickets synced
3. Previous epic dependencies are complete

---

## Core Principle: Trust But Verify

**Lesson from Epic 0:** Agent-reported "complete" is not the same as "verified complete."

Every ticket goes through this cycle:
```
IMPLEMENT → BUILD → TEST → SA VERIFY → CLICKUP DONE
```

A ticket is ONLY marked done in ClickUp after SA verification passes.

---

## Workflow Overview

```
┌─────────────────────────────────────────────────────────────────┐
│  1. LOAD EPIC                                                    │
│     Read tickets.md, fetch ClickUp status, build dependency graph│
├─────────────────────────────────────────────────────────────────┤
│  2. CREATE TASK QUEUE                                            │
│     Order tickets by dependencies, identify parallelizable groups│
├─────────────────────────────────────────────────────────────────┤
│  3. FOR EACH TICKET (or parallel group):                         │
│     a. SELECT AGENT - Pick best agent for ticket's domain        │
│     b. DELEGATE - Spawn agent with ticket context                │
│     c. BUILD & TEST - Verify compilation and tests pass          │
│     d. SA VERIFY - Systems Architect reviews against criteria    │
│     e. CLICKUP UPDATE - Mark done only after SA passes           │
├─────────────────────────────────────────────────────────────────┤
│  4. EPIC COMPLETE                                                │
│     All tickets done, final integration test, commit             │
└─────────────────────────────────────────────────────────────────┘
```

---

## Phase 1: Load Epic

### Read Epic Tickets

```
/docs/epics/epic-{N}/tickets.md
```

Parse each ticket to extract:
- Ticket ID (e.g., 0-001)
- Title
- Type (feature, infrastructure, task, research)
- System (Application, AssetManager, InputSystem, ECS, Core)
- Acceptance Criteria (checkbox list)
- Dependencies (Blocked by, Blocks)

### Sync with ClickUp

Fetch current ticket status from ClickUp:
- Identify tickets already marked "done"
- Identify tickets "in progress"
- Identify tickets "to do"

Skip tickets already verified as done.

### Build Dependency Graph

```
Example for Epic 0:
0-001 (Build)
  └─> 0-013 (Logging)
  └─> 0-025 (ECS Framework)
  └─> 0-002 (SDL Init)
        └─> 0-003 (Window)
              └─> 0-004 (Game Loop)
```

---

## Phase 2: Create Task Queue

### Ordering Rules

1. **Respect dependencies** - Never start a ticket before its blockers are done
2. **Parallelize independent tickets** - Launch multiple agents when no dependencies
3. **Infrastructure first** - Build, logging, config before features
4. **Core before extensions** - Base systems before advanced features

### Parallelization Strategy

Group tickets with no inter-dependencies for parallel execution:

```yaml
# Example parallel groups for Epic 0
wave_1:
  - 0-001  # Build (no deps)

wave_2:  # All depend only on 0-001
  - 0-013  # Logging
  - 0-025  # ECS Framework
  - 0-014  # Config
  - 0-029  # Testing
  - 0-021  # RNG
  - 0-026  # Core Types

wave_3:  # Depend on wave_2
  - 0-002  # SDL Init (needs logging)
  # ... etc
```

---

## Phase 3: Execute Tickets

### Step A: Select Agent

Map ticket system/type to best agent profile:

| System | Primary Agent | Profile |
|--------|---------------|---------|
| Build | Engine Developer | `/agents/engine-developer.md` |
| Application | Engine Developer | `/agents/engine-developer.md` |
| AssetManager | Engine Developer | `/agents/engine-developer.md` |
| InputSystem | Engine Developer | `/agents/engine-developer.md` |
| ECS | ECS Architect | `/agents/ecs-architect.md` |
| Core | Engine Developer | `/agents/engine-developer.md` |
| Rendering | Graphics Engineer | `/agents/graphics-engineer.md` |
| UI | UI Developer | `/agents/ui-developer.md` |
| Audio | Audio Engineer | `/agents/audio-engineer.md` |
| Testing | QA Engineer | `/agents/qa-engineer.md` |
| Zone | Zone Engineer | `/agents/zone-engineer.md` |
| Building | Building Engineer | `/agents/building-engineer.md` |
| Services | Services Engineer | `/agents/services-engineer.md` |
| Economy | Economy Engineer | `/agents/economy-engineer.md` |
| Infrastructure | Infrastructure Engineer | `/agents/infrastructure-engineer.md` |
| Population | Population Engineer | `/agents/population-engineer.md` |

### Step B: Delegate Implementation

Spawn implementation agent with this prompt template:

```
## Ticket: {TICKET_ID} - {TITLE}

**Agent Profile:** Read and follow /agents/{agent-profile}.md

**Type:** {TYPE}
**System:** {SYSTEM}

### Description
{DESCRIPTION}

### Acceptance Criteria
{ACCEPTANCE_CRITERIA_LIST}

### Context Files to Read First
- {List relevant existing files}
- /docs/canon/patterns.yaml (for implementation patterns)

### Files to Create/Modify
- {Expected files based on description}

### Requirements
1. Follow project patterns from canon
2. Use sims3000:: namespace
3. Ensure code compiles with no warnings
4. Add to CMakeLists.txt if creating new files

### Success Criteria
All acceptance criteria checkboxes must be satisfiable by your implementation.

### Build Command
```
CMAKE="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
"$CMAKE" --build build --config Release
```

Report what you implemented and which acceptance criteria are now met.
```

### Step C: Build & Test

After agent reports complete:

```bash
# Build
"$CMAKE" --build build --config Release

# Run tests
"$CMAKE" --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

**If build fails:** Spawn fix agent with error output.
**If tests fail:** Spawn fix agent with test output.

### Step D: SA Verification

**CRITICAL STEP - DO NOT SKIP**

Spawn Systems Architect to verify implementation against acceptance criteria:

```
## SA Verification: Ticket {TICKET_ID}

**Agent Profile:** Read and follow /agents/systems-architect.md

### Acceptance Criteria to Verify
{ACCEPTANCE_CRITERIA_LIST}

### Implementation Files
{List of files created/modified by implementation agent}

### Your Task
For EACH acceptance criterion:
1. Read the relevant code
2. Determine if the criterion is FULLY satisfied
3. If NOT satisfied, explain what's missing

### Output Format
```
## Verification: {TICKET_ID}

### Criteria Status

- [ ] Criterion 1: {PASS/FAIL} - {explanation if fail}
- [ ] Criterion 2: {PASS/FAIL} - {explanation if fail}
...

### Verdict: {PASS/FAIL}

### Issues (if FAIL)
1. {Issue description}
2. {Issue description}
```

Be STRICT. Check EXACT wording of criteria.
- "X exists" means X must exist
- "X does Y" means X must actually do Y, not just have a stub
- "X uses Y" means X must actually use Y
```

**If SA returns FAIL:**
1. Spawn fix agent with SA's issues
2. Re-run build & test
3. Re-run SA verification
4. Repeat until PASS

### Step E: ClickUp Update

**ONLY after SA verification PASSES:**

```bash
# Update ticket status in ClickUp to "done"
# (via MCP ClickUp tools)
```

---

## Phase 4: Epic Complete

After all tickets pass SA verification:

### Final Integration Test

Run the epic's integration test (if defined):
- Epic 0: Ticket 0-033 (Integration Smoke Test)
- Other epics: Check for integration ticket

### Commit Changes

```bash
git add -A
git commit -m "Complete Epic {N}: {Epic Name}

- {Summary of major features}
- All {X} tickets implemented and verified
- Build passing, all tests passing
"
```

### Update Epic Status

Mark epic as complete in project tracking.

---

## Failure Handling

### Build Failure

```
1. Capture error output
2. Spawn fix agent with error context
3. Fix agent corrects issues
4. Re-run build
5. If still failing after 3 attempts, STOP and report to user
```

### Test Failure

```
1. Capture failing test output
2. Spawn fix agent with test context
3. Fix agent corrects issues
4. Re-run tests
5. If still failing after 3 attempts, STOP and report to user
```

### SA Verification Failure

```
1. Parse SA's issue list
2. Spawn implementation agent with specific issues to fix
3. Re-run build & test
4. Re-run SA verification
5. If still failing after 3 attempts, STOP and report to user
```

### Max Retries Exceeded

When any phase fails 3 times:
1. Save current progress
2. Report to user with:
   - What was attempted
   - What failed
   - Last error/issue
   - Suggested manual intervention
3. Wait for user guidance

---

## Lessons from Epic 0

### Lesson 1: Existence != Compliance

**Wrong:** "PositionComponent exists" ✓
**Right:** "PositionComponent has elevation field, serialize/deserialize methods, matches spec" ✓

SA must verify EXACT acceptance criteria, not just file existence.

### Lesson 2: Build After Every Ticket

Don't batch multiple tickets before building. Build after each ticket to catch issues early.

### Lesson 3: Tests Must Pass

Don't skip test runs. Failing tests indicate broken implementation.

### Lesson 4: SA Verification is Mandatory

Never mark a ticket done without SA verification. The implementation agent's "done" is just a proposal.

### Lesson 5: Parallel Agents for Independent Work

When tickets have no dependencies on each other, spawn agents in parallel to maximize throughput.

### Lesson 6: Stop and Ask When Stuck

After 3 failed attempts, stop and consult user rather than spinning indefinitely.

### Lesson 7: ClickUp is Source of Truth for Status

Always check ClickUp before starting work. Don't re-implement tickets already done.

---

## Progress Tracking

Create/update progress file during execution:

```
/docs/epics/epic-{N}/progress.md
```

Format:
```markdown
# Epic {N} Progress

Started: {timestamp}
Last Updated: {timestamp}

## Tickets

| ID | Title | Status | Agent | SA Verified | ClickUp |
|----|-------|--------|-------|-------------|---------|
| 0-001 | Build Infrastructure | DONE | engine-dev | PASS | done |
| 0-002 | SDL Init | IN_PROGRESS | engine-dev | - | in progress |
| 0-003 | Window | QUEUED | engine-dev | - | to do |

## Current Wave
Executing: 0-002, 0-013, 0-025 (parallel)

## Issues Encountered
- 0-002: Build failed on first attempt, fixed missing include
- ...
```

---

## Example Execution

```
User: /do-epic 1

Orchestrator:
1. Loading Epic 1: Multiplayer Networking
2. Found 28 tickets, 0 already done in ClickUp
3. Building dependency graph...
4. Created 6 execution waves

Wave 1 (parallel):
  - 1-001: Network Protocol Definition → Engine Developer
  - 1-002: Message Types → Engine Developer

[Spawns agents in parallel]

Agent 1-001 reports: Protocol defined in include/sims3000/network/Protocol.h
Agent 1-002 reports: Message types in include/sims3000/network/Messages.h

Building... SUCCESS
Testing... 2/2 PASSED

SA Verification 1-001... PASS
SA Verification 1-002... PASS

ClickUp: 1-001 → done
ClickUp: 1-002 → done

Wave 2 (parallel):
  - 1-003: Client Connection → Engine Developer
  - 1-004: Server Listener → Engine Developer
  ...

[Continues through all waves]

Epic 1 Complete!
- 28 tickets implemented
- 28 SA verifications passed
- All tests passing
- Committed to git
```

---

## Configuration

Optional configuration in `/docs/epics/epic-{N}/do-epic-config.yaml`:

```yaml
# Override default agent assignments
agent_overrides:
  "1-015": "qa-engineer"  # This ticket needs QA focus

# Max parallel agents (default: 4)
max_parallel: 6

# Max retry attempts (default: 3)
max_retries: 5

# Skip SA verification (NOT RECOMMENDED)
skip_sa_verification: false

# Dry run mode
dry_run: false
```

---

## Output Artifacts

After `/do-epic N` completes:

```
docs/epics/epic-{N}/
├── progress.md           # Execution progress log
├── verification/
│   ├── {N}-001-sa.md     # SA verification report for each ticket
│   ├── {N}-002-sa.md
│   └── ...
└── completion-report.md  # Final summary
```
