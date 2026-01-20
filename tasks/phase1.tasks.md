# PHASE 1 - EXECUTABLE TASK LIST

This is the detailed, executable version of phase1.md.
Each task is small enough to implement, test, and verify in one session.

---

## 0. ENTRY CRITERIA

### 0.1 Phase 0 DXF Import/Export Stable ✅
- [x] DXFParser implemented
- [x] GeometryConverter working
- [x] DocumentModel loads DXF files
- [x] Validation on import

### 0.2 CAD Canvas with Geometry Rendering ✅
**GOAL: User can SEE loaded geometry**

- [x] Create `CADCanvas` widget class (inherits QWidget)
- [x] Implement viewport transformation system
  - [x] worldToScreen(Point2D) → QPointF
  - [x] screenToWorld(QPointF) → Point2D
- [x] Implement basic geometry rendering
  - [x] Render Line2D entities
  - [x] Render Arc2D entities
  - [x] Apply proper line styles (width, color)
- [x] Replace MainWindow placeholder with CADCanvas
- [x] Wire up DocumentModel to canvas
- [x] Verify: Open DXF file → See geometry rendered

**Deliverable**: Geometry visible on screen after DXF import

### 0.3 Pan / Zoom Functionality ✅
**GOAL: User can navigate the viewport**

#### Pan
- [x] Middle mouse button drag to pan
- [x] Track viewport offset (panX, panY)
- [x] Update worldToScreen transformation
- [x] Show pan cursor feedback

#### Zoom
- [x] Mouse wheel to zoom in/out
- [x] Zoom centered on mouse cursor position
- [x] Track zoom level (min: 0.01, max: 100)
- [x] Clamp zoom to prevent overflow
- [x] Update worldToScreen transformation
- [x] Show zoom level in status bar

#### Viewport State
- [x] Create Viewport class to manage state
- [x] Store: panX, panY, zoomLevel
- [x] Methods: reset(), fitToContent(), zoomExtents()

**Deliverable**: Smooth pan and zoom navigation

### 0.4 Snap System ✅
**GOAL: Precise point input for drawing/editing**

**STATUS: COMPLETE (CRITICAL FIX APPLIED 2026-01-12)**

**⚠️ CRITICAL ARCHITECTURAL FIX:**
- **ROOT CAUSE:** Snap tolerance was incorrectly implemented in world-space (10 world units)
  - At high zoom (100x): 10 world units = 1000 screen pixels → unusable
  - At low zoom (0.01x): 10 world units = 0.1 screen pixels → impossible to snap
- **PERMANENT FIX:** Converted to screen-space tolerance (10 pixels, zoom-independent)
  - SnapManager now accepts zoomLevel parameter (CADCanvas.h:92-96)
  - Dynamic conversion: `worldTolerance = pixels / zoomLevel` (CADCanvas.cpp:121-124)
  - Industry-standard behavior: constant screen-space snap radius regardless of zoom
- **FILES MODIFIED:** CADCanvas.h, CADCanvas.cpp (snap tolerance architecture)

#### Core Snap Modes ✅
- [x] Implement Midpoint snap
  - [x] snapToMidpoint() method implemented (CADCanvas.cpp:202-234)
  - [x] For Line2D: uses Line2D::pointAt(0.5)
  - [x] For Arc2D: uses Arc2D::pointAt(0.5)
  - [x] Snap tolerance detection working (10 world units)
- [x] Implement Nearest snap
  - [x] snapToNearest() method implemented (CADCanvas.cpp:236-268)
  - [x] Uses GeometryMath::closestPointOnSegment() for lines
  - [x] Uses GeometryMath::closestPointOnArc() for arcs
  - [x] Distance < tolerance check implemented

#### Priority System ✅
- [x] snap() method implements correct priority order (CADCanvas.cpp:115-160):
  - Priority 1: Endpoint (most precise)
  - Priority 2: Midpoint (construction reference)
  - Priority 3: Nearest (general alignment)
  - Priority 4: Grid (fallback)

