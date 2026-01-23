

















# Task 3: Drawing Tools - Implementation Guide

## Overview

Section 3 covers all drawing tools for creating geometry in the CAD application.

| Task | Status | Description |
|------|--------|-------------|
| 3.1 Line Tool | ✅ COMPLETE | Two-click line creation |
| 3.2 Arc Tool | ⚠️ FIX NEEDED | Three-click arc creation (compile error) |
| 3.3 Rectangle Helper | ❌ NOT STARTED | Two-click rectangle (4 lines) |
| 3.4 Tool Lifecycle | ✅ MOSTLY DONE | Base infrastructure exists |

---

## 3.2 Arc Tool - FIX REQUIRED

### Current Issue
File `src/ui/ArcTool.cpp` has a compile error at line 74:
```
error: 'distance' is not a member of 'OwnCAD::Geometry::Point2D'
```

### Root Cause
Used `Point2D::distance()` (static method that doesn't exist) instead of `point.distanceTo(other)` (instance method).

### Fix Required
**File:** `src/ui/ArcTool.cpp`, line 74

**Change from:**
```cpp
double dist = Geometry::Point2D::distance(*centerPoint_, worldPos);
```

**Change to:**
```cpp
double dist = centerPoint_->distanceTo(worldPos);
```

### Verification
After fix, Arc Tool should:
1. Click 1: Set center point (blue marker)
2. Click 2: Set start point (green marker, defines radius)
3. Mouse move: Preview arc with direction arrow
4. Press D: Toggle CCW/CW direction
5. Click 3: Commit arc to document
6. ESC: Cancel current operation

---

## 3.3 Rectangle Helper - FULL IMPLEMENTATION

### Concept
Rectangle tool creates 4 Line2D entities (not a special Rectangle type).
This is standard CAD practice - keeps geometry simple and exportable.

### Workflow
1. Click first corner
2. Move mouse → preview rectangle (4 dashed lines)
3. Click second corner → create 4 lines, add to document
4. ESC to cancel

### Subtasks

#### 3.3.1 Create RectangleTool Header
**File:** `include/ui/RectangleTool.h`

```cpp
#pragma once

#include "ui/Tool.h"
#include "geometry/Point2D.h"
#include <optional>

namespace OwnCAD {
namespace UI {

/**
 * @brief Rectangle drawing tool (creates 4 Line2D entities)
 *
 * Workflow:
 * 1. Activate tool
 * 2. Click first corner (snap-aware)
 * 3. Move mouse - preview rectangle shown
 * 4. Click second corner - 4 lines committed
 * 5. Tool stays active for continuous drawing
 * 6. ESC to cancel
 *
 * State machine:
 * [Inactive] --activate()--> [WaitingFirstCorner]
 * [WaitingFirstCorner] --click--> [WaitingSecondCorner]
 * [WaitingSecondCorner] --click--> [Commit 4 lines] --> [WaitingFirstCorner]
 * [WaitingSecondCorner] --ESC--> [WaitingFirstCorner]
 * [WaitingFirstCorner] --ESC--> [Inactive]
 */
class RectangleTool : public Tool {
public:
    RectangleTool();
    ~RectangleTool() override = default;

    // Tool interface
    QString name() const override { return QStringLiteral("Rectangle"); }
    QString id() const override { return QStringLiteral("rectangle"); }

    void activate() override;
    void deactivate() override;

    ToolState state() const override { return state_; }
    QString statusPrompt() const override;

    // Event handlers
    ToolResult handleMousePress(const Geometry::Point2D& worldPos, QMouseEvent* event) override;
    ToolResult handleMouseMove(const Geometry::Point2D& worldPos, QMouseEvent* event) override;
    ToolResult handleMouseRelease(const Geometry::Point2D& worldPos, QMouseEvent* event) override;
    ToolResult handleKeyPress(QKeyEvent* event) override;

    // Rendering
    void render(QPainter& painter, const Viewport& viewport) override;

private:
    bool commitRectangle();
    void resetToFirstCorner();
    void cancel();

    // State
    ToolState state_ = ToolState::Inactive;

    // Corners
    std::optional<Geometry::Point2D> firstCorner_;
    std::optional<Geometry::Point2D> currentCorner_;

    // Visual settings
    static constexpr int PREVIEW_LINE_WIDTH = 1;
    static constexpr int CORNER_MARKER_SIZE = 6;
};

} // namespace UI
} // namespace OwnCAD
```

#### 3.3.2 Create RectangleTool Implementation
**File:** `src/ui/RectangleTool.cpp`

Key implementation points:

```cpp
#include "ui/RectangleTool.h"
#include "ui/CADCanvas.h"
#include "model/DocumentModel.h"
#include "geometry/Line2D.h"
#include <QPainter>
#include <QDebug>

// ... standard Tool methods (activate, deactivate, state, statusPrompt) ...

bool RectangleTool::commitRectangle() {
    if (!firstCorner_ || !currentCorner_ || !documentModel_) {
        return false;
    }

    // Calculate 4 corner points
    double x1 = firstCorner_->x();
    double y1 = firstCorner_->y();
    double x2 = currentCorner_->x();
    double y2 = currentCorner_->y();

    // Validate: rectangle must have non-zero area
    if (std::abs(x2 - x1) < Geometry::MIN_LINE_LENGTH ||
        std::abs(y2 - y1) < Geometry::MIN_LINE_LENGTH) {
        qWarning() << "RectangleTool: Rectangle too small";
        return false;
    }

    // Create 4 corners
    Geometry::Point2D p1(x1, y1);  // First corner (given)
    Geometry::Point2D p2(x2, y1);  // Top-right (or bottom-right)
    Geometry::Point2D p3(x2, y2);  // Second corner (given)
    Geometry::Point2D p4(x1, y2);  // Bottom-left (or top-left)

    // Create 4 lines (counterclockwise order)
    auto line1 = Geometry::Line2D::create(p1, p2);  // Bottom
    auto line2 = Geometry::Line2D::create(p2, p3);  // Right
    auto line3 = Geometry::Line2D::create(p3, p4);  // Top
    auto line4 = Geometry::Line2D::create(p4, p1);  // Left

    // Validate all lines created successfully
    if (!line1 || !line2 || !line3 || !line4) {
        qWarning() << "RectangleTool: Failed to create lines";
        return false;
    }

    // Add all 4 lines to document
    documentModel_->addLine(*line1);
    documentModel_->addLine(*line2);
    documentModel_->addLine(*line3);
    documentModel_->addLine(*line4);

    qDebug() << "RectangleTool: Created rectangle with 4 lines";
    return true;
}

void RectangleTool::render(QPainter& painter, const Viewport& viewport) {
    if (state_ != ToolState::InProgress || !firstCorner_ || !currentCorner_) {
        return;
    }

    painter.save();

    // Preview style
    QPen previewPen(QColor(102, 170, 255));
    previewPen.setWidth(PREVIEW_LINE_WIDTH);
    previewPen.setStyle(Qt::DashLine);
    previewPen.setDashPattern({6, 4});
    painter.setPen(previewPen);

    // Calculate screen coordinates for all 4 corners
    double x1 = firstCorner_->x();
    double y1 = firstCorner_->y();
    double x2 = currentCorner_->x();
    double y2 = currentCorner_->y();

    QPointF s1 = viewport.worldToScreen(Geometry::Point2D(x1, y1));
    QPointF s2 = viewport.worldToScreen(Geometry::Point2D(x2, y1));
    QPointF s3 = viewport.worldToScreen(Geometry::Point2D(x2, y2));
    QPointF s4 = viewport.worldToScreen(Geometry::Point2D(x1, y2));

    // Draw 4 preview lines
    painter.drawLine(s1, s2);
    painter.drawLine(s2, s3);
    painter.drawLine(s3, s4);
    painter.drawLine(s4, s1);

    // Draw first corner marker
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(102, 170, 255));
    painter.drawEllipse(s1, CORNER_MARKER_SIZE/2.0, CORNER_MARKER_SIZE/2.0);

    painter.restore();
}
```

#### 3.3.3 Register RectangleTool
**File:** `src/ui/CADCanvas.cpp`

Add after LineTool and ArcTool registration:
```cpp
#include "ui/RectangleTool.h"
// ...
toolManager_->registerTool(std::make_unique<RectangleTool>());
```

#### 3.3.4 Update onRectangleTool in main.cpp
**File:** `src/main.cpp`

```cpp
void onRectangleTool() {
    ToolManager* toolMgr = canvas_->toolManager();
    toolMgr->activateTool("rectangle");
    canvas_->setCursor(Qt::CrossCursor);
}
```

#### 3.3.5 Update CMakeLists.txt
Add to UI_HEADERS and UI_SOURCES.

#### 3.3.6 Update File-Structure.md
Add RectangleTool documentation.

---

## 3.4 Tool Lifecycle - VERIFICATION

### Already Implemented ✅
- `Tool::activate()` / `deactivate()` - defined in Tool.h
- `Tool::render()` - preview rendering interface
- `Tool::handleMousePress/Move/Release/KeyPress()` - event handlers
- `ToolManager` routes events to active tool
- `ToolResult::Completed/Cancelled` signals geometry changes

### Status Bar Prompts ✅
- `Tool::statusPrompt()` returns guidance text
- `ToolManager::statusPromptChanged` signal connected in main.cpp
- `toolPromptLabel_` displays current prompt

### Verification Checklist
- [ ] Tool activate shows correct status prompt
- [ ] Tool preview renders during InProgress state
- [ ] Commit adds geometry to DocumentModel
- [ ] ESC cancels correctly (partial vs full cancel)
- [ ] Status bar updates on state changes

---

## Implementation Order

### Phase A: Fix Arc Tool (5 minutes)
1. Fix `distanceTo()` call in ArcTool.cpp line 74
2. Build and verify

### Phase B: Rectangle Tool (30 minutes)
1. Create RectangleTool.h
2. Create RectangleTool.cpp
3. Register in CADCanvas.cpp
4. Update main.cpp onRectangleTool()
5. Update CMakeLists.txt
6. Build and verify

### Phase C: Verification (10 minutes)
1. Test all tools: Line, Arc, Rectangle
2. Verify status bar prompts
3. Verify ESC cancellation
4. Verify continuous drawing mode

---

## Files Summary

| File | Action | Task |
|------|--------|------|
| `src/ui/ArcTool.cpp` | FIX | 3.2 |
| `include/ui/RectangleTool.h` | CREATE | 3.3 |
| `src/ui/RectangleTool.cpp` | CREATE | 3.3 |
| `src/ui/CADCanvas.cpp` | MODIFY | 3.3 (register) |
| `src/main.cpp` | MODIFY | 3.3 (menu handler) |
| `CMakeLists.txt` | MODIFY | 3.3 (add files) |
| `File-Structure.md` | MODIFY | 3.3 (docs) |

---

## Testing Checklist

### Arc Tool (after fix)
- [ ] Center point click shows blue marker
- [ ] Start point click shows green marker + radius line
- [ ] Preview arc follows mouse
- [ ] D key toggles direction indicator
- [ ] Third click creates arc
- [ ] ESC cancels current arc
- [ ] Double ESC exits tool

### Rectangle Tool
- [ ] First corner click shows marker
- [ ] Preview shows 4 dashed lines
- [ ] Second click creates 4 Line2D entities
- [ ] Tool stays active for next rectangle
- [ ] ESC cancels current rectangle
- [ ] Double ESC exits tool
- [ ] Zero-size rectangle rejected

### Tool Lifecycle
- [ ] Status bar shows correct prompts
- [ ] Geometry appears after commit
- [ ] Canvas repaints with new geometry
