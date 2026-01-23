# PHASE 2 — VALIDATION ENGINE (MONTH 4–5)

> **This phase turns your editor into a product.**
> Everything before this enables it. Everything after depends on it.

---

## 1. PHASE 2 PURPOSE

**Primary Goal:**
Prevent machine-level manufacturing errors before a DXF reaches the shop floor.

**Success Criteria:**
- Users trust the software to catch dangerous geometry
- The app becomes commercially usable
- Validation is predictable, explainable, and deterministic

**Why This Matters:**
This phase defines your core intellectual property and competitive advantage.

---

## 2. PHASE SCOPE DEFINITION

### What Phase 2 Includes:

- Geometry analysis engine with deterministic rule evaluation
- Structured validation reporting system
- Real-time error detection during editing
- Export blocking mechanism for critical errors
- Visual feedback system for geometry issues
- Validation panel user interface

### What Phase 2 Excludes:

- Automatic geometry fixing or repair
- CAM toolpath simulation
- Intent-based geometry interpretation
- Cosmetic or style warnings
- Machine profile management
- Batch validation of multiple files
- Rule configuration interfaces

**Guiding Principle:**
If a validation rule cannot be explained in plain language to a machine operator, it does not belong in this phase.

---

## 3. VALIDATION ENGINE ARCHITECTURE

### 3.1 Module Independence

**Design as Standalone Component:**
- No dependencies on UI framework (Qt-independent)
- No direct canvas or rendering logic
- Pure functional transformation: geometry in, validation results out
- Testable in isolation without GUI

**Benefits:**
- Can be unit tested independently
- Could be reused in command-line tools
- Could be integrated into web services
- Enables automated testing in CI/CD

---

### 3.2 Validation Result Structure

**Every Validation Issue Must Include:**

- **Rule Identifier:** Stable, unique ID that never changes (for tracking, documentation)
- **Severity Level:** ERROR (blocks export) or WARNING (alerts user)
- **Geometry References:** Which entities are affected (by stable ID or index)
- **Spatial Location:** World-space coordinates where issue occurs
- **User Explanation:** Plain-language description of the problem
- **Manufacturing Risk:** Why this matters for the machine/process
- **Auto-Fix Availability:** Boolean flag indicating if automatic repair is possible (for Phase 3)
- **Related Entities:** Any additional geometry context needed to understand the issue

**Anti-Pattern:**
No vague messages like "geometry error" or "invalid shape". Every message must be specific and actionable.

---

## 4. VALIDATION RULES SPECIFICATION

### 4.1 Open Contour Detection

**Severity:** ERROR

**Manufacturing Problem:**
Laser cutters, plasma cutters, and waterjet machines require closed paths to complete cuts. Open contours result in incomplete parts or machine errors.

**Detection Method:**
- Build topological graph of all entity endpoints
- Identify endpoints that appear only once (degree = 1)
- Flag contours with unmatched endpoints
- Apply tolerance when comparing endpoint positions

**Edge Cases to Handle:**
- Nearly-closed contours (gap smaller than tolerance)
- Multiple open gaps in same contour
- Degenerate entities (zero-length lines)

**User Communication:**
- Issue title: "Open contour detected"
- Description: "This contour has a gap and will not cut as a closed path"
- Risk explanation: "Machine cannot complete cut — part will not separate from stock material"
- Show gap location and distance

---

### 4.2 Self-Intersection Detection

**Severity:** ERROR

**Manufacturing Problem:**
Self-intersecting geometry creates ambiguous toolpaths, causes unpredictable machine behavior, and may result in program errors or material waste.

**Detection Method:**
- Implement line-to-line intersection algorithm
- Implement line-to-arc intersection algorithm
- Implement arc-to-arc intersection algorithm
- Filter out shared endpoints (these are valid connections)
- Apply geometric tolerance for near-intersections

**Edge Cases to Handle:**
- Tangent intersections (touching but not crossing)
- Multiple intersection points on same entity
- Coincident overlapping geometry
- Intersection points very close to endpoints

**User Communication:**
- Issue title: "Self-intersecting geometry"
- Description: "This contour crosses itself at one or more points"
- Risk explanation: "Undefined toolpath behavior — may cause machine error or scrap parts"
- Highlight both intersecting entities and intersection point

---

### 4.3 Minimum Feature Size (Holes)

**Severity:** ERROR

**Manufacturing Problem:**
Holes smaller than the cutting tool diameter cannot be physically cut. Attempting to cut them results in tool breakage or program rejection.

