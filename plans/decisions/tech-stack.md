# Decision: Technology Stack

**Date:** 2026-01-25
**Status:** Accepted

## Context

Selecting the foundational technology stack for Sims 3000, a SimCity-inspired city builder simulation game.

## Decision

### Primary Language: C++
- Industry standard for game development
- Direct memory control for performance-critical simulation
- Extensive ecosystem of game development libraries

### Graphics/Input/Audio: SDL3
- Cross-platform abstraction layer
- Handles window management, input, audio
- SDL3 is the latest version with improved APIs
- Lighter weight than full game engines

### Architecture: Engine-less (Custom)
- No commercial engine (Unity, Unreal, Godot)
- Build systems from scratch using SDL3 as the foundation
- Provides maximum control over architecture
- Learning opportunity for engine development

### Future Migration Path
- Architecture designed to allow extraction of reusable engine components
- Could evolve into a custom engine over time
- Keep coupling loose between game logic and rendering/systems

### Game Object Pattern: ECS (Entity-Component-System)
- To be confirmed during implementation
- Preferred pattern for simulation-heavy games
- Allows flexible composition of game objects
- Good cache coherency for large numbers of entities

## Consequences

### Positive
- Full control over codebase and architecture
- No licensing fees or engine constraints
- Valuable learning experience
- Performance can be optimized precisely

### Negative
- More initial development work
- Must implement features that engines provide out-of-box
- Steeper learning curve

## References
- SDL3 documentation: https://wiki.libsdl.org/SDL3
