#include "ui/RectangleTool.h"
#include "ui/CADCanvas.h"
#include "model/DocumentModel.h"
#include "model/CommandHistory.h"
#include "model/EntityCommands.h"
#include "geometry/Line2D.h"
#include "geometry/GeometryConstants.h"
#include "import/GeometryConverter.h"
#include <QPen>
#include <QBrush>
#include <QDebug>
#include <cmath>

namespace OwnCAD {
namespace UI {

RectangleTool::RectangleTool() = default;

void RectangleTool::activate() {
    resetToFirstCorner();
    qDebug() << "RectangleTool: Activated";
}

void RectangleTool::deactivate() {
    cancel();
    qDebug() << "RectangleTool: Deactivated";
}

QString RectangleTool::statusPrompt() const {
    switch (state_) {
        case ToolState::Inactive:
            return QString();
        case ToolState::WaitingForInput:
            return QStringLiteral("RECTANGLE: Click first corner");
        case ToolState::InProgress:
            return QStringLiteral("RECTANGLE: Click second corner (ESC to cancel)");
    }
    return QString();
}

ToolResult RectangleTool::handleMousePress(
    const Geometry::Point2D& worldPos,
    QMouseEvent* event
) {
    // Only handle left button
    if (event->button() != Qt::LeftButton) {
        return ToolResult::Ignored;
    }

    if (state_ == ToolState::WaitingForInput) {
        // First corner
        firstCorner_ = worldPos;
        currentCorner_ = worldPos;
        state_ = ToolState::InProgress;
        qDebug() << "RectangleTool: First corner set at" << worldPos.x() << "," << worldPos.y();
        return ToolResult::Continue;
    }
    else if (state_ == ToolState::InProgress) {
        // Second corner - commit rectangle
        currentCorner_ = worldPos;

        if (commitRectangle()) {
            // Stay in tool for continuous drawing
            resetToFirstCorner();
            qDebug() << "RectangleTool: Rectangle committed, ready for next";
            return ToolResult::Completed;
        } else {
            // Rectangle was invalid (zero-size)
            qDebug() << "RectangleTool: Invalid rectangle (too small), try again";
            return ToolResult::Continue;
        }
    }

    return ToolResult::Ignored;
}

ToolResult RectangleTool::handleMouseMove(
    const Geometry::Point2D& worldPos,
    QMouseEvent* /*event*/
) {
    // Always update current corner for preview
    currentCorner_ = worldPos;

    if (state_ == ToolState::InProgress) {
        // Trigger repaint for preview update
        return ToolResult::Continue;
    }

    return ToolResult::Ignored;
}

ToolResult RectangleTool::handleMouseRelease(
    const Geometry::Point2D& /*worldPos*/,
    QMouseEvent* /*event*/
) {
    // Rectangle tool doesn't use mouse release
    return ToolResult::Ignored;
}

ToolResult RectangleTool::handleKeyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (state_ == ToolState::InProgress) {
            // Cancel current rectangle, stay in tool
            resetToFirstCorner();
            qDebug() << "RectangleTool: Current rectangle cancelled";
            return ToolResult::Continue;
        }
        else if (state_ == ToolState::WaitingForInput) {
            // Exit tool completely
            cancel();
            qDebug() << "RectangleTool: Tool exited";
            return ToolResult::Cancelled;
        }
    }

    return ToolResult::Ignored;
}

void RectangleTool::render(QPainter& painter, const Viewport& viewport) {
    if (state_ != ToolState::InProgress || !firstCorner_ || !currentCorner_) {
        return;
    }

    // Save painter state
    painter.save();

    // Get corner coordinates
    double x1 = firstCorner_->x();
    double y1 = firstCorner_->y();
    double x2 = currentCorner_->x();
    double y2 = currentCorner_->y();

    // Convert all 4 corners to screen coordinates
    QPointF s1 = viewport.worldToScreen(Geometry::Point2D(x1, y1));
    QPointF s2 = viewport.worldToScreen(Geometry::Point2D(x2, y1));
    QPointF s3 = viewport.worldToScreen(Geometry::Point2D(x2, y2));
    QPointF s4 = viewport.worldToScreen(Geometry::Point2D(x1, y2));

    // Draw preview rectangle (dashed, light blue)
    QPen previewPen(QColor(102, 170, 255));  // Light blue #66AAFF
    previewPen.setWidth(PREVIEW_LINE_WIDTH);
    previewPen.setStyle(Qt::DashLine);
    previewPen.setDashPattern({6, 4});  // 6px dash, 4px gap
    painter.setPen(previewPen);

    // Draw 4 lines
    painter.drawLine(s1, s2);  // Bottom (or top)
    painter.drawLine(s2, s3);  // Right
    painter.drawLine(s3, s4);  // Top (or bottom)
    painter.drawLine(s4, s1);  // Left

    // Draw first corner marker (small filled circle)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(102, 170, 255));
    painter.drawEllipse(
        s1,
        CORNER_MARKER_SIZE / 2.0,
        CORNER_MARKER_SIZE / 2.0
    );

    // Restore painter state
    painter.restore();
}

