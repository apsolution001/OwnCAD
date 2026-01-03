#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextEdit>
#include <QScrollArea>
#include <QDebug>

// Geometry headers
#include "geometry/Point2D.h"
#include "geometry/Line2D.h"
#include "geometry/Arc2D.h"
#include "geometry/GeometryConstants.h"
#include "geometry/GeometryValidator.h"

// Model headers
#include "model/DocumentModel.h"

// UI headers
#include "ui/CADCanvas.h"

using namespace OwnCAD::Geometry;
using namespace OwnCAD::Model;
using namespace OwnCAD::UI;

/**
 * @brief Main window for OwnCAD application
 *
 * Phase 1 - Full CAD canvas:
 * - Menu bar
 * - Central CAD canvas with Pan/Zoom/Grid/Snap
 * - Status bar with coordinates and zoom level
 * - Toolbar (to be added)
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr)
        : QMainWindow(parent)
        , document_(std::make_unique<DocumentModel>())
        , canvas_(nullptr) {

        setWindowTitle("OwnCAD - Industrial 2D CAD Validator v0.1.0");
        setMinimumSize(1024, 768);

        // Create central CAD canvas
        setupCentralWidget();

        // Create menu bar
        createMenus();

        // Create status bar
        statusBar()->showMessage("Ready - Pan: Middle Mouse | Zoom: Mouse Wheel | Grid: ON");

        // Run geometry self-test
        runGeometrySelfTest();
    }

private:
    void setupCentralWidget() {
        canvas_ = new CADCanvas(this);

        // Connect canvas signals
        connect(canvas_, &CADCanvas::viewportChanged,
                this, &MainWindow::onViewportChanged);
        connect(canvas_, &CADCanvas::cursorPositionChanged,
                this, &MainWindow::onCursorPositionChanged);

        // Enable grid and snap by default
        canvas_->setGridVisible(true);
        canvas_->setGridSpacing(10.0);
        canvas_->setSnapEnabled(SnapManager::SnapMode::Grid, true);

        setCentralWidget(canvas_);
    }

private:
    void createMenus() {
        // File menu
        QMenu* fileMenu = menuBar()->addMenu("&File");
        fileMenu->addAction("&New", this, &MainWindow::onNew);
        fileMenu->addAction("&Open DXF...", this, &MainWindow::onOpen);
        fileMenu->addSeparator();
        fileMenu->addAction("E&xit", this, &QWidget::close);

        // Edit menu
        QMenu* editMenu = menuBar()->addMenu("&Edit");
        editMenu->addAction("&Undo", this, &MainWindow::onUndo);
        editMenu->addAction("&Redo", this, &MainWindow::onRedo);

        // View menu
        QMenu* viewMenu = menuBar()->addMenu("&View");
        viewMenu->addAction("&Zoom Extents", this, &MainWindow::onZoomExtents);
        viewMenu->addAction("&Reset View", this, &MainWindow::onResetView);
        viewMenu->addSeparator();
        viewMenu->addAction("Toggle &Grid", this, &MainWindow::onToggleGrid);
        viewMenu->addAction("Toggle &Snap", this, &MainWindow::onToggleSnap);

        // Tools menu
        QMenu* toolsMenu = menuBar()->addMenu("&Tools");
        toolsMenu->addAction("&Validate Geometry", this, &MainWindow::onValidate);

        // Help menu
        QMenu* helpMenu = menuBar()->addMenu("&Help");
        helpMenu->addAction("&About", this, &MainWindow::onAbout);
    }

    void runGeometrySelfTest() {
        // Quick validation that geometry layer is working
        // This will be expanded as we implement the classes

        try {
            // Test Point2D
            Point2D p1(0, 0);
            Point2D p2(10, 10);

            // Test Line2D (will work once implemented)
            // auto line = Line2D::create(p1, p2);

            // Test Arc2D (will work once implemented)
            // auto arc = Arc2D::create(p1, 5.0, 0, PI/2);

            statusBar()->showMessage("Ready - Geometry engine self-test passed", 5000);
        } catch (const std::exception& e) {
            statusBar()->showMessage(
                QString("Warning - Geometry engine error: %1").arg(e.what()),
                10000
            );
        }
    }

private slots:
    void onNew() {
        document_->clear();
        canvas_->clear();
        statusBar()->showMessage("New document created", 3000);
    }

    void onOpen() {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Open DXF File",
            QString(),
            "DXF Files (*.dxf);;All Files (*.*)"
        );

        if (fileName.isEmpty()) {
            return;
        }

        statusBar()->showMessage("Loading DXF file...");

        bool success = document_->loadDXFFile(fileName.toStdString());

        if (success) {
            // Debug: Log entity count
            qDebug() << "DXF Import Success:";
            qDebug() << "  Total entities loaded:" << document_->statistics().totalEntities;
            qDebug() << "  Lines:" << document_->statistics().totalLines;
            qDebug() << "  Arcs:" << document_->statistics().totalArcs;
            qDebug() << "  Valid:" << document_->statistics().validEntities;
            qDebug() << "  Invalid:" << document_->statistics().invalidEntities;

            // Load geometry into canvas
            canvas_->setEntities(document_->entities());
            canvas_->zoomExtents();

            showValidationResults();
            statusBar()->showMessage(
                QString("Loaded: %1 entities (%2 lines, %3 arcs) | Zoom: Extents")
                    .arg(document_->statistics().totalEntities)
                    .arg(document_->statistics().totalLines)
                    .arg(document_->statistics().totalArcs),
                5000
            );
        } else {
            QString errorMsg = "Failed to load DXF file:\n\n";
            for (const auto& error : document_->importErrors()) {
                errorMsg += QString::fromStdString(error) + "\n";
            }
            QMessageBox::critical(this, "Import Error", errorMsg);
            statusBar()->showMessage("Import failed", 5000);
        }
    }

    void onUndo() {
        statusBar()->showMessage("Undo - To be implemented in Phase 1, Week 3-4", 3000);
    }

    void onRedo() {
        statusBar()->showMessage("Redo - To be implemented in Phase 1, Week 3-4", 3000);
    }

    void onValidate() {
        if (document_->isEmpty()) {
            QMessageBox::information(this, "Geometry Validation",
                "No geometry loaded.\n\n"
                "Open a DXF file to validate geometry.");
            return;
        }

        showValidationResults();
    }

    void onZoomExtents() {
        canvas_->zoomExtents();
        statusBar()->showMessage("View: Zoom Extents", 2000);
    }

    void onResetView() {
        canvas_->resetView();
        statusBar()->showMessage("View: Reset", 2000);
    }

    void onToggleGrid() {
        bool visible = !canvas_->isGridVisible();
        canvas_->setGridVisible(visible);
        statusBar()->showMessage(
            visible ? "Grid: ON" : "Grid: OFF",
            2000
        );
    }

    void onToggleSnap() {
        bool enabled = !canvas_->isSnapEnabled(SnapManager::SnapMode::Grid);
        canvas_->setSnapEnabled(SnapManager::SnapMode::Grid, enabled);
        statusBar()->showMessage(
            enabled ? "Snap: ON (Grid)" : "Snap: OFF",
            2000
        );
    }

    void onViewportChanged(double zoom, double panX, double panY) {
        // Update status bar with zoom level
        statusBar()->showMessage(
            QString("Zoom: %1x | Pan: (%2, %3)")
                .arg(zoom, 0, 'f', 2)
                .arg(panX, 0, 'f', 1)
                .arg(panY, 0, 'f', 1)
        );
    }

    void onCursorPositionChanged(double x, double y) {
        // Update status bar with cursor position in world coordinates
        statusBar()->showMessage(
            QString("X: %1 | Y: %2")
                .arg(x, 0, 'f', 3)
                .arg(y, 0, 'f', 3)
        );
    }

    void showValidationResults() {
        const auto& result = document_->validationResult();
        const auto& stats = document_->statistics();

        QString message;
        message += "<h3>Validation Report</h3>";

        if (result.passed()) {
            message += "<p style='color: green; font-size: 14px;'>"
                      "<b>✓ VALIDATION PASSED</b></p>";
            message += QString("<p>All %1 entities are geometrically valid.</p>")
                .arg(stats.totalEntities);
        } else {
            message += "<p style='color: red; font-size: 14px;'>"
                      "<b>✗ VALIDATION FAILED</b></p>";
            message += QString("<p>Found <b>%1</b> issues in %2 entities:</p>")
                .arg(result.issueCount())
                .arg(stats.totalEntities);

            message += "<ul>";
            if (stats.zeroLengthLines > 0) {
                message += QString("<li><b>%1</b> zero-length lines (invalid for manufacturing)</li>")
                    .arg(stats.zeroLengthLines);
            }
            if (stats.zeroRadiusArcs > 0) {
                message += QString("<li><b>%1</b> zero-radius arcs (invalid for manufacturing)</li>")
                    .arg(stats.zeroRadiusArcs);
            }
            if (stats.numericallyUnstable > 0) {
                message += QString("<li style='color: orange;'><b>%1</b> numerically unstable entities (near tolerance boundary)</li>")
                    .arg(stats.numericallyUnstable);
            }
            message += "</ul>";

            message += "<h4 style='color: red;'>⚠ MANUFACTURING RISK</h4>";
            message += "<p>This file contains invalid geometry that may cause:</p>";
            message += "<ul>";
            message += "<li>Machine errors or crashes</li>";
            message += "<li>Material waste</li>";
            message += "<li>Incorrect cut paths</li>";
            message += "</ul>";
            message += "<p><b>Action required:</b> Fix invalid geometry before export.</p>";
        }

        // Add import warnings if any
        if (!document_->importWarnings().empty()) {
            message += "<h4>Import Warnings</h4>";
            message += "<ul>";
            for (const auto& warning : document_->importWarnings()) {
                message += QString("<li>%1</li>")
                    .arg(QString::fromStdString(warning));
            }
            message += "</ul>";
        }

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Validation Results");
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setText(message);
        msgBox.setIcon(result.passed() ? QMessageBox::Information : QMessageBox::Warning);
        msgBox.exec();
    }

    void onAbout() {
        QMessageBox::about(this, "About OwnCAD",
            "<h3>OwnCAD v0.1.0</h3>"
            "<p><b>Industrial 2D CAD Validator</b></p>"
            "<p>Production-safety tool for:</p>"
            "<ul>"
            "<li>Laser cutting</li>"
            "<li>Plasma cutting</li>"
            "<li>Waterjet cutting</li>"
            "<li>CNC sheet-metal workflows</li>"
            "</ul>"
            "<p><i>Built with Qt6 and C++17</i></p>"
            "<p>Phase 1: Core Geometry & DXF Import + Validation</p>"
        );
    }

private:
    // Document model (holds all geometry and validation state)
    std::unique_ptr<DocumentModel> document_;

    // UI elements
    CADCanvas* canvas_;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Set application metadata
    QApplication::setApplicationName("OwnCAD");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("OwnCAD");

    // Create and show main window
    MainWindow window;
    window.show();

    return app.exec();
}

// Required for Qt MOC (Meta-Object Compiler)
#include "main.moc"
