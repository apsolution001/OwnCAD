#include <QtTest/QtTest>
#include "geometry/GeometryMath.h"
#include "geometry/Line2D.h"
#include "geometry/Arc2D.h"
#include "geometry/Point2D.h"
#include "geometry/GeometryConstants.h"

using namespace OwnCAD::Geometry;

class TestIntersections : public QObject {
    Q_OBJECT

private slots:
    void testLineArcIntersection();
    void testArcArcIntersection();
};

void TestIntersections::testLineArcIntersection() {
    // Arc: Center(0,0), Radius 10, Full Circle 0-360
    auto arc = Arc2D::create(Point2D(0, 0), 10.0, 0, TWO_PI, true);
    QVERIFY(arc.has_value());
    
    // Line: Horizontal through y=0 (intersects at -10,0 and 10,0)
    auto line1 = Line2D::create(Point2D(-20, 0), Point2D(20, 0));
    QVERIFY(line1.has_value());
    
    auto points1 = GeometryMath::intersectLineArc(*line1, *arc);
    QCOMPARE(points1.size(), 2);
    // Sort logic not guaranteed, so check containment
    bool foundLeft = false;
    bool foundRight = false;
    for (const auto& p : points1) {
        if (p.isEqual(Point2D(-10, 0), GEOMETRY_EPSILON)) foundLeft = true;
        if (p.isEqual(Point2D(10, 0), GEOMETRY_EPSILON)) foundRight = true;
    }
    QVERIFY(foundLeft);
    QVERIFY(foundRight);
    
    // Line: Tangent at (0, 10)
    auto line2 = Line2D::create(Point2D(-10, 10), Point2D(10, 10));
    QVERIFY(line2.has_value());
    
    auto points2 = GeometryMath::intersectLineArc(*line2, *arc);
    QCOMPARE(points2.size(), 1);
    QVERIFY(points2[0].isEqual(Point2D(0, 10), GEOMETRY_EPSILON));
    
    // Line: Outside
    auto line3 = Line2D::create(Point2D(-10, 20), Point2D(10, 20));
    QVERIFY(line3.has_value());
    
    auto points3 = GeometryMath::intersectLineArc(*line3, *arc);
    QVERIFY(points3.empty());
    
    // Line: Intersects circle logic, but misses angular sweep
    // Arc: Semi-circle right half (0 to PI, CCW false -> actually 0 to -PI is right half... wait.
    // 0 to PI CCW is TOP half. 0 to PI CW is BOTTOM half.
    // Let's use 0 to PI CCW (Top Half: 1st/2nd quad).
    auto arcSemi = Arc2D::create(Point2D(0, 0), 10.0, 0, PI, true);
    QVERIFY(arcSemi.has_value());
    
    // Line: Horizontal at y=-5 (intersects bottom half circle, but arc doesn't exist there)
    auto line4 = Line2D::create(Point2D(-20, -5), Point2D(20, -5));
    QVERIFY(line4.has_value());
    
    auto points4 = GeometryMath::intersectLineArc(*line4, *arcSemi);
    QVERIFY(points4.empty());
}

void TestIntersections::testArcArcIntersection() {
    // Arc 1: Center(0,0), Radius 10
    auto arc1 = Arc2D::create(Point2D(0, 0), 10.0, 0, TWO_PI, true);
    
    // Arc 2: Center(20,0), Radius 10 (Tangent at 10,0)
    auto arc2 = Arc2D::create(Point2D(20, 0), 10.0, 0, TWO_PI, true);
    
    auto points1 = GeometryMath::intersectArcArc(*arc1, *arc2);
    QCOMPARE(points1.size(), 1);
    QVERIFY(points1[0].isEqual(Point2D(10, 0), GEOMETRY_EPSILON));
    
    // Arc 3: Center(10,0), Radius 10 (Intersects at 5, +/-8.66)
    auto arc3 = Arc2D::create(Point2D(10, 0), 10.0, 0, TWO_PI, true);
    
    auto points2 = GeometryMath::intersectArcArc(*arc1, *arc3);
    QCOMPARE(points2.size(), 2);
    // Calculated: x=5. y = sqrt(100 - 25) = sqrt(75) ~= 8.66025
    Point2D expected1(5, std::sqrt(75.0));
    Point2D expected2(5, -std::sqrt(75.0));
    
    bool foundEq1 = false;
    bool foundEq2 = false;
    for (const auto& p : points2) {
        if (p.isEqual(expected1, GEOMETRY_EPSILON)) foundEq1 = true;
        if (p.isEqual(expected2, GEOMETRY_EPSILON)) foundEq2 = true;
    }
    QVERIFY(foundEq1);
    QVERIFY(foundEq2);
    
    // Arc 4: Disjoint
    auto arc4 = Arc2D::create(Point2D(50, 0), 10.0, 0, TWO_PI, true);
    auto points3 = GeometryMath::intersectArcArc(*arc1, *arc4);
    QVERIFY(points3.empty());
}

QTEST_MAIN(TestIntersections)
#include "test_Intersections.moc"
