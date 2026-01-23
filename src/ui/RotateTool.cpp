#include "ui/RotateTool.h"
#include "ui/RotateInputDialog.h"
#include "ui/CADCanvas.h"
#include "model/DocumentModel.h"
#include "model/CommandHistory.h"
#include "model/EntityCommands.h"
#include "geometry/GeometryMath.h"
#include "geometry/GeometryConstants.h"
#include "import/GeometryConverter.h"
#include <QPen>
#include <QBrush>
#include <QDebug>
#include <cmath>

namespace OwnCAD {
namespace UI {

using namespace OwnCAD::Geometry;
using namespace OwnCAD::Import;

RotateTool::RotateTool() = default;

void RotateTool::setSelectedHandles(const std::vector<std::string>& handles) {
    selectedHandles_ = handles;
}

void RotateTool::activate() {
    // Check if we have selection
    if (selectedHandles_.empty()) {
        qWarning() << "RotateTool: No entities selected - cannot activate";
        state_ = ToolState::Inactive;
        return;
    }

    resetToCenter();
    state_ = ToolState::WaitingForInput;
    qDebug() << "RotateTool: Activated with" << selectedHandles_.size() << "entities";
}

void RotateTool::deactivate() {
    cancel();
    qDebug() << "RotateTool: Deactivated";
}

QString RotateTool::statusPrompt() const {
    switch (state_) {
        case ToolState::Inactive:
            return QString();
        case ToolState::WaitingForInput:
            return QStringLiteral("ROTATE: Click rotation center (or ESC to cancel)");
        case ToolState::InProgress:
            if (angleSnapEnabled_) {
                return QStringLiteral("ROTATE: Click to set angle (Snap: 15\u00B0) - Tab for exact angle");
            }
            return QStringLiteral("ROTATE: Click to set angle (Shift=snap, Tab=exact angle)");
    }
    return QString();
}

ToolResult RotateTool::handleMousePress(
    const Geometry::Point2D& worldPos,
    QMouseEvent* event
) {
    // Only handle left button
    if (event->button() != Qt::LeftButton) {
        return ToolResult::Ignored;
    }

    if (state_ == ToolState::WaitingForInput) {
        // Set rotation center
        centerPoint_ = worldPos;
        startPoint_ = worldPos;
        currentPoint_ = worldPos;
        baseAngle_ = 0.0;
        state_ = ToolState::InProgress;
        qDebug() << "RotateTool: Center set at" << worldPos.x() << "," << worldPos.y();
        return ToolResult::Continue;
    }
    else if (state_ == ToolState::InProgress) {
        // Set final angle and commit
        currentPoint_ = worldPos;

        if (commitRotation()) {
            qDebug() << "RotateTool: Rotation committed";
            cancel();  // Return to inactive after rotation
            return ToolResult::Completed;
        } else {
            qDebug() << "RotateTool: Rotation failed or zero angle";
            return ToolResult::Continue;
        }
    }

    return ToolResult::Ignored;
}

ToolResult RotateTool::handleMouseMove(
    const Geometry::Point2D& worldPos,
    QMouseEvent* event
) {
    // Track angle snap state from Shift key
    angleSnapEnabled_ = (event->modifiers() & Qt::ShiftModifier);

    // Always update current point for preview
    currentPoint_ = worldPos;

    if (state_ == ToolState::InProgress) {
        // Trigger repaint for preview update
        return ToolResult::Continue;
    }

    return ToolResult::Ignored;
}

ToolResult RotateTool::handleMouseRelease(
    const Geometry::Point2D& /*worldPos*/,
    QMouseEvent* /*event*/
) {
    // Rotate tool doesn't use mouse release
    return ToolResult::Ignored;
}

ToolResult RotateTool::handleKeyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (state_ == ToolState::InProgress) {
            // Cancel current rotation, go back to center selection
            resetToCenter();
            qDebug() << "RotateTool: Rotation cancelled, waiting for center";
            return ToolResult::Continue;
        }
        else if (state_ == ToolState::WaitingForInput) {
            // Exit tool completely
            cancel();
            qDebug() << "RotateTool: Tool exited";
            return ToolResult::Cancelled;
        }
    }

