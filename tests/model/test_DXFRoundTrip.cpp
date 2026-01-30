#include <QtTest/QtTest>
#include "model/DocumentModel.h"
#include "model/ExportValidator.h"
#include "geometry/Line2D.h"
#include "geometry/Arc2D.h"
#include "geometry/Ellipse2D.h"
#include "geometry/Point2D.h"
#include "geometry/GeometryConstants.h"
#include <QTemporaryFile>

using namespace OwnCAD::Model;
using namespace OwnCAD::Geometry;
using namespace OwnCAD::Import;

class TestDXFRoundTrip : public QObject {
    Q_OBJECT

private slots:
    void testFullRoundTrip();

private:
    bool compareDocuments(const DocumentModel& orig, const DocumentModel& reimp);
};

void TestDXFRoundTrip::testFullRoundTrip() {
    DocumentModel original;
    
    // 1. Populate with diverse entities
    original.addLine(*Line2D::create(Point2D(0, 0), Point2D(100, 100)), "Layer_Lines");
    original.addArc(*Arc2D::create(Point2D(50, 50), 25, 0, 1.5708, true), "Layer_Arcs"); // 90 deg CCW
    original.addArc(*Arc2D::create(Point2D(0, 0), 10, 0, 2.0 * 3.1415926535, true), "Layer_Circles");
    original.addEllipse(*Ellipse2D::create(Point2D(10, 20), Point2D(40, 20), 0.5, 0, 6.283185), "Layer_Ellipses");
    original.addPoint(Point2D(5, 5), "Layer_Points");
    
    QVERIFY(original.entities().size() == 5);

    // 2. Export to temporary file
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString filePath = tempFile.fileName();
    tempFile.close(); // Just need the path

    QVERIFY(original.exportDXFFile(filePath.toStdString()));

    // 3. Re-import
    DocumentModel reimported;
    QVERIFY(reimported.loadDXFFile(filePath.toStdString()));

    // 4. Validate

    // Count match
    QCOMPARE(reimported.entities().size(), original.entities().size());

    // Precision and Geometry
    auto precisionReport = ExportValidator::validatePrecision(original, reimported, 1e-7); // Some float conversion slack
    QVERIFY2(precisionReport.withinTolerance, "Geometry precision lost in round-trip");

    // Layers
    auto layerReport = ExportValidator::validateLayers(original, reimported);
    QVERIFY2(layerReport.matches, "Layers not preserved in round-trip");

    // Bounding Boxes
    QVERIFY2(ExportValidator::validateBoundingBoxes(original, reimported, 1e-7), "Bounding boxes inconsistent after round-trip");
}

QTEST_MAIN(TestDXFRoundTrip)
#include "test_DXFRoundTrip.moc"
