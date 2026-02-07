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
│     Create or resume from queue.yaml                             │
├─────────────────────────────────────────────────────────────────┤
│  2. CREATE TASK QUEUE                                            │
│     Order tickets by dependencies, write to queue.yaml           │
├─────────────────────────────────────────────────────────────────┤
│  3. FOR EACH TICKET (or parallel group):                         │
│     a. UPDATE QUEUE - Set status: in_progress                    │
│     b. SELECT AGENT - Pick best agent for ticket's domain        │
│     c. PRE-FLIGHT - Agent verifies work can be completed fully   │
│     d. DELEGATE - Agent implements (only after pre-flight passes)│
│        ├─ If ESCALATE: Spawn SA to answer, return to agent       │
│        └─ Agent continues with SA's guidance                     │
│     e. UPDATE QUEUE - Set status: implemented                    │
│     f. BUILD & TEST - Verify compilation and tests pass          │
│     g. UPDATE QUEUE - Set status: sa_review                      │
│     h. SA VERIFY - Systems Architect reviews against criteria    │
│     i. UPDATE QUEUE - Set status: complete (only if SA passes)   │
│     j. CLICKUP UPDATE - Mark done only after SA passes           │
├─────────────────────────────────────────────────────────────────┤
│  4. EPIC COMPLETE                                                │
│     All tickets done, final integration test, update queue.yaml  │
└─────────────────────────────────────────────────────────────────┘
```

**Queue file location:** `/docs/epics/epic-{N}/queue.yaml`

The queue file is updated at every step so you can always see exactly where execution is.

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

### Create Queue File

**Location:** `/docs/epics/epic-{N}/queue.yaml`

This file is the **source of truth** for execution progress. It persists across sessions and shows exactly where we are in the epic.

```yaml
# Epic {N} Execution Queue
# Generated: {timestamp}
# Last Updated: {timestamp}

epic: {N}
name: "{Epic Name}"
status: in_progress  # pending | in_progress | complete

# Current position in queue
current_index: 3
current_ticket: "0-004"

# Execution queue (dependency-ordered)
queue:
  - id: "0-001"
    title: "Project Build Infrastructure"
    system: "Build"
    agent: "engine-developer"
    status: complete        # pending | in_progress | implemented | sa_review | complete | failed
    implemented_at: "2026-02-05T10:30:00Z"
    sa_verified_at: "2026-02-05T10:35:00Z"
    clickup_updated: true

  - id: "0-002"
    title: "SDL3 Initialization"
    system: "Application"
    agent: "engine-developer"
    status: complete
    implemented_at: "2026-02-05T10:45:00Z"
    sa_verified_at: "2026-02-05T10:50:00Z"
    clickup_updated: true

  - id: "0-003"
    title: "Window Management"
    system: "Application"
    agent: "engine-developer"
    status: sa_review       # Currently being reviewed by SA
    implemented_at: "2026-02-05T11:00:00Z"
    sa_verified_at: null
    clickup_updated: false

  - id: "0-004"
    title: "Main Game Loop"
    system: "Application"
    agent: "engine-developer"
    status: in_progress     # Currently being implemented
    implemented_at: null
    sa_verified_at: null
    clickup_updated: false

  - id: "0-005"
    title: "Application State Management"
    system: "Application"
    agent: "engine-developer"
    status: pending         # Waiting for dependencies
    blocked_by: ["0-004"]
    implemented_at: null
    sa_verified_at: null
    clickup_updated: false

# Summary stats
stats:
  total: 34
  complete: 2
  in_progress: 2
  pending: 30
  failed: 0
```

### Queue Status Values

| Status | Meaning |
|--------|---------|
| `pending` | Not started, may be blocked by dependencies |
| `in_progress` | Agent currently implementing |
| `blocked` | Pre-flight failed, missing dependency or blocker found |
| `implemented` | Agent reported done, awaiting SA review |
| `sa_review` | SA currently verifying |
| `complete` | SA verified, ClickUp updated |
| `failed` | Failed after max retries, needs user intervention |

### Resuming from Queue

When `/do-epic` is invoked:
1. Check if `queue.yaml` exists for this epic
2. If exists, resume from `current_index`
3. If not, create fresh queue from tickets.md

This allows stopping and resuming epic execution across sessions.

---

## Phase 2: Create Task Queue

### Ordering Rules

1. **Respect dependencies** - Never start a ticket before its blockers are done
2. **Parallelize independent tickets** - Launch multiple agents when no dependencies
3. **Infrastructure first** - Build, logging, config before features
4. **Core before extensions** - Base systems before advanced features

### Parallelization Strategy

**Maximum 3 tickets in parallel at any time.**

This limit ensures:
- Questions can be batched efficiently for SA
- Build verification doesn't conflict
- Manageable complexity for orchestration

Group tickets with no inter-dependencies for parallel execution:

```yaml
# Example parallel groups for Epic 0 (max 3 per wave)
wave_1:
  - 0-001  # Build (no deps)

