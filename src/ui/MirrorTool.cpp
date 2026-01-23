#include "ui/MirrorTool.h"
#include "ui/CADCanvas.h"
#include "model/DocumentModel.h"
#include "model/CommandHistory.h"
#include "model/EntityCommands.h"
#include "geometry/GeometryMath.h"
#include "geometry/GeometryConstants.h"
#include "geometry/BoundingBox.h"
#include "import/GeometryConverter.h"
#include <QPen>
#include <QBrush>
#include <QDebug>
#include <cmath>

namespace OwnCAD {
namespace UI {

using namespace OwnCAD::Geometry;
using namespace OwnCAD::Import;

MirrorTool::MirrorTool() = default;

void MirrorTool::setSelectedHandles(const std::vector<std::string>& handles) {
    selectedHandles_ = handles;
}

void MirrorTool::activate() {
    if (selectedHandles_.empty()) {
        qWarning() << "MirrorTool: No entities selected - cannot activate";
        state_ = ToolState::Inactive;
        return;
    }

    resetToAxisPoint1();
    state_ = ToolState::WaitingForInput;
    qDebug() << "MirrorTool: Activated with" << selectedHandles_.size() << "entities";
}

void MirrorTool::deactivate() {
    cancel();
    qDebug() << "MirrorTool: Deactivated";
}

QString MirrorTool::statusPrompt() const {
    switch (state_) {
        case ToolState::Inactive:
            return QString();
        case ToolState::WaitingForInput:
            return QStringLiteral("MIRROR: Click first axis point (X=horiz, Y=vert, Tab=toggle copy)");
        case ToolState::InProgress:
            return QString("MIRROR: Click second axis point [%1] (ESC to cancel)")
                .arg(keepOriginal_ ? "COPY" : "REPLACE");
    }
    return QString();
}

ToolResult MirrorTool::handleMousePress(
    const Geometry::Point2D& worldPos,
    QMouseEvent* event
) {
    if (event->button() != Qt::LeftButton) {
        return ToolResult::Ignored;
    }

    if (state_ == ToolState::WaitingForInput) {
        // First axis point
        axisPoint1_ = worldPos;
        currentPoint_ = worldPos;
        state_ = ToolState::InProgress;
        qDebug() << "MirrorTool: Axis point 1 set at" << worldPos.x() << "," << worldPos.y();
        return ToolResult::Continue;
    }
    else if (state_ == ToolState::InProgress) {
        // Second axis point - commit
        axisPoint2_ = worldPos;

        if (commitMirror()) {
            qDebug() << "MirrorTool: Mirror committed";
            cancel();
            return ToolResult::Completed;
        } else {
            qDebug() << "MirrorTool: Mirror failed";
            return ToolResult::Continue;
        }
    }

    return ToolResult::Ignored;
}

ToolResult MirrorTool::handleMouseMove(
    const Geometry::Point2D& worldPos,
    QMouseEvent* /*event*/
) {
    currentPoint_ = worldPos;

    if (state_ == ToolState::InProgress) {
        return ToolResult::Continue;
    }

    return ToolResult::Ignored;
}

ToolResult MirrorTool::handleMouseRelease(
    const Geometry::Point2D& /*worldPos*/,
    QMouseEvent* /*event*/
) {
    return ToolResult::Ignored;
}

ToolResult MirrorTool::handleKeyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (state_ == ToolState::InProgress) {
            resetToAxisPoint1();
            qDebug() << "MirrorTool: Axis cancelled, waiting for first point";
            return ToolResult::Continue;
        }
        else if (state_ == ToolState::WaitingForInput) {
            cancel();
            qDebug() << "MirrorTool: Tool exited";
            return ToolResult::Cancelled;
        }
    }
    else if (event->key() == Qt::Key_Tab) {
        // Toggle keep original
        keepOriginal_ = !keepOriginal_;
        qDebug() << "MirrorTool: Keep original =" << keepOriginal_;
        return ToolResult::Continue;
    }
    else if (state_ == ToolState::WaitingForInput) {
        // Quick axis shortcuts
        Point2D center = calculateSelectionCenter();

        if (event->key() == Qt::Key_X) {
            // Horizontal axis (mirror over X-axis through selection center)
            axisPoint1_ = Point2D(center.x() - 1000, center.y());
            axisPoint2_ = Point2D(center.x() + 1000, center.y());

            if (commitMirror()) {
                cancel();
                return ToolResult::Completed;
            }
            return ToolResult::Continue;
        }
        else if (event->key() == Qt::Key_Y) {
            // Vertical axis (mirror over Y-axis through selection center)
            axisPoint1_ = Point2D(center.x(), center.y() - 1000);
            axisPoint2_ = Point2D(center.x(), center.y() + 1000);

            if (commitMirror()) {
                cancel();
                return ToolResult::Completed;
            }
            return ToolResult::Continue;
        }
    }

