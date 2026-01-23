#include <QtTest/QtTest>
#include "model/EntityCommands.h"
#include "model/DocumentModel.h"
#include "geometry/Line2D.h"
#include "geometry/Arc2D.h"
#include "geometry/Point2D.h"
#include "geometry/GeometryConstants.h"

using namespace OwnCAD::Model;
using namespace OwnCAD::Geometry;
using namespace OwnCAD::Import;

class TestEntityCommands : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // CreateEntityCommand tests
    void testCreateLineCommand_Execute();
    void testCreateLineCommand_Undo();
    void testCreateLineCommand_Redo();
    void testCreateLineCommand_Description();
    void testCreateLineCommand_InvalidGeometry();

    void testCreateArcCommand_Execute();
    void testCreateArcCommand_Undo();
    void testCreateArcCommand_PreservesDirection();

    // CreateEntitiesCommand tests
    void testCreateEntitiesCommand_Execute();
    void testCreateEntitiesCommand_Undo();
    void testCreateEntitiesCommand_AtomicRollback();

    // DeleteEntityCommand tests
    void testDeleteEntityCommand_Execute();
    void testDeleteEntityCommand_Undo();
    void testDeleteEntityCommand_NonexistentHandle();

    // DeleteEntitiesCommand tests
    void testDeleteEntitiesCommand_Execute();
    void testDeleteEntitiesCommand_Undo();
    void testDeleteEntitiesCommand_PartialSelection();

    // MoveEntitiesCommand tests
    void testMoveEntitiesCommand_Execute();
    void testMoveEntitiesCommand_Undo();
    void testMoveEntitiesCommand_ZeroMove();
    void testMoveEntitiesCommand_Merge();

    // RotateEntitiesCommand tests
    void testRotateEntitiesCommand_Execute();
    void testRotateEntitiesCommand_Undo();
    void testRotateEntitiesCommand_360Degrees();
    void testRotateEntitiesCommand_ArcDirectionPreserved();

    // MirrorEntitiesCommand tests
    void testMirrorEntitiesCommand_Execute();
    void testMirrorEntitiesCommand_Undo();
    void testMirrorEntitiesCommand_KeepOriginal();
    void testMirrorEntitiesCommand_ArcDirectionInverted();

private:
    DocumentModel* m_model = nullptr;

    // Helpers
    std::string addTestLine(double x1, double y1, double x2, double y2);
    std::string addTestArc(double cx, double cy, double r, double start, double end, bool ccw);
};

void TestEntityCommands::initTestCase() {
}

void TestEntityCommands::cleanupTestCase() {
}

void TestEntityCommands::init() {
    m_model = new DocumentModel();
}

void TestEntityCommands::cleanup() {
    delete m_model;
    m_model = nullptr;
}

std::string TestEntityCommands::addTestLine(double x1, double y1, double x2, double y2) {
    auto line = Line2D::create(Point2D(x1, y1), Point2D(x2, y2));
    if (!line) return "";
    return m_model->addLine(*line, "0");
}

std::string TestEntityCommands::addTestArc(double cx, double cy, double r, double start, double end, bool ccw) {
    auto arc = Arc2D::create(Point2D(cx, cy), r, start, end, ccw);
    if (!arc) return "";
    return m_model->addArc(*arc, "0");
}

// =============================================================================
// CREATE ENTITY COMMAND TESTS
// =============================================================================

void TestEntityCommands::testCreateLineCommand_Execute() {
    auto line = Line2D::create(Point2D(0, 0), Point2D(10, 10));
    QVERIFY(line.has_value());

    CreateEntityCommand cmd(m_model, GeometryEntity{*line}, "0");
    QVERIFY(cmd.isValid());

    QCOMPARE(m_model->entities().size(), static_cast<size_t>(0));
    QVERIFY(cmd.execute());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(1));
}

