#include "model/ExportValidator.h"
#include "geometry/GeometryConstants.h"
#include "geometry/GeometryMath.h"
#include <algorithm>
#include <set>
#include <cmath>

namespace OwnCAD {
namespace Model {

using namespace OwnCAD::Import;
using namespace OwnCAD::Geometry;

// ============================================================================
// ENTITY COUNT VALIDATION
// ============================================================================

EntityCountReport ExportValidator::validateEntityCount(
    const DocumentModel& document,
    const Export::ExportResult& exportResult
) {
    EntityCountReport report;

    report.importedCount = document.entities().size();
    report.exportedCount = exportResult.entities.size();
    report.matches = (report.importedCount == report.exportedCount);

    // Check for missing handles
    if (!report.matches) {
        std::set<std::string> exportedHandles;

        // Collect exported handles
        for (const auto& handle : extractHandles(exportResult.entities)) {
            exportedHandles.insert(handle);
        }

        // Find missing handles
        for (const auto& entity : document.entities()) {
            if (exportedHandles.find(entity.handle) == exportedHandles.end()) {
                report.missingHandles.push_back(entity.handle);
            }
        }
    }

    return report;
}

// ============================================================================
// PRECISION VALIDATION
// ============================================================================

PrecisionReport ExportValidator::validatePrecision(
    const DocumentModel& original,
    const DocumentModel& reimported,
    double tolerance
) {
    PrecisionReport report;
    report.maxDeviation = 0.0;
    report.withinTolerance = true;

    // Check entity counts match first
    if (original.entities().size() != reimported.entities().size()) {
        report.withinTolerance = false;
        report.entitiesWithPrecisionLoss.push_back(
            "Entity count mismatch: " +
            std::to_string(original.entities().size()) + " vs " +
            std::to_string(reimported.entities().size())
        );
        return report;
    }

    // Compare each entity pair
    for (size_t i = 0; i < original.entities().size(); ++i) {
        const auto& origEntity = original.entities()[i];
        const auto& reimpEntity = reimported.entities()[i];

        double deviation = 0.0;
        bool matched = true;

        // Compare based on entity type
        std::visit([&](auto&& origGeom) {
            using T = std::decay_t<decltype(origGeom)>;

            if (std::holds_alternative<T>(reimpEntity.entity)) {
                const auto& reimpGeom = std::get<T>(reimpEntity.entity);

                if constexpr (std::is_same_v<T, Line2D>) {
                    // Compare line endpoints
                    double startDev = GeometryMath::distance(origGeom.start(), reimpGeom.start());
                    double endDev = GeometryMath::distance(origGeom.end(), reimpGeom.end());
                    deviation = std::max(startDev, endDev);

                    if (deviation > tolerance) {
                        matched = false;
                    }
                }
                else if constexpr (std::is_same_v<T, Arc2D>) {
                    // Compare arc parameters
                    double centerDev = GeometryMath::distance(origGeom.center(), reimpGeom.center());
                    double radiusDev = std::abs(origGeom.radius() - reimpGeom.radius());
                    double startAngleDev = std::abs(origGeom.startAngle() - reimpGeom.startAngle());
                    double endAngleDev = std::abs(origGeom.endAngle() - reimpGeom.endAngle());

                    deviation = std::max({centerDev, radiusDev, startAngleDev, endAngleDev});

                    if (deviation > tolerance) {
                        matched = false;
                    }
                }
                else if constexpr (std::is_same_v<T, Ellipse2D>) {
                    // Compare ellipse parameters
                    double centerDev = GeometryMath::distance(origGeom.center(), reimpGeom.center());
                    double majorDev = std::abs(origGeom.majorAxisLength() - reimpGeom.majorAxisLength());
                    double minorDev = std::abs(origGeom.minorAxisLength() - reimpGeom.minorAxisLength());

                    deviation = std::max({centerDev, majorDev, minorDev});

                    if (deviation > tolerance) {
                        matched = false;
                    }
                }
                else if constexpr (std::is_same_v<T, Point2D>) {
                    // Compare point coordinates
                    deviation = GeometryMath::distance(origGeom, reimpGeom);

                    if (deviation > tolerance) {
                        matched = false;
                    }
                }
            } else {
                // Type mismatch
                matched = false;
                deviation = std::numeric_limits<double>::infinity();
            }
        }, origEntity.entity);

        // Update report
        if (deviation > report.maxDeviation) {
            report.maxDeviation = deviation;
        }

        if (!matched) {
            report.withinTolerance = false;
            report.entitiesWithPrecisionLoss.push_back(
                "Handle: " + origEntity.handle +
                " (deviation: " + std::to_string(deviation) + ")"
            );
        }
    }

    return report;
}

// ============================================================================
// LAYER VALIDATION
// ============================================================================

LayerReport ExportValidator::validateLayers(
    const DocumentModel& original,
    const DocumentModel& reimported
) {
    LayerReport report;

    // Get layer lists
    report.originalLayers = original.getLayers();
    report.exportedLayers = reimported.getLayers();

    // Convert to sets for comparison
    std::set<std::string> origSet(report.originalLayers.begin(), report.originalLayers.end());
    std::set<std::string> expSet(report.exportedLayers.begin(), report.exportedLayers.end());

    // Find missing layers (in original but not in export)
    std::set_difference(
        origSet.begin(), origSet.end(),
        expSet.begin(), expSet.end(),
        std::back_inserter(report.missingLayers)
    );

    // Find extra layers (in export but not in original)
    std::set_difference(
        expSet.begin(), expSet.end(),
        origSet.begin(), origSet.end(),
        std::back_inserter(report.extraLayers)
    );

    // Check if matches
    report.matches = report.missingLayers.empty() && report.extraLayers.empty();

    return report;
}

// ============================================================================
// HANDLE VALIDATION
// ============================================================================

HandleReport ExportValidator::validateHandles(
    const DocumentModel& original,
    const std::vector<DXFEntity>& exported
) {
    HandleReport report;

    // Extract original handles
    for (const auto& entity : original.entities()) {
        report.originalHandles.push_back(entity.handle);
    }

    // Extract exported handles
    report.exportedHandles = extractHandles(exported);

    // Convert to sets for comparison
    std::set<std::string> origSet(report.originalHandles.begin(), report.originalHandles.end());
    std::set<std::string> expSet(report.exportedHandles.begin(), report.exportedHandles.end());

    // Find missing handles
    std::set_difference(
        origSet.begin(), origSet.end(),
        expSet.begin(), expSet.end(),
        std::back_inserter(report.missingHandles)
    );

    // Find duplicate handles in export
    std::set<std::string> seen;
    for (const auto& handle : report.exportedHandles) {
        if (!seen.insert(handle).second) {
            report.duplicateHandles.push_back(handle);
        }
    }

    // Check if matches
    report.matches = report.missingHandles.empty() && report.duplicateHandles.empty();

    return report;
}

// ============================================================================
// BOUNDING BOX VALIDATION
// ============================================================================

bool ExportValidator::validateBoundingBoxes(
    const DocumentModel& original,
    const DocumentModel& reimported,
    double tolerance
) {
    if (original.entities().size() != reimported.entities().size()) {
        return false;
    }

    // Compare individual entity bounding boxes
    for (size_t i = 0; i < original.entities().size(); ++i) {
        const auto& origEntity = original.entities()[i];
        const auto& reimpEntity = reimported.entities()[i];

        auto getBB = [](const Import::GeometryEntityWithMetadata& e) {
            return std::visit([](auto&& geom) -> BoundingBox {
                using T = std::decay_t<decltype(geom)>;
                if constexpr (std::is_same_v<T, Point2D>) {
                    // Point2D doesn't have boundingBox() - create degenerate box
                    return BoundingBox::fromPoints(geom, geom);
                } else {
                    return geom.boundingBox();
                }
            }, e.entity);
        };

        BoundingBox bbOrig = getBB(origEntity);
        BoundingBox bbReimp = getBB(reimpEntity);

        if (!bbOrig.isValid() || !bbReimp.isValid()) {
            if (bbOrig.isValid() != bbReimp.isValid()) return false;
            continue; 
        }

        if (!areEqual(bbOrig.minX(), bbReimp.minX(), tolerance) ||
            !areEqual(bbOrig.minY(), bbReimp.minY(), tolerance) ||
            !areEqual(bbOrig.maxX(), bbReimp.maxX(), tolerance) ||
            !areEqual(bbOrig.maxY(), bbReimp.maxY(), tolerance)) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// HELPER METHODS
// ============================================================================

std::vector<std::string> ExportValidator::extractHandles(
    const std::vector<DXFEntity>& entities
) {
    std::vector<std::string> handles;

    for (const auto& entity : entities) {
        std::string handle;

        switch (entity.type) {
            case DXFEntityType::Line:
                handle = std::get<DXFLine>(entity.data).handle;
                break;
            case DXFEntityType::Arc:
                handle = std::get<DXFArc>(entity.data).handle;
                break;
            case DXFEntityType::Circle:
                handle = std::get<DXFCircle>(entity.data).handle;
                break;
            case DXFEntityType::LWPolyline:
                handle = std::get<DXFLWPolyline>(entity.data).handle;
                break;
            case DXFEntityType::Ellipse:
                handle = std::get<DXFEllipse>(entity.data).handle;
                break;
            case DXFEntityType::Point:
                handle = std::get<DXFPoint>(entity.data).handle;
                break;
            case DXFEntityType::Solid:
                handle = std::get<DXFSolid>(entity.data).handle;
                break;
            default:
                break;
        }

        if (!handle.empty()) {
            handles.push_back(handle);
        }
    }

    return handles;
}

bool ExportValidator::areEqual(double a, double b, double tolerance) noexcept {
    return std::abs(a - b) < tolerance;
}

} // namespace Model
} // namespace OwnCAD
