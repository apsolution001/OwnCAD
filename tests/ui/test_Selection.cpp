#include <QtTest/QtTest>
#include "ui/SelectionManager.h"
#include "ui/CADCanvas.h"
#include "geometry/Line2D.h"
#include "geometry/Point2D.h"
#include <vector>

using namespace OwnCAD;

class TestSelection : public QObject {
    Q_OBJECT

private slots:
    void testSelectionManager();
    void testHitTest();
};

void TestSelection::testSelectionManager() {
    UI::SelectionManager manager;
    
    QVERIFY(manager.isEmpty());
    QCOMPARE(manager.selectedCount(), 0);
    
    std::string handle1 = "H1";
    std::string handle2 = "H2";
    
    manager.select(handle1);
    QVERIFY(manager.isSelected(handle1));
    QCOMPARE(manager.selectedCount(), 1);
    
    manager.select(handle2);
    QVERIFY(manager.isSelected(handle2));
    QCOMPARE(manager.selectedCount(), 2);
    
    manager.deselect(handle1);
    QVERIFY(!manager.isSelected(handle1));
    QCOMPARE(manager.selectedCount(), 1);
    
    manager.clear();
    QVERIFY(manager.isEmpty());
}

// Mock subclass to access protected/private members for testing
class TestableCanvas : public UI::CADCanvas {
public:
    using UI::CADCanvas::hitTest;
    using UI::CADCanvas::setEntities;
};

void TestSelection::testHitTest() {
    TestableCanvas canvas;
    
    // Setup entities
    std::vector<Import::GeometryEntityWithMetadata> entities;
    
    Geometry::Line2D line = *Geometry::Line2D::create(Geometry::Point2D(0, 0), Geometry::Point2D(100, 0));
    Import::GeometryEntityWithMetadata meta1;
    meta1.entity = line;
    meta1.handle = "LINE1";
    meta1.layer = "0";
    entities.push_back(meta1);
    
    canvas.setEntities(entities);
    
    // Zoom is 1.0 by default, so world tolerance is 10.0 units
    
    // Hit directly on line
    std::string hit = canvas.hitTest(Geometry::Point2D(50, 0));
    QCOMPARE(hit, std::string("LINE1"));
    
    // Hit near line (within tolerance)
    hit = canvas.hitTest(Geometry::Point2D(50, 5));
    QCOMPARE(hit, std::string("LINE1"));
    
    // Miss line (outside tolerance)
    hit = canvas.hitTest(Geometry::Point2D(50, 15));
    QCOMPARE(hit, std::string(""));
}

QTEST_MAIN(TestSelection)
#include "test_Selection.moc"
