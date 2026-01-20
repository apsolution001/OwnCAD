#pragma once

#include "DXFEntity.h"
#include "geometry/Line2D.h"
#include "geometry/Arc2D.h"
#include "geometry/Ellipse2D.h"
#include "geometry/Point2D.h"
#include <vector>
#include <variant>
#include <string>

namespace OwnCAD {
namespace Import {

/**
 * @brief Internal geometry entity (our validated model)
 *
 * Note: Point2D is used for POINT entities
 * Splines and Solids are approximated as Line2D segments
 */
using GeometryEntity = std::variant<
    Geometry::Line2D,
    Geometry::Arc2D,
    Geometry::Ellipse2D,
    Geometry::Point2D
>;

/**
 * @brief Entity with metadata preserved from DXF
 */
struct GeometryEntityWithMetadata {
    GeometryEntity entity;
    std::string layer;
    std::string handle;
    int colorNumber;  // DXF color code (1-255, 256=BYLAYER, 0=BYBLOCK)
    size_t sourceLineNumber;  // For error reporting
};

/**
 * @brief Result of geometry conversion
 */
struct ConversionResult {
    bool success;
    std::vector<GeometryEntityWithMetadata> entities;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    size_t totalConverted;
    size_t totalFailed;

    ConversionResult()
        : success(true)
        , totalConverted(0)
        , totalFailed(0) {}
};

/**
 * @brief Converts DXF entities to validated internal geometry model
 *
 * Design decisions:
 * - DXF Z-coordinates are ignored (2D projection)
 * - DXF angles (degrees) converted to radians
 * - DXF circles converted to full arcs (0° to 360°)
 * - Invalid geometry is rejected and logged (not silently fixed)
 * - Layer and handle metadata preserved for traceability
 *
 * Validation during conversion:
 * - Zero-length lines rejected
 * - Zero-radius arcs/circles rejected
 * - Invalid coordinates (NaN, infinity) rejected
 * - Degenerate geometry rejected
 *
 * CRITICAL: This is the trust boundary between external DXF data
 * and our validated internal model. All validation happens here.
 */
class GeometryConverter {
public:
    /**
     * @brief Convert DXF entities to internal geometry model
     * @param dxfEntities DXF entities from parser
     * @return Conversion result with validated geometry or errors
     */
    static ConversionResult convert(const std::vector<DXFEntity>& dxfEntities);

    /**
     * @brief Convert single DXF LINE to Line2D
     * @param dxfLine DXF line entity
     * @return Line2D if valid, nullopt if invalid
     */
    static std::optional<Geometry::Line2D> convertLine(const DXFLine& dxfLine);

    /**
     * @brief Convert single DXF ARC to Arc2D
     * @param dxfArc DXF arc entity
     * @return Arc2D if valid, nullopt if invalid
     */
    static std::optional<Geometry::Arc2D> convertArc(const DXFArc& dxfArc);

    /**
     * @brief Convert single DXF CIRCLE to Arc2D (full circle)
     * @param dxfCircle DXF circle entity
     * @return Arc2D if valid, nullopt if invalid
     */
    static std::optional<Geometry::Arc2D> convertCircle(const DXFCircle& dxfCircle);

    /**
     * @brief Convert DXF LWPOLYLINE to geometry segments (Line2D or Arc2D)
     * @param polyline DXF polyline entity
     * @return Vector of geometry entities (Line2D for straight, Arc2D for bulge segments)
     *
     * Handles bulge values correctly:
     * - bulge = 0: straight line segment (Line2D)
     * - bulge != 0: arc segment (Arc2D) where bulge = tan(included_angle/4)
     * - Positive bulge = CCW arc, Negative bulge = CW arc
     */
    static std::vector<GeometryEntity> convertPolyline(const DXFLWPolyline& polyline);

    /**
     * @brief Convert DXF ELLIPSE to Ellipse2D
     * @param dxfEllipse DXF ellipse entity
     * @return Ellipse2D if valid, nullopt if invalid
     */
    static std::optional<Geometry::Ellipse2D> convertEllipse(const DXFEllipse& dxfEllipse);

    /**
     * @brief Convert DXF SPLINE to polyline approximation
     * @param dxfSpline DXF spline entity
     * @return Vector of Line2D segments approximating the spline
     *
     * Note: Splines are approximated by sampling control points.
     * For production, a proper B-spline evaluator should be used.
     */
    static std::vector<Geometry::Line2D> convertSpline(const DXFSpline& dxfSpline);

    /**
     * @brief Convert DXF POINT to Point2D
     * @param dxfPoint DXF point entity
     * @return Point2D if valid, nullopt if invalid
     */
    static std::optional<Geometry::Point2D> convertPoint(const DXFPoint& dxfPoint);

    /**
     * @brief Convert DXF SOLID to triangle/quad as Line2D segments
     * @param dxfSolid DXF solid entity
     * @return Vector of Line2D segments forming the outline
     */
    static std::vector<Geometry::Line2D> convertSolid(const DXFSolid& dxfSolid);

    /**
     * @brief Convert degrees to radians
     */
    static double degreesToRadians(double degrees) noexcept;

    /**
     * @brief Convert radians to degrees
     */
    static double radiansToDegrees(double radians) noexcept;

private:
    /**
     * @brief Validate DXF coordinates are usable
     */
    static bool validateCoordinates(double x, double y);

    /**
     * @brief Create error message with context
     */
    static std::string createErrorMessage(
        const std::string& entityType,
        const std::string& reason,
        size_t lineNumber
    );

    /**
     * @brief Convert polyline arc segment (bulge != 0) to Arc2D
     * @param p1 Start point of segment
     * @param p2 End point of segment
     * @param bulge DXF bulge value (tan(included_angle/4))
     * @return Arc2D if valid, nullopt if degenerate
     *
     * Bulge formula: bulge = tan(included_angle / 4)
     * - |bulge| = 1 means semicircle (180°)
     * - |bulge| < 1 means less than semicircle
     * - |bulge| > 1 means more than semicircle
     * - Positive bulge: arc curves left (CCW from p1 to p2)
     * - Negative bulge: arc curves right (CW from p1 to p2)
     */
    static std::optional<Geometry::Arc2D> convertPolylineArcSegment(
        const Geometry::Point2D& p1,
        const Geometry::Point2D& p2,
        double bulge
    );
};

} // namespace Import
} // namespace OwnCAD
