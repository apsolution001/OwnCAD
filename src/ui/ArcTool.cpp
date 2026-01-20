#include "ui/ArcTool.h"
#include "ui/CADCanvas.h"
#include "model/DocumentModel.h"
#include "geometry/GeometryConstants.h"
#include <QPen>
#include <QBrush>
#include <QPainterPath>
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

ToolResult ArcTool::handleMousePress(
    const Geometry::Point2D& worldPos,
    QMouseEvent* event
) {
    // Only handle left button
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

        case ArcState::WaitingStartPoint: {
            // Calculate radius from center to start point
            double dist = centerPoint_->distanceTo(worldPos);

            // Validate radius
            if (dist < Geometry::MIN_ARC_RADIUS) {
                qWarning() << "ArcTool: Radius too small (" << dist << "), pick different start point";
                return ToolResult::Continue;
            }

            startPoint_ = worldPos;
            radius_ = dist;
            startAngle_ = angleToPoint(*centerPoint_, *startPoint_);
            currentAngle_ = startAngle_;
            currentPoint_ = worldPos;
            arcState_ = ArcState::WaitingEndPoint;
            qDebug() << "ArcTool: Start point set, radius=" << radius_
                     << ", startAngle=" << (startAngle_ * 180.0 / Geometry::PI) << "deg";
            return ToolResult::Continue;
        }

        case ArcState::WaitingEndPoint: {
            currentPoint_ = worldPos;
            currentAngle_ = angleToPoint(*centerPoint_, *currentPoint_);

            if (commitArc()) {
                // Stay active for next arc (continuous drawing mode)
                resetToCenterPoint();
                return ToolResult::Completed;
            }
            // If commit failed (e.g., sweep too small), stay in current state
            return ToolResult::Continue;
        }

        default:
            return ToolResult::Ignored;
    }
}

ToolResult ArcTool::handleMouseMove(
    const Geometry::Point2D& worldPos,
    QMouseEvent* /*event*/
) {
    currentPoint_ = worldPos;

    if (arcState_ == ArcState::WaitingStartPoint) {
        // Show radius preview line from center to cursor
        return ToolResult::Continue;
    }

    if (arcState_ == ArcState::WaitingEndPoint) {
        // Update preview arc angle
        currentAngle_ = angleToPoint(*centerPoint_, *currentPoint_);
        return ToolResult::Continue;
    }

    return ToolResult::Ignored;
}

ToolResult ArcTool::handleMouseRelease(
    const Geometry::Point2D& /*worldPos*/,
    QMouseEvent* /*event*/
) {
    // Arc tool doesn't use mouse release
    return ToolResult::Ignored;
}

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
            // Cancel current arc, stay in tool
            resetToCenterPoint();
            qDebug() << "ArcTool: Current arc cancelled";
            return ToolResult::Continue;
        }
        else if (arcState_ == ArcState::WaitingCenterPoint) {
            // Exit tool completely
            cancel();
            qDebug() << "ArcTool: Tool exited";
            return ToolResult::Cancelled;
        }
    }

    return ToolResult::Ignored;
}

void ArcTool::render(QPainter& painter, const Viewport& viewport) {
    painter.save();

    // Preview style - dashed light blue line
    QPen previewPen(QColor(102, 170, 255));  // Light blue #66AAFF
    previewPen.setWidth(PREVIEW_LINE_WIDTH);
    previewPen.setStyle(Qt::DashLine);
    previewPen.setDashPattern({6, 4});

    // State: WaitingStartPoint - draw radius line from center to cursor
    if (arcState_ == ArcState::WaitingStartPoint && centerPoint_ && currentPoint_) {
        QPointF screenCenter = viewport.worldToScreen(*centerPoint_);
        QPointF screenCursor = viewport.worldToScreen(*currentPoint_);

        painter.setPen(previewPen);
        painter.drawLine(screenCenter, screenCursor);

        // Draw center marker (blue filled circle)
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(102, 170, 255));
        painter.drawEllipse(
            screenCenter,
            POINT_MARKER_SIZE / 2.0,
            POINT_MARKER_SIZE / 2.0
        );
    }

    // State: WaitingEndPoint - draw preview arc
    if (arcState_ == ArcState::WaitingEndPoint && centerPoint_ && startPoint_ && currentPoint_) {
        renderArcPreview(painter, viewport);
    }

    painter.restore();
}

