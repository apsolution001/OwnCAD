#pragma once

#include "ui/Tool.h"
#include "geometry/Point2D.h"
#include <optional>
#include <vector>
#include <string>

namespace OwnCAD {
namespace UI {

/**
 * @brief Move tool for translating selected entities
 *
 * Workflow:
 * 1. User selects entities (before activating tool)
 * 2. Activate tool - checks for selection
 * 3. Click base point (snap-aware)
 * 4. Move mouse - preview shows entities at new position
 * 5. Click destination - commit move
 * 6. Tool deactivates after commit
 *
 * State machine:
 * [Inactive] --activate()--> [WaitingForBasePoint] (if selection exists)
 * [Inactive] --activate()--> [Inactive] (if no selection - warning)
 * [WaitingForBasePoint] --click--> [WaitingForDestination]
 * [WaitingForDestination] --click--> [Commit] --> [Inactive]
 * [WaitingForDestination] --ESC--> [WaitingForBasePoint]
 * [WaitingForBasePoint] --ESC--> [Inactive]
 */
class MoveTool : public Tool {
public:
    MoveTool();
    ~MoveTool() override = default;

    // Tool interface
    QString name() const override { return QStringLiteral("Move"); }
    QString id() const override { return QStringLiteral("move"); }

    void activate() override;
    void deactivate() override;

    ToolState state() const override { return state_; }
    QString statusPrompt() const override;

    // Event handlers
    ToolResult handleMousePress(
        const Geometry::Point2D& worldPos,
        QMouseEvent* event
    ) override;

    ToolResult handleMouseMove(
        const Geometry::Point2D& worldPos,
        QMouseEvent* event
    ) override;

    ToolResult handleMouseRelease(
        const Geometry::Point2D& worldPos,
        QMouseEvent* event
    ) override;

    ToolResult handleKeyPress(QKeyEvent* event) override;

    // Rendering
    void render(QPainter& painter, const Viewport& viewport) override;

    // Selection handoff (called by CADCanvas before activation)
    void setSelectedHandles(const std::vector<std::string>& handles);
    const std::vector<std::string>& selectedHandles() const { return selectedHandles_; }

private:
    /**
     * @brief Commit the move operation to document
     * @return true if move was successful
     */
    bool commitMove();

    /**
     * @brief Reset to waiting for base point
     */
    void resetToBasePoint();

    /**
     * @brief Cancel and reset fully
     */
    void cancel();

    /**
     * @brief Render a single entity preview at translated position
     */
    void renderEntityPreview(
        QPainter& painter,
        const Viewport& viewport,
        const std::string& handle,
        double dx,
        double dy
    );

    // State
    ToolState state_ = ToolState::Inactive;

    // Points
    std::optional<Geometry::Point2D> basePoint_;
    std::optional<Geometry::Point2D> currentPoint_;

    // Selection snapshot (captured at activation)
    std::vector<std::string> selectedHandles_;

    // Visual settings
    static constexpr int PREVIEW_LINE_WIDTH = 1;
    static constexpr int POINT_MARKER_SIZE = 8;
    static constexpr int DISPLACEMENT_LINE_WIDTH = 1;
};

} // namespace UI
} // namespace OwnCAD