wave_2a:  # All depend only on 0-001 (first batch of 3)
  - 0-013  # Logging
  - 0-025  # ECS Framework
  - 0-014  # Config

wave_2b:  # Same deps, next batch of 3
  - 0-029  # Testing
  - 0-021  # RNG
  - 0-026  # Core Types

wave_3:  # Depend on wave_2
  - 0-002  # SDL Init (needs logging)
  # ... etc
```

When spawning parallel tickets, launch up to 3 agents simultaneously using multiple Task tool calls in a single message.

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

Spawn implementation agent with the `/implement-ticket` skill:

```
## Ticket: {TICKET_ID} - {TITLE}

**Follow skill:** /implement-ticket
**Agent Profile:** /agents/{agent-profile}.md

**Epic:** {EPIC_NUMBER}
**Type:** {TYPE}
**System:** {SYSTEM}

### Description
{DESCRIPTION}

### Acceptance Criteria
{ACCEPTANCE_CRITERIA_LIST}

### Context Files
- {List relevant existing files}
- /docs/canon/patterns.yaml

### Build Command
CMAKE="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
"$CMAKE" --build build --config Release
```

The agent will follow the `/implement-ticket` skill workflow:
1. **Pre-flight check** - Verify work can be completed
2. **Implementation** - Write code following patterns
3. **Self-verify** - Check against acceptance criteria
4. **Report** - Return structured report

See `/.claude/skills/implement-ticket/SKILL.md` for full workflow.

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

### Step E: Update Queue & ClickUp

**ONLY after SA verification PASSES:**

1. Update queue.yaml:
   ```yaml
   - id: "{TICKET_ID}"
     status: complete
     sa_verified_at: "{timestamp}"
     clickup_updated: true
   ```

2. Update ClickUp ticket status to "done" (via MCP ClickUp tools)

3. Increment `current_index` in queue.yaml to next ticket

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

## Escalation Handling

When a sub-agent encounters an issue it cannot resolve, it will send an `ESCALATE` request.

### Parallel Execution

**Maximum 3 tickets run in parallel.** This allows:
- Efficient use of agent resources
- Batched SA answers for questions
- Manageable complexity

### Questions File

All escalations are collected in a single YAML file per epic:

**Location:** `/docs/epics/epic-{N}/questions.yaml`

```yaml
# Epic {N} Implementation Questions
# This file collects questions from sub-agents for SA to batch answer

epic: {N}
created: "{timestamp}"
last_updated: "{timestamp}"

questions:
  - id: Q001
    ticket: "0-007"
    agent: "engine-developer"
    status: open          # open | answered | resolved
    timestamp: "2026-02-05T10:30:00Z"
    type: CLARIFICATION   # QUESTION | BLOCKER | CLARIFICATION | CONFLICT
    question: |
      The acceptance criterion says "handles errors gracefully" but doesn't
      specify what errors or what graceful means. Should I:
      A) Log and return nullptr
      B) Throw an exception
      C) Return an error code
    context: |
      Implementing AsyncLoader::processRequest() for ticket 0-007.
      Current codebase uses mixed patterns for error handling.
    answer: null
    answered_at: null

  - id: Q002
    ticket: "0-013"
    agent: "engine-developer"
    status: answered
    timestamp: "2026-02-05T10:32:00Z"
    type: QUESTION
    question: |
      Should the Logger write to file synchronously or buffer writes?
    context: |
      Implementing file logging for ticket 0-013.
    answer: |
      Buffer writes with flush on ERROR/FATAL or every 100 entries.
      This balances performance with reliability. See patterns.yaml
      section on "logging" for reference implementation.
    answered_at: "2026-02-05T10:45:00Z"

  - id: Q003
    ticket: "0-017"
    agent: "engine-developer"
    status: open
    timestamp: "2026-02-05T10:35:00Z"
    type: CONFLICT
    question: |
      InputSystem.h uses SDL scancodes but ActionMapping uses keycodes.
      Which should I use for mouse button tracking?
    context: |
      Adding mouse edge detection to InputSystem.
    answer: null
    answered_at: null
```

### Escalation Flow (Batched)

```
┌─────────────┐     ESCALATE      ┌─────────────────┐
│ Sub-Agent 1 │ ──────────────────│                 │
└─────────────┘                   │                 │
                                  │  Top-Level      │──── Write to ────▶ questions.yaml
┌─────────────┐     ESCALATE      │  Agent          │
│ Sub-Agent 2 │ ──────────────────│                 │
└─────────────┘                   │                 │
                                  └────────┬────────┘
                                           │
                                           │ Batch spawn SA
                                           │ (when questions pending)
                                           ▼
                                  ┌─────────────────┐
                                  │    Systems      │──── Read questions.yaml
                                  │   Architect     │──── Write answers
                                  └────────┬────────┘
                                           │
                                           │ Answers written to file
                                           ▼