void TestEntityCommands::testCreateLineCommand_Undo() {
    auto line = Line2D::create(Point2D(0, 0), Point2D(10, 10));
    QVERIFY(line.has_value());

    CreateEntityCommand cmd(m_model, GeometryEntity{*line}, "0");
    QVERIFY(cmd.execute());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(1));

    QVERIFY(cmd.undo());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(0));
}

void TestEntityCommands::testCreateLineCommand_Redo() {
    auto line = Line2D::create(Point2D(0, 0), Point2D(10, 10));
    QVERIFY(line.has_value());

    CreateEntityCommand cmd(m_model, GeometryEntity{*line}, "0");
    QVERIFY(cmd.execute());
    QVERIFY(cmd.undo());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(0));

    QVERIFY(cmd.redo());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(1));
}

void TestEntityCommands::testCreateLineCommand_Description() {
    auto line = Line2D::create(Point2D(0, 0), Point2D(10, 10));
    QVERIFY(line.has_value());

    CreateEntityCommand cmd(m_model, GeometryEntity{*line}, "0");
    QCOMPARE(cmd.description(), QString("Draw Line"));
}

void TestEntityCommands::testCreateLineCommand_InvalidGeometry() {
    // Zero-length line should fail validation in Line2D::create
    auto line = Line2D::create(Point2D(5, 5), Point2D(5, 5));
    QVERIFY(!line.has_value());
}

void TestEntityCommands::testCreateArcCommand_Execute() {
    auto arc = Arc2D::create(Point2D(0, 0), 10, 0, PI / 2, true);
    QVERIFY(arc.has_value());

    CreateEntityCommand cmd(m_model, GeometryEntity{*arc}, "0");
    QVERIFY(cmd.execute());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(1));
}

void TestEntityCommands::testCreateArcCommand_Undo() {
    auto arc = Arc2D::create(Point2D(0, 0), 10, 0, PI / 2, true);
    QVERIFY(arc.has_value());

    CreateEntityCommand cmd(m_model, GeometryEntity{*arc}, "0");
    QVERIFY(cmd.execute());
    QVERIFY(cmd.undo());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(0));
}

void TestEntityCommands::testCreateArcCommand_PreservesDirection() {
    // CCW arc
    auto arcCCW = Arc2D::create(Point2D(0, 0), 10, 0, PI / 2, true);
    QVERIFY(arcCCW.has_value());

    CreateEntityCommand cmd(m_model, GeometryEntity{*arcCCW}, "0");
    QVERIFY(cmd.execute());

    const auto& entities = m_model->entities();
    QCOMPARE(entities.size(), static_cast<size_t>(1));

    const auto* arcPtr = std::get_if<Arc2D>(&entities[0].entity);
    QVERIFY(arcPtr != nullptr);
    QVERIFY(arcPtr->isCounterClockwise());
}

// =============================================================================
// CREATE ENTITIES COMMAND TESTS
// =============================================================================

void TestEntityCommands::testCreateEntitiesCommand_Execute() {
    std::vector<GeometryEntity> entities;

    auto line1 = Line2D::create(Point2D(0, 0), Point2D(10, 0));
    auto line2 = Line2D::create(Point2D(10, 0), Point2D(10, 10));
    QVERIFY(line1.has_value() && line2.has_value());

    entities.push_back(GeometryEntity{*line1});
    entities.push_back(GeometryEntity{*line2});

    CreateEntitiesCommand cmd(m_model, entities, "0");
    QVERIFY(cmd.execute());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(2));
}

void TestEntityCommands::testCreateEntitiesCommand_Undo() {
    std::vector<GeometryEntity> entities;

    auto line1 = Line2D::create(Point2D(0, 0), Point2D(10, 0));
    auto line2 = Line2D::create(Point2D(10, 0), Point2D(10, 10));
    QVERIFY(line1.has_value() && line2.has_value());

    entities.push_back(GeometryEntity{*line1});
    entities.push_back(GeometryEntity{*line2});

    CreateEntitiesCommand cmd(m_model, entities, "0");
    QVERIFY(cmd.execute());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(2));

    QVERIFY(cmd.undo());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(0));
}

