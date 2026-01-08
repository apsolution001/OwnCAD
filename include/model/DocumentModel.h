#pragma once

#include "import/GeometryConverter.h"
#include "geometry/GeometryValidator.h"
#include <vector>
#include <string>
#include <memory>

namespace OwnCAD {
namespace Model {

/**
 * @brief Statistics about loaded document
 */
struct DocumentStatistics {
    size_t dxfEntitiesImported;   // Original DXF entities (before decomposition)
    size_t totalSegments;          // Total geometry segments (after polygon decomposition)
    size_t totalLines;             // Line segments
    size_t totalArcs;              // Arc segments
    size_t validEntities;
    size_t invalidEntities;
    size_t zeroLengthLines;
    size_t zeroRadiusArcs;
    size_t numericallyUnstable;

    DocumentStatistics()
        : dxfEntitiesImported(0), totalSegments(0), totalLines(0), totalArcs(0)
        , validEntities(0), invalidEntities(0)
        , zeroLengthLines(0), zeroRadiusArcs(0)
        , numericallyUnstable(0) {}

    // For backwards compatibility
    size_t totalEntities() const { return dxfEntitiesImported; }
};

/**
 * @brief Document model - holds all geometry and validation state
 *
 * This is the core data model for the application. It manages:
 * - Imported DXF geometry
 * - Validation results
 * - Document statistics
 * - Layer information
 *
 * Design principles:
 * - Single source of truth for all geometry
 * - Validation is always up-to-date
 * - Immutable after import (edits create new geometry)
 * - Thread-safe read access
 */
class DocumentModel {
public:
    DocumentModel();
    ~DocumentModel();

    /**
     * @brief Load DXF file and validate geometry
     * @param filePath Path to DXF file
     * @return true if loaded successfully (may have validation warnings)
     */
    bool loadDXFFile(const std::string& filePath);

    /**
     * @brief Clear all geometry
     */
    void clear();

    /**
     * @brief Check if document has geometry loaded
     */
    bool isEmpty() const noexcept { return entities_.empty(); }

    /**
     * @brief Get all entities
     */
    const std::vector<Import::GeometryEntityWithMetadata>& entities() const noexcept {
        return entities_;
    }

    /**
     * @brief Get validation result
     */
    const Geometry::ValidationResult& validationResult() const noexcept {
        return validationResult_;
    }

    /**
     * @brief Get document statistics
     */
    const DocumentStatistics& statistics() const noexcept {
        return statistics_;
    }

    /**
     * @brief Get source file path
     */
    const std::string& filePath() const noexcept {
        return filePath_;
    }

    /**
     * @brief Get import errors
     */
    const std::vector<std::string>& importErrors() const noexcept {
        return importErrors_;
    }

    /**
     * @brief Get import warnings
     */
    const std::vector<std::string>& importWarnings() const noexcept {
        return importWarnings_;
    }

    /**
     * @brief Check if document is valid (no critical errors)
     */
    bool isValid() const noexcept {
        return validationResult_.passed() && importErrors_.empty();
    }

    /**
     * @brief Get list of unique layers
     */
    std::vector<std::string> getLayers() const;

private:
    /**
     * @brief Run validation on all entities
     */
    void runValidation();

    /**
     * @brief Calculate document statistics
     */
    void calculateStatistics();

    /**
     * @brief Convert entities to variant for validation
     */
    std::vector<std::variant<Geometry::Line2D, Geometry::Arc2D>>
        getEntityVariants() const;

    // Data members
    std::vector<Import::GeometryEntityWithMetadata> entities_;
    Geometry::ValidationResult validationResult_;
    DocumentStatistics statistics_;
    std::string filePath_;
    std::vector<std::string> importErrors_;
    std::vector<std::string> importWarnings_;
};

} // namespace Model
} // namespace OwnCAD
