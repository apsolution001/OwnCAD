#include "geometry/GeometryValidator.h"
#include "geometry/GeometryConstants.h"
#include "geometry/GeometryMath.h"
#include <algorithm>
#include <cmath>

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
        case GeometryIssueType::DuplicateLine:
            return "Duplicate line";
        case GeometryIssueType::OverlappingLines:
            return "Overlapping lines";
        case GeometryIssueType::DuplicateArc:
            return "Duplicate arc";
        case GeometryIssueType::CoincidentArcs:
            return "Coincident arcs";
        default:
            return "Unknown issue";
    }
}

// ============================================================================
// DUPLICATE AND OVERLAP DETECTION - LINES
// ============================================================================

bool GeometryValidator::areLinesDuplicate(const Line2D& line1, const Line2D& line2,
                                          double tolerance) noexcept {
    // Check forward match: start1==start2 AND end1==end2
    bool forwardMatch =
        line1.start().isEqual(line2.start(), tolerance) &&
        line1.end().isEqual(line2.end(), tolerance);

    // Check reverse match: start1==end2 AND end1==start2
    bool reverseMatch =
        line1.start().isEqual(line2.end(), tolerance) &&
        line1.end().isEqual(line2.start(), tolerance);

    return forwardMatch || reverseMatch;
}

bool GeometryValidator::areLinesCollinear(const Line2D& line1, const Line2D& line2,
                                          double tolerance) noexcept {
    // All 4 endpoints must lie on the same infinite line.
    // We check if line2's endpoints are within tolerance distance of line1's infinite line.

    double dist1 = GeometryMath::distancePointToLine(line2.start(), line1);
    double dist2 = GeometryMath::distancePointToLine(line2.end(), line1);

    return dist1 < tolerance && dist2 < tolerance;
}

bool GeometryValidator::intervalsOverlap(double a1, double a2, double b1, double b2,
                                         double tolerance) noexcept {
    // Normalize intervals so a1 <= a2 and b1 <= b2
    if (a1 > a2) std::swap(a1, a2);
    if (b1 > b2) std::swap(b1, b2);

    // Intervals overlap if: max(a1, b1) < min(a2, b2)
    // Using tolerance: intervals must share interior points, not just touch
    double overlapStart = std::max(a1, b1);
    double overlapEnd = std::min(a2, b2);

    // They overlap if the overlap region has positive length (beyond tolerance)
    return (overlapEnd - overlapStart) > tolerance;
}

bool GeometryValidator::areLinesOverlapping(const Line2D& line1, const Line2D& line2,
                                            double tolerance) noexcept {
    // Step 1: Check if lines are collinear
    if (!areLinesCollinear(line1, line2, tolerance)) {
        return false;
    }

    // Step 2: Project both lines onto a common axis to check parametric overlap.
    // Use the longer line as reference to get better numerical stability.

    // Determine projection axis (use line1's direction)
    double dx = line1.end().x() - line1.start().x();
    double dy = line1.end().y() - line1.start().y();
    double len1Sq = dx * dx + dy * dy;

    if (len1Sq < tolerance * tolerance) {
        // line1 is essentially a point; can't determine direction
        // Check if line2 contains line1's start point
        return line2.containsPoint(line1.start(), tolerance);
    }

    // Project all 4 points onto line1's direction (parametric t values)
    // t = dot(P - line1.start, direction) / |direction|^2

    auto projectToLine1 = [&](const Point2D& p) -> double {
        double px = p.x() - line1.start().x();
        double py = p.y() - line1.start().y();
        return (px * dx + py * dy) / len1Sq;
    };

    double t1_start = 0.0;  // line1.start() projects to t=0
    double t1_end = 1.0;    // line1.end() projects to t=1
    double t2_start = projectToLine1(line2.start());
    double t2_end = projectToLine1(line2.end());

    // Check if parametric intervals overlap
    return intervalsOverlap(t1_start, t1_end, t2_start, t2_end, tolerance);
}

// ============================================================================
// DUPLICATE AND OVERLAP DETECTION - ARCS
// ============================================================================

bool GeometryValidator::areArcsDuplicate(const Arc2D& arc1, const Arc2D& arc2,
                                         double tolerance) noexcept {
    // Check center
    if (!arc1.center().isEqual(arc2.center(), tolerance)) {
        return false;
    }

    // Check radius
    if (!GeometryMath::areEqual(arc1.radius(), arc2.radius(), tolerance)) {
        return false;
    }

    // Check direction
    if (arc1.isCounterClockwise() != arc2.isCounterClockwise()) {
        return false;
    }

    // Check angles (normalized comparison)
    double angleTol = tolerance / arc1.radius();  // Convert linear to angular tolerance
    if (angleTol < 1e-12) angleTol = 1e-12;       // Minimum angular tolerance

    bool startMatch = GeometryMath::areEqual(
        GeometryMath::normalizeAngle(arc1.startAngle()),
        GeometryMath::normalizeAngle(arc2.startAngle()),
        angleTol
    );

    bool endMatch = GeometryMath::areEqual(
        GeometryMath::normalizeAngle(arc1.endAngle()),
        GeometryMath::normalizeAngle(arc2.endAngle()),
        angleTol
    );

    return startMatch && endMatch;
}

