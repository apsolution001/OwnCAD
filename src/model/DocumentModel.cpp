#include "model/DocumentModel.h"
#include "import/DXFParser.h"
#include "export/GeometryExporter.h"
#include "export/DXFWriter.h"
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

    // Step 5: Update handle generator to avoid conflicts with imported handles
    updateNextHandleNumber();

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

    // Snapshot entities and handles for validation
    auto entityVariants = getEntityVariants();
    auto handles = getEntityHandles();

    // Run validation synchronously with handle tracking
    validationResult_ = GeometryValidator::validateEntitiesWithHandles(
        entityVariants,
        handles,
        GEOMETRY_EPSILON
    );
}

void DocumentModel::runValidationAsync() {
    if (isValidating_) {
        return; // Already running
    }

    if (entities_.empty()) {
        if (validationCallback_) {
            validationCallback_(ValidationResult()); // Immediate empty result
        }
        return;
    }

    isValidating_ = true;

    // Snapshot entities and handles to pass to worker thread (thread safety: copy)
    // Note: This copying happens on the calling thread (Main Thread usually)
    auto entityVariants = getEntityVariants();
    auto handles = getEntityHandles();

    // Launch async task
    validationFuture_ = std::async(std::launch::async,
        [this, variants = std::move(entityVariants), hdls = std::move(handles)]() mutable {
            // This runs in background thread
            ValidationResult result = GeometryValidator::validateEntitiesWithHandles(
                variants,
                hdls,
                GEOMETRY_EPSILON
            );

            // Notify completion
            if (validationCallback_) {
                validationCallback_(result);
            }

            isValidating_ = false;
        }
    );
}

void DocumentModel::setValidationCompletionCallback(std::function<void(const Geometry::ValidationResult&)> callback) {
    validationCallback_ = callback;
}

bool DocumentModel::isValidating() const {
    return isValidating_;
}

