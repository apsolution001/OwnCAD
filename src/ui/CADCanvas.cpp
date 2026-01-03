#include "ui/CADCanvas.h"
#include "geometry/GeometryConstants.h"
#include "geometry/BoundingBox.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>
#include <cmath>
#include <variant>

namespace OwnCAD {
namespace UI {

// ============================================================================
// VIEWPORT IMPLEMENTATION
// ============================================================================

Viewport::Viewport()
    : panX_(0.0)
    , panY_(0.0)
    , zoomLevel_(1.0)
    , viewportWidth_(800)
    , viewportHeight_(600) {
}

QPointF Viewport::worldToScreen(const Geometry::Point2D& worldPoint) const {
    // Apply zoom and pan transformation
    // Screen Y is inverted (grows downward), world Y grows upward
    double screenX = worldPoint.x() * zoomLevel_ + panX_ + viewportWidth_ / 2.0;
    double screenY = -worldPoint.y() * zoomLevel_ + panY_ + viewportHeight_ / 2.0;
    return QPointF(screenX, screenY);
}

Geometry::Point2D Viewport::screenToWorld(const QPointF& screenPoint) const {
    // Reverse the transformation
    double worldX = (screenPoint.x() - panX_ - viewportWidth_ / 2.0) / zoomLevel_;
    double worldY = -(screenPoint.y() - panY_ - viewportHeight_ / 2.0) / zoomLevel_;
    return Geometry::Point2D(worldX, worldY);
}

void Viewport::pan(double dx, double dy) {
    panX_ += dx;
    panY_ += dy;
}

void Viewport::setPan(double x, double y) {
    panX_ = x;
    panY_ = y;
}

void Viewport::zoom(double factor, const QPointF& center) {
    // Zoom centered on a specific point (usually mouse cursor)
    Geometry::Point2D worldCenter = screenToWorld(center);

    double newZoom = zoomLevel_ * factor;

    // Clamp zoom level
    const double MIN_ZOOM = 0.001;
    const double MAX_ZOOM = 1000.0;
    newZoom = std::max(MIN_ZOOM, std::min(newZoom, MAX_ZOOM));

    zoomLevel_ = newZoom;

    // Adjust pan to keep the same world point under the cursor
    QPointF newScreenCenter = worldToScreen(worldCenter);
    panX_ += center.x() - newScreenCenter.x();
    panY_ += center.y() - newScreenCenter.y();
}

void Viewport::setZoom(double level) {
    const double MIN_ZOOM = 0.001;
    const double MAX_ZOOM = 1000.0;
    zoomLevel_ = std::max(MIN_ZOOM, std::min(level, MAX_ZOOM));
}

void Viewport::reset() {
    panX_ = 0.0;
    panY_ = 0.0;
    zoomLevel_ = 1.0;
}

void Viewport::setViewportSize(int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
}

// ============================================================================
// SNAP MANAGER IMPLEMENTATION
// ============================================================================

SnapManager::SnapManager()
    : snapModes_(0)
    , gridSpacing_(10.0)
    , snapTolerance_(10.0) {  // 10 pixels
}

void SnapManager::setSnapMode(SnapMode mode, bool enabled) {
    if (enabled) {
        snapModes_ |= static_cast<int>(mode);
    } else {
        snapModes_ &= ~static_cast<int>(mode);
    }
}

bool SnapManager::isSnapEnabled(SnapMode mode) const {
    return (snapModes_ & static_cast<int>(mode)) != 0;
}

std::optional<Geometry::Point2D> SnapManager::snap(
    const Geometry::Point2D& point,
    const std::vector<Import::GeometryEntityWithMetadata>& entities
) const {
    // Priority: Endpoint > Midpoint > Grid

    if (isSnapEnabled(SnapMode::Endpoint)) {
        auto endpointSnap = snapToEndpoint(point, entities);
        if (endpointSnap) {
            return endpointSnap;
        }
    }

    if (isSnapEnabled(SnapMode::Grid)) {
        return snapToGrid(point);
    }

    return point;  // No snap
}

Geometry::Point2D SnapManager::snapToGrid(const Geometry::Point2D& point) const {
    double snappedX = std::round(point.x() / gridSpacing_) * gridSpacing_;
    double snappedY = std::round(point.y() / gridSpacing_) * gridSpacing_;
    return Geometry::Point2D(snappedX, snappedY);
}

std::optional<Geometry::Point2D> SnapManager::snapToEndpoint(
    const Geometry::Point2D& point,
    const std::vector<Import::GeometryEntityWithMetadata>& entities
) const {
    std::optional<Geometry::Point2D> closestPoint;
    double closestDistSq = snapTolerance_ * snapTolerance_;

    for (const auto& entityWithMeta : entities) {
        const auto& entity = entityWithMeta.entity;

        std::vector<Geometry::Point2D> endpoints;

        if (std::holds_alternative<Geometry::Line2D>(entity)) {
            const auto& line = std::get<Geometry::Line2D>(entity);
            endpoints.push_back(line.start());
            endpoints.push_back(line.end());
        } else if (std::holds_alternative<Geometry::Arc2D>(entity)) {
            const auto& arc = std::get<Geometry::Arc2D>(entity);
            endpoints.push_back(arc.startPoint());
            endpoints.push_back(arc.endPoint());
        }

        for (const auto& endpoint : endpoints) {
            double distSq = point.distanceSquaredTo(endpoint);
            if (distSq < closestDistSq) {
                closestDistSq = distSq;
                closestPoint = endpoint;
            }
        }
    }

    return closestPoint;
}

// ============================================================================
// CAD CANVAS IMPLEMENTATION
// ============================================================================

CADCanvas::CADCanvas(QWidget* parent)
    : QWidget(parent)
    , entities_()
    , viewport_()
    , snapManager_()
    , gridVisible_(true)
    , gridSpacing_(10.0)
    , isPanning_(false)
    , lastMousePos_()
    , lastWorldPos_(0, 0) {

    setMouseTracking(true);  // Enable mouse move events without button press
    setFocusPolicy(Qt::StrongFocus);

    // Set background color
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(250, 250, 250));
    setPalette(pal);
}

CADCanvas::~CADCanvas() {
}

void CADCanvas::setEntities(const std::vector<Import::GeometryEntityWithMetadata>& entities) {
    entities_ = entities;
    qDebug() << "CADCanvas::setEntities() - Received" << entities_.size() << "entities";
    update();  // Trigger repaint
}

void CADCanvas::clear() {
    entities_.clear();
    update();
}

void CADCanvas::setGridVisible(bool visible) {
    gridVisible_ = visible;
    update();
}

void CADCanvas::setGridSpacing(double spacing) {
    gridSpacing_ = spacing;
    snapManager_.setGridSpacing(spacing);
    update();
}

void CADCanvas::setSnapEnabled(SnapManager::SnapMode mode, bool enabled) {
    snapManager_.setSnapMode(mode, enabled);
}

bool CADCanvas::isSnapEnabled(SnapManager::SnapMode mode) const {
    return snapManager_.isSnapEnabled(mode);
}

void CADCanvas::resetView() {
    viewport_.reset();
    update();
    emit viewportChanged(viewport_.zoomLevel(), viewport_.panX(), viewport_.panY());
}

void CADCanvas::zoomExtents() {
    if (entities_.empty()) {
        resetView();
        return;
    }

    // Calculate bounding box of all entities
    std::optional<Geometry::BoundingBox> totalBBox;

    for (const auto& entityWithMeta : entities_) {
        const auto& entity = entityWithMeta.entity;

        Geometry::BoundingBox bbox;
        if (std::holds_alternative<Geometry::Line2D>(entity)) {
            bbox = std::get<Geometry::Line2D>(entity).boundingBox();
        } else if (std::holds_alternative<Geometry::Arc2D>(entity)) {
            bbox = std::get<Geometry::Arc2D>(entity).boundingBox();
        }

        if (totalBBox) {
            totalBBox = totalBBox->merge(bbox);
        } else {
            totalBBox = bbox;
        }
    }

    if (!totalBBox) {
        resetView();
        return;
    }

    // Calculate zoom to fit
    double bboxWidth = totalBBox->width();
    double bboxHeight = totalBBox->height();

    if (bboxWidth < Geometry::GEOMETRY_EPSILON || bboxHeight < Geometry::GEOMETRY_EPSILON) {
        resetView();
        return;
    }

    double zoomX = (width() * 0.9) / bboxWidth;
    double zoomY = (height() * 0.9) / bboxHeight;
    double newZoom = std::min(zoomX, zoomY);

    viewport_.setZoom(newZoom);

    // Center on bounding box center
    Geometry::Point2D center = totalBBox->center();
    QPointF screenCenter = viewport_.worldToScreen(center);
    viewport_.setPan(width() / 2.0 - screenCenter.x() + viewport_.panX(),
                     height() / 2.0 - screenCenter.y() + viewport_.panY());

    update();
    emit viewportChanged(viewport_.zoomLevel(), viewport_.panX(), viewport_.panY());
}

// ============================================================================
// RENDERING
// ============================================================================

void CADCanvas::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);  // Qt macro to mark parameter as intentionally unused

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Render grid first (background)
    if (gridVisible_) {
        renderGrid(painter);
    }

    // Render origin axes
    renderOrigin(painter);

    // Render all entities
    renderEntities(painter);
}

