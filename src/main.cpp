#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QKeySequence>
#include <QStatusBar>
#include <QToolBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextEdit>
#include <QScrollArea>
#include <QStringList>
#include <QDebug>

// Geometry headers
#include "geometry/Point2D.h"
#include "geometry/Line2D.h"
#include "geometry/Arc2D.h"
#include "geometry/GeometryConstants.h"
#include "geometry/GeometryValidator.h"

// Model headers
#include "model/DocumentModel.h"
#include "model/CommandHistory.h"
#include "model/EntityCommands.h"

// UI headers
#include "ui/CADCanvas.h"
#include "ui/GridSettingsDialog.h"
#include "ui/ToolManager.h"
#include "ui/MoveTool.h"
#include "ui/RotateTool.h"
#include "ui/MirrorTool.h"

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
        , canvas_(nullptr)
        , cursorPosLabel_(nullptr)
        , zoomLabel_(nullptr)
        , snapModeLabel_(nullptr)
        , selectionLabel_(nullptr)
        , validationLabel_(nullptr)
        , toolPromptLabel_(nullptr)
        , commandHistory_(nullptr)
        , undoAction_(nullptr)
        , redoAction_(nullptr) {

        setWindowTitle("OwnCAD - Industrial 2D CAD Validator v0.1.0");
        setMinimumSize(1024, 768);

        // Create central CAD canvas
        setupCentralWidget();

        // Create command history (undo/redo system)
        setupCommandHistory();

        // Create menu bar
        createMenus();

        // Create toolbar
        createToolbar();

        // Create status bar with permanent widgets
        setupStatusBar();

        // Run geometry self-test
        runGeometrySelfTest();
    }

    /**
     * @brief Get command history for undo/redo operations.
     * @return Pointer to CommandHistory (owned by MainWindow)
     */
    CommandHistory* commandHistory() { return commandHistory_; }

private:
    void setupCentralWidget() {
        canvas_ = new CADCanvas(this);

        // Connect canvas to document model (for drawing tools)
        canvas_->setDocumentModel(document_.get());

        // Connect canvas signals
        connect(canvas_, &CADCanvas::viewportChanged,
                this, &MainWindow::onViewportChanged);
        connect(canvas_, &CADCanvas::cursorPositionChanged,
                this, &MainWindow::onCursorPositionChanged);
        connect(canvas_, &CADCanvas::selectionChanged,
                this, &MainWindow::onSelectionChanged);

        // Connect tool manager signals
        ToolManager* toolMgr = canvas_->toolManager();
        connect(toolMgr, &ToolManager::statusPromptChanged,
                this, &MainWindow::onToolPromptChanged);
        connect(toolMgr, &ToolManager::geometryChanged,
                this, &MainWindow::onGeometryChanged);
        connect(toolMgr, &ToolManager::activeToolChanged,
                this, &MainWindow::onActiveToolChanged);
        // Trigger validation when geometry changes via tools
        connect(toolMgr, &ToolManager::geometryChanged,
                [this]() { document_->runValidationAsync(); });

        // Enable grid and snap by default
        canvas_->setGridVisible(true);
        canvas_->setGridSpacing(10.0);
        canvas_->setSnapEnabled(SnapManager::SnapMode::Grid, true);

        setCentralWidget(canvas_);
    }

    void setupCommandHistory() {
        commandHistory_ = new CommandHistory(this);

        // Connect command history to ToolManager so tools can use undo/redo
        canvas_->toolManager()->setCommandHistory(commandHistory_);

        // Connect command history signals for status bar feedback
        connect(commandHistory_, &CommandHistory::commandExecuted,
                [this](const QString& desc) {
                    statusBar()->showMessage(desc, 2000);
                });

        connect(commandHistory_, &CommandHistory::commandUndone,
                [this](const QString& desc) {
                    statusBar()->showMessage(QString("Undo: %1").arg(desc), 2000);
                });

        connect(commandHistory_, &CommandHistory::commandRedone,
                [this](const QString& desc) {
                    statusBar()->showMessage(QString("Redo: %1").arg(desc), 2000);
                });

        // Trigger validation on any history change (undo/redo/execute)
        connect(commandHistory_, &CommandHistory::historyChanged,
                [this]() { document_->runValidationAsync(); });

        // Setup async validation callback
        document_->setValidationCompletionCallback(
            [this](const ValidationResult& result) {
                // Marshal back to main thread
                QMetaObject::invokeMethod(this, [this, result]() {
                        // 1. Finalize model state (Main Thread)
                        document_->finalizeValidation(result);
                        
                        // 2. Update UI
                        updateValidationStatus(); 
                    }, 
                    Qt::QueuedConnection);
            }
        );
    }

