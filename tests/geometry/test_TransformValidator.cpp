#include <QtTest/QtTest>
#include "geometry/TransformValidator.h"
#include "geometry/GeometryMath.h"
#include "geometry/GeometryConstants.h"
#include <cmath>

using namespace OwnCAD::Geometry;

class TestTransformValidator : public QObject {
    Q_OBJECT

private slots:
    // Point comparison tests
    void testComparePointsIdentical();
    void testComparePointsWithinTolerance();
    void testComparePointsOutsideTolerance();

    // Line comparison tests
    void testCompareLinesIdentical();
    void testValidateLineLengthPreserved();

    // Arc comparison tests
    void testCompareArcsIdentical();
    void testValidateArcDirectionPreserved();
    void testValidateArcDirectionChanged();
    void testValidateArcRadiusPreserved();
    void testValidateArcSweepPreserved();

    // Ellipse comparison tests
    void testCompareEllipsesIdentical();
    void testValidateEllipseAxesPreserved();

    // Round-trip validation tests
    void testPointTranslateRoundTrip();
    void testPointRotateRoundTrip();

    // Cumulative drift tests
    void testCumulativeDriftSmallRotations();
    void testCumulativeDriftLargeIterations();

    // 360-degree rotation tests
    void testRotate360Point();
    void testRotate360Line();
    void testRotate360Arc();
    void testRotate360ArcPreservesDirection();

    // Composed transform tests
    void testTranslateRotateRoundTrip();
    void testLineTransformPreservesLength();
    void testArcTransformPreservesRadiusSweep();
};

// =============================================================================
// POINT COMPARISON TESTS
// =============================================================================

void TestTransformValidator::testComparePointsIdentical() {
    Point2D p(100.123456789, 200.987654321);
    auto result = TransformValidator::comparePoints(p, p);

    QVERIFY(result.passed);
    QCOMPARE(result.maxDeviation, 0.0);
    QVERIFY(result.failureReason.empty());
}

void TestTransformValidator::testComparePointsWithinTolerance() {
    Point2D p1(100.0, 200.0);
    Point2D p2(100.0 + GEOMETRY_EPSILON * 0.5, 200.0 + GEOMETRY_EPSILON * 0.5);

    auto result = TransformValidator::comparePoints(p1, p2);
    QVERIFY(result.passed);
}

void TestTransformValidator::testComparePointsOutsideTolerance() {
    Point2D p1(100.0, 200.0);
    Point2D p2(100.1, 200.0);  // 0.1 deviation

    auto result = TransformValidator::comparePoints(p1, p2);
    QVERIFY(!result.passed);
    QVERIFY(result.maxDeviation >= 0.1);
    QVERIFY(!result.failureReason.empty());
}

// =============================================================================
// LINE COMPARISON TESTS
// =============================================================================

void TestTransformValidator::testCompareLinesIdentical() {
    auto line = Line2D::create(Point2D(0, 0), Point2D(100, 100));
    QVERIFY(line.has_value());

    auto result = TransformValidator::compareLines(*line, *line);
    QVERIFY(result.passed);
}

void TestTransformValidator::testValidateLineLengthPreserved() {
    auto original = Line2D::create(Point2D(0, 0), Point2D(100, 0));
    QVERIFY(original.has_value());

    auto translated = GeometryMath::translate(*original, 1000.0, 2000.0);
    QVERIFY(translated.has_value());

    auto result = TransformValidator::validateLineLength(*original, *translated);
    QVERIFY(result.passed);
    QVERIFY(result.maxDeviation < GEOMETRY_EPSILON);
}

// =============================================================================
// ARC COMPARISON TESTS
// =============================================================================

void TestTransformValidator::testCompareArcsIdentical() {
    auto arc = Arc2D::create(Point2D(0, 0), 50.0, 0.0, PI, true);
    QVERIFY(arc.has_value());

    auto result = TransformValidator::compareArcs(*arc, *arc);
    QVERIFY(result.passed);
}

