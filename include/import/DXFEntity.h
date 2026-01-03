#pragma once

#include <string>
#include <variant>
#include <optional>
#include <vector>

namespace OwnCAD {
namespace Import {

/**
 * @brief DXF entity types supported by the importer
 */
enum class DXFEntityType {
    Line,
    Arc,
    Circle,
    Polyline,
    LWPolyline,
    Unknown
};

/**
 * @brief DXF LINE entity (code 0 = LINE)
 *
 * Represents a straight line segment in DXF format.
 * Group codes:
 * - 10,20,30: Start point (X,Y,Z)
 * - 11,21,31: End point (X,Y,Z)
 * - 8: Layer name
 */
struct DXFLine {
    double startX;
    double startY;
    double startZ;
    double endX;
    double endY;
    double endZ;
    std::string layer;
    std::string handle;

    DXFLine()
        : startX(0), startY(0), startZ(0)
        , endX(0), endY(0), endZ(0)
        , layer("0"), handle("") {}
};

/**
 * @brief DXF ARC entity (code 0 = ARC)
 *
 * Represents a circular arc in DXF format.
 * Group codes:
 * - 10,20,30: Center point (X,Y,Z)
 * - 40: Radius
 * - 50: Start angle (degrees)
 * - 51: End angle (degrees)
 * - 8: Layer name
 *
 * NOTE: DXF arcs are ALWAYS counter-clockwise in DXF coordinate system
 */
struct DXFArc {
    double centerX;
    double centerY;
    double centerZ;
    double radius;
    double startAngle;  // Degrees
    double endAngle;    // Degrees
    std::string layer;
    std::string handle;

    DXFArc()
        : centerX(0), centerY(0), centerZ(0)
        , radius(0)
        , startAngle(0), endAngle(0)
        , layer("0"), handle("") {}
};

/**
 * @brief DXF CIRCLE entity (code 0 = CIRCLE)
 *
 * Represents a full circle in DXF format.
 * Group codes:
 * - 10,20,30: Center point (X,Y,Z)
 * - 40: Radius
 * - 8: Layer name
 */
struct DXFCircle {
    double centerX;
    double centerY;
    double centerZ;
    double radius;
    std::string layer;
    std::string handle;

    DXFCircle()
        : centerX(0), centerY(0), centerZ(0)
        , radius(0)
        , layer("0"), handle("") {}
};

/**
 * @brief Union type for all supported DXF entities
 */
using DXFEntityVariant = std::variant<DXFLine, DXFArc, DXFCircle>;

/**
 * @brief Container for a parsed DXF entity with metadata
 */
struct DXFEntity {
    DXFEntityType type;
    DXFEntityVariant data;
    size_t lineNumber;  // Line number in DXF file for error reporting

    DXFEntity()
        : type(DXFEntityType::Unknown)
        , lineNumber(0) {}
};

/**
 * @brief Result of DXF parsing operation
 */
struct DXFParseResult {
    bool success;
    std::vector<DXFEntity> entities;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    size_t totalEntities;
    size_t skippedEntities;  // Unsupported entity types

    DXFParseResult()
        : success(false)
        , totalEntities(0)
        , skippedEntities(0) {}
};

/**
 * @brief Convert DXF entity type to string
 */
inline std::string toString(DXFEntityType type) {
    switch (type) {
        case DXFEntityType::Line: return "LINE";
        case DXFEntityType::Arc: return "ARC";
        case DXFEntityType::Circle: return "CIRCLE";
        case DXFEntityType::Polyline: return "POLYLINE";
        case DXFEntityType::LWPolyline: return "LWPOLYLINE";
        case DXFEntityType::Unknown: return "UNKNOWN";
        default: return "INVALID";
    }
}

} // namespace Import
} // namespace OwnCAD
