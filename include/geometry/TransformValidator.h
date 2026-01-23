#pragma once

/**
 * @file TransformValidator.h
 * @brief Validation utilities for geometric transformations
 *
 * Provides validation functions to ensure transformations preserve:
 * - Geometric precision (no cumulative drift)
 * - Arc direction (CCW/CW)
 * - Invariant properties (length, radius, sweep angle)
 *
 * Manufacturing context: CNC machines require sub-millimeter precision.
 * Cumulative drift from repeated transforms corrupts toolpaths.
 */

#include "geometry/Point2D.h"
#include "geometry/Line2D.h"
#include "geometry/Arc2D.h"
#include "geometry/Ellipse2D.h"
#include "geometry/GeometryConstants.h"

#include <string>
#include <functional>

namespace OwnCAD {
namespace Geometry {

/**
 * @brief Result of a validation check
 */
struct ValidationResult {
    bool passed;              ///< True if validation passed
    double maxDeviation;      ///< Maximum coordinate/value difference found
    std::string failureReason; ///< Empty if passed, describes failure otherwise

    /// Create a passing result
    static ValidationResult pass(double deviation = 0.0) {
        return {true, deviation, ""};
    }

    /// Create a failing result
    static ValidationResult fail(double deviation, const std::string& reason) {
        return {false, deviation, reason};
    }
};

/**
 * @brief Utility class for validating transformation precision
 *
 * All methods are static. No instance required.
 *
 * Typical usage:
 * @code
 *   auto result = TransformValidator::comparePoints(expected, actual);
 *   if (!result.passed) {
 *       // Handle precision loss
 *   }
 * @endcode
 */
class TransformValidator {
public:
    // =========================================================================
    // Point Comparison
    // =========================================================================

    /**
     * @brief Compare two points within tolerance
     * @param expected Expected point value
     * @param actual Actual point value after transform
     * @param tolerance Maximum allowed deviation (default: GEOMETRY_EPSILON)
     * @return ValidationResult with pass/fail and deviation info
     */
    static ValidationResult comparePoints(
        const Point2D& expected,
        const Point2D& actual,
        double tolerance = GEOMETRY_EPSILON
    );

    // =========================================================================
    // Line Comparison
    // =========================================================================

    /**
     * @brief Compare two lines within tolerance
     * @param expected Expected line
     * @param actual Actual line after transform
     * @param tolerance Maximum allowed deviation per coordinate
     * @return ValidationResult with pass/fail and max deviation
     */
    static ValidationResult compareLines(
        const Line2D& expected,
        const Line2D& actual,
        double tolerance = GEOMETRY_EPSILON
    );

    /**
     * @brief Validate that line length is preserved
     * @param original Original line
     * @param transformed Transformed line
     * @param tolerance Maximum allowed length difference
     * @return ValidationResult
     */
    static ValidationResult validateLineLength(
        const Line2D& original,
        const Line2D& transformed,
        double tolerance = GEOMETRY_EPSILON
    );

    // =========================================================================
    // Arc Comparison
    // =========================================================================

    /**
     * @brief Compare two arcs within tolerance (includes direction check)
     * @param expected Expected arc
     * @param actual Actual arc after transform
     * @param tolerance Maximum allowed deviation
     * @return ValidationResult with pass/fail and deviation info
     *
     * Validates:
     * - Center point position
     * - Radius equality
     * - Start/end angles (accounting for rotation offset)
     * - Direction (CCW/CW) - CRITICAL for CNC toolpaths
     */
    static ValidationResult compareArcs(
        const Arc2D& expected,
        const Arc2D& actual,
        double tolerance = GEOMETRY_EPSILON
    );

    /**
     * @brief Validate that arc direction is preserved
     * @param original Original arc
     * @param transformed Transformed arc
     * @return ValidationResult (fails if direction changed)
     */
    static ValidationResult validateArcDirection(
        const Arc2D& original,
        const Arc2D& transformed
    );

    /**
     * @brief Validate that arc radius is preserved
     * @param original Original arc
     * @param transformed Transformed arc
     * @param tolerance Maximum allowed radius difference
     * @return ValidationResult
     */
    static ValidationResult validateArcRadius(
        const Arc2D& original,
        const Arc2D& transformed,
        double tolerance = GEOMETRY_EPSILON
    );

    /**
     * @brief Validate that arc sweep angle is preserved
     * @param original Original arc
     * @param transformed Transformed arc
     * @param tolerance Maximum allowed sweep difference (radians)
     * @return ValidationResult
     */
    static ValidationResult validateArcSweep(
        const Arc2D& original,
        const Arc2D& transformed,
        double tolerance = GEOMETRY_EPSILON
    );