void CADCanvas::renderGrid(QPainter& painter) {
    painter.setPen(QPen(QColor(220, 220, 220), 1));

    // Calculate grid spacing in screen space
    double gridScreenSpacing = gridSpacing_ * viewport_.zoomLevel();

    // Adaptive grid: hide if too dense or too sparse
    if (gridScreenSpacing < 5.0 || gridScreenSpacing > 200.0) {
        return;  // Grid too dense or too sparse
    }

    // Get visible world bounds
    Geometry::Point2D topLeft = viewport_.screenToWorld(QPointF(0, 0));
    Geometry::Point2D bottomRight = viewport_.screenToWorld(QPointF(width(), height()));

    double minX = std::floor(topLeft.x() / gridSpacing_) * gridSpacing_;
    double maxX = std::ceil(bottomRight.x() / gridSpacing_) * gridSpacing_;
    double minY = std::floor(bottomRight.y() / gridSpacing_) * gridSpacing_;
    double maxY = std::ceil(topLeft.y() / gridSpacing_) * gridSpacing_;

    // Draw vertical grid lines
    for (double x = minX; x <= maxX; x += gridSpacing_) {
        QPointF p1 = viewport_.worldToScreen(Geometry::Point2D(x, minY));
        QPointF p2 = viewport_.worldToScreen(Geometry::Point2D(x, maxY));

        // Major grid lines every 10 units
        if (std::abs(std::fmod(x, gridSpacing_ * 10.0)) < Geometry::GEOMETRY_EPSILON) {
            painter.setPen(QPen(QColor(180, 180, 180), 1));
        } else {
            painter.setPen(QPen(QColor(220, 220, 220), 1));
        }

        painter.drawLine(p1, p2);
    }

    // Draw horizontal grid lines
    for (double y = minY; y <= maxY; y += gridSpacing_) {
        QPointF p1 = viewport_.worldToScreen(Geometry::Point2D(minX, y));
        QPointF p2 = viewport_.worldToScreen(Geometry::Point2D(maxX, y));

        // Major grid lines every 10 units
        if (std::abs(std::fmod(y, gridSpacing_ * 10.0)) < Geometry::GEOMETRY_EPSILON) {
            painter.setPen(QPen(QColor(180, 180, 180), 1));
        } else {
            painter.setPen(QPen(QColor(220, 220, 220), 1));
        }

        painter.drawLine(p1, p2);
    }
}

