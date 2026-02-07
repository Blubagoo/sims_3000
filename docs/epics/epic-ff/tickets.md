# Epic FF: Fast Follow - Tickets

**Epic:** FF - Fast Follow (Bugs & Issues Backlog)
**Created:** 2026-02-07
**Purpose:** Track bugs, issues, and small improvements discovered during development

---

## Ticket Summary Table

| ID | Title | Priority | Found In | Status |
|----|-------|----------|----------|--------|
| FF-001 | Multi-instance blank window bug | Medium | Epic 3 | Open |

---

## Tickets

---

### FF-001: Multi-instance blank window bug

**Priority:** Medium
**Found During:** Epic 3 terrain integration
**Date Reported:** 2026-02-07

#### Description

When launching a second instance of sims_3000.exe, the window appears blank (no rendering) even though the console shows successful initialization:
- Terrain shaders loaded successfully
- Terrain pipeline created
- 64 chunks initialized
- "Entering main loop" reached

The first instance renders correctly. Both instances use the same seed (325) and initialize without errors.

#### Reproduction Steps

1. Launch `sims_3000.exe` (first instance renders terrain correctly)
2. Launch `sims_3000.exe` again (second instance window is blank)
3. Both console windows show successful initialization

#### Suspected Causes

- Swapchain present failing silently when window not focused
- Shared config file conflict (`%AppData%/Sims3000/config.json`)
- Window creation timing issue
- SDL_GPU device/swapchain initialization order

#### Impact

- Cannot verify multiplayer visually on single machine
- Need two physical machines for multiplayer testing until fixed

#### Acceptance Criteria

- [ ] Two instances of sims_3000.exe can run simultaneously
- [ ] Both instances render terrain correctly
- [ ] Both instances respond to input independently

#### Notes

This is NOT an SDL_GPU resource limitation - SDL_GPU is efficient. This is a bug in our multi-instance handling code.

---

## Adding New Tickets

Use this template:

```markdown
### FF-XXX: [Title]

**Priority:** High/Medium/Low
**Found During:** [Epic or feature]
**Date Reported:** YYYY-MM-DD

#### Description
[What's the bug/issue?]

#### Reproduction Steps
1. [Step 1]
2. [Step 2]

#### Acceptance Criteria
- [ ] [Criterion 1]
- [ ] [Criterion 2]
```
