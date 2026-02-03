# Enforce Canon

Scan all epic tickets and synchronize them with current canon, ensuring terminology, interfaces, patterns, and system boundaries are consistent.

## Usage

```
/enforce-canon [--epic <N>] [--dry-run] [--verbose]
```

**Examples:**
```
/enforce-canon                    # Scan and update all epics
/enforce-canon --epic 5           # Only check Epic 5
/enforce-canon --dry-run          # Report issues without making changes
/enforce-canon --verbose          # Show detailed comparison output
```

## When to Use

Use this skill when:
- Canon files have been updated (new version)
- After major planning decisions that affect multiple epics
- Before starting implementation of an epic (verification)
- Periodically to catch drift between planning and canon
- After `/update-canon` has made changes

## Prerequisites

Before running, ensure:
1. Canon files exist in `/docs/canon/`
2. Epic tickets exist in `/docs/epics/epic-N/tickets.md`
3. Canon version is documented in CANON.md

---

## Workflow Overview

```
┌─────────────────────────────────────────────────────────────────┐
│  1. LOAD CANON                                                  │
│     Read all canon files, extract current version               │
├─────────────────────────────────────────────────────────────────┤
│  2. DISCOVER EPICS                                              │
│     Find all epic tickets.md files                              │
├─────────────────────────────────────────────────────────────────┤
│  3. FOR EACH EPIC: COMPARE                                      │
│     Check terminology, interfaces, patterns, boundaries         │
├─────────────────────────────────────────────────────────────────┤
│  4. GENERATE DIFF REPORT                                        │
│     List all discrepancies found                                │
├─────────────────────────────────────────────────────────────────┤
│  5. APPLY FIXES (unless --dry-run)                              │
│     Update tickets with canon-compliant content                 │
├─────────────────────────────────────────────────────────────────┤
│  6. UPDATE HEADERS                                              │
│     Bump Canon Version in each modified file                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## Phase 1: Load Canon

Read and parse all canon files to build the reference model.

### Files to Read

```
/docs/canon/CANON.md           # Core principles, current version
/docs/canon/systems.yaml       # System boundaries, ownership
/docs/canon/interfaces.yaml    # Interface contracts
/docs/canon/patterns.yaml      # Implementation patterns
/docs/canon/terminology.yaml   # Canonical terminology
```

### Extract Canon Model

```yaml
canon_model:
  version: "0.13.0"  # From CANON.md or systems.yaml

  systems:
    # From systems.yaml - each system's owns/does_not_own/provides
    EnergySystem:
      owns: [...]
      does_not_own: [...]
      provides: [...]
      tick_priority: 10
    # ... all systems

  interfaces:
    # From interfaces.yaml - method signatures
    IEnergyProvider:
      methods:
        - name: is_powered
          params: [entity: EntityID]
          returns: bool
    # ... all interfaces

  terminology:
    # From terminology.yaml - term mappings
    power: energy
    electricity: energy
    blackout: grid_collapse
    # ... all terms

  patterns:
    # From patterns.yaml - key patterns
    dense_grid_exception:
      applies_to: [TerrainGrid, BuildingGrid, ...]
    pool_model:
      applies_to: [EnergySystem, FluidSystem]
    # ... all patterns
```

---

## Phase 2: Discover Epics

Find all epic ticket files.

```bash
# Pattern to find
/docs/epics/epic-*/tickets.md
```

### Epic Metadata Extraction

For each tickets.md, extract:
- Current canon version from header
- Epic number
- Ticket IDs and content
- Revision history

---

## Phase 3: Compare Each Epic

For each epic, run these checks:

### 3.1 Version Check

```
IF epic.canon_version < canon_model.version:
  FLAG: "Epic {N} at {epic.canon_version}, canon at {canon_model.version}"
```

### 3.2 Terminology Check

Scan all ticket text for non-canonical terms:

```yaml
terminology_violations:
  - epic: 5
    ticket: E5-003
    line: 42
    found: "power lines"
    should_be: "energy conduits"

  - epic: 7
    ticket: E7-015
    line: 88
    found: "road"
    should_be: "pathway"
