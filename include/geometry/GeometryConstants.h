#pragma once

/**
 * @file GeometryConstants.h
 * @brief Global constants for geometry calculations
 *
 * These constants define the precision and tolerance used throughout
 * the geometry engine. Values are chosen based on:
 * - DXF format precision (typically 1e-10)
 * - Manufacturing tolerances (laser cutting ~0.1mm)
 * - Numerical stability requirements
 */

namespace OwnCAD {
namespace Geometry {

/**
 * @brief Primary geometric tolerance for equality comparisons
 *
 * This value represents the smallest meaningful difference between
 * two geometric values. Set to 1e-9 to be slightly larger than
 * DXF precision while maintaining manufacturing-level accuracy.
 *
 * Usage: Two values are considered equal if |a - b| < GEOMETRY_EPSILON
 */
constexpr double GEOMETRY_EPSILON = 1e-9;

/**
 * @brief Minimum valid length for a line segment
 *
 * Any line shorter than this is considered degenerate and invalid.
 * This prevents numerical instability and meaningless geometry.
 */
constexpr double MIN_LINE_LENGTH = GEOMETRY_EPSILON;

/**
 * @brief Minimum valid radius for an arc
 *
 * Any arc with radius smaller than this is considered invalid.
 */
constexpr double MIN_ARC_RADIUS = GEOMETRY_EPSILON;

/**
 * @brief Minimum valid arc sweep angle (radians)
 *
 * Arcs with sweep angles smaller than this are considered degenerate.
 * Approximately 0.0000006 degrees.
 */
constexpr double MIN_ARC_SWEEP = GEOMETRY_EPSILON;

/**
 * @brief Mathematical constant PI
 *
 * High-precision value for angle calculations.
 */
constexpr double PI = 3.141592653589793238463;

/**
 * @brief 2*PI for full circle calculations
 */
constexpr double TWO_PI = 2.0 * PI;

/**
 * @brief PI/2 for right angle calculations
 */
constexpr double HALF_PI = PI / 2.0;

} // namespace Geometry
} // namespace OwnCAD
