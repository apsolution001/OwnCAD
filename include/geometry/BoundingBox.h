#pragma once

#include "Point2D.h"
#include <vector>
#include <limits>

namespace OwnCAD {
namespace Geometry {

// Forward declarations
class Line2D;
class Arc2D;

/**
 * @brief Axis-aligned bounding box in 2D
 *
 * Represents a rectangular region defined by minimum and maximum
 * coordinates in X and Y axes.
 *
 * Design decisions:
 * - Immutable: Cannot modify bounds after construction
 * - Factory methods for different geometry types
 * - Efficient intersection tests for selection system
 */
class BoundingBox {
private:
    double minX_;
    double minY_;
    double maxX_;
    double maxY_;

    /**
     * @brief Private constructor - use factory methods
     */
    BoundingBox(double minX, double minY, double maxX, double maxY) noexcept;

public:
    /**
     * @brief Create empty (invalid) bounding box
     */
    BoundingBox() noexcept;

    /**
     * @brief Create bounding box from two corner points
     * @param p1 First corner
     * @param p2 Opposite corner (order doesn't matter)
     */
    static BoundingBox fromPoints(const Point2D& p1, const Point2D& p2) noexcept;

    /**
     * @brief Create bounding box from collection of points
     * @param points Vector of points to enclose
     * @return Smallest box containing all points, or invalid box if empty
     */
    static BoundingBox fromPointList(const std::vector<Point2D>& points) noexcept;

    /**
     * @brief Create bounding box for a line segment
     * @param line Line to enclose
     * @return Bounding box containing the line
     */
    static BoundingBox fromLine(const Line2D& line) noexcept;

    /**
     * @brief Create bounding box for an arc
     *
     * This is complex because arcs may cross quadrant boundaries.
     * Must check for extrema in both X and Y directions.
     *
     * @param arc Arc to enclose
     * @return Bounding box containing the arc
     */
    static BoundingBox fromArc(const Arc2D& arc) noexcept;

    /**
     * @brief Get minimum X coordinate
     */
    double minX() const noexcept { return minX_; }

    /**
     * @brief Get minimum Y coordinate
     */
    double minY() const noexcept { return minY_; }

    /**
     * @brief Get maximum X coordinate
     */
    double maxX() const noexcept { return maxX_; }

    /**
     * @brief Get maximum Y coordinate
     */
    double maxY() const noexcept { return maxY_; }

    /**
     * @brief Get width (X extent)
     */
    double width() const noexcept { return maxX_ - minX_; }

    /**
     * @brief Get height (Y extent)
     */
    double height() const noexcept { return maxY_ - minY_; }

    /**
     * @brief Get center point
     */
    Point2D center() const noexcept;

    /**
     * @brief Get area
     */
    double area() const noexcept { return width() * height(); }

    /**
     * @brief Check if this bounding box is valid
     * @return true if minX <= maxX and minY <= maxY
     */
    bool isValid() const noexcept;

    /**
     * @brief Check if a point is inside this bounding box
     * @param point Point to test
     * @param tolerance Expand box by this amount (default 0)
     * @return true if point is inside (inclusive of boundaries)
     */
    bool contains(const Point2D& point, double tolerance = 0.0) const noexcept;

    /**
     * @brief Check if another bounding box intersects this one
     * @param other Bounding box to test
     * @return true if boxes overlap or touch
     */
    bool intersects(const BoundingBox& other) const noexcept;

    /**
     * @brief Check if another bounding box is completely inside this one
     * @param other Bounding box to test
     * @return true if other is completely contained
     */
    bool containsBox(const BoundingBox& other) const noexcept;

    /**
     * @brief Merge this bounding box with another
     * @param other Bounding box to merge
     * @return New bounding box containing both boxes
     */
    BoundingBox merge(const BoundingBox& other) const noexcept;

    /**
     * @brief Expand bounding box by a margin
     * @param margin Amount to expand in all directions
     * @return New expanded bounding box
     */
    BoundingBox expand(double margin) const noexcept;
};

} // namespace Geometry
} // namespace OwnCAD
