#pragma once

#include <QDialog>
#include <QDoubleSpinBox>
#include <optional>

namespace OwnCAD {
namespace UI {

/**
 * @brief Dialog for entering exact rotation angle
 *
 * Allows user to type a specific angle value in degrees.
 * Positive = Counter-clockwise (CCW)
 * Negative = Clockwise (CW)
 */
class RotateInputDialog : public QDialog {
    Q_OBJECT

public:
    explicit RotateInputDialog(QWidget* parent = nullptr);
    ~RotateInputDialog() override = default;

    /**
     * @brief Get the entered angle in degrees
     * @return Angle if user confirmed, std::nullopt if cancelled
     */
    std::optional<double> getAngleDegrees() const;

    /**
     * @brief Set initial angle value (for showing current preview angle)
     * @param angleDegrees Angle in degrees
     */
    void setInitialAngle(double angleDegrees);

private:
    void setupUI();

    QDoubleSpinBox* angleSpinBox_ = nullptr;
    bool accepted_ = false;
};

} // namespace UI
} // namespace OwnCAD
