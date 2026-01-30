# Task 7: UI STRUCTURE - Implementation Status & Guide

## Current Status Analysis

### 7.1 Main Layout
- ✅ **Fixed menu bar** (File, Edit, View, Draw, Tools, Help) - `main.cpp:171-249`
- ❌ **Left vertical toolbar** - NOT IMPLEMENTED
- ✅ **Central CADCanvas widget** - `main.cpp:95-124`
- ✅ **Status bar** (comprehensive) - `main.cpp:252-303`

**Status: 75% Complete**

### 7.2 Toolbar Icons
- ❌ Select tool - NOT IMPLEMENTED
- ❌ Line tool - NOT IMPLEMENTED
- ❌ Arc tool - NOT IMPLEMENTED
- ❌ Rectangle tool - NOT IMPLEMENTED
- ❌ Move tool - NOT IMPLEMENTED
- ❌ Rotate tool - NOT IMPLEMENTED
- ❌ Mirror tool - NOT IMPLEMENTED
- ❌ Delete tool - NOT IMPLEMENTED
- ❌ Tool tips on hover - NOT IMPLEMENTED

**Status: 0% Complete**

**Note:** All tools exist and work via menu/keyboard shortcuts. Only toolbar UI is missing.

### 7.3 Status Bar Elements
- ❌ **Units display** (mm/cm/inches) - NOT IMPLEMENTED
- ✅ **Snap mode indicator** - `main.cpp:264-268` (snapModeLabel_)
- ❌ **Grid on/off indicator** - NOT IMPLEMENTED
- ✅ **Cursor coordinates** (world space) - `main.cpp:295-299` (cursorPosLabel_)
- ✅ **Zoom level** - `main.cpp:288-292` (zoomLabel_)
- ❌ **Entity count** - NOT IMPLEMENTED (selection count exists, but not total entity count)
- ✅ **Validation warning indicator** - `main.cpp:279-285` (validationLabel_)

**Status: 57% Complete**

---

## Priority: Subtask 7.2 (Toolbar Icons)

This is the largest missing piece. The toolbar provides:
- Visual discoverability of tools
- One-click tool access (vs menu navigation)
- Industry-standard CAD UI pattern

---

## Implementation Guide: Subtask 7.2 - Toolbar Icons

### Overview
Create a **left vertical toolbar** with icons for all drawing and editing tools.

### Design Decisions

#### 1. Toolbar Position
- **Left side** (vertical orientation)
- Standard in CAD applications (AutoCAD, QCAD, LibreCAD)
- Tools flow top-to-bottom in logical order

#### 2. Icon Style
- **Text-based for now** (Qt supports text-only buttons)
- Later phase: replace with proper icons/SVGs
- Each button shows tool name (e.g., "Line", "Arc", "Move")

#### 3. Toolbar Organization
**Selection Group:**
- Select (default cursor)

**Drawing Group:**
- Line
- Arc
- Rectangle

**Transformation Group:**
- Move
- Rotate
- Mirror

**Edit Group:**
- Delete

#### 4. Visual Feedback
- Active tool: highlighted button (different background color)
- Hover: tooltip with keyboard shortcut
- Disabled state: grayed out when selection required (Move/Rotate/Mirror)

---

### Implementation Steps

#### Step 1: Create Toolbar Widget (`main.cpp`)

**Location:** In `MainWindow` class, after `createMenus()`

**Add Member Variable:**
```cpp
// In MainWindow class (private section)
QToolBar* toolToolbar_;
```

**Create Method:**
```cpp
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
```

#### Step 2: Create Toolbar Actions

**Method:**
```cpp
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
```

#### Step 3: Implement Delete Tool Handler

**New Slot Method:**
```cpp
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

    commandHistory_->execute(std::move(deleteCmd));

    // Refresh canvas
    canvas_->setEntities(document_->entities());
    canvas_->clearSelection();

    statusBar()->showMessage(
        QString("Deleted %1 entities").arg(selection.size()),
        2000
    );
}
```

#### Step 4: Wire Up in Constructor

**In `MainWindow::MainWindow()` constructor:**
```cpp
// After setupStatusBar()
createToolbar();  // Add this line
```

#### Step 5: Connect Tool Manager to Toolbar

**Problem:** When a tool is activated via menu/keyboard, toolbar should update.

