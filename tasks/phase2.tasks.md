# PHASE 2 - VALIDATION ENGINE EXECUTABLE TASKS

This is the detailed, executable version of phase2.md.
Each task is small enough to implement, test, and verify in one session.

---

## 0. ENTRY CRITERIA

### 0.1 Phase 1 Complete ✅
- [ ] Phase 1 Exit Criteria met (from phase1.tasks.md)
- [ ] Codebase stable and committed

---

## 1. VALIDATION ENGINE ARCHITECTURE

### 1.1 Core Validation Module
**GOAL: Independent, testable validation logic**

- [ ] Create `ValidationModule` directory structure (src/validation, include/validation)
- [ ] Define `ValidationResult` struct
  - [ ] Rule ID (string/enum)
  - [ ] Severity (Error/Warning)
  - [ ] Geometry Handle (std::string)
  - [ ] Location (Point2D)
  - [ ] User Message (std::string)
- [ ] Define `IValidationRule` interface
  - [ ] pure virtual `validate(Geometry)` method
  - [ ] pure virtual `id()` and `name()`
- [ ] Create `ValidationEngine` class
  - [ ] Registry for rules
  - [ ] Method `run(const DocumentModel&)` returning `vector<ValidationResult>`
  - [ ] Ensure NO dependency on UI (`Qt`, `CADCanvas`)

### 1.2 Infrastructure & Integration
**GOAL: Connect validation to the document**

- [ ] Integrate `ValidationEngine` into `DocumentModel` (optional owner)
- [ ] Threading support (future proofing performance)
  - [ ] Ensure `ValidationEngine` is thread-safe
- [ ] Implement `VerificationReport` log generation
- [ ] **Feature Flag System**
  - [ ] Create `FeatureManager` to toggle Validation Engine on/off
  - [ ] Enable/Disable via configuration file or build flag
  - [ ] Rollout strategy: Internal -> Beta -> Public


---

## 2. VALIDATION RULES (SPECIFICATION)

### 2.1 Open Contour Detection (ERROR)
**GOAL: Prevent non-closed paths for CNC**

- [ ] Implement `OpenContourRule` class
- [ ] Build adjacency graph from geometry entities
- [ ] Identify endpoints with degree != 2
- [ ] Filter using tolerance (1e-9)
- [ ] **Deliverable**: Robustly detecting open loops

### 2.2 Self-Intersection Detection (ERROR)
**GOAL: Prevent toolpath loops and excessive heat**

- [ ] Implement `SelfIntersectionRule` class
- [ ] Line-Line Intersection check (excluding shared endpoints)
- [ ] Line-Arc Intersection check
- [ ] Arc-Arc Intersection check
- [ ] Optimization: Use spatial acceleration (Grid/Quadtree) for N^2 checks
- [ ] **Deliverable**: flagging X-crossings and overlaps

### 2.3 Minimum Feature Size (ERROR)
**GOAL: Prevent features smaller than tool kerf**

- [ ] Implement `MinFeatureSizeRule` class
- [ ] Circles/Arcs: Check diameter < MIN_SIZE (e.g. 3mm)
- [ ] Lines: Check length < MIN_LENGTH
- [ ] **Deliverable**: Catching impossible-to-cut features

### 2.4 Feature Proximity (WARNING)
**GOAL: Warn about features too close to each other**

- [ ] Implement `FeatureProximityRule` class
- [ ] Distance check between all disjoint entities
- [ ] Threshold: < 2 * KERF
- [ ] **Deliverable**: Warnings for weak material areas

---

## 3. VALIDATION EXECUTION MODEL

### 3.1 Triggers
**GOAL: Ensure validation is always up to date**

- [ ] Trigger on File Import (Post-load)
- [ ] Trigger on Edit (Entity Add/Remove/Modify)
- [ ] Trigger on Undo/Redo
- [ ] Implement "Dirty" flag to avoid redundant checks
- [ ] **Performance Goal**: < 200ms for typical files

### 3.2 Result Management
- [ ] Cache validation results
- [ ] Clear results on new validation run
- [ ] Stable IDs for issues to track them across UI updates

---

## 4. VALIDATION PANEL UI

### 4.1 UI Layout
**GOAL: Persistent visibility of issues**

- [ ] Create `ValidationPanel` (inherits QWidget/QDockWidget)
- [ ] Dock to left or right side of Main Window (Persistent preference)
- [ ] Minimum width: 250px, Max width: 400px
- [ ] Sections for "Errors" (Red) and "Warnings" (Orange)
- [ ] List View for individual issues
- [ ] **Empty State**: Large green checkmark "No issues detected"
- [ ] **Status Indicators**: Count summary, Visual dots (Red/Orange/Green)

### 4.2 Interaction
**GOAL: Click to fix**

- [ ] Click issue in list -> Zoom/Pan to location on Canvas
- [ ] Hover issue -> Highlight geometry on Canvas
- [ ] Selection Sync: Select entity on Canvas -> Highlight relevant issues in panel

### 4.3 Visual Feedback on Canvas
- [ ] Render overlay markers for issues
- [ ] Red X for Errors
- [ ] Orange Triangle for Warnings
- [ ] Tooltip on hover over markers

---

## 5. EXPORT GATING

### 5.1 Export Interception
**GOAL: No bad DXFs leave the shop**

- [ ] Intercept "Save" / "Export DXF" command
- [ ] Run Full Validation
- [ ] **Export Button State Management**
  - [ ] Normal: Enabled
  - [ ] Errors: Disabled (Grayed out), Tooltip explains why
  - [ ] Warnings: Enabled with Orange Badge/Icon
- [ ] Logic:
  - [ ] If Errors > 0: **BLOCK** export, show Alert Dialog
  - [ ] If Warnings > 0: Show Confirmation Dialog (Yes/No with checkbox)
  - [ ] If Clean: Proceed with Success Toast

---

## 6. PERFORMANCE & TESTING

### 6.1 Performance Tuning
- [ ] Benchmark Suite: 
  - [ ] Small (<1k), Medium (1k-10k), Large (10k-50k) files
  - [ ] Target: <50ms (Small), <200ms (Medium), <2s (Large)
- [ ] Optimize intersection checks (Spatial Index)
- [ ] Memory profiling (leak check)

### 6.2 Automated Regression Testing
- [ ] Create suite of "Bad DXF" files
- [ ] Write headless test runner
- [ ] Assert that `ValidationEngine` catches expected errors for each file

---

## 7. DOCUMENTATION

### 7.1 User & Developer Docs
- [ ] Write "Validation Rules Reference" for users
- [ ] Update `File-Structure.md` with new modules
- [ ] Write `Validation_Architecture.md` for developers

---

## 8. PHASE 2 EXIT CRITERIA

You may proceed to Phase 3 ONLY if ALL true:

- [ ] Validation prevents all known "bad" geometry
- [ ] False positive rate < 5%
- [ ] Performance is acceptable (<200ms average)
- [ ] Export blocking is completely reliable
- [ ] Unit test coverage 100% for rules

---

## DAILY WORKFLOW

1. Pick ONE task from this list
2. Mark as [⏳] in progress
3. Implement + write test
4. Verify manually
5. Mark as [✅] complete
6. Commit
