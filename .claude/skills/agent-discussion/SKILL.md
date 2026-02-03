# Agent Discussion

Structured question/answer document format for inter-agent communication.

## When to Use

Use this skill when:
- You need clarification from another agent about shared work
- Another agent has asked you a question that needs answering
- A design decision requires input from multiple perspectives
- You're blocked and need information from a specialist agent

## Document Location

Discussion documents live in:
```
docs/discussions/
├── {topic-slug}.discussion.yaml    # Active discussions
└── archive/                        # Resolved discussions
```

## Document Format

```yaml
# docs/discussions/{topic-slug}.discussion.yaml
topic: "Brief topic title"
created: "YYYY-MM-DD"
status: active | resolved
participants:
  - agent-name-1
  - agent-name-2

threads:
  - id: Q001
    status: open | answered | resolved
    author: "agent-name"
    target: "agent-name"
    timestamp: "YYYY-MM-DD HH:MM"
    question: |
      The question text goes here.
      Can be multi-line.

    answers:
      - author: "agent-name"
        timestamp: "YYYY-MM-DD HH:MM"
        content: |
          The answer text goes here.
          Can be multi-line.

      - author: "agent-name"  # Follow-up answers allowed
        timestamp: "YYYY-MM-DD HH:MM"
        content: |
          Additional clarification.

  - id: Q002
    status: open
    author: "systems-architect"
    target: "game-designer"
    timestamp: "YYYY-MM-DD HH:MM"
    question: |
      Another question here.
    answers: []
```

## Operations

### 1. Create New Discussion

When starting work on a shared topic, create the discussion file:

```yaml
topic: "Resource System Design"
created: "2026-01-25"
status: active
participants:
  - systems-architect
  - game-designer

threads: []
```

### 2. Ask a Question

Add a new thread with your question:

```yaml
- id: Q001                          # Increment from last ID
  status: open
  author: "your-agent-name"         # Who you are
  target: "target-agent-name"       # Who should answer
  timestamp: "2026-01-25 14:30"
  question: |
    Your question here. Be specific about:
    - What you need to know
    - Why you need it (context)
    - What decision is blocked
  answers: []
```

**Question ID Convention:** Use `Q` + 3-digit number (Q001, Q002, etc.)

### 3. Answer a Question

Find threads where `target` matches your agent name and `status: open`.

Add your answer to the `answers` array and update status:

```yaml
- id: Q001
  status: answered                  # Change from 'open'
  author: "other-agent"
  target: "your-agent-name"
  timestamp: "2026-01-25 14:30"
  question: |
    Original question...
  answers:
    - author: "your-agent-name"     # You're answering
      timestamp: "2026-01-25 15:45"
      content: |
        Your answer here. Include:
        - Direct answer to the question
        - Reasoning/context
        - Any caveats or conditions
```

### 4. Mark as Resolved

When both parties agree the question is fully addressed:

```yaml
status: resolved                    # Final state
```

### 5. Follow-Up Questions

If an answer needs clarification, the original author can:
- Add another answer entry with a follow-up question
- Or create a new thread referencing the original (e.g., "Re: Q001")

## Status Flow

```
open -> answered -> resolved
         ^   |
         +---+  (follow-up needed)
```

- **open**: Question awaiting response from target
- **answered**: Target has responded, awaiting acknowledgment
- **resolved**: Question fully addressed, no action needed

## Agent Name Convention

Use kebab-case agent identifiers matching `/agents/*.md`:
- `top-level-agent`
- `systems-architect`
- `game-designer`

## Pre-Answer Checklist

Before answering a question:

1. [ ] Read the full discussion context
2. [ ] Check if related questions exist in the same file
3. [ ] Consider if your answer affects other systems/decisions
4. [ ] Reference relevant canon/docs if applicable

## Example: Complete Discussion

```yaml
topic: "Grid Coordinate System"
created: "2026-01-25"
status: active
participants:
  - systems-architect
  - game-designer

threads:
  - id: Q001
    status: resolved
    author: "systems-architect"
    target: "game-designer"
    timestamp: "2026-01-25 10:00"
    question: |
      For the grid system, should coordinates use:
      A) Integer-only (tile centers)
      B) Float (sub-tile positioning)

      This affects entity placement precision and
      pathfinding complexity.
    answers:
      - author: "game-designer"
        timestamp: "2026-01-25 10:30"
        content: |
          Use (A) Integer-only for buildings and zones.
          Use (B) Float for mobile entities (colonists, vehicles).

          Rationale: Buildings snap to grid, but colonists
          need smooth movement between tiles.

      - author: "systems-architect"
        timestamp: "2026-01-25 10:45"
        content: |
          Understood. I'll implement:
          - GridPosition component (int x, int y) for structures
          - WorldPosition component (float x, float y) for mobile

          Marking resolved.

  - id: Q002
    status: open
    author: "game-designer"
    target: "systems-architect"
    timestamp: "2026-01-25 11:00"
    question: |
      What's the maximum supported grid size?
      Need to know for UI minimap scaling.
    answers: []
```

## Finding Your Questions

To check for questions directed at you:

1. Search discussion files for `target: "your-agent-name"`
2. Filter for `status: open`
3. Answer in order of timestamp (oldest first)

## Archiving

When all threads in a discussion are `resolved`:

1. Change document `status: resolved`
2. Move to `docs/discussions/archive/`
3. Keep for historical reference

## Integration with Canon

If a discussion resolves a design decision:

1. Update relevant canon files (use `/update-canon` skill)
2. Reference the discussion: `# See: discussions/topic-slug Q001`
3. Archive the discussion
