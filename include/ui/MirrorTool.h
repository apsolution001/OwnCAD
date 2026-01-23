#pragma once

#include "ui/Tool.h"
#include "geometry/Point2D.h"
#include <optional>
#include <vector>
#include <string>

namespace OwnCAD {
namespace UI {

/**
 * @brief Tool for mirroring selected entities across a user-defined axis.
 *
 * Workflow:
 * 1. Select entities (before activating tool)
 * 2. Activate tool -> WaitingForAxisPoint1
 * 3. Click first axis point (or press X/Y for quick axis)
 * 4. Click second axis point -> preview shown
 * 5. Commit mirror
 *
 * Features:
 * - Tab key toggles keep/delete original
 * - X key: quick horizontal axis through selection center
 * - Y key: quick vertical axis through selection center
 * - ESC: cancel and return to select mode
 *
 * IMPORTANT: Arc direction is inverted during mirror (CCW <-> CW).
 * This is mathematically correct and critical for CNC toolpaths.
 */
class MirrorTool : public Tool {
public:
    MirrorTool();
    ~MirrorTool() override = default;

    // Tool identification
    QString name() const override { return QStringLiteral("Mirror"); }
    QString id() const override { return QStringLiteral("mirror"); }

    // Tool interface
    void activate() override;
    void deactivate() override;
    ToolState state() const override { return state_; }
    QString statusPrompt() const override;

    ToolResult handleMousePress(const Geometry::Point2D& worldPos, QMouseEvent* event) override;
    ToolResult handleMouseMove(const Geometry::Point2D& worldPos, QMouseEvent* event) override;
    ToolResult handleMouseRelease(const Geometry::Point2D& worldPos, QMouseEvent* event) override;
    ToolResult handleKeyPress(QKeyEvent* event) override;

    void render(QPainter& painter, const Viewport& viewport) override;

    /**
     * @brief Set the handles of entities to mirror (called before activation)
     * @param handles Vector of entity handles from selection
     */
    void setSelectedHandles(const std::vector<std::string>& handles);

private:
    // Rendering helpers
    void renderAxisPreview(QPainter& painter, const Viewport& viewport);
    void renderMirroredEntitiesPreview(QPainter& painter, const Viewport& viewport);
    void renderEntityMirrored(QPainter& painter, const Viewport& viewport,
                               const std::string& handle);

    // Commit the mirror operation
    bool commitMirror();

    // Calculate selection center for quick axis shortcuts
    Geometry::Point2D calculateSelectionCenter() const;

    // Reset state
    void resetToAxisPoint1();
    void cancel();

    // State
    ToolState state_ = ToolState::Inactive;
    std::vector<std::string> selectedHandles_;

    // Mirror axis definition
    std::optional<Geometry::Point2D> axisPoint1_;
    std::optional<Geometry::Point2D> axisPoint2_;
    std::optional<Geometry::Point2D> currentPoint_;  // For live preview

    // Options
    bool keepOriginal_ = false;  // Tab toggles this

    // Rendering constants
    static constexpr int AXIS_LINE_WIDTH = 2;
    static constexpr int PREVIEW_LINE_WIDTH = 2;
    static constexpr int POINT_MARKER_SIZE = 8;
};

} // namespace UI
} // namespace OwnCAD
