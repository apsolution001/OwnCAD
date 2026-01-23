#include "ui/CADCanvas.h"
#include "ui/LineTool.h"
#include "ui/ArcTool.h"
#include "ui/RectangleTool.h"
#include "ui/MoveTool.h"
#include "ui/RotateTool.h"
#include "ui/MirrorTool.h"
#include "geometry/GeometryConstants.h"
#include "geometry/BoundingBox.h"
#include "geometry/GeometryMath.h"
#include "import/DXFColors.h"
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QToolTip>
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
    , snapTolerancePixels_(10.0)  // 10 screen pixels (zoom-independent)
    , lastSnapPoint_(std::nullopt)
    , lastSnapType_(SnapType::None) {
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
    const std::vector<Import::GeometryEntityWithMetadata>& entities,
    double zoomLevel
) {
    // Convert screen-space tolerance (pixels) to world-space tolerance
    // At high zoom (e.g., 100x): 10 pixels = 0.1 world units (tight)
    // At low zoom (e.g., 0.01x): 10 pixels = 1000 world units (loose)
    const double worldTolerance = snapTolerancePixels_ / zoomLevel;

    // Priority: Endpoint > Midpoint > Nearest > Grid
    // Always update tracking for visual feedback

    if (isSnapEnabled(SnapMode::Endpoint)) {
        auto endpointSnap = snapToEndpoint(point, entities, worldTolerance);
        if (endpointSnap) {
            lastSnapPoint_ = endpointSnap;
            lastSnapType_ = SnapType::Endpoint;
            return endpointSnap;
        }
    }

    if (isSnapEnabled(SnapMode::Midpoint)) {
        auto midpointSnap = snapToMidpoint(point, entities, worldTolerance);
        if (midpointSnap) {
            lastSnapPoint_ = midpointSnap;
            lastSnapType_ = SnapType::Midpoint;
            return midpointSnap;
        }
    }

    if (isSnapEnabled(SnapMode::Nearest)) {
        auto nearestSnap = snapToNearest(point, entities, worldTolerance);
        if (nearestSnap) {
            lastSnapPoint_ = nearestSnap;
            lastSnapType_ = SnapType::Nearest;
            return nearestSnap;
        }
    }

    if (isSnapEnabled(SnapMode::Grid)) {
        auto gridSnap = snapToGrid(point);
        lastSnapPoint_ = gridSnap;
        lastSnapType_ = SnapType::Grid;
        return gridSnap;
    }

    // No snap
    lastSnapPoint_ = std::nullopt;
    lastSnapType_ = SnapType::None;
    return point;
}

Geometry::Point2D SnapManager::snapToGrid(const Geometry::Point2D& point) const {
    double snappedX = std::round(point.x() / gridSpacing_) * gridSpacing_;
    double snappedY = std::round(point.y() / gridSpacing_) * gridSpacing_;
    return Geometry::Point2D(snappedX, snappedY);
}