```

**Terms to check (from terminology.yaml):**
- Infrastructure: power → energy, water → fluid, road → pathway, etc.
- Zones: residential → habitation, commercial → exchange, industrial → fabrication
- Simulation: crime → disorder, pollution → contamination, land_value → sector_value
- Buildings: building → structure, under_construction → materializing
- Economy: money → credits, taxes → tribute, budget → colony_treasury

### 3.3 Interface Check

For epics that implement or consume interfaces, verify:

```yaml
interface_violations:
  - epic: 4
    ticket: E4-019
    issue: "References IEnergyProvider.has_power() but interface uses is_powered()"

  - epic: 6
    ticket: E6-012
    issue: "Missing method has_fluid_at() from IFluidProvider interface"
```

**Check these interfaces per epic:**
| Epic | Implements | Consumes |
|------|------------|----------|
| 3 | ITerrainQueryable, ITerrainRenderData, ITerrainModifier | - |
| 4 | - | ITerrainQueryable |
| 5 | IEnergyProvider | - |
| 6 | IFluidProvider | IEnergyProvider |
| 7 | ITransportProvider | ITerrainQueryable |
| 8 | IPortProvider | ITerrainQueryable, ITransportProvider |
| 9 | IServiceQueryable | - |
| 10 | IDemandProvider, IGridOverlay | IServiceQueryable, IEconomyQueryable |
| 11 | IEconomyQueryable | - |

### 3.4 System Boundary Check

Verify tickets don't violate system ownership from systems.yaml:

```yaml
boundary_violations:
  - epic: 5
    ticket: E5-020
    issue: "EnergySystem ticket references 'building power consumption amounts' but systems.yaml says 'buildings define' consumption"

  - epic: 7
    ticket: E7-008
    issue: "TransportSystem ticket owns 'road rendering' but systems.yaml says RenderingSystem owns"
```

### 3.5 Pattern Check

Verify tickets follow canonical patterns:

```yaml
pattern_violations:
  - epic: 10
    ticket: E10-015
    issue: "DisorderGrid should use double-buffering per patterns.yaml but ticket doesn't mention it"

  - epic: 5
    ticket: E5-007
    issue: "Should use pool_model not 'flow simulation' per patterns.yaml"
```

### 3.6 Tick Priority Check

Verify system tick priorities match systems.yaml:

```yaml
priority_violations:
  - epic: 7
    ticket: E7-006
    issue: "States tick_priority 50 but systems.yaml says 45"
```

### 3.7 Component Size Check

Verify component struct sizes match specifications:

```yaml
size_violations:
  - epic: 4
    ticket: E4-003
    issue: "BuildingComponent shows 20 bytes but should be 16 bytes per static_assert"
```

---

## Phase 4: Generate Diff Report

Compile all violations into a report:

```markdown
# Canon Enforcement Report

**Date:** {date}
**Canon Version:** {version}
**Epics Scanned:** {count}

## Summary

| Category | Violations | Epics Affected |
|----------|------------|----------------|
| Version Drift | 3 | Epic 0, 1, 2 |
| Terminology | 12 | Epic 3, 4, 5, 7 |
| Interface | 2 | Epic 4, 6 |
| Boundary | 1 | Epic 5 |
| Pattern | 3 | Epic 5, 10 |
| Tick Priority | 1 | Epic 7 |
| **Total** | **22** | |

## Violations by Epic

### Epic 3
- [TERM] E3-005: "forest" → "biolume_grove"
- [TERM] E3-012: "water" → "fluid" (in context of terrain moisture)

### Epic 4
- [INTF] E4-019: IEnergyProvider method name mismatch
- [TERM] E4-008: "has_water" → "has_fluid"

...
```

---

## Phase 5: Apply Fixes

Unless `--dry-run`, apply automatic fixes where possible:

### 5.1 Terminology Fixes (Automatic)

Use find-and-replace for clear terminology violations:

```python
# Pseudo-code for terminology fix
for violation in terminology_violations:
    if is_safe_replacement(violation):
        replace_in_file(
            file=violation.file,
            old=violation.found,
            new=violation.should_be
        )
