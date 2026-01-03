#include <QtTest/QtTest>
#include "geometry/GeometryMath.h"
#include "geometry/GeometryConstants.h"

using namespace OwnCAD::Geometry;
using namespace OwnCAD::Geometry::GeometryMath;

class TestGeometryMath : public QObject {
    Q_OBJECT

private slots:
    void testDistance();
    void testAngleNormalization();
    void testAngleBetweenPoints();
    void testArcCalculations();
    void testToleranceUtilities();
};

void TestGeometryMath::testDistance() {
    Point2D p1(0, 0);
    Point2D p2(3, 4);

    QCOMPARE(distance(p1, p2), 5.0);
    QCOMPARE(distanceSquared(p1, p2), 25.0);
}

void TestGeometryMath::testAngleNormalization() {
    // Positive angles
    QVERIFY(std::abs(normalizeAngle(0.0) - 0.0) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(normalizeAngle(PI) - PI) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(normalizeAngle(TWO_PI) - 0.0) < GEOMETRY_EPSILON);

    // Negative angles
    QVERIFY(std::abs(normalizeAngle(-PI) - PI) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(normalizeAngle(-HALF_PI) - (TWO_PI - HALF_PI)) < GEOMETRY_EPSILON);

    // Angles > 2π
    QVERIFY(std::abs(normalizeAngle(3 * PI) - PI) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(normalizeAngle(4 * PI) - 0.0) < GEOMETRY_EPSILON);

    // Signed normalization [-π, π)
    QVERIFY(std::abs(normalizeAngleSigned(0.0) - 0.0) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(normalizeAngleSigned(PI) - PI) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(normalizeAngleSigned(TWO_PI) - 0.0) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(normalizeAngleSigned(3 * HALF_PI) - (-HALF_PI)) < GEOMETRY_EPSILON);
}

void TestGeometryMath::testAngleBetweenPoints() {
    Point2D origin(0, 0);

    // 0° (right)
    Point2D right(10, 0);
    QVERIFY(std::abs(angleBetweenPoints(origin, right) - 0.0) < GEOMETRY_EPSILON);

    // 90° (up)
    Point2D up(0, 10);
    QVERIFY(std::abs(angleBetweenPoints(origin, up) - HALF_PI) < GEOMETRY_EPSILON);

    // 180° (left)
    Point2D left(-10, 0);
    QVERIFY(std::abs(angleBetweenPoints(origin, left) - PI) < GEOMETRY_EPSILON);

    // 270° (down)
    Point2D down(0, -10);
    double expectedDown = TWO_PI - HALF_PI;  // 3π/2
    QVERIFY(std::abs(angleBetweenPoints(origin, down) - expectedDown) < GEOMETRY_EPSILON);
}

void TestGeometryMath::testArcCalculations() {
    double radius = 10.0;

    // Quarter circle
    double sweep1 = HALF_PI;
    double length1 = arcLength(radius, sweep1);
    QVERIFY(std::abs(length1 - (HALF_PI * radius)) < GEOMETRY_EPSILON);

    // Full circle
    double sweep2 = TWO_PI;
    double length2 = arcLength(radius, sweep2);
    QVERIFY(std::abs(length2 - (TWO_PI * radius)) < GEOMETRY_EPSILON);

    // Sweep angle calculation CCW
    double sweepCCW = sweepAngle(0.0, HALF_PI, true);
    QVERIFY(std::abs(sweepCCW - HALF_PI) < GEOMETRY_EPSILON);

    // Sweep angle calculation CW
    double sweepCW = sweepAngle(HALF_PI, 0.0, false);
    QVERIFY(std::abs(sweepCW - HALF_PI) < GEOMETRY_EPSILON);

    // Sweep across 0° boundary CCW
    double sweepWrap = sweepAngle(3 * HALF_PI, HALF_PI, true);
    QVERIFY(std::abs(sweepWrap - PI) < GEOMETRY_EPSILON);
}

void TestGeometryMath::testToleranceUtilities() {
    // areEqual
    QVERIFY(areEqual(1.0, 1.0 + GEOMETRY_EPSILON * 0.5, GEOMETRY_EPSILON));
    QVERIFY(!areEqual(1.0, 1.0 + GEOMETRY_EPSILON * 2.0, GEOMETRY_EPSILON));

    // isZero
    QVERIFY(isZero(0.0, GEOMETRY_EPSILON));
    QVERIFY(isZero(GEOMETRY_EPSILON * 0.5, GEOMETRY_EPSILON));
    QVERIFY(!isZero(GEOMETRY_EPSILON * 2.0, GEOMETRY_EPSILON));

    // clamp
    QCOMPARE(clamp(5.0, 0.0, 10.0), 5.0);
    QCOMPARE(clamp(-5.0, 0.0, 10.0), 0.0);
    QCOMPARE(clamp(15.0, 0.0, 10.0), 10.0);
}

QTEST_MAIN(TestGeometryMath)
#include "test_GeometryMath.moc"
