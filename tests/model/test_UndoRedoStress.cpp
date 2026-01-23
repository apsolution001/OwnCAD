#include <QtTest>
#include "model/DocumentModel.h"
#include "model/CommandHistory.h"
#include "model/EntityCommands.h"
#include "geometry/Line2D.h"
#include "geometry/Arc2D.h"
#include "geometry/Point2D.h"
#include "geometry/GeometryMath.h"
#include <random>
#include <chrono>

using namespace OwnCAD::Model;
using namespace OwnCAD::Geometry;
using namespace OwnCAD::Import; // For GeometryEntity

class TestUndoRedoStress : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}
    
    void init() {
        m_model = new DocumentModel();
        m_history = new CommandHistory();
        // Use steady_clock for better seeding
        auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        m_rng.seed(seed);
        qDebug() << "Random seed:" << seed;
    }

    void cleanup() {
        delete m_history;
        delete m_model;
        m_history = nullptr;
        m_model = nullptr;
    }

    // Volume tests
    void test100UndoRedoCycles() {
        // Create 100 lines
        for (int i = 0; i < 100; ++i) {
            auto line = Line2D::create(Point2D(i, 0), Point2D(i, 100));
            QVERIFY(line.has_value());

            auto cmd = std::make_unique<CreateEntityCommand>(
                m_model,
                GeometryEntity{*line},
                "0"
            );
            QVERIFY(m_history->executeCommand(std::move(cmd)));
        }

        QCOMPARE(m_model->entities().size(), size_t(100));
        QCOMPARE(m_history->undoCount(), size_t(100));

        // Undo all 100
        for (int i = 0; i < 100; ++i) {
            QVERIFY2(m_history->undo(),
                     qPrintable(QString("Undo failed at iteration %1").arg(i)));
        }

        QCOMPARE(m_model->entities().size(), size_t(0));
        QCOMPARE(m_history->redoCount(), size_t(100));

        // Redo all 100
        for (int i = 0; i < 100; ++i) {
            QVERIFY2(m_history->redo(),
                     qPrintable(QString("Redo failed at iteration %1").arg(i)));
        }

        QCOMPARE(m_model->entities().size(), size_t(100));
        QCOMPARE(m_history->undoCount(), size_t(100));
    }

    void test500Operations() {
        const int TOTAL_OPS = 500;

        for (int i = 0; i < TOTAL_OPS; ++i) {
            int opType = m_rng() % 4;

            switch (opType) {
                case 0: addRandomLine(); break;
                case 1: addRandomArc(); break;
                case 2:
                    if (m_model->entities().size() > 0) {
                        moveRandomEntity(10.0, 10.0);
                    } else {
                        addRandomLine();
                    }
                    break;
                case 3:
                    if (m_model->entities().size() > 5) {
                        deleteRandomEntity();
                    } else {
                        addRandomLine();
                    }
                    break;
            }
        }

        // Record state
        size_t finalCount = m_model->entities().size();

        // Undo everything
        while (m_history->canUndo()) {
            QVERIFY(m_history->undo());
        }

        QCOMPARE(m_model->entities().size(), size_t(0));

        // Redo everything
        while (m_history->canRedo()) {
            QVERIFY(m_history->redo());
        }

        // Should be back to original state
        QCOMPARE(m_model->entities().size(), finalCount);
    }

    void testMaxStackSize() {
        // Set small max for testing
        m_history->setMaxHistorySize(50);

        // Add 100 operations
        for (int i = 0; i < 100; ++i) {
            addRandomLine();
        }

        // Should only have 50 in undo stack
        QCOMPARE(m_history->undoCount(), size_t(50));

        // Oldest 50 operations are lost (by design)
        // But newest 50 should undo correctly
        for (int i = 0; i < 50; ++i) {
            QVERIFY(m_history->undo());
        }

        // Can't undo more
        QVERIFY(!m_history->canUndo());

        // Restore default (though m_history destroyed in cleanup)
        m_history->setMaxHistorySize(100);
    }

    // Sequence tests
    void testRandomOperationSequence() {
        // Seed for reproducibility
        m_rng.seed(42);

        const int ITERATIONS = 200;

        for (int i = 0; i < ITERATIONS; ++i) {
            int action = m_rng() % 10;

            if (action < 4) {
                // 40% chance: add entity
                if (m_rng() % 2 == 0) {
                    addRandomLine();
                } else {
                    addRandomArc();
                }
            } else if (action < 6) {
                // 20% chance: undo
                if (m_history->canUndo()) {
                    QVERIFY(m_history->undo());
                }
            } else if (action < 8) {
                // 20% chance: redo
                if (m_history->canRedo()) {
                    QVERIFY(m_history->redo());
                }
            } else if (action < 9) {
                // 10% chance: move
                if (m_model->entities().size() > 0) {
                    double dx = (m_rng() % 100) - 50;
                    double dy = (m_rng() % 100) - 50;
                    moveRandomEntity(dx, dy);
                }
            } else {
                // 10% chance: delete
                if (m_model->entities().size() > 0) {
                    deleteRandomEntity();
                }
            }
        }

        // System should still be consistent (reached here without crash)
        QVERIFY(true);

        // Full undo should work (cleanup to 0)
        while (m_history->canUndo()) {
            QVERIFY(m_history->undo());
        }
        
        QCOMPARE(m_model->entities().size(), size_t(0));
    }

    void testInterleavedUndoRedo() {
        // Create 10 entities
        for (int i = 0; i < 10; ++i) {
            addRandomLine();
        }

        // Interleaved pattern: undo 2, redo 1, undo 2, redo 1...
        for (int i = 0; i < 5; ++i) {
            QVERIFY(m_history->undo());
            QVERIFY(m_history->undo());
            QVERIFY(m_history->redo());
        }

        // Net result: 10 - (2*5) + (1*5) = 10 - 10 + 5 = 5
        // Wait, undo 2, redo 1 => net -1 per loop
        // 10 - 5 = 5
        QCOMPARE(m_model->entities().size(), size_t(5));
    }

    void testUndoAllRedoAll() {
        // Mixed operations
        addRandomLine();
        addRandomArc();
        addRandomLine();
        moveRandomEntity(50, 50);
        addRandomArc();

        // Capture final state snapshots (count)
        size_t finalCount = m_model->entities().size();

        // Undo all
        while (m_history->canUndo()) {
            QVERIFY(m_history->undo());
        }
        QCOMPARE(m_model->entities().size(), size_t(0));

        // Redo all
        while (m_history->canRedo()) {
            QVERIFY(m_history->redo());
        }

        // Compare to final state
        QCOMPARE(m_model->entities().size(), finalCount);
    }

    // Memory tests
    void testMemoryStability() {
        // This test requires manual memory observation or tools like Valgrind
        // Here we do a basic allocation pattern test

        const int CYCLES = 20; // Reduced for unit test speed

        for (int cycle = 0; cycle < CYCLES; ++cycle) {
            // Add 20 entities
            for (int i = 0; i < 20; ++i) {
                addRandomLine();
            }

            // Undo all
            while (m_history->canUndo()) {
                m_history->undo();
            }

            // Redo all
            while (m_history->canRedo()) {
                m_history->redo();
            }

            // Clear everything
            m_history->clear();
            m_model->clear();
        }

        // If we got here without crash or hang, basic stability is OK
        QVERIFY(true);
    }

    void testNoLeaksAfterClear() {
        // Add operations
        for (int i = 0; i < 50; ++i) {
            addRandomLine();
        }

        // Undo some
        for (int i = 0; i < 25; ++i) {
            m_history->undo();
        }

        // Now we have both undo and redo stacks populated
        QCOMPARE(m_history->undoCount(), size_t(25));
        QCOMPARE(m_history->redoCount(), size_t(25));

        // Clear
        m_history->clear();

        QCOMPARE(m_history->undoCount(), size_t(0));
        QCOMPARE(m_history->redoCount(), size_t(0));
    }

    // Integrity tests
    void testGeometryPreservedAfterUndo() {
        // Create specific geometry
        Point2D p1(100.123456789, 200.987654321);
        Point2D p2(300.111111111, 400.222222222);
        auto line = Line2D::create(p1, p2);
        QVERIFY(line.has_value());

        auto cmd = std::make_unique<CreateEntityCommand>(
            m_model, GeometryEntity{*line}, "0"
        );
        m_history->executeCommand(std::move(cmd));

        // Get the created entity
        QCOMPARE(m_model->entities().size(), size_t(1));
        auto original = m_model->entities()[0];
        auto& originalLine = std::get<Line2D>(original.entity);

        // Undo
        m_history->undo();
        QCOMPARE(m_model->entities().size(), size_t(0));

        // Redo
        m_history->redo();
        QCOMPARE(m_model->entities().size(), size_t(1));

        // Compare geometry (handle may differ)
        auto& restoredLine = std::get<Line2D>(m_model->entities()[0].entity);

        // Must be exactly equal (within epsilon)
        QVERIFY(originalLine.start().isEqual(restoredLine.start(), GEOMETRY_EPSILON));
        QVERIFY(originalLine.end().isEqual(restoredLine.end(), GEOMETRY_EPSILON));

        // Full precision comparison
        QCOMPARE(originalLine.start().x(), restoredLine.start().x());
        QCOMPARE(originalLine.start().y(), restoredLine.start().y());
        QCOMPARE(originalLine.end().x(), restoredLine.end().x());
        QCOMPARE(originalLine.end().y(), restoredLine.end().y());
    }

    void testArcDirectionPreserved() {
        // Create CCW arc
        auto ccwArc = Arc2D::create(Point2D(0, 0), 100.0, 0.0, M_PI, true);
        QVERIFY(ccwArc.has_value());
        QVERIFY(ccwArc->isCounterClockwise());

        auto cmd1 = std::make_unique<CreateEntityCommand>(
            m_model, GeometryEntity{*ccwArc}, "0"
        );
        m_history->executeCommand(std::move(cmd1));

        // Create CW arc
        auto cwArc = Arc2D::create(Point2D(200, 0), 50.0, 0.0, M_PI, false);
        QVERIFY(cwArc.has_value());
        QVERIFY(!cwArc->isCounterClockwise());

        auto cmd2 = std::make_unique<CreateEntityCommand>(
            m_model, GeometryEntity{*cwArc}, "0"
        );
        m_history->executeCommand(std::move(cmd2));

        // Undo both
        m_history->undo();
        m_history->undo();

        // Redo both
        m_history->redo();
        m_history->redo();

        // Check directions preserved
        auto& arc1 = std::get<Arc2D>(m_model->entities()[0].entity);
        auto& arc2 = std::get<Arc2D>(m_model->entities()[1].entity);

        QVERIFY(arc1.isCounterClockwise());   // CCW preserved
        QVERIFY(!arc2.isCounterClockwise());  // CW preserved
    }

    void test360RotationIdentity() {
        // Create a line
        Point2D p1(100.0, 100.0);
        Point2D p2(200.0, 100.0);
        auto line = Line2D::create(p1, p2);
        QVERIFY(line.has_value());

        auto createCmd = std::make_unique<CreateEntityCommand>(
            m_model, GeometryEntity{*line}, "0"
        );
        m_history->executeCommand(std::move(createCmd));

        std::string handle = m_model->entities()[0].handle;

        // Rotate by 360 degrees (full circle)
        Point2D center(150.0, 100.0);
        double fullRotation = 2.0 * M_PI;

        auto rotateCmd = std::make_unique<RotateEntitiesCommand>(
            m_model,
            std::vector<std::string>{handle},
            center,
            fullRotation
        );
        m_history->executeCommand(std::move(rotateCmd));

        // Line should be identical to original (within tolerance)
        auto* entity = m_model->findEntityByHandle(handle);
        QVERIFY(entity != nullptr);

        auto& rotatedLine = std::get<Line2D>(entity->entity);

        // 360° rotation should give identical geometry
        QVERIFY(p1.isEqual(rotatedLine.start(), 1e-6));
        QVERIFY(p2.isEqual(rotatedLine.end(), 1e-6));

        // Undo should also give identical geometry
        m_history->undo();
        entity = m_model->findEntityByHandle(handle);
        auto& undoLine = std::get<Line2D>(entity->entity);

        using OwnCAD::Geometry::GEOMETRY_EPSILON;
        QVERIFY(p1.isEqual(undoLine.start(), GEOMETRY_EPSILON));
        QVERIFY(p2.isEqual(undoLine.end(), GEOMETRY_EPSILON));
    }

    void testMoveUndoExactRestore() {
        // Create line at exact position
        Point2D p1(123.456789012345, 234.567890123456);
        Point2D p2(345.678901234567, 456.789012345678);
        auto line = Line2D::create(p1, p2);
        QVERIFY(line.has_value());

        auto createCmd = std::make_unique<CreateEntityCommand>(
            m_model, GeometryEntity{*line}, "0"
        );
        m_history->executeCommand(std::move(createCmd));

        std::string handle = m_model->entities()[0].handle;

        // Move by arbitrary amount
        double dx = 17.123456789;
        double dy = -23.987654321;

        auto moveCmd = std::make_unique<MoveEntitiesCommand>(
            m_model,
            std::vector<std::string>{handle},
            dx, dy
        );
        m_history->executeCommand(std::move(moveCmd));

        // Undo
        m_history->undo();

        // Should be exactly at original position
        auto* entity = m_model->findEntityByHandle(handle);
        auto& restoredLine = std::get<Line2D>(entity->entity);

        // Exact comparison (no accumulated error)
        QCOMPARE(restoredLine.start().x(), p1.x());
        QCOMPARE(restoredLine.start().y(), p1.y());
        QCOMPARE(restoredLine.end().x(), p2.x());
        QCOMPARE(restoredLine.end().y(), p2.y());
    }

    // Edge case tests
    void testUndoOnEmptyStack() {
        QVERIFY(!m_history->canUndo());
        QVERIFY(!m_history->undo());  // Should return false, not crash

        // Add and remove one operation
        addRandomLine();
        m_history->undo();

        QVERIFY(!m_history->canUndo());
        QVERIFY(!m_history->undo());  // Still safe
    }

    void testRedoOnEmptyStack() {
        QVERIFY(!m_history->canRedo());
        QVERIFY(!m_history->redo());  // Should return false, not crash

        // Add operation (clears redo stack)
        addRandomLine();

        QVERIFY(!m_history->canRedo());
        QVERIFY(!m_history->redo());  // Still safe
    }

    void testRapidUndoRedo() {
        // Simulate rapid key presses
        for (int i = 0; i < 50; ++i) {
            addRandomLine();
        }

        // Rapid alternating undo/redo
        for (int i = 0; i < 100; ++i) {
            if (i % 2 == 0) {
                m_history->undo();
            } else {
                m_history->redo();
            }
        }

        // Should be stable (reached here without crash)
        QVERIFY(true);
    }

    void testUndoRedoAfterClear() {
        addRandomLine();
        addRandomLine();
        m_history->undo();

        // Clear with items in both stacks
        m_history->clear();

        // Should be safe to call undo/redo
        QVERIFY(!m_history->undo());
        QVERIFY(!m_history->redo());

        // New operations should work
        addRandomLine();
        QCOMPARE(m_history->undoCount(), size_t(1));
        QVERIFY(m_history->undo());
    }

