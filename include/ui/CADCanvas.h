#pragma once

#include <QWidget>
#include <QPoint>
#include <QPointF>
#include "geometry/Point2D.h"
#include "geometry/Line2D.h"
#include "geometry/Arc2D.h"
#include "geometry/BoundingBox.h"
#include "import/GeometryConverter.h"
#include "ui/GridSettingsDialog.h"
#include "ui/SelectionManager.h"
#include <vector>
#include <optional>

namespace OwnCAD {
namespace UI {

/**
 * @brief Viewport state manager
 *
 * Handles coordinate transformation between world space and screen space.
 * World space: DXF coordinates (mm, inches, etc.)
 * Screen space: Qt pixel coordinates
 */
class Viewport {
public:
    Viewport();

    // Coordinate transformations
    QPointF worldToScreen(const Geometry::Point2D& worldPoint) const;
    Geometry::Point2D screenToWorld(const QPointF& screenPoint) const;

    // Pan controls
    void pan(double dx, double dy);
    void setPan(double x, double y);
    double panX() const { return panX_; }
    double panY() const { return panY_; }

    // Zoom controls
    void zoom(double factor, const QPointF& center);
    void setZoom(double level);
    double zoomLevel() const { return zoomLevel_; }

    // Viewport management
    void reset();
    void setViewportSize(int width, int height);

private:
    double panX_;       // Pan offset X (screen space)
    double panY_;       // Pan offset Y (screen space)
    double zoomLevel_;  // Zoom level (1.0 = 1 world unit = 1 pixel)
    int viewportWidth_;
    int viewportHeight_;
};

/**
 * @brief Snap manager for precise point input
 *
 * Implements CAD-style snapping with priority:
 * 1. Endpoint - snap to line/arc endpoints (most precise)
 * 2. Midpoint - snap to line/arc midpoints (construction reference)
 * 3. Nearest - snap to nearest point on entity curve (general alignment)
 * 4. Grid - snap to grid intersections (fallback)
 */
class SnapManager {
public:
    enum class SnapMode {
        None = 0,
        Grid = 1,
        Endpoint = 2,
        Midpoint = 4,
        Nearest = 8
    };

    enum class SnapType {
        None,
        Grid,
        Endpoint,
        Midpoint,
        Nearest
    };

    SnapManager();

    // Snap settings
    void setSnapMode(SnapMode mode, bool enabled);
    bool isSnapEnabled(SnapMode mode) const;
    void setGridSpacing(double spacing) { gridSpacing_ = spacing; }
    double gridSpacing() const { return gridSpacing_; }
    void setSnapTolerancePixels(double pixels) { snapTolerancePixels_ = pixels; }
    double snapTolerancePixels() const { return snapTolerancePixels_; }

    // Snap calculation (zoom-aware)
    std::optional<Geometry::Point2D> snap(
        const Geometry::Point2D& point,
        const std::vector<Import::GeometryEntityWithMetadata>& entities,
        double zoomLevel
    );

    // Snap result tracking
    std::optional<Geometry::Point2D> lastSnapPoint() const { return lastSnapPoint_; }
    SnapType lastSnapType() const { return lastSnapType_; }

private:
    Geometry::Point2D snapToGrid(const Geometry::Point2D& point) const;
    std::optional<Geometry::Point2D> snapToEndpoint(
        const Geometry::Point2D& point,
        const std::vector<Import::GeometryEntityWithMetadata>& entities,
        double worldTolerance
    ) const;
    std::optional<Geometry::Point2D> snapToMidpoint(
        const Geometry::Point2D& point,
        const std::vector<Import::GeometryEntityWithMetadata>& entities,
        double worldTolerance
    ) const;
    std::optional<Geometry::Point2D> snapToNearest(
        const Geometry::Point2D& point,
        const std::vector<Import::GeometryEntityWithMetadata>& entities,
        double worldTolerance
    ) const;

    int snapModes_;
    double gridSpacing_;              // Grid spacing (world units)
    double snapTolerancePixels_;      // Snap detection radius (screen pixels)

