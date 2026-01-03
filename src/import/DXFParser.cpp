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
