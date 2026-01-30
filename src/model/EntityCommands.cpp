#include "model/EntityCommands.h"
#include "geometry/GeometryMath.h"
#include "geometry/GeometryConstants.h"
#include <QDebug>
#include <cmath>
#include <set>

namespace OwnCAD {
namespace Model {

using namespace Geometry;
using namespace Import;

// =============================================================================
// CREATE ENTITY COMMAND
// =============================================================================

CreateEntityCommand::CreateEntityCommand(DocumentModel* model,
                                         const GeometryEntity& entity,
                                         const std::string& layer)
    : Command(model)
    , m_entity(entity)
    , m_layer(layer)
{
}

bool CreateEntityCommand::execute()
{
    if (!isValid()) {
        return false;
    }

    // Add entity based on type
    m_generatedHandle = std::visit([this](auto&& geom) -> std::string {
        using T = std::decay_t<decltype(geom)>;

        if constexpr (std::is_same_v<T, Line2D>) {
            return m_documentModel->addLine(geom, m_layer);
        } else if constexpr (std::is_same_v<T, Arc2D>) {
            return m_documentModel->addArc(geom, m_layer);
        } else if constexpr (std::is_same_v<T, Ellipse2D>) {
            return m_documentModel->addEllipse(geom, m_layer);
        } else if constexpr (std::is_same_v<T, Point2D>) {
            return m_documentModel->addPoint(geom, m_layer);
        }
        return std::string();
    }, m_entity);

    if (m_generatedHandle.empty()) {
        qWarning() << "CreateEntityCommand: failed to add entity";
        return false;
    }

    m_executed = true;
    return true;
}

bool CreateEntityCommand::undo()
{
    if (!m_executed || m_generatedHandle.empty()) {
        return false;
    }

    bool success = m_documentModel->removeEntity(m_generatedHandle);
    if (success) {
        m_executed = false;
    }
    return success;
}

bool CreateEntityCommand::redo()
{
    // Re-execute, will generate a new handle
    return execute();
}

QString CreateEntityCommand::description() const
{
    return std::visit([](auto&& geom) -> QString {
        using T = std::decay_t<decltype(geom)>;

        if constexpr (std::is_same_v<T, Line2D>) {
            return QStringLiteral("Draw Line");
        } else if constexpr (std::is_same_v<T, Arc2D>) {
            // Check if it's a full circle
            if (geom.isFullCircle()) {
                return QStringLiteral("Draw Circle");
            }
            return QStringLiteral("Draw Arc");
        } else if constexpr (std::is_same_v<T, Ellipse2D>) {
            return QStringLiteral("Draw Ellipse");
        } else if constexpr (std::is_same_v<T, Point2D>) {
            return QStringLiteral("Draw Point");
        }
        return QStringLiteral("Draw Entity");
    }, m_entity);
}

bool CreateEntityCommand::isValid() const
{
    if (!Command::isValid()) {
        return false;
    }

    // Validate geometry
    return std::visit([](auto&& geom) -> bool {
        using T = std::decay_t<decltype(geom)>;

        if constexpr (std::is_same_v<T, Line2D>) {
            return geom.isValid();
        } else if constexpr (std::is_same_v<T, Arc2D>) {
            return geom.isValid();
        } else if constexpr (std::is_same_v<T, Ellipse2D>) {
            return geom.isValid();
        } else if constexpr (std::is_same_v<T, Point2D>) {
            return true;  // Points are always valid
        }
        return false;
    }, m_entity);
}

// =============================================================================
// CREATE ENTITIES COMMAND (BATCH)
// =============================================================================

CreateEntitiesCommand::CreateEntitiesCommand(DocumentModel* model,
                                             const std::vector<GeometryEntity>& entities,
                                             const std::string& layer)
    : Command(model)
    , m_entities(entities)
    , m_layer(layer)
{
}

bool CreateEntitiesCommand::execute()
{
    if (!isValid()) {
        return false;
    }

    m_generatedHandles.clear();
    m_generatedHandles.reserve(m_entities.size());

    for (const auto& entity : m_entities) {
        std::string handle = std::visit([this](auto&& geom) -> std::string {
            using T = std::decay_t<decltype(geom)>;

            if constexpr (std::is_same_v<T, Line2D>) {
                return m_documentModel->addLine(geom, m_layer);
            } else if constexpr (std::is_same_v<T, Arc2D>) {
                return m_documentModel->addArc(geom, m_layer);
            } else if constexpr (std::is_same_v<T, Ellipse2D>) {
                return m_documentModel->addEllipse(geom, m_layer);
            } else if constexpr (std::is_same_v<T, Point2D>) {
                return m_documentModel->addPoint(geom, m_layer);
            }
            return std::string();
        }, entity);

        if (handle.empty()) {
            // Rollback all previously added entities
            for (const auto& h : m_generatedHandles) {
                m_documentModel->removeEntity(h);
            }
            m_generatedHandles.clear();
            return false;
        }

        m_generatedHandles.push_back(handle);
    }

    m_executed = true;
    return true;
}

bool CreateEntitiesCommand::undo()
{
    if (!m_executed || m_generatedHandles.empty()) {
        return false;
    }

    // Remove in reverse order
    for (auto it = m_generatedHandles.rbegin(); it != m_generatedHandles.rend(); ++it) {
        m_documentModel->removeEntity(*it);
    }

    m_generatedHandles.clear();
    m_executed = false;
    return true;
}

bool CreateEntitiesCommand::redo()
{
    return execute();
}

QString CreateEntitiesCommand::description() const
{
    size_t count = m_entities.size();
    if (count == 4) {
        // Likely a rectangle
        return QStringLiteral("Draw Rectangle");
    }
    return QString("Draw %1 entities").arg(count);
}

bool CreateEntitiesCommand::isValid() const
{
    if (!Command::isValid()) {
        return false;
    }
    return !m_entities.empty();
}

// =============================================================================
// DELETE ENTITY COMMAND
// =============================================================================

DeleteEntityCommand::DeleteEntityCommand(DocumentModel* model, const std::string& handle)
    : Command(model)
    , m_handle(handle)
{
}

bool DeleteEntityCommand::execute()
{
    if (!isValid()) {
        return false;
    }

    // Save entity and index for undo
    const auto* entity = m_documentModel->findEntityByHandle(m_handle);
    if (!entity) {
        return false;
    }

    m_savedEntity = *entity;
    m_originalIndex = m_documentModel->findEntityIndexByHandle(m_handle).value_or(0);

    // Remove entity
    bool success = m_documentModel->removeEntity(m_handle);
    if (success) {
        m_executed = true;
    }
    return success;
}

bool DeleteEntityCommand::undo()
{
    if (!m_executed || !m_savedEntity) {
        return false;
    }

    bool success = m_documentModel->restoreEntityAtIndex(*m_savedEntity, m_originalIndex);
    if (success) {
        m_executed = false;
    }
    return success;
}

QString DeleteEntityCommand::description() const
{
    if (m_savedEntity) {
        return std::visit([](auto&& geom) -> QString {
            using T = std::decay_t<decltype(geom)>;

            if constexpr (std::is_same_v<T, Line2D>) {
                return QStringLiteral("Delete Line");
            } else if constexpr (std::is_same_v<T, Arc2D>) {
                if (geom.isFullCircle()) {
                    return QStringLiteral("Delete Circle");
                }
                return QStringLiteral("Delete Arc");
            } else if constexpr (std::is_same_v<T, Ellipse2D>) {
                return QStringLiteral("Delete Ellipse");
            } else if constexpr (std::is_same_v<T, Point2D>) {
                return QStringLiteral("Delete Point");
            }
            return QStringLiteral("Delete Entity");
        }, m_savedEntity->entity);
    }
    return QStringLiteral("Delete Entity");
}

bool DeleteEntityCommand::isValid() const
{
    if (!Command::isValid()) {
        return false;
    }
    return !m_handle.empty() && m_documentModel->findEntityByHandle(m_handle) != nullptr;
}

// =============================================================================
// DELETE ENTITIES COMMAND (BATCH)
// =============================================================================

DeleteEntitiesCommand::DeleteEntitiesCommand(DocumentModel* model,
                                             const std::vector<std::string>& handles)
    : Command(model)
    , m_handles(handles)
{
}

bool DeleteEntitiesCommand::execute()
{
    if (!isValid()) {
        return false;
    }

    m_savedEntities.clear();
    m_originalIndices.clear();

    // Find and save entities in THEIR DOCUMENT ORDER to ensure correct restoration
    const auto& allEntities = m_documentModel->entities();
    std::set<std::string> targetHandles(m_handles.begin(), m_handles.end());

    for (size_t i = 0; i < allEntities.size(); ++i) {
        if (targetHandles.count(allEntities[i].handle)) {
            m_savedEntities.push_back(allEntities[i]);
            m_originalIndices.push_back(i);
        }
    }

    if (m_savedEntities.empty()) {
        return false;
    }

    // Remove all entities
    for (const auto& savedEntity : m_savedEntities) {
        m_documentModel->removeEntity(savedEntity.handle);
    }

    m_executed = true;
    return true;
}

bool DeleteEntitiesCommand::undo()
{
    if (!m_executed || m_savedEntities.empty()) {
        return false;
    }

    // Restore in ORIGINAL DOCUMENT ORDER (increasing index)
    // Since we saved them in document order, we just iterate normally
    bool allSuccess = true;
    for (size_t i = 0; i < m_savedEntities.size(); ++i) {
        if (!m_documentModel->restoreEntityAtIndex(m_savedEntities[i], m_originalIndices[i])) {
            allSuccess = false;
        }
    }

    if (allSuccess) {
        m_executed = false;
    }
    return allSuccess;
}

QString DeleteEntitiesCommand::description() const
{
    size_t count = m_savedEntities.empty() ? m_handles.size() : m_savedEntities.size();
    if (count == 1) {
        return QStringLiteral("Delete entity");
    }
    return QString("Delete %1 entities").arg(count);
}

bool DeleteEntitiesCommand::isValid() const
{
    if (!Command::isValid()) {
        return false;
    }
    return !m_handles.empty();
}

// =============================================================================
// MOVE ENTITIES COMMAND
// =============================================================================

MoveEntitiesCommand::MoveEntitiesCommand(DocumentModel* model,
                                         const std::vector<std::string>& handles,
                                         double dx, double dy)
    : Command(model)
    , m_handles(handles)
    , m_dx(dx)
    , m_dy(dy)
{
}

bool MoveEntitiesCommand::execute()
{
    if (!isValid()) {
        return false;
    }

    bool success = applyTranslation(m_dx, m_dy);
    if (success) {
        m_executed = true;
    }
    return success;
}

bool MoveEntitiesCommand::undo()
{
    if (!m_executed) {
        return false;
    }

    // Apply reverse translation
    bool success = applyTranslation(-m_dx, -m_dy);
    if (success) {
        m_executed = false;
    }
    return success;
}

QString MoveEntitiesCommand::description() const
{
    size_t count = m_handles.size();
    if (count == 1) {
        return QStringLiteral("Move entity");
    }
    return QString("Move %1 entities").arg(count);
}

bool MoveEntitiesCommand::isValid() const
{
    if (!Command::isValid()) {
        return false;
    }
    return !m_handles.empty();
}

bool MoveEntitiesCommand::canMerge(const Command* other) const
{
    // Can merge with another move command on the same entities
    // within a short time window (for smooth dragging)
    const auto* otherMove = dynamic_cast<const MoveEntitiesCommand*>(other);
    if (!otherMove) {
        return false;
    }

    // Same entity set
    if (m_handles != otherMove->m_handles) {
        return false;
    }

    // Within 500ms
    qint64 msDiff = m_timestamp.msecsTo(otherMove->m_timestamp);
    return std::abs(msDiff) < 500;
}

bool MoveEntitiesCommand::merge(const Command* other)
{
    const auto* otherMove = dynamic_cast<const MoveEntitiesCommand*>(other);
    if (!otherMove) {
        return false;
    }

    // Accumulate translations
    m_dx += otherMove->m_dx;
    m_dy += otherMove->m_dy;

    return true;
}

bool MoveEntitiesCommand::applyTranslation(double dx, double dy)
{
    for (const auto& handle : m_handles) {
        auto* entityPtr = m_documentModel->findEntityByHandle(handle);
        if (!entityPtr) {
            qWarning() << "MoveEntitiesCommand: entity not found:" << QString::fromStdString(handle);
            continue;
        }

        // Apply translation to geometry
        std::optional<GeometryEntity> translated = std::visit([dx, dy](auto&& geom)
            -> std::optional<GeometryEntity> {
            using T = std::decay_t<decltype(geom)>;

            if constexpr (std::is_same_v<T, Line2D>) {
                auto result = GeometryMath::translate(geom, dx, dy);
                if (result) return GeometryEntity{*result};
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, Arc2D>) {
                auto result = GeometryMath::translate(geom, dx, dy);
                if (result) return GeometryEntity{*result};
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, Ellipse2D>) {
                auto result = GeometryMath::translate(geom, dx, dy);
                if (result) return GeometryEntity{*result};
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, Point2D>) {
                return GeometryEntity{GeometryMath::translate(geom, dx, dy)};
            }
            return std::nullopt;
        }, entityPtr->entity);

        if (translated) {
            m_documentModel->updateEntity(handle, *translated);
        }
    }

    return true;
}

// =============================================================================
// ROTATE ENTITIES COMMAND
// =============================================================================

RotateEntitiesCommand::RotateEntitiesCommand(DocumentModel* model,
                                             const std::vector<std::string>& handles,
                                             const Point2D& center,
                                             double angleRadians)
    : Command(model)
    , m_handles(handles)
    , m_center(center)
    , m_angleRadians(angleRadians)
{
}

bool RotateEntitiesCommand::execute()
{
    if (!isValid()) {
        return false;
    }

    bool success = applyRotation(m_angleRadians);
    if (success) {
        m_executed = true;
    }
    return success;
}

bool RotateEntitiesCommand::undo()
{
    if (!m_executed) {
        return false;
    }

    // Apply reverse rotation
    bool success = applyRotation(-m_angleRadians);
    if (success) {
        m_executed = false;
    }
    return success;
}

QString RotateEntitiesCommand::description() const
{
    // Convert to degrees for display
    double degrees = m_angleRadians * 180.0 / M_PI;
    size_t count = m_handles.size();

    if (count == 1) {
        return QString("Rotate entity by %1%2").arg(degrees, 0, 'f', 1).arg(QChar(0x00B0));
    }
    return QString("Rotate %1 entities by %2%3")
        .arg(count)
        .arg(degrees, 0, 'f', 1)
        .arg(QChar(0x00B0));
}

bool RotateEntitiesCommand::isValid() const
{
    if (!Command::isValid()) {
        return false;
    }
    return !m_handles.empty();
}

bool RotateEntitiesCommand::applyRotation(double angleRadians)
{
    for (const auto& handle : m_handles) {
        auto* entityPtr = m_documentModel->findEntityByHandle(handle);
        if (!entityPtr) {
            qWarning() << "RotateEntitiesCommand: entity not found:" << QString::fromStdString(handle);
            continue;
        }

        // Apply rotation to geometry
        std::optional<GeometryEntity> rotated = std::visit([this, angleRadians](auto&& geom)
            -> std::optional<GeometryEntity> {
            using T = std::decay_t<decltype(geom)>;

            if constexpr (std::is_same_v<T, Line2D>) {
                auto result = GeometryMath::rotate(geom, m_center, angleRadians);
                if (result) return GeometryEntity{*result};
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, Arc2D>) {
                auto result = GeometryMath::rotate(geom, m_center, angleRadians);
                if (result) return GeometryEntity{*result};
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, Ellipse2D>) {
                auto result = GeometryMath::rotate(geom, m_center, angleRadians);
                if (result) return GeometryEntity{*result};
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, Point2D>) {
                return GeometryEntity{GeometryMath::rotate(geom, m_center, angleRadians)};
            }
            return std::nullopt;
        }, entityPtr->entity);

        if (rotated) {
            m_documentModel->updateEntity(handle, *rotated);
        }
    }

    return true;
}

// =============================================================================
// MIRROR ENTITIES COMMAND
// =============================================================================

MirrorEntitiesCommand::MirrorEntitiesCommand(DocumentModel* model,
                                             const std::vector<std::string>& handles,
                                             const Point2D& axisPoint1,
                                             const Point2D& axisPoint2,
                                             bool keepOriginal)
    : Command(model)
    , m_handles(handles)
    , m_axisPoint1(axisPoint1)
    , m_axisPoint2(axisPoint2)
    , m_keepOriginal(keepOriginal)
{
}

bool MirrorEntitiesCommand::execute()
{
    if (!isValid()) {
        return false;
    }

    m_createdHandles.clear();
    m_originalEntities.clear();

    for (const auto& handle : m_handles) {
        auto* entityPtr = m_documentModel->findEntityByHandle(handle);
        if (!entityPtr) {
            continue;
        }

        // Mirror the geometry
        std::optional<GeometryEntity> mirrored = std::visit([this](auto&& geom)
            -> std::optional<GeometryEntity> {
            using T = std::decay_t<decltype(geom)>;

            if constexpr (std::is_same_v<T, Line2D>) {
                auto result = GeometryMath::mirror(geom, m_axisPoint1, m_axisPoint2);
                if (result) return GeometryEntity{*result};
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, Arc2D>) {
                auto result = GeometryMath::mirror(geom, m_axisPoint1, m_axisPoint2);
                if (result) return GeometryEntity{*result};
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, Ellipse2D>) {
                auto result = GeometryMath::mirror(geom, m_axisPoint1, m_axisPoint2);
                if (result) return GeometryEntity{*result};
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, Point2D>) {
                return GeometryEntity{GeometryMath::mirror(geom, m_axisPoint1, m_axisPoint2)};
            }
            return std::nullopt;
        }, entityPtr->entity);

        if (!mirrored) {
            continue;
        }

        if (m_keepOriginal) {
            // Create a copy with the mirrored geometry
            std::string newHandle = std::visit([this, &entityPtr](auto&& geom) -> std::string {
                using T = std::decay_t<decltype(geom)>;

                if constexpr (std::is_same_v<T, Line2D>) {
                    return m_documentModel->addLine(geom, entityPtr->layer);
                } else if constexpr (std::is_same_v<T, Arc2D>) {
                    return m_documentModel->addArc(geom, entityPtr->layer);
                } else if constexpr (std::is_same_v<T, Ellipse2D>) {
                    return m_documentModel->addEllipse(geom, entityPtr->layer);
                } else if constexpr (std::is_same_v<T, Point2D>) {
                    return m_documentModel->addPoint(geom, entityPtr->layer);
                }
                return std::string();
            }, *mirrored);

            if (!newHandle.empty()) {
                m_createdHandles.push_back(newHandle);
            }
        } else {
            // Save original for undo
            m_originalEntities.push_back(*entityPtr);

            // Replace with mirrored geometry
            m_documentModel->updateEntity(handle, *mirrored);
        }
    }

    m_executed = true;
    return true;
}

bool MirrorEntitiesCommand::undo()
{
    if (!m_executed) {
        return false;
    }

    if (m_keepOriginal) {
        // Remove created copies
        for (const auto& handle : m_createdHandles) {
            m_documentModel->removeEntity(handle);
        }
        m_createdHandles.clear();
    } else {
        // Restore original entities
        for (const auto& original : m_originalEntities) {
            m_documentModel->updateEntity(original.handle, original.entity);
        }
    }

    m_executed = false;
    return true;
}

QString MirrorEntitiesCommand::description() const
{
    size_t count = m_handles.size();
    if (m_keepOriginal) {
        if (count == 1) {
            return QStringLiteral("Mirror and copy entity");
        }
        return QString("Mirror and copy %1 entities").arg(count);
    } else {
        if (count == 1) {
            return QStringLiteral("Mirror entity");
        }
        return QString("Mirror %1 entities").arg(count);
    }
}

bool MirrorEntitiesCommand::isValid() const
{
    if (!Command::isValid()) {
        return false;
    }

    // Check axis is not degenerate
    double axisDx = m_axisPoint2.x() - m_axisPoint1.x();
    double axisDy = m_axisPoint2.y() - m_axisPoint1.y();
    double axisLen = std::sqrt(axisDx * axisDx + axisDy * axisDy);

    if (axisLen < GEOMETRY_EPSILON) {
        return false;  // Degenerate axis
    }

    return !m_handles.empty();
}

} // namespace Model
} // namespace OwnCAD
