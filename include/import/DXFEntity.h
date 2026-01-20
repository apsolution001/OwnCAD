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
    Ellipse,
    Spline,
    Point,
    Solid,
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
 * @brief DXF ELLIPSE entity (code 0 = ELLIPSE)
 *
 * Represents an ellipse or elliptical arc in DXF format.
 * Group codes:
 * - 10,20,30: Center point (X,Y,Z)
 * - 11,21,31: Endpoint of major axis (relative to center)
 * - 40: Ratio of minor axis to major axis
 * - 41: Start parameter (0 for full ellipse)
 * - 42: End parameter (2π for full ellipse)
 * - 8: Layer name
 * - 62: Color number
 */
struct DXFEllipse {
    double centerX;
    double centerY;
    double centerZ;
    double majorAxisX;  // Endpoint of major axis relative to center
    double majorAxisY;
    double majorAxisZ;
    double minorAxisRatio;  // Ratio of minor to major axis (0 < ratio <= 1)
    double startParameter;   // Start parameter (radians)
    double endParameter;     // End parameter (radians)
    std::string layer;
    std::string handle;
    int colorNumber;

    DXFEllipse()
        : centerX(0), centerY(0), centerZ(0)
        , majorAxisX(1), majorAxisY(0), majorAxisZ(0)
        , minorAxisRatio(1.0)
        , startParameter(0), endParameter(2.0 * 3.14159265358979323846)
        , layer("0"), handle(""), colorNumber(256) {}
};

/**
 * @brief DXF SPLINE entity (code 0 = SPLINE)
 *
 * Represents a B-spline curve in DXF format.
 * For simplicity, we'll approximate splines as polylines by sampling points.
 * Group codes:
 * - 70: Spline flag (1=closed, 2=periodic, 4=rational, 8=planar)
 * - 71: Degree of spline
 * - 72: Number of knots
 * - 73: Number of control points
 * - 10,20,30: Control points (repeated)
 * - 40: Knot values (repeated)
 * - 8: Layer name
 * - 62: Color number
 */
struct DXFSpline {
    std::vector<DXFVertex> controlPoints;
    std::vector<double> knots;
    int degree;
    bool closed;
    bool periodic;
    bool rational;
    std::string layer;
    std::string handle;
    int colorNumber;

    DXFSpline()
        : degree(3), closed(false), periodic(false), rational(false)
        , layer("0"), handle(""), colorNumber(256) {}
};

/**
 * @brief DXF POINT entity (code 0 = POINT)
 *
 * Represents a single point in DXF format.
 * Group codes:
 * - 10,20,30: Point location (X,Y,Z)
 * - 8: Layer name
 * - 62: Color number
 */
struct DXFPoint {
    double x;
    double y;
    double z;
    std::string layer;
    std::string handle;
    int colorNumber;

    DXFPoint()
        : x(0), y(0), z(0)
        , layer("0"), handle(""), colorNumber(256) {}
};

/**
 * @brief DXF SOLID entity (code 0 = SOLID)
 *
 * Represents a filled triangle or quadrilateral in DXF format.
 * Group codes:
 * - 10,20,30: First corner (X,Y,Z)
 * - 11,21,31: Second corner
 * - 12,22,32: Third corner
 * - 13,23,33: Fourth corner (optional, if omitted it's a triangle)
 * - 8: Layer name
 * - 62: Color number
 */
struct DXFSolid {
    double x1, y1, z1;
    double x2, y2, z2;
    double x3, y3, z3;
    double x4, y4, z4;
    bool isTriangle;  // true if only 3 corners
    std::string layer;
    std::string handle;
    int colorNumber;

    DXFSolid()
        : x1(0), y1(0), z1(0)
        , x2(0), y2(0), z2(0)
        , x3(0), y3(0), z3(0)
        , x4(0), y4(0), z4(0)
        , isTriangle(true)
        , layer("0"), handle(""), colorNumber(256) {}
};

/**
 * @brief Union type for all supported DXF entities
 */
using DXFEntityVariant = std::variant<
    DXFLine,
    DXFArc,
    DXFCircle,
    DXFPolyline,
    DXFLWPolyline,
    DXFEllipse,
    DXFSpline,
    DXFPoint,
    DXFSolid
>;

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
        case DXFEntityType::Ellipse: return "ELLIPSE";
        case DXFEntityType::Spline: return "SPLINE";
        case DXFEntityType::Point: return "POINT";
        case DXFEntityType::Solid: return "SOLID";
        case DXFEntityType::Unknown: return "UNKNOWN";
        default: return "INVALID";
    }
}

} // namespace Import
} // namespace OwnCAD
