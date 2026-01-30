#pragma once

#include "import/GeometryConverter.h"
#include "geometry/GeometryValidator.h"
#include <vector>
#include <string>
#include <memory>
#include <future>
#include <atomic>
#include <functional>
#include <mutex>

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

    // =========================================================================
    // ENTITY CREATION (for drawing tools)
    // =========================================================================

    /**
     * @brief Add a line entity to the document
     * @param line Valid Line2D geometry
     * @param layer Target layer (default: "0")
     * @return Generated handle string, empty on failure
     */
    std::string addLine(const Geometry::Line2D& line, const std::string& layer = "0");

    /**
     * @brief Add an arc entity to the document
     * @param arc Valid Arc2D geometry
     * @param layer Target layer (default: "0")
     * @return Generated handle string, empty on failure
     */
    std::string addArc(const Geometry::Arc2D& arc, const std::string& layer = "0");

    /**
     * @brief Generate next unique entity handle
     * @return Handle string (e.g., "E00001")
     */
    std::string generateHandle();

    // =========================================================================
    // ENTITY MODIFICATION (for transformation tools)
    // =========================================================================

    /**
     * @brief Find entity by handle
     * @param handle Entity handle string
     * @return Pointer to entity, nullptr if not found
     */
    Import::GeometryEntityWithMetadata* findEntityByHandle(const std::string& handle);
    const Import::GeometryEntityWithMetadata* findEntityByHandle(const std::string& handle) const;

    /**
     * @brief Get index of entity by handle
     * @param handle Entity handle string
     * @return Index in entities_ vector, or std::nullopt if not found
     */
    std::optional<size_t> findEntityIndexByHandle(const std::string& handle) const;

    /**
     * @brief Update entity geometry (preserves metadata)
     * @param handle Entity handle
     * @param newGeometry New geometry to replace existing
     * @return true if entity found and updated
     */
    bool updateEntity(const std::string& handle, const Import::GeometryEntity& newGeometry);

    /**
     * @brief Remove entity from document
     * @param handle Entity handle
     * @return true if entity found and removed
     */
    bool removeEntity(const std::string& handle);

    /**
     * @brief Restore a previously removed entity (for undo support)
     * @param entity Complete entity with metadata (handle, layer, color, etc.)
     * @return true if restored successfully, false if handle conflict exists
     *
     * This method reuses the original handle. If an entity with the same
     * handle already exists, the operation fails (indicates a bug in
     * command undo/redo logic).
     */
    bool restoreEntity(const Import::GeometryEntityWithMetadata& entity);

    /**
     * @brief Restore a previously removed entity at a specific index (for undo support)
     * @param entity Complete entity with metadata
     * @param index Original index in the entities vector
     * @return true if restored successfully
     */
    bool restoreEntityAtIndex(const Import::GeometryEntityWithMetadata& entity, size_t index);

    /**
     * @brief Add an ellipse entity to the document
     * @param ellipse Valid Ellipse2D geometry
     * @param layer Target layer (default: "0")
     * @return Generated handle string, empty on failure
     */
    std::string addEllipse(const Geometry::Ellipse2D& ellipse, const std::string& layer = "0");

    /**
     * @brief Add a point entity to the document
     * @param point Valid Point2D geometry
     * @param layer Target layer (default: "0")
     * @return Generated handle string, empty on failure
     */
    std::string addPoint(const Geometry::Point2D& point, const std::string& layer = "0");

    /**
     * @brief Run validation asynchronously (non-blocking)
     *
     * Spawns a background task to validate all entities.
     * When complete, updates validationResult and calls the completion callback.
     */
    void runValidationAsync();

    /**
     * @brief Set callback to be notified when validation completes
     * @param callback Function to call (NOTE: may be called from background thread)
     */
    void setValidationCompletionCallback(std::function<void(const Geometry::ValidationResult&)> callback);

    /**
     * @brief Apply validation result and update statistics (Main Thread Only)
     * @param result Result computed by async validator
     */
    void finalizeValidation(const Geometry::ValidationResult& result);

    /**
     * @brief Check if validation is currently running
     */
    bool isValidating() const;

    // =========================================================================
    // DXF EXPORT
    // =========================================================================

    /**
     * @brief Export document to DXF file
     * @param filePath Output DXF file path
     * @return true if successful, false on error
     *
     * Exports all entities with preserved metadata (handles, layers, colors).
     * Uses GeometryExporter and DXFWriter for conversion.
     */
    bool exportDXFFile(const std::string& filePath) const;

    /**
     * @brief Get export errors from last export operation
     */
    const std::vector<std::string>& exportErrors() const noexcept;

    /**
     * @brief Update next handle number by scanning existing entities
     *
     * Should be called after importing a DXF to avoid handle conflicts.
     * Scans all entities and sets nextHandleNumber_ to max(handle) + 1.
     * Handles both decimal ("E00001") and hex formats.
     */
    void updateNextHandleNumber();

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

    /**
     * @brief Extract entity handles (parallel to getEntityVariants)
     */
    std::vector<std::string> getEntityHandles() const;

    // Data members
    std::vector<Import::GeometryEntityWithMetadata> entities_;
    Geometry::ValidationResult validationResult_;
    mutable std::mutex validationMutex_; // Protect validationResult_ access
    DocumentStatistics statistics_;
    std::string filePath_;
    std::vector<std::string> importErrors_;
    std::vector<std::string> importWarnings_;
    mutable std::vector<std::string> exportErrors_;

    // Handle generation
    size_t nextHandleNumber_ = 1;

    // Async validation
    std::future<void> validationFuture_;
    std::atomic<bool> isValidating_{false};
    std::function<void(const Geometry::ValidationResult&)> validationCallback_;
};

} // namespace Model
} // namespace OwnCAD
