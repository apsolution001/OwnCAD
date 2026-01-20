# Task 3.2: Arc Tool Implementation Guide

## Overview

Implement `ArcTool` class for creating arcs using the **Center–Start–End** workflow.

**User Workflow:**
1. Click 1: Set center point
2. Click 2: Set start point (defines radius and start angle)
3. Mouse move: Preview arc from start toward cursor
4. Click 3: Set end point (defines end angle, commits arc)
5. ESC at any point cancels current operation

---

## Subtasks

### 3.2.1 Create ArcTool Header (`include/ui/ArcTool.h`)

**Files to create:** `include/ui/ArcTool.h`

```cpp
#pragma once

#include "ui/Tool.h"
#include "geometry/Point2D.h"
#include "geometry/Arc2D.h"
#include <optional>

namespace OwnCAD {
namespace UI {

/**
 * Arc drawing tool using Center-Start-End workflow.
 *
 * State machine:
 * [Inactive] --activate()--> [WaitingCenterPoint]
 * [WaitingCenterPoint] --click--> [WaitingStartPoint]
 * [WaitingStartPoint] --click--> [WaitingEndPoint]
 * [WaitingEndPoint] --click--> [Commit] --> [WaitingCenterPoint]
 * [WaitingEndPoint] --ESC--> [WaitingCenterPoint]
 * [WaitingStartPoint] --ESC--> [WaitingCenterPoint]
 * [WaitingCenterPoint] --ESC--> [Inactive]
 */
class ArcTool : public Tool {
public:
    ArcTool();
    ~ArcTool() override = default;

    // Tool interface
    QString name() const override { return QStringLiteral("Arc"); }
    QString id() const override { return QStringLiteral("arc"); }

    void activate() override;
    void deactivate() override;

    ToolState state() const override;
    QString statusPrompt() const override;

    // Event handlers
    ToolResult handleMousePress(const Geometry::Point2D& worldPos, QMouseEvent* event) override;
    ToolResult handleMouseMove(const Geometry::Point2D& worldPos, QMouseEvent* event) override;
    ToolResult handleMouseRelease(const Geometry::Point2D& worldPos, QMouseEvent* event) override;
    ToolResult handleKeyPress(QKeyEvent* event) override;

    // Rendering
    void render(QPainter& painter, const Viewport& viewport) override;

private:
    // Internal state (more granular than base ToolState)
    enum class ArcState {
        Inactive,
        WaitingCenterPoint,
        WaitingStartPoint,
        WaitingEndPoint
    };

    bool commitArc();
    void resetToCenterPoint();
    void cancel();

    // Calculate angle from center to point
    double angleToPoint(const Geometry::Point2D& from, const Geometry::Point2D& to) const;

    // State
    ArcState arcState_ = ArcState::Inactive;

    // Points
    std::optional<Geometry::Point2D> centerPoint_;
    std::optional<Geometry::Point2D> startPoint_;
    std::optional<Geometry::Point2D> currentPoint_;  // For preview

    // Derived values (calculated from points)
    double radius_ = 0.0;
    double startAngle_ = 0.0;
    double currentAngle_ = 0.0;

    // Arc direction (CCW by default, toggled with D key)
    bool counterClockwise_ = true;

    // Visual settings
    static constexpr int PREVIEW_LINE_WIDTH = 1;
    static constexpr int POINT_MARKER_SIZE = 6;
    static constexpr int DIRECTION_ARROW_SIZE = 10;
};

} // namespace UI
} // namespace OwnCAD
```

---

### 3.2.2 Create ArcTool Implementation (`src/ui/ArcTool.cpp`)

**Files to create:** `src/ui/ArcTool.cpp`

#### Core Implementation Structure:

```cpp
#include "ui/ArcTool.h"
#include "ui/CADCanvas.h"
#include "model/DocumentModel.h"
#include "geometry/GeometryConstants.h"
#include <QPen>
#include <QBrush>
#include <QDebug>
#include <cmath>

namespace OwnCAD {
namespace UI {

ArcTool::ArcTool() = default;

void ArcTool::activate() {
    resetToCenterPoint();
    qDebug() << "ArcTool: Activated";
}

void ArcTool::deactivate() {
    cancel();
    qDebug() << "ArcTool: Deactivated";
}

ToolState ArcTool::state() const {
    switch (arcState_) {
        case ArcState::Inactive:
            return ToolState::Inactive;
        case ArcState::WaitingCenterPoint:
            return ToolState::WaitingForInput;
        case ArcState::WaitingStartPoint:
        case ArcState::WaitingEndPoint:
            return ToolState::InProgress;
    }
    return ToolState::Inactive;
}

QString ArcTool::statusPrompt() const {
    switch (arcState_) {
        case ArcState::Inactive:
            return QString();
        case ArcState::WaitingCenterPoint:
            return QStringLiteral("ARC: Click center point");
        case ArcState::WaitingStartPoint:
            return QStringLiteral("ARC: Click start point (defines radius)");
        case ArcState::WaitingEndPoint:
            return counterClockwise_
                ? QStringLiteral("ARC: Click end point (CCW) [D=toggle direction, ESC=cancel]")
                : QStringLiteral("ARC: Click end point (CW) [D=toggle direction, ESC=cancel]");
    }
    return QString();
}

// ... handleMousePress, handleMouseMove, handleKeyPress, render, commitArc implementations ...

} // namespace UI
} // namespace OwnCAD
```

