#include "geometry/GeometryValidator.h"
#include "geometry/GeometryConstants.h"
#include "geometry/GeometryMath.h"

namespace OwnCAD {
namespace Geometry {

// ============================================================================
// VALIDATION RESULT METHODS
// ============================================================================

bool ValidationResult::hasIssueType(GeometryIssueType type) const noexcept {
    for (const auto& issue : issues) {
        if (issue.type == type) {
            return true;
        }
    }
    return false;
}

std::vector<GeometryIssue> ValidationResult::getIssuesOfType(GeometryIssueType type) const {
    std::vector<GeometryIssue> result;
    for (const auto& issue : issues) {
        if (issue.type == type) {
            result.push_back(issue);
        }
    }
    return result;
}

// ============================================================================
// VALIDATION METHODS
// ============================================================================

ValidationResult GeometryValidator::validateLine(const Line2D& line, double tolerance) noexcept {
    ValidationResult result;
    result.isValid = true;

    // Check zero-length
    if (isZeroLength(line, tolerance)) {
        result.isValid = false;
        result.issues.push_back({
            GeometryIssueType::ZeroLengthLine,
            0,
            "Line has zero or near-zero length"
        });
    }

    // Check numerical stability
    if (!isNumericallyStable(line, tolerance)) {
        result.issues.push_back({
            GeometryIssueType::NumericalInstability,
            0,
            "Line length is close to tolerance boundary (numerically unstable)"
        });
    }

    // Check for invalid coordinates
    if (!line.start().isValid() || !line.end().isValid()) {
        result.isValid = false;
        result.issues.push_back({
            GeometryIssueType::InvalidCoordinates,
            0,
            "Line contains invalid coordinates (NaN or infinity)"
        });
    }

    return result;
}

ValidationResult GeometryValidator::validateArc(const Arc2D& arc, double tolerance) noexcept {
    ValidationResult result;
    result.isValid = true;

    // Check zero-radius
    if (isZeroRadius(arc, tolerance)) {
        result.isValid = false;
        result.issues.push_back({
            GeometryIssueType::ZeroRadiusArc,
            0,
            "Arc has zero or near-zero radius"
        });
    }

    // Check valid angles
    if (!hasValidAngles(arc, tolerance)) {
        result.isValid = false;
        result.issues.push_back({
            GeometryIssueType::InvalidArcAngle,
            0,
            "Arc has invalid or degenerate angle configuration"
        });
    }

    // Check numerical stability
    if (!isNumericallyStable(arc, tolerance)) {
        result.issues.push_back({
            GeometryIssueType::NumericalInstability,
            0,
            "Arc parameters are close to tolerance boundary (numerically unstable)"
        });
    }

    // Check for invalid coordinates
    if (!arc.center().isValid()) {
        result.isValid = false;
        result.issues.push_back({
            GeometryIssueType::InvalidCoordinates,
            0,
            "Arc contains invalid center coordinates (NaN or infinity)"
        });
    }

    return result;
}

ValidationResult GeometryValidator::validateEntities(
    const std::vector<std::variant<Line2D, Arc2D>>& entities,
    double tolerance
) noexcept {
    ValidationResult result;
    result.isValid = true;

    for (size_t i = 0; i < entities.size(); ++i) {
        std::visit([&](auto&& entity) {
            using T = std::decay_t<decltype(entity)>;
            ValidationResult entityResult;

            if constexpr (std::is_same_v<T, Line2D>) {
                entityResult = validateLine(entity, tolerance);
            } else if constexpr (std::is_same_v<T, Arc2D>) {
                entityResult = validateArc(entity, tolerance);
            }

            if (!entityResult.isValid) {
                result.isValid = false;
            }

            // Add entity-specific issues with index
            for (auto issue : entityResult.issues) {
                issue.entityIndex = i;
                result.issues.push_back(issue);
            }
        }, entities[i]);
    }

    return result;
}

// ============================================================================
// SPECIFIC CHECKS
// ============================================================================

bool GeometryValidator::isZeroLength(const Line2D& line, double tolerance) noexcept {
    return line.length() < tolerance;
}

bool GeometryValidator::isZeroRadius(const Arc2D& arc, double tolerance) noexcept {
    return arc.radius() < tolerance;
}

bool GeometryValidator::hasValidAngles(const Arc2D& arc, double tolerance) noexcept {
    const double sweep = arc.sweepAngle();
    return sweep >= tolerance && std::isfinite(sweep);
}

bool GeometryValidator::isNumericallyStable(const Line2D& line, double tolerance) noexcept {
    const double length = line.length();
    // Consider unstable if within 10x tolerance
    return length > tolerance * 10.0;
}

bool GeometryValidator::isNumericallyStable(const Arc2D& arc, double tolerance) noexcept {
    const double radius = arc.radius();
    const double sweep = arc.sweepAngle();
    // Consider unstable if within 10x tolerance
    return radius > tolerance * 10.0 && sweep > tolerance * 10.0;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

std::string toString(GeometryIssueType type) {
    switch (type) {
        case GeometryIssueType::ZeroLengthLine:
            return "Zero-length line";
        case GeometryIssueType::ZeroRadiusArc:
            return "Zero-radius arc";
        case GeometryIssueType::InvalidArcAngle:
            return "Invalid arc angle";
        case GeometryIssueType::DegenerateGeometry:
            return "Degenerate geometry";
        case GeometryIssueType::NumericalInstability:
            return "Numerical instability";
        case GeometryIssueType::InvalidCoordinates:
            return "Invalid coordinates";
        default:
            return "Unknown issue";
    }
}

} // namespace Geometry
} // namespace OwnCAD
