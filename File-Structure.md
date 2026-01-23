# File Structure

## Project Root
- `CMakeLists.txt`: Main CMake build configuration file.
- `README.md`: Project overview and setup instructions.
- `CLAUDE.md`: Assistant-specific guidelines and context.
- `File-Structure.md`: Detailed file descriptions (this file).

## Source Code (`src/` and `include/`)

### Geometry (`geometry/`)
Core geometric primitives and math utilities.
- `Point2D.h/cpp`: Represents a 2D point with coordinate operations.
- `Line2D.h/cpp`: Represents a line segment defined by two points.
- `Arc2D.h/cpp`: Represents a circular arc defined by center, radius, and angles.
- `Ellipse2D.h/cpp`: Represents an elliptical arc.
- `BoundingBox.h/cpp`: Axis-Aligned Bounding Box (AABB) for efficiency calculations.
- `GeometryConstants.h`: Mathematical constants and global tolerances (e.g., epsilon).
- `GeometryMath.h/cpp`: Utility functions for intersection, distance, and vector math.
  - Distance calculations: point-to-point, point-to-segment, point-to-arc, point-to-ellipse.
  - Angle utilities: normalization, sweep calculations, angle-between checks.
  - Intersection calculations: line-line, segment-segment, line-arc, arc-arc.
  - Projection/closest point: on segment, on arc, on ellipse.
- `GeometryValidator.h/cpp`: Helper class for geometry validation checks.
- `TransformValidator.h/cpp`: Validation utilities for geometric transformations.
  - Validates precision preservation after translate/rotate operations.
  - Detects cumulative drift from repeated transformations.
  - Verifies arc direction (CCW/CW) is preserved (critical for CNC toolpaths).
  - Round-trip validation: transform → inverse → compare to original.
  - 360° rotation identity tests.

### Import/Export (`import/`)
File format handling, currently focused on DXF.
- `DXFEntity.h`: Data structures reflecting raw DXF entity properties.
- `DXFParser.h/cpp`: Parses DXF text files into `DXFEntity` structures.
- `DXFColors.h/cpp`: DXF color index to RGB mappings.
- `GeometryConverter.h/cpp`: Converts raw `DXFEntity` objects into internal `geometry` classes, including polyline bulge-to-arc conversion.

### Model (`model/`)
Data management and application state.
- `DocumentModel.h/cpp`: Manages the collection of all geometric entities in the active document.
- `Command.h`: Interface for the Command pattern (Undo/Redo support).
  - Pure virtual methods: `execute()`, `undo()`, `redo()`.
  - Properties: `name()`, `mergeId()`, `canMergeWith()`.
- `EntityCommands.h/cpp`: Concrete implementations of `Command` for entity operations.
  - `CreateEntityCommand`: Adds a single primitive (Line, Arc, etc.).
  - `CreateEntitiesCommand`: Adds multiple entities (e.g., Rectangle).
  - `DeleteEntityCommand` / `DeleteEntitiesCommand`: Removes entities.
  - `MoveEntitiesCommand`: Translates entities.
  - `RotateEntitiesCommand`: Rotates entities.
  - `MirrorEntitiesCommand`: Mirrors entities.
- `CommandHistory.h/cpp`: Manages the undo/redo stacks.
  - Stores unique_ptr to executed commands.
  - Handles stack limits and state notifications.

### UI (`ui/`)
User interface components and interaction logic.
- `CADCanvas.h/cpp`: Custom Qt widget responsible for rendering geometry and handling user interaction.
  - Viewport: coordinate transformation (world ↔ screen), pan, zoom.
  - SnapManager: grid/endpoint/midpoint/nearest snap with visual feedback.
  - Rendering: grid, origin axes, geometry entities, snap indicators, selection highlights.
  - Selection visuals: bounding box (dashed blue rectangle), grip points (filled blue squares at corners).
  - Hit testing: finds entities near click point (supports Line2D, Arc2D, Ellipse2D, Point2D).
  - Selection: single-click, Shift+click (toggle), Ctrl+click (add), box selection (left-drag=Inside, right-drag=Crossing).
- `SelectionManager.h/cpp`: Manages the set of selected entity handles.
  - Tracks selection state using std::set<std::string> (DXF handles).
  - Methods: select(), deselect(), toggle(), clear(), isSelected(), selectedCount().