**Detection Method:**
- Identify closed contours classified as holes (inner contours)
- Calculate minimum bounding diameter or inscribed circle diameter
- Compare against configured minimum hole size
- Account for kerf offset (material removal by cutting process)

**Configuration Required:**
- Default minimum hole diameter (suggest 3mm as conservative default)
- Basis for future per-machine configuration

**Edge Cases to Handle:**
- Oblong holes (calculate narrowest dimension)
- Irregular hole shapes
- Holes that become too small after kerf compensation

**User Communication:**
- Issue title: "Hole too small to cut"
- Description: "Hole diameter (X mm) is below minimum cuttable size (Y mm)"
- Risk explanation: "Tool cannot fit inside hole — may cause tool damage or program error"
- Show hole outline and calculated diameter

---

### 4.4 Feature Proximity (Kerf Spacing)

**Severity:** WARNING

**Manufacturing Problem:**
Features too close together may merge during cutting due to heat-affected zone, kerf width, or material deformation. Results in scrap parts or dimensional inaccuracy.

**Detection Method:**
- Calculate minimum distance between parallel edges
- Calculate minimum distance between holes and edges
- Calculate minimum distance between adjacent holes
- Account for material thickness and kerf width

**Configuration Required:**
- Default minimum spacing (suggest 2× kerf width)
- Material-dependent adjustment factors

**Edge Cases to Handle:**
- Parallel vs. non-parallel edges (different rules may apply)
- Corner-to-edge distances
- Nested contours

**User Communication:**
- Issue title: "Features may be too close"
- Description: "Minimum distance between features is X mm (recommended minimum: Y mm)"
- Risk explanation: "Features may merge during cutting or exhibit poor edge quality"
- Show measurement lines between affected features

**Important:**
This is a WARNING, not an ERROR. Export should be allowed with user confirmation.

---

### 4.5 Additional Rules for Consideration

**Document these for potential Phase 2 inclusion:**

- **Duplicate/Overlapping Geometry:** Same geometry defined multiple times causes double-cutting
- **Minimum Edge Length:** Very short segments may exceed machine acceleration limits
- **Sharp Angles:** Acute angles may require special cutting strategies
- **Material Bounds Violations:** Geometry extends beyond material sheet size
- **Lead-in/Lead-out Clearance:** Insufficient space for tool entry/exit

**Decision Criteria:**
Only include rules where:
1. The consequence is machine error or scrap parts (not just quality degradation)
2. The detection algorithm is deterministic
3. The issue is common in real shop DXFs

---

## 5. VALIDATION EXECUTION MODEL

### 5.1 When Validation Occurs

**Automatic Validation Triggers:**
- Immediately after DXF import completes
- After any geometry modification (add, delete, move, scale, rotate)
- After undo/redo operations
- After paste operations

**Manual Validation Triggers:**
- User clicks "Revalidate" or "Check Geometry" button
- Before export initiation (final check)

**Performance Requirement:**
Validation must feel instantaneous for normal-sized files. Target: under 100ms for files with fewer than 10,000 entities.

---

### 5.2 Incremental vs. Full Validation

**For Phase 2:**
Implement full validation only (re-check entire model on every trigger).

**For Future Optimization (Phase 3+):**
Document approach for incremental validation (only re-check affected regions after local edits).

**Rationale:**
Full validation is simpler, more reliable, and sufficient for Phase 2 performance targets. Optimization is premature at this stage.

---

### 5.3 Validation Blocking Logic

**ERROR-Level Issues:**
- Export button becomes disabled
- Export menu item becomes disabled
- Tooltip explains: "Cannot export — X errors must be resolved"
- User cannot bypass (no "export anyway" option)

**WARNING-Level Issues:**
- Export remains enabled
- Confirmation dialog appears before export
- Dialog lists all warnings with descriptions
- User must explicitly acknowledge warnings to proceed
- User choice is logged (for future quality tracking)

**Zero Issues:**
- Export proceeds normally
- Brief confirmation message: "Validation passed — no issues detected"

---

## 6. VALIDATION PANEL USER INTERFACE

### 6.1 Panel Layout and Position

**Panel Characteristics:**
- Docked to left or right side of main window
- User can choose which side (preference saved)
- Minimum width: 250px, maximum width: 400px
- Resizable by dragging edge
- Cannot be closed or hidden (always visible)
- Persists across sessions

**Rationale:**
Validation status must always be visible. Hidden panels lead to ignored errors.

---

### 6.2 Panel Content Structure

