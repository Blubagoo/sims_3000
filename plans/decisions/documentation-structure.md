# Decision: Documentation Structure

**Date:** 2026-01-25
**Status:** Accepted

## Context

Establishing conventions for project documentation to support both human developers and AI agents working on the codebase.

## Decision

### Directory Structure

```
sims_3000/
├── agents/                  # Agent profile definitions
│   └── top-level-agent.md   # Orchestrator behavior
├── .claude/
│   ├── settings.json        # Shared project settings (committed)
│   └── settings.local.json  # Local settings (gitignored)
├── docs/                    # Development documentation
├── plans/
│   ├── decisions/           # Technical decision records
│   └── *featureName*/       # Feature planning documents
└── CLAUDE.md                # Project context, loads agent profile
```

### Folder Purposes

#### `/agents` - Agent Profiles
- Agent behavior definitions
- `top-level-agent.md` - Orchestrator (coordinates, doesn't implement)
- Future subagent profiles as patterns emerge

#### `/.claude` - Claude Code Configuration
- `settings.json` - Shared project settings, MCP servers (committed)
- `settings.local.json` - User-specific settings (gitignored)

#### `/docs` - Development Documentation
- Stable documentation
- Architecture overviews
- API documentation
- How-to guides
- Reference material

#### `/plans/decisions` - Technical Decision Records
- Document significant technical choices
- Include context, decision, and consequences
- Use descriptive names (e.g., `tech-stack.md`)

#### `/plans/*featureName*` - Feature Plans
- Working documents for features
- May contain research, design, and implementation notes
- Use descriptive folder names (e.g., `terrain-rendering/`)

### File Naming Conventions
- Use descriptive kebab-case names
- No numbering prefixes
- Examples: `tech-stack.md`, `agent-architecture.md`

## Consequences

### Positive
- Clear separation between docs and plans
- Decision history is preserved and searchable
- AI agents can find relevant context quickly
- Simple, readable file names

### Negative
- Some overhead in maintaining structure
