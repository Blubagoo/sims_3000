# Update Canon

Update the project canon files with new decisions, patterns, or terminology.

## When to Use

Use this skill when:
- A product decision has been made that affects architecture
- New systems, interfaces, or patterns are defined
- Terminology is added or changed
- Phase ordering or dependencies change
- Any "source of truth" information needs updating

## Canon File Structure

```
docs/canon/
├── CANON.md           # Master file - principles, overview, changelog
├── terminology.yaml   # Official terms (alien theme + code naming)
├── patterns.yaml      # How we build (ECS, multiplayer, resources, grid)
├── interfaces.yaml    # Contracts between systems
└── systems.yaml       # What each system owns/provides/depends on
```

## Pre-Update Checklist

Before making changes, read these files to understand current state:

1. [ ] Read `/docs/canon/CANON.md` - understand core principles
2. [ ] Read the specific YAML file(s) you'll modify
3. [ ] Search for related terms/systems that might need cross-updates

## Update Process

### Step 1: Identify Impact

Determine which files need updates:

| Change Type | Files to Update |
|-------------|-----------------|
| New system | `systems.yaml`, possibly `interfaces.yaml` |
| New interface | `interfaces.yaml`, `systems.yaml` (implementers) |
| New terminology | `terminology.yaml` |
| Architecture change | `patterns.yaml`, possibly `CANON.md` |
| Phase/dependency change | `systems.yaml`, `CANON.md` (epic overview) |
| Core principle change | `CANON.md` |

### Step 2: Make Cross-Connected Updates

When adding something new, ensure ALL references are updated:

**Adding a new System:**
```yaml
# In systems.yaml - define the system
NewSystem:
  type: ecs_system
  owns: [...]
  does_not_own: [...]
  provides: [...]
  depends_on: [...]        # What systems does it need?
  components: [...]

# ALSO update other systems that might depend on or provide to this one
# Search for systems that might need "depends_on: NewSystem" added
```

**Adding a new Interface:**
```yaml
# In interfaces.yaml - define the interface
INewInterface:
  description: "..."
  purpose: "..."
  methods: [...]
  implemented_by: [...]    # List ALL implementers

# ALSO in systems.yaml - add interface to relevant systems
# Search for systems that should implement this interface
```

**Adding new Terminology:**
```yaml
# In terminology.yaml - add the term
category:
  human_term: alien_term

# ALSO search existing files for the human term and update if needed
```

### Step 3: Version Bump

Increment version in ALL modified YAML files:

```yaml
# Minor change (additions, clarifications)
version: "0.2.0" -> "0.2.1"

# Major change (breaking changes, restructuring)
version: "0.2.1" -> "0.3.0"
```

### Step 4: Update Changelog

Add entry to `CANON.md` changelog:

```markdown
| Version | Date | Change |
|---------|------|--------|
| 0.2.1 | YYYY-MM-DD | Brief description of what changed |
```

### Step 5: Verify Connections

Run these searches to verify no orphaned references:

```
1. Search systems.yaml for "depends_on" - verify all referenced systems exist
2. Search interfaces.yaml for "implemented_by" - verify all listed systems exist
3. Search for any "TBD", "TODO", "undefined" - flag for follow-up
```

## YAML Formatting Conventions

### systems.yaml

```yaml
epic_N_name:
  name: "Human Readable Name"
  note: "Optional context note"  # Use for important clarifications
  systems:
    SystemName:
      type: ecs_system | core      # ecs_system or core (non-ECS)
      owns:
        - "Thing this system is responsible for"
        - "Another responsibility"
      does_not_own:
        - "Thing explicitly NOT this system's job (SystemName owns)"
      provides:
        - "Data/queries this system exposes to others"
      depends_on:
        - SystemName              # Other systems this needs
      multiplayer:                # Optional - only if relevant
        authority: server | client
        per_player: true | false
      notes:                      # Optional - important implementation notes
        - "Note about implementation"
      components:
        - ComponentName
```

### interfaces.yaml

```yaml
domain_name:
  IInterfaceName:
    description: "What this interface represents"
    purpose: "Why it exists / what problem it solves"

    component_requirements:       # Optional - required components
      - ComponentName

    methods:
      - name: method_name
        params:
          - name: param_name
            type: param_type
            description: "What this param is"  # Optional
        returns: return_type
        description: "What this method does"

    implemented_by:
      - System or entity that implements this

    notes:                        # Optional
      - "Implementation notes"
```

### terminology.yaml

```yaml
category_name:
  human_term: alien_equivalent    # Simple mapping

  # OR for code naming
  code_naming:
    convention_name: PascalCase   # The convention to use
```

### patterns.yaml

```yaml
pattern_category:
  overview: |
    Multi-line description of this pattern category.
    Use pipe (|) for multi-line strings.

  sub_pattern:
    description: "What this is"
    rules:
      - "Rule 1"
      - "Rule 2"

    example: |
      Code or structured example here
```

## Common Pitfalls

1. **Orphaned dependencies** - Adding a system but forgetting to update other systems' `depends_on`
2. **Missing implementers** - Defining an interface but not listing all systems that implement it
3. **Version mismatch** - Updating content but forgetting to bump version
4. **No changelog** - Making changes without documenting in CANON.md
5. **Terminology drift** - Using human terms in technical docs instead of alien terms

## Post-Update Verification

After updates, optionally run the Systems Architect agent to verify coherence:

```
Spawn Systems Architect to review canon v{NEW_VERSION}
and verify no gaps were introduced.
```

## Example: Adding a New System

**Task:** Add TradingSystem to handle resource trading

**Files to update:**
1. `systems.yaml` - Add TradingSystem definition
2. `interfaces.yaml` - Add ITradeable interface if needed
3. `patterns.yaml` - Update trading section if exists
4. `CANON.md` - Bump version, add changelog entry

**Cross-reference checks:**
- Does EconomySystem need `depends_on: TradingSystem`?
- Does TradingSystem need `depends_on: EconomySystem`?
- What components does TradingSystem own?
- What interfaces does it implement?

**Version bump:** 0.2.0 -> 0.2.1 (addition, not breaking)

**Changelog:** "Added TradingSystem for player resource trading"