```

**Safe replacements** (whole-word, not in code blocks with different meaning):
- "power lines" → "energy conduits"
- "road" → "pathway" (when referring to transport)
- "residential" → "habitation"
- "commercial" → "exchange"
- "industrial" → "fabrication"

### 5.2 Interface Fixes (Manual Review)

Flag for manual review - don't auto-fix method signatures:

```
MANUAL REVIEW REQUIRED:
- Epic 4, E4-019: IEnergyProvider method name
  Current: has_power(entity)
  Canon: is_powered(entity)

  Suggest: Update ticket to use is_powered()
```

### 5.3 Boundary Fixes (Manual Review)

Flag ownership violations for manual review:

```
MANUAL REVIEW REQUIRED:
- Epic 5, E5-020: Boundary violation
  Issue: Ticket claims ownership of building consumption amounts
  Canon: BuildingSystem owns consumption amounts, EnergySystem queries

  Suggest: Reword ticket to query consumption, not own it
```

---

## Phase 6: Update Headers

For each modified epic tickets.md:

### 6.1 Update Canon Version

```markdown
# Before
**Canon Version:** 0.10.0

# After
**Canon Version:** 0.13.0
```

### 6.2 Add Revision History Entry

```markdown
## Revision History

| Date | Canon Change | Summary |
|------|-------------|---------|
| 2026-01-29 | v0.10.0 → v0.13.0 | Canon enforcement: 5 terminology fixes, 1 interface fix |
```

### 6.3 Add Verification Note

```markdown
> **Verification Note ({date}):** Verified against canon v{version}. {summary of checks passed}
```

---

## Output Artifacts

After running `/enforce-canon`:

```
# Console output
Canon Enforcement Complete
- Epics scanned: 12
- Violations found: 22
- Auto-fixed: 18
- Manual review needed: 4

# Files modified
docs/epics/epic-3/tickets.md  (2 terminology fixes)
docs/epics/epic-4/tickets.md  (3 terminology fixes, 1 interface fix flagged)
docs/epics/epic-5/tickets.md  (1 pattern fix, 1 boundary fix flagged)
...

# Report generated
docs/reports/canon-enforcement-{date}.md
```

---

## Check Categories Reference

### Terminology Categories

| Category | Check For | Replace With |
|----------|-----------|--------------|
| Infrastructure | power, electricity | energy |
| Infrastructure | water (utility) | fluid |
| Infrastructure | power plant | energy_nexus |
| Infrastructure | power line | energy_conduit |
| Infrastructure | road, street | pathway |
| Infrastructure | highway | transit_corridor |
| Infrastructure | traffic | flow |
| Infrastructure | congestion | flow_blockage |
| Zones | residential | habitation |
| Zones | commercial | exchange |
| Zones | industrial | fabrication |
| Simulation | crime | disorder |
| Simulation | pollution | contamination |
| Simulation | land value | sector_value |
| Simulation | happiness | harmony |
| Economy | money, dollar | credits |
| Economy | taxes | tribute |
| Economy | budget | colony_treasury |
| Time | year | cycle |
| Time | month | phase |
| Buildings | building (generic) | structure |
| Buildings | under construction | materializing |
| Buildings | abandoned | derelict |
| Population | citizen | being |
| Population | population | colony_population |

### Interface Method Signatures

Check exact method names and parameter types from interfaces.yaml:

```yaml
# Example checks
IEnergyProvider:
  - is_powered(entity: EntityID) -> bool
  - is_powered_at(x: int32, y: int32, owner: PlayerID) -> bool
  - get_pool_state(owner: PlayerID) -> EnergyPoolState

IFluidProvider:
  - has_fluid(entity: EntityID) -> bool
  - has_fluid_at(x: int32, y: int32, owner: PlayerID) -> bool

ITransportProvider:
  - is_road_accessible(entity: EntityID) -> bool
  - is_road_accessible_at(x: int32, y: int32, max_distance: uint8) -> bool
  - get_nearest_road_distance(x: int32, y: int32) -> uint8
