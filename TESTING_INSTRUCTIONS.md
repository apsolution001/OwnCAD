# Testing Instructions - Task 0.2: CAD Canvas with Geometry Rendering

## Goal
Verify that loaded DXF geometry is rendered correctly on screen.

## Test Files Provided

### 1. `test_square_100x100.dxf`
- **Contents**: Perfect 100x100 square made of 4 lines
- **Expected Result**: You should see a complete square
- **Lines**:
  - Bottom edge: (0,0) to (100,0)
  - Right edge: (100,0) to (100,100)
  - Top edge: (100,100) to (0,100)
  - Left edge: (0,100) to (0,0)

### 2. `test_shapes.dxf`
- **Contents**: Multiple shapes to test full rendering pipeline
- **Expected Result**: You should see:
  - A 100x100 square (4 lines)
  - A semicircular arc (180°)
  - A circle (converted to full 360° arc internally)
- **Total**: 4 lines + 1 arc + 1 circle (6 entities)

### 3. `test_sample.dxf` (existing)
- **Contents**: Mixed entities including zero-length line
- **Expected Result**: Multiple lines, arcs, circles with validation warnings

## How to Test

### Step 1: Build the Application
```bash
# Build using your Qt Creator or CMake
cd build/Desktop_Qt_6_10_1_MinGW_64_bit-Debug
cmake --build . --config Debug
```

### Step 2: Run OwnCAD
```bash
./OwnCAD.exe
# or
./Debug/OwnCAD.exe
```

### Step 3: Open Test File
1. Click **File → Open DXF...**
2. Select `test_square_100x100.dxf`
3. **Expected behavior**:
   - Status bar shows: "Loaded: 4 entities (4 lines, 0 arcs)"
   - You should SEE a complete 100x100 square on the canvas
   - Grid should be visible in background
   - Origin axes (red X, green Y) at (0,0)

### Step 4: Verify All Entities Render
Open `test_shapes.dxf`:
- Status bar should show: "Loaded: 6 entities (4 lines, 2 arcs)"
- You should see:
  - Complete square on the left
  - Semicircular arc in the middle
  - Full circle on the right

### Step 5: Check Debug Console
The application now outputs debug information:
```
DXF Import Success:
  Total entities loaded: 4
  Lines: 4
  Arcs: 0
  Valid: 4
  Invalid: 0
CADCanvas::setEntities() - Received 4 entities
CADCanvas::renderEntities() - Rendering 4 entities
  Lines rendered: 4
  Arcs rendered: 0
```

**If you see "Received 4 entities" but only 1 line on screen**: This is a rendering bug.
**If you see "Received 1 entity"**: This is an import bug.

## Viewport Controls (Already Implemented)

- **Pan**: Middle mouse button drag
- **Zoom**: Mouse wheel
- **Zoom Extents**: View → Zoom Extents (or press when you open a file)
- **Reset View**: View → Reset View
- **Toggle Grid**: View → Toggle Grid

## Task 0.2 Checklist

✅ **CADCanvas widget class** - Implemented (483 lines)
✅ **Viewport transformation system** - Implemented
  - ✅ worldToScreen(Point2D) → QPointF
  - ✅ screenToWorld(QPointF) → Point2D
✅ **Implement basic geometry rendering** - Implemented
  - ✅ Render Line2D entities
  - ✅ Render Arc2D entities
  - ✅ Apply proper line styles (width, color)
✅ **Replace MainWindow placeholder with CADCanvas** - Done
✅ **Wire up DocumentModel to canvas** - Done
✅ **Debug logging added** - Lines 163-168 in main.cpp, lines 198-199 and 377-393 in CADCanvas.cpp

## Expected Console Output (Example)

```
DXF Import Success:
  Total entities loaded: 4
  Lines: 4
  Arcs: 0
  Valid: 4
  Invalid: 0
CADCanvas::setEntities() - Received 4 entities
CADCanvas::renderEntities() - Rendering 4 entities
  Lines rendered: 4
  Arcs rendered: 0
```

## Troubleshooting

### Issue: "Only see 1 line instead of 4"

**Root Cause Investigation:**
1. Check console output - how many entities were loaded?
2. If console shows "Loaded: 4 entities" but you only see 1:
   - **Problem**: Rendering issue (lines overlap or viewport issue)
   - **Solution**: Try "View → Zoom Extents" or zoom out with mouse wheel
3. If console shows "Loaded: 1 entity":
   - **Problem**: DXF import issue (parser or conversion bug)
   - **Solution**: Check the DXF file structure

### Issue: "Geometry is very small or invisible"
- **Solution**: Use "View → Zoom Extents" to fit all geometry to screen
- The viewport auto-calculates zoom level based on bounding box

### Issue: "Grid is too dense or sparse"
- **Solution**: Grid adapts to zoom level automatically
- Adjust grid spacing: Currently fixed at 10 units

## Code Changes Made

### 1. `src/main.cpp` - Enhanced Import Feedback
- Added `#include <QDebug>`
- Lines 163-168: Debug output showing entity counts
- Lines 176-180: Status bar shows detailed entity breakdown

### 2. `src/ui/CADCanvas.cpp` - Rendering Diagnostics
- Added `#include <QDebug>`
- Line 199: Log entity count when setEntities() is called
- Lines 377-393: Log rendering details (line count, arc count)

### 3. Test Files Created
- `test_square_100x100.dxf` - Perfect square for basic testing
- `test_shapes.dxf` - Multiple shapes for comprehensive testing

## Success Criteria (Task 0.2 Complete)

✅ Open `test_square_100x100.dxf` → See complete square
✅ Open `test_shapes.dxf` → See square + arc + circle
✅ Status bar shows correct entity counts
✅ Console shows all entities loaded and rendered
✅ Pan and zoom work smoothly
✅ Grid renders in background

---

**If all above criteria pass**: Task 0.2 is COMPLETE ✅
**Deliverable**: Geometry visible on screen after DXF import