private:
    DocumentModel* m_model = nullptr;
    CommandHistory* m_history = nullptr;
    std::mt19937 m_rng;

    // Helpers
    void addRandomLine() {
        double x1 = m_rng() % 1000;
        double y1 = m_rng() % 1000;
        double x2 = x1 + (m_rng() % 100) + 1;
        double y2 = y1 + (m_rng() % 100) + 1;

        auto line = Line2D::create(Point2D(x1, y1), Point2D(x2, y2));
        if (line) {
            auto cmd = std::make_unique<CreateEntityCommand>(
                m_model, GeometryEntity{*line}, "0"
            );
            m_history->executeCommand(std::move(cmd));
        }
    }

    void addRandomArc() {
        double cx = m_rng() % 1000;
        double cy = m_rng() % 1000;
        double radius = (m_rng() % 100) + 10;
        double startAngle = (m_rng() % 360) * M_PI / 180.0;
        double endAngle = startAngle + ((m_rng() % 180) + 30) * M_PI / 180.0;
        bool ccw = m_rng() % 2 == 0;

        auto arc = Arc2D::create(Point2D(cx, cy), radius, startAngle, endAngle, ccw);
        if (arc) {
            auto cmd = std::make_unique<CreateEntityCommand>(
                m_model, GeometryEntity{*arc}, "0"
            );
            m_history->executeCommand(std::move(cmd));
        }
    }

    void moveRandomEntity(double dx, double dy) {
        if (m_model->entities().empty()) return;

        size_t idx = m_rng() % m_model->entities().size();
        std::string handle = m_model->entities()[idx].handle;

        auto cmd = std::make_unique<MoveEntitiesCommand>(
            m_model, std::vector<std::string>{handle}, dx, dy
        );
        m_history->executeCommand(std::move(cmd));
    }

    void deleteRandomEntity() {
        if (m_model->entities().empty()) return;

        size_t idx = m_rng() % m_model->entities().size();
        std::string handle = m_model->entities()[idx].handle;

        auto cmd = std::make_unique<DeleteEntitiesCommand>(
            m_model, std::vector<std::string>{handle}
        );
        m_history->executeCommand(std::move(cmd));
    }
};

QTEST_MAIN(TestUndoRedoStress)
#include "test_UndoRedoStress.moc"