    // Tab or Enter triggers numeric angle input dialog
    if (state_ == ToolState::InProgress &&
        (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Return)) {

        // Show numeric input dialog
        RotateInputDialog dialog;

        // Pre-fill with current preview angle (converted to degrees)
        double currentAngle = calculateAngle(angleSnapEnabled_);
        double currentDegrees = currentAngle * 180.0 / PI;
        dialog.setInitialAngle(currentDegrees);

        if (dialog.exec() == QDialog::Accepted) {
            auto angleDegrees = dialog.getAngleDegrees();
            if (angleDegrees) {
                // Convert degrees to radians and commit
                double angleRadians = *angleDegrees * PI / 180.0;
                if (commitRotationWithAngle(angleRadians)) {
                    qDebug() << "RotateTool: Rotation committed via numeric input:" << *angleDegrees << "degrees";
                    cancel();
                    return ToolResult::Completed;
                }
            }
        }
        // Dialog cancelled or rotation failed - stay in current state
        return ToolResult::Continue;
    }

    return ToolResult::Ignored;
}

double RotateTool::calculateAngle(bool applySnap) const {
    if (!centerPoint_ || !currentPoint_) {
        return 0.0;
    }

    // Calculate angle from center to current point
    double dx = currentPoint_->x() - centerPoint_->x();
    double dy = currentPoint_->y() - centerPoint_->y();

    // Avoid division by zero for very small movements
    if (std::abs(dx) < GEOMETRY_EPSILON && std::abs(dy) < GEOMETRY_EPSILON) {
        return 0.0;
    }

    double angle = std::atan2(dy, dx) - baseAngle_;

    if (applySnap) {
        angle = GeometryMath::snapAngle(angle, ANGLE_SNAP_INCREMENT);
    }

    return angle;
}

void RotateTool::render(QPainter& painter, const Viewport& viewport) {
    if (state_ != ToolState::InProgress || !centerPoint_ || !currentPoint_) {
        return;
    }

    double angle = calculateAngle(angleSnapEnabled_);

    painter.save();

    // Draw rotation center marker (cross with circle)
    QPointF screenCenter = viewport.worldToScreen(*centerPoint_);

    // Circle
    painter.setPen(QPen(QColor(255, 100, 100), 2));  // Red
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(screenCenter, CENTER_MARKER_SIZE, CENTER_MARKER_SIZE);

    // Cross
    painter.drawLine(
        QPointF(screenCenter.x() - CENTER_MARKER_SIZE, screenCenter.y()),
        QPointF(screenCenter.x() + CENTER_MARKER_SIZE, screenCenter.y())
    );
    painter.drawLine(
        QPointF(screenCenter.x(), screenCenter.y() - CENTER_MARKER_SIZE),
        QPointF(screenCenter.x(), screenCenter.y() + CENTER_MARKER_SIZE)
    );

    // Draw angle indicator arc
    QPointF screenCurrent = viewport.worldToScreen(*currentPoint_);
    double radius = std::sqrt(
        std::pow(screenCurrent.x() - screenCenter.x(), 2) +
        std::pow(screenCurrent.y() - screenCenter.y(), 2)
    );

    if (radius > 10.0) {  // Only draw arc if there's meaningful distance
        // Draw reference line (from center to current position)
        painter.setPen(QPen(QColor(100, 200, 100), 1, Qt::DashLine));
        painter.drawLine(screenCenter, screenCurrent);

        // Draw angle arc
        QRectF arcRect(
            screenCenter.x() - radius / 3,
            screenCenter.y() - radius / 3,
            radius * 2 / 3,
            radius * 2 / 3
        );

        // Qt uses 1/16th of a degree for arc angles, and angles start from 3 o'clock
        int startAngle16 = static_cast<int>(baseAngle_ * 180.0 / PI * 16);
        int spanAngle16 = static_cast<int>(angle * 180.0 / PI * 16);

        painter.setPen(QPen(QColor(100, 200, 255), 2));
        painter.drawArc(arcRect, -startAngle16, -spanAngle16);

        // Draw angle text
        double angleDegrees = angle * 180.0 / PI;
        QString angleText = QString("%1\u00B0").arg(angleDegrees, 0, 'f', 1);
        painter.setPen(QColor(100, 200, 255));
        painter.drawText(
            screenCenter.x() + 15,
            screenCenter.y() - 15,
            angleText
        );
    }

    // Draw preview of each selected entity at rotated position
    if (documentModel_) {
        for (const auto& handle : selectedHandles_) {
            renderEntityPreview(painter, viewport, handle, angle);
        }
    }

    painter.restore();
}

