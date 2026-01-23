#include "ui/ToolManager.h"
#include <QDebug>

namespace OwnCAD {
namespace UI {

ToolManager::ToolManager(QObject* parent)
    : QObject(parent)
{
}

ToolManager::~ToolManager() = default;

void ToolManager::registerTool(std::unique_ptr<Tool> tool) {
    if (!tool) {
        qWarning() << "ToolManager: Attempted to register null tool";
        return;
    }

    QString toolId = tool->id();
    if (tools_.find(toolId) != tools_.end()) {
        qWarning() << "ToolManager: Tool with ID" << toolId << "already registered";
        return;
    }

    // Set document model if available
    if (documentModel_) {
        tool->setDocumentModel(documentModel_);
    }

    // Set command history if available
    if (commandHistory_) {
        tool->setCommandHistory(commandHistory_);
    }

    tools_[toolId] = std::move(tool);
    qDebug() << "ToolManager: Registered tool" << toolId;
}

void ToolManager::setDocumentModel(Model::DocumentModel* model) {
    documentModel_ = model;

    // Update all registered tools
    for (auto& [id, tool] : tools_) {
        tool->setDocumentModel(model);
    }
}

void ToolManager::setCommandHistory(Model::CommandHistory* history) {
    commandHistory_ = history;

    // Update all registered tools
    for (auto& [id, tool] : tools_) {
        tool->setCommandHistory(history);
    }
}

bool ToolManager::activateTool(const QString& toolId) {
    auto it = tools_.find(toolId);
    if (it == tools_.end()) {
        qWarning() << "ToolManager: Unknown tool ID" << toolId;
        return false;
    }

    // Deactivate current tool if different
    if (activeTool_ && activeTool_->id() != toolId) {
        activeTool_->deactivate();
    }

    activeTool_ = it->second.get();
    activeTool_->activate();

    emit activeToolChanged(toolId);
    updateStatusPrompt();

    qDebug() << "ToolManager: Activated tool" << toolId;
    return true;
}

void ToolManager::deactivateTool() {
    if (activeTool_) {
        activeTool_->deactivate();
        activeTool_ = nullptr;

        emit activeToolChanged(QString());
        emit statusPromptChanged(QString());

        qDebug() << "ToolManager: Deactivated tool";
    }
}

Tool* ToolManager::tool(const QString& toolId) const {
    auto it = tools_.find(toolId);
    return (it != tools_.end()) ? it->second.get() : nullptr;
}

ToolResult ToolManager::handleMousePress(
    const Geometry::Point2D& worldPos,
    QMouseEvent* event
) {
    if (!activeTool_) {
        return ToolResult::Ignored;
    }

    ToolResult result = activeTool_->handleMousePress(worldPos, event);

    // Update status prompt after handling
    updateStatusPrompt();

    // Emit geometry changed if tool completed an action
    if (result == ToolResult::Completed) {
        emit geometryChanged();
    }

    return result;
}

ToolResult ToolManager::handleMouseMove(
    const Geometry::Point2D& worldPos,
    QMouseEvent* event
) {
    if (!activeTool_) {
        return ToolResult::Ignored;
    }

    return activeTool_->handleMouseMove(worldPos, event);
}

ToolResult ToolManager::handleMouseRelease(
    const Geometry::Point2D& worldPos,
    QMouseEvent* event
) {
    if (!activeTool_) {
        return ToolResult::Ignored;
    }

    return activeTool_->handleMouseRelease(worldPos, event);
}

ToolResult ToolManager::handleKeyPress(QKeyEvent* event) {
    if (!activeTool_) {
        return ToolResult::Ignored;
    }

    ToolResult result = activeTool_->handleKeyPress(event);

    // Update status prompt after handling
    updateStatusPrompt();

    // If tool was cancelled, deactivate it
    if (result == ToolResult::Cancelled) {
        // Check if tool wants to stay active (continuous mode)
        if (activeTool_->state() == ToolState::Inactive) {
            deactivateTool();
        }
    }

    return result;
}

void ToolManager::render(QPainter& painter, const Viewport& viewport) {
    if (activeTool_) {
        activeTool_->render(painter, viewport);
    }
}

void ToolManager::updateStatusPrompt() {
    if (activeTool_) {
        emit statusPromptChanged(activeTool_->statusPrompt());
    }
}

} // namespace UI
} // namespace OwnCAD