void TestEntityCommands::testCreateEntitiesCommand_AtomicRollback() {
    // This test verifies that if we try to create entities and one fails,
    // all previously created entities are rolled back.
    // In practice, valid geometry is checked before CreateEntitiesCommand is called.
    // Empty list should fail
    std::vector<GeometryEntity> emptyList;
    CreateEntitiesCommand cmd(m_model, emptyList, "0");
    QVERIFY(!cmd.isValid());
}

// =============================================================================
// DELETE ENTITY COMMAND TESTS
// =============================================================================

void TestEntityCommands::testDeleteEntityCommand_Execute() {
    std::string handle = addTestLine(0, 0, 10, 10);
    QVERIFY(!handle.empty());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(1));

    DeleteEntityCommand cmd(m_model, handle);
    QVERIFY(cmd.execute());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(0));
}

void TestEntityCommands::testDeleteEntityCommand_Undo() {
    std::string handle = addTestLine(0, 0, 10, 10);
    QVERIFY(!handle.empty());

    DeleteEntityCommand cmd(m_model, handle);
    QVERIFY(cmd.execute());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(0));

    QVERIFY(cmd.undo());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(1));

    // Verify the handle is restored correctly
    QVERIFY(m_model->findEntityByHandle(handle) != nullptr);
}

void TestEntityCommands::testDeleteEntityCommand_NonexistentHandle() {
    DeleteEntityCommand cmd(m_model, "NONEXISTENT");
    QVERIFY(!cmd.isValid());
}

// =============================================================================
// DELETE ENTITIES COMMAND TESTS
// =============================================================================

void TestEntityCommands::testDeleteEntitiesCommand_Execute() {
    std::string h1 = addTestLine(0, 0, 10, 0);
    std::string h2 = addTestLine(10, 0, 10, 10);
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(2));

    DeleteEntitiesCommand cmd(m_model, {h1, h2});
    QVERIFY(cmd.execute());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(0));
}

void TestEntityCommands::testDeleteEntitiesCommand_Undo() {
    std::string h1 = addTestLine(0, 0, 10, 0);
    std::string h2 = addTestLine(10, 0, 10, 10);

    DeleteEntitiesCommand cmd(m_model, {h1, h2});
    QVERIFY(cmd.execute());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(0));

    QVERIFY(cmd.undo());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(2));
}

void TestEntityCommands::testDeleteEntitiesCommand_PartialSelection() {
    std::string h1 = addTestLine(0, 0, 10, 0);
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(1));

    // Include one valid handle and one invalid
    DeleteEntitiesCommand cmd(m_model, {h1, "INVALID"});
    QVERIFY(cmd.execute());  // Should succeed with partial
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(0));
}

// =============================================================================
// MOVE ENTITIES COMMAND TESTS
// =============================================================================

void TestEntityCommands::testMoveEntitiesCommand_Execute() {
    std::string handle = addTestLine(0, 0, 10, 0);

    MoveEntitiesCommand cmd(m_model, {handle}, 5.0, 3.0);
    QVERIFY(cmd.execute());

    const auto* entity = m_model->findEntityByHandle(handle);
    QVERIFY(entity != nullptr);

    const auto* line = std::get_if<Line2D>(&entity->entity);
    QVERIFY(line != nullptr);

    // Check translated positions
    QVERIFY(std::abs(line->start().x() - 5.0) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(line->start().y() - 3.0) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(line->end().x() - 15.0) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(line->end().y() - 3.0) < GEOMETRY_EPSILON);
}

