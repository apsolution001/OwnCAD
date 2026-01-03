#include "geometry/Line2D.h"
#include "geometry/GeometryConstants.h"
#include "geometry/GeometryMath.h"
#include <cmath>

namespace OwnCAD {
namespace Geometry {

// ============================================================================
// CONSTRUCTION
// ============================================================================

Line2D::Line2D(const Point2D& start, const Point2D& end) noexcept
    : start_(start)
    , end_(end)
    , cachedBBox_(std::nullopt)
    , cachedLength_(0.0)
    , lengthCached_(false) {
}

std::optional<Line2D> Line2D::create(const Point2D& start, const Point2D& end) noexcept {
    if (!wouldBeValid(start, end)) {
        return std::nullopt;
    }
    return Line2D(start, end);
}

// ============================================================================
// QUERIES
// ============================================================================

double Line2D::length() const noexcept {
    if (!lengthCached_) {
        cachedLength_ = start_.distanceTo(end_);
        lengthCached_ = true;
    }
    return cachedLength_;
}

const BoundingBox& Line2D::boundingBox() const noexcept {
    if (!cachedBBox_) {
        cachedBBox_ = BoundingBox::fromLine(*this);
    }
    return *cachedBBox_;
}

bool Line2D::isEqual(const Line2D& other, double tolerance) const noexcept {
    return start_.isEqual(other.start_, tolerance) &&
           end_.isEqual(other.end_, tolerance);
}

bool Line2D::isValid() const noexcept {
    return wouldBeValid(start_, end_);
}

bool Line2D::wouldBeValid(const Point2D& start, const Point2D& end) noexcept {
    if (!start.isValid() || !end.isValid()) {
        return false;
    }

    const double distSq = start.distanceSquaredTo(end);
    return distSq >= MIN_LINE_LENGTH * MIN_LINE_LENGTH;
}

bool Line2D::containsPoint(const Point2D& point, double tolerance) const noexcept {
    // Check if point is within bounding box first (quick rejection)
    if (!boundingBox().contains(point, tolerance)) {
        return false;
    }

    // Check distance to segment
    const double dist = GeometryMath::distancePointToSegment(point, *this);
    return dist < tolerance;
}

Point2D Line2D::pointAt(double t) const noexcept {
    const double x = start_.x() + t * (end_.x() - start_.x());
    const double y = start_.y() + t * (end_.y() - start_.y());
    return Point2D(x, y);
}

double Line2D::angle() const noexcept {
    return GeometryMath::angleBetweenPoints(start_, end_);
}

} // namespace Geometry
} // namespace OwnCAD
