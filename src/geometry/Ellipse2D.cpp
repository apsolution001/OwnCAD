#include "geometry/Ellipse2D.h"
#include "geometry/GeometryMath.h"
#include <algorithm>
#include <cmath>

namespace OwnCAD {
namespace Geometry {

// ============================================================================
// FACTORY
// ============================================================================

std::optional<Ellipse2D> Ellipse2D::create(
    const Point2D& center,
    const Point2D& majorAxisEnd,
    double minorAxisRatio,
    double startAngle,
    double endAngle
) {
    if (!wouldBeValid(center, majorAxisEnd, minorAxisRatio, startAngle, endAngle)) {
        return std::nullopt;
    }

    return Ellipse2D(center, majorAxisEnd, minorAxisRatio, startAngle, endAngle);
}

bool Ellipse2D::wouldBeValid(
    const Point2D& center,
    const Point2D& majorAxisEnd,
    double minorAxisRatio,
    double startAngle,
    double endAngle
) noexcept {
    // Check if center is valid
    if (!center.isValid()) {
        return false;
    }

    // Calculate major axis length
    double dx = majorAxisEnd.x() - center.x();
    double dy = majorAxisEnd.y() - center.y();
    double majorLength = std::sqrt(dx * dx + dy * dy);

    // Check major axis length
    if (majorLength < MIN_LINE_LENGTH) {
        return false;
    }

    // Check minor axis ratio (must be between 0 and 1, exclusive/inclusive)
    if (minorAxisRatio <= 0.0 || minorAxisRatio > 1.0) {
        return false;
    }

    // Check angles are valid numbers
    if (!std::isfinite(startAngle) || !std::isfinite(endAngle)) {
        return false;
    }

    // Normalize angles to [0, 2π)
    double normStart = GeometryMath::normalizeAngle(startAngle);
    double normEnd = GeometryMath::normalizeAngle(endAngle);

    // For full ellipse, angles should be 0 and 2π (or very close)
    // For arc, end should be different from start
    if (GeometryMath::areEqual(normStart, normEnd, MIN_ARC_SWEEP)) {
        // Check if this is a full ellipse (2π sweep)
        double sweep = std::abs(endAngle - startAngle);
        if (!GeometryMath::areEqual(sweep, 2.0 * PI, MIN_ARC_SWEEP)) {
            return false;  // Zero-sweep arc
        }
    }

    return true;
}

// ============================================================================
// CONSTRUCTOR
// ============================================================================

Ellipse2D::Ellipse2D(
    const Point2D& center,
    const Point2D& majorAxisEnd,
    double minorAxisRatio,
    double startAngle,
    double endAngle
)
    : center_(center)
    , majorAxisEnd_(majorAxisEnd)
    , minorAxisRatio_(minorAxisRatio)
    , startAngle_(GeometryMath::normalizeAngle(startAngle))
    , endAngle_(GeometryMath::normalizeAngle(endAngle))
    , boundingBoxCached_(false)
    , boundingBox_(BoundingBox::fromPoints(center, center))  // Dummy initial value
    , lengthsCached_(false)
    , majorLength_(0.0)
    , minorLength_(0.0)
    , rotation_(0.0)
{
}

// ============================================================================
// ACCESSORS
// ============================================================================

double Ellipse2D::majorAxisLength() const noexcept {
    if (!lengthsCached_) {
        double dx = majorAxisEnd_.x() - center_.x();
        double dy = majorAxisEnd_.y() - center_.y();
        majorLength_ = std::sqrt(dx * dx + dy * dy);
        minorLength_ = majorLength_ * minorAxisRatio_;
        rotation_ = std::atan2(dy, dx);
        lengthsCached_ = true;
    }
    return majorLength_;
}

double Ellipse2D::minorAxisLength() const noexcept {
    if (!lengthsCached_) {
        majorAxisLength();  // This will cache all values
    }
    return minorLength_;
}

double Ellipse2D::rotation() const noexcept {
    if (!lengthsCached_) {
        majorAxisLength();  // This will cache all values
    }
    return rotation_;
}

bool Ellipse2D::isFullEllipse() const noexcept {
    double sweep = std::abs(endAngle_ - startAngle_);
    if (sweep < PI) {
        // Handle wrap-around case
        sweep = 2.0 * PI - sweep;
    }
    return GeometryMath::areEqual(sweep, 2.0 * PI, MIN_ARC_SWEEP);
}

double Ellipse2D::sweepAngle() const noexcept {
    if (endAngle_ >= startAngle_) {
        return endAngle_ - startAngle_;
    } else {
        return (2.0 * PI - startAngle_) + endAngle_;
    }
}

// ============================================================================
// GEOMETRY QUERIES
// ============================================================================

Point2D Ellipse2D::pointAt(double t) const noexcept {
    double angle = startAngle_ + t * sweepAngle();
    return pointAtAngle(angle);
}

Point2D Ellipse2D::pointAtAngle(double angle) const noexcept {
    double a = majorAxisLength();
    double b = minorAxisLength();
    double rot = rotation();

    // Parametric ellipse equation:
    // x = center.x + a*cos(angle)*cos(rot) - b*sin(angle)*sin(rot)
    // y = center.y + a*cos(angle)*sin(rot) + b*sin(angle)*cos(rot)

    double cosAngle = std::cos(angle);
    double sinAngle = std::sin(angle);
    double cosRot = std::cos(rot);
    double sinRot = std::sin(rot);

    double x = center_.x() + a * cosAngle * cosRot - b * sinAngle * sinRot;
    double y = center_.y() + a * cosAngle * sinRot + b * sinAngle * cosRot;

    return Point2D(x, y);
}

const BoundingBox& Ellipse2D::boundingBox() const noexcept {
    if (boundingBoxCached_) {
        return boundingBox_;
    }

    // For full ellipse, we can calculate exact bounding box
    // For elliptical arc, we need to check extrema within the arc sweep

    double a = majorAxisLength();
    double b = minorAxisLength();
    double rot = rotation();

    // Calculate bounding box by checking critical angles
    // For rotated ellipse, extrema occur at specific angles

    double cosRot = std::cos(rot);
    double sinRot = std::sin(rot);

    // Find x extrema: dx/dθ = 0
    // This gives: tan(θ) = -b*tan(rot) / a
    double xExtremaAngle1 = std::atan2(-b * sinRot, a * cosRot);
    double xExtremaAngle2 = xExtremaAngle1 + PI;

    // Find y extrema: dy/dθ = 0
    // This gives: tan(θ) = b*cot(rot) / a = b*cos(rot) / (a*sin(rot))
    double yExtremaAngle1 = std::atan2(b * cosRot, a * sinRot);
    double yExtremaAngle2 = yExtremaAngle1 + PI;

    // Normalize angles
    xExtremaAngle1 = GeometryMath::normalizeAngle(xExtremaAngle1);
    xExtremaAngle2 = GeometryMath::normalizeAngle(xExtremaAngle2);
    yExtremaAngle1 = GeometryMath::normalizeAngle(yExtremaAngle1);
    yExtremaAngle2 = GeometryMath::normalizeAngle(yExtremaAngle2);

    // Start with start and end points
    Point2D p1 = startPoint();
    Point2D p2 = endPoint();

    double minX = std::min(p1.x(), p2.x());
    double maxX = std::max(p1.x(), p2.x());
    double minY = std::min(p1.y(), p2.y());
    double maxY = std::max(p1.y(), p2.y());

    // Check if extrema angles are within the arc sweep
    auto checkExtrema = [&](double angle) {
        if (isFullEllipse() || GeometryMath::isAngleBetween(angle, startAngle_, endAngle_, true)) {
            Point2D p = pointAtAngle(angle);
            minX = std::min(minX, p.x());
            maxX = std::max(maxX, p.x());
            minY = std::min(minY, p.y());
            maxY = std::max(maxY, p.y());
        }
    };

    checkExtrema(xExtremaAngle1);
    checkExtrema(xExtremaAngle2);
    checkExtrema(yExtremaAngle1);
    checkExtrema(yExtremaAngle2);

    boundingBox_ = BoundingBox::fromPoints(Point2D(minX, minY), Point2D(maxX, maxY));
    boundingBoxCached_ = true;

    return boundingBox_;
}

bool Ellipse2D::containsPoint(const Point2D& point, double tolerance) const noexcept {
    // Transform point to ellipse local coordinates (centered, unrotated)
    double rot = rotation();
    double dx = point.x() - center_.x();
    double dy = point.y() - center_.y();

    // Rotate point by -rotation to align with ellipse axes
    double cosRot = std::cos(-rot);
    double sinRot = std::sin(-rot);
    double localX = dx * cosRot - dy * sinRot;
    double localY = dx * sinRot + dy * cosRot;

    // Check if point is on ellipse: (x/a)² + (y/b)² = 1
    double a = majorAxisLength();
    double b = minorAxisLength();

    double ellipseEq = (localX * localX) / (a * a) + (localY * localY) / (b * b);

    if (!GeometryMath::areEqual(ellipseEq, 1.0, tolerance)) {
        return false;
    }

    // If this is an arc, check if point is within angular range
    if (!isFullEllipse()) {
        double angle = std::atan2(localY / b, localX / a);
        angle = GeometryMath::normalizeAngle(angle);
        if (!GeometryMath::isAngleBetween(angle, startAngle_, endAngle_, true)) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// VALIDATION
// ============================================================================

bool Ellipse2D::isValid() const noexcept {
    return wouldBeValid(center_, majorAxisEnd_, minorAxisRatio_, startAngle_, endAngle_);
}

bool Ellipse2D::isEqual(const Ellipse2D& other, double tolerance) const noexcept {
    return center_.isEqual(other.center_, tolerance)
        && majorAxisEnd_.isEqual(other.majorAxisEnd_, tolerance)
        && GeometryMath::areEqual(minorAxisRatio_, other.minorAxisRatio_, tolerance)
        && GeometryMath::areEqual(startAngle_, other.startAngle_, tolerance)
        && GeometryMath::areEqual(endAngle_, other.endAngle_, tolerance);
}

} // namespace Geometry
} // namespace OwnCAD