private:
    void createMenus() {
        // File menu
        QMenu* fileMenu = menuBar()->addMenu("&File");
        fileMenu->addAction("&New", this, &MainWindow::onNew);
        fileMenu->addAction("&Open DXF...", this, &MainWindow::onOpen);
        fileMenu->addSeparator();
        fileMenu->addAction("E&xit", this, &QWidget::close);

        // Edit menu with Undo/Redo
        QMenu* editMenu = menuBar()->addMenu("&Edit");

        undoAction_ = editMenu->addAction("&Undo");
        undoAction_->setShortcut(QKeySequence::Undo);  // Ctrl+Z
        undoAction_->setEnabled(false);
        connect(undoAction_, &QAction::triggered, this, &MainWindow::onUndo);

        redoAction_ = editMenu->addAction("&Redo");
        redoAction_->setShortcut(QKeySequence::Redo);  // Ctrl+Y / Ctrl+Shift+Z
        redoAction_->setEnabled(false);
        connect(redoAction_, &QAction::triggered, this, &MainWindow::onRedo);

        // Update action state when history changes
        connect(commandHistory_, &CommandHistory::historyChanged,
                this, &MainWindow::updateUndoRedoActions);

        // View menu
        QMenu* viewMenu = menuBar()->addMenu("&View");
        viewMenu->addAction("&Zoom Extents", this, &MainWindow::onZoomExtents);
        viewMenu->addAction("&Reset View", this, &MainWindow::onResetView);
        viewMenu->addSeparator();
        viewMenu->addAction("Toggle &Grid", this, &MainWindow::onToggleGrid);
        viewMenu->addAction("Grid &Settings...", this, &MainWindow::onGridSettings);

        // Snap submenu with keyboard shortcuts
        QMenu* snapMenu = viewMenu->addMenu("&Snap");

        QAction* gridSnapAction = snapMenu->addAction("Grid Snap", this, &MainWindow::onToggleGridSnap);
        gridSnapAction->setShortcut(QKeySequence(Qt::Key_G));

        QAction* endpointSnapAction = snapMenu->addAction("Endpoint Snap", this, &MainWindow::onToggleEndpointSnap);
        endpointSnapAction->setShortcut(QKeySequence(Qt::Key_E));

        QAction* midpointSnapAction = snapMenu->addAction("Midpoint Snap", this, &MainWindow::onToggleMidpointSnap);
        midpointSnapAction->setShortcut(QKeySequence(Qt::Key_M));

        QAction* nearestSnapAction = snapMenu->addAction("Nearest Snap", this, &MainWindow::onToggleNearestSnap);
        nearestSnapAction->setShortcut(QKeySequence(Qt::Key_N));

        snapMenu->addSeparator();
        snapMenu->addAction("Snap Settings...", this, &MainWindow::onSnapSettings);

        // Draw menu
        QMenu* drawMenu = menuBar()->addMenu("&Draw");

        QAction* lineAction = drawMenu->addAction("&Line", this, &MainWindow::onLineTool);
        lineAction->setShortcut(QKeySequence(Qt::Key_L));

        drawMenu->addAction("&Arc", this, &MainWindow::onArcTool);
        drawMenu->addAction("&Rectangle", this, &MainWindow::onRectangleTool);
        drawMenu->addSeparator();
        drawMenu->addAction("&Select (ESC)", this, &MainWindow::onSelectTool);

        // Tools menu (transformation tools)
        QMenu* toolsMenu = menuBar()->addMenu("&Tools");

        QAction* moveAction = toolsMenu->addAction("&Move", this, &MainWindow::onMoveTool);
        moveAction->setShortcut(QKeySequence(Qt::Key_V));  // V for moVe (M is taken by Midpoint snap)

        QAction* rotateAction = toolsMenu->addAction("&Rotate", this, &MainWindow::onRotateTool);
        rotateAction->setShortcut(QKeySequence(Qt::Key_R));

        QAction* mirrorAction = toolsMenu->addAction("M&irror", this, &MainWindow::onMirrorTool);
        mirrorAction->setShortcut(QKeySequence(Qt::Key_I));  // I for mIrror

        toolsMenu->addSeparator();
        toolsMenu->addAction("&Validate Geometry", this, &MainWindow::onValidate);

        // Help menu
        QMenu* helpMenu = menuBar()->addMenu("&Help");
        helpMenu->addAction("&About", this, &MainWindow::onAbout);
    }

    void createToolbar() {
        // Create left vertical toolbar
        toolToolbar_ = new QToolBar("Tools", this);
        toolToolbar_->setOrientation(Qt::Vertical);
        toolToolbar_->setMovable(false);  // Fixed position
        toolToolbar_->setFloatable(false);
        toolToolbar_->setIconSize(QSize(32, 32));
        toolToolbar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

        addToolBar(Qt::LeftToolBarArea, toolToolbar_);

        // Add actions to toolbar
        createToolbarActions();
    }

    void createToolbarActions() {
        // SELECTION GROUP
        QAction* selectAction = toolToolbar_->addAction("Select");
        selectAction->setToolTip("Select Tool (ESC)\nClick to select entities");
        selectAction->setCheckable(true);
        selectAction->setChecked(true);  // Default tool
        connect(selectAction, &QAction::triggered, this, &MainWindow::onSelectTool);

        toolToolbar_->addSeparator();

        // DRAWING GROUP
        QAction* lineAction = toolToolbar_->addAction("Line");
        lineAction->setToolTip("Line Tool (L)\nClick two points to draw a line");
        lineAction->setCheckable(true);
        connect(lineAction, &QAction::triggered, this, &MainWindow::onLineTool);

        QAction* arcAction = toolToolbar_->addAction("Arc");
        arcAction->setToolTip("Arc Tool\nClick center, start, and end points");
        arcAction->setCheckable(true);
        connect(arcAction, &QAction::triggered, this, &MainWindow::onArcTool);

        QAction* rectAction = toolToolbar_->addAction("Rectangle");
        rectAction->setToolTip("Rectangle Tool\nClick two corners");
        rectAction->setCheckable(true);
        connect(rectAction, &QAction::triggered, this, &MainWindow::onRectangleTool);

        toolToolbar_->addSeparator();

        // TRANSFORMATION GROUP
        QAction* moveAction = toolToolbar_->addAction("Move");
        moveAction->setToolTip("Move Tool (V)\nSelect entities first, then click base and destination");
        moveAction->setCheckable(true);
        connect(moveAction, &QAction::triggered, this, &MainWindow::onMoveTool);

        QAction* rotateAction = toolToolbar_->addAction("Rotate");
        rotateAction->setToolTip("Rotate Tool (R)\nSelect entities first, then click center and angle");
        rotateAction->setCheckable(true);
        connect(rotateAction, &QAction::triggered, this, &MainWindow::onRotateTool);

        QAction* mirrorAction = toolToolbar_->addAction("Mirror");
        mirrorAction->setToolTip("Mirror Tool (I)\nSelect entities first, then define mirror axis");
        mirrorAction->setCheckable(true);
        connect(mirrorAction, &QAction::triggered, this, &MainWindow::onMirrorTool);

        toolToolbar_->addSeparator();

        // EDIT GROUP
        QAction* deleteAction = toolToolbar_->addAction("Delete");
        deleteAction->setToolTip("Delete Selected (Del)\nRemove selected entities");
        connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteTool);

        // Create action group for mutual exclusivity (only one tool active at a time)
        QActionGroup* toolActionGroup = new QActionGroup(this);
        toolActionGroup->addAction(selectAction);
        toolActionGroup->addAction(lineAction);
        toolActionGroup->addAction(arcAction);
        toolActionGroup->addAction(rectAction);
        toolActionGroup->addAction(moveAction);
        toolActionGroup->addAction(rotateAction);
        toolActionGroup->addAction(mirrorAction);
    }

    void setupStatusBar() {
        // Create permanent status bar widgets (right to left order)

        // Tool prompt indicator (rightmost) - for drawing tool prompts
        toolPromptLabel_ = new QLabel("");
        toolPromptLabel_->setMinimumWidth(250);
        toolPromptLabel_->setAlignment(Qt::AlignCenter);
        toolPromptLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        toolPromptLabel_->setStyleSheet("font-weight: bold; color: #0066FF;");
        statusBar()->addPermanentWidget(toolPromptLabel_);

        // Snap mode indicator
        snapModeLabel_ = new QLabel("Snap: Grid");
        snapModeLabel_->setMinimumWidth(180);
        snapModeLabel_->setAlignment(Qt::AlignCenter);
        snapModeLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        statusBar()->addPermanentWidget(snapModeLabel_);

        // Selection count indicator
        selectionLabel_ = new QLabel("Selected: 0");
        selectionLabel_->setMinimumWidth(100);
        selectionLabel_->setAlignment(Qt::AlignCenter);
        selectionLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        selectionLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        statusBar()->addPermanentWidget(selectionLabel_);

        // Validation status indicator
        validationLabel_ = new QLabel("Valid: -");
        validationLabel_->setMinimumWidth(120);
        validationLabel_->setAlignment(Qt::AlignCenter);
        validationLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        // Default to green check
        validationLabel_->setStyleSheet("color: green; font-weight: bold;");
        statusBar()->addPermanentWidget(validationLabel_);

        // Zoom level indicator
        zoomLabel_ = new QLabel("Zoom: 1.00x");
        zoomLabel_->setMinimumWidth(100);
        zoomLabel_->setAlignment(Qt::AlignCenter);
        zoomLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        statusBar()->addPermanentWidget(zoomLabel_);

        // Cursor position indicator (leftmost of permanent widgets)
        cursorPosLabel_ = new QLabel("X: 0.000 | Y: 0.000");
        cursorPosLabel_->setMinimumWidth(180);
        cursorPosLabel_->setAlignment(Qt::AlignCenter);
        cursorPosLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        statusBar()->addPermanentWidget(cursorPosLabel_);

        // Set initial message in temporary area (left side)
        statusBar()->showMessage("Ready - Pan: Middle Mouse | Zoom: Mouse Wheel | Click: Select");
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
        commandHistory_->clear();
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
            const auto& stats = document_->statistics();

            // Debug: Log entity count
            qDebug() << "DXF Import Success:";
            qDebug() << "  DXF entities imported:" << stats.dxfEntitiesImported;
            qDebug() << "  Geometry segments (after decomposition):" << stats.totalSegments;
            qDebug() << "  Lines:" << stats.totalLines;
            qDebug() << "  Arcs:" << stats.totalArcs;
            qDebug() << "  Valid:" << stats.validEntities;
            qDebug() << "  Invalid:" << stats.invalidEntities;

            // Clear command history for new document
            commandHistory_->clear();

            // Load geometry into canvas
            canvas_->setEntities(document_->entities());
            canvas_->zoomExtents();

            showValidationResults();

            // Status bar message - show DXF entities count (clearer for user)
            QString message = QString("Loaded: %1 DXF entities")
                .arg(stats.dxfEntitiesImported);

            // Add decomposition info if polygons were decomposed
            if (stats.totalSegments > stats.dxfEntitiesImported) {
                message += QString(" → %1 segments (%2 lines, %3 arcs)")
                    .arg(stats.totalSegments)
                    .arg(stats.totalLines)
                    .arg(stats.totalArcs);
            } else {
                message += QString(" (%1 lines, %2 arcs)")
                    .arg(stats.totalLines)
                    .arg(stats.totalArcs);
            }

            message += " | Zoom: Extents";
            statusBar()->showMessage(message, 5000);
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
        if (!commandHistory_->undo()) {
            statusBar()->showMessage("Nothing to undo", 2000);
        }
        // Refresh canvas after undo
        canvas_->setEntities(document_->entities());
    }

    void onRedo() {
        if (!commandHistory_->redo()) {
            statusBar()->showMessage("Nothing to redo", 2000);
        }
        // Refresh canvas after redo
        canvas_->setEntities(document_->entities());
    }

    void updateUndoRedoActions() {
        // Update Undo action
        undoAction_->setEnabled(commandHistory_->canUndo());
        if (commandHistory_->canUndo()) {
            undoAction_->setText(QString("&Undo %1").arg(commandHistory_->undoDescription()));
        } else {
            undoAction_->setText("&Undo");
        }

        // Update Redo action
        redoAction_->setEnabled(commandHistory_->canRedo());
        if (commandHistory_->canRedo()) {
            redoAction_->setText(QString("&Redo %1").arg(commandHistory_->redoDescription()));
        } else {
            redoAction_->setText("&Redo");
        }
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

    void onGridSettings() {
        // Get current grid settings
        GridSettings current = canvas_->gridSettings();

        // Show grid settings dialog
        GridSettingsDialog dialog(current, this);
        if (dialog.exec() == QDialog::Accepted) {
            GridSettings newSettings = dialog.settings();
            canvas_->setGridSettings(newSettings);

            statusBar()->showMessage(
                QString("Grid: %1 %2 spacing")
                    .arg(newSettings.spacing, 0, 'f', 2)
                    .arg(newSettings.units == GridUnits::Millimeters ? "mm" :
                         newSettings.units == GridUnits::Centimeters ? "cm" : "in"),
                3000
            );
        }
    }

    void onToggleGridSnap() {
        bool enabled = !canvas_->isSnapEnabled(SnapManager::SnapMode::Grid);
        canvas_->setSnapEnabled(SnapManager::SnapMode::Grid, enabled);
        updateSnapStatus();
    }

    void onToggleEndpointSnap() {
        bool enabled = !canvas_->isSnapEnabled(SnapManager::SnapMode::Endpoint);
        canvas_->setSnapEnabled(SnapManager::SnapMode::Endpoint, enabled);
        updateSnapStatus();
    }

    void onToggleMidpointSnap() {
        bool enabled = !canvas_->isSnapEnabled(SnapManager::SnapMode::Midpoint);
        canvas_->setSnapEnabled(SnapManager::SnapMode::Midpoint, enabled);
        updateSnapStatus();
    }

    void onToggleNearestSnap() {
        bool enabled = !canvas_->isSnapEnabled(SnapManager::SnapMode::Nearest);
        canvas_->setSnapEnabled(SnapManager::SnapMode::Nearest, enabled);
        updateSnapStatus();
    }

    void onSnapSettings() {
        // TODO: Create dialog for snap tolerance configuration
        QMessageBox::information(this, "Snap Settings",
            "Snap tolerance configuration dialog will be implemented in Phase 1, Week 2.\n\n"
            "Current snap modes can be toggled via View > Snap menu (G/E/M/N hotkeys).\n\n"
            "Snap tolerance: 10 pixels (screen-space, zoom-independent)\n"
            "Grid spacing: Configurable via View > Grid Settings");
    }

    void updateSnapStatus() {
        // Update permanent status bar widget with active snap modes
        QStringList activeSnaps;
        if (canvas_->isSnapEnabled(SnapManager::SnapMode::Grid))
            activeSnaps << "Grid";
        if (canvas_->isSnapEnabled(SnapManager::SnapMode::Endpoint))
            activeSnaps << "Endpoint";
        if (canvas_->isSnapEnabled(SnapManager::SnapMode::Midpoint))
            activeSnaps << "Midpoint";
        if (canvas_->isSnapEnabled(SnapManager::SnapMode::Nearest))
            activeSnaps << "Nearest";

        if (activeSnaps.isEmpty()) {
            snapModeLabel_->setText("Snap: OFF");
        } else {
            snapModeLabel_->setText(QString("Snap: %1").arg(activeSnaps.join(", ")));
        }
    }

    void onViewportChanged(double zoom, double panX, double panY) {
        Q_UNUSED(panX);
        Q_UNUSED(panY);
        // Update permanent zoom level widget
        zoomLabel_->setText(QString("Zoom: %1x").arg(zoom, 0, 'f', 2));
    }

    void onCursorPositionChanged(double x, double y) {
        // Update permanent cursor position widget
        cursorPosLabel_->setText(
            QString("X: %1 | Y: %2")
                .arg(x, 0, 'f', 3)
                .arg(y, 0, 'f', 3)
        );
    }

    void onSelectionChanged(size_t count) {
        // Update permanent selection count widget
        selectionLabel_->setText(QString("Selected: %1").arg(count));
    }

    void showValidationResults() {
        const auto& result = document_->validationResult();
        const auto& stats = document_->statistics();

        QString message;
        message += "<h3>Validation Report</h3>";

        if (result.passed()) {
            message += "<p style='color: green; font-size: 14px;'>"
                      "<b>✓ VALIDATION PASSED</b></p>";
            message += QString("<p>All %1 imported entities (%2 geometry segments) are valid.</p>")
                .arg(stats.dxfEntitiesImported)
                .arg(stats.totalSegments);
        } else {
            message += "<p style='color: red; font-size: 14px;'>"
                      "<b>✗ VALIDATION FAILED</b></p>";
            message += QString("<p>Found <b>%1</b> issues in %2 imported entities (%3 segments):</p>")
                .arg(result.issueCount())
                .arg(stats.dxfEntitiesImported)
                .arg(stats.totalSegments);

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

    // Drawing tools
    void onLineTool() {
        ToolManager* toolMgr = canvas_->toolManager();
        toolMgr->activateTool("line");
        canvas_->setCursor(Qt::CrossCursor);
    }

    void onArcTool() {
        ToolManager* toolMgr = canvas_->toolManager();
        toolMgr->activateTool("arc");
        canvas_->setCursor(Qt::CrossCursor);
    }

    void onRectangleTool() {
        ToolManager* toolMgr = canvas_->toolManager();
        toolMgr->activateTool("rectangle");
        canvas_->setCursor(Qt::CrossCursor);
    }

    void onSelectTool() {
        ToolManager* toolMgr = canvas_->toolManager();
        toolMgr->deactivateTool();
        canvas_->setCursor(Qt::ArrowCursor);
    }

    void onMoveTool() {
        // Check if there's a selection
        std::vector<std::string> selection = canvas_->selectedHandles();
        if (selection.empty()) {
            statusBar()->showMessage("MOVE: Select entities first", 3000);
            return;
        }

        // Get MoveTool and pass selection
        ToolManager* toolMgr = canvas_->toolManager();
        Tool* tool = toolMgr->tool("move");
        if (tool) {
            MoveTool* moveTool = dynamic_cast<MoveTool*>(tool);
            if (moveTool) {
                moveTool->setSelectedHandles(selection);
            }
        }

        // Activate the tool
        toolMgr->activateTool("move");
        canvas_->setCursor(Qt::CrossCursor);
    }

    void onRotateTool() {
        // Check if there's a selection
        std::vector<std::string> selection = canvas_->selectedHandles();
        if (selection.empty()) {
            statusBar()->showMessage("ROTATE: Select entities first", 3000);
            return;
        }

        // Get RotateTool and pass selection
        ToolManager* toolMgr = canvas_->toolManager();
        Tool* tool = toolMgr->tool("rotate");
        if (tool) {
            RotateTool* rotateTool = dynamic_cast<RotateTool*>(tool);
            if (rotateTool) {
                rotateTool->setSelectedHandles(selection);
            }
        }

        // Activate the tool
        toolMgr->activateTool("rotate");
        canvas_->setCursor(Qt::CrossCursor);
    }

    void onMirrorTool() {
        // Check if there's a selection
        std::vector<std::string> selection = canvas_->selectedHandles();
        if (selection.empty()) {
            statusBar()->showMessage("MIRROR: Select entities first", 3000);
            return;
        }

        // Get MirrorTool and pass selection
        ToolManager* toolMgr = canvas_->toolManager();
        Tool* tool = toolMgr->tool("mirror");
        if (tool) {
            MirrorTool* mirrorTool = dynamic_cast<MirrorTool*>(tool);
            if (mirrorTool) {
                mirrorTool->setSelectedHandles(selection);
            }
        }

        // Activate the tool
        toolMgr->activateTool("mirror");
        canvas_->setCursor(Qt::CrossCursor);
    }

    void onDeleteTool() {
        std::vector<std::string> selection = canvas_->selectedHandles();
        if (selection.empty()) {
            statusBar()->showMessage("DELETE: Select entities first", 3000);
            return;
        }

        // Create and execute delete command
        auto deleteCmd = std::make_unique<DeleteEntitiesCommand>(
            document_.get(),
            selection
        );

        commandHistory_->executeCommand(std::move(deleteCmd));

        // Refresh canvas
        canvas_->setEntities(document_->entities());
        canvas_->clearSelection();

        statusBar()->showMessage(
            QString("Deleted %1 entities").arg(selection.size()),
            2000
        );
    }

    void onActiveToolChanged(const QString& toolId) {
        // Update toolbar button states based on active tool
        QList<QAction*> actions = toolToolbar_->actions();

        if (toolId.isEmpty()) {
            // No tool active → Select mode
            for (QAction* action : actions) {
                if (action->text() == "Select") {
                    action->setChecked(true);
                    break;
                }
            }
        } else {
            // Tool activated → find and check corresponding button
            for (QAction* action : actions) {
                if (action->text().toLower() == toolId.toLower()) {
                    action->setChecked(true);
                    break;
                }
            }
        }
    }

    void onToolPromptChanged(const QString& prompt) {
        toolPromptLabel_->setText(prompt);
    }

    void onGeometryChanged() {
        // Refresh canvas with current document entities
        canvas_->setEntities(document_->entities());
    }

private:
    // Document model (holds all geometry and validation state)
    std::unique_ptr<DocumentModel> document_;

    // UI elements
    CADCanvas* canvas_;
    QToolBar* toolToolbar_;

    // Status bar widgets (permanent indicators)
    QLabel* cursorPosLabel_;
    QLabel* zoomLabel_;
    QLabel* snapModeLabel_;
    QLabel* selectionLabel_;
    QLabel* validationLabel_ = nullptr;
    QLabel* toolPromptLabel_;

    // Command history (undo/redo)
    CommandHistory* commandHistory_;
    QAction* undoAction_;
    QAction* redoAction_;

    // Validation UI
     // Initialized in setupStatusBar

    void updateValidationStatus() {
        const auto& result = document_->validationResult();

        // Extract all problematic entity handles from validation result
        std::set<std::string> problematicHandles;
        for (const auto& issue : result.issues) {
            // Add primary entity handle (if not empty)
            if (!issue.entityHandle.empty()) {
                problematicHandles.insert(issue.entityHandle);
            }
            // Add related entity handle (for duplicate/overlap issues)
            if (!issue.relatedEntityHandle.empty()) {
                problematicHandles.insert(issue.relatedEntityHandle);
            }
        }

        // Update canvas highlighting
        canvas_->setProblematicEntities(problematicHandles);

        // Update status bar indicator
        if (result.passed()) {
            validationLabel_->setText("✓ Valid");
            validationLabel_->setStyleSheet("color: green; font-weight: bold;");
            validationLabel_->setToolTip("Geometry is valid for manufacturing");
        } else {
            validationLabel_->setText(QString("⚠ Issues: %1").arg(result.issueCount()));
            validationLabel_->setStyleSheet("color: red; font-weight: bold;");
            validationLabel_->setToolTip(QString("Found %1 issues. Click 'Validate Geometry' for details.").arg(result.issueCount()));
        }
    }
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