void RotateTool::renderEntityPreview(
    QPainter& painter,
    const Viewport& viewport,
    const std::string& handle,
    double angleRadians
) {
    const auto* entityMeta = documentModel_->findEntityByHandle(handle);
    if (!entityMeta || !centerPoint_) {
        return;
    }

    // Preview pen: dashed, semi-transparent
    QPen previewPen(QColor(102, 170, 255, 180));  // Light blue, semi-transparent
    previewPen.setWidth(PREVIEW_LINE_WIDTH);
    previewPen.setStyle(Qt::DashLine);
    previewPen.setDashPattern({6, 4});
    painter.setPen(previewPen);
    painter.setBrush(Qt::NoBrush);

    // Render based on entity type
    std::visit([&](auto&& entity) {
        using T = std::decay_t<decltype(entity)>;

        if constexpr (std::is_same_v<T, Line2D>) {
            auto rotated = GeometryMath::rotate(entity, *centerPoint_, angleRadians);
            if (rotated) {
                QPointF screenStart = viewport.worldToScreen(rotated->start());
                QPointF screenEnd = viewport.worldToScreen(rotated->end());
                painter.drawLine(screenStart, screenEnd);
            }
        }
        else if constexpr (std::is_same_v<T, Arc2D>) {
            auto rotated = GeometryMath::rotate(entity, *centerPoint_, angleRadians);
            if (rotated) {
                // Draw arc as polyline approximation for preview
                const int segments = 32;
                QPointF prevPoint;
                for (int i = 0; i <= segments; ++i) {
                    double t = static_cast<double>(i) / segments;
                    Point2D worldPt = rotated->pointAt(t);
                    QPointF screenPt = viewport.worldToScreen(worldPt);
                    if (i > 0) {
                        painter.drawLine(prevPoint, screenPt);
                    }
                    prevPoint = screenPt;
                }
            }
        }
        else if constexpr (std::is_same_v<T, Ellipse2D>) {
            auto rotated = GeometryMath::rotate(entity, *centerPoint_, angleRadians);
            if (rotated) {
                // Draw ellipse as polyline approximation for preview
                const int segments = 48;
                QPointF prevPoint;
                for (int i = 0; i <= segments; ++i) {
                    double t = static_cast<double>(i) / segments;
                    Point2D worldPt = rotated->pointAt(t);
                    QPointF screenPt = viewport.worldToScreen(worldPt);
                    if (i > 0) {
                        painter.drawLine(prevPoint, screenPt);
                    }
                    prevPoint = screenPt;
                }
            }
        }
        else if constexpr (std::is_same_v<T, Point2D>) {
            Point2D rotated = GeometryMath::rotate(entity, *centerPoint_, angleRadians);
            QPointF screenPt = viewport.worldToScreen(rotated);
            // Draw point as small cross
            int size = 4;
            painter.drawLine(
                QPointF(screenPt.x() - size, screenPt.y() - size),
                QPointF(screenPt.x() + size, screenPt.y() + size)
            );
            painter.drawLine(
                QPointF(screenPt.x() - size, screenPt.y() + size),
                QPointF(screenPt.x() + size, screenPt.y() - size)
            );
        }
    }, entityMeta->entity);
}

