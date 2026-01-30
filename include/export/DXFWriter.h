#pragma once

#include "import/DXFEntity.h"
#include <string>
#include <vector>
#include <ostream>

namespace OwnCAD {
namespace Export {

/**
 * @brief DXF file writer - converts DXF entities to DXF text format
 *
 * Design principles:
 * - Write minimal valid DXF structure (HEADER, TABLES, ENTITIES, EOF)
 * - Target DXF R2018 AC1032 format (modern, widely supported)
 * - Preserve handles, layers, colors exactly as imported
 * - Use high-precision formatting (15 decimal places) for coordinates
 * - Write clean, readable DXF with proper formatting
 *
 * DXF Structure:
 * - SECTION HEADER: Metadata (version, units, etc.)
 * - SECTION TABLES: Layers, linetypes, etc.
 * - SECTION ENTITIES: Geometry entities
 * - EOF: End of file
 *
 * CRITICAL: This is the inverse of DXFParser. Must produce output
 * that DXFParser can re-import without data loss.
 */
class DXFWriter {
public:
    /**
     * @brief Write DXF entities to file
     * @param filePath Output DXF file path
     * @param entities DXF entities to write
     * @return true if successful, false on file write error
     *
     * Writes a complete, valid DXF file with all sections.
     * All entities must have valid handles, layers, and geometry.
     */
    static bool writeFile(
        const std::string& filePath,
        const std::vector<Import::DXFEntity>& entities
    );

    /**
     * @brief Write DXF entities to stream
     * @param out Output stream
     * @param entities DXF entities to write
     * @return true if successful, false on stream error
     *
     * For testing and in-memory DXF generation.
     */
    static bool writeStream(
        std::ostream& out,
        const std::vector<Import::DXFEntity>& entities
    );

private:
    /**
     * @brief Write DXF HEADER section
     *
     * Contains:
     * - DXF version (AC1032 = AutoCAD 2018)
     * - Database version
     * - Minimal required header variables
     */
    static void writeHeader(std::ostream& out);

    /**
     * @brief Write DXF TABLES section
     *
     * Contains:
     * - Layer table (extracted from entities)
     * - Line type table (CONTINUOUS, BYLAYER)
     */
    static void writeTables(std::ostream& out, const std::vector<Import::DXFEntity>& entities);

    /**
     * @brief Write DXF ENTITIES section
     *
     * Writes all geometry entities with full metadata.
     */
    static void writeEntities(std::ostream& out, const std::vector<Import::DXFEntity>& entities);

    /**
     * @brief Write EOF marker
     */
    static void writeFooter(std::ostream& out);

    // ========================================================================
    // ENTITY WRITERS
    // ========================================================================

    /**
     * @brief Write LINE entity
     */
    static void writeLine(std::ostream& out, const Import::DXFLine& line);

    /**
     * @brief Write ARC entity
     */
    static void writeArc(std::ostream& out, const Import::DXFArc& arc);

    /**
     * @brief Write CIRCLE entity
     */
    static void writeCircle(std::ostream& out, const Import::DXFCircle& circle);

    /**
     * @brief Write LWPOLYLINE entity
     */
    static void writeLWPolyline(std::ostream& out, const Import::DXFLWPolyline& polyline);

    /**
     * @brief Write ELLIPSE entity
     */
    static void writeEllipse(std::ostream& out, const Import::DXFEllipse& ellipse);

    /**
     * @brief Write POINT entity
     */
    static void writePoint(std::ostream& out, const Import::DXFPoint& point);

    /**
     * @brief Write SOLID entity
     */
    static void writeSolid(std::ostream& out, const Import::DXFSolid& solid);

    // ========================================================================
    // HELPER METHODS
    // ========================================================================

    /**
     * @brief Write group code + value pair
     * @param out Output stream
     * @param code DXF group code
     * @param value String value
     */
    static void writeGroup(std::ostream& out, int code, const std::string& value);

    /**
     * @brief Write group code + integer value
     */
    static void writeGroup(std::ostream& out, int code, int value);

    /**
     * @brief Write group code + double value (high precision)
     */
    static void writeGroup(std::ostream& out, int code, double value);

    /**
     * @brief Extract unique layer names from entities
     */
    static std::vector<std::string> extractLayers(const std::vector<Import::DXFEntity>& entities);
};

} // namespace Export
} // namespace OwnCAD
