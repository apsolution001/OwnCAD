#pragma once

#include "ui/Tool.h"
#include "geometry/Point2D.h"
#include "geometry/Arc2D.h"
#include <optional>

namespace OwnCAD {
namespace UI {

/**
 * @brief Arc drawing tool using Center-Start-End workflow
 *
 * Workflow:
 * 1. Activate tool
 * 2. Click center point
 * 3. Click start point (defines radius and start angle)
 * 4. Move mouse - preview arc shown
 * 5. Click end point - arc committed
 * 6. Tool stays active for continuous drawing
 * 7. ESC to cancel current arc or exit tool
 * 8. D key to toggle direction (CCW/CW)
 *
 * State machine:
 * [Inactive] --activate()--> [WaitingCenterPoint]
 * [WaitingCenterPoint] --click--> [WaitingStartPoint]
 * [WaitingStartPoint] --click--> [WaitingEndPoint]
 * [WaitingEndPoint] --click--> [Commit] --> [WaitingCenterPoint]
 * [WaitingEndPoint] --ESC--> [WaitingCenterPoint]
 * [WaitingStartPoint] --ESC--> [WaitingCenterPoint]
 * [WaitingCenterPoint] --ESC--> [Inactive]
 * [Any] --deactivate()--> [Inactive]
 *
 * CRITICAL FOR MANUFACTURING:
 * Arc direction (CCW vs CW) affects toolpath generation.
 * Default is CCW (standard CAD convention).
 */
class ArcTool : public Tool {
public:
    ArcTool();
    ~ArcTool() override = default;

    // Tool interface
    QString name() const override { return QStringLiteral("Arc"); }
    QString id() const override { return QStringLiteral("arc"); }

    void activate() override;
    void deactivate() override;

    ToolState state() const override;
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
     * @brief Internal state (more granular than base ToolState)
     */
    enum class ArcState {
        Inactive,
        WaitingCenterPoint,
        WaitingStartPoint,
        WaitingEndPoint
    };

    /**
     * @brief Attempt to commit the arc to document
     * @return true if arc was created successfully
     */
    bool commitArc();

    /**
     * @brief Reset to waiting for center point
     */
    void resetToCenterPoint();

    /**
     * @brief Cancel and reset fully
     */
    void cancel();

    /**
     * @brief Calculate angle from one point to another
     * @param from Origin point
     * @param to Target point
     * @return Angle in radians [0, 2*PI)
     */
    double angleToPoint(const Geometry::Point2D& from, const Geometry::Point2D& to) const;

    /**
     * @brief Render arc preview during WaitingEndPoint state
     */
    void renderArcPreview(QPainter& painter, const Viewport& viewport);

    /**
     * @brief Render direction indicator arrow on preview arc
     */
    void renderDirectionArrow(QPainter& painter, const Viewport& viewport);

    // State
    ArcState arcState_ = ArcState::Inactive;

    // Points (set during workflow)
    std::optional<Geometry::Point2D> centerPoint_;
    std::optional<Geometry::Point2D> startPoint_;
    std::optional<Geometry::Point2D> currentPoint_;  // Current mouse position (for preview)

    // Derived values (calculated from points)
    double radius_ = 0.0;
    double startAngle_ = 0.0;
    double currentAngle_ = 0.0;

    // Arc direction (CCW by default, toggled with D key)
    bool counterClockwise_ = true;

    // Visual settings
    static constexpr int PREVIEW_LINE_WIDTH = 1;
    static constexpr int POINT_MARKER_SIZE = 6;
    static constexpr int DIRECTION_ARROW_SIZE = 12;
};

} // namespace UI
} // namespace OwnCAD
