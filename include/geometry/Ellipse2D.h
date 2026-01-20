#pragma once

#include "Point2D.h"
#include "BoundingBox.h"
#include "GeometryConstants.h"
#include <optional>
#include <cmath>

namespace OwnCAD {
namespace Geometry {

/**
 * @brief 2D Ellipse geometry primitive
 *
 * Represents an ellipse defined by:
 * - Center point
 * - Major axis endpoint (relative to center)
 * - Minor axis length ratio
 * - Start/end angles (for elliptical arcs)
 *
 * Design decisions:
 * - Immutable after construction (thread-safe)
 * - Factory pattern with validation
 * - Lazy bounding box caching
 * - Tolerance-based equality
 * - Manufacturing-grade precision
 *
 * Full ellipse: startAngle = 0, endAngle = 2π
 * Elliptical arc: startAngle < endAngle
 */
class Ellipse2D {
public:
    /**
     * @brief Factory method to create an ellipse with validation
     * @param center Center point of ellipse
     * @param majorAxisEnd Endpoint of major axis (relative to center defines length and rotation)
     * @param minorAxisRatio Ratio of minor to major axis (0 < ratio <= 1.0)
     * @param startAngle Start angle in radians (default 0 for full ellipse)
     * @param endAngle End angle in radians (default 2π for full ellipse)
     * @return Ellipse2D if valid, nullopt if invalid
     */
    static std::optional<Ellipse2D> create(
        const Point2D& center,
        const Point2D& majorAxisEnd,
        double minorAxisRatio,
        double startAngle = 0.0,
        double endAngle = 2.0 * PI
    );

    /**
     * @brief Validate ellipse parameters without creating object
     */
    static bool wouldBeValid(
        const Point2D& center,
        const Point2D& majorAxisEnd,
        double minorAxisRatio,
        double startAngle,
        double endAngle
    ) noexcept;

    // ========================================================================
    // ACCESSORS
    // ========================================================================

    const Point2D& center() const noexcept { return center_; }
    const Point2D& majorAxisEnd() const noexcept { return majorAxisEnd_; }
    double minorAxisRatio() const noexcept { return minorAxisRatio_; }
    double startAngle() const noexcept { return startAngle_; }
    double endAngle() const noexcept { return endAngle_; }

    /**
     * @brief Get major axis length
     */
    double majorAxisLength() const noexcept;

    /**
     * @brief Get minor axis length
     */
    double minorAxisLength() const noexcept;

    /**
     * @brief Get rotation angle of major axis (radians)
     */
    double rotation() const noexcept;

    /**
     * @brief Check if this is a full ellipse (not an arc)
     */
    bool isFullEllipse() const noexcept;

    /**
     * @brief Get sweep angle (endAngle - startAngle)
     */
    double sweepAngle() const noexcept;

    // ========================================================================
    // GEOMETRY QUERIES
    // ========================================================================

    /**
     * @brief Get point at parametric position t (0 to 1)
     * t=0 → start point, t=1 → end point
     */
    Point2D pointAt(double t) const noexcept;

    /**
     * @brief Get point at specific angle (radians)
     */
    Point2D pointAtAngle(double angle) const noexcept;

    /**
     * @brief Get start point of ellipse/arc
     */
    Point2D startPoint() const noexcept {
        return pointAtAngle(startAngle_);
    }

    /**
     * @brief Get end point of ellipse/arc
     */
    Point2D endPoint() const noexcept {
        return pointAtAngle(endAngle_);
    }

    /**
     * @brief Calculate bounding box (cached)
     */
    const BoundingBox& boundingBox() const noexcept;

    /**
     * @brief Check if point lies on ellipse (within tolerance)
     */
    bool containsPoint(const Point2D& point, double tolerance = GEOMETRY_EPSILON) const noexcept;

    // ========================================================================
    // VALIDATION
    // ========================================================================

    /**
     * @brief Runtime validation check
     */
    bool isValid() const noexcept;

    /**
     * @brief Tolerance-based equality comparison
     */
    bool isEqual(const Ellipse2D& other, double tolerance = GEOMETRY_EPSILON) const noexcept;

private:
    /**
     * @brief Private constructor - use create() factory method
     */
    Ellipse2D(
        const Point2D& center,
        const Point2D& majorAxisEnd,
        double minorAxisRatio,
        double startAngle,
        double endAngle
    );

    Point2D center_;
    Point2D majorAxisEnd_;  // Endpoint of major axis (defines length and rotation)
    double minorAxisRatio_; // Ratio of minor to major axis (0 < ratio <= 1.0)
    double startAngle_;     // Start angle in radians
    double endAngle_;       // End angle in radians

    // Cached values (mutable for lazy evaluation in const methods)
    mutable bool boundingBoxCached_;
    mutable BoundingBox boundingBox_;
    mutable bool lengthsCached_;
    mutable double majorLength_;
    mutable double minorLength_;
    mutable double rotation_;
};

} // namespace Geometry
} // namespace OwnCAD