void TestTransformValidator::testValidateArcDirectionPreserved() {
    auto ccwArc = Arc2D::create(Point2D(0, 0), 50.0, 0.0, PI, true);  // CCW
    auto cwArc = Arc2D::create(Point2D(0, 0), 50.0, 0.0, PI, false);  // CW
    QVERIFY(ccwArc.has_value());
    QVERIFY(cwArc.has_value());

    // Rotate CCW arc
    auto rotatedCcw = GeometryMath::rotate(*ccwArc, Point2D(100, 100), PI / 4);
    QVERIFY(rotatedCcw.has_value());

    auto result = TransformValidator::validateArcDirection(*ccwArc, *rotatedCcw);
    QVERIFY2(result.passed, "CCW arc direction not preserved after rotation");

    // Rotate CW arc
    auto rotatedCw = GeometryMath::rotate(*cwArc, Point2D(100, 100), PI / 4);
    QVERIFY(rotatedCw.has_value());

    result = TransformValidator::validateArcDirection(*cwArc, *rotatedCw);
    QVERIFY2(result.passed, "CW arc direction not preserved after rotation");
}

void TestTransformValidator::testValidateArcDirectionChanged() {
    auto ccwArc = Arc2D::create(Point2D(0, 0), 50.0, 0.0, PI, true);   // CCW
    auto cwArc = Arc2D::create(Point2D(0, 0), 50.0, 0.0, PI, false);   // CW
    QVERIFY(ccwArc.has_value());
    QVERIFY(cwArc.has_value());

    auto result = TransformValidator::validateArcDirection(*ccwArc, *cwArc);
    QVERIFY(!result.passed);
    QVERIFY(result.failureReason.find("direction changed") != std::string::npos);
}

void TestTransformValidator::testValidateArcRadiusPreserved() {
    auto arc = Arc2D::create(Point2D(0, 0), 75.5, 0.0, PI, true);
    QVERIFY(arc.has_value());

    auto translated = GeometryMath::translate(*arc, 999.999, -888.888);
    QVERIFY(translated.has_value());

    auto result = TransformValidator::validateArcRadius(*arc, *translated);
    QVERIFY(result.passed);

    auto rotated = GeometryMath::rotate(*arc, Point2D(500, 500), PI / 3);
    QVERIFY(rotated.has_value());

    result = TransformValidator::validateArcRadius(*arc, *rotated);
    QVERIFY(result.passed);
}

void TestTransformValidator::testValidateArcSweepPreserved() {
    auto arc = Arc2D::create(Point2D(0, 0), 50.0, 0.0, PI, true);
    QVERIFY(arc.has_value());
    double originalSweep = arc->sweepAngle();

    auto rotated = GeometryMath::rotate(*arc, Point2D(100, 100), PI / 6);
    QVERIFY(rotated.has_value());

    auto result = TransformValidator::validateArcSweep(*arc, *rotated);
    QVERIFY(result.passed);
    QVERIFY(std::abs(originalSweep - rotated->sweepAngle()) < GEOMETRY_EPSILON);
}

// =============================================================================
// ELLIPSE COMPARISON TESTS
// =============================================================================

void TestTransformValidator::testCompareEllipsesIdentical() {
    auto ellipse = Ellipse2D::create(
        Point2D(0, 0),
        Point2D(100, 0),
        0.5,
        0.0, TWO_PI
    );
    QVERIFY(ellipse.has_value());

    auto result = TransformValidator::compareEllipses(*ellipse, *ellipse);
    QVERIFY(result.passed);
}

void TestTransformValidator::testValidateEllipseAxesPreserved() {
    auto ellipse = Ellipse2D::create(
        Point2D(0, 0),
        Point2D(100, 0),
        0.5,
        0.0, TWO_PI
    );
    QVERIFY(ellipse.has_value());

    auto rotated = GeometryMath::rotate(*ellipse, Point2D(50, 50), PI / 4);
    QVERIFY(rotated.has_value());

    auto result = TransformValidator::validateEllipseAxes(*ellipse, *rotated);
    QVERIFY(result.passed);
}