```

### System Tick Priorities

From systems.yaml ISimulatable implemented_by:

| System | Priority |
|--------|----------|
| TerrainSystem | 5 |
| EnergySystem | 10 |
| FluidSystem | 20 |
| ZoneSystem | 30 |
| BuildingSystem | 40 |
| TransportSystem | 45 |
| RailSystem | 47 |
| PortSystem | 48 |
| PopulationSystem | 50 |
| DemandSystem | 52 |
| ServicesSystem | 55 |
| EconomySystem | 60 |
| DisorderSystem | 70 |
| ContaminationSystem | 80 |
| LandValueSystem | 85 |

---

## Example: Running Enforce Canon

```bash
> /enforce-canon

Loading canon files...
  - CANON.md: v0.13.0
  - systems.yaml: 17 systems
  - interfaces.yaml: 23 interfaces
  - terminology.yaml: 89 terms
  - patterns.yaml: 12 patterns

Discovering epics...
  Found 13 epic ticket files

Scanning Epic 0 (v0.13.0)... OK (0 violations)
Scanning Epic 1 (v0.13.0)... OK (0 violations)
Scanning Epic 2 (v0.13.0)... OK (0 violations)
Scanning Epic 3 (v0.13.0)... OK (0 violations)
Scanning Epic 4 (v0.13.0)... OK (0 violations)
Scanning Epic 5 (v0.13.0)... OK (0 violations)
Scanning Epic 6 (v0.13.0)... OK (0 violations)
Scanning Epic 7 (v0.13.0)... OK (0 violations)
Scanning Epic 8 (v0.13.0)... OK (0 violations)
Scanning Epic 9 (v0.13.0)... OK (0 violations)
Scanning Epic 10 (v0.13.0)... OK (0 violations)
Scanning Epic 11 (v0.13.0)... OK (0 violations)
Scanning Epic 12 (v0.13.0)... OK (0 violations)

=====================================
Canon Enforcement Complete
=====================================
Epics scanned: 13
All epics are canon-compliant at v0.13.0
No changes needed.
```

---

## Flags and Options

| Flag | Effect |
|------|--------|
| `--epic N` | Only scan Epic N |
| `--dry-run` | Report violations but don't modify files |
| `--verbose` | Show detailed comparison for each check |
| `--auto-fix` | Apply all safe fixes without confirmation (default: prompt) |
| `--report-only` | Generate report file only, no console output |
| `--strict` | Treat warnings as errors |

---

## Integration with Other Skills

### After /update-canon

When canon is updated, run enforce-canon to propagate changes:

```
/update-canon          # Make canon changes
/enforce-canon         # Propagate to all epics
```

### Before /plan-epic

Verify canon compliance before planning new epic:

```
/enforce-canon --epic 8   # Verify Epic 8 context
/plan-epic 8              # Plan with verified canon
```

### Periodic Maintenance

Run periodically to catch drift:

```
/enforce-canon --dry-run   # Check without modifying
```

---

## Error Handling

### Canon File Missing

```
ERROR: Canon file not found: /docs/canon/systems.yaml
Please ensure all canon files exist before running enforce-canon.
```

### Epic File Missing

```
WARNING: Epic 13 tickets.md not found (expected at /docs/epics/epic-13/tickets.md)
Skipping Epic 13.
```

### Parse Error

```
ERROR: Failed to parse Epic 5 tickets.md at line 142
Invalid markdown structure. Please fix manually.
```

---

## Best Practices

1. **Run after canon changes** - Always run enforce-canon after modifying canon files
2. **Review manual fixes** - Don't blindly accept suggested changes; verify context
3. **Use dry-run first** - Check what would change before applying
4. **Commit separately** - Commit canon changes and enforcement updates separately for clear history
5. **Document exceptions** - If a ticket intentionally differs from canon, add a note explaining why

---

## Notes

- This skill reads but does not modify canon files
- Epic tickets are the target of enforcement, not the source
- When in doubt, flag for manual review rather than auto-fix
- Terminology fixes are context-sensitive (e.g., "water" as terrain type vs utility)
