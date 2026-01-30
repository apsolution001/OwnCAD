#include "export/DXFWriter.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <cmath>

namespace OwnCAD {
namespace Export {

using namespace OwnCAD::Import;

// ============================================================================
// PUBLIC API
// ============================================================================

bool DXFWriter::writeFile(
    const std::string& filePath,
    const std::vector<DXFEntity>& entities
) {
    std::ofstream file(filePath, std::ios::out | std::ios::trunc);

    if (!file.is_open()) {
        return false;
    }

    bool success = writeStream(file, entities);
    file.close();

    return success;
}

bool DXFWriter::writeStream(
    std::ostream& out,
    const std::vector<DXFEntity>& entities
) {
    if (!out.good()) {
        return false;
    }

    // Write DXF sections in order
    writeHeader(out);
    writeTables(out, entities);
    writeEntities(out, entities);
    writeFooter(out);

    return out.good();
}

// ============================================================================
// SECTION WRITERS
// ============================================================================

void DXFWriter::writeHeader(std::ostream& out) {
    writeGroup(out, 0, "SECTION");
    writeGroup(out, 2, "HEADER");

    // DXF version (AC1032 = AutoCAD 2018)
    writeGroup(out, 9, "$ACADVER");
    writeGroup(out, 1, "AC1032");

    // Database version (R2018)
    writeGroup(out, 9, "$ACADMAINTVER");
    writeGroup(out, 70, 104);

    // Units (0 = unitless, suitable for CAD)
    writeGroup(out, 9, "$INSUNITS");
    writeGroup(out, 70, 0);

    writeGroup(out, 0, "ENDSEC");
}

void DXFWriter::writeTables(std::ostream& out, const std::vector<DXFEntity>& entities) {
    writeGroup(out, 0, "SECTION");
    writeGroup(out, 2, "TABLES");

    // ========================================================================
    // LAYER TABLE
    // ========================================================================
    writeGroup(out, 0, "TABLE");
    writeGroup(out, 2, "LAYER");
    writeGroup(out, 70, 0);  // Max layers (0 = no limit)

    // Extract unique layer names
    std::vector<std::string> layers = extractLayers(entities);

    // Write layer entries
    for (const auto& layerName : layers) {
        writeGroup(out, 0, "LAYER");
        writeGroup(out, 2, layerName);  // Layer name
        writeGroup(out, 70, 0);         // Flags (0 = none)
        writeGroup(out, 62, 7);         // Color (7 = white/black)
        writeGroup(out, 6, "CONTINUOUS"); // Linetype
    }

    writeGroup(out, 0, "ENDTAB");

    // ========================================================================
    // LTYPE TABLE (line types)
    // ========================================================================
    writeGroup(out, 0, "TABLE");
    writeGroup(out, 2, "LTYPE");
    writeGroup(out, 70, 0);  // Max linetypes

    // CONTINUOUS linetype (required)
    writeGroup(out, 0, "LTYPE");
    writeGroup(out, 2, "CONTINUOUS");
    writeGroup(out, 70, 0);  // Flags
    writeGroup(out, 3, "Solid line");  // Description
    writeGroup(out, 72, 65);  // Alignment code
    writeGroup(out, 73, 0);   // Dash count

    // BYLAYER linetype
    writeGroup(out, 0, "LTYPE");
    writeGroup(out, 2, "BYLAYER");
    writeGroup(out, 70, 0);
    writeGroup(out, 3, "ByLayer");
    writeGroup(out, 72, 65);
    writeGroup(out, 73, 0);

    writeGroup(out, 0, "ENDTAB");

    writeGroup(out, 0, "ENDSEC");
}

void DXFWriter::writeEntities(std::ostream& out, const std::vector<DXFEntity>& entities) {
    writeGroup(out, 0, "SECTION");
    writeGroup(out, 2, "ENTITIES");

    // Write each entity based on its type
    for (const auto& entity : entities) {
        switch (entity.type) {
            case DXFEntityType::Line:
                writeLine(out, std::get<DXFLine>(entity.data));
                break;

            case DXFEntityType::Arc:
                writeArc(out, std::get<DXFArc>(entity.data));
                break;

            case DXFEntityType::Circle:
                writeCircle(out, std::get<DXFCircle>(entity.data));
                break;

            case DXFEntityType::LWPolyline:
                writeLWPolyline(out, std::get<DXFLWPolyline>(entity.data));
                break;

            case DXFEntityType::Ellipse:
                writeEllipse(out, std::get<DXFEllipse>(entity.data));
                break;

            case DXFEntityType::Point:
                writePoint(out, std::get<DXFPoint>(entity.data));
                break;

            case DXFEntityType::Solid:
                writeSolid(out, std::get<DXFSolid>(entity.data));
                break;

            default:
                // Skip unsupported entity types
                break;
        }
    }

    writeGroup(out, 0, "ENDSEC");
}

void DXFWriter::writeFooter(std::ostream& out) {
    writeGroup(out, 0, "EOF");
}

// ============================================================================
// ENTITY WRITERS
// ============================================================================

void DXFWriter::writeLine(std::ostream& out, const DXFLine& line) {
    writeGroup(out, 0, "LINE");

    // Handle (entity ID)
    if (!line.handle.empty()) {
        writeGroup(out, 5, line.handle);
    }

    // Layer
    writeGroup(out, 8, line.layer);

    // Color number
    writeGroup(out, 62, line.colorNumber);

    // Start point
    writeGroup(out, 10, line.startX);
    writeGroup(out, 20, line.startY);
    writeGroup(out, 30, line.startZ);

    // End point
    writeGroup(out, 11, line.endX);
    writeGroup(out, 21, line.endY);
    writeGroup(out, 31, line.endZ);
}

void DXFWriter::writeArc(std::ostream& out, const DXFArc& arc) {
    writeGroup(out, 0, "ARC");

    // Handle
    if (!arc.handle.empty()) {
        writeGroup(out, 5, arc.handle);
    }

    // Layer
    writeGroup(out, 8, arc.layer);

    // Color number
    writeGroup(out, 62, arc.colorNumber);

    // Center point
    writeGroup(out, 10, arc.centerX);
    writeGroup(out, 20, arc.centerY);
    writeGroup(out, 30, arc.centerZ);

    // Radius
    writeGroup(out, 40, arc.radius);

    // Angles (degrees)
    writeGroup(out, 50, arc.startAngle);
    writeGroup(out, 51, arc.endAngle);
}

void DXFWriter::writeCircle(std::ostream& out, const DXFCircle& circle) {
    writeGroup(out, 0, "CIRCLE");

    // Handle
    if (!circle.handle.empty()) {
        writeGroup(out, 5, circle.handle);
    }

    // Layer
    writeGroup(out, 8, circle.layer);

    // Color number
    writeGroup(out, 62, circle.colorNumber);

    // Center point
    writeGroup(out, 10, circle.centerX);
    writeGroup(out, 20, circle.centerY);
    writeGroup(out, 30, circle.centerZ);

    // Radius
    writeGroup(out, 40, circle.radius);
}

void DXFWriter::writeLWPolyline(std::ostream& out, const DXFLWPolyline& polyline) {
    writeGroup(out, 0, "LWPOLYLINE");

    // Handle
    if (!polyline.handle.empty()) {
        writeGroup(out, 5, polyline.handle);
    }

    // Layer
    writeGroup(out, 8, polyline.layer);

    // Color number
    writeGroup(out, 62, polyline.colorNumber);

    // Number of vertices
    writeGroup(out, 90, static_cast<int>(polyline.vertices.size()));

    // Flags (1 = closed, 0 = open)
    writeGroup(out, 70, polyline.closed ? 1 : 0);

    // Vertices
    for (const auto& vertex : polyline.vertices) {
        writeGroup(out, 10, vertex.x);
        writeGroup(out, 20, vertex.y);

        // Bulge (only if non-zero)
        if (std::abs(vertex.bulge) > 1e-10) {
            writeGroup(out, 42, vertex.bulge);
        }
    }
}

void DXFWriter::writeEllipse(std::ostream& out, const DXFEllipse& ellipse) {
    writeGroup(out, 0, "ELLIPSE");

    // Handle
    if (!ellipse.handle.empty()) {
        writeGroup(out, 5, ellipse.handle);
    }

    // Layer
    writeGroup(out, 8, ellipse.layer);

    // Color number
    writeGroup(out, 62, ellipse.colorNumber);

    // Center point
    writeGroup(out, 10, ellipse.centerX);
    writeGroup(out, 20, ellipse.centerY);
    writeGroup(out, 30, ellipse.centerZ);

    // Major axis endpoint (relative to center)
    writeGroup(out, 11, ellipse.majorAxisX);
    writeGroup(out, 21, ellipse.majorAxisY);
    writeGroup(out, 31, ellipse.majorAxisZ);

    // Minor to major axis ratio
    writeGroup(out, 40, ellipse.minorAxisRatio);

    // Start and end parameters (radians)
    writeGroup(out, 41, ellipse.startParameter);
    writeGroup(out, 42, ellipse.endParameter);
}

void DXFWriter::writePoint(std::ostream& out, const DXFPoint& point) {
    writeGroup(out, 0, "POINT");

    // Handle
    if (!point.handle.empty()) {
        writeGroup(out, 5, point.handle);
    }

    // Layer
    writeGroup(out, 8, point.layer);

    // Color number
    writeGroup(out, 62, point.colorNumber);

    // Point location
    writeGroup(out, 10, point.x);
    writeGroup(out, 20, point.y);
    writeGroup(out, 30, point.z);
}

void DXFWriter::writeSolid(std::ostream& out, const DXFSolid& solid) {
    writeGroup(out, 0, "SOLID");

    // Handle
    if (!solid.handle.empty()) {
        writeGroup(out, 5, solid.handle);
    }

    // Layer
    writeGroup(out, 8, solid.layer);

    // Color number
    writeGroup(out, 62, solid.colorNumber);

    // First corner
    writeGroup(out, 10, solid.x1);
    writeGroup(out, 20, solid.y1);
    writeGroup(out, 30, solid.z1);

    // Second corner
    writeGroup(out, 11, solid.x2);
    writeGroup(out, 21, solid.y2);
    writeGroup(out, 31, solid.z2);

    // Third corner
    writeGroup(out, 12, solid.x3);
    writeGroup(out, 22, solid.y3);
    writeGroup(out, 32, solid.z3);

    // Fourth corner (if quad, not triangle)
    if (!solid.isTriangle) {
        writeGroup(out, 13, solid.x4);
        writeGroup(out, 23, solid.y4);
        writeGroup(out, 33, solid.z4);
    }
}

// ============================================================================
// HELPER METHODS
// ============================================================================

void DXFWriter::writeGroup(std::ostream& out, int code, const std::string& value) {
    out << std::setw(3) << std::setfill(' ') << code << "\n";
    out << value << "\n";
}

void DXFWriter::writeGroup(std::ostream& out, int code, int value) {
    out << std::setw(3) << std::setfill(' ') << code << "\n";
    out << std::setw(6) << std::setfill(' ') << value << "\n";
}

void DXFWriter::writeGroup(std::ostream& out, int code, double value) {
    out << std::setw(3) << std::setfill(' ') << code << "\n";

    // High precision: 15 decimal places (matches double precision)
    // Use fixed notation to avoid scientific notation for small values
    out << std::fixed << std::setprecision(15) << value << "\n";
}

std::vector<std::string> DXFWriter::extractLayers(const std::vector<DXFEntity>& entities) {
    std::set<std::string> layerSet;

    // Extract layer names from all entities
    for (const auto& entity : entities) {
        std::string layerName;

        switch (entity.type) {
            case DXFEntityType::Line:
                layerName = std::get<DXFLine>(entity.data).layer;
                break;
            case DXFEntityType::Arc:
                layerName = std::get<DXFArc>(entity.data).layer;
                break;
            case DXFEntityType::Circle:
                layerName = std::get<DXFCircle>(entity.data).layer;
                break;
            case DXFEntityType::LWPolyline:
                layerName = std::get<DXFLWPolyline>(entity.data).layer;
                break;
            case DXFEntityType::Ellipse:
                layerName = std::get<DXFEllipse>(entity.data).layer;
                break;
            case DXFEntityType::Point:
                layerName = std::get<DXFPoint>(entity.data).layer;
                break;
            case DXFEntityType::Solid:
                layerName = std::get<DXFSolid>(entity.data).layer;
                break;
            default:
                break;
        }

        if (!layerName.empty()) {
            layerSet.insert(layerName);
        }
    }

    // Ensure "0" layer always exists (DXF default)
    layerSet.insert("0");

    return std::vector<std::string>(layerSet.begin(), layerSet.end());
}

} // namespace Export
} // namespace OwnCAD
