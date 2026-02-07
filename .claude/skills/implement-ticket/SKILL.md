# Implement Ticket

Standard workflow for sub-agents implementing epic tickets. Invoked by the top-level agent during `/do-epic` execution.

## Usage

This skill is invoked by the top-level agent, not directly by users.

```
/implement-ticket <ticket-id>
```

**Context provided by top-level agent:**
- Ticket ID
- Epic number
- Ticket details (title, description, acceptance criteria)
- Agent profile to follow
- Relevant context files

---

## Workflow

```
┌─────────────────────────────────────────────────────────────────┐
│  1. PRE-FLIGHT CHECK                                             │
│     Verify all dependencies exist and work can be completed      │
├─────────────────────────────────────────────────────────────────┤
│  2. IMPLEMENTATION                                               │
│     Write code following patterns and agent profile              │
├─────────────────────────────────────────────────────────────────┤
│  3. TESTING                                                      │
│     Write unit/integration tests to prove implementation works   │
├─────────────────────────────────────────────────────────────────┤
│  4. SELF-VERIFY                                                  │
│     Check implementation against acceptance criteria             │
├─────────────────────────────────────────────────────────────────┤
│  5. REPORT                                                       │
│     Return structured report to top-level agent                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Phase 1: Pre-Flight Check

**MANDATORY - Do not skip this phase.**

Before writing any code, verify you can complete the ticket entirely.

### Checklist

1. **Read context files**
   - Read all files listed in the ticket context
   - Understand existing patterns and conventions

2. **Check dependencies**
   - [ ] Required types exist (check header files)
   - [ ] Required interfaces exist
   - [ ] Base classes exist (if extending)
   - [ ] Required libraries available (check vcpkg.json, CMakeLists.txt)

3. **Check acceptance criteria feasibility**
   - [ ] Each criterion is achievable with current codebase
   - [ ] No criterion requires work from another unfinished ticket
   - [ ] No criterion is ambiguous or contradictory

4. **Platform/subsystem constraints**
   - [ ] SDL_GPU and SDL_Renderer cannot coexist (use SDL_GPU for all rendering including UI)
   - [ ] SDL3 subsystem availability verified (SDL_INIT_VIDEO, SDL_INIT_AUDIO, etc.)
   - [ ] Component design follows canon: trivially copyable, no pointers, entity IDs for references
   - [ ] Widget base classes exist (if creating new UI widgets)

5. **Identify files to create/modify**
   - List all files you'll need to touch
   - Verify parent directories exist
   - Check you won't conflict with other pending work

### Pre-Flight Output

If ALL checks pass:
```
PRE-FLIGHT: PASS
Ready to implement ticket {TICKET_ID}
Files to create: [list]
Files to modify: [list]
```

If ANY check fails:
```
PRE-FLIGHT: BLOCKED

