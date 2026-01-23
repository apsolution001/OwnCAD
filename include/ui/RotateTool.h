#pragma once

#include "ui/Tool.h"
#include "geometry/Point2D.h"
#include <optional>
#include <vector>
#include <string>

namespace OwnCAD {
namespace UI {

/**
 * @brief Rotate tool for rotating selected entities around a center point
 *
 * Workflow:
 * 1. User selects entities (before activating tool)
 * 2. Activate tool - checks for selection
 * 3. Click rotation center
 * 4. Move mouse - preview shows rotated entities
 * 5. Click to set angle (hold Shift for angle snap)
 * 6. Tool deactivates after commit
 *
 * Angle snap (when Shift held):
 * - 15°, 30°, 45°, 60°, 90° increments
 *
 * State machine:
 * [Inactive] --activate()--> [WaitingForCenter] (if selection exists)
 * [WaitingForCenter] --click--> [WaitingForAngle]
 * [WaitingForAngle] --click--> [Commit] --> [Inactive]
 * [WaitingForAngle] --ESC--> [WaitingForCenter]
 * [WaitingForCenter] --ESC--> [Inactive]
 */
class RotateTool : public Tool {
public:
    RotateTool();
    ~RotateTool() override = default;

    // Tool interface
    QString name() const override { return QStringLiteral("Rotate"); }
    QString id() const override { return QStringLiteral("rotate"); }

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
     * @brief Commit the rotation operation to document (uses calculated angle)
     * @return true if rotation was successful
     */
    bool commitRotation();

    /**
     * @brief Commit rotation with specified angle (for numeric input)
     * @param angleRadians Rotation angle in radians
     * @return true if rotation was successful
     */
    bool commitRotationWithAngle(double angleRadians);

    /**
     * @brief Reset to waiting for center point
     */
    void resetToCenter();

    /**
     * @brief Cancel and reset fully
     */
    void cancel();

    /**
     * @brief Calculate rotation angle from center to current mouse position
     * @param applySnap If true, snap to 15° increments
     * @return Angle in radians
     */
    double calculateAngle(bool applySnap) const;

    /**
     * @brief Render a single entity preview at rotated position
     */
    void renderEntityPreview(
        QPainter& painter,
        const Viewport& viewport,
        const std::string& handle,
        double angleRadians
    );

    // State
    ToolState state_ = ToolState::Inactive;

    // Points
    std::optional<Geometry::Point2D> centerPoint_;
    std::optional<Geometry::Point2D> startPoint_;    // First click after center (defines 0° reference)
    std::optional<Geometry::Point2D> currentPoint_;  // Current mouse position

    // Angle tracking
    double baseAngle_ = 0.0;  // Angle from center to start point

    // Modifier state
    bool angleSnapEnabled_ = false;

    // Selection snapshot (captured at activation)
    std::vector<std::string> selectedHandles_;

    // Visual settings
    static constexpr int PREVIEW_LINE_WIDTH = 1;
    static constexpr int CENTER_MARKER_SIZE = 10;
    static constexpr double ANGLE_SNAP_INCREMENT = 0.261799387799;  // 15 degrees in radians (PI/12)
};

} // namespace UI
} // namespace OwnCAD