**Header Section:**
- Issue count summary: "X Errors, Y Warnings"
- Visual indicators: red dot for errors, orange dot for warnings
- Green checkmark when no issues exist
- "Revalidate" button (with refresh icon)

**Issue List Section:**
- Grouped by severity (Errors first, then Warnings)
- Each group collapsible/expandable
- Issues sorted by: severity → rule type → spatial location

**Individual Issue Display:**
- Icon indicating severity (red X or orange triangle)
- Rule name as bold header
- One-line summary below header
- Optional: entity count if issue affects multiple items
- Click anywhere on issue to select and zoom

**Footer Section:**
- Last validation timestamp
- Total entity count
- Validation duration (for performance monitoring)

---

### 6.3 Interaction Behaviors

**Click Issue in Panel:**
- Canvas zooms to show affected geometry with padding around edges
- Affected geometry highlighted in severity color (red or orange)
- Issue remains selected (highlighted background in panel)
- Canvas pan/zoom locked to selection until user clicks elsewhere

**Hover Over Issue in Panel:**
- Affected geometry temporarily highlighted on canvas
- Highlight color: semi-transparent overlay of severity color
- No zoom change
- Removes highlight when hover ends

**Select Geometry on Canvas:**
- All related validation issues highlighted in panel
- Panel auto-scrolls to first related issue
- Visual connection: geometry highlight matches issue highlight

**Multiple Issues on Same Geometry:**
- Show count badge on geometry
- Panel shows all applicable issues
- Clicking geometry cycles through related issues (or shows list)

---

### 6.4 Visual Design Language