    return ToolResult::Ignored;
}

void MirrorTool::render(QPainter& painter, const Viewport& viewport) {
    if (state_ == ToolState::Inactive) {
        return;
    }

    painter.save();

    // Draw axis line preview
    renderAxisPreview(painter, viewport);

    // Draw mirrored entities preview
    if (state_ == ToolState::InProgress && axisPoint1_ && currentPoint_) {
        renderMirroredEntitiesPreview(painter, viewport);
    }

    painter.restore();
}

void MirrorTool::renderAxisPreview(QPainter& painter, const Viewport& viewport) {
    if (!axisPoint1_) {
        return;
    }

    Point2D p1 = *axisPoint1_;
    Point2D p2 = currentPoint_ ? *currentPoint_ : p1;

    QPointF screenP1 = viewport.worldToScreen(p1);
    QPointF screenP2 = viewport.worldToScreen(p2);

    // Draw axis line (dashed magenta)
    QPen axisPen(QColor(255, 0, 255));  // Magenta
    axisPen.setWidth(AXIS_LINE_WIDTH);
    axisPen.setStyle(Qt::DashLine);
    axisPen.setDashPattern({8, 4});
    painter.setPen(axisPen);
    painter.drawLine(screenP1, screenP2);

    // Draw axis point markers
    painter.setPen(QPen(QColor(255, 0, 255), 2));

    // First point: filled circle
    painter.setBrush(QColor(255, 0, 255));
    painter.drawEllipse(screenP1, POINT_MARKER_SIZE / 2.0, POINT_MARKER_SIZE / 2.0);

    // Second point: hollow circle
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(screenP2, POINT_MARKER_SIZE / 2.0, POINT_MARKER_SIZE / 2.0);
}

void MirrorTool::renderMirroredEntitiesPreview(QPainter& painter, const Viewport& viewport) {
    if (!documentModel_ || !axisPoint1_ || !currentPoint_) {
        return;
    }

    // Preview pen: semi-transparent magenta
    QPen previewPen(QColor(255, 100, 255, 150));
    previewPen.setWidth(PREVIEW_LINE_WIDTH);
    previewPen.setStyle(Qt::DashLine);
    previewPen.setDashPattern({4, 4});
    painter.setPen(previewPen);
    painter.setBrush(Qt::NoBrush);

    for (const auto& handle : selectedHandles_) {
        renderEntityMirrored(painter, viewport, handle);
    }
}

