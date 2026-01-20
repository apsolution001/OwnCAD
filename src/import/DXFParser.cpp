#include "import/DXFParser.h"
#include <sstream>
#include <cctype>
#include <cmath>
#include <limits>

namespace OwnCAD {
namespace Import {

// ============================================================================
// PUBLIC API
// ============================================================================

DXFParseResult DXFParser::parseFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::in);

    if (!file.is_open()) {
        DXFParseResult result;
        result.success = false;
        result.errors.push_back("Failed to open file: " + filePath);
        return result;
    }

    return parse(file);
}

DXFParseResult DXFParser::parseString(const std::string& content) {
    std::istringstream stream(content);
    return parse(stream);
}

// ============================================================================
// CORE PARSING
// ============================================================================

DXFParseResult DXFParser::parse(std::istream& input) {
    ParserState state;

    int code;
    std::string value;

    while (true) {
        // Check if we have a lookahead group to process first
        if (state.lookahead.valid) {
            code = state.lookahead.code;
            value = state.lookahead.value;
            state.lookahead.valid = false;
        } else {
            // Read next group from stream
            if (!readGroup(input, code, value, state.lineNumber)) {
                break;  // End of stream
            }
        }

        // Section markers
        if (code == 0) {
            if (value == "SECTION") {
                // Read section name
                if (readGroup(input, code, value, state.lineNumber) && code == 2) {
                    state.currentSection = value;
                    if (value == "ENTITIES") {
                        state.inEntitiesSection = true;
                    }
                }
            }
            else if (value == "ENDSEC") {
                if (state.currentSection == "ENTITIES") {
                    state.inEntitiesSection = false;
                }
                state.currentSection.clear();
            }
            else if (value == "EOF") {
                break;  // End of file
            }
            // Entity types
            else if (state.inEntitiesSection) {
                // Parse the entity - parseLine/Arc/Circle will consume groups
                // and set state.lookahead when they encounter the next entity
                auto entity = parseEntity(input, value, state);
                if (entity.has_value()) {
                    state.result.entities.push_back(entity.value());
                    state.result.totalEntities++;
                } else {
                    state.result.skippedEntities++;
                }
                // Lookahead will be processed in next iteration
            }
        }
    }

    state.result.success = state.result.errors.empty();
    return state.result;
}

bool DXFParser::readGroup(std::istream& input, int& code, std::string& value, size_t& lineNumber) {
    std::string codeLine;

    // Read group code
    if (!std::getline(input, codeLine)) {
        return false;
    }
    lineNumber++;
    codeLine = trim(codeLine);

    if (!stringToInt(codeLine, code)) {
        return false;
    }

    // Read group value
    if (!std::getline(input, value)) {
        return false;
    }
    lineNumber++;
    value = trim(value);

    return true;
}

// ============================================================================
// ENTITY PARSING
// ============================================================================

std::optional<DXFEntity> DXFParser::parseEntity(
    std::istream& input,
    const std::string& entityType,
    ParserState& state
) {
    if (entityType == "LINE") {
        return parseLine(input, state);
    }
    else if (entityType == "ARC") {
        return parseArc(input, state);
    }
    else if (entityType == "CIRCLE") {
        return parseCircle(input, state);
    }
    else if (entityType == "LWPOLYLINE") {
        return parseLWPolyline(input, state);
    }
    else if (entityType == "ELLIPSE") {
        return parseEllipse(input, state);
    }
    else if (entityType == "SPLINE") {
        return parseSpline(input, state);
    }
    else if (entityType == "POINT") {
        return parsePoint(input, state);
    }
    else if (entityType == "SOLID") {
        return parseSolid(input, state);
    }
    else {
        // Unsupported entity - skip it
        skipEntity(input, state);
        return std::nullopt;
    }
}

