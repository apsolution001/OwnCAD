#include "ui/GridSettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>

namespace OwnCAD {
namespace UI {

GridSettingsDialog::GridSettingsDialog(const GridSettings& current, QWidget* parent)
    : QDialog(parent)
    , currentSettings_(current)
    , unitsComboBox_(nullptr)
    , spacingSpinBox_(nullptr)
    , buttonBox_(nullptr)
    , resultSettings_(current) {

    setWindowTitle("Grid Settings");
    setModal(true);
    setMinimumWidth(350);

    setupUI();
    connectSignals();
}

GridSettingsDialog::~GridSettingsDialog() {
}

void GridSettingsDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Grid settings group
    QGroupBox* gridGroup = new QGroupBox("Grid Configuration");
    QFormLayout* formLayout = new QFormLayout(gridGroup);

    // Units combo box
    unitsComboBox_ = new QComboBox(this);
    unitsComboBox_->addItem("Millimeters (mm)", static_cast<int>(GridUnits::Millimeters));
    unitsComboBox_->addItem("Centimeters (cm)", static_cast<int>(GridUnits::Centimeters));
    unitsComboBox_->addItem("Inches (in)", static_cast<int>(GridUnits::Inches));
    unitsComboBox_->setCurrentIndex(static_cast<int>(currentSettings_.units));
    formLayout->addRow("Units:", unitsComboBox_);

    // Spacing spin box
    spacingSpinBox_ = new QDoubleSpinBox(this);
    spacingSpinBox_->setMinimum(0.1);
    spacingSpinBox_->setMaximum(1000.0);
    spacingSpinBox_->setDecimals(2);
    spacingSpinBox_->setSingleStep(1.0);
    spacingSpinBox_->setValue(currentSettings_.spacing);
    spacingSpinBox_->setSuffix(QString(" %1").arg(unitsToString(currentSettings_.units)));
    formLayout->addRow("Grid Spacing:", spacingSpinBox_);

    // Add info label
    QLabel* infoLabel = new QLabel(
        "<i>Major grid lines appear every 10× spacing.</i>"
    );
    infoLabel->setWordWrap(true);
    formLayout->addRow("", infoLabel);

    mainLayout->addWidget(gridGroup);

    // Button box (OK/Cancel)
    buttonBox_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    mainLayout->addWidget(buttonBox_);

    setLayout(mainLayout);
}

void GridSettingsDialog::connectSignals() {
    connect(unitsComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GridSettingsDialog::onUnitsChanged);
    connect(spacingSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &GridSettingsDialog::onSpacingChanged);
    connect(buttonBox_, &QDialogButtonBox::accepted,
            this, &GridSettingsDialog::onAccepted);
    connect(buttonBox_, &QDialogButtonBox::rejected,
            this, &GridSettingsDialog::onRejected);
}

GridSettings GridSettingsDialog::settings() const {
    return resultSettings_;
}

QString GridSettingsDialog::unitsToString(GridUnits units) const {
    switch (units) {
        case GridUnits::Millimeters:
            return "mm";
        case GridUnits::Centimeters:
            return "cm";
        case GridUnits::Inches:
            return "in";
        default:
            return "mm";
    }
}

GridUnits GridSettingsDialog::stringToUnits(const QString& str) const {
    if (str == "cm") return GridUnits::Centimeters;
    if (str == "in") return GridUnits::Inches;
    return GridUnits::Millimeters;
}

void GridSettingsDialog::onUnitsChanged(int index) {
    GridUnits newUnits = static_cast<GridUnits>(unitsComboBox_->itemData(index).toInt());

    // Update suffix on spacing spin box
    spacingSpinBox_->setSuffix(QString(" %1").arg(unitsToString(newUnits)));

    // Update result settings
    resultSettings_.units = newUnits;
}

void GridSettingsDialog::onSpacingChanged(double value) {
    // Update result settings
    resultSettings_.spacing = value;
}

void GridSettingsDialog::onAccepted() {
    // Validate settings before accepting
    if (resultSettings_.spacing < 0.1) {
        resultSettings_.spacing = 0.1;
    }

    accept();
}

void GridSettingsDialog::onRejected() {
    // Restore original settings
    resultSettings_ = currentSettings_;
    reject();
}

} // namespace UI
} // namespace OwnCAD
