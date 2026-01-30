#pragma once

#include "model/Command.h"
#include "model/DocumentModel.h"
#include "import/GeometryConverter.h"
#include "geometry/Point2D.h"
#include <vector>
#include <string>
#include <optional>

namespace OwnCAD {
namespace Model {

// =============================================================================
// CREATE ENTITY COMMAND
// =============================================================================

/**
 * @brief Command to add a single entity to the document.
 *
 * Supports all entity types: Line2D, Arc2D, Ellipse2D, Point2D.
 * On undo, removes the created entity using its generated handle.
 */
class CreateEntityCommand : public Command {
public:
    /**
     * @brief Construct create command
     * @param model Target document (non-owning)
     * @param entity Geometry to add (variant type)
     * @param layer Target layer name (default: "0")
     */
    CreateEntityCommand(DocumentModel* model,
                        const Import::GeometryEntity& entity,
                        const std::string& layer = "0");

    bool execute() override;
    bool undo() override;
    bool redo() override;
    QString description() const override;
    bool isValid() const override;

private:
    Import::GeometryEntity m_entity;
    std::string m_layer;
    std::string m_generatedHandle;  // Set by execute(), used by undo()
};

// =============================================================================
// CREATE ENTITIES COMMAND (BATCH)
// =============================================================================

/**
 * @brief Command to add multiple entities as a single undoable operation.
 *
 * Used by RectangleTool (4 lines) and similar composite operations.
 * All entities are added/removed atomically.
 */
class CreateEntitiesCommand : public Command {
public:
    /**
     * @brief Construct batch create command
     * @param model Target document (non-owning)
     * @param entities Geometry entities to add
     * @param layer Target layer name (default: "0")
     */
    CreateEntitiesCommand(DocumentModel* model,
                          const std::vector<Import::GeometryEntity>& entities,
                          const std::string& layer = "0");

    bool execute() override;
    bool undo() override;
    bool redo() override;
    QString description() const override;
    bool isValid() const override;

private:
    std::vector<Import::GeometryEntity> m_entities;
    std::string m_layer;
    std::vector<std::string> m_generatedHandles;  // Handles for undo
};

// =============================================================================
// DELETE ENTITY COMMAND (SINGLE)
// =============================================================================

/**
 * @brief Command to remove a single entity from the document.
 *
 * Stores full entity copy for undo restoration.
 */
class DeleteEntityCommand : public Command {
public:
    /**
     * @brief Construct delete command
     * @param model Target document (non-owning)
     * @param handle Entity handle to delete
     */
    DeleteEntityCommand(DocumentModel* model, const std::string& handle);

    bool execute() override;
    bool undo() override;
    QString description() const override;
    bool isValid() const override;

private:
    std::string m_handle;
    std::optional<Import::GeometryEntityWithMetadata> m_savedEntity;
    size_t m_originalIndex = 0;  // Saved during execute() for undo()
};

// =============================================================================
// DELETE ENTITIES COMMAND (BATCH)
// =============================================================================

/**
 * @brief Command to remove multiple entities as a single undoable operation.
 *
 * Used for selection-based delete operations.
 */
class DeleteEntitiesCommand : public Command {
public:
    /**
     * @brief Construct bulk delete command
     * @param model Target document (non-owning)
     * @param handles Entity handles to delete
     */
    DeleteEntitiesCommand(DocumentModel* model,
                          const std::vector<std::string>& handles);

    bool execute() override;
    bool undo() override;
    QString description() const override;
    bool isValid() const override;

private:
    std::vector<std::string> m_handles;
    std::vector<Import::GeometryEntityWithMetadata> m_savedEntities;
    std::vector<size_t> m_originalIndices;  // Saved during execute() for undo()
};

// =============================================================================
// MOVE ENTITIES COMMAND
// =============================================================================

/**
 * @brief Command to translate entities by (dx, dy).
 *
 * Undo is achieved by applying reverse translation (-dx, -dy).
 * No geometry copies needed.
 */
class MoveEntitiesCommand : public Command {
public:
    /**
     * @brief Construct move command
     * @param model Target document (non-owning)
     * @param handles Entity handles to move
     * @param dx X translation in world units
     * @param dy Y translation in world units
     */
    MoveEntitiesCommand(DocumentModel* model,
                        const std::vector<std::string>& handles,
                        double dx, double dy);

    bool execute() override;
    bool undo() override;
    QString description() const override;
    bool isValid() const override;

    // Command merging for rapid drag updates
    bool canMerge(const Command* other) const override;
    bool merge(const Command* other) override;

private:
    bool applyTranslation(double dx, double dy);

    std::vector<std::string> m_handles;
    double m_dx;
    double m_dy;
};

// =============================================================================
// ROTATE ENTITIES COMMAND
// =============================================================================

/**
 * @brief Command to rotate entities around a center point.
 *
 * Undo is achieved by applying reverse rotation (-angle).
 * Arc direction is preserved during rotation.
 */
class RotateEntitiesCommand : public Command {
public:
    /**
     * @brief Construct rotate command
     * @param model Target document (non-owning)
     * @param handles Entity handles to rotate
     * @param center Rotation center point
     * @param angleRadians Rotation angle (positive = CCW)
     */
    RotateEntitiesCommand(DocumentModel* model,
                          const std::vector<std::string>& handles,
                          const Geometry::Point2D& center,
                          double angleRadians);

    bool execute() override;
    bool undo() override;
    QString description() const override;
    bool isValid() const override;

private:
    bool applyRotation(double angleRadians);

    std::vector<std::string> m_handles;
    Geometry::Point2D m_center;
    double m_angleRadians;
};

// =============================================================================
// MIRROR ENTITIES COMMAND
// =============================================================================

/**
 * @brief Command to mirror entities across an axis.
 *
 * Supports two modes:
 * - keepOriginal=true: Creates mirrored copies (undo removes copies)
 * - keepOriginal=false: Replaces originals (undo restores originals)
 *
 * IMPORTANT: Arc direction is inverted during mirror (CCW <-> CW).
 * This is mathematically correct and critical for CNC toolpaths.
 */
class MirrorEntitiesCommand : public Command {
public:
    /**
     * @brief Construct mirror command
     * @param model Target document (non-owning)
     * @param handles Entity handles to mirror
     * @param axisPoint1 First point defining mirror axis
     * @param axisPoint2 Second point defining mirror axis
     * @param keepOriginal If true, create copies; if false, replace originals
     */
    MirrorEntitiesCommand(DocumentModel* model,
                          const std::vector<std::string>& handles,
                          const Geometry::Point2D& axisPoint1,
                          const Geometry::Point2D& axisPoint2,
                          bool keepOriginal = false);

    bool execute() override;
    bool undo() override;
    QString description() const override;
    bool isValid() const override;

private:
    std::vector<std::string> m_handles;
    Geometry::Point2D m_axisPoint1;
    Geometry::Point2D m_axisPoint2;
    bool m_keepOriginal;

    // Undo state
    std::vector<std::string> m_createdHandles;  // If keepOriginal=true
    std::vector<Import::GeometryEntityWithMetadata> m_originalEntities;  // If keepOriginal=false
};

} // namespace Model
} // namespace OwnCAD