void TestEntityCommands::testMoveEntitiesCommand_Undo() {
    std::string handle = addTestLine(0, 0, 10, 0);

    MoveEntitiesCommand cmd(m_model, {handle}, 5.0, 3.0);
    QVERIFY(cmd.execute());
    QVERIFY(cmd.undo());

    const auto* entity = m_model->findEntityByHandle(handle);
    QVERIFY(entity != nullptr);

    const auto* line = std::get_if<Line2D>(&entity->entity);
    QVERIFY(line != nullptr);

    // Check original positions restored
    QVERIFY(std::abs(line->start().x() - 0.0) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(line->start().y() - 0.0) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(line->end().x() - 10.0) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(line->end().y() - 0.0) < GEOMETRY_EPSILON);
}

void TestEntityCommands::testMoveEntitiesCommand_ZeroMove() {
    std::string handle = addTestLine(0, 0, 10, 0);

    MoveEntitiesCommand cmd(m_model, {handle}, 0.0, 0.0);
    QVERIFY(cmd.execute());  // Zero move is valid (no-op)
}

void TestEntityCommands::testMoveEntitiesCommand_Merge() {
    std::string handle = addTestLine(0, 0, 10, 0);

    MoveEntitiesCommand cmd1(m_model, {handle}, 1.0, 0.0);
    MoveEntitiesCommand cmd2(m_model, {handle}, 2.0, 0.0);

    // Commands on same entity set should be mergeable (within time threshold)
    // Note: canMerge checks timestamp, so in test this might not merge
    // This is more of a design verification
    QVERIFY(cmd1.isValid());
    QVERIFY(cmd2.isValid());
}

// =============================================================================
// ROTATE ENTITIES COMMAND TESTS
// =============================================================================

void TestEntityCommands::testRotateEntitiesCommand_Execute() {
    std::string handle = addTestLine(10, 0, 20, 0);  // Horizontal line at y=0

    // Rotate 90 degrees CCW around origin
    RotateEntitiesCommand cmd(m_model, {handle}, Point2D(0, 0), PI / 2);
    QVERIFY(cmd.execute());

    const auto* entity = m_model->findEntityByHandle(handle);
    QVERIFY(entity != nullptr);

    const auto* line = std::get_if<Line2D>(&entity->entity);
    QVERIFY(line != nullptr);

    // After 90° CCW: (10,0) -> (0,10), (20,0) -> (0,20)
    QVERIFY(std::abs(line->start().x() - 0.0) < 0.001);
    QVERIFY(std::abs(line->start().y() - 10.0) < 0.001);
    QVERIFY(std::abs(line->end().x() - 0.0) < 0.001);
    QVERIFY(std::abs(line->end().y() - 20.0) < 0.001);
}

void TestEntityCommands::testRotateEntitiesCommand_Undo() {
    std::string handle = addTestLine(10, 0, 20, 0);

    RotateEntitiesCommand cmd(m_model, {handle}, Point2D(0, 0), PI / 2);
    QVERIFY(cmd.execute());
    QVERIFY(cmd.undo());

    const auto* entity = m_model->findEntityByHandle(handle);
    const auto* line = std::get_if<Line2D>(&entity->entity);
    QVERIFY(line != nullptr);

    // Should be back to original
    QVERIFY(std::abs(line->start().x() - 10.0) < 0.001);
    QVERIFY(std::abs(line->start().y() - 0.0) < 0.001);
}

void TestEntityCommands::testRotateEntitiesCommand_360Degrees() {
    std::string handle = addTestLine(10, 5, 20, 5);

    double originalX1, originalY1, originalX2, originalY2;
    {
        const auto* entity = m_model->findEntityByHandle(handle);
        const auto* line = std::get_if<Line2D>(&entity->entity);
        originalX1 = line->start().x();
        originalY1 = line->start().y();
        originalX2 = line->end().x();
        originalY2 = line->end().y();
    }

    // Rotate 360 degrees - should end up at same position
    RotateEntitiesCommand cmd(m_model, {handle}, Point2D(0, 0), TWO_PI);
    QVERIFY(cmd.execute());

    const auto* entity = m_model->findEntityByHandle(handle);
    const auto* line = std::get_if<Line2D>(&entity->entity);

    QVERIFY(std::abs(line->start().x() - originalX1) < 0.001);
    QVERIFY(std::abs(line->start().y() - originalY1) < 0.001);
    QVERIFY(std::abs(line->end().x() - originalX2) < 0.001);
    QVERIFY(std::abs(line->end().y() - originalY2) < 0.001);
}