**Color Coding:**
- ERROR geometry: Bright red (#E53935 or similar)
- WARNING geometry: Orange/amber (#FB8C00 or similar)
- Selected geometry: Blue outline
- Normal geometry: Default black/gray
- Background: Light neutral gray (#F5F5F5)

**Visual Principles:**
- No animations or blinking (accessibility concern + distraction)
- No gradients or effects on geometry highlighting
- Solid, clear colors for maximum contrast
- Icons should be simple and universally recognizable
- Text should be readable at default size without zooming

**Accessibility Considerations:**
- Color blind safe palette
- Sufficient contrast ratios (WCAG AA minimum)
- Hover states clearly visible
- Keyboard navigation support (tab through issues, Enter to zoom)

---

### 6.5 Empty State Design

**When No Issues Exist:**
- Large green checkmark icon
- Message: "No issues detected"
- Submessage: "File is ready for export"
- Last validation timestamp
- Keep "Revalidate" button visible

**Rationale:**
Positive feedback is important. Users should feel confident when validation passes.

---

## 7. EXPORT GATING IMPLEMENTATION

### 7.1 Export Flow with Validation

**Step 1: Pre-Export Validation Check**
- Run full validation immediately before showing export dialog
- Display progress indicator if validation takes time
- Block progression if validation fails

**Step 2: Validation Result Handling**

*If Errors Exist:*
- Show modal dialog: "Cannot Export — Errors Detected"
- List all errors with descriptions
- Provide "Go to First Error" button (zooms to first issue)
- Only option is "Cancel Export"
- No way to force export

*If Only Warnings Exist:*
- Show modal dialog: "Warnings Detected"
- List all warnings with descriptions
- Checkbox: "I understand these warnings and accept the risks"
- Buttons: "Cancel Export" and "Export Anyway" (disabled until checkbox checked)
- Log user's decision for quality tracking

*If No Issues:*
- Proceed directly to export file dialog
- Show brief success toast: "Validation passed"

**Step 3: Post-Export Confirmation**
- Show export success message
- Include validation status summary
- Optionally: Save validation report alongside DXF

---

### 7.2 Export Button State Management

**Visual States:**

*Normal (No Issues):*
- Export button fully enabled
- Default appearance
- Tooltip: "Export DXF file"

*Disabled (Errors Present):*
- Export button visually disabled (grayed out)
- Tooltip: "Cannot export — X errors must be resolved. Click for details."
- Clicking button opens validation panel (if collapsed) and zooms to first error

*Warning State (Warnings Only):*
- Export button enabled but with indicator badge
- Orange warning icon overlay on button
- Tooltip: "Y warnings detected — review before export"

---

## 8. PERFORMANCE AND STABILITY REQUIREMENTS

### 8.1 Performance Targets

**Validation Speed:**
- Small files (< 1,000 entities): Under 50ms
- Medium files (1,000 - 10,000 entities): Under 200ms
- Large files (10,000 - 50,000 entities): Under 2 seconds
- Very large files (> 50,000 entities): Under 10 seconds with progress indicator

**Memory Constraints:**
- Validation should not double memory footprint
- Incremental validation results (not entire geometry duplication)
- Clean up temporary structures immediately after validation

**UI Responsiveness:**
- Validation must not block UI thread for more than 100ms
- Show progress indicator for validation taking longer than 500ms
- Allow user to cancel long-running validation

---

### 8.2 Stability Requirements

**Graceful Degradation:**
- Validation errors should never crash the application
- If a specific rule fails, continue with other rules
- Report validation engine errors to user separately from geometry errors
- Log internal errors for debugging

**Malformed Geometry Handling:**
- Handle infinite values gracefully
- Handle NaN (not-a-number) values
- Handle degenerate geometry (zero-length, zero-radius)
- Handle extreme coordinate values (very large or very small)

**Memory Safety:**
- No memory leaks during repeated validation
- Bounded memory growth for very large files
- Proper cleanup of validation results when document closes

---

### 8.3 Edge Case Requirements

**Must Handle Successfully:**
- DXF files with only points (no contours)
- DXF files with no geometry at all
- Single entity files
- Files with thousands of duplicate entities
- Files with geometry outside reasonable coordinate ranges
- Files with mixed unit systems
- Files with extremely small tolerance values

**For Each Edge Case:**
- Document expected behavior
- Create test case
- Verify no crashes or hangs

---

## 9. TESTING STRATEGY

### 9.1 Unit Testing

**Validation Rule Testing:**
- Create isolated test for each validation rule
- Test with known-good geometry (should pass)
- Test with known-bad geometry (should fail appropriately)
- Test edge cases specific to each rule
- Test tolerance boundary conditions

**Test Coverage Target:**
- 100% coverage of validation rule logic
- 100% coverage of validation result generation
- 90%+ coverage of validation engine infrastructure

**Example Test Cases Per Rule:**

*Open Contour Tests:*
- Completely closed square → no error
- Square with one missing line → error with correct gap location
- Almost-closed square (gap within tolerance) → no error
- Almost-closed square (gap outside tolerance) → error
- Multiple open gaps → multiple errors reported

*Self-Intersection Tests:*
- Figure-8 shape → error at intersection point
- Tangent circles → no error (touching is valid)
- Overlapping lines → error
- Complex polygon with self-crossing → all intersections detected

---

### 9.2 Integration Testing

**Full System Tests:**
- Import DXF → automatic validation runs → results displayed in panel
- Edit geometry → validation updates automatically
- Click issue in panel → canvas zooms correctly
- Attempt export with errors → export blocked
- Resolve errors → export enabled

**User Workflow Tests:**
- Complete editing session from import to export
- Undo/redo with validation tracking
- Multiple documents open simultaneously
- Document switching with different validation states

---

### 9.3 Regression Testing with Real Files

**Collect Shop Floor Files:**
- Known-bad DXFs that have caused machine errors (critical set)
- Known-good DXFs that cut successfully (control set)
- Edge case DXFs from customer support issues
- Stress test DXFs (very large, very complex)

**Build Regression Test Suite:**
- Each file has documented expected validation results
- Automated test runs validation and compares to expected output
- Any deviation from expected results fails the test
- Maintain this suite as living documentation

**Test Organization:**
- Group by geometry type (sheet metal, signage, decorative)
- Group by file size
- Group by common error patterns
- Track validation accuracy over time

---

### 9.4 UI Testing

**Visual Validation Tests:**
- Issue highlighting appears in correct color
- Zoom-to-issue frames geometry appropriately
- Multiple issues on same geometry handled correctly
- Panel scrolling and selection work smoothly

**Interaction Tests:**
- Click behaviors work as specified
- Hover behaviors work as specified
- Keyboard navigation functional
- Tooltips appear correctly

**Accessibility Tests:**
- Screen reader compatibility
- Keyboard-only navigation
- High-contrast mode support
- Text scaling support

---

### 9.5 Performance Testing

**Benchmark Suite:**
- Measure validation time for standard file sizes
- Track validation time regression over development
- Identify performance bottlenecks with profiling
- Test with files at upper size limits

**Stress Testing:**
- Maximum entity count before performance degrades
- Maximum validation issue count before UI slows
- Memory usage under sustained validation
- Concurrent validation of multiple documents

---

## 10. DOCUMENTATION REQUIREMENTS

### 10.1 User-Facing Documentation

**Validation Rule Reference:**
- Document each validation rule in user terms
- Explain why each rule matters for manufacturing
- Provide visual examples of each error type
- Explain how to resolve each type of issue

**User Guide Sections:**
- "Understanding Validation Results"
- "How to Fix Common Geometry Errors"
- "Export Blocking and Why It Happens"
- "Working with Warnings vs Errors"

---

### 10.2 Developer Documentation

**Architecture Documentation:**
- Validation engine design and data flow
- Adding new validation rules (extension guide)
- Validation result format specification
- Performance optimization guidelines

**API Documentation:**
- Public interfaces for validation engine
- Integration points with geometry model
- UI integration patterns

---

### 10.3 Test Documentation

**Test Plan:**
- What gets tested and how
- Test coverage goals
- Regression test suite maintenance

**Known Issues Log:**
- Document any edge cases not yet handled
- Track validation false positives/negatives
- Priority order for future improvements

---

## 11. DEPLOYMENT AND ROLLOUT

### 11.1 Feature Flag Strategy

**Controlled Rollout:**
- Implement behind feature flag initially
- Enable for internal testing first
- Enable for beta users second
- Full public release after validation period

**Rollback Plan:**
- Ability to disable validation engine if critical bugs found
- Fallback to export without validation (emergency only)
- Communication plan for users if rollback needed

---

### 11.2 User Communication

**Announcement Plan:**
- Blog post explaining new validation features
- In-app notification on first use
- Tutorial video showing validation workflow
- Email to existing users about update

**Educational Content:**
- "Why Validation Matters" explainer
- Common error types and fixes
- Success stories from early adopters

---

### 11.3 Feedback Collection

**Metrics to Track:**
- Validation accuracy (false positives/negatives)
- User satisfaction with validation feedback
- Time saved vs manual checking
- Export blocking frequency
- Most common validation errors (informs UI priorities)

**Feedback Mechanisms:**
- "Was this helpful?" on each validation issue
- General feedback button in validation panel
- Support tickets tagged by validation topic
- User interviews with early adopters

---

## 12. WHAT IS EXPLICITLY DEFERRED

### Do Not Implement in Phase 2:

**Automatic Geometry Repair:**
- "Fix All" buttons
- "Auto-close contours" features
- Geometry cleanup wizards
- Simplification or optimization tools

*Rationale:* Fixing geometry requires user intent understanding. Phase 2 focuses purely on detection.

**Rule Configuration Interface:**
- Editable tolerance values
- Custom rule enabling/disabling
- User-defined validation rules
- Severity level customization

*Rationale:* Configuration adds complexity. Phase 2 uses sensible defaults.

**Machine Profile System:**
- Machine-specific validation rules
- Material-specific tolerances
- Process-specific checks (laser vs plasma vs waterjet)
- Tool database integration

*Rationale:* Machine profiles require domain knowledge research and multi-machine testing.

**Advanced Visualization:**
- 3D preview of cutting process
- Animated toolpath simulation
- Heat-affected zone visualization
- Kerf compensation preview

*Rationale:* CAM visualization is separate product feature, not validation.

**Batch Processing:**
- Validate multiple files simultaneously
- Command-line validation tool
- Automated validation in CI/CD
- Validation report generation for entire folders

*Rationale:* Single-file validation must work perfectly first.

**Export Format Variations:**
- Format-specific validation rules
- Output optimization suggestions
- Legacy format compatibility checks

*Rationale:* Focus on geometry correctness, not format nuances.

---

## 13. PHASE 2 EXIT CRITERIA

### Phase 2 is Complete Only When:

**User Trust Validation:**
- At least 3 shop floor users say: "This caught a real problem I would have missed"
- Zero reports of validation missing obvious errors in first month
- Users report checking validation before export becomes habitual
- Validation is trusted more than manual geometry inspection

**Technical Validation:**
- All validation rules have 100% unit test coverage
- Regression test suite runs cleanly against 50+ real shop files
- No validation-related crashes in 2 weeks of beta testing
- Validation completes in under 2 seconds for 95% of test files

**Quality Validation:**
- False positive rate under 5% (validation flags issues that aren't real)
- False negative rate under 1% (validation misses real issues)
- Every validation error has clear, actionable description
- Users can resolve 90% of errors without external documentation

**Stability Validation:**
- No DXF file corruption during or after validation
- No data loss when resolving validation errors
- Validation engine handles all edge cases without crashing
- UI remains responsive during validation of large files

**Documentation Validation:**
- User guide explains all validation rules
- Developer guide enables adding new rules
- All validation messages reviewed for clarity
- FAQ covers common user questions

**Business Validation:**
- Product demo leads with validation as key differentiator
- Sales conversations confirm validation solves real pain points
- Support tickets about validation are feature requests, not bug reports
- Users willing to pay for validation feature specifically

---

## 14. SUCCESS METRICS

### Quantitative Metrics:

- **Validation Accuracy:** 95%+ correct error detection
- **Performance:** 99% of validations complete in under 1 second
- **User Engagement:** 80%+ of users check validation before export
- **Error Prevention:** 50%+ reduction in shop floor geometry errors
- **User Satisfaction:** 4.5+ / 5 stars for validation feature

### Qualitative Metrics:

- User testimonials mentioning validation specifically
- Shops adopt software primarily for validation capability
- Competitors begin copying validation approach
- Industry recognition of validation as best practice

---

## 15. RISK MITIGATION

### Technical Risks:

**Risk: Validation Performance Unacceptable on Large Files**
- Mitigation: Implement spatial indexing early
- Mitigation: Profile and optimize hot paths continuously
- Mitigation: Add progress indicators for long operations
- Fallback: Manual validation bypass for expert users (logged)

**Risk: Geometric Algorithms Have Edge Cases**
- Mitigation: Extensive unit testing with degenerate cases
- Mitigation: Collect real-world problematic files early
- Mitigation: Conservative tolerance handling
- Fallback: Manual override with warning logged

**Risk: False Positives Erode User Trust**
- Mitigation: Conservative rule design (prefer false negatives initially)
- Mitigation: Beta testing with real shop users
- Mitigation: Feedback mechanism on every validation issue
- Fallback: Severity downgrade for controversial rules

### User Experience Risks:

**Risk: Users Ignore Validation Warnings**
- Mitigation: Make validation always visible
- Mitigation: Block export on errors (no bypass)
- Mitigation: Educational content on consequences
- Fallback: Log all ignored warnings for quality analysis

**Risk: Validation Messages Unclear**
- Mitigation: User testing of all error messages
- Mitigation: Plain language review by non-technical users
- Mitigation: Visual examples in documentation
- Fallback: Iterative message refinement based on support tickets

**Risk: Users Frustrated by Export Blocking**
- Mitigation: Clear explanation of why blocking occurs
- Mitigation: Zoom-to-error makes fixing easy
- Mitigation: Future phase includes auto-fix options
- Fallback: Override option for advanced users (with liability waiver)

### Business Risks:

**Risk: Validation Not Seen as Valuable**
- Mitigation: Beta with shops who have had expensive errors
- Mitigation: Quantify cost savings from error prevention
- Mitigation: Case studies and testimonials
- Fallback: Make validation part of premium tier only

**Risk: Competitors Clone Feature Quickly**
- Mitigation: Build patent-defensible unique aspects
- Mitigation: Focus on execution quality, not just feature existence
- Mitigation: Build network effects (shared rule libraries)
- Fallback: Compete on overall user experience, not just validation

---

## 16. PHASE 2 TIMELINE

### Week 1-2: Foundation
- Design validation engine architecture
- Define validation result data structures
- Implement spatial indexing for geometry queries
- Set up unit testing framework
- Create initial regression test suite

### Week 3-4: Core Rules Implementation
- Implement open contour detection
- Implement self-intersection detection
- Implement minimum hole size checking
- Unit test all rules comprehensively

### Week 5-6: UI Integration
- Design and implement validation panel
- Implement zoom-to-issue functionality
- Implement geometry highlighting
- Connect validation to edit operations

### Week 7: Export Gating
- Implement export blocking logic
- Design and build confirmation dialogs
- Add validation pre-export check
- Implement warning acknowledgment flow

### Week 8: Polish and Testing
- Performance optimization
- Edge case handling
- Beta user testing
- Bug fixes and refinement
- Documentation completion

---

## 17. FINAL PRINCIPLE

> **If validation is wrong once, trust is lost forever.**

Every false positive trains users to ignore validation.
Every false negative makes users doubt validation.
Every unclear message frustrates users.

Validation must be:
- **Accurate** — correct 99% of the time
- **Explainable** — users understand why errors exist
- **Deterministic** — same file always produces same results
- **Fast** — feels instantaneous for normal files
- **Trustworthy** — users believe it over their own eyes

This is the foundation everything else is built on.

**Do not compromise on validation quality to ship faster.**

---

## PHASE 2 COMPLETE

When shops say "I can't imagine working without this validation anymore," Phase 2 has succeeded.
