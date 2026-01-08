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

### 0.4 Snap System ⏳
**GOAL: Precise point input for drawing/editing**

- [ ] Create SnapManager class
- [ ] Implement snap modes:
  - [ ] Grid snap (configurable spacing: 1, 5, 10, 25mm)
  - [ ] Endpoint snap (detect entity endpoints within tolerance)
  - [ ] Midpoint snap (detect line/arc midpoints)
  - [ ] Nearest snap (snap to nearest point on entity)
- [ ] Visual snap indicators
  - [ ] Draw snap marker at snap point
  - [ ] Different colors for different snap types
- [ ] Snap settings UI (toolbar or status bar toggles)
- [ ] Snap priority system (endpoint > midpoint > grid)

**Deliverable**: Cursor snaps to meaningful points

### 0.5 Grid Rendering ⏳
**GOAL: Visual reference grid**

- [ ] Render background grid
  - [ ] Major grid lines (every 10 units)
  - [ ] Minor grid lines (every 1 unit)
  - [ ] Adaptive spacing based on zoom level
- [ ] Grid styling
  - [ ] Major: darker gray (#999)
  - [ ] Minor: lighter gray (#ddd)
  - [ ] Background: white or light gray
- [ ] Grid visibility toggle (View menu)
- [ ] Grid settings
  - [ ] Units: mm, cm, inches
  - [ ] Spacing: configurable
- [ ] Origin indicator (red/green axes at 0,0)

**Deliverable**: Clean, professional grid background

---

## 1. CORE GEOMETRY STABILITY

### 1.1 Geometry Primitives Hardening ✅
- [x] Line2D defined with start, end
- [x] Arc2D defined with center, radius, angles
- [x] Bounding box calculation
- [x] Tolerance-based equality

### 1.2 Geometry Math Utilities ✅
- [x] Distance point–point
- [x] Distance point–line
- [x] Distance point–arc
- [x] Angle normalization
- [x] Arc length validation
- [x] Intersection calculations

### 1.3 Geometry Safety Checks ✅
- [x] Zero-length line detection
- [x] Zero-radius arc detection
- [x] Invalid arc angle detection

---

## 2. SELECTION SYSTEM

### 2.1 Basic Selection
- [ ] Create SelectionManager class
- [ ] Single click select (hit test within tolerance)
- [ ] Clear selection on empty click
- [ ] Selection highlight rendering (blue color)
- [ ] Track selected entities (IDs or indices)

### 2.2 Multi-Selection
- [ ] Shift + click toggle selection
- [ ] Ctrl + click add to selection
- [ ] Selection list management (add/remove/clear)
- [ ] Highlight all selected entities

### 2.3 Box Selection
- [ ] Left-drag box selection (inside only mode)
- [ ] Right-drag box selection (crossing mode)
- [ ] Draw selection rectangle while dragging
- [ ] Bounding box intersection logic
- [ ] Inside mode: entity fully inside box
- [ ] Crossing mode: entity touches or crosses box

### 2.4 Selection Visuals
- [ ] Blue highlight for selected entities (#0066FF)
- [ ] Draw bounding box around selection
- [ ] Grip points at corners (small squares)
- [ ] Selection count in status bar

**Deliverable**: Robust, predictable selection system

---

## 3. DRAWING TOOLS

### 3.1 Line Tool
- [ ] Create LineTool class
- [ ] Click first point (snap-aware)
- [ ] Move mouse → show preview line
- [ ] Click second point → commit line
- [ ] ESC to cancel
- [ ] Add to DocumentModel
- [ ] Validate: no zero-length lines

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
