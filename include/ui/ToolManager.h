#pragma once

#include "ui/Tool.h"
#include <QObject>
#include <QString>
#include <memory>
#include <unordered_map>

namespace OwnCAD {

namespace Model {
    class DocumentModel;
    class CommandHistory;
}

namespace UI {

class Viewport;

/**
 * @brief Manages drawing and editing tools
 *
 * Responsibilities:
 * - Owns and manages tool instances
 * - Routes mouse/keyboard events to active tool
 * - Handles tool switching (deactivate old, activate new)
 * - Emits signals for status bar updates
 *
 * Design:
 * - Tools are registered by ID at startup
 * - Only one tool can be active at a time
 * - When no tool is active, events are not consumed (selection mode)
 */
class ToolManager : public QObject {
    Q_OBJECT

public:
    explicit ToolManager(QObject* parent = nullptr);
    ~ToolManager();

    /**
     * @brief Register a tool with the manager
     * @param tool Tool to register (ownership transferred)
     *
     * Tool ID must be unique. Duplicate IDs are rejected.
     */
    void registerTool(std::unique_ptr<Tool> tool);

    /**
     * @brief Set document model for all tools
     * @param model Document model pointer
     */
    void setDocumentModel(Model::DocumentModel* model);

    /**
     * @brief Set command history for all tools (enables undo/redo)
     * @param history Command history pointer
     */
    void setCommandHistory(Model::CommandHistory* history);

    /**
     * @brief Activate a tool by ID
     * @param toolId Tool identifier (e.g., "line", "arc")
     * @return true if tool was found and activated
     */
    bool activateTool(const QString& toolId);

    /**
     * @brief Deactivate current tool (return to selection mode)
     */
    void deactivateTool();

    /**
     * @brief Get currently active tool (may be nullptr)
     */
    Tool* activeTool() const { return activeTool_; }

    /**
     * @brief Check if any tool is active
     */
    bool hasActiveTool() const { return activeTool_ != nullptr; }

    /**
     * @brief Get tool by ID
     * @param toolId Tool identifier
     * @return Tool pointer or nullptr if not found
     */
    Tool* tool(const QString& toolId) const;

    // -------------------------------------------------------------------------
    // Event routing (called by CADCanvas)
    // -------------------------------------------------------------------------

    /**
     * @brief Route mouse press to active tool
     * @return ToolResult::Ignored if no active tool
     */
    ToolResult handleMousePress(
        const Geometry::Point2D& worldPos,
        QMouseEvent* event
    );

    /**
     * @brief Route mouse move to active tool
     * @return ToolResult::Ignored if no active tool
     */
    ToolResult handleMouseMove(
        const Geometry::Point2D& worldPos,
        QMouseEvent* event
    );

    /**
     * @brief Route mouse release to active tool
     * @return ToolResult::Ignored if no active tool
     */
    ToolResult handleMouseRelease(
        const Geometry::Point2D& worldPos,
        QMouseEvent* event
    );

    /**
     * @brief Route key press to active tool
     * @return ToolResult::Ignored if no active tool
     */
    ToolResult handleKeyPress(QKeyEvent* event);

    /**
     * @brief Render active tool preview
     * @param painter Qt painter
     * @param viewport Viewport for transformations
     */
    void render(QPainter& painter, const Viewport& viewport);

signals:
    /**
     * @brief Emitted when active tool changes
     * @param toolId New tool ID (empty string if deactivated)
     */
    void activeToolChanged(const QString& toolId);

    /**
     * @brief Emitted when status prompt should update
     * @param prompt New status bar message
     */
    void statusPromptChanged(const QString& prompt);

    /**
     * @brief Emitted when tool creates or modifies geometry
     *
     * Canvas should repaint after this signal.
     */
    void geometryChanged();

private:
    std::unordered_map<QString, std::unique_ptr<Tool>> tools_;
    Tool* activeTool_ = nullptr;
    Model::DocumentModel* documentModel_ = nullptr;
    Model::CommandHistory* commandHistory_ = nullptr;

    void updateStatusPrompt();
};

} // namespace UI
} // namespace OwnCAD