bool GeometryValidator::angularRangesOverlap(double start1, double end1, bool ccw1,
                                             double start2, double end2, bool ccw2,
                                             double tolerance) noexcept {
    // Normalize all angles to [0, 2π)
    start1 = GeometryMath::normalizeAngle(start1);
    end1 = GeometryMath::normalizeAngle(end1);
    start2 = GeometryMath::normalizeAngle(start2);
    end2 = GeometryMath::normalizeAngle(end2);

    // Helper: check if angle is within arc's range
    auto isAngleInArc = [tolerance](double angle, double start, double end, bool ccw) -> bool {
        angle = GeometryMath::normalizeAngle(angle);
        return GeometryMath::isAngleBetween(angle, start, end, ccw);
    };

    // Check if any endpoint of arc2 is within arc1's range
    if (isAngleInArc(start2, start1, end1, ccw1) ||
        isAngleInArc(end2, start1, end1, ccw1)) {
        return true;
    }

    // Check if any endpoint of arc1 is within arc2's range
    if (isAngleInArc(start1, start2, end2, ccw2) ||
        isAngleInArc(end1, start2, end2, ccw2)) {
        return true;
    }

    // Check for complete containment (one arc entirely within the other)
    // This is already covered by the above checks since we check both directions

    return false;
}

bool GeometryValidator::areArcsCoincident(const Arc2D& arc1, const Arc2D& arc2,
                                          double tolerance) noexcept {
    // Step 1: Check if arcs share the same base circle (center and radius)
    if (!arc1.center().isEqual(arc2.center(), tolerance)) {
        return false;
    }

    if (!GeometryMath::areEqual(arc1.radius(), arc2.radius(), tolerance)) {
        return false;
    }

    // Step 2: Check if angular ranges overlap
    double angleTol = tolerance / arc1.radius();
    if (angleTol < 1e-12) angleTol = 1e-12;

    return angularRangesOverlap(
        arc1.startAngle(), arc1.endAngle(), arc1.isCounterClockwise(),
        arc2.startAngle(), arc2.endAngle(), arc2.isCounterClockwise(),
        angleTol
    );
}

// ============================================================================
// COLLECTION VALIDATION WITH HANDLES
// ============================================================================

ValidationResult GeometryValidator::detectDuplicates(
    const std::vector<std::variant<Line2D, Arc2D>>& entities,
    const std::vector<std::string>& handles,
    double tolerance
) noexcept {
    ValidationResult result;
    result.isValid = true;

    const size_t n = entities.size();
    if (n < 2) {
        return result;  // Need at least 2 entities for duplicates
    }

    // Ensure handles vector matches entities (or use empty strings)
    auto getHandle = [&handles, n](size_t idx) -> std::string {
        if (idx < handles.size()) {
            return handles[idx];
        }
        return "";
    };

    // Pairwise comparison O(n²) - acceptable for typical CAD document sizes
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            // Check Line-Line pairs
            if (std::holds_alternative<Line2D>(entities[i]) &&
                std::holds_alternative<Line2D>(entities[j])) {

                const auto& line1 = std::get<Line2D>(entities[i]);
                const auto& line2 = std::get<Line2D>(entities[j]);

                if (areLinesDuplicate(line1, line2, tolerance)) {
                    result.isValid = false;
                    result.issues.emplace_back(
                        GeometryIssueType::DuplicateLine,
                        i, j,
                        "Duplicate line detected (identical endpoints)",
                        getHandle(i), getHandle(j)
                    );
                } else if (areLinesOverlapping(line1, line2, tolerance)) {
                    result.isValid = false;
                    result.issues.emplace_back(
                        GeometryIssueType::OverlappingLines,
                        i, j,
                        "Overlapping lines detected (collinear with shared portion)",
                        getHandle(i), getHandle(j)
                    );
                }
            }

            // Check Arc-Arc pairs
            if (std::holds_alternative<Arc2D>(entities[i]) &&
                std::holds_alternative<Arc2D>(entities[j])) {

                const auto& arc1 = std::get<Arc2D>(entities[i]);
                const auto& arc2 = std::get<Arc2D>(entities[j]);

                if (areArcsDuplicate(arc1, arc2, tolerance)) {
                    result.isValid = false;
                    result.issues.emplace_back(
                        GeometryIssueType::DuplicateArc,
                        i, j,
                        "Duplicate arc detected (identical parameters)",
                        getHandle(i), getHandle(j)
                    );
                } else if (areArcsCoincident(arc1, arc2, tolerance)) {
                    result.isValid = false;
                    result.issues.emplace_back(
                        GeometryIssueType::CoincidentArcs,
                        i, j,
                        "Coincident arcs detected (same circle with angular overlap)",
                        getHandle(i), getHandle(j)
                    );
                }
            }
        }
    }

    return result;
}

ValidationResult GeometryValidator::validateEntitiesWithHandles(
    const std::vector<std::variant<Line2D, Arc2D>>& entities,
    const std::vector<std::string>& handles,
    double tolerance
) noexcept {
    ValidationResult result;
    result.isValid = true;

    auto getHandle = [&handles](size_t idx) -> std::string {
        if (idx < handles.size()) {
            return handles[idx];
        }
        return "";
    };

    // Step 1: Validate individual entities
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

            // Add entity-specific issues with index and handle
            for (auto issue : entityResult.issues) {
                issue.entityIndex = i;
                issue.entityHandle = getHandle(i);
                result.issues.push_back(issue);
            }
        }, entities[i]);
    }

    // Step 2: Detect duplicates and overlaps
    ValidationResult duplicateResult = detectDuplicates(entities, handles, tolerance);
    if (!duplicateResult.isValid) {
        result.isValid = false;
    }
    for (const auto& issue : duplicateResult.issues) {
        result.issues.push_back(issue);
    }

    return result;
}

} // namespace Geometry
} // namespace OwnCAD
