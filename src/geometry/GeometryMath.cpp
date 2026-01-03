#include "geometry/GeometryMath.h"
#include "geometry/GeometryConstants.h"
#include <cmath>
#include <algorithm>

namespace OwnCAD {
namespace Geometry {
namespace GeometryMath {

// ============================================================================
// DISTANCE CALCULATIONS
// ============================================================================

double distance(const Point2D& p1, const Point2D& p2) noexcept {
    return p1.distanceTo(p2);
}

double distanceSquared(const Point2D& p1, const Point2D& p2) noexcept {
    return p1.distanceSquaredTo(p2);
}

double distancePointToLine(const Point2D& point, const Line2D& line) noexcept {
    const double dx = line.end().x() - line.start().x();
    const double dy = line.end().y() - line.start().y();
    const double lengthSq = dx * dx + dy * dy;

    if (lengthSq < GEOMETRY_EPSILON * GEOMETRY_EPSILON) {
        return distance(point, line.start());
    }

    // Calculate perpendicular distance using cross product
    const double numerator = std::abs(
        dx * (line.start().y() - point.y()) -
        dy * (line.start().x() - point.x())
    );

    return numerator / std::sqrt(lengthSq);
}

double distancePointToSegment(const Point2D& point, const Line2D& line) noexcept {
    const Point2D closest = closestPointOnSegment(point, line);
    return distance(point, closest);
}

double distancePointToArc(const Point2D& point, const Arc2D& arc) noexcept {
    const Point2D closest = closestPointOnArc(point, arc);
    return distance(point, closest);
}

// ============================================================================
// ANGLE UTILITIES
// ============================================================================

double normalizeAngle(double radians) noexcept {
    double result = std::fmod(radians, TWO_PI);
    if (result < 0.0) {
        result += TWO_PI;
    }
    return result;
}

double normalizeAngleSigned(double radians) noexcept {
    double result = std::fmod(radians + PI, TWO_PI);
    if (result < 0.0) {
        result += TWO_PI;
    }
    return result - PI;
}

double angleBetweenPoints(const Point2D& from, const Point2D& to) noexcept {
    return normalizeAngle(std::atan2(to.y() - from.y(), to.x() - from.x()));
}

double angleDifference(double angle1, double angle2) noexcept {
    double diff = normalizeAngleSigned(angle2 - angle1);
    return diff;
}

bool isAngleBetween(double angle, double start, double end, bool ccw) noexcept {
    angle = normalizeAngle(angle);
    start = normalizeAngle(start);
    end = normalizeAngle(end);

    if (ccw) {
        if (start <= end) {
            return angle >= start && angle <= end;
        } else {
            return angle >= start || angle <= end;
        }
    } else {
        if (start >= end) {
            return angle <= start && angle >= end;
        } else {
            return angle <= start || angle >= end;
        }
    }
}

// ============================================================================
// ARC CALCULATIONS
// ============================================================================

double arcLength(double radius, double sweepAngle) noexcept {
    return radius * std::abs(sweepAngle);
}

double sweepAngle(double startAngle, double endAngle, bool ccw) noexcept {
    startAngle = normalizeAngle(startAngle);
    endAngle = normalizeAngle(endAngle);

    double sweep;
    if (ccw) {
        sweep = endAngle - startAngle;
        if (sweep < 0.0) {
            sweep += TWO_PI;
        }
    } else {
        sweep = startAngle - endAngle;
        if (sweep < 0.0) {
            sweep += TWO_PI;
        }
    }

    // Handle near-zero sweep (might be full circle)
    if (sweep < GEOMETRY_EPSILON &&
        std::abs(startAngle - endAngle) < GEOMETRY_EPSILON) {
        sweep = TWO_PI;
    }

    return sweep;
}

// ============================================================================
// TOLERANCE UTILITIES
// ============================================================================

bool areEqual(double a, double b, double tolerance) noexcept {
    return std::abs(a - b) < tolerance;
}

bool isZero(double value, double tolerance) noexcept {
    return std::abs(value) < tolerance;
}

double clamp(double value, double min, double max) noexcept {
    return std::max(min, std::min(value, max));
}

// ============================================================================
// INTERSECTION CALCULATIONS
// ============================================================================

std::optional<Point2D> lineLineIntersection(const Line2D& l1, const Line2D& l2) noexcept {
    const double x1 = l1.start().x();
    const double y1 = l1.start().y();
    const double x2 = l1.end().x();
    const double y2 = l1.end().y();
    const double x3 = l2.start().x();
    const double y3 = l2.start().y();
    const double x4 = l2.end().x();
    const double y4 = l2.end().y();

    const double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

    if (std::abs(denom) < GEOMETRY_EPSILON) {
        return std::nullopt; // Parallel or coincident
    }

    const double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;

    const double ix = x1 + t * (x2 - x1);
    const double iy = y1 + t * (y2 - y1);

    return Point2D(ix, iy);
}

std::optional<Point2D> segmentSegmentIntersection(const Line2D& l1, const Line2D& l2) noexcept {
    const double x1 = l1.start().x();
    const double y1 = l1.start().y();
    const double x2 = l1.end().x();
    const double y2 = l1.end().y();
    const double x3 = l2.start().x();
    const double y3 = l2.start().y();
    const double x4 = l2.end().x();
    const double y4 = l2.end().y();

    const double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

    if (std::abs(denom) < GEOMETRY_EPSILON) {
        return std::nullopt; // Parallel
    }

    const double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
    const double u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;

    if (t >= 0.0 && t <= 1.0 && u >= 0.0 && u <= 1.0) {
        const double ix = x1 + t * (x2 - x1);
        const double iy = y1 + t * (y2 - y1);
        return Point2D(ix, iy);
    }

    return std::nullopt;
}

bool segmentsIntersect(const Line2D& l1, const Line2D& l2) noexcept {
    return segmentSegmentIntersection(l1, l2).has_value();
}

// ============================================================================
// PROJECTION AND CLOSEST POINT
// ============================================================================

Point2D projectPointOnLine(const Point2D& point, const Line2D& line) noexcept {
    const double dx = line.end().x() - line.start().x();
    const double dy = line.end().y() - line.start().y();
    const double lengthSq = dx * dx + dy * dy;

    if (lengthSq < GEOMETRY_EPSILON * GEOMETRY_EPSILON) {
        return line.start();
    }

    const double t = ((point.x() - line.start().x()) * dx +
                     (point.y() - line.start().y()) * dy) / lengthSq;

    return Point2D(
        line.start().x() + t * dx,
        line.start().y() + t * dy
    );
}

Point2D closestPointOnSegment(const Point2D& point, const Line2D& line) noexcept {
    const double dx = line.end().x() - line.start().x();
    const double dy = line.end().y() - line.start().y();
    const double lengthSq = dx * dx + dy * dy;

    if (lengthSq < GEOMETRY_EPSILON * GEOMETRY_EPSILON) {
        return line.start();
    }

    const double t = clamp(
        ((point.x() - line.start().x()) * dx +
         (point.y() - line.start().y()) * dy) / lengthSq,
        0.0, 1.0
    );

    return Point2D(
        line.start().x() + t * dx,
        line.start().y() + t * dy
    );
}

Point2D closestPointOnArc(const Point2D& point, const Arc2D& arc) noexcept {
    const double angle = angleBetweenPoints(arc.center(), point);

    if (isAngleBetween(angle, arc.startAngle(), arc.endAngle(),
                       arc.isCounterClockwise())) {
        return arc.pointAtAngle(angle);
    }

    // Point is outside arc sweep - closest is an endpoint
    const Point2D start = arc.startPoint();
    const Point2D end = arc.endPoint();

    if (distance(point, start) < distance(point, end)) {
        return start;
    } else {
        return end;
    }
}

} // namespace GeometryMath
} // namespace Geometry
} // namespace OwnCAD
