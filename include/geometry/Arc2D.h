#pragma once

#include "Point2D.h"
#include "BoundingBox.h"
#include <optional>
#include <memory>

namespace OwnCAD {
namespace Geometry {

/**
 * @brief Immutable 2D circular arc
 *
 * Represents a circular arc defined by:
 * - Center point
 * - Radius
 * - Start angle (radians)
 * - End angle (radians)
 * - Direction (counter-clockwise or clockwise)
 *
 * Design decisions:
 * - Immutable: Cannot modify properties after construction
 * - Factory pattern: Use create() to handle validation
 * - Angles normalized to [0, 2π) internally
 * - Direction preserved for CNC toolpath correctness
 * - Lazy caching: Bounding box computed on demand
 *
 * Validation rules:
 * - Radius must be >= MIN_ARC_RADIUS
 * - Angles must be finite (no NaN/infinity)
 * - Sweep angle must be >= MIN_ARC_SWEEP (prevents degenerate arcs)
 * - Full circles (360°) are valid and have special handling
 *
 * CRITICAL FOR MANUFACTURING:
 * Arc direction (CCW vs CW) affects toolpath generation and must
 * be preserved exactly as specified in the source DXF.
 */
class Arc2D {
private:
    Point2D center_;
    double radius_;
    double startAngle_;  // Normalized [0, 2π)
    double endAngle_;    // Normalized [0, 2π)
    bool counterClockwise_;

    // Cached values for performance
    mutable std::optional<BoundingBox> cachedBBox_;
    mutable double cachedLength_;
    mutable bool lengthCached_;

    /**
     * @brief Private constructor - use create() factory method
     */
    Arc2D(const Point2D& center, double radius,
          double startAngle, double endAngle, bool ccw) noexcept;

public:
    /**
     * @brief Factory method to create a validated arc
     *
     * @param center Center point
     * @param radius Radius (must be > 0)
     * @param startAngle Start angle in radians
     * @param endAngle End angle in radians
     * @param counterClockwise Direction flag (default: true)
     * @return Arc2D object if valid, std::nullopt if invalid
     *
     * Returns nullopt if:
     * - Center point is invalid (NaN/infinity)
     * - Radius <= MIN_ARC_RADIUS
     * - Angles are invalid (NaN/infinity)
     * - Sweep angle < MIN_ARC_SWEEP
     */
    static std::optional<Arc2D> create(
        const Point2D& center,
        double radius,
        double startAngle,
        double endAngle,
        bool counterClockwise = true
    ) noexcept;

    /**
     * @brief Get center point
     */
    const Point2D& center() const noexcept { return center_; }

    /**
     * @brief Get radius
     */
    double radius() const noexcept { return radius_; }

    /**
     * @brief Get start angle (radians, normalized to [0, 2π))
     */
    double startAngle() const noexcept { return startAngle_; }

    /**
     * @brief Get end angle (radians, normalized to [0, 2π))
     */
    double endAngle() const noexcept { return endAngle_; }

    /**
     * @brief Check if arc is counter-clockwise
     */
    bool isCounterClockwise() const noexcept { return counterClockwise_; }

    /**
     * @brief Calculate sweep angle (radians)
     * @return Angle swept by arc, considering direction
     */
    double sweepAngle() const noexcept;

    /**
     * @brief Check if this is a full circle (360°)
     */
    bool isFullCircle() const noexcept;

    /**
     * @brief Get start point of arc
     */
    Point2D startPoint() const noexcept;

    /**
     * @brief Get end point of arc
     */
    Point2D endPoint() const noexcept;

    /**
     * @brief Get point at angle on arc
     * @param angle Angle in radians
     * @return Point on arc at specified angle
     */
    Point2D pointAtAngle(double angle) const noexcept;

    /**
     * @brief Get point at parameter t along arc
     * @param t Parameter [0,1] where 0=start, 1=end
     * @return Point at position along arc
     */
    Point2D pointAt(double t) const noexcept;

    /**
     * @brief Calculate arc length
     * @return Length in world units (cached after first call)
     */
    double length() const noexcept;

    /**
     * @brief Get bounding box
     *
     * This is complex! Must check if arc crosses 0°, 90°, 180°, or 270°
     * to find actual min/max X and Y coordinates.
     *
     * @return Axis-aligned bounding box (cached after first call)
     */
    const BoundingBox& boundingBox() const noexcept;

    /**
     * @brief Check if this arc equals another within tolerance
     * @param other Arc to compare with
     * @param tolerance Maximum allowed difference
     * @return true if all properties match within tolerance
     */
    bool isEqual(const Arc2D& other, double tolerance) const noexcept;

    /**
     * @brief Validate that this arc is geometrically valid
     * @return true if radius and sweep are valid
     */
    bool isValid() const noexcept;

    /**
     * @brief Static validation helper
     * @param center Center point
     * @param radius Radius
     * @param startAngle Start angle
     * @param endAngle End angle
     * @param ccw Direction
     * @return true if these values would form a valid arc
     */
    static bool wouldBeValid(
        const Point2D& center,
        double radius,
        double startAngle,
        double endAngle,
        bool ccw = true
    ) noexcept;

    /**
     * @brief Check if point lies on this arc
     * @param point Point to test
     * @param tolerance Maximum distance from arc
     * @return true if point is on the arc within tolerance
     */
    bool containsPoint(const Point2D& point, double tolerance) const noexcept;
};

} // namespace Geometry
} // namespace OwnCAD