void CADCanvas::renderOrigin(QPainter& painter) {
    // Draw X axis (red) and Y axis (green) at origin
    Geometry::Point2D origin(0, 0);
    QPointF originScreen = viewport_.worldToScreen(origin);

    // X axis (red, horizontal right)
    painter.setPen(QPen(QColor(255, 0, 0, 128), 2));
    painter.drawLine(originScreen, originScreen + QPointF(50, 0));

    // Y axis (green, vertical up)
    painter.setPen(QPen(QColor(0, 200, 0, 128), 2));
    painter.drawLine(originScreen, originScreen + QPointF(0, -50));
}

void CADCanvas::renderEntities(QPainter& painter) {
    qDebug() << "CADCanvas::renderEntities() - Rendering" << entities_.size() << "entities";

    int lineCount = 0;
    int arcCount = 0;

    for (const auto& entityWithMeta : entities_) {
        if (std::holds_alternative<Geometry::Line2D>(entityWithMeta.entity)) {
            lineCount++;
        } else if (std::holds_alternative<Geometry::Arc2D>(entityWithMeta.entity)) {
            arcCount++;
        }
        renderEntity(painter, entityWithMeta);
    }

    qDebug() << "  Lines rendered:" << lineCount;
    qDebug() << "  Arcs rendered:" << arcCount;
}

