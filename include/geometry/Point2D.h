#pragma once

#include <cmath>
#include <stdexcept>

namespace OwnCAD {
namespace Geometry {

/**
 * @brief Immutable 2D point in Cartesian coordinates
 *
 * Represents a point in 2D space with double-precision coordinates.
 * This class is immutable by design to prevent accidental modification
 * and ensure thread-safety.
 *
 * Design decisions:
 * - Immutable: Once created, values cannot change
 * - Value semantics: Copyable, assignable, comparable
 * - Rejects invalid values: NaN and infinity cause construction to fail
 * - Tolerance-aware equality for manufacturing precision
 */
class Point2D {
private:
    double x_;
    double y_;

public:
    /**
     * @brief Construct a point at (0, 0)
     */
    Point2D() noexcept;

    /**
     * @brief Construct a point at (x, y)
     * @param x X-coordinate
     * @param y Y-coordinate
     * @throws std::invalid_argument if x or y is NaN or infinity
     */
    Point2D(double x, double y);

    /**
     * @brief Get X-coordinate
     */
    double x() const noexcept { return x_; }

    /**
     * @brief Get Y-coordinate
     */
    double y() const noexcept { return y_; }

    /**
     * @brief Check if this point equals another (exact comparison)
     * @param other Point to compare with
     * @return true if coordinates are exactly equal
     */
    bool operator==(const Point2D& other) const noexcept;

    /**
     * @brief Check if this point differs from another (exact comparison)
     * @param other Point to compare with
     * @return true if coordinates differ
     */
    bool operator!=(const Point2D& other) const noexcept;

    /**
     * @brief Check if this point equals another within tolerance
     * @param other Point to compare with
     * @param tolerance Maximum allowed difference (default: GEOMETRY_EPSILON)
     * @return true if both coordinates differ by less than tolerance
     */
    bool isEqual(const Point2D& other, double tolerance) const noexcept;

    /**
     * @brief Calculate distance to another point
     * @param other Target point
     * @return Euclidean distance
     */
    double distanceTo(const Point2D& other) const noexcept;

    /**
     * @brief Calculate squared distance to another point (faster, no sqrt)
     * @param other Target point
     * @return Squared Euclidean distance
     */
    double distanceSquaredTo(const Point2D& other) const noexcept;

    /**
     * @brief Validate that coordinates are finite
     * @return true if both x and y are finite (not NaN, not infinity)
     */
    bool isValid() const noexcept;

    /**
     * @brief Static validation helper
     * @param x X-coordinate to check
     * @param y Y-coordinate to check
     * @return true if both values are finite
     */
    static bool isValid(double x, double y) noexcept;
};

} // namespace Geometry
} // namespace OwnCAD
