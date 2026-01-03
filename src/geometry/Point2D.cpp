#include "geometry/Point2D.h"
#include "geometry/GeometryConstants.h"
#include <cmath>
#include <stdexcept>
#include <limits>

namespace OwnCAD {
namespace Geometry {

// ============================================================================
// CONSTRUCTION
// ============================================================================

Point2D::Point2D() noexcept
    : x_(0.0), y_(0.0) {
}

Point2D::Point2D(double x, double y)
    : x_(x), y_(y) {
    if (!isValid(x, y)) {
        throw std::invalid_argument(
            "Point2D: Cannot construct with invalid coordinates (NaN or infinity)"
        );
    }
}

// ============================================================================
// COMPARISON
// ============================================================================

bool Point2D::operator==(const Point2D& other) const noexcept {
    return x_ == other.x_ && y_ == other.y_;
}

bool Point2D::operator!=(const Point2D& other) const noexcept {
    return !(*this == other);
}

bool Point2D::isEqual(const Point2D& other, double tolerance) const noexcept {
    return std::abs(x_ - other.x_) < tolerance &&
           std::abs(y_ - other.y_) < tolerance;
}

// ============================================================================
// DISTANCE CALCULATIONS
// ============================================================================

double Point2D::distanceTo(const Point2D& other) const noexcept {
    return std::sqrt(distanceSquaredTo(other));
}

double Point2D::distanceSquaredTo(const Point2D& other) const noexcept {
    const double dx = x_ - other.x_;
    const double dy = y_ - other.y_;
    return dx * dx + dy * dy;
}

// ============================================================================
// VALIDATION
// ============================================================================

bool Point2D::isValid() const noexcept {
    return isValid(x_, y_);
}

bool Point2D::isValid(double x, double y) noexcept {
    return std::isfinite(x) && std::isfinite(y);
}

} // namespace Geometry
} // namespace OwnCAD