void CADCanvas::renderEntity(QPainter& painter, const Import::GeometryEntityWithMetadata& entityWithMeta) {
    const auto& entity = entityWithMeta.entity;

    if (std::holds_alternative<Geometry::Line2D>(entity)) {
        renderLine(painter, std::get<Geometry::Line2D>(entity));
    } else if (std::holds_alternative<Geometry::Arc2D>(entity)) {
        renderArc(painter, std::get<Geometry::Arc2D>(entity));
    }
}

void CADCanvas::renderLine(QPainter& painter, const Geometry::Line2D& line) {
    QPointF p1 = viewport_.worldToScreen(line.start());
    QPointF p2 = viewport_.worldToScreen(line.end());

    painter.setPen(QPen(QColor(0, 0, 0), 2));
    painter.drawLine(p1, p2);
}

void CADCanvas::renderArc(QPainter& painter, const Geometry::Arc2D& arc) {
    QPointF center = viewport_.worldToScreen(arc.center());
    double radiusScreen = arc.radius() * viewport_.zoomLevel();

    // Qt uses 1/16th degree units
    double startAngleDeg = arc.startAngle() * 180.0 / Geometry::PI;
    double sweepAngleDeg = arc.sweepAngle() * 180.0 / Geometry::PI;

    // Qt angles: 0° is 3 o'clock, positive is counter-clockwise
    // Our angles: 0° is 3 o'clock, positive is counter-clockwise
    // So we need to adjust for Qt's coordinate system (Y grows down)
    startAngleDeg = -startAngleDeg;  // Flip for screen space

    if (!arc.isCounterClockwise()) {
        sweepAngleDeg = -sweepAngleDeg;
    } else {
        sweepAngleDeg = -sweepAngleDeg;  // Already flipped by Y inversion
    }

    int startAngle16 = static_cast<int>(startAngleDeg * 16);
    int spanAngle16 = static_cast<int>(sweepAngleDeg * 16);

    QRectF rect(center.x() - radiusScreen, center.y() - radiusScreen,
                2 * radiusScreen, 2 * radiusScreen);

    painter.setPen(QPen(QColor(0, 0, 0), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(rect, startAngle16, spanAngle16);
}

// ============================================================================
// MOUSE INTERACTION
// ============================================================================

void CADCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        isPanning_ = true;
        lastMousePos_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void CADCanvas::mouseMoveEvent(QMouseEvent* event) {
    // Update world position
    lastWorldPos_ = viewport_.screenToWorld(event->pos());
    emit cursorPositionChanged(lastWorldPos_.x(), lastWorldPos_.y());

    if (isPanning_) {
        QPoint delta = event->pos() - lastMousePos_;
        viewport_.pan(delta.x(), delta.y());
        lastMousePos_ = event->pos();
        update();
        emit viewportChanged(viewport_.zoomLevel(), viewport_.panX(), viewport_.panY());
    }
}

void CADCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        isPanning_ = false;
        setCursor(Qt::ArrowCursor);
    }
}

void CADCanvas::wheelEvent(QWheelEvent* event) {
    // Zoom in/out with mouse wheel
    double zoomFactor = 1.0;

    if (event->angleDelta().y() > 0) {
        zoomFactor = 1.15;  // Zoom in
    } else {
        zoomFactor = 1.0 / 1.15;  // Zoom out
    }

    viewport_.zoom(zoomFactor, event->position());
    update();
    emit viewportChanged(viewport_.zoomLevel(), viewport_.panX(), viewport_.panY());
}

void CADCanvas::resizeEvent(QResizeEvent* event) {
    viewport_.setViewportSize(width(), height());
    QWidget::resizeEvent(event);
}

} // namespace UI
} // namespace OwnCAD