**Solution:** Use existing `activeToolChanged(QString)` signal from ToolManager.

**Add to `setupCentralWidget()`:**
```cpp
// Connect tool activation to toolbar state
connect(toolMgr, &ToolManager::activeToolChanged,
        this, &MainWindow::onActiveToolChanged);
```

**New Slot Method:**
```cpp
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
```

---

### Files to Modify

**Only 1 file needs changes:**

1. **`src/main.cpp`**
   - Add `toolToolbar_` member variable (line ~61, with other UI pointers)
   - Add `createToolbar()` method
   - Add `createToolbarActions()` method
   - Add `onDeleteTool()` slot
   - Add `onActiveToolChanged(QString)` slot
   - Call `createToolbar()` in constructor (after `setupStatusBar()`)

**No changes needed to ToolManager** - `activeToolChanged(QString)` signal already exists!

---

### Testing Checklist

After implementation, verify:

- [ ] Toolbar appears on left side
- [ ] All 8 tools visible (Select, Line, Arc, Rectangle, Move, Rotate, Mirror, Delete)
- [ ] Click Select → cursor changes to arrow, can select entities
- [ ] Click Line → activates line tool, button highlighted
- [ ] Click Arc → activates arc tool
- [ ] Move/Rotate/Mirror buttons work with selection
- [ ] Delete button removes selected entities (with undo)
- [ ] Tooltips show on hover
- [ ] ESC deactivates tool → Select button becomes active
- [ ] Menu activation updates toolbar button state
- [ ] Keyboard shortcuts update toolbar button state
- [ ] Only one tool button active at a time

---

### Future Enhancements (Post-Phase 1)

1. **Icon Graphics:**
   - Replace text with SVG icons
   - Consistent 32x32px icon set
   - Light/dark theme support

2. **Tool Groups:**
   - Expandable/collapsible groups
   - Custom toolbar configuration

3. **Quick Access:**
   - Recently used tools
   - Favorites/pinning

---

## Other Missing Subtasks (Lower Priority)

### 7.3.1: Units Display in Status Bar

**What:** Show current document units (mm/cm/in)

**Implementation:**
```cpp
// Add to setupStatusBar():
unitsLabel_ = new QLabel("Units: mm");
unitsLabel_->setMinimumWidth(80);
unitsLabel_->setAlignment(Qt::AlignCenter);
unitsLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
statusBar()->addPermanentWidget(unitsLabel_);
```

**Integration:** Read from GridSettings.units

---

### 7.3.2: Grid On/Off Indicator in Status Bar

**What:** Visual indicator showing grid visibility

**Implementation:**
```cpp
// Add to setupStatusBar():
gridLabel_ = new QLabel("Grid: ON");
gridLabel_->setMinimumWidth(80);
gridLabel_->setAlignment(Qt::AlignCenter);
gridLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
statusBar()->addPermanentWidget(gridLabel_);

// Update in onToggleGrid():
gridLabel_->setText(visible ? "Grid: ON" : "Grid: OFF");
```

---

### 7.3.3: Entity Count in Status Bar

**What:** Show total entity count (not just selected)

**Implementation:**
```cpp
// Add to setupStatusBar():
entityCountLabel_ = new QLabel("Entities: 0");
entityCountLabel_->setMinimumWidth(100);
entityCountLabel_->setAlignment(Qt::AlignCenter);
entityCountLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
statusBar()->addPermanentWidget(entityCountLabel_);

// Update when document changes:
void updateEntityCount() {
    size_t count = document_->entityCount();
    entityCountLabel_->setText(QString("Entities: %1").arg(count));
}
```

---

## Recommended Implementation Order

1. **7.2 Toolbar Icons** (Highest Impact) ← START HERE
2. **7.3.3 Entity Count** (Low effort, useful)
3. **7.3.2 Grid Indicator** (Low effort)
4. **7.3.1 Units Display** (Low effort)

---

## Summary

**Subtask 7.2 (Toolbar Icons) is the critical missing piece.**

- Current workaround: Menu + keyboard shortcuts (functional but not user-friendly)
- Toolbar provides: Visual discoverability, one-click access, industry-standard UX
- Estimated effort: ~2-3 hours (implementation + testing)
- No new logic required: all tools already implemented, just need UI wiring

**Ready to implement when approved.**