// =============================================================================
// ROUND-TRIP VALIDATION TESTS
// =============================================================================

void TestTransformValidator::testPointTranslateRoundTrip() {
    Point2D p(100.123456789, 200.987654321);

    auto result = TransformValidator::validatePointRoundTrip(
        p,
        [](const Point2D& pt) { return GeometryMath::translate(pt, 50.0, -30.0); },
        [](const Point2D& pt) { return GeometryMath::translate(pt, -50.0, 30.0); }
    );

    QVERIFY2(result.passed, result.failureReason.c_str());
}

void TestTransformValidator::testPointRotateRoundTrip() {
    Point2D p(100.0, 50.0);
    Point2D center(0.0, 0.0);

    auto result = TransformValidator::validatePointRoundTrip(
        p,
        [&center](const Point2D& pt) { return GeometryMath::rotate(pt, center, PI / 3); },
        [&center](const Point2D& pt) { return GeometryMath::rotate(pt, center, -PI / 3); }
    );

    QVERIFY2(result.passed, result.failureReason.c_str());
}

// =============================================================================
// CUMULATIVE DRIFT TESTS
// =============================================================================

void TestTransformValidator::testCumulativeDriftSmallRotations() {
    Point2D p(100.0, 0.0);
    Point2D center(0.0, 0.0);
    double smallAngle = TWO_PI / 100.0;  // 3.6 degrees

    // 100 rotations of 3.6 degrees = 360 degrees = back to start
    auto result = TransformValidator::validateCumulativeDrift(
        p,
        [&center, smallAngle](const Point2D& pt) {
            return GeometryMath::rotate(pt, center, smallAngle);
        },
        100,
        p,
        0.001  // 1 micron tolerance
    );

    QVERIFY2(result.passed, result.failureReason.c_str());
}

void TestTransformValidator::testCumulativeDriftLargeIterations() {
    Point2D p(100.0, 0.0);
    Point2D center(0.0, 0.0);
    double smallAngle = TWO_PI / 1000.0;  // 0.36 degrees

    // 1000 rotations of 0.36 degrees = 360 degrees = back to start
    auto result = TransformValidator::validateCumulativeDrift(
        p,
        [&center, smallAngle](const Point2D& pt) {
            return GeometryMath::rotate(pt, center, smallAngle);
        },
        1000,
        p,
        0.001  // 1 micron tolerance (manufacturing standard)
    );

    QVERIFY2(result.passed, result.failureReason.c_str());
}

// =============================================================================
// 360-DEGREE ROTATION TESTS
// =============================================================================

void TestTransformValidator::testRotate360Point() {
    Point2D p(100.0, 50.0);
    Point2D center(0.0, 0.0);

    auto result = TransformValidator::validate360Rotation(p, center);
    QVERIFY2(result.passed, result.failureReason.c_str());
}

void TestTransformValidator::testRotate360Line() {
    auto line = Line2D::create(Point2D(10, 20), Point2D(50, 80));
    QVERIFY(line.has_value());

    Point2D center(0.0, 0.0);
    auto result = TransformValidator::validate360Rotation(*line, center);
    QVERIFY2(result.passed, result.failureReason.c_str());
}

void TestTransformValidator::testRotate360Arc() {
    auto arc = Arc2D::create(Point2D(50, 50), 25.0, 0.0, PI, true);
    QVERIFY(arc.has_value());

    Point2D center(0.0, 0.0);
    auto result = TransformValidator::validate360Rotation(*arc, center);
    QVERIFY2(result.passed, result.failureReason.c_str());
}