void DocumentModel::finalizeValidation(const Geometry::ValidationResult& result) {
    std::lock_guard<std::mutex> lock(validationMutex_);
    validationResult_ = result;
    calculateStatistics(); // Re-calculate stats based on new validation results
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

std::vector<std::string> DocumentModel::getEntityHandles() const {
    std::vector<std::string> handles;
    handles.reserve(entities_.size());

    for (const auto& entityWithMeta : entities_) {
        // Only extract handles for Line2D and Arc2D (parallel to getEntityVariants)
        // Must maintain same order as getEntityVariants() for correct mapping
        if (std::holds_alternative<Line2D>(entityWithMeta.entity) ||
            std::holds_alternative<Arc2D>(entityWithMeta.entity)) {
            handles.push_back(entityWithMeta.handle);
        }
        // Skip Ellipse2D and Point2D for now (no validator yet)
    }

    return handles;
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

    // Create entity with metadata (aggregate initialization)
    GeometryEntityWithMetadata entityWithMeta{
        line,           // entity
        layer,          // layer
        handle,         // handle
        256,            // colorNumber (BYLAYER)
        0               // sourceLineNumber (not from file)
    };

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

    // Create entity with metadata (aggregate initialization)
    GeometryEntityWithMetadata entityWithMeta{
        arc,            // entity
        layer,          // layer
        handle,         // handle
        256,            // colorNumber (BYLAYER)
        0               // sourceLineNumber (not from file)
    };

    // Add to collection
    entities_.push_back(entityWithMeta);

    // Update statistics
    statistics_.totalArcs++;
    statistics_.totalSegments++;
    statistics_.validEntities++;

    return handle;
}

std::string DocumentModel::generateHandle() {
    // Generate handle in hex format (standard for DXF)
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%zX", nextHandleNumber_++);
    return std::string(buffer);
}

std::optional<size_t> DocumentModel::findEntityIndexByHandle(const std::string& handle) const {
    for (size_t i = 0; i < entities_.size(); ++i) {
        if (entities_[i].handle == handle) {
            return i;
        }
    }
    return std::nullopt;
}

// ============================================================================
// ENTITY MODIFICATION
// ============================================================================

GeometryEntityWithMetadata* DocumentModel::findEntityByHandle(const std::string& handle) {
    for (auto& entity : entities_) {
        if (entity.handle == handle) {
            return &entity;
        }
    }
    return nullptr;
}

const GeometryEntityWithMetadata* DocumentModel::findEntityByHandle(const std::string& handle) const {
    for (const auto& entity : entities_) {
        if (entity.handle == handle) {
            return &entity;
        }
    }
    return nullptr;
}

bool DocumentModel::updateEntity(const std::string& handle, const GeometryEntity& newGeometry) {
    GeometryEntityWithMetadata* entity = findEntityByHandle(handle);
    if (!entity) {
        return false;
    }

    // Track old type for statistics update
    bool wasLine = std::holds_alternative<Line2D>(entity->entity);
    bool wasArc = std::holds_alternative<Arc2D>(entity->entity);

    // Replace geometry (metadata preserved)
    entity->entity = newGeometry;

    // Track new type for statistics update
    bool isLine = std::holds_alternative<Line2D>(newGeometry);
    bool isArc = std::holds_alternative<Arc2D>(newGeometry);

    // Update statistics if type changed
    if (wasLine && !isLine) statistics_.totalLines--;
    if (wasArc && !isArc) statistics_.totalArcs--;
    if (!wasLine && isLine) statistics_.totalLines++;
    if (!wasArc && isArc) statistics_.totalArcs++;

    return true;
}

bool DocumentModel::removeEntity(const std::string& handle) {
    auto it = std::find_if(entities_.begin(), entities_.end(),
        [&handle](const GeometryEntityWithMetadata& e) {
            return e.handle == handle;
        });

    if (it == entities_.end()) {
        return false;
    }

    // Update statistics before removal
    if (std::holds_alternative<Line2D>(it->entity)) {
        statistics_.totalLines--;
    } else if (std::holds_alternative<Arc2D>(it->entity)) {
        statistics_.totalArcs--;
    }
    statistics_.totalSegments--;
    statistics_.validEntities--;

    // Remove entity
    entities_.erase(it);

    return true;
}

bool DocumentModel::restoreEntity(const Import::GeometryEntityWithMetadata& entity) {
    // Re-use restoreEntityAtIndex logic at the end of the collection
    return restoreEntityAtIndex(entity, entities_.size());
}

bool DocumentModel::restoreEntityAtIndex(const Import::GeometryEntityWithMetadata& entity, size_t index) {
    // Check for handle conflict - indicates a bug if this happens
    if (findEntityByHandle(entity.handle) != nullptr) {
        return false;
    }

    // Clamp index to valid range
    if (index > entities_.size()) {
        index = entities_.size();
    }

    // Add entity back to collection at original position
    entities_.insert(entities_.begin() + index, entity);

    // Update statistics
    std::visit([this](auto&& geom) {
        using T = std::decay_t<decltype(geom)>;
        if constexpr (std::is_same_v<T, Line2D>) {
            statistics_.totalLines++;
        } else if constexpr (std::is_same_v<T, Arc2D>) {
            statistics_.totalArcs++;
        }
    }, entity.entity);

    statistics_.totalSegments++;
    statistics_.validEntities++;

    return true;
}

std::string DocumentModel::addEllipse(const Ellipse2D& ellipse, const std::string& layer) {
    // Validate input
    if (!ellipse.isValid()) {
        return std::string();
    }

    // Generate handle
    std::string handle = generateHandle();

    // Create entity with metadata
    GeometryEntityWithMetadata entityWithMeta{
        ellipse,        // entity
        layer,          // layer
        handle,         // handle
        256,            // colorNumber (BYLAYER)
        0               // sourceLineNumber (not from file)
    };

    // Add to collection
    entities_.push_back(entityWithMeta);

    // Update statistics
    statistics_.totalSegments++;
    statistics_.validEntities++;

    return handle;
}

std::string DocumentModel::addPoint(const Point2D& point, const std::string& layer) {
    // Generate handle
    std::string handle = generateHandle();

    // Create entity with metadata
    GeometryEntityWithMetadata entityWithMeta{
        point,          // entity
        layer,          // layer
        handle,         // handle
        256,            // colorNumber (BYLAYER)
        0               // sourceLineNumber (not from file)
    };

    // Add to collection
    entities_.push_back(entityWithMeta);

    // Update statistics
    statistics_.totalSegments++;
    statistics_.validEntities++;

    return handle;
}

// ============================================================================
// DXF EXPORT
// ============================================================================

bool DocumentModel::exportDXFFile(const std::string& filePath) const {
    exportErrors_.clear();

    // Step 1: Convert internal geometry to DXF entities
    Export::ExportResult exportResult = Export::GeometryExporter::exportToDXF(entities_);

    if (!exportResult.success || !exportResult.errors.empty()) {
        exportErrors_ = exportResult.errors;
        return false;
    }

    // Step 2: Write DXF entities to file
    bool writeSuccess = Export::DXFWriter::writeFile(filePath, exportResult.entities);

    if (!writeSuccess) {
        exportErrors_.push_back("Failed to write DXF file: " + filePath);
        return false;
    }

    return true;
}

const std::vector<std::string>& DocumentModel::exportErrors() const noexcept {
    return exportErrors_;
}

void DocumentModel::updateNextHandleNumber() {
    size_t maxHandle = 0;

    for (const auto& entity : entities_) {
        if (entity.handle.empty()) continue;

        try {
            size_t val = 0;
            if (entity.handle.length() > 1 && entity.handle[0] == 'E') {
                // Legacy decimal format "E00001"
                val = std::stoull(entity.handle.substr(1));
            } else {
                // Standard hex format
                val = std::stoull(entity.handle, nullptr, 16);
            }

            if (val > maxHandle) {
                maxHandle = val;
            }
        } catch (...) {
            // Non-numeric handle, skip
        }
    }

    // Ensure next handle is greater than any existing handle
    if (maxHandle >= nextHandleNumber_) {
        nextHandleNumber_ = maxHandle + 1;
    }
}

} // namespace Model
} // namespace OwnCAD