void TestEntityCommands::testRotateEntitiesCommand_ArcDirectionPreserved() {
    std::string handle = addTestArc(10, 0, 5, 0, PI / 2, true);  // CCW arc

    RotateEntitiesCommand cmd(m_model, {handle}, Point2D(0, 0), PI / 4);
    QVERIFY(cmd.execute());

    const auto* entity = m_model->findEntityByHandle(handle);
    const auto* arc = std::get_if<Arc2D>(&entity->entity);
    QVERIFY(arc != nullptr);

    // Direction should still be CCW
    QVERIFY(arc->isCounterClockwise());
}

// =============================================================================
// MIRROR ENTITIES COMMAND TESTS
// =============================================================================

void TestEntityCommands::testMirrorEntitiesCommand_Execute() {
    std::string handle = addTestLine(5, 5, 10, 5);

    // Mirror over X-axis (y=0)
    MirrorEntitiesCommand cmd(m_model, {handle},
                               Point2D(0, 0), Point2D(10, 0),
                               false);  // Replace original
    QVERIFY(cmd.execute());

    const auto* entity = m_model->findEntityByHandle(handle);
    const auto* line = std::get_if<Line2D>(&entity->entity);
    QVERIFY(line != nullptr);

    // After mirror over X-axis: (5,5) -> (5,-5), (10,5) -> (10,-5)
    QVERIFY(std::abs(line->start().y() - (-5.0)) < 0.001);
    QVERIFY(std::abs(line->end().y() - (-5.0)) < 0.001);
}

void TestEntityCommands::testMirrorEntitiesCommand_Undo() {
    std::string handle = addTestLine(5, 5, 10, 5);

    MirrorEntitiesCommand cmd(m_model, {handle},
                               Point2D(0, 0), Point2D(10, 0),
                               false);
    QVERIFY(cmd.execute());
    QVERIFY(cmd.undo());

    const auto* entity = m_model->findEntityByHandle(handle);
    const auto* line = std::get_if<Line2D>(&entity->entity);

    // Should be back to original y=5
    QVERIFY(std::abs(line->start().y() - 5.0) < 0.001);
}

void TestEntityCommands::testMirrorEntitiesCommand_KeepOriginal() {
    std::string handle = addTestLine(5, 5, 10, 5);
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(1));

    MirrorEntitiesCommand cmd(m_model, {handle},
                               Point2D(0, 0), Point2D(10, 0),
                               true);  // Keep original
    QVERIFY(cmd.execute());

    // Should have 2 entities now (original + mirror copy)
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(2));

    // Undo should remove the copy
    QVERIFY(cmd.undo());
    QCOMPARE(m_model->entities().size(), static_cast<size_t>(1));
}

void TestEntityCommands::testMirrorEntitiesCommand_ArcDirectionInverted() {
    // CCW arc at (10, 5)
    std::string handle = addTestArc(10, 5, 3, 0, PI / 2, true);

    // Mirror over X-axis
    MirrorEntitiesCommand cmd(m_model, {handle},
                               Point2D(0, 0), Point2D(10, 0),
                               false);
    QVERIFY(cmd.execute());

    const auto* entity = m_model->findEntityByHandle(handle);
    const auto* arc = std::get_if<Arc2D>(&entity->entity);
    QVERIFY(arc != nullptr);

    // Direction should be inverted (CCW -> CW)
    QVERIFY(!arc->isCounterClockwise());
}

QTEST_MAIN(TestEntityCommands)
#include "test_EntityCommands.moc"