---

### 3.2.3 Implement Event Handlers

#### handleMousePress Logic:

```cpp
ToolResult ArcTool::handleMousePress(const Geometry::Point2D& worldPos, QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        return ToolResult::Ignored;
    }

    switch (arcState_) {
        case ArcState::WaitingCenterPoint:
            centerPoint_ = worldPos;
            currentPoint_ = worldPos;
            arcState_ = ArcState::WaitingStartPoint;
            qDebug() << "ArcTool: Center set at" << worldPos.x() << "," << worldPos.y();
            return ToolResult::Continue;

        case ArcState::WaitingStartPoint:
            startPoint_ = worldPos;
            radius_ = Geometry::Point2D::distance(*centerPoint_, *startPoint_);

            // Validate radius
            if (radius_ < Geometry::MIN_ARC_RADIUS) {
                qWarning() << "ArcTool: Radius too small, pick different start point";
                return ToolResult::Continue;
            }

            startAngle_ = angleToPoint(*centerPoint_, *startPoint_);
            currentAngle_ = startAngle_;
            arcState_ = ArcState::WaitingEndPoint;
            qDebug() << "ArcTool: Start point set, radius=" << radius_;
            return ToolResult::Continue;

        case ArcState::WaitingEndPoint:
            currentPoint_ = worldPos;
            currentAngle_ = angleToPoint(*centerPoint_, *currentPoint_);

            if (commitArc()) {
                // Stay active for next arc
                resetToCenterPoint();
                return ToolResult::Completed;
            }
            return ToolResult::Continue;

        default:
            return ToolResult::Ignored;
    }
}
```

#### handleMouseMove Logic:

```cpp
ToolResult ArcTool::handleMouseMove(const Geometry::Point2D& worldPos, QMouseEvent*) {
    currentPoint_ = worldPos;

    if (arcState_ == ArcState::WaitingStartPoint) {
        // Show radius preview line
        return ToolResult::Continue;
    }

    if (arcState_ == ArcState::WaitingEndPoint) {
        // Update preview arc angle
        currentAngle_ = angleToPoint(*centerPoint_, *currentPoint_);
        return ToolResult::Continue;
    }

    return ToolResult::Ignored;
}
```

#### handleKeyPress Logic:

```cpp
ToolResult ArcTool::handleKeyPress(QKeyEvent* event) {
    // Toggle direction with D key
    if (event->key() == Qt::Key_D && arcState_ == ArcState::WaitingEndPoint) {
        counterClockwise_ = !counterClockwise_;
        qDebug() << "ArcTool: Direction toggled to" << (counterClockwise_ ? "CCW" : "CW");
        return ToolResult::Continue;
    }

    // ESC to cancel
    if (event->key() == Qt::Key_Escape) {
        if (arcState_ == ArcState::WaitingEndPoint || arcState_ == ArcState::WaitingStartPoint) {
            resetToCenterPoint();
            qDebug() << "ArcTool: Operation cancelled";
            return ToolResult::Continue;
        }
        else if (arcState_ == ArcState::WaitingCenterPoint) {
            cancel();
            qDebug() << "ArcTool: Tool exited";
            return ToolResult::Cancelled;
        }
    }

    return ToolResult::Ignored;
}
```

---

### 3.2.4 Implement Arc Preview Rendering

#### render() Method:

