#pragma once

#include "ui/Tool.h"
#include "geometry/Point2D.h"
#include "geometry/Line2D.h"
#include <optional>

namespace OwnCAD {
namespace UI {

/**
 * @brief Line drawing tool
 *
 * Workflow:
 * 1. Activate tool
 * 2. Click first point (snap-aware)
 * 3. Move mouse - preview line shown
 * 4. Click second point - line committed
 * 5. Tool stays active for continuous drawing
 * 6. ESC to cancel current line or exit tool
 *
 * State machine:
 * [Inactive] --activate()--> [WaitingFirstPoint]
 * [WaitingFirstPoint] --click--> [WaitingSecondPoint]
 * [WaitingSecondPoint] --click--> [Commit] --> [WaitingFirstPoint]
 * [WaitingSecondPoint] --ESC--> [WaitingFirstPoint]
 * [WaitingFirstPoint] --ESC--> [Inactive]
 * [Any] --deactivate()--> [Inactive]
 */
class LineTool : public Tool {
public:
    LineTool();
    ~LineTool() override = default;

    // Tool interface
    QString name() const override { return QStringLiteral("Line"); }
    QString id() const override { return QStringLiteral("line"); }

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

private:
    /**
     * @brief Attempt to commit the line to document
     * @return true if line was created successfully
     */
    bool commitLine();

    /**
     * @brief Reset to waiting for first point
     */
    void resetToFirstPoint();

    /**
     * @brief Cancel and reset fully
     */
    void cancel();

    // State
    ToolState state_ = ToolState::Inactive;

    // Points
    std::optional<Geometry::Point2D> firstPoint_;
    std::optional<Geometry::Point2D> currentPoint_;  // Current mouse position (for preview)

    // Visual settings
    static constexpr int PREVIEW_LINE_WIDTH = 1;
    static constexpr int POINT_MARKER_SIZE = 6;
};

} // namespace UI
} // namespace OwnCAD
