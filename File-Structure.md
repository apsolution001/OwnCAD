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

### Import/Export (`import/`)
File format handling, currently focused on DXF.
- `DXFEntity.h`: Data structures reflecting raw DXF entity properties.
- `DXFParser.h/cpp`: Parses DXF text files into `DXFEntity` structures.
- `DXFColors.h/cpp`: DXF color index to RGB mappings.
- `GeometryConverter.h/cpp`: Converts raw `DXFEntity` objects into internal `geometry` classes, including polyline bulge-to-arc conversion.

### Model (`model/`)
Data management and application state.
- `DocumentModel.h/cpp`: Manages the collection of all geometric entities in the active document.

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

### Main
- `src/main.cpp`: Application entry point; initializes `MainWindow` and the application loop.

  - MainWindow: menu bar, central CADCanvas, status bar with cursor position/zoom/snap/selection indicators.

## Tests (`tests/`)
Unit tests using Google Test framework.
- `tests/geometry/`: Tests for specific geometric primitives.
- `tests/ui/`: Tests for UI components (e.g., Viewport transformations).

## Tasks (`tasks/`)
Project planning and progress tracking.
- `phase1.tasks.md`: Detailed checklist for Phase 1 development.
- `IMPLEMENTATION_SUMMARY.md`: Summary of implemented features and current status.