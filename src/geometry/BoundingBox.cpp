#include "geometry/BoundingBox.h"
#include "geometry/Line2D.h"
#include "geometry/Arc2D.h"
#include "geometry/GeometryMath.h"
#include "geometry/GeometryConstants.h"
#include <algorithm>
#include <limits>

namespace OwnCAD {
namespace Geometry {

// ============================================================================
// CONSTRUCTION
// ============================================================================

BoundingBox::BoundingBox() noexcept
    : minX_(std::numeric_limits<double>::max())
    , minY_(std::numeric_limits<double>::max())
    , maxX_(std::numeric_limits<double>::lowest())
    , maxY_(std::numeric_limits<double>::lowest()) {
}

BoundingBox::BoundingBox(double minX, double minY, double maxX, double maxY) noexcept
    : minX_(minX), minY_(minY), maxX_(maxX), maxY_(maxY) {
}

// ============================================================================
// FACTORY METHODS
// ============================================================================

BoundingBox BoundingBox::fromPoints(const Point2D& p1, const Point2D& p2) noexcept {
    return BoundingBox(
        std::min(p1.x(), p2.x()),
        std::min(p1.y(), p2.y()),
        std::max(p1.x(), p2.x()),
        std::max(p1.y(), p2.y())
    );
}

BoundingBox BoundingBox::fromPointList(const std::vector<Point2D>& points) noexcept {
    if (points.empty()) {
        return BoundingBox(); // Invalid box
    }

    double minX = points[0].x();
    double minY = points[0].y();
    double maxX = points[0].x();
    double maxY = points[0].y();

    for (size_t i = 1; i < points.size(); ++i) {
        minX = std::min(minX, points[i].x());
        minY = std::min(minY, points[i].y());
        maxX = std::max(maxX, points[i].x());
        maxY = std::max(maxY, points[i].y());
    }

    return BoundingBox(minX, minY, maxX, maxY);
}

BoundingBox BoundingBox::fromLine(const Line2D& line) noexcept {
    return fromPoints(line.start(), line.end());
}

BoundingBox BoundingBox::fromArc(const Arc2D& arc) noexcept {
    // Start with endpoints
    std::vector<Point2D> points;
    points.push_back(arc.startPoint());
    points.push_back(arc.endPoint());

    // Check if arc crosses 0° (maxX)
    if (GeometryMath::isAngleBetween(0.0, arc.startAngle(), arc.endAngle(),
                                     arc.isCounterClockwise())) {
        points.push_back(Point2D(arc.center().x() + arc.radius(), arc.center().y()));
    }

    // Check if arc crosses 90° (maxY)
    if (GeometryMath::isAngleBetween(HALF_PI, arc.startAngle(), arc.endAngle(),
                                     arc.isCounterClockwise())) {
        points.push_back(Point2D(arc.center().x(), arc.center().y() + arc.radius()));
    }

    // Check if arc crosses 180° (minX)
    if (GeometryMath::isAngleBetween(PI, arc.startAngle(), arc.endAngle(),
                                     arc.isCounterClockwise())) {
        points.push_back(Point2D(arc.center().x() - arc.radius(), arc.center().y()));
    }

    // Check if arc crosses 270° (minY)
    if (GeometryMath::isAngleBetween(PI + HALF_PI, arc.startAngle(), arc.endAngle(),
                                     arc.isCounterClockwise())) {
        points.push_back(Point2D(arc.center().x(), arc.center().y() - arc.radius()));
    }

    return fromPointList(points);
}

// ============================================================================
// QUERIES
// ============================================================================

Point2D BoundingBox::center() const noexcept {
    return Point2D((minX_ + maxX_) / 2.0, (minY_ + maxY_) / 2.0);
}

bool BoundingBox::isValid() const noexcept {
    return minX_ <= maxX_ && minY_ <= maxY_;
}

// ============================================================================
// CONTAINMENT TESTS
// ============================================================================

bool BoundingBox::contains(const Point2D& point, double tolerance) const noexcept {
    return point.x() >= minX_ - tolerance &&
           point.x() <= maxX_ + tolerance &&
           point.y() >= minY_ - tolerance &&
           point.y() <= maxY_ + tolerance;
}

bool BoundingBox::intersects(const BoundingBox& other) const noexcept {
    return !(maxX_ < other.minX_ ||
             minX_ > other.maxX_ ||
             maxY_ < other.minY_ ||
             minY_ > other.maxY_);
}

bool BoundingBox::containsBox(const BoundingBox& other) const noexcept {
    return other.minX_ >= minX_ &&
           other.maxX_ <= maxX_ &&
           other.minY_ >= minY_ &&
           other.maxY_ <= maxY_;
}

// ============================================================================
// OPERATIONS
// ============================================================================

BoundingBox BoundingBox::merge(const BoundingBox& other) const noexcept {
    if (!isValid()) return other;
    if (!other.isValid()) return *this;

    return BoundingBox(
        std::min(minX_, other.minX_),
        std::min(minY_, other.minY_),
        std::max(maxX_, other.maxX_),
        std::max(maxY_, other.maxY_)
    );
}

BoundingBox BoundingBox::expand(double margin) const noexcept {
    return BoundingBox(
        minX_ - margin,
        minY_ - margin,
        maxX_ + margin,
        maxY_ + margin
    );
}

} // namespace Geometry
} // namespace OwnCAD
