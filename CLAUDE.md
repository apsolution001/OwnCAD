# Claude Code – Project Memory (C++ / Qt CAD Application)

## 0. Scope & Authority

This file defines **MANDATORY project-level rules** for Claude Code.
Claude MUST follow this document before responding to any task.

Priority order (highest → lowest):
1. User prompt
2. File-Structure.md
3. Other project documentation
4. Existing code patterns

If a conflict exists, STOP and ask for clarification.

---

## 1. Project Context

- Application Type: Windows Desktop CAD Application
- Domain: 2D CAD (AutoCAD Lite alternative)
- Language: C++
- Framework: Qt
- Build System: CMake
- Architecture Style: Modular, geometry-core–driven, UI separated from model
- Current Status: Active production repository (not experimental)

This is **not a prototype**. All code is expected to be production-grade.

---

## 2. Mandatory File Awareness Rule (CRITICAL)

### ALWAYS DO THIS FIRST:
Before planning, coding, refactoring, or reviewing:

1. **Read `File-Structure.md`**
2. Treat it as the **single source of truth** for:
   - File responsibilities
   - Module boundaries
   - Where changes are allowed

### STRICT RULES:
- ❌ Do NOT guess file locations
- ❌ Do NOT duplicate responsibilities across files
- ❌ Do NOT introduce new files unless explicitly approved
- ❌ Do NOT bypass documented ownership of a file

If a requested change does not clearly map to an existing file:
→ Propose an update to `File-Structure.md` FIRST.

---

## 3. Workflow for Any Change (MANDATORY)

For **every task**, Claude MUST follow this sequence:

### Step 1 – Change Plan (NO CODE)
- Briefly summarize:
  - Which files will be read
  - Which files will change
  - What responsibility each change addresses
- Keep this short and structured
- WAIT for user approval

### Step 2 – Implementation
- Implement only approved changes
- No scope creep
- No unrelated refactors

### Step 3 – Summary
Provide a concise summary:
- Files changed
- Reasoning
- Architectural impact (if any)

---

## 4. Zero Patchwork Policy (ABSOLUTE)

Claude MUST NEVER:
- Apply band-aid fixes
- Mask root causes
- Add temporary workarounds
- Leave TODOs in production code

### When fixing bugs:
Claude MUST:
- Identify the ROOT CAUSE
- Explain why it happened
- Explain implications if left unfixed
- Fix the underlying design or logic flaw

---

## 5. File & Module Boundaries (ENFORCED)

### Geometry (`geometry/`)
- Pure domain logic only
- No Qt dependencies
- Deterministic, testable, unit-safe
- Numerical stability and tolerance handling required

### Import (`import/`)
- Format parsing only
- No UI logic
- DXF → internal geometry conversion only

### Model (`model/`)
- Owns application state
- No rendering logic
- No file parsing

### UI (`ui/`)
- Qt-only layer
- Rendering, interaction, dialogs
- No geometry math beyond transformations

Violations of boundaries are NOT allowed.

---

## 6. Naming Conventions (STRICT)

### Files
- PascalCase
- Header/Source pairs (`ClassName.h / ClassName.cpp`)
- One primary class per file

### Classes
- PascalCase
- Clear domain meaning (`Line2D`, `DocumentModel`, `CADCanvas`)

### Methods & Variables
- camelCase
- Verbs for methods, nouns for data
- No abbreviations unless industry-standard

### Constants
- `constexpr`
- UpperCamelCase or ALL_CAPS for global constants

---

## 7. Folder Conventions

- `geometry/` → math & primitives only
- `import/` → file formats & conversion
- `model/` → document & state
- `ui/` → Qt widgets & interaction
- `tests/` → mirrors source structure

No cross-folder leakage of responsibilities.

---

## 8. Comment Style & Documentation

### Comments are REQUIRED when:
- Math is non-trivial
- Tolerance decisions are made
- Algorithms are not obvious

### Comment Rules:
- Explain **WHY**, not WHAT
- No redundant comments
- No TODOs in production code
- Use clear, technical language

Public classes and complex methods must have brief documentation comments.

---

## 9. Error Handling Rules

- Never assume input is valid
- Always validate:
  - Geometry parameters
  - File input
  - User-driven values

### Preferred Patterns:
- Explicit error states
- Defensive checks
- Fail fast with clear messages

Silent failures are forbidden.

---

## 10. Logging Rules

- Logging must be:
  - Intentional
  - Actionable
  - Non-noisy

### Log when:
- Import fails or partially succeeds
- Geometry validation fails
- User-visible operations fail

No debug spam. No commented-out logs.

---

## 11. Testing Discipline

- Geometry logic must be unit-testable
- Edge cases required (precision, degenerates)
- Tests must reflect real CAD scenarios

If code cannot be tested → design is wrong.

---

## 12. Claude Behavior Rules (STRICT)

Claude MUST:
- Summarize intended changes before coding
- Mention which files were read
- Minimize token usage
- Avoid re-explaining known context
- Respect existing architecture
- Ask questions if ambiguity exists

Claude MUST NOT:
- Reformat unrelated code
- Introduce style inconsistencies
- Over-engineer without justification

---

## 13. Team Readiness

Although currently solo:
- Code must be readable by senior engineers
- Decisions must scale to a 1–5 person team
- Architectural clarity > cleverness

This codebase is expected to live for years.

---

## Final Principle

This is a **professional CAD system**, not a demo.

Every change should pass the question:
“Would this survive a senior engineering design review?”

If not — stop and redesign.
