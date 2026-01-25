# Decision: Multi-Agent Workflow Architecture

**Date:** 2026-01-25
**Status:** Accepted

## Context

Defining how AI agents collaborate on the Sims 3000 project, with clear separation between coordination and implementation responsibilities.

## Decision

### Agent Roles

#### Orchestrator Agent (Top-Level)
**Defined in:** `/CLAUDE.md`

**Responsibilities:**
- Coordinate work across the project
- Break down tasks into implementable units
- Spawn subagents for implementation work
- Verify quality of completed work
- Ensure tests pass and code meets standards
- Maintain project coherence

**Restrictions:**
- Does NOT write implementation code directly
- Does NOT make changes without verification
- Delegates all implementation to subagents

#### Subagents (Spawned Workers)
**Defined in:** Future `/agents/*.md` profiles as patterns emerge

**Responsibilities:**
- Receive specific, scoped tasks from orchestrator
- Implement features, fixes, or documentation
- Work with fresh context + relevant files only
- Report completion back to orchestrator

**Characteristics:**
- Start with clean context (no conversation history)
- Receive only files/information relevant to their task
- Focused on single responsibility
- Can be specialized (e.g., "testing agent", "rendering agent")

### Context Passing Strategy

When spawning a subagent, the orchestrator provides:
1. **Task description** - Clear, specific goal
2. **Relevant file paths** - Files the subagent needs to read/modify
3. **Constraints** - Any rules or patterns to follow
4. **Success criteria** - How to know when done

Subagents do NOT receive:
- Full conversation history
- Unrelated project context
- Previous agent outputs (unless explicitly passed)

### Quality Verification Requirements

Before accepting subagent work, orchestrator must:
1. Review changes for correctness
2. Ensure tests pass (when applicable)
3. Verify code follows project patterns
4. Check for unintended side effects

### Subagent Profile Evolution

- Start with generic subagent spawning
- Document patterns as they emerge during development
- Create specialized agent profiles when patterns stabilize
- Store profiles in `/agents/` directory (future)

## Consequences

### Positive
- Clear separation of concerns
- Fresh context prevents confusion
- Orchestrator maintains big-picture view
- Subagents can focus deeply on specific tasks
- Quality gates prevent errors from propagating

### Negative
- Overhead of spawning/coordinating agents
- Context must be explicitly passed
- Some duplication of information

## Example Workflow

```
User Request: "Add terrain rendering"

Orchestrator:
  1. Reads request, plans approach
  2. Spawns subagent: "Create TerrainMesh class"
     - Passes: relevant file paths, class requirements
  3. Verifies: code compiles, follows patterns
  4. Spawns subagent: "Add terrain shaders"
     - Passes: shader requirements, TerrainMesh interface
  5. Verifies: shaders compile, integrate correctly
  6. Spawns subagent: "Write terrain rendering tests"
  7. Final verification: all tests pass, feature complete
```
