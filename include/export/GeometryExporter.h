#pragma once

#include "import/DXFEntity.h"
#include "import/GeometryConverter.h"
#include "geometry/Line2D.h"
#include "geometry/Arc2D.h"
#include "geometry/Ellipse2D.h"
#include "geometry/Point2D.h"
#include <vector>
#include <string>
#include <optional>

namespace OwnCAD {
namespace Export {

/**
 * @brief Result of geometry export operation
 */
struct ExportResult {
    bool success;
    std::vector<Import::DXFEntity> entities;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    size_t totalExported;
    size_t totalFailed;

    ExportResult()
        : success(true)
        , totalExported(0)
        , totalFailed(0) {}
};

/**
 * @brief Converts internal geometry back to DXF entities
 *
 * Design principles:
 * - Exact inverse of GeometryConverter (GeometryConverter converts DXF → internal, this converts internal → DXF)
 * - Preserves all metadata: handle, layer, color
 * - Converts radians back to degrees (DXF uses degrees)
 * - Handles edge cases: full circles (Arc2D with 360° sweep → DXFCircle)
 * - Error handling: logs unconvertible geometry, never fails silently
 *
 * CRITICAL: This is the trust boundary between our validated internal model
 * and external DXF data. All conversions must be exact and lossless.
 */
class GeometryExporter {
public:
    /**
     * @brief Convert internal geometry back to DXF entities
     * @param entities Internal geometry with metadata
     * @return Export result with DXF entities or errors
     *
     * Processes all entities and converts them back to DXF format.
     * Preserves handles, layers, colors, and source line numbers.
     */
    static ExportResult exportToDXF(
        const std::vector<Import::GeometryEntityWithMetadata>& entities
    );

private:
    /**
     * @brief Export Line2D to DXFLine
     * @param line Internal line geometry
     * @param layer Layer name
     * @param handle Entity handle
     * @param colorNumber DXF color code
     * @return DXFLine if valid, nullopt if invalid
     */
    static std::optional<Import::DXFLine> exportLine(
        const Geometry::Line2D& line,
        const std::string& layer,
        const std::string& handle,
        int colorNumber
    );

    /**
     * @brief Export Arc2D to DXFArc or DXFCircle
     * @param arc Internal arc geometry
     * @param layer Layer name
     * @param handle Entity handle
     * @param colorNumber DXF color code
     * @return DXFArc if partial arc, DXFCircle if full circle (360°)
     *
     * CRITICAL: Full circles must be exported as DXFCircle for compatibility.
     * Partial arcs are exported as DXFArc.
     */
    static std::variant<Import::DXFArc, Import::DXFCircle> exportArc(
        const Geometry::Arc2D& arc,
        const std::string& layer,
        const std::string& handle,
        int colorNumber
    );

    /**
     * @brief Export Ellipse2D to DXFEllipse
     * @param ellipse Internal ellipse geometry
     * @param layer Layer name
     * @param handle Entity handle
     * @param colorNumber DXF color code
     * @return DXFEllipse if valid, nullopt if invalid
     */
    static std::optional<Import::DXFEllipse> exportEllipse(
        const Geometry::Ellipse2D& ellipse,
        const std::string& layer,
        const std::string& handle,
        int colorNumber
    );

    /**
     * @brief Export Point2D to DXFPoint
     * @param point Internal point geometry
     * @param layer Layer name
     * @param handle Entity handle
     * @param colorNumber DXF color code
     * @return DXFPoint (always valid)
     */
    static Import::DXFPoint exportPoint(
        const Geometry::Point2D& point,
        const std::string& layer,
        const std::string& handle,
        int colorNumber
    );

    // ========================================================================
    // CONVERSION HELPERS
    // ========================================================================

    /**
     * @brief Convert radians to degrees
     */
    static double radiansToDegrees(double radians) noexcept;

    /**
     * @brief Check if arc is a full circle (360° sweep)
     */
    static bool isFullCircle(const Geometry::Arc2D& arc) noexcept;
};

} // namespace Export
} // namespace OwnCAD