void ArcTool::renderArcPreview(QPainter& painter, const Viewport& viewport) {
    // Calculate sweep angle based on direction
    double sweepAngle;
    if (counterClockwise_) {
        sweepAngle = currentAngle_ - startAngle_;
        if (sweepAngle <= 0) sweepAngle += 2.0 * Geometry::PI;
    } else {
        sweepAngle = startAngle_ - currentAngle_;
        if (sweepAngle <= 0) sweepAngle += 2.0 * Geometry::PI;
    }

    // Use point-sampling approach (same as CADCanvas::renderArc)
    // This avoids complex Qt angle transformation issues
    const int numSamples = std::max(12, static_cast<int>(sweepAngle * 180.0 / Geometry::PI / 15.0));

    QPainterPath path;
    bool firstPoint = true;

    for (int i = 0; i <= numSamples; ++i) {
        double t = static_cast<double>(i) / numSamples;

        // Calculate angle at parameter t
        double angle;
        if (counterClockwise_) {
            angle = startAngle_ + t * sweepAngle;
        } else {
            angle = startAngle_ - t * sweepAngle;
        }

        // Calculate point on arc at this angle
        double x = centerPoint_->x() + radius_ * std::cos(angle);
        double y = centerPoint_->y() + radius_ * std::sin(angle);
        Geometry::Point2D worldPt(x, y);
        QPointF screenPt = viewport.worldToScreen(worldPt);

        if (firstPoint) {
            path.moveTo(screenPt);
            firstPoint = false;
        } else {
            path.lineTo(screenPt);
        }
    }

    // Draw preview arc (dashed light blue)
    QPen previewPen(QColor(102, 170, 255));
    previewPen.setWidth(PREVIEW_LINE_WIDTH);
    previewPen.setStyle(Qt::DashLine);
    previewPen.setDashPattern({6, 4});
    painter.setPen(previewPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // Draw center marker (blue)
    QPointF screenCenter = viewport.worldToScreen(*centerPoint_);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(102, 170, 255));
    painter.drawEllipse(screenCenter, POINT_MARKER_SIZE / 2.0, POINT_MARKER_SIZE / 2.0);

    // Draw start point marker (green)
    QPointF screenStart = viewport.worldToScreen(*startPoint_);
    painter.setBrush(QColor(0, 200, 0));
    painter.drawEllipse(screenStart, POINT_MARKER_SIZE / 2.0, POINT_MARKER_SIZE / 2.0);

    // Draw direction indicator arrow
    renderDirectionArrow(painter, viewport);
}

void ArcTool::renderDirectionArrow(QPainter& painter, const Viewport& viewport) {
    // Calculate sweep angle
    double sweepAngle;
    if (counterClockwise_) {
        sweepAngle = currentAngle_ - startAngle_;
        if (sweepAngle <= 0) sweepAngle += 2.0 * Geometry::PI;
    } else {
        sweepAngle = startAngle_ - currentAngle_;
        if (sweepAngle <= 0) sweepAngle += 2.0 * Geometry::PI;
    }

    // Don't draw arrow if sweep is too small
    if (sweepAngle < 0.1) return;

    // Arrow at midpoint of arc
    double midT = 0.5;
    double midAngle;
    if (counterClockwise_) {
        midAngle = startAngle_ + midT * sweepAngle;
    } else {
        midAngle = startAngle_ - midT * sweepAngle;
    }

    // Point on arc at midpoint
    double midX = centerPoint_->x() + radius_ * std::cos(midAngle);
    double midY = centerPoint_->y() + radius_ * std::sin(midAngle);
    QPointF screenMid = viewport.worldToScreen(Geometry::Point2D(midX, midY));

    // Tangent direction (perpendicular to radius)
    double tangentAngle;
    if (counterClockwise_) {
        tangentAngle = midAngle + Geometry::PI / 2.0;  // 90 degrees ahead
    } else {
        tangentAngle = midAngle - Geometry::PI / 2.0;  // 90 degrees behind
    }

    // Arrow points
    double arrowLen = DIRECTION_ARROW_SIZE;
    double arrowWidth = arrowLen * 0.5;

    // Tip of arrow (in tangent direction)
    QPointF tip(
        screenMid.x() + arrowLen * std::cos(-tangentAngle),  // Negate for screen Y inversion
        screenMid.y() + arrowLen * std::sin(-tangentAngle)
    );

    // Base points of arrow (perpendicular to tangent)
    double perpAngle = tangentAngle + Geometry::PI / 2.0;
    QPointF base1(
        screenMid.x() + arrowWidth * 0.5 * std::cos(-perpAngle),
        screenMid.y() + arrowWidth * 0.5 * std::sin(-perpAngle)
    );
    QPointF base2(
        screenMid.x() - arrowWidth * 0.5 * std::cos(-perpAngle),
        screenMid.y() - arrowWidth * 0.5 * std::sin(-perpAngle)
    );

    // Draw arrow
    QPolygonF arrow;
    arrow << tip << base1 << base2;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 150, 0));  // Orange for direction
    painter.drawPolygon(arrow);
}

bool ArcTool::commitArc() {
    if (!centerPoint_ || !startPoint_ || !documentModel_) {
        qWarning() << "ArcTool: Cannot commit - missing data";
        return false;
    }

    // Calculate sweep angle for validation
    double sweepAngle;
    if (counterClockwise_) {
        sweepAngle = currentAngle_ - startAngle_;
        if (sweepAngle <= 0) sweepAngle += 2.0 * Geometry::PI;
    } else {
        sweepAngle = startAngle_ - currentAngle_;
        if (sweepAngle <= 0) sweepAngle += 2.0 * Geometry::PI;
    }

    if (sweepAngle < Geometry::MIN_ARC_SWEEP) {
        qWarning() << "ArcTool: Sweep angle too small (" << sweepAngle << " rad)";
        return false;
    }

    // Create arc using factory method
    // Arc2D::create expects start and end angles, and direction
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
             << "sweep=" << (sweepAngle * 180.0 / Geometry::PI) << "deg"
             << (counterClockwise_ ? "CCW" : "CW");
    return true;
}

double ArcTool::angleToPoint(const Geometry::Point2D& from, const Geometry::Point2D& to) const {
    double dx = to.x() - from.x();
    double dy = to.y() - from.y();
    double angle = std::atan2(dy, dx);
    // Normalize to [0, 2*PI)
    if (angle < 0) {
        angle += 2.0 * Geometry::PI;
    }
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

} // namespace UI
} // namespace OwnCAD