```cpp
void ArcTool::render(QPainter& painter, const Viewport& viewport) {
    painter.save();

    // Preview style
    QPen previewPen(QColor(102, 170, 255));  // Light blue
    previewPen.setWidth(PREVIEW_LINE_WIDTH);
    previewPen.setStyle(Qt::DashLine);

    // State: WaitingStartPoint - draw radius line from center to cursor
    if (arcState_ == ArcState::WaitingStartPoint && centerPoint_ && currentPoint_) {
        QPointF screenCenter = viewport.worldToScreen(*centerPoint_);
        QPointF screenCursor = viewport.worldToScreen(*currentPoint_);

        painter.setPen(previewPen);
        painter.drawLine(screenCenter, screenCursor);

        // Draw center marker
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(102, 170, 255));
        painter.drawEllipse(screenCenter, POINT_MARKER_SIZE/2.0, POINT_MARKER_SIZE/2.0);
    }

    // State: WaitingEndPoint - draw preview arc
    if (arcState_ == ArcState::WaitingEndPoint && centerPoint_ && startPoint_ && currentPoint_) {
        renderArcPreview(painter, viewport);
    }

    painter.restore();
}

void ArcTool::renderArcPreview(QPainter& painter, const Viewport& viewport) {
    QPointF screenCenter = viewport.worldToScreen(*centerPoint_);
    double screenRadius = radius_ * viewport.zoomLevel();

    // Calculate sweep for Qt (in 16ths of degrees)
    double startDeg = startAngle_ * 180.0 / M_PI;
    double endDeg = currentAngle_ * 180.0 / M_PI;

    double sweepDeg;
    if (counterClockwise_) {
        sweepDeg = endDeg - startDeg;
        if (sweepDeg <= 0) sweepDeg += 360.0;
    } else {
        sweepDeg = startDeg - endDeg;
        if (sweepDeg <= 0) sweepDeg += 360.0;
        sweepDeg = -sweepDeg;  // Negative for CW
    }

    // Qt draws arcs counter-clockwise from start angle
    QRectF arcRect(
        screenCenter.x() - screenRadius,
        screenCenter.y() - screenRadius,
        screenRadius * 2,
        screenRadius * 2
    );

    // NOTE: Qt y-axis is inverted, so angles need adjustment
    // Qt measures from 3 o'clock position, counter-clockwise
    // Our angles are standard math convention (from positive X, CCW)
    double qtStartAngle = -startDeg;  // Negate for Qt's inverted Y

    QPen previewPen(QColor(102, 170, 255));
    previewPen.setWidth(PREVIEW_LINE_WIDTH);
    previewPen.setStyle(Qt::DashLine);
    painter.setPen(previewPen);
    painter.drawArc(arcRect, qtStartAngle * 16, sweepDeg * 16);

    // Draw start point marker
    QPointF screenStart = viewport.worldToScreen(*startPoint_);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 200, 0));  // Green for start
    painter.drawEllipse(screenStart, POINT_MARKER_SIZE/2.0, POINT_MARKER_SIZE/2.0);

    // Draw center marker
    painter.setBrush(QColor(102, 170, 255));
    painter.drawEllipse(screenCenter, POINT_MARKER_SIZE/2.0, POINT_MARKER_SIZE/2.0);

    // Draw direction indicator arrow
    renderDirectionArrow(painter, viewport);
}
```

---

### 3.2.5 Implement Arc Commit Logic

```cpp
bool ArcTool::commitArc() {
    if (!centerPoint_ || !startPoint_ || !documentModel_) {
        qWarning() << "ArcTool: Cannot commit - missing data";
        return false;
    }

    // Validate sweep angle
    double sweepAngle;
    if (counterClockwise_) {
        sweepAngle = currentAngle_ - startAngle_;
        if (sweepAngle <= 0) sweepAngle += 2.0 * M_PI;
    } else {
        sweepAngle = startAngle_ - currentAngle_;
        if (sweepAngle <= 0) sweepAngle += 2.0 * M_PI;
    }

    if (sweepAngle < Geometry::MIN_ARC_SWEEP) {
        qWarning() << "ArcTool: Sweep angle too small";
        return false;
    }

    // Create arc using factory method
    auto arc = Geometry::Arc2D::create(
        *centerPoint_,
        radius_,
        startAngle_,
        currentAngle_,
        counterClockwise_
    );

    if (!arc) {
        qWarning() << "ArcTool: Failed to create arc - invalid geometry";
        return false;
    }

    // Add to document
    std::string handle = documentModel_->addArc(*arc);
    if (handle.empty()) {
        qWarning() << "ArcTool: Failed to add arc to document";
        return false;
    }

    qDebug() << "ArcTool: Created arc with handle" << QString::fromStdString(handle)
             << "radius=" << radius_
             << "sweep=" << (sweepAngle * 180.0 / M_PI) << "deg"
             << (counterClockwise_ ? "CCW" : "CW");
    return true;
}

double ArcTool::angleToPoint(const Geometry::Point2D& from, const Geometry::Point2D& to) const {
    double dx = to.x() - from.x();
    double dy = to.y() - from.y();
    double angle = std::atan2(dy, dx);
    if (angle < 0) angle += 2.0 * M_PI;  // Normalize to [0, 2π)
    return angle;
}

void ArcTool::resetToCenterPoint() {
    centerPoint_.reset();
    startPoint_.reset();
    currentPoint_.reset();
    radius_ = 0.0;
    startAngle_ = 0.0;
    currentAngle_ = 0.0;
    counterClockwise_ = true;  // Reset to default direction
    arcState_ = ArcState::WaitingCenterPoint;
}

void ArcTool::cancel() {
    resetToCenterPoint();
    arcState_ = ArcState::Inactive;
}
```

