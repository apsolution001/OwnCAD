#pragma once

#include "geometry/Point2D.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QString>
#include <optional>

namespace OwnCAD {

// Forward declarations
namespace Model {
    class DocumentModel;
    class CommandHistory;
}

namespace UI {

class Viewport;

/**
 * @brief Tool state enumeration
 *
 * Describes the current state of a drawing/editing tool.
 */
enum class ToolState {
    Inactive,           // Tool is not active
    WaitingForInput,    // Tool is active, waiting for first input
    InProgress          // Tool has partial input, working toward completion
};

/**
 * @brief Result of a tool action
 *
 * Returned by tool event handlers to indicate what happened.
 */
enum class ToolResult {
    Continue,           // Tool consumed event, continue in current state
    Completed,          // Tool completed its action (e.g., line created)
    Cancelled,          // Tool operation was cancelled
    Ignored             // Tool did not handle the event
};

/**
 * @brief Abstract base class for all drawing and editing tools
 *
 * Tools handle mouse and keyboard events to create or modify geometry.
 * Each tool implements a state machine for its specific workflow.
 *
 * Design principles:
 * - Tools are stateful (track partial input)
 * - Tools render their own preview
 * - Tools validate before committing
 * - Tools are activated/deactivated by ToolManager
 *
 * Lifecycle:
 * 1. activate() called when tool becomes active
 * 2. handleMouse/Key events called during operation
 * 3. render() called each frame for preview
 * 4. deactivate() called when switching to another tool
 */
class Tool {
public:
    virtual ~Tool() = default;

    /**
     * @brief Get the tool name for display
     */
    virtual QString name() const = 0;

    /**
     * @brief Get the tool identifier (for tool switching)
     */
    virtual QString id() const = 0;

    /**
     * @brief Called when tool becomes active
     *
     * Reset internal state, prepare for first input.
     */
    virtual void activate() = 0;

    /**
     * @brief Called when tool is deactivated
     *
     * Clean up any partial state, cancel pending operations.
     */
    virtual void deactivate() = 0;

    /**
     * @brief Get current tool state
     */
    virtual ToolState state() const = 0;

    /**
     * @brief Get status bar prompt for current state
     *
     * Returns a message to display in status bar guiding the user.
     * Example: "LINE: Click first point"
     */
    virtual QString statusPrompt() const = 0;

    // -------------------------------------------------------------------------
    // Event handlers
    // -------------------------------------------------------------------------

    /**
     * @brief Handle mouse press event
     * @param worldPos Position in world coordinates (after snap)
     * @param event Original Qt mouse event (for modifiers, button)
     * @return Result indicating how event was handled
     */
    virtual ToolResult handleMousePress(
        const Geometry::Point2D& worldPos,
        QMouseEvent* event
    ) = 0;

    /**
     * @brief Handle mouse move event
     * @param worldPos Position in world coordinates (after snap)
     * @param event Original Qt mouse event
     * @return Result indicating how event was handled
     */
    virtual ToolResult handleMouseMove(
        const Geometry::Point2D& worldPos,
        QMouseEvent* event
    ) = 0;

    /**
     * @brief Handle mouse release event
     * @param worldPos Position in world coordinates (after snap)
     * @param event Original Qt mouse event
     * @return Result indicating how event was handled
     */
    virtual ToolResult handleMouseRelease(
        const Geometry::Point2D& worldPos,
        QMouseEvent* event
    ) = 0;

    /**
     * @brief Handle key press event
     * @param event Qt key event
     * @return Result indicating how event was handled
     */
    virtual ToolResult handleKeyPress(QKeyEvent* event) = 0;

    // -------------------------------------------------------------------------
    // Rendering
    // -------------------------------------------------------------------------

    /**
     * @brief Render tool preview/feedback
     * @param painter Qt painter (already transformed for screen space)
     * @param viewport Viewport for coordinate transformations
     *
     * Called during paintEvent to render tool-specific visuals:
     * - Preview geometry (e.g., rubber-band line)
     * - Construction points
     * - Guides and constraints
     */
    virtual void render(QPainter& painter, const Viewport& viewport) = 0;

    // -------------------------------------------------------------------------
    // Document access
    // -------------------------------------------------------------------------

    /**
     * @brief Set the document model for committing changes
     * @param model Pointer to document model (owned by application)
     */
    void setDocumentModel(Model::DocumentModel* model) { documentModel_ = model; }

    /**
     * @brief Set the command history for undo/redo support
     * @param history Pointer to command history (owned by application)
     */
    void setCommandHistory(Model::CommandHistory* history) { commandHistory_ = history; }

protected:
    Model::DocumentModel* documentModel_ = nullptr;
    Model::CommandHistory* commandHistory_ = nullptr;
};

} // namespace UI
} // namespace OwnCAD
