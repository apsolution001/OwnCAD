#pragma once

#include "DXFEntity.h"
#include "geometry/Line2D.h"
#include "geometry/Arc2D.h"
#include <vector>
#include <variant>
#include <string>

namespace OwnCAD {
namespace Import {

/**
 * @brief Internal geometry entity (our validated model)
 */
using GeometryEntity = std::variant<Geometry::Line2D, Geometry::Arc2D>;

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
     * @brief Convert DXF LWPOLYLINE to multiple Line2D segments
     * @param polyline DXF polyline entity
     * @return Vector of Line2D segments (empty if invalid)
     *
     * Note: Bulge values (arc segments) are currently ignored.
     * All segments are converted to straight lines.
     */
    static std::vector<Geometry::Line2D> convertPolyline(const DXFLWPolyline& polyline);

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
};

} // namespace Import
} // namespace OwnCAD
