#include <QtTest/QtTest>
#include "geometry/Line2D.h"
#include "geometry/GeometryConstants.h"

using namespace OwnCAD::Geometry;

class TestLine2D : public QObject {
    Q_OBJECT

private slots:
    void testValidCreation();
    void testInvalidCreation();
    void testLength();
    void testBoundingBox();
    void testEquality();
    void testPointAt();
    void testAngle();
};

void TestLine2D::testValidCreation() {
    Point2D p1(0, 0);
    Point2D p2(10, 10);

    auto line = Line2D::create(p1, p2);
    QVERIFY(line.has_value());
    QVERIFY(line->isValid());
    QCOMPARE(line->start(), p1);
    QCOMPARE(line->end(), p2);
}

void TestLine2D::testInvalidCreation() {
    Point2D p1(0, 0);
    Point2D p2(0, 0);  // Same point - zero length

    auto line = Line2D::create(p1, p2);
    QVERIFY(!line.has_value());  // Should return nullopt

    // Nearly coincident points (within tolerance)
    Point2D p3(0, GEOMETRY_EPSILON * 0.5);
    auto line2 = Line2D::create(p1, p3);
    QVERIFY(!line2.has_value());
}

void TestLine2D::testLength() {
    Point2D p1(0, 0);
    Point2D p2(3, 4);

    auto line = Line2D::create(p1, p2);
    QVERIFY(line.has_value());
    QCOMPARE(line->length(), 5.0);  // 3-4-5 triangle
}

void TestLine2D::testBoundingBox() {
    Point2D p1(5, 10);
    Point2D p2(15, 20);

    auto line = Line2D::create(p1, p2);
    QVERIFY(line.has_value());

    const BoundingBox& bbox = line->boundingBox();
    QCOMPARE(bbox.minX(), 5.0);
    QCOMPARE(bbox.minY(), 10.0);
    QCOMPARE(bbox.maxX(), 15.0);
    QCOMPARE(bbox.maxY(), 20.0);
}

void TestLine2D::testEquality() {
    Point2D p1(0, 0);
    Point2D p2(10, 10);
    Point2D p3(10, 10.0 + GEOMETRY_EPSILON * 0.5);

    auto line1 = Line2D::create(p1, p2);
    auto line2 = Line2D::create(p1, p2);
    auto line3 = Line2D::create(p1, p3);

    QVERIFY(line1.has_value() && line2.has_value() && line3.has_value());
    QVERIFY(line1->isEqual(*line2, GEOMETRY_EPSILON));
    QVERIFY(line1->isEqual(*line3, GEOMETRY_EPSILON));
}

void TestLine2D::testPointAt() {
    Point2D p1(0, 0);
    Point2D p2(10, 0);

    auto line = Line2D::create(p1, p2);
    QVERIFY(line.has_value());

    Point2D mid = line->pointAt(0.5);
    QCOMPARE(mid.x(), 5.0);
    QCOMPARE(mid.y(), 0.0);

    Point2D start = line->pointAt(0.0);
    QVERIFY(start.isEqual(p1, GEOMETRY_EPSILON));

    Point2D end = line->pointAt(1.0);
    QVERIFY(end.isEqual(p2, GEOMETRY_EPSILON));
}

void TestLine2D::testAngle() {
    Point2D p1(0, 0);
    Point2D p2(10, 0);

    auto line = Line2D::create(p1, p2);
    QVERIFY(line.has_value());

    double angle = line->angle();
    QVERIFY(std::abs(angle - 0.0) < GEOMETRY_EPSILON);  // Horizontal line, 0 degrees

    // Vertical line (90 degrees)
    auto line2 = Line2D::create(p1, Point2D(0, 10));
    QVERIFY(line2.has_value());
    double angle2 = line2->angle();
    QVERIFY(std::abs(angle2 - HALF_PI) < GEOMETRY_EPSILON);
}

QTEST_MAIN(TestLine2D)
#include "test_Line2D.moc"
