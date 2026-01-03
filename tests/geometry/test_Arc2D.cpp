#include <QtTest/QtTest>
#include "geometry/Arc2D.h"
#include "geometry/GeometryConstants.h"

using namespace OwnCAD::Geometry;

class TestArc2D : public QObject {
    Q_OBJECT

private slots:
    void testValidCreation();
    void testInvalidCreation();
    void testSweepAngle();
    void testFullCircle();
    void testPoints();
    void testLength();
};

void TestArc2D::testValidCreation() {
    Point2D center(0, 0);
    double radius = 10.0;
    double startAngle = 0.0;
    double endAngle = HALF_PI;

    auto arc = Arc2D::create(center, radius, startAngle, endAngle);
    QVERIFY(arc.has_value());
    QVERIFY(arc->isValid());
    QCOMPARE(arc->radius(), radius);
}

void TestArc2D::testInvalidCreation() {
    Point2D center(0, 0);

    // Zero radius
    auto arc1 = Arc2D::create(center, 0.0, 0.0, HALF_PI);
    QVERIFY(!arc1.has_value());

    // Negative radius
    auto arc2 = Arc2D::create(center, -5.0, 0.0, HALF_PI);
    QVERIFY(!arc2.has_value());

    // Invalid angles
    auto arc3 = Arc2D::create(center, 10.0,
                              std::numeric_limits<double>::quiet_NaN(), HALF_PI);
    QVERIFY(!arc3.has_value());
}

void TestArc2D::testSweepAngle() {
    Point2D center(0, 0);
    double radius = 10.0;

    // Quarter circle CCW
    auto arc1 = Arc2D::create(center, radius, 0.0, HALF_PI, true);
    QVERIFY(arc1.has_value());
    QVERIFY(std::abs(arc1->sweepAngle() - HALF_PI) < GEOMETRY_EPSILON);

    // Quarter circle CW
    auto arc2 = Arc2D::create(center, radius, HALF_PI, 0.0, false);
    QVERIFY(arc2.has_value());
    QVERIFY(std::abs(arc2->sweepAngle() - HALF_PI) < GEOMETRY_EPSILON);

    // Half circle CCW
    auto arc3 = Arc2D::create(center, radius, 0.0, PI, true);
    QVERIFY(arc3.has_value());
    QVERIFY(std::abs(arc3->sweepAngle() - PI) < GEOMETRY_EPSILON);
}

void TestArc2D::testFullCircle() {
    Point2D center(0, 0);
    double radius = 10.0;

    // Full circle (same start and end angle)
    auto arc = Arc2D::create(center, radius, 0.0, TWO_PI, true);
    QVERIFY(arc.has_value());
    QVERIFY(arc->isFullCircle());

    // Near-full circle
    auto arc2 = Arc2D::create(center, radius, 0.0, TWO_PI - 0.1, true);
    QVERIFY(arc2.has_value());
    QVERIFY(!arc2->isFullCircle());
}

void TestArc2D::testPoints() {
    Point2D center(0, 0);
    double radius = 10.0;

    // Quarter circle from 0° to 90°
    auto arc = Arc2D::create(center, radius, 0.0, HALF_PI, true);
    QVERIFY(arc.has_value());

    Point2D start = arc->startPoint();
    QVERIFY(std::abs(start.x() - 10.0) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(start.y() - 0.0) < GEOMETRY_EPSILON);

    Point2D end = arc->endPoint();
    QVERIFY(std::abs(end.x() - 0.0) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(end.y() - 10.0) < GEOMETRY_EPSILON);

    // Midpoint (45°)
    Point2D mid = arc->pointAt(0.5);
    double expected = radius / std::sqrt(2.0);
    QVERIFY(std::abs(mid.x() - expected) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(mid.y() - expected) < GEOMETRY_EPSILON);
}

void TestArc2D::testLength() {
    Point2D center(0, 0);
    double radius = 10.0;

    // Quarter circle: length = (π/2) * r
    auto arc1 = Arc2D::create(center, radius, 0.0, HALF_PI, true);
    QVERIFY(arc1.has_value());
    double expectedLength = HALF_PI * radius;
    QVERIFY(std::abs(arc1->length() - expectedLength) < GEOMETRY_EPSILON);

    // Full circle: length = 2πr
    auto arc2 = Arc2D::create(center, radius, 0.0, TWO_PI, true);
    QVERIFY(arc2.has_value());
    double expectedFullCircle = TWO_PI * radius;
    QVERIFY(std::abs(arc2->length() - expectedFullCircle) < GEOMETRY_EPSILON);
}

QTEST_MAIN(TestArc2D)
#include "test_Arc2D.moc"
