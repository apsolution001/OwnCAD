#include "import/GeometryConverter.h"
#include "geometry/GeometryConstants.h"
#include <cmath>
#include <sstream>
#include <iostream>

namespace OwnCAD {
namespace Import {

using namespace OwnCAD::Geometry;

// ============================================================================
// PUBLIC API
// ============================================================================

ConversionResult GeometryConverter::convert(const std::vector<DXFEntity>& dxfEntities) {
    ConversionResult result;

    std::cout << "\n=== GeometryConverter: Converting " << dxfEntities.size() << " DXF entities ===" << std::endl;

    for (size_t i = 0; i < dxfEntities.size(); ++i) {
        const auto& dxfEntity = dxfEntities[i];
        std::cout << "\nEntity " << (i+1) << "/" << dxfEntities.size() << " (line " << dxfEntity.lineNumber << "):" << std::endl;
        std::optional<GeometryEntity> converted;
        std::string layer;
        std::string handle;

        // Convert based on type
        std::visit([&](auto&& entity) {
            using T = std::decay_t<decltype(entity)>;

            if constexpr (std::is_same_v<T, DXFLine>) {
                std::cout << "  Type: LINE" << std::endl;
                std::cout << "  Start: (" << entity.startX << ", " << entity.startY << ")" << std::endl;
                std::cout << "  End: (" << entity.endX << ", " << entity.endY << ")" << std::endl;

                auto line = convertLine(entity);
                if (line.has_value()) {
                    double len = line->length();
                    std::cout << "  ✅ VALID - Length: " << len << std::endl;
                    converted = GeometryEntity(line.value());
                    layer = entity.layer;
                    handle = entity.handle;
                } else {
                    std::cout << "  ❌ REJECTED - Zero-length or invalid coordinates" << std::endl;
                    result.errors.push_back(
                        createErrorMessage("LINE", "Invalid geometry (zero-length or bad coordinates)",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }
            }
            else if constexpr (std::is_same_v<T, DXFArc>) {
                std::cout << "  Type: ARC" << std::endl;
                std::cout << "  Center: (" << entity.centerX << ", " << entity.centerY << ")" << std::endl;
                std::cout << "  Radius: " << entity.radius << std::endl;
                std::cout << "  Angles: " << entity.startAngle << "° to " << entity.endAngle << "°" << std::endl;

                auto arc = convertArc(entity);
                if (arc.has_value()) {
                    std::cout << "  ✅ VALID" << std::endl;
                    converted = GeometryEntity(arc.value());
                    layer = entity.layer;
                    handle = entity.handle;
                } else {
                    std::cout << "  ❌ REJECTED - Zero-radius or degenerate" << std::endl;
                    result.errors.push_back(
                        createErrorMessage("ARC", "Invalid geometry (zero-radius or degenerate)",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }
            }
            else if constexpr (std::is_same_v<T, DXFCircle>) {
                std::cout << "  Type: CIRCLE" << std::endl;
                std::cout << "  Center: (" << entity.centerX << ", " << entity.centerY << ")" << std::endl;
                std::cout << "  Radius: " << entity.radius << std::endl;

                auto arc = convertCircle(entity);
                if (arc.has_value()) {
                    std::cout << "  ✅ VALID (converted to full arc)" << std::endl;
                    converted = GeometryEntity(arc.value());
                    layer = entity.layer;
                    handle = entity.handle;
                } else {
                    std::cout << "  ❌ REJECTED - Zero-radius" << std::endl;
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

    std::cout << "\n=== Conversion Summary ===" << std::endl;
    std::cout << "  Total converted: " << result.totalConverted << std::endl;
    std::cout << "  Total failed: " << result.totalFailed << std::endl;
    std::cout << "  Errors: " << result.errors.size() << std::endl;

    if (!result.errors.empty()) {
        std::cout << "\n  Error details:" << std::endl;
        for (const auto& error : result.errors) {
            std::cout << "    - " << error << std::endl;
        }
    }
    std::cout << "========================\n" << std::endl;

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
