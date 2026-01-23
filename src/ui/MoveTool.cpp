#include "ui/MoveTool.h"
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

namespace OwnCAD {
namespace UI {

using namespace OwnCAD::Geometry;
using namespace OwnCAD::Import;

MoveTool::MoveTool() = default;

void MoveTool::setSelectedHandles(const std::vector<std::string>& handles) {
    selectedHandles_ = handles;
}

void MoveTool::activate() {
    // Check if we have selection
    if (selectedHandles_.empty()) {
        qWarning() << "MoveTool: No entities selected - cannot activate";
        state_ = ToolState::Inactive;
        return;
    }

    resetToBasePoint();
    state_ = ToolState::WaitingForInput;
    qDebug() << "MoveTool: Activated with" << selectedHandles_.size() << "entities";
}

void MoveTool::deactivate() {
    cancel();
    qDebug() << "MoveTool: Deactivated";
}

QString MoveTool::statusPrompt() const {
    switch (state_) {
        case ToolState::Inactive:
            return QString();
        case ToolState::WaitingForInput:
            return QStringLiteral("MOVE: Click base point");
        case ToolState::InProgress:
            return QStringLiteral("MOVE: Click destination point (ESC to cancel)");
    }
    return QString();
}

ToolResult MoveTool::handleMousePress(
    const Geometry::Point2D& worldPos,
    QMouseEvent* event
) {
    // Only handle left button
    if (event->button() != Qt::LeftButton) {
        return ToolResult::Ignored;
    }

    if (state_ == ToolState::WaitingForInput) {
        // Set base point
        basePoint_ = worldPos;
        currentPoint_ = worldPos;
        state_ = ToolState::InProgress;
        qDebug() << "MoveTool: Base point set at" << worldPos.x() << "," << worldPos.y();
        return ToolResult::Continue;
    }
    else if (state_ == ToolState::InProgress) {
        // Set destination point and commit
        currentPoint_ = worldPos;

        if (commitMove()) {
            qDebug() << "MoveTool: Move committed";
            cancel();  // Return to inactive after move
            return ToolResult::Completed;
        } else {
            qDebug() << "MoveTool: Move failed or zero displacement";
            return ToolResult::Continue;
        }
    }

    return ToolResult::Ignored;
}

ToolResult MoveTool::handleMouseMove(
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

ToolResult MoveTool::handleMouseRelease(
    const Geometry::Point2D& /*worldPos*/,
    QMouseEvent* /*event*/
) {
    // Move tool doesn't use mouse release
    return ToolResult::Ignored;
}

ToolResult MoveTool::handleKeyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (state_ == ToolState::InProgress) {
            // Cancel current move, go back to base point selection
            resetToBasePoint();
            qDebug() << "MoveTool: Move cancelled, waiting for base point";
            return ToolResult::Continue;
        }
        else if (state_ == ToolState::WaitingForInput) {
            // Exit tool completely
            cancel();
            qDebug() << "MoveTool: Tool exited";
            return ToolResult::Cancelled;
        }
    }