std::optional<DXFEntity> DXFParser::parseLine(std::istream& input, ParserState& state) {
    DXFLine line;
    int code;
    std::string value;
    size_t startLine = state.lineNumber;

    // Read groups until we hit code 0 (next entity) or end of stream
    while (readGroup(input, code, value, state.lineNumber)) {
        if (code == 0) {
            // Save for next entity parsing
            state.lookahead = GroupPair(code, value);
            break;
        }

        switch (code) {
            case 8:  // Layer
                line.layer = value;
                break;
            case 5:  // Handle
                line.handle = value;
                break;
            case 62:  // Color number
                stringToInt(value, line.colorNumber);
                break;
            case 10:  // Start X
                if (!stringToDouble(value, line.startX) || !isValidNumber(line.startX)) {
                    state.result.errors.push_back(
                        "LINE: Invalid start X coordinate at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                break;
            case 20:  // Start Y
                if (!stringToDouble(value, line.startY) || !isValidNumber(line.startY)) {
                    state.result.errors.push_back(
                        "LINE: Invalid start Y coordinate at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                break;
            case 30:  // Start Z
                stringToDouble(value, line.startZ);
                break;
            case 11:  // End X
                if (!stringToDouble(value, line.endX) || !isValidNumber(line.endX)) {
                    state.result.errors.push_back(
                        "LINE: Invalid end X coordinate at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                break;
            case 21:  // End Y
                if (!stringToDouble(value, line.endY) || !isValidNumber(line.endY)) {
                    state.result.errors.push_back(
                        "LINE: Invalid end Y coordinate at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                break;
            case 31:  // End Z
                stringToDouble(value, line.endZ);
                break;
        }
    }

    DXFEntity entity;
    entity.type = DXFEntityType::Line;
    entity.data = line;
    entity.lineNumber = startLine;

    return entity;
}

std::optional<DXFEntity> DXFParser::parseArc(std::istream& input, ParserState& state) {
    DXFArc arc;
    int code;
    std::string value;
    size_t startLine = state.lineNumber;

    while (readGroup(input, code, value, state.lineNumber)) {
        if (code == 0) {
            // Save for next entity
            state.lookahead = GroupPair(code, value);
            break;
        }

        switch (code) {
            case 8:  // Layer
                arc.layer = value;
                break;
            case 5:  // Handle
                arc.handle = value;
                break;
            case 62:  // Color number
                stringToInt(value, arc.colorNumber);
                break;
            case 10:  // Center X
                if (!stringToDouble(value, arc.centerX) || !isValidNumber(arc.centerX)) {
                    state.result.errors.push_back(
                        "ARC: Invalid center X coordinate at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                break;
            case 20:  // Center Y
                if (!stringToDouble(value, arc.centerY) || !isValidNumber(arc.centerY)) {
                    state.result.errors.push_back(
                        "ARC: Invalid center Y coordinate at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                break;
            case 30:  // Center Z
                stringToDouble(value, arc.centerZ);
                break;
            case 40:  // Radius
                if (!stringToDouble(value, arc.radius) || !isValidNumber(arc.radius)) {
                    state.result.errors.push_back(
                        "ARC: Invalid radius at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                break;
            case 50:  // Start angle
                if (!stringToDouble(value, arc.startAngle) || !isValidNumber(arc.startAngle)) {
                    state.result.errors.push_back(
                        "ARC: Invalid start angle at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                break;
            case 51:  // End angle
                if (!stringToDouble(value, arc.endAngle) || !isValidNumber(arc.endAngle)) {
                    state.result.errors.push_back(
                        "ARC: Invalid end angle at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                break;
        }
    }

    DXFEntity entity;
    entity.type = DXFEntityType::Arc;
    entity.data = arc;
    entity.lineNumber = startLine;

    return entity;
}

std::optional<DXFEntity> DXFParser::parseCircle(std::istream& input, ParserState& state) {
    DXFCircle circle;
    int code;
    std::string value;
    size_t startLine = state.lineNumber;

    while (readGroup(input, code, value, state.lineNumber)) {
        if (code == 0) {
            // Save for next entity
            state.lookahead = GroupPair(code, value);
            break;
        }

        switch (code) {
            case 8:  // Layer
                circle.layer = value;
                break;
            case 5:  // Handle
                circle.handle = value;
                break;
            case 62:  // Color number
                stringToInt(value, circle.colorNumber);
                break;
            case 10:  // Center X
                if (!stringToDouble(value, circle.centerX) || !isValidNumber(circle.centerX)) {
                    state.result.errors.push_back(
                        "CIRCLE: Invalid center X coordinate at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                break;
            case 20:  // Center Y
                if (!stringToDouble(value, circle.centerY) || !isValidNumber(circle.centerY)) {
                    state.result.errors.push_back(
                        "CIRCLE: Invalid center Y coordinate at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                break;
            case 30:  // Center Z
                stringToDouble(value, circle.centerZ);
                break;
            case 40:  // Radius
                if (!stringToDouble(value, circle.radius) || !isValidNumber(circle.radius)) {
                    state.result.errors.push_back(
                        "CIRCLE: Invalid radius at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                break;
        }
    }

    DXFEntity entity;
    entity.type = DXFEntityType::Circle;
    entity.data = circle;
    entity.lineNumber = startLine;

    return entity;
}

std::optional<DXFEntity> DXFParser::parseLWPolyline(std::istream& input, ParserState& state) {
    DXFLWPolyline polyline;
    int code;
    std::string value;
    size_t startLine = state.lineNumber;
    int numVertices = 0;

    // Current vertex being constructed
    DXFVertex currentVertex;
    bool hasX = false;
    bool hasY = false;

    while (readGroup(input, code, value, state.lineNumber)) {
        if (code == 0) {
            // Save for next entity
            state.lookahead = GroupPair(code, value);
            break;
        }

        switch (code) {
            case 8:  // Layer
                polyline.layer = value;
                break;
            case 5:  // Handle
                polyline.handle = value;
                break;
            case 70:  // Flags (1 = closed)
                {
                    int flags;
                    if (stringToInt(value, flags)) {
                        polyline.closed = (flags & 1) != 0;
                    }
                }
                break;
            case 62:  // Color number
                stringToInt(value, polyline.colorNumber);
                break;
            case 90:  // Number of vertices
                if (!stringToInt(value, numVertices)) {
                    state.result.errors.push_back(
                        "LWPOLYLINE: Invalid vertex count at line " + std::to_string(state.lineNumber)
                    );
                }
                break;
            case 10:  // Vertex X
                // If we have a complete vertex, save it
                if (hasX && hasY) {
                    polyline.vertices.push_back(currentVertex);
                    currentVertex = DXFVertex();  // Reset
                    hasX = hasY = false;
                }
                if (!stringToDouble(value, currentVertex.x) || !isValidNumber(currentVertex.x)) {
                    state.result.errors.push_back(
                        "LWPOLYLINE: Invalid X coordinate at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                hasX = true;
                break;
            case 20:  // Vertex Y
                if (!stringToDouble(value, currentVertex.y) || !isValidNumber(currentVertex.y)) {
                    state.result.errors.push_back(
                        "LWPOLYLINE: Invalid Y coordinate at line " + std::to_string(state.lineNumber)
                    );
                    skipEntity(input, state);
                    return std::nullopt;
                }
                hasY = true;
                break;
            case 42:  // Bulge (for arc segments)
                stringToDouble(value, currentVertex.bulge);
                break;
        }
    }

    // Save last vertex if complete
    if (hasX && hasY) {
        polyline.vertices.push_back(currentVertex);
    }

    // Validate: must have at least 2 vertices
    if (polyline.vertices.size() < 2) {
        state.result.errors.push_back(
            "LWPOLYLINE: Too few vertices (" + std::to_string(polyline.vertices.size()) +
            ") at line " + std::to_string(startLine)
        );
        return std::nullopt;
    }

    DXFEntity entity;
    entity.type = DXFEntityType::LWPolyline;
    entity.data = polyline;
    entity.lineNumber = startLine;

    return entity;
}

std::optional<DXFEntity> DXFParser::parseEllipse(std::istream& input, ParserState& state) {
    DXFEllipse ellipse;
    int code;
    std::string value;
    size_t startLine = state.lineNumber;

    while (readGroup(input, code, value, state.lineNumber)) {
        if (code == 0) {
            state.lookahead = GroupPair(code, value);
            break;
        }

        double numValue;
        switch (code) {
            case 8:  // Layer
                ellipse.layer = value;
                break;
            case 5:  // Handle
                ellipse.handle = value;
                break;
            case 62: // Color number
                if (stringToInt(value, ellipse.colorNumber)) {
                    // Valid
                }
                break;
            case 10: // Center X
                if (stringToDouble(value, numValue)) ellipse.centerX = numValue;
                break;
            case 20: // Center Y
                if (stringToDouble(value, numValue)) ellipse.centerY = numValue;
                break;
            case 30: // Center Z
                if (stringToDouble(value, numValue)) ellipse.centerZ = numValue;
                break;
            case 11: // Major axis endpoint X (relative to center)
                if (stringToDouble(value, numValue)) ellipse.majorAxisX = numValue;
                break;
            case 21: // Major axis endpoint Y (relative to center)
                if (stringToDouble(value, numValue)) ellipse.majorAxisY = numValue;
                break;
            case 31: // Major axis endpoint Z (relative to center)
                if (stringToDouble(value, numValue)) ellipse.majorAxisZ = numValue;
                break;
            case 40: // Minor to major axis ratio
                if (stringToDouble(value, numValue)) ellipse.minorAxisRatio = numValue;
                break;
            case 41: // Start parameter
                if (stringToDouble(value, numValue)) ellipse.startParameter = numValue;
                break;
            case 42: // End parameter
                if (stringToDouble(value, numValue)) ellipse.endParameter = numValue;
                break;
        }
    }

    DXFEntity entity;
    entity.type = DXFEntityType::Ellipse;
    entity.data = ellipse;
    entity.lineNumber = startLine;

    return entity;
}

std::optional<DXFEntity> DXFParser::parseSpline(std::istream& input, ParserState& state) {
    DXFSpline spline;
    int code;
    std::string value;
    size_t startLine = state.lineNumber;

    DXFVertex currentVertex;
    bool hasVertexData = false;

    while (readGroup(input, code, value, state.lineNumber)) {
        if (code == 0) {
            state.lookahead = GroupPair(code, value);
            break;
        }

        double numValue;
        int intValue;
        switch (code) {
            case 8:  // Layer
                spline.layer = value;
                break;
            case 5:  // Handle
                spline.handle = value;
                break;
            case 62: // Color number
                if (stringToInt(value, spline.colorNumber)) {
                    // Valid
                }
                break;
            case 70: // Spline flag
                if (stringToInt(value, intValue)) {
                    spline.closed = (intValue & 1) != 0;
                    spline.periodic = (intValue & 2) != 0;
                    spline.rational = (intValue & 4) != 0;
                }
                break;
            case 71: // Degree
                if (stringToInt(value, spline.degree)) {
                    // Valid
                }
                break;
            case 72: // Number of knots
                // Just informational, knots will be read via code 40
                break;
            case 73: // Number of control points
                // Just informational, points will be read via codes 10/20/30
                break;
            case 10: // Control point X
                if (hasVertexData) {
                    spline.controlPoints.push_back(currentVertex);
                }
                currentVertex = DXFVertex();
                if (stringToDouble(value, numValue)) currentVertex.x = numValue;
                hasVertexData = true;
                break;
            case 20: // Control point Y
                if (stringToDouble(value, numValue)) currentVertex.y = numValue;
                break;
            case 30: // Control point Z
                if (stringToDouble(value, numValue)) currentVertex.z = numValue;
                break;
            case 40: // Knot value
                if (stringToDouble(value, numValue)) {
                    spline.knots.push_back(numValue);
                }
                break;
        }
    }

    // Add last vertex if exists
    if (hasVertexData) {
        spline.controlPoints.push_back(currentVertex);
    }

    DXFEntity entity;
    entity.type = DXFEntityType::Spline;
    entity.data = spline;
    entity.lineNumber = startLine;

    return entity;
}

std::optional<DXFEntity> DXFParser::parsePoint(std::istream& input, ParserState& state) {
    DXFPoint point;
    int code;
    std::string value;
    size_t startLine = state.lineNumber;

    while (readGroup(input, code, value, state.lineNumber)) {
        if (code == 0) {
            state.lookahead = GroupPair(code, value);
            break;
        }

        double numValue;
        switch (code) {
            case 8:  // Layer
                point.layer = value;
                break;
            case 5:  // Handle
                point.handle = value;
                break;
            case 62: // Color number
                if (stringToInt(value, point.colorNumber)) {
                    // Valid
                }
                break;
            case 10: // X
                if (stringToDouble(value, numValue)) point.x = numValue;
                break;
            case 20: // Y
                if (stringToDouble(value, numValue)) point.y = numValue;
                break;
            case 30: // Z
                if (stringToDouble(value, numValue)) point.z = numValue;
                break;
        }
    }

    DXFEntity entity;
    entity.type = DXFEntityType::Point;
    entity.data = point;
    entity.lineNumber = startLine;

    return entity;
}

std::optional<DXFEntity> DXFParser::parseSolid(std::istream& input, ParserState& state) {
    DXFSolid solid;
    int code;
    std::string value;
    size_t startLine = state.lineNumber;
    bool hasPoint4 = false;

    while (readGroup(input, code, value, state.lineNumber)) {
        if (code == 0) {
            state.lookahead = GroupPair(code, value);
            break;
        }

        double numValue;
        switch (code) {
            case 8:  // Layer
                solid.layer = value;
                break;
            case 5:  // Handle
                solid.handle = value;
                break;
            case 62: // Color number
                if (stringToInt(value, solid.colorNumber)) {
                    // Valid
                }
                break;
            case 10: // First corner X
                if (stringToDouble(value, numValue)) solid.x1 = numValue;
                break;
            case 20: // First corner Y
                if (stringToDouble(value, numValue)) solid.y1 = numValue;
                break;
            case 30: // First corner Z
                if (stringToDouble(value, numValue)) solid.z1 = numValue;
                break;
            case 11: // Second corner X
                if (stringToDouble(value, numValue)) solid.x2 = numValue;
                break;
            case 21: // Second corner Y
                if (stringToDouble(value, numValue)) solid.y2 = numValue;
                break;
            case 31: // Second corner Z
                if (stringToDouble(value, numValue)) solid.z2 = numValue;
                break;
            case 12: // Third corner X
                if (stringToDouble(value, numValue)) solid.x3 = numValue;
                break;
            case 22: // Third corner Y
                if (stringToDouble(value, numValue)) solid.y3 = numValue;
                break;
            case 32: // Third corner Z
                if (stringToDouble(value, numValue)) solid.z3 = numValue;
                break;
            case 13: // Fourth corner X (optional)
                if (stringToDouble(value, numValue)) {
                    solid.x4 = numValue;
                    hasPoint4 = true;
                }
                break;
            case 23: // Fourth corner Y (optional)
                if (stringToDouble(value, numValue)) solid.y4 = numValue;
                break;
            case 33: // Fourth corner Z (optional)
                if (stringToDouble(value, numValue)) solid.z4 = numValue;
                break;
        }
    }

    solid.isTriangle = !hasPoint4;

    DXFEntity entity;
    entity.type = DXFEntityType::Solid;
    entity.data = solid;
    entity.lineNumber = startLine;

    return entity;
}

void DXFParser::skipEntity(std::istream& input, ParserState& state) {
    int code;
    std::string value;

    while (readGroup(input, code, value, state.lineNumber)) {
        if (code == 0) {
            // Start of next entity - save it
            state.lookahead = GroupPair(code, value);
            break;
        }
    }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

bool DXFParser::stringToDouble(const std::string& str, double& result) {
    try {
        size_t pos;
        result = std::stod(str, &pos);
        return pos == str.length();
    } catch (...) {
        return false;
    }
}

bool DXFParser::stringToInt(const std::string& str, int& result) {
    try {
        size_t pos;
        result = std::stoi(str, &pos);
        return pos == str.length();
    } catch (...) {
        return false;
    }
}

std::string DXFParser::trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();

    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }

    return str.substr(start, end - start);
}

bool DXFParser::isValidNumber(double value) {
    return std::isfinite(value);
}

} // namespace Import
} // namespace OwnCAD
