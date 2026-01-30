#include "export/GeometryExporter.h"
#include "geometry/GeometryConstants.h"
#include <cmath>

namespace OwnCAD {
namespace Export {

using namespace OwnCAD::Import;
using namespace OwnCAD::Geometry;

// ============================================================================
// PUBLIC API
// ============================================================================

ExportResult GeometryExporter::exportToDXF(
    const std::vector<GeometryEntityWithMetadata>& entities
) {
    ExportResult result;

    for (const auto& entityWithMeta : entities) {
        DXFEntity dxfEntity;
        dxfEntity.lineNumber = entityWithMeta.sourceLineNumber;

        bool exported = false;

        // Convert based on geometry type
        std::visit([&](auto&& geometry) {
            using T = std::decay_t<decltype(geometry)>;

            if constexpr (std::is_same_v<T, Line2D>) {
                auto dxfLine = exportLine(
                    geometry,
                    entityWithMeta.layer,
                    entityWithMeta.handle,
                    entityWithMeta.colorNumber
                );

                if (dxfLine.has_value()) {
                    dxfEntity.type = DXFEntityType::Line;
                    dxfEntity.data = dxfLine.value();
                    exported = true;
                } else {
                    result.errors.push_back(
                        "Failed to export Line2D (handle: " + entityWithMeta.handle + ")"
                    );
                    result.totalFailed++;
                }
            }
            else if constexpr (std::is_same_v<T, Arc2D>) {
                auto dxfArc = exportArc(
                    geometry,
                    entityWithMeta.layer,
                    entityWithMeta.handle,
                    entityWithMeta.colorNumber
                );

                // Check if full circle or partial arc
                if (std::holds_alternative<DXFCircle>(dxfArc)) {
                    dxfEntity.type = DXFEntityType::Circle;
                    dxfEntity.data = std::get<DXFCircle>(dxfArc);
                    exported = true;
                } else {
                    dxfEntity.type = DXFEntityType::Arc;
                    dxfEntity.data = std::get<DXFArc>(dxfArc);
                    exported = true;
                }
            }
            else if constexpr (std::is_same_v<T, Ellipse2D>) {
                auto dxfEllipse = exportEllipse(
                    geometry,
                    entityWithMeta.layer,
                    entityWithMeta.handle,
                    entityWithMeta.colorNumber
                );

                if (dxfEllipse.has_value()) {
                    dxfEntity.type = DXFEntityType::Ellipse;
                    dxfEntity.data = dxfEllipse.value();
                    exported = true;
                } else {
                    result.errors.push_back(
                        "Failed to export Ellipse2D (handle: " + entityWithMeta.handle + ")"
                    );
                    result.totalFailed++;
                }
            }
            else if constexpr (std::is_same_v<T, Point2D>) {
                dxfEntity.type = DXFEntityType::Point;
                dxfEntity.data = exportPoint(
                    geometry,
                    entityWithMeta.layer,
                    entityWithMeta.handle,
                    entityWithMeta.colorNumber
                );
                exported = true;
            }
        }, entityWithMeta.entity);

        if (exported) {
            result.entities.push_back(dxfEntity);
            result.totalExported++;
        }
    }

    result.success = result.errors.empty();
    return result;
}

// ============================================================================
// ENTITY EXPORTERS
// ============================================================================

std::optional<DXFLine> GeometryExporter::exportLine(
    const Line2D& line,
    const std::string& layer,
    const std::string& handle,
    int colorNumber
) {
    // Validate line
    if (!line.isValid()) {
        return std::nullopt;
    }

    DXFLine dxfLine;

    // Geometry
    dxfLine.startX = line.start().x();
    dxfLine.startY = line.start().y();
    dxfLine.startZ = 0.0;  // 2D only

    dxfLine.endX = line.end().x();
    dxfLine.endY = line.end().y();
    dxfLine.endZ = 0.0;  // 2D only

    // Metadata
    dxfLine.layer = layer;
    dxfLine.handle = handle;
    dxfLine.colorNumber = colorNumber;

    return dxfLine;
}

std::variant<DXFArc, DXFCircle> GeometryExporter::exportArc(
    const Arc2D& arc,
    const std::string& layer,
    const std::string& handle,
    int colorNumber
) {
    // Check if full circle
    if (isFullCircle(arc)) {
        DXFCircle dxfCircle;

        // Geometry
        dxfCircle.centerX = arc.center().x();
        dxfCircle.centerY = arc.center().y();
        dxfCircle.centerZ = 0.0;  // 2D only
        dxfCircle.radius = arc.radius();

        // Metadata
        dxfCircle.layer = layer;
        dxfCircle.handle = handle;
        dxfCircle.colorNumber = colorNumber;

        return dxfCircle;
    } else {
        DXFArc dxfArc;

        // Geometry
        dxfArc.centerX = arc.center().x();
        dxfArc.centerY = arc.center().y();
        dxfArc.centerZ = 0.0;  // 2D only
        dxfArc.radius = arc.radius();

        // Convert angles from radians to degrees
        dxfArc.startAngle = radiansToDegrees(arc.startAngle());
        dxfArc.endAngle = radiansToDegrees(arc.endAngle());

        // Metadata
        dxfArc.layer = layer;
        dxfArc.handle = handle;
        dxfArc.colorNumber = colorNumber;

        return dxfArc;
    }
}

std::optional<DXFEllipse> GeometryExporter::exportEllipse(
    const Ellipse2D& ellipse,
    const std::string& layer,
    const std::string& handle,
    int colorNumber
) {
    // Validate ellipse
    if (!ellipse.isValid()) {
        return std::nullopt;
    }

    DXFEllipse dxfEllipse;

    // Geometry
    dxfEllipse.centerX = ellipse.center().x();
    dxfEllipse.centerY = ellipse.center().y();
    dxfEllipse.centerZ = 0.0;  // 2D only

    // Major axis endpoint (relative to center)
    dxfEllipse.majorAxisX = ellipse.majorAxisLength() * std::cos(ellipse.rotation());
    dxfEllipse.majorAxisY = ellipse.majorAxisLength() * std::sin(ellipse.rotation());
    dxfEllipse.majorAxisZ = 0.0;  // 2D only

    // Minor to major axis ratio
    dxfEllipse.minorAxisRatio = ellipse.minorAxisRatio();

    // Start and end angles (radians in DXF ellipse)
    dxfEllipse.startParameter = ellipse.startAngle();
    dxfEllipse.endParameter = ellipse.endAngle();

    // Metadata
    dxfEllipse.layer = layer;
    dxfEllipse.handle = handle;
    dxfEllipse.colorNumber = colorNumber;

    return dxfEllipse;
}

DXFPoint GeometryExporter::exportPoint(
    const Point2D& point,
    const std::string& layer,
    const std::string& handle,
    int colorNumber
) {
    DXFPoint dxfPoint;

    // Geometry
    dxfPoint.x = point.x();
    dxfPoint.y = point.y();
    dxfPoint.z = 0.0;  // 2D only

    // Metadata
    dxfPoint.layer = layer;
    dxfPoint.handle = handle;
    dxfPoint.colorNumber = colorNumber;

    return dxfPoint;
}

// ============================================================================
// CONVERSION HELPERS
// ============================================================================

double GeometryExporter::radiansToDegrees(double radians) noexcept {
    return radians * 180.0 / PI;
}

bool GeometryExporter::isFullCircle(const Arc2D& arc) noexcept {
    return arc.isFullCircle();
}

} // namespace Export
} // namespace OwnCAD
