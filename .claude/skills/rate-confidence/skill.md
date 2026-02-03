# Rate Confidence

Have the Systems Architect evaluate a plan and provide structured confidence ratings.

## Usage

```
/rate-confidence <path-to-plan>
```

**Examples:**
```
/rate-confidence docs/epics/epic-0/tickets.md
/rate-confidence plans/feature-x.md
```

## When to Use

Use this skill when:
- Before starting implementation of a planned epic
- After major plan revisions
- When uncertain about plan readiness
- As a gate before committing resources

---

## Workflow

1. Spawn Systems Architect agent to analyze the plan
2. Agent reads the plan file and relevant canon files
3. Agent produces confidence assessment
4. Output written to same directory as plan with `-confidence-{YYYY-MM-DD}.md` timestamped suffix

---

## Agent Prompt

```
You are the Systems Architect assessing confidence in a plan.

Agent Profile: Read and follow /agents/systems-architect.md

PLAN TO ASSESS:
{path-to-plan}

CANON CONTEXT (read for reference):
- /docs/canon/CANON.md
- /docs/canon/systems.yaml
- /docs/canon/interfaces.yaml
- /docs/canon/patterns.yaml

YOUR TASK:
Analyze this plan and rate your confidence across these dimensions:

1. **Canon Alignment** - Does the plan conform to canonical decisions?
2. **System Boundaries** - Are system responsibilities clear and non-overlapping?
3. **Multiplayer Readiness** - Does the plan account for dedicated server architecture?
4. **Dependency Clarity** - Are dependencies between items well-defined?
5. **Implementation Feasibility** - Can this realistically be built as specified?
6. **Risk Coverage** - Are risks identified and mitigated?

For each dimension, rate HIGH / MEDIUM / LOW with rationale.

OUTPUT FORMAT:
Write to: {plan-directory}/{plan-name}-confidence-{YYYY-MM-DD}.md

Use this structure:
---
# Confidence Assessment: {Plan Name}

**Assessed:** {date}
**Assessor:** Systems Architect
**Plan:** {path-to-plan}

## Overall Confidence: [HIGH | MEDIUM | LOW]

[1-2 sentence summary of overall assessment]

## Dimension Ratings

| Dimension | Rating | Rationale |
|-----------|--------|-----------|
| Canon Alignment | | |
| System Boundaries | | |
| Multiplayer Readiness | | |
| Dependency Clarity | | |
| Implementation Feasibility | | |
| Risk Coverage | | |

## Strengths
- [What the plan does well]

## Key Concerns
- [Issues that lower confidence]

## Recommendations Before Proceeding
- [ ] [Actionable item to address before implementation]

## Proceed?
[YES / YES WITH CAVEATS / NO - NEEDS WORK]
---
```

---

## Rating Guidelines

### HIGH Confidence
- Plan clearly follows canon
- No ambiguity in system boundaries
- Multiplayer implications addressed
- Dependencies form a clear DAG
- Scope estimates are realistic
- Risks have mitigations

### MEDIUM Confidence
- Minor canon deviations or unclear areas
- Some boundary questions remain
- Multiplayer mostly addressed, some gaps
- Most dependencies clear
- Some scope uncertainty
- Some risks unaddressed

### LOW Confidence
- Significant canon conflicts
- Overlapping or unclear ownership
- Multiplayer not adequately considered
- Circular or missing dependencies
- Unrealistic scope
- Major unmitigated risks

---

## Output Location

The confidence assessment is written next to the plan file:
- Input: `docs/epics/epic-0/tickets.md`
- Output: `docs/epics/epic-0/tickets-confidence-2026-01-25.md`

---

## Example Output

Example filename: `tickets-confidence-2026-01-25.md`

```markdown
# Confidence Assessment: Epic 5 Power Tickets

**Assessed:** 2026-01-25
**Assessor:** Systems Architect
**Plan:** docs/epics/epic-5/tickets.md

## Overall Confidence: MEDIUM

Plan is solid but has gaps in multiplayer sync strategy and unclear EnergySystem/BuildingSystem integration points.

## Dimension Ratings

| Dimension | Rating | Rationale |
|-----------|--------|-----------|
| Canon Alignment | HIGH | Uses pool model per canon, correct terminology |
| System Boundaries | MEDIUM | EnergySystem boundary with BuildingSystem needs clarification |
| Multiplayer Readiness | LOW | No sync protocol defined for power state changes |
| Dependency Clarity | HIGH | Clear ticket dependencies, correct ordering |
| Implementation Feasibility | MEDIUM | Coverage algorithm complexity underestimated |
| Risk Coverage | MEDIUM | Performance risk noted but no mitigation specified |

## Strengths
- Correct use of pool model (not flow simulation)
- Good ticket granularity
- Experience requirements from Game Designer integrated

## Key Concerns
- No specification for how power state syncs to clients
- EnergyConduit coverage calculation may be expensive at scale
- Missing ticket for brownout UI feedback

## Recommendations Before Proceeding
- [ ] Add ticket for power state sync protocol
- [ ] Clarify EnergySystem/BuildingSystem interface in canon
- [ ] Add performance budget for coverage calculation
- [ ] Add brownout UI ticket or defer to Epic 12

## Proceed?
YES WITH CAVEATS - Address sync protocol before implementation
```
