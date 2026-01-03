#pragma once

#include "Line2D.h"
#include "Arc2D.h"
#include <vector>
#include <variant>
#include <string>

namespace OwnCAD {
namespace Geometry {

/**
 * @brief Types of geometry issues that can be detected
 */
enum class GeometryIssueType {
    ZeroLengthLine,          ///< Line with length < MIN_LINE_LENGTH
    ZeroRadiusArc,           ///< Arc with radius < MIN_ARC_RADIUS
    InvalidArcAngle,         ///< Arc with invalid or degenerate angles
    DegenerateGeometry,      ///< Geometry that is numerically unstable
    NumericalInstability,    ///< Values near the tolerance boundary
    InvalidCoordinates       ///< NaN or infinity in coordinates
};

/**
 * @brief Details about a single geometry issue
 */
struct GeometryIssue {
    GeometryIssueType type;
    size_t entityIndex;      ///< Index in the entity list (if applicable)
    std::string description; ///< Human-readable description
};

/**
 * @brief Result of geometry validation
 */
struct ValidationResult {
    bool isValid;                       ///< true if all geometry is valid
    std::vector<GeometryIssue> issues;  ///< List of detected issues

    /**
     * @brief Check if validation passed (no issues)
     */
    bool passed() const noexcept { return issues.empty(); }

    /**
     * @brief Get count of issues
     */
    size_t issueCount() const noexcept { return issues.size(); }

    /**
     * @brief Check if specific issue type exists
     */
    bool hasIssueType(GeometryIssueType type) const noexcept;

    /**
     * @brief Get all issues of a specific type
     */
    std::vector<GeometryIssue> getIssuesOfType(GeometryIssueType type) const;
};

/**
 * @brief Geometry validation system
 *
 * This class provides validation for individual entities and collections.
 * It is separate from the geometry classes themselves to allow:
 * - Batch validation after DXF import
 * - Background scanning after edits
 * - Application-level rule checking beyond structural validity
 *
 * Design principles:
 * - Non-destructive: Never modifies geometry
 * - Explicit: Returns detailed information about issues
 * - Deterministic: Same input always produces same results
 * - Fast: Optimized for scanning thousands of entities
 */
class GeometryValidator {
public:
    /**
     * @brief Validate a line
     * @param line Line to validate
     * @param tolerance Tolerance for validation (default: GEOMETRY_EPSILON)
     * @return Validation result
     */
    static ValidationResult validateLine(const Line2D& line, double tolerance) noexcept;

    /**
     * @brief Validate an arc
     * @param arc Arc to validate
     * @param tolerance Tolerance for validation (default: GEOMETRY_EPSILON)
     * @return Validation result
     */
    static ValidationResult validateArc(const Arc2D& arc, double tolerance) noexcept;

    /**
     * @brief Validate collection of entities
     * @param entities Vector of lines and arcs
     * @param tolerance Tolerance for validation
     * @return Validation result with all detected issues
     */
    static ValidationResult validateEntities(
        const std::vector<std::variant<Line2D, Arc2D>>& entities,
        double tolerance
    ) noexcept;

    /**
     * @brief Check if line is zero-length
     * @param line Line to check
     * @param tolerance Minimum valid length
     * @return true if length < tolerance
     */
    static bool isZeroLength(const Line2D& line, double tolerance) noexcept;

    /**
     * @brief Check if arc has zero radius
     * @param arc Arc to check
     * @param tolerance Minimum valid radius
     * @return true if radius < tolerance
     */
    static bool isZeroRadius(const Arc2D& arc, double tolerance) noexcept;

    /**
     * @brief Check if arc has valid angle configuration
     * @param arc Arc to check
     * @param tolerance Minimum valid sweep
     * @return true if sweep angle >= tolerance
     */
    static bool hasValidAngles(const Arc2D& arc, double tolerance) noexcept;

    /**
     * @brief Check if geometry is numerically stable
     *
     * Detects values that are very close to tolerance boundaries,
     * which may cause inconsistent behavior.
     *
     * @param line Line to check
     * @param tolerance Stability threshold
     * @return true if geometry is well away from tolerance boundaries
     */
    static bool isNumericallyStable(const Line2D& line, double tolerance) noexcept;

    /**
     * @brief Check if geometry is numerically stable
     * @param arc Arc to check
     * @param tolerance Stability threshold
     * @return true if geometry is well away from tolerance boundaries
     */
    static bool isNumericallyStable(const Arc2D& arc, double tolerance) noexcept;
};

/**
 * @brief Convert issue type to human-readable string
 * @param type Issue type
 * @return String description
 */
std::string toString(GeometryIssueType type);

} // namespace Geometry
} // namespace OwnCAD
