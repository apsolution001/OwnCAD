#include "model/DocumentModel.h"
#include "import/DXFParser.h"
#include "geometry/GeometryConstants.h"
#include <algorithm>
#include <set>

namespace OwnCAD {
namespace Model {

using namespace OwnCAD::Import;
using namespace OwnCAD::Geometry;

// ============================================================================
// CONSTRUCTION
// ============================================================================

DocumentModel::DocumentModel() {
}

DocumentModel::~DocumentModel() {
}

// ============================================================================
// LOADING
// ============================================================================

bool DocumentModel::loadDXFFile(const std::string& filePath) {
    // Clear previous state
    clear();
    filePath_ = filePath;

    // Step 1: Parse DXF file
    DXFParseResult parseResult = DXFParser::parseFile(filePath);

    if (!parseResult.success) {
        importErrors_ = parseResult.errors;
        importWarnings_ = parseResult.warnings;
        return false;
    }

    // Store parse warnings
    importWarnings_ = parseResult.warnings;

    // Step 2: Convert DXF entities to internal geometry model
    ConversionResult conversionResult = GeometryConverter::convert(parseResult.entities);

    if (!conversionResult.success) {
        importErrors_ = conversionResult.errors;
        importWarnings_.insert(
            importWarnings_.end(),
            conversionResult.warnings.begin(),
            conversionResult.warnings.end()
        );
        // Continue even with some conversion errors - load what we can
    }

    // Store converted entities
    entities_ = conversionResult.entities;

    // Store original DXF entity count (before decomposition)
    statistics_.dxfEntitiesImported = conversionResult.totalConverted;

    // Add conversion errors to warnings (non-fatal)
    importWarnings_.insert(
        importWarnings_.end(),
        conversionResult.errors.begin(),
        conversionResult.errors.end()
    );

    // Step 3: Validate geometry
    runValidation();

    // Step 4: Calculate statistics
    calculateStatistics();

    return !entities_.empty();
}

void DocumentModel::clear() {
    entities_.clear();
    validationResult_ = ValidationResult();
    statistics_ = DocumentStatistics();
    filePath_.clear();
    importErrors_.clear();
    importWarnings_.clear();
}

// ============================================================================
// VALIDATION
// ============================================================================

void DocumentModel::runValidation() {
    if (entities_.empty()) {
        validationResult_ = ValidationResult();
        return;
    }

    // Convert entities to variant for validation
    auto entityVariants = getEntityVariants();

    // Run validation with default tolerance
    validationResult_ = GeometryValidator::validateEntities(
        entityVariants,
        GEOMETRY_EPSILON
    );
}

std::vector<std::variant<Line2D, Arc2D>> DocumentModel::getEntityVariants() const {
    std::vector<std::variant<Line2D, Arc2D>> variants;
    variants.reserve(entities_.size());

    for (const auto& entityWithMeta : entities_) {
        // Only extract Line2D and Arc2D for validation
        // (GeometryValidator currently only supports these types)
        if (std::holds_alternative<Line2D>(entityWithMeta.entity)) {
            variants.push_back(std::get<Line2D>(entityWithMeta.entity));
        } else if (std::holds_alternative<Arc2D>(entityWithMeta.entity)) {
            variants.push_back(std::get<Arc2D>(entityWithMeta.entity));
        }
        // Skip Ellipse2D and Point2D for now (no validator yet)
    }

    return variants;
}

// ============================================================================
// STATISTICS
// ============================================================================

void DocumentModel::calculateStatistics() {
    // Don't reset dxfEntitiesImported - it was set during load
    size_t originalCount = statistics_.dxfEntitiesImported;
    statistics_ = DocumentStatistics();
    statistics_.dxfEntitiesImported = originalCount;

    // Count actual geometry segments
    statistics_.totalSegments = entities_.size();

    for (const auto& entityWithMeta : entities_) {
        std::visit([&](auto&& entity) {
            using T = std::decay_t<decltype(entity)>;

            if constexpr (std::is_same_v<T, Line2D>) {
                statistics_.totalLines++;
            }
            else if constexpr (std::is_same_v<T, Arc2D>) {
                statistics_.totalArcs++;
            }
            else if constexpr (std::is_same_v<T, Ellipse2D>) {
                // Ellipse - counted in totalSegments
            }
            else if constexpr (std::is_same_v<T, Point2D>) {
                // Point - counted in totalSegments
            }
        }, entityWithMeta.entity);
    }

    // Count validation issues
    for (const auto& issue : validationResult_.issues) {
        switch (issue.type) {
            case GeometryIssueType::ZeroLengthLine:
                statistics_.zeroLengthLines++;
                statistics_.invalidEntities++;
                break;
            case GeometryIssueType::ZeroRadiusArc:
                statistics_.zeroRadiusArcs++;
                statistics_.invalidEntities++;
                break;
            case GeometryIssueType::NumericalInstability:
                statistics_.numericallyUnstable++;
                break;
            default:
                statistics_.invalidEntities++;
                break;
        }
    }

    statistics_.validEntities = statistics_.totalSegments - statistics_.invalidEntities;
}

// ============================================================================
// QUERIES
// ============================================================================

std::vector<std::string> DocumentModel::getLayers() const {
    std::set<std::string> layerSet;

    for (const auto& entityWithMeta : entities_) {
        layerSet.insert(entityWithMeta.layer);
    }

    return std::vector<std::string>(layerSet.begin(), layerSet.end());
}

// ============================================================================
// ENTITY CREATION
// ============================================================================

std::string DocumentModel::addLine(const Line2D& line, const std::string& layer) {
    // Validate input
    if (!line.isValid()) {
        return std::string();
    }

    // Generate handle
    std::string handle = generateHandle();

    // Create entity with metadata
    GeometryEntityWithMetadata entityWithMeta;
    entityWithMeta.entity = line;
    entityWithMeta.layer = layer;
    entityWithMeta.handle = handle;
    entityWithMeta.colorNumber = 256;  // BYLAYER
    entityWithMeta.sourceLineNumber = 0;  // Not from file

    // Add to collection
    entities_.push_back(entityWithMeta);

    // Update statistics
    statistics_.totalLines++;
    statistics_.totalSegments++;
    statistics_.validEntities++;

    return handle;
}

std::string DocumentModel::addArc(const Arc2D& arc, const std::string& layer) {
    // Validate input
    if (!arc.isValid()) {
        return std::string();
    }

    // Generate handle
    std::string handle = generateHandle();

    // Create entity with metadata
    GeometryEntityWithMetadata entityWithMeta;
    entityWithMeta.entity = arc;
    entityWithMeta.layer = layer;
    entityWithMeta.handle = handle;
    entityWithMeta.colorNumber = 256;  // BYLAYER
    entityWithMeta.sourceLineNumber = 0;  // Not from file

    // Add to collection
    entities_.push_back(entityWithMeta);

    // Update statistics
    statistics_.totalArcs++;
    statistics_.totalSegments++;
    statistics_.validEntities++;

    return handle;
}

std::string DocumentModel::generateHandle() {
    // Generate handle in format "E00001", "E00002", etc.
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "E%05zu", nextHandleNumber_++);
    return std::string(buffer);
}

} // namespace Model
} // namespace OwnCAD
