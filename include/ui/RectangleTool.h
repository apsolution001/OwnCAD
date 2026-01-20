#pragma once

#include "ui/Tool.h"
#include "geometry/Point2D.h"
#include <optional>

namespace OwnCAD {
namespace UI {

/**
 * @brief Rectangle drawing tool (creates 4 Line2D entities)
 *
 * Rectangle is not a special entity type - it creates 4 Line2D segments.
 * This is standard CAD practice for clean geometry export.
 *
 * Workflow:
 * 1. Activate tool
 * 2. Click first corner (snap-aware)
 * 3. Move mouse - preview rectangle shown (4 dashed lines)
 * 4. Click second corner - 4 lines committed to document
 * 5. Tool stays active for continuous drawing
 * 6. ESC to cancel current rectangle or exit tool
 *
 * State machine:
 * [Inactive] --activate()--> [WaitingFirstCorner]
 * [WaitingFirstCorner] --click--> [WaitingSecondCorner]
 * [WaitingSecondCorner] --click--> [Commit 4 lines] --> [WaitingFirstCorner]
 * [WaitingSecondCorner] --ESC--> [WaitingFirstCorner]
 * [WaitingFirstCorner] --ESC--> [Inactive]
 * [Any] --deactivate()--> [Inactive]
 */
class RectangleTool : public Tool {
public:
    RectangleTool();
    ~RectangleTool() override = default;

    // Tool interface
    QString name() const override { return QStringLiteral("Rectangle"); }
    QString id() const override { return QStringLiteral("rectangle"); }

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
     * @brief Commit rectangle as 4 Line2D entities
     * @return true if all 4 lines were created successfully
     */
    bool commitRectangle();

    /**
     * @brief Reset to waiting for first corner
     */
    void resetToFirstCorner();

    /**
     * @brief Cancel and reset fully
     */
    void cancel();

    // State
    ToolState state_ = ToolState::Inactive;

    // Corner points
    std::optional<Geometry::Point2D> firstCorner_;
    std::optional<Geometry::Point2D> currentCorner_;  // Current mouse position (for preview)

    // Visual settings
    static constexpr int PREVIEW_LINE_WIDTH = 1;
    static constexpr int CORNER_MARKER_SIZE = 6;
};

} // namespace UI
} // namespace OwnCAD
