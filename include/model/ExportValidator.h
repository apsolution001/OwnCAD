#pragma once

#include "model/DocumentModel.h"
#include "export/GeometryExporter.h"
#include "import/DXFEntity.h"
#include <vector>
#include <string>

namespace OwnCAD {
namespace Model {

/**
 * @brief Report on entity count validation
 */
struct EntityCountReport {
    size_t importedCount;
    size_t exportedCount;
    bool matches;
    std::vector<std::string> missingHandles;

    EntityCountReport()
        : importedCount(0), exportedCount(0), matches(false) {}
};

/**
 * @brief Report on geometry precision validation
 */
struct PrecisionReport {
    std::vector<std::string> entitiesWithPrecisionLoss;
    double maxDeviation;
    bool withinTolerance;

    PrecisionReport()
        : maxDeviation(0.0), withinTolerance(true) {}
};

/**
 * @brief Report on layer integrity validation
 */
struct LayerReport {
    std::vector<std::string> originalLayers;
    std::vector<std::string> exportedLayers;
    std::vector<std::string> missingLayers;
    std::vector<std::string> extraLayers;
    bool matches;

    LayerReport()
        : matches(false) {}
};

/**
 * @brief Report on handle preservation validation
 */
struct HandleReport {
    std::vector<std::string> originalHandles;
    std::vector<std::string> exportedHandles;
    std::vector<std::string> missingHandles;
    std::vector<std::string> duplicateHandles;
    bool matches;

    HandleReport()
        : matches(false) {}
};

/**
 * @brief Validates DXF export correctness
 *
 * Design principles:
 * - Verify no data loss during export
 * - Check precision preservation (within tolerance)
 * - Validate metadata integrity (handles, layers, colors)
 * - Detect silent failures
 *
 * Usage:
 * 1. Export document to DXF
 * 2. Run validation checks
 * 3. Review reports for issues
 */
class ExportValidator {
public:
    /**
     * @brief Validate entity count matches between import and export
     * @param document Original document model
     * @param exportResult Export result from GeometryExporter
     * @return Report showing count match status
     *
     * Checks:
     * - Total entity count matches
     * - All handles present in export
     * - No missing entities
     */
    static EntityCountReport validateEntityCount(
        const DocumentModel& document,
        const Export::ExportResult& exportResult
    );

    /**
     * @brief Validate geometry precision after export/re-import
     * @param original Original document
     * @param reimported Re-imported document after export
     * @param tolerance Precision tolerance (default: GEOMETRY_EPSILON)
     * @return Report showing precision deviations
     *
     * Checks:
     * - Line endpoints within tolerance
     * - Arc center/radius/angles within tolerance
     * - Ellipse parameters within tolerance
     * - Point coordinates within tolerance
     */
    static PrecisionReport validatePrecision(
        const DocumentModel& original,
        const DocumentModel& reimported,
        double tolerance = 1e-9
    );

    /**
     * @brief Validate layer integrity
     * @param original Original document
     * @param reimported Re-imported document after export
     * @return Report showing layer match status
     *
     * Checks:
     * - All original layers present in export
     * - No extra layers introduced
     * - Layer names exact match (case-sensitive)
     */
    static LayerReport validateLayers(
        const DocumentModel& original,
        const DocumentModel& reimported
    );

    /**
     * @brief Validate handle preservation
     * @param original Original document
     * @param exported Exported DXF entities
     * @return Report showing handle match status
     *
     * Checks:
     * - All original handles present in export
     * - No duplicate handles
     * - Handle format preserved
     */
    static HandleReport validateHandles(
        const DocumentModel& original,
        const std::vector<Import::DXFEntity>& exported
    );

    /**
     * @brief Validate bounding box consistency
     * @param original Original document
     * @param reimported Re-imported document
     * @param tolerance Precision tolerance
     * @return true if bounding boxes are consistent
     */
    static bool validateBoundingBoxes(
        const DocumentModel& original,
        const DocumentModel& reimported,
        double tolerance = 1e-9
    );

private:
    /**
     * @brief Extract handles from DXF entities
     */
    static std::vector<std::string> extractHandles(
        const std::vector<Import::DXFEntity>& entities
    );

    /**
     * @brief Check if two doubles are equal within tolerance
     */
    static bool areEqual(double a, double b, double tolerance) noexcept;
};

} // namespace Model
} // namespace OwnCAD
