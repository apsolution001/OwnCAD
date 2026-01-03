#include <QtTest/QtTest>
#include "geometry/Point2D.h"
#include "geometry/GeometryConstants.h"
#include <limits>

using namespace OwnCAD::Geometry;

class TestPoint2D : public QObject {
    Q_OBJECT

private slots:
    // Construction tests
    void testDefaultConstruction();
    void testValidConstruction();
    void testInvalidConstruction();

    // Equality tests
    void testExactEquality();
    void testToleranceEquality();

    // Distance tests
    void testDistance();
    void testDistanceSquared();

    // Validation tests
    void testValidation();
};

// ============================================================================
// CONSTRUCTION TESTS
// ============================================================================

void TestPoint2D::testDefaultConstruction() {
    Point2D p;
    QCOMPARE(p.x(), 0.0);
    QCOMPARE(p.y(), 0.0);
    QVERIFY(p.isValid());
}

void TestPoint2D::testValidConstruction() {
    Point2D p1(10.0, 20.0);
    QCOMPARE(p1.x(), 10.0);
    QCOMPARE(p1.y(), 20.0);
    QVERIFY(p1.isValid());

    Point2D p2(-5.5, 3.14159);
    QCOMPARE(p2.x(), -5.5);
    QCOMPARE(p2.y(), 3.14159);
    QVERIFY(p2.isValid());
}

void TestPoint2D::testInvalidConstruction() {
    // NaN should throw
    QVERIFY_EXCEPTION_THROWN(
        Point2D(std::numeric_limits<double>::quiet_NaN(), 0.0),
        std::invalid_argument
    );

    QVERIFY_EXCEPTION_THROWN(
        Point2D(0.0, std::numeric_limits<double>::quiet_NaN()),
        std::invalid_argument
    );

    // Infinity should throw
    QVERIFY_EXCEPTION_THROWN(
        Point2D(std::numeric_limits<double>::infinity(), 0.0),
        std::invalid_argument
    );

    QVERIFY_EXCEPTION_THROWN(
        Point2D(0.0, -std::numeric_limits<double>::infinity()),
        std::invalid_argument
    );
}

// ============================================================================
// EQUALITY TESTS
// ============================================================================

void TestPoint2D::testExactEquality() {
    Point2D p1(10.0, 20.0);
    Point2D p2(10.0, 20.0);
    Point2D p3(10.1, 20.0);

    QVERIFY(p1 == p2);
    QVERIFY(!(p1 == p3));
    QVERIFY(p1 != p3);
}

void TestPoint2D::testToleranceEquality() {
    Point2D p1(10.0, 20.0);
    Point2D p2(10.0 + GEOMETRY_EPSILON * 0.5, 20.0);
    Point2D p3(10.0 + GEOMETRY_EPSILON * 2.0, 20.0);

    QVERIFY(p1.isEqual(p2, GEOMETRY_EPSILON));
    QVERIFY(!p1.isEqual(p3, GEOMETRY_EPSILON));
}

// ============================================================================
// DISTANCE TESTS
// ============================================================================

void TestPoint2D::testDistance() {
    Point2D p1(0.0, 0.0);
    Point2D p2(3.0, 4.0);

    QCOMPARE(p1.distanceTo(p2), 5.0);  // 3-4-5 triangle
    QCOMPARE(p2.distanceTo(p1), 5.0);  // Symmetric

    Point2D p3(10.0, 10.0);
    QCOMPARE(p3.distanceTo(p3), 0.0);  // Distance to self
}

void TestPoint2D::testDistanceSquared() {
    Point2D p1(0.0, 0.0);
    Point2D p2(3.0, 4.0);

    QCOMPARE(p1.distanceSquaredTo(p2), 25.0);  // 3^2 + 4^2
    QCOMPARE(p2.distanceSquaredTo(p1), 25.0);  // Symmetric
}

// ============================================================================
// VALIDATION TESTS
// ============================================================================

void TestPoint2D::testValidation() {
    Point2D valid(100.0, -200.0);
    QVERIFY(valid.isValid());

    QVERIFY(Point2D::isValid(0.0, 0.0));
    QVERIFY(Point2D::isValid(1e10, -1e10));
    QVERIFY(!Point2D::isValid(std::numeric_limits<double>::quiet_NaN(), 0.0));
    QVERIFY(!Point2D::isValid(0.0, std::numeric_limits<double>::infinity()));
}

QTEST_MAIN(TestPoint2D)
#include "test_Point2D.moc"