bool RotateTool::commitRotation() {
    if (!centerPoint_ || !currentPoint_) {
        qWarning() << "RotateTool: Cannot commit - missing center or current point";
        return false;
    }

    double angle = calculateAngle(angleSnapEnabled_);
    return commitRotationWithAngle(angle);
}

bool RotateTool::commitRotationWithAngle(double angleRadians) {
    if (!centerPoint_ || !documentModel_) {
        qWarning() << "RotateTool: Cannot commit - missing center point or document model";
        return false;
    }

    // Skip if zero rotation
    if (std::abs(angleRadians) < GEOMETRY_EPSILON) {
        qDebug() << "RotateTool: Zero rotation - no-op";
        return false;
    }

    double angleDegrees = angleRadians * 180.0 / PI;

    // Use command system if available (enables undo/redo)
    if (commandHistory_) {
        auto command = std::make_unique<Model::RotateEntitiesCommand>(
            documentModel_,
            selectedHandles_,
            *centerPoint_,
            angleRadians
        );

        if (!commandHistory_->executeCommand(std::move(command))) {
            qWarning() << "RotateTool: Failed to execute rotate command";
            return false;
        }
        qDebug() << "RotateTool: Rotated" << selectedHandles_.size() << "entities by"
                 << angleDegrees << "degrees via command system";
        return true;
    }

    // Fallback: direct modification (no undo support)
    int successCount = 0;
    int failCount = 0;

    for (const auto& handle : selectedHandles_) {
        auto* entityMeta = documentModel_->findEntityByHandle(handle);
        if (!entityMeta) {
            failCount++;
            continue;
        }

        // Rotate based on entity type
        bool updated = std::visit([&](auto&& entity) -> bool {
            using T = std::decay_t<decltype(entity)>;

            if constexpr (std::is_same_v<T, Line2D>) {
                auto rotated = GeometryMath::rotate(entity, *centerPoint_, angleRadians);
                if (rotated && rotated->isValid()) {
                    return documentModel_->updateEntity(handle, *rotated);
                }
                return false;
            }
            else if constexpr (std::is_same_v<T, Arc2D>) {
                auto rotated = GeometryMath::rotate(entity, *centerPoint_, angleRadians);
                if (rotated && rotated->isValid()) {
                    return documentModel_->updateEntity(handle, *rotated);
                }
                return false;
            }
            else if constexpr (std::is_same_v<T, Ellipse2D>) {
                auto rotated = GeometryMath::rotate(entity, *centerPoint_, angleRadians);
                if (rotated && rotated->isValid()) {
                    return documentModel_->updateEntity(handle, *rotated);
                }
                return false;
            }
            else if constexpr (std::is_same_v<T, Point2D>) {
                Point2D rotated = GeometryMath::rotate(entity, *centerPoint_, angleRadians);
                return documentModel_->updateEntity(handle, rotated);
            }
            return false;
        }, entityMeta->entity);

        if (updated) {
            successCount++;
        } else {
            failCount++;
        }
    }

    qDebug() << "RotateTool: Rotated" << successCount << "entities by"
             << angleDegrees << "degrees," << failCount << "failed";
    return successCount > 0;
}

void RotateTool::resetToCenter() {
    centerPoint_.reset();
    startPoint_.reset();
    currentPoint_.reset();
    baseAngle_ = 0.0;
    angleSnapEnabled_ = false;
    state_ = ToolState::WaitingForInput;
}

void RotateTool::cancel() {
    resetToCenter();
    selectedHandles_.clear();
    state_ = ToolState::Inactive;
}

} // namespace UI
} // namespace OwnCAD
