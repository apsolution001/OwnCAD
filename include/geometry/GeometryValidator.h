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
    // Individual entity issues
    ZeroLengthLine,          ///< Line with length < MIN_LINE_LENGTH
    ZeroRadiusArc,           ///< Arc with radius < MIN_ARC_RADIUS
    InvalidArcAngle,         ///< Arc with invalid or degenerate angles
    DegenerateGeometry,      ///< Geometry that is numerically unstable
    NumericalInstability,    ///< Values near the tolerance boundary
    InvalidCoordinates,      ///< NaN or infinity in coordinates

    // Duplicate/overlap issues (pairwise)
    DuplicateLine,           ///< Two lines with identical endpoints
    OverlappingLines,        ///< Two collinear lines sharing a portion
    DuplicateArc,            ///< Two arcs identical in all parameters
    CoincidentArcs           ///< Arcs with same center/radius and angular overlap
};

/**
 * @brief Details about a single geometry issue
 */
struct GeometryIssue {
    GeometryIssueType type;
    size_t entityIndex;              ///< Index in the entity list (if applicable)
    std::string description;         ///< Human-readable description
    std::string entityHandle;        ///< Entity handle/ID (if available)
    size_t relatedEntityIndex;       ///< Index of related entity (for pairwise issues)
    std::string relatedEntityHandle; ///< Handle of related entity (for pairwise issues)

    GeometryIssue()
        : type(GeometryIssueType::ZeroLengthLine)
        , entityIndex(0)
        , relatedEntityIndex(0)
    {}

    GeometryIssue(GeometryIssueType t, size_t idx, const std::string& desc)
        : type(t)
        , entityIndex(idx)
        , description(desc)
        , relatedEntityIndex(0)
    {}

    GeometryIssue(GeometryIssueType t, size_t idx, const std::string& desc,
                  const std::string& handle)
        : type(t)
        , entityIndex(idx)
        , description(desc)
        , entityHandle(handle)
        , relatedEntityIndex(0)
    {}

    GeometryIssue(GeometryIssueType t, size_t idx1, size_t idx2,
                  const std::string& desc,
                  const std::string& handle1, const std::string& handle2)
        : type(t)
        , entityIndex(idx1)
        , description(desc)
        , entityHandle(handle1)
        , relatedEntityIndex(idx2)
        , relatedEntityHandle(handle2)
    {}
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

    // =========================================================================
    // DUPLICATE AND OVERLAP DETECTION
    // =========================================================================

    /**
     * @brief Check if two lines are duplicates (identical endpoints)
     * @param line1 First line
     * @param line2 Second line
     * @param tolerance Comparison tolerance
     * @return true if lines have identical endpoints (in either direction)
     *
     * Considers lines duplicate if:
     * - (line1.start == line2.start AND line1.end == line2.end) OR
     * - (line1.start == line2.end AND line1.end == line2.start)
     */
    static bool areLinesDuplicate(const Line2D& line1, const Line2D& line2,
                                  double tolerance) noexcept;

    /**
     * @brief Check if two lines overlap (collinear and share a portion)
     * @param line1 First line
     * @param line2 Second line
     * @param tolerance Comparison tolerance
     * @return true if lines are collinear and share at least one interior point
     *
     * Two lines overlap if:
     * 1. They are collinear (all 4 endpoints lie on the same infinite line)
     * 2. Their parametric intervals overlap (not just touching at endpoints)
     *
     * Note: Duplicate lines are a subset of overlapping lines.
     */
    static bool areLinesOverlapping(const Line2D& line1, const Line2D& line2,
                                    double tolerance) noexcept;

    /**
     * @brief Check if two arcs are duplicates (identical in all parameters)
     * @param arc1 First arc
     * @param arc2 Second arc
     * @param tolerance Comparison tolerance
     * @return true if arcs have identical center, radius, angles, and direction
     */
    static bool areArcsDuplicate(const Arc2D& arc1, const Arc2D& arc2,
                                 double tolerance) noexcept;

    /**
     * @brief Check if two arcs are coincident (same circle, overlapping angles)
     * @param arc1 First arc
     * @param arc2 Second arc
     * @param tolerance Comparison tolerance
     * @return true if arcs share the same circle and have overlapping angular ranges
     *
     * Two arcs are coincident if:
     * 1. Same center (within tolerance)
     * 2. Same radius (within tolerance)
     * 3. Angular ranges overlap (accounting for direction)
     *
     * Note: Duplicate arcs are a subset of coincident arcs.
     */
    static bool areArcsCoincident(const Arc2D& arc1, const Arc2D& arc2,
                                  double tolerance) noexcept;

    /**
     * @brief Validate entities with handle information for detailed reporting
     * @param entities Vector of geometry entities
     * @param handles Vector of entity handles (parallel to entities)
     * @param tolerance Tolerance for validation
     * @return Validation result with entity handles populated
     *
     * This method performs both individual validation and pairwise
     * duplicate/overlap detection, populating handles in issues.
     */
    static ValidationResult validateEntitiesWithHandles(
        const std::vector<std::variant<Line2D, Arc2D>>& entities,
        const std::vector<std::string>& handles,
        double tolerance
    ) noexcept;

    /**
     * @brief Detect all duplicate and overlapping geometry in a collection
     * @param entities Vector of geometry entities
     * @param handles Vector of entity handles (parallel to entities)
     * @param tolerance Tolerance for detection
     * @return ValidationResult containing only duplicate/overlap issues
     *
     * This is a focused method for duplicate detection only.
     * Does not check individual entity validity.
     */
    static ValidationResult detectDuplicates(
        const std::vector<std::variant<Line2D, Arc2D>>& entities,
        const std::vector<std::string>& handles,
        double tolerance
    ) noexcept;

private:
    /**
     * @brief Check if lines are collinear (all endpoints on same infinite line)
     */
    static bool areLinesCollinear(const Line2D& line1, const Line2D& line2,
                                  double tolerance) noexcept;

    /**
     * @brief Check if 1D intervals overlap
     * @param a1 Start of interval 1
     * @param a2 End of interval 1
     * @param b1 Start of interval 2
     * @param b2 End of interval 2
     * @return true if intervals share interior points (not just endpoints)
     */
    static bool intervalsOverlap(double a1, double a2, double b1, double b2,
                                 double tolerance) noexcept;

    /**
     * @brief Check if angular ranges overlap (accounting for wrap-around)
     * @param start1 Start angle of arc 1
     * @param end1 End angle of arc 1
     * @param ccw1 Direction of arc 1
     * @param start2 Start angle of arc 2
     * @param end2 End angle of arc 2
     * @param ccw2 Direction of arc 2
     * @param tolerance Angular tolerance
     * @return true if angular ranges overlap
     */
    static bool angularRangesOverlap(double start1, double end1, bool ccw1,
                                     double start2, double end2, bool ccw2,
                                     double tolerance) noexcept;
};

/**
 * @brief Convert issue type to human-readable string
 * @param type Issue type
 * @return String description
 */
std::string toString(GeometryIssueType type);

} // namespace Geometry
} // namespace OwnCAD