┌─────────────┐                   ┌─────────────────┐
│ Sub-Agents  │ ◄──── Resume ─────│  Top-Level      │──── Read answers
│ continue    │   with answers    │  Agent          │
└─────────────┘                   └─────────────────┘
```

### Handling Escalations

When you receive an `ESCALATE` from a sub-agent:

1. **Add question to questions.yaml**
   - Generate next question ID (Q001, Q002, etc.)
   - Record ticket, agent, type, question, context
   - Set status: open

2. **Check for pending questions periodically**
   - After each sub-agent completes or escalates
   - If there are open questions, spawn SA to batch answer

3. **Spawn Systems Architect for batch answers**
   ```
   ## SA Batch Consultation: Epic {N}

   **Questions File:** /docs/epics/epic-{N}/questions.yaml

   **Your Task:**
   1. Read all questions with status: open
   2. For each question, provide a clear, actionable answer
   3. Update the question's `answer` field in the YAML
   4. Set `answered_at` timestamp
   5. Set status: answered

   Reference specific patterns, files, or canon decisions where applicable.
   Be concise but complete - the implementing agent needs to continue work.
   ```

4. **Return answers to waiting sub-agents**
   - Read answered questions from questions.yaml
   - Resume each sub-agent with their answer
   - Sub-agent marks question as `resolved` when work continues

5. **Update queue.yaml**
   - Log that escalation occurred
   - Track which questions were asked

### Escalation Types

| Type | When Used | SA Should Provide |
|------|-----------|-------------------|
| QUESTION | Agent needs information | Direct answer with references |
| BLOCKER | Something prevents progress | Solution or workaround |
| CLARIFICATION | Requirement is ambiguous | Clear interpretation |
| CONFLICT | Patterns/requirements conflict | Which to follow and why |

### Do NOT Escalate to User

Sub-agent escalations go to SA, not to the user. Only escalate to user when:
- SA cannot answer (truly novel decision)
- Max retries exceeded
- User intervention explicitly needed

---

## User Communication

**Do NOT report progress to user during epic execution.**

The user will be notified only:
1. **At epic completion** - Full summary of what was done
2. **When user input is required** - Decisions SA cannot make

The queue.yaml file provides progress visibility if the user wants to check.

### What Requires User Input

- Novel architectural decisions not covered by canon
- Conflicting requirements that SA cannot resolve
- External blockers (missing assets, credentials, etc.)
- Max retries exceeded on a ticket

### What Does NOT Require User Input

- Questions SA can answer
- Build failures (fix and retry)
- SA verification failures (fix and retry)
- Dependency ordering issues (reorder queue)

---

## Failure Handling

### Pre-Flight Failure

When an agent's pre-flight check fails:

```
1. Agent reports blocker (missing dependency, type, interface, etc.)
2. Update queue.yaml: status → "blocked"
3. Add blocker details to queue entry
4. Check if blocker is another ticket in the queue
   - If yes: ensure that ticket runs first (reorder if needed)
   - If no: report to user for guidance
5. Move to next unblocked ticket
```

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

### Lesson 8: Pre-Flight Before Implementation

Agents must verify they can complete work BEFORE starting. Check:
- Dependencies exist
- Required types/interfaces available
- No blockers in codebase

This catches issues early instead of discovering them mid-implementation.

### Lesson 9: Escalate Don't Guess

When sub-agents encounter issues:
- Don't guess at the answer
- Don't make assumptions
- Don't implement workarounds without guidance

Instead: Escalate to top-level agent → SA provides authoritative answer → continue with guidance.

This prevents bad assumptions from propagating through the codebase.

---

## Progress Tracking

The queue file is the single source of truth for progress:

```
/docs/epics/epic-{N}/queue.yaml
```

Check progress at any time by reading this file. Key fields:

```yaml
current_index: 5          # We're on the 6th ticket (0-indexed)
current_ticket: "0-006"   # Current ticket ID

stats:
  total: 34               # Total tickets in epic
  complete: 5             # SA-verified and done
  in_progress: 1          # Currently being worked
  pending: 28             # Not yet started
  failed: 0               # Failed and need intervention
```

The queue persists across sessions, so you can stop and resume at any time.

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
├── queue.yaml            # Execution queue (source of truth for progress)
├── questions.yaml        # All escalations and SA answers
├── verification/
│   ├── {N}-001-sa.md     # SA verification report for each ticket
│   ├── {N}-002-sa.md
│   └── ...
└── completion-report.md  # Final summary
```

### Reading the Queue

To check progress at any time, read `/docs/epics/epic-{N}/queue.yaml`:

- `current_index` - Which ticket we're on
- `current_ticket` - Current ticket ID
- `queue[].status` - Status of each ticket
- `stats` - Summary counts

### Reading Questions

To see all escalations and answers, read `/docs/epics/epic-{N}/questions.yaml`:

- `questions[].status` - open / answered / resolved
- `questions[].question` - What was asked
- `questions[].answer` - SA's response (if answered)
