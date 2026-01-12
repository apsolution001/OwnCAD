#pragma once

#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>

namespace OwnCAD {
namespace UI {

/**
 * @brief Units for grid spacing and display
 *
 * Industrial CAD standard units for fabrication workflows.
 */
enum class GridUnits {
    Millimeters = 0,  // mm (metric default)
    Centimeters = 1,  // cm (metric)
    Inches = 2        // in (imperial)
};

/**
 * @brief Grid settings configuration structure
 *
 * Stores user-configurable grid properties.
 */
struct GridSettings {
    GridUnits units;       // Display units
    double spacing;        // Grid spacing in current units
    bool visible;          // Grid visibility

    GridSettings()
        : units(GridUnits::Millimeters)
        , spacing(10.0)
        , visible(true) {
    }
};

/**
 * @brief Grid settings dialog
 *
 * Allows user to configure:
 * - Units (mm, cm, inches)
 * - Grid spacing
 * - Grid visibility
 *
 * Design principles:
 * - Simple, focused dialog (no feature creep)
 * - Industrial aesthetics (clean, professional)
 * - Validates input (positive spacing only)
 */
class GridSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit GridSettingsDialog(const GridSettings& current, QWidget* parent = nullptr);
    ~GridSettingsDialog();

    /**
     * @brief Get configured grid settings
     * @return Grid settings from user input
     */
    GridSettings settings() const;

private:
    void setupUI();
    void connectSignals();
    QString unitsToString(GridUnits units) const;
    GridUnits stringToUnits(const QString& str) const;

private slots:
    void onUnitsChanged(int index);
    void onSpacingChanged(double value);
    void onAccepted();
    void onRejected();

private:
    // Current settings (passed from parent)
    GridSettings currentSettings_;

    // UI widgets
    QComboBox* unitsComboBox_;
    QDoubleSpinBox* spacingSpinBox_;
    QDialogButtonBox* buttonBox_;

    // Result settings (updated on Accept)
    GridSettings resultSettings_;
};

} // namespace UI
} // namespace OwnCAD
