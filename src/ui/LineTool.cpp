#include "ui/LineTool.h"
#include "ui/CADCanvas.h"
#include "model/DocumentModel.h"
#include <QPen>
#include <QBrush>
#include <QDebug>

namespace OwnCAD {
namespace UI {

LineTool::LineTool() = default;

void LineTool::activate() {
    resetToFirstPoint();
    state_ = ToolState::WaitingForInput;
    qDebug() << "LineTool: Activated";
}

void LineTool::deactivate() {
    cancel();
    qDebug() << "LineTool: Deactivated";
}

QString LineTool::statusPrompt() const {
    switch (state_) {
        case ToolState::Inactive:
            return QString();
        case ToolState::WaitingForInput:
            return QStringLiteral("LINE: Click first point");
        case ToolState::InProgress:
            return QStringLiteral("LINE: Click second point (ESC to cancel)");
    }
    return QString();
}

ToolResult LineTool::handleMousePress(
    const Geometry::Point2D& worldPos,
    QMouseEvent* event
) {
    // Only handle left button
    if (event->button() != Qt::LeftButton) {
        return ToolResult::Ignored;
    }

    if (state_ == ToolState::WaitingForInput) {
        // First point
        firstPoint_ = worldPos;
        currentPoint_ = worldPos;
        state_ = ToolState::InProgress;
        qDebug() << "LineTool: First point set at" << worldPos.x() << "," << worldPos.y();
        return ToolResult::Continue;
    }
    else if (state_ == ToolState::InProgress) {
        // Second point - commit line
        currentPoint_ = worldPos;

        if (commitLine()) {
            // Stay in tool for continuous drawing
            // First point of next line is end point of previous
            firstPoint_ = worldPos;
            qDebug() << "LineTool: Line committed, ready for next point";
            return ToolResult::Completed;
        } else {
            // Line was invalid (zero-length)
            qDebug() << "LineTool: Invalid line (zero-length), staying in progress";
            return ToolResult::Continue;
        }
    }

    return ToolResult::Ignored;
}

ToolResult LineTool::handleMouseMove(
    const Geometry::Point2D& worldPos,
    QMouseEvent* /*event*/
) {
    // Always update current point for preview
    currentPoint_ = worldPos;

    if (state_ == ToolState::InProgress) {
        // Trigger repaint for preview update
        return ToolResult::Continue;
    }

    return ToolResult::Ignored;
}

ToolResult LineTool::handleMouseRelease(
    const Geometry::Point2D& /*worldPos*/,
    QMouseEvent* /*event*/
) {
    // Line tool doesn't use mouse release
    return ToolResult::Ignored;
}

ToolResult LineTool::handleKeyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (state_ == ToolState::InProgress) {
            // Cancel current line, stay in tool
            resetToFirstPoint();
            qDebug() << "LineTool: Current line cancelled";
            return ToolResult::Continue;
        }
        else if (state_ == ToolState::WaitingForInput) {
            // Exit tool completely
            cancel();
            qDebug() << "LineTool: Tool exited";
            return ToolResult::Cancelled;
        }
    }

    return ToolResult::Ignored;
}

void LineTool::render(QPainter& painter, const Viewport& viewport) {
    if (state_ != ToolState::InProgress || !firstPoint_ || !currentPoint_) {
        return;
    }

    // Save painter state
    painter.save();

    // Convert points to screen coordinates
    QPointF screenStart = viewport.worldToScreen(*firstPoint_);
    QPointF screenEnd = viewport.worldToScreen(*currentPoint_);

    // Draw preview line (dashed, light blue)
    QPen previewPen(QColor(102, 170, 255));  // Light blue #66AAFF
    previewPen.setWidth(PREVIEW_LINE_WIDTH);
    previewPen.setStyle(Qt::DashLine);
    previewPen.setDashPattern({6, 4});  // 6px dash, 4px gap
    painter.setPen(previewPen);
    painter.drawLine(screenStart, screenEnd);

    // Draw first point marker (small filled circle)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(102, 170, 255));
    painter.drawEllipse(
        screenStart,
        POINT_MARKER_SIZE / 2.0,
        POINT_MARKER_SIZE / 2.0
    );

    // Restore painter state
    painter.restore();
}

bool LineTool::commitLine() {
    if (!firstPoint_ || !currentPoint_ || !documentModel_) {
        qWarning() << "LineTool: Cannot commit - missing point or document model";
        return false;
    }

    // Try to create line (validates length)
    auto line = Geometry::Line2D::create(*firstPoint_, *currentPoint_);
    if (!line) {
        qWarning() << "LineTool: Cannot create line - invalid geometry (zero-length?)";
        return false;
    }

    // Add to document model
    std::string handle = documentModel_->addLine(*line);
    if (handle.empty()) {
        qWarning() << "LineTool: Failed to add line to document";
        return false;
    }

    qDebug() << "LineTool: Created line with handle" << QString::fromStdString(handle);
    return true;
}

void LineTool::resetToFirstPoint() {
    firstPoint_.reset();
    currentPoint_.reset();
    state_ = ToolState::WaitingForInput;
}

void LineTool::cancel() {
    resetToFirstPoint();
    state_ = ToolState::Inactive;
}

} // namespace UI
} // namespace OwnCAD
