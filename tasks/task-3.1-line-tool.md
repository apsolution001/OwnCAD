# Task 3.1 - Line Tool Implementation Guide

## Overview

The Line Tool is the first drawing tool in OwnCAD. Its implementation establishes the **tool architecture pattern** that will be reused for Arc Tool, Rectangle Helper, Move Tool, Rotate Tool, and Mirror Tool.

**This is a foundational task** - architectural decisions made here affect all future tools.

---

## Subtask Breakdown

### Phase A: Tool Architecture Foundation (3 subtasks)

#### A.1 Create Tool Base Interface
**Files to create:**
- `include/ui/Tool.h` - Abstract base class

**Responsibilities:**
- Define common tool interface (activate, deactivate, handleMousePress, handleMouseMove, handleMouseRelease, handleKeyPress)
- Define tool state enum (Idle, Active, WaitingForInput)
- Define tool result struct (success/cancel/continue)
- Virtual render method for previews

**Why this first:** Every tool needs this interface. Establishing it now ensures consistency.

#### A.2 Create ToolManager Class
**Files to create:**
- `include/ui/ToolManager.h`
- `src/ui/ToolManager.cpp`

**Responsibilities:**
- Hold current active tool pointer
- Route mouse/keyboard events to active tool
- Handle tool switching (deactivate old, activate new)
- Emit signals for status bar updates

**Why separate from CADCanvas:** Separation of concerns. CADCanvas handles rendering and viewport; ToolManager handles tool state machine.

#### A.3 Integrate ToolManager into CADCanvas
**Files to modify:**
- `include/ui/CADCanvas.h`
- `src/ui/CADCanvas.cpp`

**Changes:**
- Add ToolManager member
- Forward mouse events to ToolManager when tool active
- Forward key events to ToolManager
- Call tool->render() in paintEvent

---

### Phase B: Line Tool Core Implementation (4 subtasks)

#### B.1 Create LineTool Class (State Machine)
**Files to create:**
- `include/ui/LineTool.h`
- `src/ui/LineTool.cpp`

**State Machine:**
```
[Idle] --activate()--> [WaitingFirstPoint]
[WaitingFirstPoint] --click()--> [WaitingSecondPoint]
[WaitingSecondPoint] --click()--> [Commit] --> [WaitingFirstPoint] (continuous)
[Any] --ESC--> [Idle]
[Any] --deactivate()--> [Idle]
```

**Data Members:**
- `std::optional<Point2D> firstPoint_` - First clicked point (after snap)
- `std::optional<Point2D> previewEndPoint_` - Current mouse position (after snap)
- `State state_` - Current state enum

#### B.2 Implement Snap Integration
**Dependencies:** Existing SnapManager in CADCanvas

**Implementation:**
- LineTool receives snapped points from CADCanvas (via SnapManager)
- LineTool does NOT duplicate snap logic
- CADCanvas calls `snap()` on mouse move and passes result to active tool

**Interface:**
```cpp
void LineTool::setCurrentSnapPoint(const std::optional<Point2D>& point);
```

#### B.3 Implement Preview Rendering
**In LineTool:**
```cpp
void LineTool::render(QPainter& painter, const Viewport& viewport);
```

