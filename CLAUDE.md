# Sims 3000

SimCity-inspired city builder simulation game.

## Tech Stack

- **Language:** C++
- **Graphics/Input/Audio:** SDL3
- **Architecture:** Engine-less (custom systems)
- **Pattern:** ECS (Entity-Component-System)

## Agent Profile

Read and follow: [/agents/top-level-agent.md](/agents/top-level-agent.md)

## Project Structure

```
sims_3000/
├── agents/                  # Agent profiles
│   └── top-level-agent.md   # Orchestrator definition
├── .claude/
│   └── settings.json        # Shared settings, MCP servers
├── src/                     # Source code
├── docs/                    # Documentation
├── plans/
│   └── decisions/           # Technical decision records
└── CLAUDE.md                # This file
```

## Key References

- Agent behavior: `/agents/top-level-agent.md`
- Tech decisions: `/plans/decisions/tech-stack.md`
- Folder conventions: `/plans/decisions/documentation-structure.md`
- Agent architecture: `/plans/decisions/agent-architecture.md`

## Current State

- **Phase:** Initial setup
- **Next:** First feature implementation
