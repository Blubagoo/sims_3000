# Session Context

Last updated: 2026-02-02

---

## Current State

**Phase:** Pre-POC Preparation Complete - Ready to Begin POC Implementation

**What was just completed:**
- All 18 epics (0-17) fully planned with tickets
- Project confidence assessment: 6.5/10
- POC validation plan created with 4 POCs
- Pre-POC decisions finalized (tech stack, network format, asset requirements)
- 4 POC tickets created in ClickUp
- Browser MCP (Puppeteer) added to `.mcp.json`

---

## Project Confidence

**Current Rating:** 6.5/10
**Target After POCs:** 8.5/10

**High-Risk Areas Requiring Validation:**
| POC | Risk Area | Confidence |
|-----|-----------|------------|
| POC-1 | SDL_GPU Toon Shading | 6/10 |
| POC-2 | ENet Multiplayer Sync | 5/10 |
| POC-3 | Dense Grid Performance | 7/10 |
| POC-4 | ECS Simulation Loop | 5/10 |

---

## POC Tickets (ClickUp)

All 4 POCs are in the ZergCity list, ready for implementation:

1. **POC-1: SDL_GPU Toon Shading Pipeline**
   - Benchmark: 1000 instances @ ≤16.67ms frame time, ≤10 draw calls

2. **POC-2: ENet Multiplayer Snapshot Sync**
   - Benchmark: 50K entities @ ≤100 KB/s, ≤5ms processing, 0 desyncs

3. **POC-3: Dense Grid Performance at Scale**
   - Benchmark: 512×512 iteration ≤0.5ms, serialization ≤10ms

4. **POC-4: ECS Simulation Loop with EnTT**
   - Benchmark: 50K entities tick ≤25ms, 1000 events ≤1ms

---

## Pre-POC Decisions (Finalized)

**Reference:** `plans/decisions/tech-stack-finalization.md`

| Decision | Value |
|----------|-------|
| Database | SQLite 3.45+ with WAL mode |
| SDL | SDL3 3.2.0 |
| ECS | EnTT 3.13.2 |
| Networking | ENet 1.3.18 |
| Compression | LZ4 1.9.4 |
| Build System | CMake 3.25+, C++20 |
| Package Manager | vcpkg manifest mode |

**Other Pre-POC Documents:**
- `docs/architecture/network-message-format.md` - Message specs
- `docs/architecture/poc-asset-requirements.md` - 3D models needed for testing
- `docs/architecture/poc-validation-plan.md` - Success criteria and decision matrix

---

## Asset Acquisition (Pending)

**Needed for POC-1 testing:** 8 low-poly building models in glTF format

| Category | Count | Source |
|----------|-------|--------|
| Low-density habitation | 2 | Poly Pizza / itch.io |
| Medium-density habitation | 2 | Poly Pizza / itch.io |
| High-density habitation | 2 | Poly Pizza / itch.io |
| Low-density exchange | 1 | Poly Pizza / itch.io |
| Low-density fabrication | 1 | Poly Pizza / itch.io |

**Recommended Sources:**
- [Poly Pizza](https://poly.pizza/explore/Buildings) - Free glTF, no login
- [itch.io Low Poly](https://itch.io/game-assets/free/tag-low-poly) - KayKit packs
- [ITHappy Studios](https://ithappystudios.com/) - Consistent style

---

## MCP Servers Configured

**Location:** `.mcp.json`

| Server | Purpose |
|--------|---------|
| computer-control | Screen automation |
| clickup | Project management API |
| puppeteer | Browser automation (NEW) |

**Note:** Restart Claude Code to activate puppeteer MCP.

---

## Canon System

**Location:** `/docs/canon/`
**Version:** 0.17.0 (Epic 16 complete, Epic 17 requires no updates)

---

## ClickUp Structure

**Workspace:** Team Space (ID: 9012853182)
**Space:** ZergCity (ID: 90173684876)
**List:** ZergCity (ID: 901308969498)

---

## Epic Planning Status

All 18 epics fully planned:

| Phase | Epics | Status |
|-------|-------|--------|
| Foundation | 0, 1, 2, 3 | ✅ Planned |
| Core Gameplay | 4, 5, 6, 7, 10 | ✅ Planned |
| Services & UI | 9, 11, 12 | ✅ Planned |
| Advanced | 8, 13, 14 | ✅ Planned |
| Polish | 15, 16, 17 | ✅ Planned |

**Total Tickets:** ~900 across all epics

---

## Next Steps

1. **Restart Claude Code** to activate Puppeteer MCP
2. **Acquire test assets** from Poly Pizza or itch.io
3. **Begin POC-1** (SDL_GPU Toon Shading) or user's choice
4. **Validate benchmarks** against success criteria
5. **Update confidence assessment** based on POC results

---

## Key Files

| File | Purpose |
|------|---------|
| `docs/architecture/project-confidence-assessment.md` | Risk analysis |
| `docs/architecture/poc-validation-plan.md` | POC strategy |
| `plans/decisions/tech-stack-finalization.md` | Library versions |
| `docs/architecture/network-message-format.md` | Protocol spec |
| `docs/architecture/poc-asset-requirements.md` | Asset checklist |

---

## Notes

- **Claude is the development capacity** - tickets will be handed off one at a time
- **POCs are independent** - can be executed in parallel if needed
- **All POCs produce working demos** with benchmark output
- **Confidence assessment updates** after each POC completes