Blocker: {description}
Missing: {what's missing}
Dependency: {ticket ID if known, or "unknown"}

Recommendation: {what needs to happen first}
```

**If blocked: STOP HERE. Do not attempt partial implementation.**

---

## Phase 2: Implementation

Only proceed after pre-flight passes.

### Guidelines

1. **Follow agent profile**
   - Read `/agents/{your-profile}.md`
   - Apply domain-specific patterns

2. **Follow project conventions**
   - Namespace: `sims3000::`
   - File locations per canon structure
   - Patterns from `/docs/canon/patterns.yaml`

3. **Write complete implementations**
   - No TODOs or placeholders
   - No stub implementations that "will be filled later"
   - Every function must do what it claims

4. **Update build system**
   - Add new source files to CMakeLists.txt
   - Add new dependencies to vcpkg.json if needed

5. **Keep scope tight**
   - Only implement what the ticket asks for
   - Don't add extra features "while you're there"
   - Don't refactor unrelated code

6. **SDL3/SDL_GPU patterns**
   - Check all SDL function return values (nullptr, negative codes)
   - Log `SDL_GetError()` on failure
   - Document resource ownership and cleanup order in header comments
   - No blocking I/O in game loop (async loading for large assets)

7. **UI constraints**
   - UI reads simulation state, never calls simulation directly
   - Reference animation timings from `/docs/canon/patterns.yaml`
   - Reference color palette from `/docs/canon/patterns.yaml`

8. **For bug-fix tickets**
   - Make minimal, targeted changes
   - Verify root cause is fixed, not just symptoms
   - Don't fix "other bugs noticed along the way" without proper investigation

### Code Quality

- Compiles with no warnings
- Follows existing code style
- Includes necessary includes
- No memory leaks (RAII patterns)
- Thread-safe where applicable
- EnTT: Handle deferred destruction; don't modify views during iteration
- Components: Use `static_assert(sizeof(Component) == N)` for size constraints
- Audio: Handle missing audio device gracefully (code must not crash)
- GPU: Clean up resources (SDL_ReleaseGPUTexture, SDL_ReleaseGPUBuffer, etc.)

---

## Phase 3: Testing

**Every ticket requires tests to prove the implementation works.**

### Test Requirements

1. **Write tests that verify acceptance criteria**
   - Each criterion should have at least one test
   - Tests should fail if the criterion isn't met

2. **Test types (in order of preference)**
   - **Unit tests** - Test individual functions/classes in isolation
   - **Integration tests** - Test components working together
   - **Manual testing** - Only when automated testing isn't feasible

3. **Test location**
   - Unit tests: `tests/{system}/test_{component}.cpp`
   - Integration tests: `tests/integration/test_{feature}.cpp`
   - Add to `tests/CMakeLists.txt`

4. **Test quality**
   - Naming: `test_{Component}_{Scenario}_{ExpectedOutcome}`
   - Isolation: Tests must not depend on other tests or global state
   - Determinism: Run tests multiple times to verify no flaky behavior
   - Cleanup: Release all test resources (files, handles, allocations)

### When Automated Tests Aren't Possible

Some things can't be easily tested automatically:
- Window creation/rendering (requires display)
- Audio playback (requires audio device)
- Input handling (requires user interaction)
- GPU operations (requires graphics context)

For these cases:
1. Document what manual testing is required
2. Describe the expected behavior
3. List the steps to manually verify

### Test Output in Report

```markdown
## Tests Written
- `tests/core/test_interpolatable.cpp` - 8 test cases
  - Construction (default, value, two-value)
  - rotateTick behavior
  - lerp interpolation
  - Integer type handling

## Test Results
- All tests pass: YES
- Test count: 8
- Manual testing required: NO

## Manual Testing (if applicable)
- Required for: Window resize handling
- Steps:
  1. Launch application
  2. Drag window corner to resize
  3. Verify no flickering or artifacts
- Expected: Smooth resize, content scales correctly
```

### No Tests = No Complete

A ticket is NOT complete without tests (automated or documented manual).

If you cannot write tests:
1. ESCALATE to explain why
2. Get SA approval for manual-only testing
3. Document manual test procedure thoroughly

---

## Phase 4: Self-Verify

Before reporting done, verify your implementation AND tests.

### For each acceptance criterion:

1. Read the criterion exactly as written
2. Find the code that satisfies it
3. Verify it actually works, not just exists

### Common mistakes to avoid:

| Criterion says | Wrong | Right |
|----------------|-------|-------|
| "X exists" | File exists but is empty | X is fully implemented |
| "X does Y" | X has method for Y but it's a stub | X actually does Y |
| "X handles Z" | X mentions Z in comments | X has code that handles Z |
| "X with A, B, C" | X has A and B | X has A, B, AND C |

### Build & Runtime Verification

- [ ] Code compiles with no warnings
- [ ] DLLs copied to exe directory (Windows: lz4.dll, SDL3.dll, etc.)
- [ ] Exe runs without crash: `powershell.exe -Command "& '<path>'"`
- [ ] Existing tests still pass (no regressions)
- [ ] New tests pass

### Domain-Specific Verification (if applicable)

- [ ] GPU resources: Shader compiles, textures create, render passes complete
- [ ] Audio: Graceful degradation when device unavailable
- [ ] Persistence: Handles corrupt/invalid data gracefully
- [ ] File writes: Uses atomic operations (write temp, rename)

### Self-Verify Output

```
SELF-VERIFY: {PASS/FAIL}

Criteria Status:
- [x] Criterion 1: {how it's satisfied}
- [x] Criterion 2: {how it's satisfied}
- [ ] Criterion 3: {why it failed / what's missing}
```

If self-verify fails, fix the issues before reporting.

---

## Phase 5: Report

Return a structured report to the top-level agent.

### Report Format

```markdown
# Implementation Report: {TICKET_ID}

## Status: {COMPLETE / BLOCKED / ESCALATE / FAILED}

## Pre-Flight
{PASS / BLOCKED with details}

## Files Created
- `path/to/new/file.h` - {brief description}
- `path/to/new/file.cpp` - {brief description}

## Files Modified
- `path/to/existing/file.h` - {what changed}
- `CMakeLists.txt` - Added new source files

## Implementation Summary
{2-3 sentences describing what was built}

## Tests Written
- `tests/{system}/test_{name}.cpp` - {N} test cases
  - {Test case 1 description}
  - {Test case 2 description}

## Test Results
- All tests pass: {YES/NO}
- Test count: {N}
- Manual testing required: {YES/NO}

## Manual Testing (if required)
- Required for: {what couldn't be automated - SDL3 window/rendering, audio playback, visual verification}
- Run command: `powershell.exe -Command "& 'C:/path/to/exe.exe'"`
- Steps:
  1. {step 1}
  2. {step 2}
- Expected visual/audio outcome: {specific description of what should happen}

## Acceptance Criteria Verification
- [x] Criterion 1: Implemented in `ClassName::method()`, tested in `test_xxx`
- [x] Criterion 2: Added to `file.h` lines 45-60, tested in `test_yyy`
- [x] Criterion 3: Requires manual verification (see above)

## Build Verification
- [ ] Code compiles: {YES/NO}
- [ ] No warnings: {YES/NO}
- [ ] All tests pass: {YES/NO}

## Notes
{Any important context for SA review}
```

---

## Handling Issues

### Escalation to Top-Level Agent

**When you encounter a problem you cannot resolve yourself, escalate to the top-level agent.**

Do NOT:
- Guess at the answer
- Make assumptions
- Implement a workaround without guidance
- Skip the problematic part

DO:
- Stop work
- Report the issue with an `ESCALATE` request
- Wait for the answer
- Continue work with the provided guidance

### Escalation Format

```markdown
# ESCALATE: {TICKET_ID}

## Issue Type: {QUESTION / BLOCKER / CLARIFICATION / CONFLICT}

## Description
{Clear description of the problem}

## Context
- What I was implementing: {description}
- Where I got stuck: {specific point}
- What I need to know: {specific question}

## Options Considered (if applicable)
A) {Option A}
B) {Option B}

## Awaiting Response
{true}
```

The top-level agent will:
1. Receive your escalation
2. Spawn the **Systems Architect** to answer
3. Return the SA's answer to you
4. You continue implementation with that guidance

### When to Escalate

| Situation | Escalate? | Example |
|-----------|-----------|---------|
| Missing dependency (another ticket) | No - report BLOCKED | "Needs Interpolatable<T> from 0-007" |
| Ambiguous requirement | **Yes** | "What does 'handles gracefully' mean?" |
| Conflicting patterns in codebase | **Yes** | "Two different error patterns exist, which to use?" |
| Architectural decision needed | **Yes** | "Should this be sync or async?" |
| Don't know where file should go | **Yes** | "Should this be in core/ or app/?" |
| Build error I can't fix | **Yes** | "Getting linker error, tried X and Y" |
| Acceptance criterion seems wrong | **Yes** | "Criterion says X but that conflicts with Y" |

### Missing Dependency

If blocked by another ticket's work:

```
Status: BLOCKED
Blocker: Missing ClassName from ticket 0-015
Cannot proceed until 0-015 is complete.
```

This is NOT an escalation - it's a dependency that needs to be resolved by completing the other ticket first.

### Build Failure

If your code doesn't compile:
1. Try to fix the compile errors yourself
2. If you can't fix after reasonable effort, ESCALATE
3. Only report COMPLETE when it compiles

Do NOT report COMPLETE with compile errors.

### Partial Completion

**Never report partial completion.**

Either:
- Complete ALL acceptance criteria → Status: COMPLETE
- Cannot complete due to blocker → Status: BLOCKED
- Need guidance → Status: ESCALATE
- Tried and failed → Status: FAILED

---

## Examples

### Successful Implementation

```markdown
# Implementation Report: 0-007

## Status: COMPLETE

## Pre-Flight
PASS - All dependencies available

## Files Created
- `include/sims3000/core/Interpolatable.h` - Template class for smooth interpolation
- `tests/core/test_interpolatable.cpp` - Unit tests for interpolation

## Files Modified
- `CMakeLists.txt` - Added header to SIMS3000_HEADERS
- `tests/CMakeLists.txt` - Added test executable

## Implementation Summary
Created Interpolatable<T> template that stores previous and current values for
lerping between simulation ticks. Supports float, vec2, vec3, vec4 via glm::mix.

## Tests Written
- `tests/core/test_interpolatable.cpp` - 8 test cases
  - Default construction
  - Value construction
  - rotateTick behavior
  - lerp with alpha 0, 0.5, 1.0
  - Integer type handling (no interpolation)
  - Vec3 interpolation

## Test Results
- All tests pass: YES
- Test count: 8
- Manual testing required: NO

## Acceptance Criteria Verification
- [x] Interpolatable<T> template provides previous/current state storage: Lines 25-40, tested in test_construction
- [x] Pre-tick hook rotates buffers: rotateTick() method line 83, tested in test_rotate_tick
- [x] Linear interpolation helper: lerp(alpha) method line 136, tested in test_lerp_*
- [x] Pattern documented: Header comments lines 1-20

## Build Verification
- [x] Code compiles: YES
- [x] No warnings: YES
- [x] All tests pass: YES

## Notes
Used glm::mix for type-generic interpolation. Integer types skip interpolation.
```

### Blocked Implementation

```markdown
# Implementation Report: 0-023

## Status: BLOCKED

## Pre-Flight
BLOCKED

Blocker: PositionComponent requires Interpolatable<T> template
Missing: include/sims3000/core/Interpolatable.h
Dependency: Ticket 0-007

## Recommendation
Complete ticket 0-007 first, then retry 0-023.
```

---

## Integration with Top-Level Agent

The top-level agent will:

1. Invoke this skill with ticket context
2. Receive your structured report
3. If COMPLETE:
   - Run build verification
   - Spawn SA for acceptance verification
4. If BLOCKED:
   - Check if blocker is in queue
   - Reorder or report to user
5. If ESCALATE:
   - Spawn Systems Architect to answer your question
   - Return SA's answer to you
   - You continue implementation with guidance
6. If FAILED:
   - Spawn fix agent or report to user

### Escalation Flow

```
┌─────────────┐     ESCALATE      ┌─────────────────┐
│  Sub-Agent  │ ───────────────── │ Top-Level Agent │
│  (you)      │                   │                 │
└─────────────┘                   └────────┬────────┘
      │                                    │
      │                                    │ Spawn
      │                                    ▼
      │                           ┌─────────────────┐
      │                           │    Systems      │
      │                           │   Architect     │
      │                           └────────┬────────┘
      │                                    │
      │                                    │ Answer
      │                                    ▼
      │                           ┌─────────────────┐
      │      ANSWER               │ Top-Level Agent │
      │ ◄──────────────────────── │                 │
      │                           └─────────────────┘
      ▼
┌─────────────┐
│  Continue   │
│  Work       │
└─────────────┘
```

Your report format enables automated processing by the orchestrator.