---

### 3.2.6 Register ArcTool in ToolManager

**File to modify:** `src/ui/ToolManager.cpp`

Add registration:
```cpp
#include "ui/ArcTool.h"

void ToolManager::registerDefaultTools() {
    registerTool(std::make_unique<LineTool>());
    registerTool(std::make_unique<ArcTool>());  // ADD THIS LINE
}
```

---

### 3.2.7 Update CMakeLists.txt

**File to modify:** `CMakeLists.txt`

Add to source files:
```cmake
set(UI_SOURCES
    src/ui/CADCanvas.cpp
    src/ui/SelectionManager.cpp
    src/ui/GridSettingsDialog.cpp
    src/ui/ToolManager.cpp
    src/ui/LineTool.cpp
    src/ui/ArcTool.cpp       # ADD THIS LINE
)

set(UI_HEADERS
    include/ui/CADCanvas.h
    include/ui/SelectionManager.h
    include/ui/GridSettingsDialog.h
    include/ui/Tool.h
    include/ui/ToolManager.h
    include/ui/LineTool.h
    include/ui/ArcTool.h     # ADD THIS LINE
)
```

---

### 3.2.8 Update File-Structure.md

**File to modify:** `File-Structure.md`

Add to UI section:
```markdown
- `ArcTool.h/cpp`: Arc drawing tool implementation.
  - State machine: WaitingCenterPoint → WaitingStartPoint → WaitingEndPoint → Commit.
  - Center-Start-End workflow (3 clicks).
  - Direction toggle (D key), CCW default.
  - Preview rendering with direction indicator.
```

---

## Implementation Checklist

| Subtask | Description | Files |
|---------|-------------|-------|
| 3.2.1 | Create ArcTool header | `include/ui/ArcTool.h` |
| 3.2.2 | Create ArcTool implementation skeleton | `src/ui/ArcTool.cpp` |
| 3.2.3 | Implement event handlers | `src/ui/ArcTool.cpp` |
| 3.2.4 | Implement arc preview rendering | `src/ui/ArcTool.cpp` |
| 3.2.5 | Implement arc commit logic | `src/ui/ArcTool.cpp` |
| 3.2.6 | Register in ToolManager | `src/ui/ToolManager.cpp` |
| 3.2.7 | Update CMakeLists.txt | `CMakeLists.txt` |
| 3.2.8 | Update File-Structure.md | `File-Structure.md` |

---

## Key Design Decisions

### 1. Internal State vs Base ToolState
The base `Tool.h` defines only 3 states: `Inactive`, `WaitingForInput`, `InProgress`. Since ArcTool needs 4 states (Inactive + 3 click stages), we use an internal `ArcState` enum and map it to the base `ToolState` in `state()`.

### 2. Direction Toggle
- Default: Counter-clockwise (CCW) - standard CAD convention
- Toggle with `D` key during `WaitingEndPoint` state
- Critical for CNC toolpath correctness

### 3. Qt Coordinate System Handling
Qt's Y-axis is inverted (positive Y goes down). The arc rendering code must negate angles when drawing with `QPainter::drawArc()`.

### 4. Validation Points
- **Radius check**: At start point click (must be >= MIN_ARC_RADIUS)
- **Sweep check**: At end point click (must be >= MIN_ARC_SWEEP)
- Uses `Arc2D::create()` factory which enforces all validation

### 5. Visual Feedback
- Center point: Blue circle marker
- Start point: Green circle marker
- Preview arc: Dashed blue line
- Direction indicator: Arrow showing CCW/CW

---

## Testing Strategy

1. **Basic arc creation**: Click center, start, end → arc appears
2. **Direction toggle**: Press D during step 3 → arc direction changes
3. **ESC cancellation**: ESC at each stage → appropriate reset
4. **Zero-radius rejection**: Click same point for center and start → error/reject
5. **Zero-sweep rejection**: Click same angle for start and end → error/reject
6. **Snap integration**: Verify snapping works at all 3 click points
7. **Continuous mode**: After commit, tool ready for next arc

---

## Dependencies

- `Tool.h` (base class) ✅
- `Arc2D.h/cpp` (geometry) ✅
- `DocumentModel::addArc()` ✅
- `Viewport` (coordinate transforms) ✅
- `ToolManager` (tool registration) ✅