    // Snap result tracking
    std::optional<Geometry::Point2D> lastSnapPoint_;
    SnapType lastSnapType_;
};

/**
 * @brief Box selection mode
 *
 * Inside: Entity must be fully inside selection box (left-to-right drag)
 * Crossing: Entity can touch or cross selection box (right-to-left drag)
 */
enum class BoxSelectMode {
    None,
    Inside,   // Entity fully inside box (solid rectangle)
    Crossing  // Entity touches or crosses box (dashed rectangle)
};

/**
 * @brief Main CAD canvas widget for geometry rendering and interaction
 *
 * Features:
 * - Renders Line2D and Arc2D entities
 * - Pan (middle mouse button drag)
 * - Zoom (mouse wheel)
 * - Grid rendering with adaptive spacing
 * - Snap system (grid, endpoint, midpoint)
 * - Selection feedback
 * - Box selection (inside and crossing modes)
 *
 * Design principles:
 * - Viewport transformation separates world/screen coordinates
 * - All rendering happens in paintEvent
 * - Mouse events update viewport state and trigger repaint
 * - No direct geometry modification (read-only view for now)
 */
class CADCanvas : public QWidget {
    Q_OBJECT

public:
    explicit CADCanvas(QWidget* parent = nullptr);
    ~CADCanvas();

    // Entity management
    void setEntities(const std::vector<Import::GeometryEntityWithMetadata>& entities);
    void clear();

    // Grid settings
    void setGridSettings(const GridSettings& settings);
    GridSettings gridSettings() const { return gridSettings_; }
    void setGridVisible(bool visible);
    bool isGridVisible() const { return gridSettings_.visible; }
    void setGridSpacing(double spacing);

    // Snap settings
    void setSnapEnabled(SnapManager::SnapMode mode, bool enabled);
    bool isSnapEnabled(SnapManager::SnapMode mode) const;

    // Viewport controls
    void resetView();
    void zoomExtents();
    const Viewport& viewport() const { return viewport_; }

signals:
    void viewportChanged(double zoom, double panX, double panY);
    void cursorPositionChanged(double x, double y);
    void selectionChanged(size_t selectedCount);

protected:
    // Qt event handlers
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // Rendering methods
    void renderGrid(QPainter& painter);
    void renderOrigin(QPainter& painter);
    void renderEntities(QPainter& painter);
    void renderEntity(QPainter& painter, const Import::GeometryEntityWithMetadata& entity);
    void renderLine(QPainter& painter, const Geometry::Line2D& line, const Import::GeometryEntityWithMetadata& metadata);
    void renderArc(QPainter& painter, const Geometry::Arc2D& arc, const Import::GeometryEntityWithMetadata& metadata);
    void renderEllipse(QPainter& painter, const Geometry::Ellipse2D& ellipse, const Import::GeometryEntityWithMetadata& metadata);
    void renderPoint(QPainter& painter, const Geometry::Point2D& point, const Import::GeometryEntityWithMetadata& metadata);
    void renderSnapIndicator(QPainter& painter);
    void renderSelectionBoundingBox(QPainter& painter);
    void renderGripPoints(QPainter& painter);

    // Data members
    std::vector<Import::GeometryEntityWithMetadata> entities_;
    Viewport viewport_;
    SnapManager snapManager_;

    // UI state
    GridSettings gridSettings_;

    // Mouse interaction state
    bool isPanning_;
    QPoint lastMousePos_;
    Geometry::Point2D lastWorldPos_;

    // Selection state
    SelectionManager selectionManager_;

    // Box selection state
    BoxSelectMode boxSelectMode_;
    QPointF boxSelectStartScreen_;  // Start point in screen coordinates
    QPointF boxSelectCurrentScreen_;  // Current point in screen coordinates

    // Hit testing
    std::string hitTest(const Geometry::Point2D& point);

    // Box selection helpers
    void renderSelectionBox(QPainter& painter);
    std::vector<std::string> getEntitiesInBox(const Geometry::BoundingBox& selectionBox, BoxSelectMode mode);
};

} // namespace UI
} // namespace OwnCAD