void TestTransformValidator::testRotate360ArcPreservesDirection() {
    // Test CCW arc
    auto ccwArc = Arc2D::create(Point2D(50, 50), 25.0, 0.0, PI, true);
    QVERIFY(ccwArc.has_value());

    auto rotatedCcw = GeometryMath::rotate(*ccwArc, Point2D(0, 0), TWO_PI);
    QVERIFY(rotatedCcw.has_value());
    QVERIFY2(rotatedCcw->isCounterClockwise(),
             "360-degree rotation changed CCW arc to CW");

    // Test CW arc
    auto cwArc = Arc2D::create(Point2D(50, 50), 25.0, 0.0, PI, false);
    QVERIFY(cwArc.has_value());

    auto rotatedCw = GeometryMath::rotate(*cwArc, Point2D(0, 0), TWO_PI);
    QVERIFY(rotatedCw.has_value());
    QVERIFY2(!rotatedCw->isCounterClockwise(),
             "360-degree rotation changed CW arc to CCW");
}

// =============================================================================
// COMPOSED TRANSFORM TESTS
// =============================================================================

void TestTransformValidator::testTranslateRotateRoundTrip() {
    auto line = Line2D::create(Point2D(10, 20), Point2D(50, 80));
    QVERIFY(line.has_value());

    // Apply sequence: translate → rotate → rotate back → translate back
    auto step1 = GeometryMath::translate(*line, 100, 200);
    QVERIFY(step1.has_value());

    auto step2 = GeometryMath::rotate(*step1, Point2D(0, 0), PI / 3);
    QVERIFY(step2.has_value());

    auto step3 = GeometryMath::rotate(*step2, Point2D(0, 0), -PI / 3);
    QVERIFY(step3.has_value());

    auto step4 = GeometryMath::translate(*step3, -100, -200);
    QVERIFY(step4.has_value());

    // Verify round-trip with relaxed tolerance (composed operations accumulate error)
    double tolerance = GEOMETRY_EPSILON * 100;  // Allow 100x epsilon for composition
    auto result = TransformValidator::compareLines(*line, *step4, tolerance);
    QVERIFY2(result.passed, result.failureReason.c_str());
}

void TestTransformValidator::testLineTransformPreservesLength() {
    auto line = Line2D::create(Point2D(0, 0), Point2D(100, 0));
    QVERIFY(line.has_value());
    double originalLength = line->length();

    // Translate
    auto translated = GeometryMath::translate(*line, 500, -300);
    QVERIFY(translated.has_value());
    QVERIFY(std::abs(translated->length() - originalLength) < GEOMETRY_EPSILON);

    // Rotate
    auto rotated = GeometryMath::rotate(*line, Point2D(50, 0), PI / 4);
    QVERIFY(rotated.has_value());
    QVERIFY(std::abs(rotated->length() - originalLength) < GEOMETRY_EPSILON);

    // Combined
    auto combined = GeometryMath::rotate(*translated, Point2D(0, 0), PI / 6);
    QVERIFY(combined.has_value());
    QVERIFY(std::abs(combined->length() - originalLength) < GEOMETRY_EPSILON);
}

void TestTransformValidator::testArcTransformPreservesRadiusSweep() {
    auto arc = Arc2D::create(Point2D(0, 0), 50.0, 0.0, PI / 2, true);
    QVERIFY(arc.has_value());

    double originalRadius = arc->radius();
    double originalSweep = arc->sweepAngle();

    // Translate
    auto translated = GeometryMath::translate(*arc, 1000, 2000);
    QVERIFY(translated.has_value());
    QVERIFY(std::abs(translated->radius() - originalRadius) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(translated->sweepAngle() - originalSweep) < GEOMETRY_EPSILON);

    // Rotate
    auto rotated = GeometryMath::rotate(*arc, Point2D(100, 100), PI / 3);
    QVERIFY(rotated.has_value());
    QVERIFY(std::abs(rotated->radius() - originalRadius) < GEOMETRY_EPSILON);
    QVERIFY(std::abs(rotated->sweepAngle() - originalSweep) < GEOMETRY_EPSILON);
}

QTEST_MAIN(TestTransformValidator)
#include "test_TransformValidator.moc"
