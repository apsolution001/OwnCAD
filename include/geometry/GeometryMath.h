#pragma once

#include "Point2D.h"
#include "Line2D.h"
#include "Arc2D.h"
#include "Ellipse2D.h"
#include <optional>
#include <vector>

namespace OwnCAD {
namespace Geometry {
namespace GeometryMath {

// ============================================================================
// DISTANCE CALCULATIONS
// ============================================================================

/**
 * @brief Calculate distance between two points
 * @param p1 First point
 * @param p2 Second point
 * @return Euclidean distance
 */
double distance(const Point2D& p1, const Point2D& p2) noexcept;

/**
 * @brief Calculate squared distance between two points (faster)
 * @param p1 First point
 * @param p2 Second point
 * @return Squared Euclidean distance (avoids sqrt)
 */
double distanceSquared(const Point2D& p1, const Point2D& p2) noexcept;

/**
 * @brief Calculate perpendicular distance from point to infinite line
 * @param point Point to measure from
 * @param line Line to measure to
 * @return Perpendicular distance to line (extended infinitely)
 */
double distancePointToLine(const Point2D& point, const Line2D& line) noexcept;

/**
 * @brief Calculate distance from point to line segment
 *
 * Unlike distancePointToLine, this measures to the actual segment,
 * not the infinite line. Returns distance to nearest point on segment.
 *
 * @param point Point to measure from
 * @param line Line segment to measure to
 * @return Distance to nearest point on segment
 */
double distancePointToSegment(const Point2D& point, const Line2D& line) noexcept;

/**
 * @brief Calculate distance from point to arc
 * @param point Point to measure from
 * @param arc Arc to measure to
 * @return Distance to nearest point on arc
 */
double distancePointToArc(const Point2D& point, const Arc2D& arc) noexcept;

/**
 * @brief Calculate distance from point to ellipse
 * @param point Point to measure from
 * @param ellipse Ellipse to measure to
 * @return Distance to nearest point on ellipse
 *
 * Uses sampling approach suitable for hit testing (not analytical).
 * Samples points along the ellipse to find the closest.
 */
double distancePointToEllipse(const Point2D& point, const Ellipse2D& ellipse) noexcept;

// ============================================================================
// ANGLE UTILITIES
// ============================================================================

/**
 * @brief Normalize angle to [0, 2π) range
 * @param radians Angle in radians (any value)
 * @return Equivalent angle in [0, 2π)
 */
double normalizeAngle(double radians) noexcept;

/**
 * @brief Normalize angle to [-π, π) range
 * @param radians Angle in radians (any value)
 * @return Equivalent angle in [-π, π)
 */
double normalizeAngleSigned(double radians) noexcept;

/**
 * @brief Calculate angle from one point to another
 * @param from Starting point
 * @param to Ending point
 * @return Angle in radians [0, 2π)
 */
double angleBetweenPoints(const Point2D& from, const Point2D& to) noexcept;

/**
 * @brief Calculate smallest difference between two angles
 * @param angle1 First angle (radians)
 * @param angle2 Second angle (radians)
 * @return Difference in range [-π, π]
 */
double angleDifference(double angle1, double angle2) noexcept;

/**
 * @brief Check if angle is between two other angles
 * @param angle Angle to test
 * @param start Start angle
 * @param end End angle
 * @param ccw Counter-clockwise direction
 * @return true if angle falls in the arc from start to end
 */
bool isAngleBetween(double angle, double start, double end, bool ccw) noexcept;

// ============================================================================
// ARC CALCULATIONS
// ============================================================================

/**
 * @brief Calculate arc length
 * @param radius Arc radius
 * @param sweepAngle Sweep angle in radians
 * @return Arc length
 */
double arcLength(double radius, double sweepAngle) noexcept;

/**
 * @brief Calculate sweep angle from start to end
 * @param startAngle Start angle (radians)
 * @param endAngle End angle (radians)
 * @param ccw Counter-clockwise direction
 * @return Sweep angle (always positive)
 */
double sweepAngle(double startAngle, double endAngle, bool ccw) noexcept;

// ============================================================================
// TOLERANCE UTILITIES
// ============================================================================

/**
 * @brief Check if two values are equal within tolerance
 * @param a First value
 * @param b Second value
 * @param tolerance Maximum allowed difference
 * @return true if |a - b| < tolerance
 */
bool areEqual(double a, double b, double tolerance) noexcept;

/**
 * @brief Check if value is effectively zero
 * @param value Value to test
 * @param tolerance Maximum allowed magnitude
 * @return true if |value| < tolerance
 */
bool isZero(double value, double tolerance) noexcept;

/**
 * @brief Clamp value to range [min, max]
 * @param value Value to clamp
 * @param min Minimum value
 * @param max Maximum value
 * @return Clamped value
 */
double clamp(double value, double min, double max) noexcept;

// ============================================================================
// INTERSECTION CALCULATIONS
// ============================================================================

/**
 * @brief Find intersection point of two infinite lines
 * @param l1 First line
 * @param l2 Second line
 * @return Intersection point, or nullopt if parallel
 */
std::optional<Point2D> lineLineIntersection(const Line2D& l1, const Line2D& l2) noexcept;

/**
 * @brief Find intersection point of two line segments
 * @param l1 First line segment
 * @param l2 Second line segment
 * @return Intersection point, or nullopt if no intersection
 */
std::optional<Point2D> segmentSegmentIntersection(const Line2D& l1, const Line2D& l2) noexcept;

/**
 * @brief Check if two line segments intersect
 * @param l1 First line segment
 * @param l2 Second line segment
 * @return true if segments intersect or overlap
 */
bool segmentsIntersect(const Line2D& l1, const Line2D& l2) noexcept;

/**
 * @brief Find intersection points between a line segment and an arc
 * @param line The line segment
 * @param arc The arc
 * @return Vector of intersection points (0, 1, or 2 points)
 */
std::vector<Point2D> intersectLineArc(const Line2D& line, const Arc2D& arc) noexcept;

/**
 * @brief Find intersection points between two arcs
 * @param arc1 First arc
 * @param arc2 Second arc
 * @return Vector of intersection points (0, 1, or 2 points)
 */
std::vector<Point2D> intersectArcArc(const Arc2D& arc1, const Arc2D& arc2) noexcept;

// ============================================================================
// PROJECTION AND CLOSEST POINT
// ============================================================================

/**
 * @brief Project point onto infinite line
 * @param point Point to project
 * @param line Line to project onto
 * @return Closest point on infinite line
 */
Point2D projectPointOnLine(const Point2D& point, const Line2D& line) noexcept;

/**
 * @brief Find closest point on line segment to given point
 * @param point Point to find closest to
 * @param line Line segment
 * @return Closest point on segment (may be an endpoint)
 */
Point2D closestPointOnSegment(const Point2D& point, const Line2D& line) noexcept;

/**
 * @brief Find closest point on arc to given point
 * @param point Point to find closest to
 * @param arc Arc
 * @return Closest point on arc
 */
Point2D closestPointOnArc(const Point2D& point, const Arc2D& arc) noexcept;

/**
 * @brief Find closest point on ellipse to given point
 * @param point Point to find closest to
 * @param ellipse Ellipse
 * @return Closest point on ellipse (approximate via sampling)
 *
 * Uses sampling approach suitable for hit testing.
 */
Point2D closestPointOnEllipse(const Point2D& point, const Ellipse2D& ellipse) noexcept;

// ============================================================================
// TRANSLATION (for transformation tools)
// ============================================================================

/**
 * @brief Translate a point by displacement
 * @param point Point to translate
 * @param dx X displacement
 * @param dy Y displacement
 * @return New translated point
 */
Point2D translate(const Point2D& point, double dx, double dy) noexcept;

/**
 * @brief Translate a line by displacement (moves both endpoints)
 * @param line Line to translate
 * @param dx X displacement
 * @param dy Y displacement
 * @return New translated line, or nullopt if result is invalid
 */
std::optional<Line2D> translate(const Line2D& line, double dx, double dy) noexcept;

/**
 * @brief Translate an arc by displacement (moves center, preserves radius/angles)
 * @param arc Arc to translate
 * @param dx X displacement
 * @param dy Y displacement
 * @return New translated arc, or nullopt if result is invalid
 */
std::optional<Arc2D> translate(const Arc2D& arc, double dx, double dy) noexcept;

/**
 * @brief Translate an ellipse by displacement (moves center, preserves axes/angles)
 * @param ellipse Ellipse to translate
 * @param dx X displacement
 * @param dy Y displacement
 * @return New translated ellipse, or nullopt if result is invalid
 */
std::optional<Ellipse2D> translate(const Ellipse2D& ellipse, double dx, double dy) noexcept;

// ============================================================================
// ROTATION (for transformation tools)
// ============================================================================

/**
 * @brief Rotate a point around a center point
 * @param point Point to rotate
 * @param center Center of rotation
 * @param angleRadians Rotation angle in radians (positive = CCW)
 * @return New rotated point
 */
Point2D rotate(const Point2D& point, const Point2D& center, double angleRadians) noexcept;

/**
 * @brief Rotate a line around a center point (rotates both endpoints)
 * @param line Line to rotate
 * @param center Center of rotation
 * @param angleRadians Rotation angle in radians (positive = CCW)
 * @return New rotated line, or nullopt if result is invalid
 */
std::optional<Line2D> rotate(const Line2D& line, const Point2D& center, double angleRadians) noexcept;

/**
 * @brief Rotate an arc around a center point
 * @param arc Arc to rotate
 * @param center Center of rotation
 * @param angleRadians Rotation angle in radians (positive = CCW)
 * @return New rotated arc, or nullopt if result is invalid
 *
 * Note: Rotates arc center AND adjusts start/end angles by the same amount.
 * Arc direction (CCW/CW) is preserved.
 */
std::optional<Arc2D> rotate(const Arc2D& arc, const Point2D& center, double angleRadians) noexcept;

/**
 * @brief Rotate an ellipse around a center point
 * @param ellipse Ellipse to rotate
 * @param center Center of rotation
 * @param angleRadians Rotation angle in radians (positive = CCW)
 * @return New rotated ellipse, or nullopt if result is invalid
 *
 * Note: Rotates ellipse center AND major axis endpoint.
 */
std::optional<Ellipse2D> rotate(const Ellipse2D& ellipse, const Point2D& center, double angleRadians) noexcept;

/**
 * @brief Snap angle to common increments
 * @param angleRadians Raw angle in radians
 * @param snapIncrement Snap increment in radians (e.g., PI/12 for 15°)
 * @return Snapped angle
 */
double snapAngle(double angleRadians, double snapIncrement) noexcept;

// ============================================================================
// MIRROR (for transformation tools)
// ============================================================================

/**
 * @brief Mirror a point across an axis defined by two points
 * @param point Point to mirror
 * @param axisP1 First point on mirror axis
 * @param axisP2 Second point on mirror axis
 * @return Mirrored point
 *
 * The axis is the infinite line passing through axisP1 and axisP2.
 */
Point2D mirror(const Point2D& point, const Point2D& axisP1, const Point2D& axisP2) noexcept;

/**
 * @brief Mirror a line across an axis
 * @param line Line to mirror
 * @param axisP1 First point on mirror axis
 * @param axisP2 Second point on mirror axis
 * @return Mirrored line, or nullopt if axis is degenerate
 */
std::optional<Line2D> mirror(const Line2D& line, const Point2D& axisP1, const Point2D& axisP2) noexcept;

/**
 * @brief Mirror an arc across an axis
 * @param arc Arc to mirror
 * @param axisP1 First point on mirror axis
 * @param axisP2 Second point on mirror axis
 * @return Mirrored arc with INVERTED direction, or nullopt if invalid
 *
 * IMPORTANT: Mirror operation inverts arc direction (CCW <-> CW).
 * This is mathematically correct and critical for CNC toolpaths.
 */
std::optional<Arc2D> mirror(const Arc2D& arc, const Point2D& axisP1, const Point2D& axisP2) noexcept;

/**
 * @brief Mirror an ellipse across an axis
 * @param ellipse Ellipse to mirror
 * @param axisP1 First point on mirror axis
 * @param axisP2 Second point on mirror axis
 * @return Mirrored ellipse, or nullopt if invalid
 */
std::optional<Ellipse2D> mirror(const Ellipse2D& ellipse, const Point2D& axisP1, const Point2D& axisP2) noexcept;

} // namespace GeometryMath
} // namespace Geometry
} // namespace OwnCAD
