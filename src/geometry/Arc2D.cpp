#include "geometry/Arc2D.h"
#include "geometry/GeometryConstants.h"
#include "geometry/GeometryMath.h"
#include <cmath>

namespace OwnCAD {
namespace Geometry {

// ============================================================================
// CONSTRUCTION
// ============================================================================

Arc2D::Arc2D(const Point2D& center, double radius,
             double startAngle, double endAngle, bool ccw) noexcept
    : center_(center)
    , radius_(radius)
    , startAngle_(GeometryMath::normalizeAngle(startAngle))
    , endAngle_(GeometryMath::normalizeAngle(endAngle))
    , counterClockwise_(ccw)
    , cachedBBox_(std::nullopt)
    , cachedLength_(0.0)
    , lengthCached_(false) {
}

std::optional<Arc2D> Arc2D::create(
    const Point2D& center,
    double radius,
    double startAngle,
    double endAngle,
    bool counterClockwise
) noexcept {
    if (!wouldBeValid(center, radius, startAngle, endAngle, counterClockwise)) {
        return std::nullopt;
    }
    return Arc2D(center, radius, startAngle, endAngle, counterClockwise);
}

// ============================================================================
// QUERIES
// ============================================================================

double Arc2D::sweepAngle() const noexcept {
    return GeometryMath::sweepAngle(startAngle_, endAngle_, counterClockwise_);
}

bool Arc2D::isFullCircle() const noexcept {
    const double sweep = sweepAngle();
    return std::abs(sweep - TWO_PI) < GEOMETRY_EPSILON;
}

Point2D Arc2D::startPoint() const noexcept {
    return pointAtAngle(startAngle_);
}

Point2D Arc2D::endPoint() const noexcept {
    return pointAtAngle(endAngle_);
}

Point2D Arc2D::pointAtAngle(double angle) const noexcept {
    const double x = center_.x() + radius_ * std::cos(angle);
    const double y = center_.y() + radius_ * std::sin(angle);
    return Point2D(x, y);
}

Point2D Arc2D::pointAt(double t) const noexcept {
    const double sweep = sweepAngle();
    const double angle = counterClockwise_
        ? startAngle_ + t * sweep
        : startAngle_ - t * sweep;
    return pointAtAngle(angle);
}

double Arc2D::length() const noexcept {
    if (!lengthCached_) {
        cachedLength_ = GeometryMath::arcLength(radius_, sweepAngle());
        lengthCached_ = true;
    }
    return cachedLength_;
}

const BoundingBox& Arc2D::boundingBox() const noexcept {
    if (!cachedBBox_) {
        cachedBBox_ = BoundingBox::fromArc(*this);
    }
    return *cachedBBox_;
}

bool Arc2D::isEqual(const Arc2D& other, double tolerance) const noexcept {
    return center_.isEqual(other.center_, tolerance) &&
           GeometryMath::areEqual(radius_, other.radius_, tolerance) &&
           GeometryMath::areEqual(startAngle_, other.startAngle_, tolerance) &&
           GeometryMath::areEqual(endAngle_, other.endAngle_, tolerance) &&
           counterClockwise_ == other.counterClockwise_;
}

bool Arc2D::isValid() const noexcept {
    return wouldBeValid(center_, radius_, startAngle_, endAngle_, counterClockwise_);
}

bool Arc2D::wouldBeValid(
    const Point2D& center,
    double radius,
    double startAngle,
    double endAngle,
    bool ccw
) noexcept {
    if (!center.isValid()) {
        return false;
    }

    if (!std::isfinite(radius) || radius < MIN_ARC_RADIUS) {
        return false;
    }

    if (!std::isfinite(startAngle) || !std::isfinite(endAngle)) {
        return false;
    }

    const double sweep = GeometryMath::sweepAngle(startAngle, endAngle, ccw);
    if (sweep < MIN_ARC_SWEEP) {
        return false;
    }

    return true;
}

bool Arc2D::containsPoint(const Point2D& point, double tolerance) const noexcept {
    // Check if point is at correct radius
    const double dist = center_.distanceTo(point);
    if (std::abs(dist - radius_) > tolerance) {
        return false;
    }

    // Check if angle is within arc sweep
    const double angle = GeometryMath::angleBetweenPoints(center_, point);
    return GeometryMath::isAngleBetween(angle, startAngle_, endAngle_, counterClockwise_);
}

} // namespace Geometry
} // namespace OwnCAD