- `GridSettingsDialog.h/cpp`: Dialog for configuring grid spacing and visual settings.
- `Tool.h`: Abstract base class for all drawing and editing tools.
  - Defines tool interface: activate(), deactivate(), handleMouse/Key events, render().
  - Tool state enum: Inactive, WaitingForInput, InProgress.
  - Tool result enum: Continue, Completed, Cancelled, Ignored.
- `ToolManager.h/cpp`: Manages drawing and editing tools.
  - Owns and registers tool instances.
  - Routes mouse/keyboard events to active tool.
  - Handles tool switching and emits signals for status bar updates.
- `LineTool.h/cpp`: Line drawing tool implementation.
  - State machine: WaitingFirstPoint → WaitingSecondPoint → Commit.
  - Snap-aware point input, preview rendering (dashed line).
  - Continuous drawing mode, ESC to cancel.
- `ArcTool.h/cpp`: Arc drawing tool implementation.
  - State machine: WaitingCenterPoint → WaitingStartPoint → WaitingEndPoint → Commit.
  - Center-Start-End workflow (3 clicks).
  - Direction toggle (D key), CCW default.
  - Preview rendering with direction indicator arrow.
- `RectangleTool.h/cpp`: Rectangle drawing tool implementation.
  - State machine: WaitingFirstCorner → WaitingSecondCorner → Commit.
  - Two-click workflow, creates 4 Line2D entities (not a special type).
  - Preview rendering with 4 dashed lines.
  - Continuous drawing mode, ESC to cancel.
- `MoveTool.h/cpp`: Move tool implementation for translating entities.
  - State machine: Inactive → WaitingForBasePoint → InProgress (Moving) → Commit.
  - Workflow: Select entities → Activate → Click Base → Drag/Preview → Click Destination.
  - Features: Snapping support, real-time preview of all selected entities, transaction safety.
  - Uses `GeometryMath::translate` for actual geometry modification.
- `RotateTool.h/cpp`: Rotate tool implementation for rotating entities around a center point.
  - State machine: Inactive → WaitingForCenter → InProgress (WaitingForAngle) → Commit.
  - Workflow: Select entities → Activate → Click Center → Drag/Preview → Click or Tab for angle.
  - Features: Shift for 15° angle snap, Tab/Enter for exact numeric angle input dialog.
  - Uses `GeometryMath::rotate` for actual geometry modification.
- `RotateInputDialog.h/cpp`: Dialog for entering exact rotation angle in degrees.
  - Allows numeric input for precise rotation (positive = CCW, negative = CW).
  - Pre-fills with current preview angle when opened.
- `MirrorTool.h/cpp`: Mirror tool implementation.
  - State machine: Inactive → WaitingForSelection → WaitingForAxisStart → WaitingForAxisEnd → Commit.
  - Workflow: Select entities → Activate → Click Axis Points (or X/Y keys).
  - Features: Previewing mirrored geometry, toggle keep original (Tab), X/Y axis shortcuts.

### Main
- `src/main.cpp`: Application entry point; initializes `MainWindow` and the application loop.

  - MainWindow: menu bar, central CADCanvas, status bar with cursor position/zoom/snap/selection indicators.

## Tests (`tests/`)
Unit tests using Qt Test framework.
- `tests/geometry/`: Tests for geometric primitives and utilities.
  - `test_Point2D.cpp`: Point2D construction, equality, distance.
  - `test_Line2D.cpp`: Line2D validation, length, containment.
  - `test_Arc2D.cpp`: Arc2D angles, sweep, direction.
  - `test_GeometryMath.cpp`: Distance, angle, tolerance utilities.
  - `test_Intersections.cpp`: Line-line, line-arc, arc-arc intersections.
  - `test_TransformValidator.cpp`: Transform precision, cumulative drift, round-trip validation.
- `tests/ui/`: Tests for UI components (e.g., Viewport transformations).

## Tasks (`tasks/`)
Project planning and progress tracking.
- `phase1.tasks.md`: Detailed checklist for Phase 1 development.
- `IMPLEMENTATION_SUMMARY.md`: Summary of implemented features and current status.