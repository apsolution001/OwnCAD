#include "ui/RotateInputDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

namespace OwnCAD {
namespace UI {

RotateInputDialog::RotateInputDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Enter Rotation Angle"));
    setModal(true);
    setFixedSize(280, 120);
    setupUI();
}

void RotateInputDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // Angle input row
    auto* inputLayout = new QHBoxLayout();
    auto* label = new QLabel(tr("Angle (degrees):"), this);

    angleSpinBox_ = new QDoubleSpinBox(this);
    angleSpinBox_->setRange(-3600.0, 3600.0);  // Allow multiple rotations
    angleSpinBox_->setDecimals(2);
    angleSpinBox_->setSuffix(QStringLiteral("\u00B0"));  // Degree symbol
    angleSpinBox_->setValue(0.0);
    angleSpinBox_->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    angleSpinBox_->selectAll();  // Select all text for easy overwrite

    inputLayout->addWidget(label);
    inputLayout->addWidget(angleSpinBox_);
    mainLayout->addLayout(inputLayout);

    // Info label
    auto* infoLabel = new QLabel(
        tr("Positive = CCW, Negative = CW"),
        this
    );
    infoLabel->setStyleSheet("color: gray; font-size: 10px;");
    mainLayout->addWidget(infoLabel);

    // Buttons
    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        accepted_ = true;
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Focus on spinbox
    angleSpinBox_->setFocus();
}

std::optional<double> RotateInputDialog::getAngleDegrees() const {
    if (accepted_) {
        return angleSpinBox_->value();
    }
    return std::nullopt;
}

void RotateInputDialog::setInitialAngle(double angleDegrees) {
    angleSpinBox_->setValue(angleDegrees);
    angleSpinBox_->selectAll();
}

} // namespace UI
} // namespace OwnCAD
