#include "geometry/TransformValidator.h"
#include "geometry/GeometryMath.h"

#include <cmath>
#include <algorithm>
#include <sstream>

namespace OwnCAD {
namespace Geometry {

// =============================================================================
// Point Comparison
// =============================================================================

ValidationResult TransformValidator::comparePoints(
    const Point2D& expected,
    const Point2D& actual,
    double tolerance
) {
    double dx = std::abs(expected.x() - actual.x());
    double dy = std::abs(expected.y() - actual.y());
    double maxDev = std::max(dx, dy);

    if (maxDev < tolerance) {
        return ValidationResult::pass(maxDev);
    }

    std::ostringstream oss;
    oss << "Point deviation exceeds tolerance: "
        << "expected (" << expected.x() << ", " << expected.y() << "), "
        << "actual (" << actual.x() << ", " << actual.y() << "), "
        << "deviation: " << maxDev << " > " << tolerance;

    return ValidationResult::fail(maxDev, oss.str());
}

// =============================================================================
// Line Comparison
// =============================================================================

ValidationResult TransformValidator::compareLines(
    const Line2D& expected,
    const Line2D& actual,
    double tolerance
) {
    auto startResult = comparePoints(expected.start(), actual.start(), tolerance);
    auto endResult = comparePoints(expected.end(), actual.end(), tolerance);

    double maxDev = std::max(startResult.maxDeviation, endResult.maxDeviation);

    if (startResult.passed && endResult.passed) {
        return ValidationResult::pass(maxDev);
    }

    std::ostringstream oss;
    oss << "Line deviation exceeds tolerance: ";
    if (!startResult.passed) {
        oss << "start point failed (" << startResult.failureReason << ")";
    }
    if (!endResult.passed) {
        if (!startResult.passed) oss << "; ";
        oss << "end point failed (" << endResult.failureReason << ")";
    }

    return ValidationResult::fail(maxDev, oss.str());
}

ValidationResult TransformValidator::validateLineLength(
    const Line2D& original,
    const Line2D& transformed,
    double tolerance
) {
    double originalLen = original.length();
    double transformedLen = transformed.length();
    double diff = std::abs(originalLen - transformedLen);

    if (diff < tolerance) {
        return ValidationResult::pass(diff);
    }

    std::ostringstream oss;
    oss << "Line length changed: original " << originalLen
        << ", transformed " << transformedLen
        << ", difference " << diff << " > " << tolerance;

    return ValidationResult::fail(diff, oss.str());
}

// =============================================================================
// Arc Comparison
// =============================================================================

ValidationResult TransformValidator::compareArcs(
    const Arc2D& expected,
    const Arc2D& actual,
    double tolerance
) {
    // Check center point
    auto centerResult = comparePoints(expected.center(), actual.center(), tolerance);
    if (!centerResult.passed) {
        return ValidationResult::fail(
            centerResult.maxDeviation,
            "Arc center mismatch: " + centerResult.failureReason
        );
    }

    // Check radius
    double radiusDiff = std::abs(expected.radius() - actual.radius());
    if (radiusDiff >= tolerance) {
        std::ostringstream oss;
        oss << "Arc radius mismatch: expected " << expected.radius()
            << ", actual " << actual.radius()
            << ", difference " << radiusDiff;
        return ValidationResult::fail(radiusDiff, oss.str());
    }

    // Check direction (CRITICAL for CNC)
    if (expected.isCounterClockwise() != actual.isCounterClockwise()) {
        return ValidationResult::fail(
            0.0,
            "Arc direction changed: expected "
            + std::string(expected.isCounterClockwise() ? "CCW" : "CW")
            + ", actual "
            + std::string(actual.isCounterClockwise() ? "CCW" : "CW")
        );
    }

    // Check sweep angle (more robust than comparing start/end angles directly)
    double sweepDiff = std::abs(expected.sweepAngle() - actual.sweepAngle());
    if (sweepDiff >= tolerance) {
        std::ostringstream oss;
        oss << "Arc sweep angle mismatch: expected " << expected.sweepAngle()
            << " rad, actual " << actual.sweepAngle()
            << " rad, difference " << sweepDiff;
        return ValidationResult::fail(sweepDiff, oss.str());
    }

    double maxDev = std::max({centerResult.maxDeviation, radiusDiff, sweepDiff});
    return ValidationResult::pass(maxDev);
}

ValidationResult TransformValidator::validateArcDirection(
    const Arc2D& original,
    const Arc2D& transformed
) {
    if (original.isCounterClockwise() == transformed.isCounterClockwise()) {
        return ValidationResult::pass(0.0);
    }

    return ValidationResult::fail(
        0.0,
        "Arc direction changed from "
        + std::string(original.isCounterClockwise() ? "CCW" : "CW")
        + " to "
        + std::string(transformed.isCounterClockwise() ? "CCW" : "CW")
    );
}

ValidationResult TransformValidator::validateArcRadius(
    const Arc2D& original,
    const Arc2D& transformed,
    double tolerance
) {
    double diff = std::abs(original.radius() - transformed.radius());

    if (diff < tolerance) {
        return ValidationResult::pass(diff);
    }

    std::ostringstream oss;
    oss << "Arc radius changed: original " << original.radius()
        << ", transformed " << transformed.radius()
        << ", difference " << diff << " > " << tolerance;

    return ValidationResult::fail(diff, oss.str());
}

ValidationResult TransformValidator::validateArcSweep(
    const Arc2D& original,
    const Arc2D& transformed,
    double tolerance
) {
    double diff = std::abs(original.sweepAngle() - transformed.sweepAngle());

    if (diff < tolerance) {
        return ValidationResult::pass(diff);
    }

    std::ostringstream oss;
    oss << "Arc sweep changed: original " << original.sweepAngle()
        << " rad, transformed " << transformed.sweepAngle()
        << " rad, difference " << diff << " > " << tolerance;

    return ValidationResult::fail(diff, oss.str());
}

// =============================================================================
// Ellipse Comparison
// =============================================================================

ValidationResult TransformValidator::compareEllipses(
    const Ellipse2D& expected,
    const Ellipse2D& actual,
    double tolerance
) {
    // Check center
    auto centerResult = comparePoints(expected.center(), actual.center(), tolerance);
    if (!centerResult.passed) {
        return ValidationResult::fail(
            centerResult.maxDeviation,
            "Ellipse center mismatch: " + centerResult.failureReason
        );
    }

    // Check major axis length
    double majorDiff = std::abs(expected.majorAxisLength() - actual.majorAxisLength());
    if (majorDiff >= tolerance) {
        std::ostringstream oss;
        oss << "Ellipse major axis mismatch: expected " << expected.majorAxisLength()
            << ", actual " << actual.majorAxisLength();
        return ValidationResult::fail(majorDiff, oss.str());
    }

    // Check minor axis length
    double minorDiff = std::abs(expected.minorAxisLength() - actual.minorAxisLength());
    if (minorDiff >= tolerance) {
        std::ostringstream oss;
        oss << "Ellipse minor axis mismatch: expected " << expected.minorAxisLength()
            << ", actual " << actual.minorAxisLength();
        return ValidationResult::fail(minorDiff, oss.str());
    }

    double maxDev = std::max({centerResult.maxDeviation, majorDiff, minorDiff});
    return ValidationResult::pass(maxDev);
}

ValidationResult TransformValidator::validateEllipseAxes(
    const Ellipse2D& original,
    const Ellipse2D& transformed,
    double tolerance
) {
    double majorDiff = std::abs(original.majorAxisLength() - transformed.majorAxisLength());
    double minorDiff = std::abs(original.minorAxisLength() - transformed.minorAxisLength());
    double ratioDiff = std::abs(original.minorAxisRatio() - transformed.minorAxisRatio());

    double maxDiff = std::max({majorDiff, minorDiff, ratioDiff});

    if (maxDiff < tolerance) {
        return ValidationResult::pass(maxDiff);
    }

    std::ostringstream oss;
    oss << "Ellipse axes changed: "
        << "major diff=" << majorDiff
        << ", minor diff=" << minorDiff
        << ", ratio diff=" << ratioDiff;

    return ValidationResult::fail(maxDiff, oss.str());
}

// =============================================================================
// Round-Trip Validation
// =============================================================================

ValidationResult TransformValidator::validatePointRoundTrip(
    const Point2D& original,
    const std::function<Point2D(const Point2D&)>& transform,
    const std::function<Point2D(const Point2D&)>& inverse,
    double tolerance
) {
    Point2D transformed = transform(original);
    Point2D roundTripped = inverse(transformed);

    return comparePoints(original, roundTripped, tolerance);
}

// =============================================================================
// Cumulative Drift Detection
// =============================================================================

ValidationResult TransformValidator::validateCumulativeDrift(
    const Point2D& original,
    const std::function<Point2D(const Point2D&)>& transform,
    int iterations,
    const Point2D& expectedFinal,
    double tolerance
) {
    Point2D current = original;

    for (int i = 0; i < iterations; ++i) {
        current = transform(current);
    }

    double drift = current.distanceTo(expectedFinal);

    if (drift < tolerance) {
        return ValidationResult::pass(drift);
    }

    std::ostringstream oss;
    oss << "Cumulative drift after " << iterations << " iterations: "
        << "expected (" << expectedFinal.x() << ", " << expectedFinal.y() << "), "
        << "actual (" << current.x() << ", " << current.y() << "), "
        << "drift: " << drift << " > " << tolerance;

    return ValidationResult::fail(drift, oss.str());
}

// =============================================================================
// 360-Degree Rotation Validation
// =============================================================================

ValidationResult TransformValidator::validate360Rotation(
    const Point2D& point,
    const Point2D& center,
    double tolerance
) {
    Point2D rotated = GeometryMath::rotate(point, center, TWO_PI);
    return comparePoints(point, rotated, tolerance);
}

ValidationResult TransformValidator::validate360Rotation(
    const Line2D& line,
    const Point2D& center,
    double tolerance
) {
    auto rotated = GeometryMath::rotate(line, center, TWO_PI);

    if (!rotated.has_value()) {
        return ValidationResult::fail(
            0.0,
            "360-degree rotation produced invalid line"
        );
    }

    return compareLines(line, *rotated, tolerance);
}

ValidationResult TransformValidator::validate360Rotation(
    const Arc2D& arc,
    const Point2D& center,
    double tolerance
) {
    auto rotated = GeometryMath::rotate(arc, center, TWO_PI);

    if (!rotated.has_value()) {
        return ValidationResult::fail(
            0.0,
            "360-degree rotation produced invalid arc"
        );
    }

    // Check direction first (most critical)
    auto dirResult = validateArcDirection(arc, *rotated);
    if (!dirResult.passed) {
        return dirResult;
    }

    // Check radius
    auto radiusResult = validateArcRadius(arc, *rotated, tolerance);
    if (!radiusResult.passed) {
        return radiusResult;
    }

    // Check sweep
    auto sweepResult = validateArcSweep(arc, *rotated, tolerance);
    if (!sweepResult.passed) {
        return sweepResult;
    }

    // Check center position
    return comparePoints(arc.center(), rotated->center(), tolerance);
}

} // namespace Geometry
} // namespace OwnCAD