**Visual Specification:**
- Dashed line from `firstPoint_` to `previewEndPoint_`
- Color: Light blue (#66AAFF) - distinguishable from committed geometry
- Width: 1 pixel (screen space)
- Small circle at first point (6px diameter, same color)

#### B.4 Implement Commit Logic
**Validation:**
- Check `Line2D::create(firstPoint, secondPoint)` returns valid optional
- If returns `std::nullopt` (zero-length), reject and show status message

**Commit:**
- Create `Line2D` via factory
- Add to `DocumentModel` via new method `addEntity()`
- Emit signal for canvas redraw
- Reset to WaitingFirstPoint for continuous drawing

---

### Phase C: DocumentModel Integration (2 subtasks)

#### C.1 Add Entity Creation Method to DocumentModel
**Files to modify:**
- `include/model/DocumentModel.h`
- `src/model/DocumentModel.cpp`

**New Method:**
```cpp
std::string DocumentModel::addLine(const Line2D& line, const std::string& layer = "0");
```

**Returns:** Generated handle (for undo/redo support later)

**Considerations:**
- Generate unique handle (e.g., incrementing counter or UUID)
- Store in internal entity collection
- Emit signal: `entityAdded(const std::string& handle)`

#### C.2 Wire LineTool to DocumentModel
**Implementation:**
- LineTool holds pointer/reference to DocumentModel
- On commit, call `model->addLine(line)`
- Handle success/failure

---

### Phase D: UI Integration (3 subtasks)

#### D.1 Add Line Tool Activation to Menu/Shortcut
**Files to modify:**
- `src/main.cpp`

**Add:**
- Menu item: Tools → Line (or Draw → Line)
- Keyboard shortcut: `L`
- Connect to `toolManager->activateTool("line")`

#### D.2 Status Bar Prompts
**Messages by state:**
- WaitingFirstPoint: "LINE: Click first point"
- WaitingSecondPoint: "LINE: Click second point (ESC to cancel)"
- After commit: "LINE: Line created. Click next point or ESC to exit"

**Implementation:**
- Tool emits signal with prompt text
- MainWindow connects signal to status bar

#### D.3 Cursor Feedback
**Cursor changes:**
- Line tool active: Crosshair cursor (`Qt::CrossCursor`)
- Hovering over snap point: Crosshair (snap indicator already visible)

---

### Phase E: Keyboard Handling (1 subtask)

#### E.1 ESC Key Cancellation
**Behavior:**
- If in WaitingSecondPoint: Cancel current line, return to WaitingFirstPoint
- If in WaitingFirstPoint: Deactivate tool, return to selection mode
- Clear preview data
- Update status bar

---

### Phase F: Testing & Validation (2 subtasks)

#### F.1 Manual Testing Checklist
- [ ] Click point A, click point B → Line appears
- [ ] Snap to endpoint → First point snaps correctly
- [ ] Snap to grid → Both points snap correctly
- [ ] Preview updates as mouse moves
- [ ] ESC during drawing cancels
- [ ] ESC with no first point exits tool
- [ ] Zero-length line rejected (click same point twice)
- [ ] Continuous drawing works (after commit, can draw next line)
- [ ] Status bar shows correct prompts
- [ ] Tool switch (L key) works

#### F.2 Edge Cases
- [ ] Click exactly on existing line endpoint
- [ ] Draw at extreme zoom levels (0.01 and 100)
- [ ] Draw outside visible viewport (pan afterward - line exists)
- [ ] Rapid clicking (no double-commit)

---

## File Structure Update (Proposed)

After implementation, File-Structure.md should include:

```
### UI (`ui/`)
...existing...
- `Tool.h`: Abstract base class for all drawing/editing tools.
- `ToolManager.h/cpp`: Manages active tool, routes events, handles tool switching.
- `LineTool.h/cpp`: Line drawing tool implementation.
```

---

## Implementation Order (Recommended)

| Step | Subtask | Estimated Complexity |
|------|---------|---------------------|
| 1 | A.1 Tool Base Interface | Low |
| 2 | A.2 ToolManager Class | Medium |
| 3 | A.3 Integrate into CADCanvas | Medium |
| 4 | B.1 LineTool State Machine | Medium |
| 5 | B.2 Snap Integration | Low |
| 6 | C.1 DocumentModel addLine | Low |
| 7 | B.3 Preview Rendering | Low |
| 8 | B.4 Commit Logic | Low |
| 9 | C.2 Wire to DocumentModel | Low |
| 10 | D.1 Menu/Shortcut | Low |
| 11 | D.2 Status Bar Prompts | Low |
| 12 | D.3 Cursor Feedback | Low |
| 13 | E.1 ESC Handling | Low |
| 14 | F.1-F.2 Testing | - |

---

## Architectural Decisions (For Discussion)

### Decision 1: Tool Ownership
**Options:**
1. ToolManager owns tools (creates/destroys)
2. Tools are singletons managed externally
3. CADCanvas owns ToolManager, ToolManager owns Tools

**Recommendation:** Option 3 - clear ownership chain, easy lifetime management.

### Decision 2: Event Routing
**Options:**
1. CADCanvas forwards all events to ToolManager
2. CADCanvas handles selection, ToolManager handles tools (mode-based)
3. Hybrid: CADCanvas decides based on active tool

**Recommendation:** Option 2 - cleaner separation. When a tool is active, CADCanvas defers to ToolManager. When no tool active, CADCanvas handles selection.

### Decision 3: Continuous vs Single-Shot Drawing
**Options:**
1. After drawing one line, tool remains active (continuous)
2. After drawing one line, return to selection mode

**Recommendation:** Option 1 (continuous) - matches AutoCAD behavior. User presses ESC to exit.

### Decision 4: Preview Line Style
**Options:**
1. Dashed line (standard CAD convention)
2. Solid line with different color
3. Rubber-band style (XOR drawing - outdated)

**Recommendation:** Option 1 - dashed, light blue, matches industry standard.

---

## Dependencies

**Existing code to use:**
- `Line2D::create()` - Factory with validation
- `SnapManager` - Already in CADCanvas
- `Point2D` - Geometry primitive
- `Viewport::worldToScreen()` / `screenToWorld()` - Coordinate transforms

**No new external dependencies required.**

---

## Questions for Discussion

1. **Continuous drawing:** Should pressing Enter/Space also commit and continue? (AutoCAD does this)

2. **Right-click behavior:** Should right-click cancel? Or open context menu?

3. **Numeric input:** Should we support typing coordinates? (e.g., "100,50" for relative, "@100,50" for absolute) - This could be deferred to later.

4. **Layer assignment:** New lines go to which layer? (Current active layer concept needed?)

5. **Line properties:** Should line tool have a properties panel (color, width)? Or use document defaults?

---

## Success Criteria

Task 3.1 is complete when:
- [x] User can activate Line tool (menu or L key)
- [x] Click first point (snaps work)
- [x] Preview line shown while moving mouse
- [x] Click second point commits line
- [x] Line appears in document (persists after tool deactivation)
- [x] ESC cancels operation
- [x] Zero-length lines rejected
- [x] Status bar shows appropriate prompts
- [x] Tool architecture established for reuse
