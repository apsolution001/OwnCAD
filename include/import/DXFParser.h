#pragma once

#include "DXFEntity.h"
#include <string>
#include <fstream>
#include <memory>

namespace OwnCAD {
namespace Import {

/**
 * @brief DXF file parser
 *
 * Parses DXF (Drawing Exchange Format) files and extracts geometric entities.
 *
 * Supported DXF versions: R12, R2000, R2004, R2007, R2010, R2013
 * Supported entities: LINE, ARC, CIRCLE
 *
 * Design principles:
 * - Robust: Handles malformed DXF files gracefully
 * - Informative: Provides detailed error messages with line numbers
 * - Minimal: Only parses what's needed for validation (2D geometry)
 * - Safe: Validates all numeric conversions
 *
 * DXF Structure:
 * - Group code (integer on odd line)
 * - Group value (string on even line)
 * - Entities in ENTITIES section between "ENDSEC" markers
 *
 * Example DXF:
 * ```
 * 0
 * LINE
 * 8
 * LayerName
 * 10
 * 0.0
 * 20
 * 0.0
 * 11
 * 100.0
 * 21
 * 100.0
 * ```
 */
class DXFParser {
public:
    /**
     * @brief Parse DXF file from path
     * @param filePath Absolute path to DXF file
     * @return Parse result with entities or errors
     */
    static DXFParseResult parseFile(const std::string& filePath);

    /**
     * @brief Parse DXF content from string
     * @param content DXF file content
     * @return Parse result with entities or errors
     */
    static DXFParseResult parseString(const std::string& content);

private:
    /**
     * @brief Internal parser state
     */
    struct ParserState {
        size_t lineNumber;
        std::string currentSection;
        bool inEntitiesSection;
        DXFParseResult result;

        ParserState()
            : lineNumber(0)
            , inEntitiesSection(false) {}
    };

    /**
     * @brief Parse DXF from input stream
     */
    static DXFParseResult parse(std::istream& input);

    /**
     * @brief Read group code and value pair
     */
    static bool readGroup(std::istream& input, int& code, std::string& value, size_t& lineNumber);

    /**
     * @brief Parse single entity starting at current position
     */
    static std::optional<DXFEntity> parseEntity(
        std::istream& input,
        const std::string& entityType,
        ParserState& state
    );

    /**
     * @brief Parse LINE entity
     */
    static std::optional<DXFEntity> parseLine(std::istream& input, ParserState& state);

    /**
     * @brief Parse ARC entity
     */
    static std::optional<DXFEntity> parseArc(std::istream& input, ParserState& state);

    /**
     * @brief Parse CIRCLE entity
     */
    static std::optional<DXFEntity> parseCircle(std::istream& input, ParserState& state);

    /**
     * @brief Skip unsupported entity
     */
    static void skipEntity(std::istream& input, ParserState& state);

    /**
     * @brief Safe string to double conversion
     */
    static bool stringToDouble(const std::string& str, double& result);

    /**
     * @brief Safe string to int conversion
     */
    static bool stringToInt(const std::string& str, int& result);

    /**
     * @brief Trim whitespace from string
     */
    static std::string trim(const std::string& str);

    /**
     * @brief Validate numeric value is finite
     */
    static bool isValidNumber(double value);
};

} // namespace Import
} // namespace OwnCAD
