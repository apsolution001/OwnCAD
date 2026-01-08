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
 * - 62: Color number (0=BYBLOCK, 256=BYLAYER)
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
    int colorNumber;  // DXF color code (1-255, 256=BYLAYER, 0=BYBLOCK)

    DXFLine()
        : startX(0), startY(0), startZ(0)
        , endX(0), endY(0), endZ(0)
        , layer("0"), handle(""), colorNumber(256) {}  // Default: BYLAYER
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
 * - 62: Color number
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
    int colorNumber;

    DXFArc()
        : centerX(0), centerY(0), centerZ(0)
        , radius(0)
        , startAngle(0), endAngle(0)
        , layer("0"), handle(""), colorNumber(256) {}
};

/**
 * @brief DXF CIRCLE entity (code 0 = CIRCLE)
 *
 * Represents a full circle in DXF format.
 * Group codes:
 * - 10,20,30: Center point (X,Y,Z)
 * - 40: Radius
 * - 8: Layer name
 * - 62: Color number
 */
struct DXFCircle {
    double centerX;
    double centerY;
    double centerZ;
    double radius;
    std::string layer;
    std::string handle;
    int colorNumber;

    DXFCircle()
        : centerX(0), centerY(0), centerZ(0)
        , radius(0)
        , layer("0"), handle(""), colorNumber(256) {}
};

/**
 * @brief Vertex in a polyline
 */
struct DXFVertex {
    double x;
    double y;
    double z;
    double bulge;  // For arc segments (0 = straight line)

    DXFVertex()
        : x(0), y(0), z(0), bulge(0) {}

    DXFVertex(double x_, double y_, double z_ = 0, double bulge_ = 0)
        : x(x_), y(y_), z(z_), bulge(bulge_) {}
};

/**
 * @brief DXF POLYLINE entity (code 0 = POLYLINE)
 *
 * Legacy polyline format with separate VERTEX entities.
 * Group codes:
 * - 8: Layer name
 * - 70: Flags (1 = closed)
 * - Followed by VERTEX entities
 * - Terminated by SEQEND
 */
struct DXFPolyline {
    std::vector<DXFVertex> vertices;
    bool closed;
    std::string layer;
    std::string handle;
    int colorNumber;

    DXFPolyline()
        : closed(false), layer("0"), handle(""), colorNumber(256) {}
};

/**
 * @brief DXF LWPOLYLINE entity (code 0 = LWPOLYLINE)
 *
 * Modern lightweight polyline format (R14+).
 * All vertex data in one entity.
 * Group codes:
 * - 8: Layer name
 * - 70: Flags (1 = closed)
 * - 90: Number of vertices
 * - 10,20: Vertex X,Y (repeated for each vertex)
 * - 42: Bulge (optional, for arc segments)
 * - 62: Color number
 */
struct DXFLWPolyline {
    std::vector<DXFVertex> vertices;
    bool closed;
    std::string layer;
    std::string handle;
    int colorNumber;

    DXFLWPolyline()
        : closed(false), layer("0"), handle(""), colorNumber(256) {}
};

/**
 * @brief Union type for all supported DXF entities
 */
using DXFEntityVariant = std::variant<DXFLine, DXFArc, DXFCircle, DXFPolyline, DXFLWPolyline>;

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