    return ToolResult::Ignored;
}

void MoveTool::render(QPainter& painter, const Viewport& viewport) {
    if (state_ != ToolState::InProgress || !basePoint_ || !currentPoint_) {
        return;
    }

    // Calculate displacement
    double dx = currentPoint_->x() - basePoint_->x();
    double dy = currentPoint_->y() - basePoint_->y();

    painter.save();

    // Draw displacement vector (from base to current)
    QPointF screenBase = viewport.worldToScreen(*basePoint_);
    QPointF screenCurrent = viewport.worldToScreen(*currentPoint_);

    QPen displacementPen(QColor(100, 100, 255));  // Light blue
    displacementPen.setWidth(DISPLACEMENT_LINE_WIDTH);
    displacementPen.setStyle(Qt::DashLine);
    displacementPen.setDashPattern({4, 4});
    painter.setPen(displacementPen);
    painter.drawLine(screenBase, screenCurrent);

    // Draw base point marker (cross)
    painter.setPen(QPen(QColor(255, 100, 100), 2));  // Red
    int crossSize = POINT_MARKER_SIZE;
    painter.drawLine(
        QPointF(screenBase.x() - crossSize, screenBase.y()),
        QPointF(screenBase.x() + crossSize, screenBase.y())
    );
    painter.drawLine(
        QPointF(screenBase.x(), screenBase.y() - crossSize),
        QPointF(screenBase.x(), screenBase.y() + crossSize)
    );

    // Draw current point marker (circle)
    painter.setPen(QPen(QColor(100, 255, 100), 2));  // Green
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(screenCurrent, crossSize / 2.0, crossSize / 2.0);

    // Draw preview of each selected entity at displaced position
    if (documentModel_) {
        for (const auto& handle : selectedHandles_) {
            renderEntityPreview(painter, viewport, handle, dx, dy);
        }
    }

    painter.restore();
}

void MoveTool::renderEntityPreview(
    QPainter& painter,
    const Viewport& viewport,
    const std::string& handle,
    double dx,
    double dy
) {
    const auto* entityMeta = documentModel_->findEntityByHandle(handle);
    if (!entityMeta) {
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
            auto translated = GeometryMath::translate(entity, dx, dy);
            if (translated) {
                QPointF screenStart = viewport.worldToScreen(translated->start());
                QPointF screenEnd = viewport.worldToScreen(translated->end());
                painter.drawLine(screenStart, screenEnd);
            }
        }
        else if constexpr (std::is_same_v<T, Arc2D>) {
            auto translated = GeometryMath::translate(entity, dx, dy);
            if (translated) {
                // Draw arc as polyline approximation for preview
                const int segments = 32;
                QPointF prevPoint;
                for (int i = 0; i <= segments; ++i) {
                    double t = static_cast<double>(i) / segments;
                    Point2D worldPt = translated->pointAt(t);
                    QPointF screenPt = viewport.worldToScreen(worldPt);
                    if (i > 0) {
                        painter.drawLine(prevPoint, screenPt);
                    }
                    prevPoint = screenPt;
                }
            }
        }
        else if constexpr (std::is_same_v<T, Ellipse2D>) {
            auto translated = GeometryMath::translate(entity, dx, dy);
            if (translated) {
                // Draw ellipse as polyline approximation for preview
                const int segments = 48;
                QPointF prevPoint;
                for (int i = 0; i <= segments; ++i) {
                    double t = static_cast<double>(i) / segments;
                    Point2D worldPt = translated->pointAt(t);
                    QPointF screenPt = viewport.worldToScreen(worldPt);
                    if (i > 0) {
                        painter.drawLine(prevPoint, screenPt);
                    }
                    prevPoint = screenPt;
                }
            }
        }
        else if constexpr (std::is_same_v<T, Point2D>) {
            Point2D translated = GeometryMath::translate(entity, dx, dy);
            QPointF screenPt = viewport.worldToScreen(translated);
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

bool MoveTool::commitMove() {
    if (!basePoint_ || !currentPoint_ || !documentModel_) {
        qWarning() << "MoveTool: Cannot commit - missing data";
        return false;
    }

    double dx = currentPoint_->x() - basePoint_->x();
    double dy = currentPoint_->y() - basePoint_->y();

    // Skip if zero displacement
    if (std::abs(dx) < GEOMETRY_EPSILON && std::abs(dy) < GEOMETRY_EPSILON) {
        qDebug() << "MoveTool: Zero displacement - no-op";
        return false;
    }

    // Use command system if available (enables undo/redo)
    if (commandHistory_) {
        auto command = std::make_unique<Model::MoveEntitiesCommand>(
            documentModel_,
            selectedHandles_,
            dx, dy
        );

        if (!commandHistory_->executeCommand(std::move(command))) {
            qWarning() << "MoveTool: Failed to execute move command";
            return false;
        }
        qDebug() << "MoveTool: Moved" << selectedHandles_.size() << "entities via command system"
                 << "dx=" << dx << "dy=" << dy;
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

        // Translate based on entity type
        bool updated = std::visit([&](auto&& entity) -> bool {
            using T = std::decay_t<decltype(entity)>;

            if constexpr (std::is_same_v<T, Line2D>) {
                auto translated = GeometryMath::translate(entity, dx, dy);
                if (translated && translated->isValid()) {
                    return documentModel_->updateEntity(handle, *translated);
                }
                return false;
            }
            else if constexpr (std::is_same_v<T, Arc2D>) {
                auto translated = GeometryMath::translate(entity, dx, dy);
                if (translated && translated->isValid()) {
                    return documentModel_->updateEntity(handle, *translated);
                }
                return false;
            }
            else if constexpr (std::is_same_v<T, Ellipse2D>) {
                auto translated = GeometryMath::translate(entity, dx, dy);
                if (translated && translated->isValid()) {
                    return documentModel_->updateEntity(handle, *translated);
                }
                return false;
            }
            else if constexpr (std::is_same_v<T, Point2D>) {
                Point2D translated = GeometryMath::translate(entity, dx, dy);
                return documentModel_->updateEntity(handle, translated);
            }
            return false;
        }, entityMeta->entity);

        if (updated) {
            successCount++;
        } else {
            failCount++;
        }
    }

    qDebug() << "MoveTool: Moved" << successCount << "entities," << failCount << "failed";
    return successCount > 0;
}

void MoveTool::resetToBasePoint() {
    basePoint_.reset();
    currentPoint_.reset();
    state_ = ToolState::WaitingForInput;
}

void MoveTool::cancel() {
    resetToBasePoint();
    selectedHandles_.clear();
    state_ = ToolState::Inactive;
}

} // namespace UI
} // namespace OwnCAD
