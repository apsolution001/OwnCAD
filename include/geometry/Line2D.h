#pragma once

#include "Point2D.h"
#include "BoundingBox.h"
#include <optional>
#include <memory>

namespace OwnCAD {
namespace Geometry {

/**
 * @brief Immutable 2D line segment
 *
 * Represents a line segment defined by two endpoints.
 * Lines are validated at construction - zero-length lines cannot be created.
 *
 * Design decisions:
 * - Immutable: Cannot modify endpoints after construction
 * - Factory pattern: Use create() to handle validation
 * - Lazy caching: Bounding box and length computed on demand
 * - No inheritance: Value type with concrete implementation
 *
 * Validation rules:
 * - Start and end must be different points (within tolerance)
 * - Both endpoints must be valid (no NaN/infinity)
 * - Minimum length enforced (MIN_LINE_LENGTH)
 */
class Line2D {
private:
    Point2D start_;
    Point2D end_;

    // Cached values for performance (computed lazily)
    mutable std::optional<BoundingBox> cachedBBox_;
    mutable double cachedLength_;
    mutable bool lengthCached_;

    /**
     * @brief Private constructor - use create() factory method
     */
    Line2D(const Point2D& start, const Point2D& end) noexcept;

public:
    /**
     * @brief Factory method to create a validated line
     *
     * @param start Start point
     * @param end End point
     * @return Line2D object if valid, std::nullopt if invalid
     *
     * Returns nullopt if:
     * - Either point is invalid (NaN/infinity)
     * - Points are coincident (length < MIN_LINE_LENGTH)
     */
    static std::optional<Line2D> create(const Point2D& start, const Point2D& end) noexcept;

    /**
     * @brief Get start point
     */
    const Point2D& start() const noexcept { return start_; }

    /**
     * @brief Get end point
     */
    const Point2D& end() const noexcept { return end_; }

    /**
     * @brief Calculate line length
     * @return Length in world units (cached after first call)
     */
    double length() const noexcept;

    /**
     * @brief Get bounding box
     * @return Axis-aligned bounding box (cached after first call)
     */
    const BoundingBox& boundingBox() const noexcept;

    /**
     * @brief Check if this line equals another within tolerance
     * @param other Line to compare with
     * @param tolerance Maximum allowed difference (default: GEOMETRY_EPSILON)
     * @return true if both endpoints match within tolerance
     */
    bool isEqual(const Line2D& other, double tolerance) const noexcept;

    /**
     * @brief Validate that this line is geometrically valid
     * @return true if length >= MIN_LINE_LENGTH
     */
    bool isValid() const noexcept;

    /**
     * @brief Static validation helper
     * @param start Start point
     * @param end End point
     * @return true if these points would form a valid line
     */
    static bool wouldBeValid(const Point2D& start, const Point2D& end) noexcept;

    /**
     * @brief Check if point lies on this line segment
     * @param point Point to test
     * @param tolerance Maximum distance from line (default: GEOMETRY_EPSILON)
     * @return true if point is on the line segment within tolerance
     */
    bool containsPoint(const Point2D& point, double tolerance) const noexcept;

    /**
     * @brief Get point at parameter t along line
     * @param t Parameter [0,1] where 0=start, 1=end
     * @return Point at position start + t*(end-start)
     */
    Point2D pointAt(double t) const noexcept;

    /**
     * @brief Calculate angle of line in radians
     * @return Angle from start to end, range [0, 2π)
     */
    double angle() const noexcept;
};

} // namespace Geometry
} // namespace OwnCAD
