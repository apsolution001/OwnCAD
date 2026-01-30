#include <QtTest/QtTest>
#include "model/DocumentModel.h"
#include "model/EntityCommands.h"
#include "geometry/Line2D.h"
#include "geometry/Point2D.h"
#include "geometry/GeometryConstants.h"

using namespace OwnCAD::Model;
using namespace OwnCAD::Geometry;
using namespace OwnCAD::Import;

class TestMetadataPreservation : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testHexHandleGeneration();
    void testHandleConflictAvoidance();
    void testEntityOrderPreservation_SingleDelete();
    void testEntityOrderPreservation_BatchDelete();

private:
    DocumentModel* m_model = nullptr;
};

void TestMetadataPreservation::init() {
    m_model = new DocumentModel();
}

void TestMetadataPreservation::cleanup() {
    delete m_model;
    m_model = nullptr;
}

void TestMetadataPreservation::testHexHandleGeneration() {
    std::string h1 = m_model->addLine(*Line2D::create(Point2D(0, 0), Point2D(10, 0)), "0");
    std::string h2 = m_model->addLine(*Line2D::create(Point2D(10, 0), Point2D(20, 0)), "0");

    // Should be hex strings (e.g. "0", "1", "2" or starting from some base)
    // By default nextHandleNumber_ starts at 1
    QCOMPARE(h1, std::string("1"));
    QCOMPARE(h2, std::string("2"));

    // Skip ahead
    for(int i=0; i<13; ++i) m_model->generateHandle(); // 3 to 15 (0xF)
    
    std::string h16 = m_model->generateHandle();
    QCOMPARE(h16, std::string("10")); // 16 in hex
}

void TestMetadataPreservation::testHandleConflictAvoidance() {
    // Simulate loading a DXF with high handles
    std::vector<DXFEntity> entities;
    
    DXFLine line;
    line.startX = 0; line.startY = 0; line.endX = 10; line.endY = 10;
    line.handle = "FF"; // 255
    line.layer = "0";
    
    DXFEntity e;
    e.type = DXFEntityType::Line;
    e.data = line;
    entities.push_back(e);
    
    // Internal conversion
    ConversionResult res = GeometryConverter::convert(entities);
    m_model->restoreEntity(res.entities[0]);
    m_model->updateNextHandleNumber();
    
    // New handle should be greater than FF (255) -> 100 (256)
    std::string nextH = m_model->generateHandle();
    QCOMPARE(nextH, std::string("100"));
}

void TestMetadataPreservation::testEntityOrderPreservation_SingleDelete() {
    std::string h1 = m_model->addLine(*Line2D::create(Point2D(0, 0), Point2D(10, 0)), "0");
    std::string h2 = m_model->addLine(*Line2D::create(Point2D(10, 0), Point2D(20, 0)), "0");
    std::string h3 = m_model->addLine(*Line2D::create(Point2D(20, 0), Point2D(30, 0)), "0");

    QCOMPARE(m_model->entities().size(), static_cast<size_t>(3));
    QCOMPARE(m_model->entities()[1].handle, h2);

    // Delete middle entity
    DeleteEntityCommand cmd(m_model, h2);
    QVERIFY(cmd.execute());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(2));
    QCOMPARE(m_model->entities()[1].handle, h3);

    // Undo delete
    QVERIFY(cmd.undo());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(3));
    
    // Verify it's back in the MIDDLE (index 1)
    QCOMPARE(m_model->entities()[1].handle, h2);
}

void TestMetadataPreservation::testEntityOrderPreservation_BatchDelete() {
    std::string h1 = m_model->addLine(*Line2D::create(Point2D(0, 0), Point2D(10, 0)), "0");
    std::string h2 = m_model->addLine(*Line2D::create(Point2D(10, 0), Point2D(20, 0)), "0");
    std::string h3 = m_model->addLine(*Line2D::create(Point2D(20, 0), Point2D(30, 0)), "0");
    std::string h4 = m_model->addLine(*Line2D::create(Point2D(30, 0), Point2D(40, 0)), "0");

    // Delete 1 and 3 (index 0 and 2)
    DeleteEntitiesCommand cmd(m_model, {h1, h3});
    QVERIFY(cmd.execute());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(2));
    QCOMPARE(m_model->entities()[0].handle, h2);
    QCOMPARE(m_model->entities()[1].handle, h4);

    // Undo batch delete
    QVERIFY(cmd.undo());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(4));
    
    // Verify exact order restoration
    QCOMPARE(m_model->entities()[0].handle, h1);
    QCOMPARE(m_model->entities()[1].handle, h2);
    QCOMPARE(m_model->entities()[2].handle, h3);
    QCOMPARE(m_model->entities()[3].handle, h4);
}

QTEST_MAIN(TestMetadataPreservation)
#include "test_MetadataPreservation.moc"
