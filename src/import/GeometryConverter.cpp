#include "import/GeometryConverter.h"
#include "geometry/GeometryConstants.h"
#include <cmath>
#include <sstream>

namespace OwnCAD {
namespace Import {

using namespace OwnCAD::Geometry;

// ============================================================================
// PUBLIC API
// ============================================================================

ConversionResult GeometryConverter::convert(const std::vector<DXFEntity>& dxfEntities) {
    ConversionResult result;

    for (const auto& dxfEntity : dxfEntities) {
        std::optional<GeometryEntity> converted;
        std::string layer;
        std::string handle;

        // Convert based on type
        std::visit([&](auto&& entity) {
            using T = std::decay_t<decltype(entity)>;

            if constexpr (std::is_same_v<T, DXFLine>) {
                auto line = convertLine(entity);
                if (line.has_value()) {
                    converted = GeometryEntity(line.value());
                    layer = entity.layer;
                    handle = entity.handle;
                } else {
                    result.errors.push_back(
                        createErrorMessage("LINE", "Invalid geometry (zero-length or bad coordinates)",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }
            }
            else if constexpr (std::is_same_v<T, DXFArc>) {
                auto arc = convertArc(entity);
                if (arc.has_value()) {
                    converted = GeometryEntity(arc.value());
                    layer = entity.layer;
                    handle = entity.handle;
                } else {
                    result.errors.push_back(
                        createErrorMessage("ARC", "Invalid geometry (zero-radius or degenerate)",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }
            }
            else if constexpr (std::is_same_v<T, DXFCircle>) {
                auto arc = convertCircle(entity);
                if (arc.has_value()) {
                    converted = GeometryEntity(arc.value());
                    layer = entity.layer;
                    handle = entity.handle;
                } else {
                    result.errors.push_back(
                        createErrorMessage("CIRCLE", "Invalid geometry (zero-radius)",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }
            }
        }, dxfEntity.data);

        // Add to result if conversion succeeded
        if (converted.has_value()) {
            GeometryEntityWithMetadata entityWithMeta{
                converted.value(),  // entity
                layer,              // layer
                handle,             // handle
                dxfEntity.lineNumber // sourceLineNumber
            };

            result.entities.push_back(entityWithMeta);
            result.totalConverted++;
        }
    }

    result.success = result.errors.empty();
    return result;
}

// ============================================================================
// INDIVIDUAL CONVERTERS
// ============================================================================

std::optional<Line2D> GeometryConverter::convertLine(const DXFLine& dxfLine) {
    // Validate coordinates
    if (!validateCoordinates(dxfLine.startX, dxfLine.startY) ||
        !validateCoordinates(dxfLine.endX, dxfLine.endY)) {
        return std::nullopt;
    }

    // Create points (Z coordinate ignored - 2D projection)
    Point2D start(dxfLine.startX, dxfLine.startY);
    Point2D end(dxfLine.endX, dxfLine.endY);

    // Use factory pattern - will reject zero-length lines
    return Line2D::create(start, end);
}

std::optional<Arc2D> GeometryConverter::convertArc(const DXFArc& dxfArc) {
    // Validate coordinates
    if (!validateCoordinates(dxfArc.centerX, dxfArc.centerY)) {
        return std::nullopt;
    }

    // Validate radius
    if (dxfArc.radius <= MIN_ARC_RADIUS || !std::isfinite(dxfArc.radius)) {
        return std::nullopt;
    }

    // Create center point (Z coordinate ignored)
    Point2D center(dxfArc.centerX, dxfArc.centerY);

    // Convert angles from degrees to radians
    double startAngleRad = degreesToRadians(dxfArc.startAngle);
    double endAngleRad = degreesToRadians(dxfArc.endAngle);

    // DXF arcs are always counter-clockwise
    // Use factory pattern - will reject degenerate arcs
    return Arc2D::create(center, dxfArc.radius, startAngleRad, endAngleRad, true);
}

std::optional<Arc2D> GeometryConverter::convertCircle(const DXFCircle& dxfCircle) {
    // Validate coordinates
    if (!validateCoordinates(dxfCircle.centerX, dxfCircle.centerY)) {
        return std::nullopt;
    }

    // Validate radius
    if (dxfCircle.radius <= MIN_ARC_RADIUS || !std::isfinite(dxfCircle.radius)) {
        return std::nullopt;
    }

    // Create center point (Z coordinate ignored)
    Point2D center(dxfCircle.centerX, dxfCircle.centerY);

    // Convert circle to full arc (0° to 360°)
    // Use factory pattern - will validate
    return Arc2D::create(center, dxfCircle.radius, 0.0, TWO_PI, true);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

double GeometryConverter::degreesToRadians(double degrees) noexcept {
    return degrees * (PI / 180.0);
}

double GeometryConverter::radiansToDegrees(double radians) noexcept {
    return radians * (180.0 / PI);
}

bool GeometryConverter::validateCoordinates(double x, double y) {
    return std::isfinite(x) && std::isfinite(y);
}

std::string GeometryConverter::createErrorMessage(
    const std::string& entityType,
    const std::string& reason,
    size_t lineNumber
) {
    std::ostringstream oss;
    oss << entityType << " at line " << lineNumber << ": " << reason;
    return oss.str();
}

} // namespace Import
} // namespace OwnCAD