#### Visual Feedback ✅
- [x] Snap result tracking in SnapManager
  - [x] lastSnapPoint_ stores current snap (optional<Point2D>)
  - [x] lastSnapType_ stores snap type (enum: None/Grid/Endpoint/Midpoint/Nearest)
- [x] renderSnapIndicator() implemented (CADCanvas.cpp:553-600)
  - [x] Draws cross + circle marker at snap point
  - [x] Color coding implemented:
    - Grid: Light gray (#C8C8C8)
    - Endpoint: Blue (#0066FF) - 10px
    - Midpoint: Green (#00CC00) - 8px
    - Nearest: Orange (#FF8800) - 8px
  - [x] Marker size in screen-space (not world-space)

#### UI Integration ✅
- [x] Keyboard shortcuts implemented (main.cpp:109-119)
  - [x] Grid snap (G hotkey)
  - [x] Endpoint snap (E hotkey)
  - [x] Midpoint snap (M hotkey)
  - [x] Nearest snap (N hotkey)
- [x] Persistent status bar indicator showing active snap modes (main.cpp:138-164)
- [x] Tooltip showing snap type and coordinates when snapping (CADCanvas.cpp:624-656)
- [ ] Toolbar buttons - **DEFERRED to Section 7.2 (UI Structure)**
  - Reason: Toolbar infrastructure doesn't exist yet
  - Menu + keyboard shortcuts are sufficient for Phase 1

#### Edge Case Testing ✅ (Verified by Code Analysis)
- [x] Zero-length lines: Rejected at creation (MIN_LINE_LENGTH check), GeometryMath handles gracefully
- [x] Full-circle arcs: pointAt(0.5) correctly calculates 180° midpoint
- [x] Snap tolerance at zoom: Tolerance in world units (zoom-independent by design)
- [x] Snap priority: Enforced by check order in snap() method
- [x] Multiple entities at same location: Deterministic (first match wins)

**Deliverable**: ✅ Cursor snaps to meaningful points with visual feedback and keyboard control

### 0.5 Grid Rendering ✅
**GOAL: Visual reference grid**

**STATUS: COMPLETE**

#### Grid Rendering ✅
- [x] Render background grid
  - [x] Major grid lines (every 10× spacing) (CADCanvas.cpp:476-484, 491-499)
  - [x] Minor grid lines (every 1× spacing)
  - [x] Adaptive spacing based on zoom level (CADCanvas.cpp:453-456)
    - Hides when too dense (<5px screen spacing)
    - Hides when too sparse (>200px screen spacing)
- [x] Grid styling
  - [x] Major: darker gray (#999 = RGB(153,153,153)) (CADCanvas.cpp:468)
  - [x] Minor: lighter gray (#ddd = RGB(221,221,221)) (CADCanvas.cpp:469)
  - [x] Background: light gray (RGB(250,250,250)) (CADCanvas.cpp:299-301)
- [x] Grid visibility toggle (View menu) (main.cpp:111, 291-298)
- [x] Origin indicator (red/green axes at 0,0) (CADCanvas.cpp:502-513)

#### Grid Settings System ✅ **NEW: Separate Files Architecture**
- [x] GridSettings structure (GridSettingsDialog.h:25-35)
  - Units enum: Millimeters, Centimeters, Inches
  - Spacing: configurable (0.1 to 1000 units)
  - Visibility flag
- [x] GridSettingsDialog class (**NEW FILES** - proper separation)
  - **include/ui/GridSettingsDialog.h** (89 lines)
  - **src/ui/GridSettingsDialog.cpp** (137 lines)
  - Units dropdown (mm/cm/in)
  - Spacing spin box with validation
  - Info label explaining major grid lines
  - OK/Cancel buttons
- [x] CADCanvas integration (CADCanvas.h:159-163, CADCanvas.cpp:337-352)
  - setGridSettings() method
  - gridSettings() getter
  - Backward-compatible setGridVisible()/setGridSpacing()
- [x] Main window integration (main.cpp:112, 300-318)
  - "Grid Settings..." menu item
  - Dialog invocation with current settings
  - Status bar feedback on changes

#### File Organization ✅
- **Separate dialog files** (clean architecture)
- GridSettingsDialog.h: 89 lines (header with struct + class)
- GridSettingsDialog.cpp: 137 lines (implementation)
- Added to CMakeLists.txt (line 152, 157)

**Deliverable**: ✅ Clean, professional grid background with configurable settings dialog

---

## 1. CORE GEOMETRY STABILITY

### 1.1 Geometry Primitives Hardening ✅
**GOAL: Rock-solid geometry foundation for manufacturing**

**STATUS: COMPLETE (PRODUCTION-READY)**

#### Line2D Implementation ✅
- [x] **Defined with start, end** (Line2D.h:30-31)
  - Immutable endpoints (Point2D)
  - Private constructor + factory pattern (Line2D.h:41, 55)
  - Validation: MIN_LINE_LENGTH check (GeometryConstants.h:34)
  - Factory returns `std::optional<Line2D>` (safe failure)
- [x] **Tolerance-based equality** (Line2D.cpp:47-50)
  - Compares both endpoints within tolerance
  - Uses Point2D::isEqual internally
  - Default tolerance: GEOMETRY_EPSILON (1e-9)
- [x] **Bounding box calculation** (Line2D.h:77, BoundingBox.cpp:60-62)
  - fromLine() factory method
  - O(1) complexity (just endpoints)
  - Cached after first access
- [x] **Additional methods:**
  - length() with caching (Line2D.h:71)
  - pointAt(t) for interpolation (Line2D.h:114)
  - containsPoint() with tolerance (Line2D.h:107)
  - angle() calculation (Line2D.h:120)
  - isValid() runtime check (Line2D.h:91)

#### Arc2D Implementation ✅
- [x] **Defined with center, radius, angles** (Arc2D.h:40-44)
  - Center point (Point2D)
  - Radius (double, >= MIN_ARC_RADIUS)
  - Start angle, end angle (radians, normalized to [0, 2π))
  - Direction flag (CCW/CW) - **CRITICAL FOR CNC TOOLPATHS**
  - Immutable after construction
- [x] **Tolerance-based equality** (Arc2D.cpp:88-93)
  - Compares center (Point2D::isEqual)
  - Compares radius, angles (GeometryMath::areEqual)
  - Compares direction (exact boolean match)
  - Manufacturing-aware: direction matters for toolpaths
- [x] **Bounding box calculation** (Arc2D.h:156, BoundingBox.cpp:64-95)
  - fromArc() factory method
  - **COMPLEX ALGORITHM:** Checks arc quadrant crossings
  - Tests if arc crosses 0°, 90°, 180°, 270° (extrema points)
  - Uses GeometryMath::isAngleBetween for sweep detection
  - Handles full circles (360°) correctly
  - Cached after first access
- [x] **Additional methods:**
  - sweepAngle() calculation (Arc2D.h:111)
  - isFullCircle() check (Arc2D.h:115)
  - startPoint(), endPoint() (Arc2D.h:121, 126)
  - pointAt(t) and pointAtAngle() (Arc2D.h:140, 133)
  - length() with caching (Arc2D.h:146)
  - containsPoint() with tolerance (Arc2D.h:195)
  - isValid() runtime check (Arc2D.h:170)

#### BoundingBox Calculation ✅
- [x] **Factory methods** (BoundingBox.h:48-73)
  - fromPoints(p1, p2) - Two corners
  - fromPointList(vector) - Arbitrary point cloud
  - fromLine(Line2D) - Line segment
  - fromArc(Arc2D) - Arc with quadrant crossing detection
- [x] **Containment tests** (BoundingBox.h:127-141)
  - contains(Point2D, tolerance) - Point inside check
  - intersects(BoundingBox) - Box overlap test
  - containsBox(BoundingBox) - Full containment check
- [x] **Operations** (BoundingBox.h:148-155)
  - merge(BoundingBox) - Union of two boxes
  - expand(margin) - Grow by margin
- [x] **Queries** (BoundingBox.h:95-113)
  - width(), height(), area()
  - center() - Center point
  - isValid() - Check if min <= max

#### Tolerance-Based Equality ✅
- [x] **Point2D::isEqual** (Point2D.h:71, Point2D.cpp:39-42)
  - Compares X and Y independently
  - Uses absolute difference: |x1 - x2| < tolerance
  - Default tolerance: GEOMETRY_EPSILON (1e-9)
- [x] **Line2D::isEqual** (Line2D.h:85, Line2D.cpp:47-50)
  - Delegates to Point2D::isEqual for both endpoints
  - Composable tolerance propagation
- [x] **Arc2D::isEqual** (Arc2D.h:164, Arc2D.cpp:88-93)
  - Center: Point2D::isEqual
  - Radius/angles: GeometryMath::areEqual
  - Direction: Exact match (boolean)
- [x] **Constants** (GeometryConstants.h:26-49)
  - GEOMETRY_EPSILON = 1e-9 (primary tolerance)
  - MIN_LINE_LENGTH = 1e-9 (degenerate line threshold)
  - MIN_ARC_RADIUS = 1e-9 (degenerate arc threshold)
  - MIN_ARC_SWEEP = 1e-9 (degenerate sweep threshold)

#### Manufacturing Safety Features ✅
- [x] **Factory pattern validation** (prevents invalid geometry creation)
  - Line2D::create() returns std::nullopt if invalid
  - Arc2D::create() returns std::nullopt if invalid
  - No way to construct invalid geometry in normal use
- [x] **Immutability** (prevents accidental modification)
  - All geometry classes are immutable after construction
  - No setters, only getters
  - Thread-safe by design
- [x] **Runtime validation** (double-check at runtime)
  - isValid() methods on all primitives
  - wouldBeValid() static helpers for pre-validation
- [x] **Lazy caching** (performance optimization)
  - Bounding boxes computed on first access
  - Length/area computed on first access
  - Marked mutable for const correctness

**Deliverable**: ✅ Production-grade geometry primitives with comprehensive validation, tolerance-aware equality, and manufacturing safety features.

### Shape Support Status (Updated 2026-01-16)

**Fully Implemented & Rendering:**
- [x] Line – Straight line between two points (Line2D)
- [x] Polyline – Connected lines and arcs treated as one object (decomposed to Line2D/Arc2D)
- [x] Circle – Perfect round shape (stored as Arc2D with 360° sweep)
- [x] Arc – Part of a circle (Arc2D)
- [x] Ellipse – Oval shape (Ellipse2D)
- [x] Spline – Smooth free-form curve (approximated as Line2D segments)
- [x] Point – Single point marker (Point2D)
- [x] Solid – Filled triangle/quad (outline as Line2D segments)

**Not Separate DXF Entities (handled via Polyline):**
- Rectangle – Four-sided shape (imported as LWPOLYLINE → 4 Line2D)
- Polygon – Regular shapes (imported as LWPOLYLINE → N Line2D)

**Not Yet Implemented (Complex entities):**
- [ ] Hatch – Filled area with patterns or solid fill
- [ ] Region – Closed area converted into a single surface
- [ ] Donut – Ring-shaped object (typically HATCH in DXF)

### 1.2 Geometry Math Utilities ✅
- [x] Distance point–point (GeometryMath.cpp:14-16, 18-20)
- [x] Distance point–line (GeometryMath.cpp:22-38, 40-43)
- [x] Distance point–arc (GeometryMath.cpp:45-48)
- [x] Angle normalization (GeometryMath.cpp:54-77)
- [x] Arc length validation (GeometryMath.cpp:103-131)
- [x] Intersection calculations (GeometryMath.cpp:153-317)

### 1.3 Geometry Safety Checks ✅
- [x] Zero-length line detection (Line2D::create() with MIN_LINE_LENGTH check)
- [x] Zero-radius arc detection (Arc2D::create() with MIN_ARC_RADIUS check)
- [x] Invalid arc angle detection (Arc2D::create() with MIN_ARC_SWEEP check)

---

## 2. SELECTION SYSTEM

### 2.1 Basic Selection ✅
**STATUS: COMPLETE**

- [x] Create SelectionManager class
  - SelectionManager.h/cpp implemented (include/ui/, src/ui/)
  - Maintains std::set<std::string> of selected entity handles
  - Methods: select(), deselect(), toggle(), clear(), isSelected(), selectedCount(), selectedHandles(), isEmpty()
- [x] Single click select (hit test within tolerance)
  - hitTest() implemented in CADCanvas (CADCanvas.cpp:831-862)
  - Screen-space tolerance: 10 pixels (zoom-independent)
  - Supports all entity types: Line2D, Arc2D, Ellipse2D, Point2D
  - Prioritizes closest entity within tolerance
- [x] Clear selection on empty click
  - Implemented in CADCanvas::mousePressEvent() (CADCanvas.cpp:735-747)
  - Clear + select pattern: always clears before selecting new
- [x] Selection highlight rendering (blue color)
  - Blue color #0066FF applied in renderLine/Arc/Ellipse/Point()
  - Selected entities rendered with width 3 (vs normal 2)
- [x] Track selected entities (IDs or indices)
  - Uses DXF handles (std::string) for stable identification
  - std::set<std::string> ensures uniqueness and O(log n) operations
- [x] Selection count in status bar
  - selectionChanged signal emitted on selection change
  - Status bar shows "Selected: N" indicator (main.cpp:154-159, 398-401)

**Deliverable**: ✅ Robust, predictable single-click selection system

### 2.2 Multi-Selection ✅
**STATUS: COMPLETE**

- [x] Shift + click toggle selection (CADCanvas.cpp:739-743)
- [x] Ctrl + click add to selection (CADCanvas.cpp:744-748)
- [x] Selection list management (add/remove/clear)
  - SelectionManager class (SelectionManager.h/cpp)
  - Methods: select(), deselect(), toggle(), clear(), selectedHandles()
- [x] Highlight all selected entities
  - Blue #0066FF highlight in renderLine/Arc/Ellipse/Point()
  - Selected entities rendered with width 3 (vs normal 2)

**Deliverable**: ✅ Complete multi-selection with Shift (toggle) and Ctrl (add) modifiers

### 2.3 Box Selection ✅
**STATUS: COMPLETE**

- [x] Left-drag box selection (inside only mode)
  - Left-click on empty space starts Inside mode (CADCanvas.cpp:756-765)
  - Solid blue rectangle visual feedback
- [x] Right-drag box selection (crossing mode)
  - Right-click on empty space starts Crossing mode (CADCanvas.cpp:769-782)
  - Dashed green rectangle visual feedback
- [x] Draw selection rectangle while dragging
  - renderSelectionBox() method (CADCanvas.cpp:954-980)
  - Semi-transparent fill with colored border
- [x] Bounding box intersection logic
  - Uses BoundingBox::containsBox() for Inside mode
  - Uses BoundingBox::intersects() for Crossing mode
- [x] Inside mode: entity fully inside box
  - getEntitiesInBox() with BoxSelectMode::Inside (CADCanvas.cpp:1009-1011)
- [x] Crossing mode: entity touches or crosses box
  - getEntitiesInBox() with BoxSelectMode::Crossing (CADCanvas.cpp:1012-1014)

**Deliverable**: ✅ Complete box selection with CAD-standard inside/crossing modes

### 2.4 Selection Visuals ✅
**STATUS: COMPLETE**

- [x] Blue highlight for selected entities (#0066FF)
  - Already implemented in 2.1 (renderLine/Arc/Ellipse/Point methods)
- [x] Draw bounding box around selection
  - renderSelectionBoundingBox() method (CADCanvas.cpp:729-785)
  - Dashed blue rectangle (#0066FF) around merged bounding box of selected entities
- [x] Grip points at corners (small squares)
  - renderGripPoints() method (CADCanvas.cpp:787-850)
  - 6x6 pixel filled blue squares at 4 corners of selection bounding box
- [x] Selection count in status bar
  - Already implemented in 2.1 (selectionChanged signal + status bar indicator)

**Deliverable**: ✅ Complete selection visual feedback with bounding box and grip points

---

## 3. DRAWING TOOLS

### 3.1 Line Tool
- [x] Create LineTool class
- [x] Click first point (snap-aware)
- [x] Move mouse → show preview line
- [x] Click second point → commit line
- [x] ESC to cancel
- [x] Add to DocumentModel
- [x] Validate: no zero-length lines

### 3.2 Arc Tool
- [ ] Create ArcTool class
- [ ] Center–start–end workflow
- [ ] Click 1: center point
- [ ] Click 2: start point (defines radius)
- [ ] Move mouse: preview arc
- [ ] Click 3: end point (defines sweep)
- [ ] Direction indicator (CCW default)
- [ ] ESC to cancel

### 3.3 Rectangle Helper
- [ ] Click first corner
- [ ] Move mouse: preview rectangle
- [ ] Click second corner: create 4 lines
- [ ] Internally creates Line2D entities
- [ ] No special "Rectangle" entity type

### 3.4 Tool Lifecycle
- [ ] Tool activate/deactivate
- [ ] Tool preview rendering
- [ ] Tool commit (add to document)
- [ ] Tool cancel (ESC key)
- [ ] Status bar shows tool prompts

**Deliverable**: Minimal, predictable creation tools

---

## 4. TRANSFORMATION TOOLS

### 4.1 Move Tool
- [ ] Create MoveTool class
- [ ] Select entities first
- [ ] Click base point
- [ ] Move mouse → show preview
- [ ] Click destination → commit move
- [ ] Numeric input option (dx, dy dialog)
- [ ] Snap respected for all points
- [ ] Validate: no invalid geometry after move

### 4.2 Rotate Tool
- [ ] Create RotateTool class
- [ ] Select entities first
- [ ] Click rotation center
- [ ] Move mouse → show preview rotation
- [ ] Click to set angle OR type angle
- [ ] Angle snap: 15°, 30°, 45°, 90°
- [ ] Preview before commit
- [ ] Validate: preserve arc direction

### 4.3 Mirror Tool
- [ ] Create MirrorTool class
- [ ] Select entities first
- [ ] X-axis mirror (horizontal)
- [ ] Y-axis mirror (vertical)
- [ ] Custom axis: click two points
- [ ] Preview overlay
- [ ] Option: keep or delete original
- [ ] Validate: preserve geometry

### 4.4 Transform Validation
- [ ] Precision preserved (no cumulative drift)
- [ ] No rounding errors
- [ ] Arc direction preserved
- [ ] Test: 360° rotation → identical geometry

**Deliverable**: Exact, repeatable transformations

---

## 5. UNDO / REDO SYSTEM

### 5.1 Command Pattern Architecture
- [ ] Create base Command interface
- [ ] execute() method
- [ ] undo() method
- [ ] redo() method

### 5.2 Command Implementations
- [ ] CreateEntityCommand (add Line/Arc)
- [ ] DeleteEntityCommand (remove entities)
- [ ] MoveEntitiesCommand (translate)
- [ ] RotateEntitiesCommand (rotate)
- [ ] MirrorEntitiesCommand (mirror)

### 5.3 Command Stack
- [ ] CommandHistory class
- [ ] Undo stack (std::vector)
- [ ] Redo stack (cleared on new command)
- [ ] Execute command → push to undo stack
- [ ] Undo → move to redo stack
- [ ] Redo → move back to undo stack

### 5.4 UI Integration
- [ ] Ctrl+Z → Undo
- [ ] Ctrl+Y → Redo
- [ ] Edit menu: Undo/Redo with descriptions
- [ ] Status bar shows undo/redo availability

### 5.5 Stress Testing
- [ ] Test 100+ undo/redo cycles
- [ ] Random operation sequences
- [ ] Memory leak testing
- [ ] Verify geometry integrity

**Deliverable**: Unbreakable Ctrl+Z

---

## 6. DUPLICATE & ZERO-LENGTH DETECTION

### 6.1 Detection Algorithms
- [ ] Detect overlapping lines (tolerance-based)
- [ ] Detect coincident arcs (same center, radius, angles)
- [ ] Detect zero-length lines
- [ ] Detect zero-radius arcs
- [ ] Store issue list with entity IDs

### 6.2 Background Validation
- [ ] Run after DXF import
- [ ] Run after every edit operation
- [ ] Async validation (don't block UI)
- [ ] Update issue count

### 6.3 Visual Feedback
- [ ] Highlight duplicates in muted yellow (#FFDD66)
- [ ] Issue counter in status bar
- [ ] Click issue → select problematic entity
- [ ] Validation panel (list of issues)

### 6.4 Manual Fix Tools
- [ ] "Remove Duplicates" button
- [ ] "Remove Zero-Length" button
- [ ] Confirmation dialog before delete
- [ ] Undo support for cleanup

**Deliverable**: Clean geometry without auto-guessing

---

## 7. UI STRUCTURE

### 7.1 Main Layout
- [ ] Fixed menu bar (File, Edit, View, Tools, Help)
- [ ] Left vertical toolbar (tools)
- [ ] Central CADCanvas widget
- [ ] Status bar (coordinates, snap, zoom, issues)

### 7.2 Toolbar Icons
- [ ] Select tool
- [ ] Line tool
- [ ] Arc tool
- [ ] Rectangle tool
- [ ] Move tool
- [ ] Rotate tool
- [ ] Mirror tool
- [ ] Delete tool
- [ ] Tool tips on hover

### 7.3 Status Bar Elements
- [ ] Units display (mm/cm/inches)
- [ ] Snap mode indicator
- [ ] Grid on/off indicator
- [ ] Cursor coordinates (world space)
- [ ] Zoom level
- [ ] Entity count
- [ ] Validation warning indicator

**Deliverable**: Industrial, distraction-free UI

---

## 8. DXF ROUND-TRIP SAFETY

### 8.1 Metadata Preservation
- [ ] Track DXF handle for each entity
- [ ] Preserve layer information
- [ ] Preserve entity order (where possible)
- [ ] Store source line numbers

### 8.2 Export Validation
- [ ] Entity count match (import vs export)
- [ ] Geometry precision check (tolerance-based)
- [ ] Layer integrity check
- [ ] No silent modifications

### 8.3 Re-import Verification
- [ ] Export DXF → Re-import DXF
- [ ] Visual diff (overlay original vs re-imported)
- [ ] Bounding box consistency check
- [ ] Automated test suite

**Deliverable**: Import → Edit → Export → Re-import = Identical

---

## 9. EXIT CRITERIA (PHASE 1 COMPLETE)

You may proceed to Phase 2 ONLY if ALL true:

- [ ] Real shop DXFs load and display correctly
- [ ] Geometry can be edited safely (no corruption)
- [ ] Undo/Redo works flawlessly (100+ cycles)
- [ ] Transform tools are trusted (repeatable, precise)
- [ ] Duplicate cleanup works reliably
- [ ] No DXF regression bugs (round-trip identical)
- [ ] UI is production-ready (no placeholders)
- [ ] Manual testing by fabrication operator passes

---

## DAILY WORKFLOW

1. Pick ONE task from this file
2. Mark as [⏳] in progress
3. Implement + write test
4. Verify manually with real DXF
5. Mark as [✅] complete
6. Git commit with clear message
7. Move to next task

**NEVER:**
- Skip tests
- Commit broken code
- Start new task before finishing current
- Add features not in this list

---

END OF PHASE 1 TASKS