std::optional<Geometry::Point2D> SnapManager::snapToEndpoint(
    const Geometry::Point2D& point,
    const std::vector<Import::GeometryEntityWithMetadata>& entities,
    double worldTolerance
) const {
    std::optional<Geometry::Point2D> closestPoint;
    double closestDistSq = worldTolerance * worldTolerance;

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

std::optional<Geometry::Point2D> SnapManager::snapToMidpoint(
    const Geometry::Point2D& point,
    const std::vector<Import::GeometryEntityWithMetadata>& entities,
    double worldTolerance
) const {
    std::optional<Geometry::Point2D> closestPoint;
    double closestDistSq = worldTolerance * worldTolerance;

    for (const auto& entityWithMeta : entities) {
        const auto& entity = entityWithMeta.entity;

        std::optional<Geometry::Point2D> midpoint;

        if (std::holds_alternative<Geometry::Line2D>(entity)) {
            const auto& line = std::get<Geometry::Line2D>(entity);
            // Midpoint of line: start + (end - start) * 0.5
            midpoint = line.pointAt(0.5);
        } else if (std::holds_alternative<Geometry::Arc2D>(entity)) {
            const auto& arc = std::get<Geometry::Arc2D>(entity);
            // Midpoint of arc: point at 50% of sweep angle
            midpoint = arc.pointAt(0.5);
        }

        if (midpoint) {
            double distSq = point.distanceSquaredTo(*midpoint);
            if (distSq < closestDistSq) {
                closestDistSq = distSq;
                closestPoint = midpoint;
            }
        }
    }

    return closestPoint;
}

std::optional<Geometry::Point2D> SnapManager::snapToNearest(
    const Geometry::Point2D& point,
    const std::vector<Import::GeometryEntityWithMetadata>& entities,
    double worldTolerance
) const {
    std::optional<Geometry::Point2D> closestPoint;
    double closestDistSq = worldTolerance * worldTolerance;

    for (const auto& entityWithMeta : entities) {
        const auto& entity = entityWithMeta.entity;

        std::optional<Geometry::Point2D> nearestOnEntity;

        if (std::holds_alternative<Geometry::Line2D>(entity)) {
            const auto& line = std::get<Geometry::Line2D>(entity);
            // Use GeometryMath to find closest point on line segment
            nearestOnEntity = Geometry::GeometryMath::closestPointOnSegment(point, line);
        } else if (std::holds_alternative<Geometry::Arc2D>(entity)) {
            const auto& arc = std::get<Geometry::Arc2D>(entity);
            // Use GeometryMath to find closest point on arc
            nearestOnEntity = Geometry::GeometryMath::closestPointOnArc(point, arc);
        }

        if (nearestOnEntity) {
            double distSq = point.distanceSquaredTo(*nearestOnEntity);
            if (distSq < closestDistSq) {
                closestDistSq = distSq;
                closestPoint = nearestOnEntity;
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
    , toolManager_(std::make_unique<ToolManager>(this))
    , gridSettings_()
    , isPanning_(false)
    , lastMousePos_()
    , lastWorldPos_(0, 0)
    , selectionManager_()
    , boxSelectMode_(BoxSelectMode::None)
    , boxSelectStartScreen_()
    , boxSelectCurrentScreen_() {

    setMouseTracking(true);  // Enable mouse move events without button press
    setFocusPolicy(Qt::StrongFocus);

    // Set background color
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(250, 250, 250));
    setPalette(pal);

    // Initialize default grid settings
    gridSettings_.visible = true;
    gridSettings_.spacing = 10.0;
    gridSettings_.units = GridUnits::Millimeters;

    // Register drawing tools
    toolManager_->registerTool(std::make_unique<LineTool>());
    toolManager_->registerTool(std::make_unique<ArcTool>());
    toolManager_->registerTool(std::make_unique<RectangleTool>());
    toolManager_->registerTool(std::make_unique<MoveTool>());
    toolManager_->registerTool(std::make_unique<RotateTool>());
    toolManager_->registerTool(std::make_unique<MirrorTool>());

    // Connect tool manager signals
    connect(toolManager_.get(), &ToolManager::geometryChanged, this, [this]() {
        // Refresh entities from document model would go here
        // For now, just repaint
        update();
    });
}

CADCanvas::~CADCanvas() {
}

void CADCanvas::setDocumentModel(Model::DocumentModel* model) {
    toolManager_->setDocumentModel(model);
}

std::vector<std::string> CADCanvas::selectedHandles() const {
    return selectionManager_.selectedHandles();
}

void CADCanvas::setEntities(const std::vector<Import::GeometryEntityWithMetadata>& entities) {
    entities_ = entities;

    // Count entity types for logging
    int lineCount = 0;
    int arcCount = 0;
    int ellipseCount = 0;
    int pointCount = 0;
    for (const auto& e : entities_) {
        if (std::holds_alternative<Geometry::Line2D>(e.entity)) {
            lineCount++;
        } else if (std::holds_alternative<Geometry::Arc2D>(e.entity)) {
            arcCount++;
        } else if (std::holds_alternative<Geometry::Ellipse2D>(e.entity)) {
            ellipseCount++;
        } else if (std::holds_alternative<Geometry::Point2D>(e.entity)) {
            pointCount++;
        }
    }

    qDebug() << "CADCanvas: Loaded" << entities_.size() << "entities ("
             << lineCount << "lines," << arcCount << "arcs,"
             << ellipseCount << "ellipses," << pointCount << "points)";

    update();  // Trigger repaint
}

void CADCanvas::clear() {
    entities_.clear();
    update();
}

void CADCanvas::setGridSettings(const GridSettings& settings) {
    gridSettings_ = settings;
    snapManager_.setGridSpacing(settings.spacing);
    update();
}

void CADCanvas::setGridVisible(bool visible) {
    gridSettings_.visible = visible;
    update();
}

void CADCanvas::setGridSpacing(double spacing) {
    gridSettings_.spacing = spacing;
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
        } else if (std::holds_alternative<Geometry::Ellipse2D>(entity)) {
            bbox = std::get<Geometry::Ellipse2D>(entity).boundingBox();
        } else if (std::holds_alternative<Geometry::Point2D>(entity)) {
            // Point has no area, create tiny bbox around it
            const auto& pt = std::get<Geometry::Point2D>(entity);
            bbox = Geometry::BoundingBox::fromPoints(pt, pt);
        } else {
            continue;  // Skip unknown types
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
    if (gridSettings_.visible) {
        renderGrid(painter);
    }

    // Render origin axes
    renderOrigin(painter);

    // Render all entities
    renderEntities(painter);

    // Render selection visuals (bounding box and grip points)
    renderSelectionBoundingBox(painter);
    renderGripPoints(painter);

    // Render box selection rectangle (if in box select mode)
    if (boxSelectMode_ != BoxSelectMode::None) {
        renderSelectionBox(painter);
    }

    // Render tool preview (before snap indicator)
    if (toolManager_->hasActiveTool()) {
        toolManager_->render(painter, viewport_);
    }

    // Render snap indicator (foreground, always on top)
    renderSnapIndicator(painter);
}

void CADCanvas::renderGrid(QPainter& painter) {
    // Calculate grid spacing in screen space
    double gridScreenSpacing = gridSettings_.spacing * viewport_.zoomLevel();

    // Adaptive grid: hide if too dense or too sparse
    if (gridScreenSpacing < 5.0 || gridScreenSpacing > 200.0) {
        return;  // Grid too dense or too sparse
    }

    // Get visible world bounds
    Geometry::Point2D topLeft = viewport_.screenToWorld(QPointF(0, 0));
    Geometry::Point2D bottomRight = viewport_.screenToWorld(QPointF(width(), height()));

    double minX = std::floor(topLeft.x() / gridSettings_.spacing) * gridSettings_.spacing;
    double maxX = std::ceil(bottomRight.x() / gridSettings_.spacing) * gridSettings_.spacing;
    double minY = std::floor(bottomRight.y() / gridSettings_.spacing) * gridSettings_.spacing;
    double maxY = std::ceil(topLeft.y() / gridSettings_.spacing) * gridSettings_.spacing;

    // Grid colors (Phase 1 spec: Major #999, Minor #ddd)
    const QColor majorGridColor(153, 153, 153);  // #999999 (darker gray)
    const QColor minorGridColor(221, 221, 221);  // #dddddd (lighter gray)

    // Draw vertical grid lines
    for (double x = minX; x <= maxX; x += gridSettings_.spacing) {
        QPointF p1 = viewport_.worldToScreen(Geometry::Point2D(x, minY));
        QPointF p2 = viewport_.worldToScreen(Geometry::Point2D(x, maxY));

        // Major grid lines every 10 units
        if (std::abs(std::fmod(x, gridSettings_.spacing * 10.0)) < Geometry::GEOMETRY_EPSILON) {
            painter.setPen(QPen(majorGridColor, 1));
        } else {
            painter.setPen(QPen(minorGridColor, 1));
        }

        painter.drawLine(p1, p2);
    }

    // Draw horizontal grid lines
    for (double y = minY; y <= maxY; y += gridSettings_.spacing) {
        QPointF p1 = viewport_.worldToScreen(Geometry::Point2D(minX, y));
        QPointF p2 = viewport_.worldToScreen(Geometry::Point2D(maxX, y));

        // Major grid lines every 10 units
        if (std::abs(std::fmod(y, gridSettings_.spacing * 10.0)) < Geometry::GEOMETRY_EPSILON) {
            painter.setPen(QPen(majorGridColor, 1));
        } else {
            painter.setPen(QPen(minorGridColor, 1));
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
    for (const auto& entityWithMeta : entities_) {
        renderEntity(painter, entityWithMeta);
    }

    // Note: Removed verbose logging - it was printing on every repaint
    // Use qDebug() in setEntities() instead for import logging
}

void CADCanvas::renderEntity(QPainter& painter, const Import::GeometryEntityWithMetadata& entityWithMeta) {
    const auto& entity = entityWithMeta.entity;

    if (std::holds_alternative<Geometry::Line2D>(entity)) {
        renderLine(painter, std::get<Geometry::Line2D>(entity), entityWithMeta);
    } else if (std::holds_alternative<Geometry::Arc2D>(entity)) {
        renderArc(painter, std::get<Geometry::Arc2D>(entity), entityWithMeta);
    } else if (std::holds_alternative<Geometry::Ellipse2D>(entity)) {
        renderEllipse(painter, std::get<Geometry::Ellipse2D>(entity), entityWithMeta);
    } else if (std::holds_alternative<Geometry::Point2D>(entity)) {
        renderPoint(painter, std::get<Geometry::Point2D>(entity), entityWithMeta);
    }
}

void CADCanvas::renderLine(QPainter& painter, const Geometry::Line2D& line, const Import::GeometryEntityWithMetadata& metadata) {
    QPointF p1 = viewport_.worldToScreen(line.start());
    QPointF p2 = viewport_.worldToScreen(line.end());

    // Get color from DXF
    QColor color;
    int width = 2;

    if (selectionManager_.isSelected(metadata.handle)) {
        color = QColor(0, 102, 255); // Blue #0066FF
        width = 3;
    } else {
        color = Import::DXFColors::toQColor(metadata.colorNumber, QColor(0, 0, 0));
    }

    painter.setPen(QPen(color, width));
    painter.drawLine(p1, p2);
}

void CADCanvas::renderArc(QPainter& painter, const Geometry::Arc2D& arc, const Import::GeometryEntityWithMetadata& metadata) {
    // Get color from DXF
    QColor color;
    int width = 2;

    if (selectionManager_.isSelected(metadata.handle)) {
        color = QColor(0, 102, 255); // Blue #0066FF
        width = 3;
    } else {
        color = Import::DXFColors::toQColor(metadata.colorNumber, QColor(0, 0, 0));
    }

    // SIMPLIFIED ROBUST APPROACH: Sample points along arc and draw path
    // This avoids all the complex angle transformation math that's been causing issues
    const int numSamples = std::max(12, static_cast<int>(arc.sweepAngle() * 180.0 / Geometry::PI / 15.0));  // ~15° per segment

    QPainterPath path;
    bool firstPoint = true;

    for (int i = 0; i <= numSamples; ++i) {
        double t = static_cast<double>(i) / numSamples;
        Geometry::Point2D worldPt = arc.pointAt(t);
        QPointF screenPt = viewport_.worldToScreen(worldPt);

        if (firstPoint) {
            path.moveTo(screenPt);
            firstPoint = false;
        } else {
            path.lineTo(screenPt);
        }
    }

    painter.setPen(QPen(color, width));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

void CADCanvas::renderEllipse(QPainter& painter, const Geometry::Ellipse2D& ellipse, const Import::GeometryEntityWithMetadata& metadata) {
    // Get color from DXF
    QColor color;
    int width = 2;

    if (selectionManager_.isSelected(metadata.handle)) {
        color = QColor(0, 102, 255); // Blue #0066FF
        width = 3;
    } else {
        color = Import::DXFColors::toQColor(metadata.colorNumber, QColor(0, 0, 0));
    }

    // Sample points along ellipse and draw path
    const int numSamples = std::max(24, static_cast<int>(ellipse.sweepAngle() * 180.0 / Geometry::PI / 10.0));  // ~10° per segment

    QPainterPath path;
    bool firstPoint = true;

    for (int i = 0; i <= numSamples; ++i) {
        double t = static_cast<double>(i) / numSamples;
        Geometry::Point2D worldPt = ellipse.pointAt(t);
        QPointF screenPt = viewport_.worldToScreen(worldPt);

        if (firstPoint) {
            path.moveTo(screenPt);
            firstPoint = false;
        } else {
            path.lineTo(screenPt);
        }
    }

    painter.setPen(QPen(color, width));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

void CADCanvas::renderPoint(QPainter& painter, const Geometry::Point2D& point, const Import::GeometryEntityWithMetadata& metadata) {
    QPointF screenPt = viewport_.worldToScreen(point);

    // Get color from DXF
    QColor color;
    int size = 4;  // Point marker size in pixels

    if (selectionManager_.isSelected(metadata.handle)) {
        color = QColor(0, 102, 255); // Blue #0066FF
        size = 6;
    } else {
        color = Import::DXFColors::toQColor(metadata.colorNumber, QColor(0, 0, 0));
    }

    // Draw point as a small filled circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(color));
    painter.drawEllipse(screenPt, size, size);

    // Draw cross marker
    painter.setPen(QPen(color, 1));
    painter.drawLine(screenPt - QPointF(size * 2, 0), screenPt + QPointF(size * 2, 0));
    painter.drawLine(screenPt - QPointF(0, size * 2), screenPt + QPointF(0, size * 2));
}

void CADCanvas::renderSnapIndicator(QPainter& painter) {
    // Only render if there's an active snap point
    auto snapPoint = snapManager_.lastSnapPoint();
    if (!snapPoint) {
        return;
    }

    // Convert world snap point to screen coordinates
    QPointF screenPos = viewport_.worldToScreen(*snapPoint);

    // Get snap type to determine color
    auto snapType = snapManager_.lastSnapType();

    QColor markerColor;
    int markerSize = 8;  // Size in pixels (screen space)

    switch (snapType) {
        case SnapManager::SnapType::Grid:
            markerColor = QColor(200, 200, 200);  // Light gray
            break;
        case SnapManager::SnapType::Endpoint:
            markerColor = QColor(0, 102, 255);  // Blue
            markerSize = 10;  // Slightly larger for endpoints
            break;
        case SnapManager::SnapType::Midpoint:
            markerColor = QColor(0, 204, 0);  // Green
            break;
        case SnapManager::SnapType::Nearest:
            markerColor = QColor(255, 136, 0);  // Orange
            break;
        default:
            return;  // No marker for SnapType::None
    }

    // Draw snap marker as a cross with a circle
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(markerColor, 2));

    // Draw cross
    double halfSize = markerSize / 2.0;
    painter.drawLine(QPointF(screenPos.x() - halfSize, screenPos.y()),
                     QPointF(screenPos.x() + halfSize, screenPos.y()));
    painter.drawLine(QPointF(screenPos.x(), screenPos.y() - halfSize),
                     QPointF(screenPos.x(), screenPos.y() + halfSize));

    // Draw circle around the cross
    painter.drawEllipse(screenPos, halfSize + 2, halfSize + 2);
}

void CADCanvas::renderSelectionBoundingBox(QPainter& painter) {
    // Only render if there are selected entities
    if (selectionManager_.isEmpty()) {
        return;
    }

    // Calculate merged bounding box of all selected entities
    std::optional<Geometry::BoundingBox> selectionBBox;

    for (const auto& entityWithMeta : entities_) {
        if (!selectionManager_.isSelected(entityWithMeta.handle)) {
            continue;
        }

        const auto& entity = entityWithMeta.entity;
        Geometry::BoundingBox entityBox;

        if (std::holds_alternative<Geometry::Line2D>(entity)) {
            entityBox = std::get<Geometry::Line2D>(entity).boundingBox();
        } else if (std::holds_alternative<Geometry::Arc2D>(entity)) {
            entityBox = std::get<Geometry::Arc2D>(entity).boundingBox();
        } else if (std::holds_alternative<Geometry::Ellipse2D>(entity)) {
            entityBox = std::get<Geometry::Ellipse2D>(entity).boundingBox();
        } else if (std::holds_alternative<Geometry::Point2D>(entity)) {
            const auto& pt = std::get<Geometry::Point2D>(entity);
            entityBox = Geometry::BoundingBox::fromPoints(pt, pt);
        } else {
            continue;
        }

        if (selectionBBox) {
            selectionBBox = selectionBBox->merge(entityBox);
        } else {
            selectionBBox = entityBox;
        }
    }

    if (!selectionBBox || !selectionBBox->isValid()) {
        return;
    }

    // Convert bounding box corners to screen coordinates
    QPointF topLeft = viewport_.worldToScreen(
        Geometry::Point2D(selectionBBox->minX(), selectionBBox->maxY()));
    QPointF bottomRight = viewport_.worldToScreen(
        Geometry::Point2D(selectionBBox->maxX(), selectionBBox->minY()));

    QRectF rect(topLeft, bottomRight);
    rect = rect.normalized();

    // Draw dashed blue selection bounding box
    const QColor selectionColor(0, 102, 255);  // #0066FF
    QPen pen(selectionColor, 1, Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect);
}

void CADCanvas::renderGripPoints(QPainter& painter) {
    // Only render if there are selected entities
    if (selectionManager_.isEmpty()) {
        return;
    }

    // Calculate merged bounding box of all selected entities
    std::optional<Geometry::BoundingBox> selectionBBox;

    for (const auto& entityWithMeta : entities_) {
        if (!selectionManager_.isSelected(entityWithMeta.handle)) {
            continue;
        }

        const auto& entity = entityWithMeta.entity;
        Geometry::BoundingBox entityBox;

        if (std::holds_alternative<Geometry::Line2D>(entity)) {
            entityBox = std::get<Geometry::Line2D>(entity).boundingBox();
        } else if (std::holds_alternative<Geometry::Arc2D>(entity)) {
            entityBox = std::get<Geometry::Arc2D>(entity).boundingBox();
        } else if (std::holds_alternative<Geometry::Ellipse2D>(entity)) {
            entityBox = std::get<Geometry::Ellipse2D>(entity).boundingBox();
        } else if (std::holds_alternative<Geometry::Point2D>(entity)) {
            const auto& pt = std::get<Geometry::Point2D>(entity);
            entityBox = Geometry::BoundingBox::fromPoints(pt, pt);
        } else {
            continue;
        }

        if (selectionBBox) {
            selectionBBox = selectionBBox->merge(entityBox);
        } else {
            selectionBBox = entityBox;
        }
    }

    if (!selectionBBox || !selectionBBox->isValid()) {
        return;
    }

    // Get the 4 corners in world coordinates
    Geometry::Point2D corners[4] = {
        Geometry::Point2D(selectionBBox->minX(), selectionBBox->maxY()),  // Top-left
        Geometry::Point2D(selectionBBox->maxX(), selectionBBox->maxY()),  // Top-right
        Geometry::Point2D(selectionBBox->maxX(), selectionBBox->minY()),  // Bottom-right
        Geometry::Point2D(selectionBBox->minX(), selectionBBox->minY())   // Bottom-left
    };

    // Grip point style: 6x6 pixel filled blue squares
    const int gripSize = 6;
    const int halfGrip = gripSize / 2;
    const QColor gripColor(0, 102, 255);  // #0066FF

    painter.setPen(QPen(gripColor, 1));
    painter.setBrush(QBrush(gripColor));

    for (const auto& corner : corners) {
        QPointF screenPos = viewport_.worldToScreen(corner);
        QRectF gripRect(screenPos.x() - halfGrip, screenPos.y() - halfGrip,
                        gripSize, gripSize);
        painter.drawRect(gripRect);
    }
}

// ============================================================================
// MOUSE INTERACTION
// ============================================================================

void CADCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        isPanning_ = true;
        lastMousePos_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
    } else if (event->button() == Qt::LeftButton) {
        // If a tool is active, forward to tool manager first
        if (toolManager_->hasActiveTool()) {
            Geometry::Point2D worldPos = viewport_.screenToWorld(event->pos());
            // Apply snap to get precise point
            auto snappedPoint = snapManager_.snap(worldPos, entities_, viewport_.zoomLevel());
            Geometry::Point2D inputPoint = snappedPoint.value_or(worldPos);

            ToolResult result = toolManager_->handleMousePress(inputPoint, event);
            if (result != ToolResult::Ignored) {
                update();
                return;  // Tool handled the event
            }
        }

        // Handle selection (no tool active or tool ignored event)
        Geometry::Point2D worldPos = viewport_.screenToWorld(event->pos());
        std::string hitHandle = hitTest(worldPos);

        // Check for modifier keys
        bool shiftHeld = event->modifiers() & Qt::ShiftModifier;
        bool ctrlHeld = event->modifiers() & Qt::ControlModifier;

        if (!hitHandle.empty()) {
            // Clicked on an entity - handle single selection
            if (shiftHeld) {
                // Shift + click: Toggle selection (add/remove without clearing)
                selectionManager_.toggle(hitHandle);
            } else if (ctrlHeld) {
                // Ctrl + click: Add to selection (without clearing)
                selectionManager_.select(hitHandle);
            } else {
                // No modifier: Clear current selection, then select
                selectionManager_.clear();
                selectionManager_.select(hitHandle);
            }
            emit selectionChanged(selectionManager_.selectedCount());
        } else {
            // Clicked on empty space
            // Check if we are already in box select mode (second click of Click-Move-Click)
            if (boxSelectMode_ != BoxSelectMode::None) {
                completeBoxSelection();
                return;
            }

            // Otherwise, start box selection
            if (!shiftHeld && !ctrlHeld) {
                selectionManager_.clear();
                emit selectionChanged(selectionManager_.selectedCount());
            }
            
            // Start as None, will decide mode on first move
            boxSelectMode_ = BoxSelectMode::None; 
            boxSelectStartScreen_ = event->pos();
            boxSelectCurrentScreen_ = event->pos();
             
             // For now, let's strictly follow the plan:
             // Start tracking, but mode is undetermined yet (or default Inside)
             // Let's set it to Inside initially to match "start" logic, 
             // but `mouseMoveEvent` will immediately correct it.
             // Actually, better to use a specific state or just check start pos vs current pos.
             boxSelectMode_ = BoxSelectMode::Inside; // Default start
             
            setCursor(Qt::CrossCursor);
        }

        update(); // Trigger repaint
    } else if (event->button() == Qt::RightButton) {
        // Right click handling - reserved for future Context Menu
        // Previously used for Crossing mode, now removed.
    }
}

void CADCanvas::mouseMoveEvent(QMouseEvent* event) {
    // Update world position
    lastWorldPos_ = viewport_.screenToWorld(event->pos());

    // Calculate snap point (updates snapManager_ internal state for visual feedback)
    // Pass current zoom level for screen-space snap tolerance conversion
    auto snappedPoint = snapManager_.snap(lastWorldPos_, entities_, viewport_.zoomLevel());

    // Forward to tool manager if tool is active
    if (toolManager_->hasActiveTool()) {
        Geometry::Point2D inputPoint = snappedPoint.value_or(lastWorldPos_);
        toolManager_->handleMouseMove(inputPoint, event);
    }

    if (snappedPoint) {
        emit cursorPositionChanged(snappedPoint->x(), snappedPoint->y());

        // Show tooltip with snap information (only when not box selecting)
        if (boxSelectMode_ == BoxSelectMode::None) {
            auto snapType = snapManager_.lastSnapType();
            if (snapType != SnapManager::SnapType::None) {
                QString snapTypeName;
                switch (snapType) {
                    case SnapManager::SnapType::Grid:
                        snapTypeName = "Grid";
                        break;
                    case SnapManager::SnapType::Endpoint:
                        snapTypeName = "Endpoint";
                        break;
                    case SnapManager::SnapType::Midpoint:
                        snapTypeName = "Midpoint";
                        break;
                    case SnapManager::SnapType::Nearest:
                        snapTypeName = "Nearest";
                        break;
                    default:
                        snapTypeName = "Unknown";
                        break;
                }

                QString tooltipText = QString("%1 Snap\nX: %2\nY: %3")
                    .arg(snapTypeName)
                    .arg(snappedPoint->x(), 0, 'f', 3)
                    .arg(snappedPoint->y(), 0, 'f', 3);

                QToolTip::showText(event->globalPosition().toPoint(), tooltipText, this);
            }
        }
    } else {
        emit cursorPositionChanged(lastWorldPos_.x(), lastWorldPos_.y());
        QToolTip::hideText();
    }

    if (isPanning_) {
        QPoint delta = event->pos() - lastMousePos_;
        viewport_.pan(delta.x(), delta.y());
        lastMousePos_ = event->pos();
        update();
        emit viewportChanged(viewport_.zoomLevel(), viewport_.panX(), viewport_.panY());
    } else if (boxSelectMode_ != BoxSelectMode::None) {
        // Update box selection rectangle
        boxSelectCurrentScreen_ = event->pos();

        // Dynamic Mode Switching based on Drag Direction (AutoCAD style)
        // Right drag (Current X > Start X) -> Inside Mode
        // Left drag (Current X < Start X) -> Crossing Mode
        if (boxSelectCurrentScreen_.x() >= boxSelectStartScreen_.x()) {
            boxSelectMode_ = BoxSelectMode::Inside;
        } else {
            boxSelectMode_ = BoxSelectMode::Crossing;
        }

        update();
    } else {
        // Trigger repaint to show/update snap indicator
        update();
    }
}

void CADCanvas::mouseReleaseEvent(QMouseEvent* event) {
    // Forward to tool manager first if tool is active
    if (toolManager_->hasActiveTool()) {
        Geometry::Point2D worldPos = viewport_.screenToWorld(event->pos());
        // Apply snap to get precise point (same as press/move)
        auto snappedPoint = snapManager_.snap(worldPos, entities_, viewport_.zoomLevel());
        Geometry::Point2D inputPoint = snappedPoint.value_or(worldPos);

        ToolResult result = toolManager_->handleMouseRelease(inputPoint, event);
        if (result != ToolResult::Ignored) {
            update();
            return;  // Tool handled the event
        }
    }

    if (event->button() == Qt::MiddleButton) {
        isPanning_ = false;
        setCursor(Qt::ArrowCursor);
    } else if (event->button() == Qt::LeftButton && boxSelectMode_ != BoxSelectMode::None) {
        // Check for Drag vs Click
        double screenDist = (event->pos() - boxSelectStartScreen_).manhattanLength();

        if (screenDist >= 5.0) {
            // Drag > 5px: Standard Drag-and-Release selection
            completeBoxSelection();
        } 
        // Else: Click < 5px: Do nothing (keep selection mode active for Click-Move-Click)


        // If it was a click (distance < 5), we assume user is starting Click-Move-Click
        // So we just return and let them move the mouse. Do NOT reset boxSelectMode_.
    }
}

void CADCanvas::completeBoxSelection() {
    // Calculate selection box in world coordinates
    Geometry::Point2D worldStart = viewport_.screenToWorld(boxSelectStartScreen_);
    Geometry::Point2D worldEnd = viewport_.screenToWorld(boxSelectCurrentScreen_);
    Geometry::BoundingBox selectionBox = Geometry::BoundingBox::fromPoints(worldStart, worldEnd);

    // Only perform selection if box has meaningful size (> 3 pixels in both directions)
    double screenWidth = std::abs(boxSelectCurrentScreen_.x() - boxSelectStartScreen_.x());
    double screenHeight = std::abs(boxSelectCurrentScreen_.y() - boxSelectStartScreen_.y());

    if (screenWidth > 3 && screenHeight > 3) {
        // Get entities in the selection box
        std::vector<std::string> selectedEntities = getEntitiesInBox(selectionBox, boxSelectMode_);

        // Check for modifier keys
        // Note: modifiers might not be accurate if called from non-event context, but usually OK
        Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
        bool shiftHeld = modifiers & Qt::ShiftModifier;
        bool ctrlHeld = modifiers & Qt::ControlModifier;

        // Apply selection based on modifiers
        for (const auto& handle : selectedEntities) {
            if (shiftHeld) {
                selectionManager_.toggle(handle);
            } else if (ctrlHeld) {
                selectionManager_.select(handle);
            } else {
                selectionManager_.select(handle);
            }
        }

        emit selectionChanged(selectionManager_.selectedCount());
    }

    // Reset box selection state
    boxSelectMode_ = BoxSelectMode::None;
    setCursor(Qt::ArrowCursor);
    update();


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

void CADCanvas::keyPressEvent(QKeyEvent* event) {
    // Forward to tool manager first if tool is active
    if (toolManager_->hasActiveTool()) {
        ToolResult result = toolManager_->handleKeyPress(event);
        if (result != ToolResult::Ignored) {
            update();
            // If tool was cancelled, update cursor
            if (result == ToolResult::Cancelled && !toolManager_->hasActiveTool()) {
                setCursor(Qt::ArrowCursor);
            }
            return;
        }
    }

    // Handle other keyboard shortcuts here (tool activation handled in MainWindow)
    QWidget::keyPressEvent(event);
}

std::string CADCanvas::hitTest(const Geometry::Point2D& point) {
    // Tolerances:
    // Screen tolerance: 10 pixels
    // World tolerance: 10 pixels / zoomLevel
    double worldTolerance = 10.0 / viewport_.zoomLevel();

    std::string closestHandle = "";
    double closestDist = worldTolerance; // Initialize with max acceptable distance

    for (const auto& entityWithMeta : entities_) {
        const auto& entity = entityWithMeta.entity;
        double dist = std::numeric_limits<double>::max();

        if (std::holds_alternative<Geometry::Line2D>(entity)) {
            dist = Geometry::GeometryMath::distancePointToSegment(point, std::get<Geometry::Line2D>(entity));
        } else if (std::holds_alternative<Geometry::Arc2D>(entity)) {
            dist = Geometry::GeometryMath::distancePointToArc(point, std::get<Geometry::Arc2D>(entity));
        } else if (std::holds_alternative<Geometry::Ellipse2D>(entity)) {
            dist = Geometry::GeometryMath::distancePointToEllipse(point, std::get<Geometry::Ellipse2D>(entity));
        } else if (std::holds_alternative<Geometry::Point2D>(entity)) {
            // Direct distance to point entity
            dist = point.distanceTo(std::get<Geometry::Point2D>(entity));
        }

        if (dist < closestDist) {
            closestDist = dist;
            closestHandle = entityWithMeta.handle;
        }
    }

    return closestHandle;
}

void CADCanvas::renderSelectionBox(QPainter& painter) {
    // Calculate rectangle from start to current position
    QRectF rect(boxSelectStartScreen_, boxSelectCurrentScreen_);
    rect = rect.normalized();  // Ensure positive width/height

    // Set style based on mode
    QColor fillColor;
    QColor borderColor;
    Qt::PenStyle penStyle;

    if (boxSelectMode_ == BoxSelectMode::Inside) {
        // Inside mode: solid blue rectangle (left-to-right drag convention)
        fillColor = QColor(0, 102, 255, 40);    // Semi-transparent blue
        borderColor = QColor(0, 102, 255);      // Blue border
        penStyle = Qt::SolidLine;
    } else {
        // Crossing mode: dashed green rectangle (right-to-left drag convention)
        fillColor = QColor(0, 200, 0, 40);      // Semi-transparent green
        borderColor = QColor(0, 200, 0);        // Green border
        penStyle = Qt::DashLine;
    }

    // Draw filled rectangle
    painter.setBrush(QBrush(fillColor));
    painter.setPen(QPen(borderColor, 1, penStyle));
    painter.drawRect(rect);
}

std::vector<std::string> CADCanvas::getEntitiesInBox(
    const Geometry::BoundingBox& selectionBox,
    BoxSelectMode mode
) {
    std::vector<std::string> result;

    for (const auto& entityWithMeta : entities_) {
        const auto& entity = entityWithMeta.entity;
        Geometry::BoundingBox entityBox;

        // Get bounding box for each entity type
        if (std::holds_alternative<Geometry::Line2D>(entity)) {
            entityBox = std::get<Geometry::Line2D>(entity).boundingBox();
        } else if (std::holds_alternative<Geometry::Arc2D>(entity)) {
            entityBox = std::get<Geometry::Arc2D>(entity).boundingBox();
        } else if (std::holds_alternative<Geometry::Ellipse2D>(entity)) {
            entityBox = std::get<Geometry::Ellipse2D>(entity).boundingBox();
        } else if (std::holds_alternative<Geometry::Point2D>(entity)) {
            // Point is a single point - create tiny bbox
            const auto& pt = std::get<Geometry::Point2D>(entity);
            entityBox = Geometry::BoundingBox::fromPoints(pt, pt);
        } else {
            continue;  // Unknown entity type
        }

        bool selected = false;

        if (mode == BoxSelectMode::Inside) {
            // Inside mode: entity must be fully contained in selection box
            selected = selectionBox.containsBox(entityBox);
        } else {
            // Crossing mode: entity must touch or intersect selection box
            selected = selectionBox.intersects(entityBox);
        }

        if (selected) {
            result.push_back(entityWithMeta.handle);
        }
    }

    return result;
}

} // namespace UI
} // namespace OwnCAD