void MirrorTool::renderEntityMirrored(
    QPainter& painter,
    const Viewport& viewport,
    const std::string& handle
) {
    const auto* entityMeta = documentModel_->findEntityByHandle(handle);
    if (!entityMeta) {
        return;
    }

    Point2D axisP1 = *axisPoint1_;
    Point2D axisP2 = *currentPoint_;

    std::visit([&](auto&& entity) {
        using T = std::decay_t<decltype(entity)>;

        if constexpr (std::is_same_v<T, Line2D>) {
            auto mirrored = GeometryMath::mirror(entity, axisP1, axisP2);
            if (mirrored) {
                QPointF screenStart = viewport.worldToScreen(mirrored->start());
                QPointF screenEnd = viewport.worldToScreen(mirrored->end());
                painter.drawLine(screenStart, screenEnd);
            }
        }
        else if constexpr (std::is_same_v<T, Arc2D>) {
            auto mirrored = GeometryMath::mirror(entity, axisP1, axisP2);
            if (mirrored) {
                const int segments = 32;
                QPointF prevPoint;
                for (int i = 0; i <= segments; ++i) {
                    double t = static_cast<double>(i) / segments;
                    Point2D worldPt = mirrored->pointAt(t);
                    QPointF screenPt = viewport.worldToScreen(worldPt);
                    if (i > 0) {
                        painter.drawLine(prevPoint, screenPt);
                    }
                    prevPoint = screenPt;
                }
            }
        }
        else if constexpr (std::is_same_v<T, Ellipse2D>) {
            auto mirrored = GeometryMath::mirror(entity, axisP1, axisP2);
            if (mirrored) {
                const int segments = 48;
                QPointF prevPoint;
                for (int i = 0; i <= segments; ++i) {
                    double t = static_cast<double>(i) / segments;
                    Point2D worldPt = mirrored->pointAt(t);
                    QPointF screenPt = viewport.worldToScreen(worldPt);
                    if (i > 0) {
                        painter.drawLine(prevPoint, screenPt);
                    }
                    prevPoint = screenPt;
                }
            }
        }
        else if constexpr (std::is_same_v<T, Point2D>) {
            Point2D mirrored = GeometryMath::mirror(entity, axisP1, axisP2);
            QPointF screenPt = viewport.worldToScreen(mirrored);
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

bool MirrorTool::commitMirror() {
    if (!axisPoint1_ || !axisPoint2_ || !documentModel_) {
        qWarning() << "MirrorTool: Cannot commit - missing data";
        return false;
    }

    // Check for degenerate axis (same point)
    double axisDx = axisPoint2_->x() - axisPoint1_->x();
    double axisDy = axisPoint2_->y() - axisPoint1_->y();
    double axisLen = std::sqrt(axisDx * axisDx + axisDy * axisDy);

    if (axisLen < GEOMETRY_EPSILON) {
        qWarning() << "MirrorTool: Degenerate axis (same point)";
        return false;
    }

    // Use command system if available
    if (commandHistory_) {
        auto command = std::make_unique<Model::MirrorEntitiesCommand>(
            documentModel_,
            selectedHandles_,
            *axisPoint1_,
            *axisPoint2_,
            keepOriginal_
        );

        if (!commandHistory_->executeCommand(std::move(command))) {
            qWarning() << "MirrorTool: Failed to execute mirror command";
            return false;
        }
        qDebug() << "MirrorTool: Mirrored" << selectedHandles_.size()
                 << "entities, keepOriginal=" << keepOriginal_;
        return true;
    }

    qWarning() << "MirrorTool: No command history available";
    return false;
}

Point2D MirrorTool::calculateSelectionCenter() const {
    if (selectedHandles_.empty() || !documentModel_) {
        return Point2D(0, 0);
    }

    BoundingBox merged;
    bool first = true;

    for (const auto& handle : selectedHandles_) {
        const auto* entityMeta = documentModel_->findEntityByHandle(handle);
        if (!entityMeta) continue;

        std::optional<BoundingBox> bbox = std::visit([](auto&& entity) -> std::optional<BoundingBox> {
            using T = std::decay_t<decltype(entity)>;

            if constexpr (std::is_same_v<T, Line2D>) {
                return BoundingBox::fromLine(entity);
            } else if constexpr (std::is_same_v<T, Arc2D>) {
                return BoundingBox::fromArc(entity);
            } else if constexpr (std::is_same_v<T, Ellipse2D>) {
                // Approximate ellipse bounding box using center and axis lengths
                Point2D center = entity.center();
                double maxRadius = std::max(entity.majorAxisLength(), entity.minorAxisLength());
                return BoundingBox::fromPoints(
                    Point2D(center.x() - maxRadius, center.y() - maxRadius),
                    Point2D(center.x() + maxRadius, center.y() + maxRadius)
                );
            } else if constexpr (std::is_same_v<T, Point2D>) {
                return BoundingBox::fromPoints(entity, entity);
            }
            return std::nullopt;
        }, entityMeta->entity);

        if (bbox) {
            if (first) {
                merged = *bbox;
                first = false;
            } else {
                merged = merged.merge(*bbox);
            }
        }
    }

    return merged.center();
}

void MirrorTool::resetToAxisPoint1() {
    axisPoint1_.reset();
    axisPoint2_.reset();
    currentPoint_.reset();
    state_ = ToolState::WaitingForInput;
}

void MirrorTool::cancel() {
    resetToAxisPoint1();
    selectedHandles_.clear();
    keepOriginal_ = false;
    state_ = ToolState::Inactive;
}

} // namespace UI
} // namespace OwnCAD