    // =========================================================================
    // Ellipse Comparison
    // =========================================================================

    /**
     * @brief Compare two ellipses within tolerance
     * @param expected Expected ellipse
     * @param actual Actual ellipse after transform
     * @param tolerance Maximum allowed deviation
     * @return ValidationResult
     */
    static ValidationResult compareEllipses(
        const Ellipse2D& expected,
        const Ellipse2D& actual,
        double tolerance = GEOMETRY_EPSILON
    );

    /**
     * @brief Validate that ellipse axis ratio is preserved
     * @param original Original ellipse
     * @param transformed Transformed ellipse
     * @param tolerance Maximum allowed ratio difference
     * @return ValidationResult
     */
    static ValidationResult validateEllipseAxes(
        const Ellipse2D& original,
        const Ellipse2D& transformed,
        double tolerance = GEOMETRY_EPSILON
    );

    // =========================================================================
    // Round-Trip Validation
    // =========================================================================

    /**
     * @brief Validate round-trip: apply transform, then inverse, compare to original
     * @param original Original point
     * @param transform Forward transformation function
     * @param inverse Inverse transformation function
     * @param tolerance Maximum allowed deviation after round-trip
     * @return ValidationResult
     *
     * Example:
     * @code
     *   auto result = TransformValidator::validatePointRoundTrip(
     *       point,
     *       [](const Point2D& p) { return GeometryMath::translate(p, 100, 100); },
     *       [](const Point2D& p) { return GeometryMath::translate(p, -100, -100); }
     *   );
     * @endcode
     */
    static ValidationResult validatePointRoundTrip(
        const Point2D& original,
        const std::function<Point2D(const Point2D&)>& transform,
        const std::function<Point2D(const Point2D&)>& inverse,
        double tolerance = GEOMETRY_EPSILON
    );

    // =========================================================================
    // Cumulative Drift Detection
    // =========================================================================

    /**
     * @brief Validate that repeated transforms don't accumulate drift
     * @param original Starting point
     * @param transform Transform to apply repeatedly
     * @param iterations Number of times to apply transform
     * @param expectedFinal Expected point after all iterations (often same as original for cyclic transforms)
     * @param tolerance Maximum allowed final deviation
     * @return ValidationResult with actual drift measured
     *
     * Example (1000 rotations of 0.36 degrees should return to start):
     * @code
     *   Point2D center(0, 0);
     *   double smallAngle = 2 * PI / 1000.0;
     *   auto result = TransformValidator::validateCumulativeDrift(
     *       Point2D(100, 0),
     *       [&](const Point2D& p) { return GeometryMath::rotate(p, center, smallAngle); },
     *       1000,
     *       Point2D(100, 0),  // Should return to start
     *       0.001             // Manufacturing tolerance: 1 micron
     *   );
     * @endcode
     */
    static ValidationResult validateCumulativeDrift(
        const Point2D& original,
        const std::function<Point2D(const Point2D&)>& transform,
        int iterations,
        const Point2D& expectedFinal,
        double tolerance = 0.001  // 1 micron default for manufacturing
    );

    // =========================================================================
    // Identity Transform Validation
    // =========================================================================

    /**
     * @brief Validate that 360-degree rotation returns identical point
     * @param point Point to rotate
     * @param center Rotation center
     * @param tolerance Maximum allowed deviation
     * @return ValidationResult
     */
    static ValidationResult validate360Rotation(
        const Point2D& point,
        const Point2D& center,
        double tolerance = GEOMETRY_EPSILON
    );

    /**
     * @brief Validate that 360-degree rotation returns identical line
     * @param line Line to rotate
     * @param center Rotation center
     * @param tolerance Maximum allowed deviation
     * @return ValidationResult
     */
    static ValidationResult validate360Rotation(
        const Line2D& line,
        const Point2D& center,
        double tolerance = GEOMETRY_EPSILON
    );

    /**
     * @brief Validate that 360-degree rotation returns identical arc
     * @param arc Arc to rotate
     * @param center Rotation center
     * @param tolerance Maximum allowed deviation
     * @return ValidationResult
     */
    static ValidationResult validate360Rotation(
        const Arc2D& arc,
        const Point2D& center,
        double tolerance = GEOMETRY_EPSILON
    );

private:
    TransformValidator() = delete;  // Static-only class
};

} // namespace Geometry
} // namespace OwnCAD