bool RectangleTool::commitRectangle() {
    if (!firstCorner_ || !currentCorner_ || !documentModel_) {
        qWarning() << "RectangleTool: Cannot commit - missing corner or document model";
        return false;
    }

    // Get corner coordinates
    double x1 = firstCorner_->x();
    double y1 = firstCorner_->y();
    double x2 = currentCorner_->x();
    double y2 = currentCorner_->y();

    // Validate: rectangle must have meaningful size
    double width = std::abs(x2 - x1);
    double height = std::abs(y2 - y1);

    if (width < Geometry::MIN_LINE_LENGTH || height < Geometry::MIN_LINE_LENGTH) {
        qWarning() << "RectangleTool: Rectangle too small (width=" << width << ", height=" << height << ")";
        return false;
    }

    // Create 4 corner points
    Geometry::Point2D p1(x1, y1);  // First corner
    Geometry::Point2D p2(x2, y1);  // Adjacent corner (same Y)
    Geometry::Point2D p3(x2, y2);  // Opposite corner (second click)
    Geometry::Point2D p4(x1, y2);  // Adjacent corner (same X as first)

    // Create 4 lines (counterclockwise from first corner)
    auto line1 = Geometry::Line2D::create(p1, p2);
    auto line2 = Geometry::Line2D::create(p2, p3);
    auto line3 = Geometry::Line2D::create(p3, p4);
    auto line4 = Geometry::Line2D::create(p4, p1);

    // Validate all lines created successfully
    if (!line1 || !line2 || !line3 || !line4) {
        qWarning() << "RectangleTool: Failed to create one or more lines";
        return false;
    }

    // Use command system if available (enables undo/redo)
    if (commandHistory_) {
        // Create batch of 4 lines as single undoable operation
        std::vector<Import::GeometryEntity> entities;
        entities.push_back(Import::GeometryEntity{*line1});
        entities.push_back(Import::GeometryEntity{*line2});
        entities.push_back(Import::GeometryEntity{*line3});
        entities.push_back(Import::GeometryEntity{*line4});

        auto command = std::make_unique<Model::CreateEntitiesCommand>(
            documentModel_,
            entities,
            "0"  // default layer
        );

        if (!commandHistory_->executeCommand(std::move(command))) {
            qWarning() << "RectangleTool: Failed to execute create command";
            return false;
        }
        qDebug() << "RectangleTool: Created rectangle (4 lines) via command system"
                 << "width=" << width << "height=" << height;
    } else {
        // Fallback: direct add (no undo support)
        std::string h1 = documentModel_->addLine(*line1);
        std::string h2 = documentModel_->addLine(*line2);
        std::string h3 = documentModel_->addLine(*line3);
        std::string h4 = documentModel_->addLine(*line4);

        if (h1.empty() || h2.empty() || h3.empty() || h4.empty()) {
            qWarning() << "RectangleTool: Failed to add one or more lines to document";
            return false;
        }
        qDebug() << "RectangleTool: Created rectangle (4 lines)"
                 << "width=" << width << "height=" << height;
    }

    return true;
}

void RectangleTool::resetToFirstCorner() {
    firstCorner_.reset();
    currentCorner_.reset();
    state_ = ToolState::WaitingForInput;
}

void RectangleTool::cancel() {
    resetToFirstCorner();
    state_ = ToolState::Inactive;
}

} // namespace UI
} // namespace OwnCAD
