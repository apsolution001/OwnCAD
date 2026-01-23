#include <QtTest/QtTest>
#include "geometry/GeometryValidator.h"
#include "geometry/GeometryMath.h"
#include "geometry/GeometryConstants.h"
#include <cmath>

using namespace OwnCAD::Geometry;

class TestDuplicateDetection : public QObject {
    Q_OBJECT

private slots:
    // =========================================================================
    // ZERO-LENGTH LINE TESTS (existing functionality verification)
    // =========================================================================
    void testZeroLengthLine();
    void testNearZeroLengthLine();
    void testValidLengthLine();

    // =========================================================================
    // ZERO-RADIUS ARC TESTS (existing functionality verification)
    // =========================================================================
    void testZeroRadiusArc();
    void testNearZeroRadiusArc();
    void testValidRadiusArc();

    // =========================================================================
    // DUPLICATE LINE TESTS
    // =========================================================================
    void testIdenticalLines();
    void testIdenticalLinesReversed();
    void testDifferentLines();
    void testNearIdenticalLinesWithinTolerance();
    void testNearIdenticalLinesOutsideTolerance();

    // =========================================================================
    // OVERLAPPING LINE TESTS
    // =========================================================================
    void testOverlappingCollinearLines();
    void testPartiallyOverlappingLines();
    void testTouchingLinesEndToEnd();
    void testParallelNonCollinearLines();
    void testNonOverlappingCollinearLines();

    // =========================================================================
    // DUPLICATE ARC TESTS
    // =========================================================================
    void testIdenticalArcs();
    void testIdenticalArcsOppositeDirection();
    void testDifferentArcs();

    // =========================================================================
    // COINCIDENT ARC TESTS
    // =========================================================================
    void testCoincidentArcsOverlappingAngles();
    void testCoincidentArcsContained();
    void testSameCircleDifferentArcs();
    void testDifferentCenterArcs();
    void testDifferentRadiusArcs();

    // =========================================================================
    // COLLECTION VALIDATION TESTS
    // =========================================================================
    void testValidateEntitiesWithDuplicateLines();
    void testValidateEntitiesWithCoincidentArcs();
    void testValidateEntitiesWithMixedIssues();
    void testValidateEntitiesEmpty();
    void testValidateEntitiesSingleEntity();

    // =========================================================================
    // ENTITY HANDLE TESTS
    // =========================================================================
    void testIssueContainsEntityHandles();

private:
    static constexpr double TOLERANCE = GEOMETRY_EPSILON;
};

// =============================================================================
// ZERO-LENGTH LINE TESTS
// =============================================================================

void TestDuplicateDetection::testZeroLengthLine() {
    // Cannot create zero-length line via factory, but we can test detection
    // with a near-zero line
    auto line = Line2D::create(Point2D(0, 0), Point2D(1e-12, 0));
    // Factory should reject this
    QVERIFY(!line.has_value());
}

void TestDuplicateDetection::testNearZeroLengthLine() {
    // Create a very short but valid line
    auto line = Line2D::create(Point2D(0, 0), Point2D(GEOMETRY_EPSILON * 2, 0));
    if (line.has_value()) {
        bool isZero = GeometryValidator::isZeroLength(*line, GEOMETRY_EPSILON * 10);
        QVERIFY(isZero);
    }
}

void TestDuplicateDetection::testValidLengthLine() {
    auto line = Line2D::create(Point2D(0, 0), Point2D(100, 100));
    QVERIFY(line.has_value());
    QVERIFY(!GeometryValidator::isZeroLength(*line, TOLERANCE));
}

// =============================================================================
// ZERO-RADIUS ARC TESTS
// =============================================================================

void TestDuplicateDetection::testZeroRadiusArc() {
    auto arc = Arc2D::create(Point2D(0, 0), 1e-12, 0, M_PI, true);
    // Factory should reject zero-radius arcs
    QVERIFY(!arc.has_value());
}

void TestDuplicateDetection::testNearZeroRadiusArc() {
    auto arc = Arc2D::create(Point2D(0, 0), GEOMETRY_EPSILON * 2, 0, M_PI, true);
    if (arc.has_value()) {
        bool isZero = GeometryValidator::isZeroRadius(*arc, GEOMETRY_EPSILON * 10);
        QVERIFY(isZero);
    }
}

void TestDuplicateDetection::testValidRadiusArc() {
    auto arc = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, true);
    QVERIFY(arc.has_value());
    QVERIFY(!GeometryValidator::isZeroRadius(*arc, TOLERANCE));
}

// =============================================================================
// DUPLICATE LINE TESTS
// =============================================================================

void TestDuplicateDetection::testIdenticalLines() {
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(100, 100));
    auto line2 = Line2D::create(Point2D(0, 0), Point2D(100, 100));
    QVERIFY(line1.has_value() && line2.has_value());

    QVERIFY(GeometryValidator::areLinesDuplicate(*line1, *line2, TOLERANCE));
}

void TestDuplicateDetection::testIdenticalLinesReversed() {
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(100, 100));
    auto line2 = Line2D::create(Point2D(100, 100), Point2D(0, 0));
    QVERIFY(line1.has_value() && line2.has_value());

    // Reversed lines should still be detected as duplicates
    QVERIFY(GeometryValidator::areLinesDuplicate(*line1, *line2, TOLERANCE));
}

void TestDuplicateDetection::testDifferentLines() {
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(100, 0));
    auto line2 = Line2D::create(Point2D(0, 0), Point2D(0, 100));
    QVERIFY(line1.has_value() && line2.has_value());

    QVERIFY(!GeometryValidator::areLinesDuplicate(*line1, *line2, TOLERANCE));
}

void TestDuplicateDetection::testNearIdenticalLinesWithinTolerance() {
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(100, 100));
    auto line2 = Line2D::create(
        Point2D(TOLERANCE * 0.1, TOLERANCE * 0.1),
        Point2D(100 + TOLERANCE * 0.1, 100 + TOLERANCE * 0.1)
    );
    QVERIFY(line1.has_value() && line2.has_value());

    QVERIFY(GeometryValidator::areLinesDuplicate(*line1, *line2, TOLERANCE));
}

void TestDuplicateDetection::testNearIdenticalLinesOutsideTolerance() {
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(100, 100));
    auto line2 = Line2D::create(Point2D(1.0, 0), Point2D(101, 100));
    QVERIFY(line1.has_value() && line2.has_value());

    QVERIFY(!GeometryValidator::areLinesDuplicate(*line1, *line2, TOLERANCE));
}

// =============================================================================
// OVERLAPPING LINE TESTS
// =============================================================================

void TestDuplicateDetection::testOverlappingCollinearLines() {
    // Line from (0,0) to (100,0) and (50,0) to (150,0)
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(100, 0));
    auto line2 = Line2D::create(Point2D(50, 0), Point2D(150, 0));
    QVERIFY(line1.has_value() && line2.has_value());

    QVERIFY(GeometryValidator::areLinesOverlapping(*line1, *line2, TOLERANCE));
}

void TestDuplicateDetection::testPartiallyOverlappingLines() {
    // Diagonal lines with overlap
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(100, 100));
    auto line2 = Line2D::create(Point2D(50, 50), Point2D(150, 150));
    QVERIFY(line1.has_value() && line2.has_value());

    QVERIFY(GeometryValidator::areLinesOverlapping(*line1, *line2, TOLERANCE));
}

void TestDuplicateDetection::testTouchingLinesEndToEnd() {
    // Lines that share only an endpoint should NOT be considered overlapping
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(100, 0));
    auto line2 = Line2D::create(Point2D(100, 0), Point2D(200, 0));
    QVERIFY(line1.has_value() && line2.has_value());

    // Touching at endpoint only - not overlapping (no interior points shared)
    QVERIFY(!GeometryValidator::areLinesOverlapping(*line1, *line2, TOLERANCE));
}

void TestDuplicateDetection::testParallelNonCollinearLines() {
    // Parallel lines that are not on the same infinite line
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(100, 0));
    auto line2 = Line2D::create(Point2D(0, 10), Point2D(100, 10));
    QVERIFY(line1.has_value() && line2.has_value());

    QVERIFY(!GeometryValidator::areLinesOverlapping(*line1, *line2, TOLERANCE));
}

void TestDuplicateDetection::testNonOverlappingCollinearLines() {
    // Collinear lines with a gap between them
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(50, 0));
    auto line2 = Line2D::create(Point2D(100, 0), Point2D(150, 0));
    QVERIFY(line1.has_value() && line2.has_value());

    QVERIFY(!GeometryValidator::areLinesOverlapping(*line1, *line2, TOLERANCE));
}

// =============================================================================
// DUPLICATE ARC TESTS
// =============================================================================

void TestDuplicateDetection::testIdenticalArcs() {
    auto arc1 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, true);
    auto arc2 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, true);
    QVERIFY(arc1.has_value() && arc2.has_value());

    QVERIFY(GeometryValidator::areArcsDuplicate(*arc1, *arc2, TOLERANCE));
}

void TestDuplicateDetection::testIdenticalArcsOppositeDirection() {
    auto arc1 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, true);   // CCW
    auto arc2 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, false);  // CW
    QVERIFY(arc1.has_value() && arc2.has_value());

    // Different direction means not duplicate
    QVERIFY(!GeometryValidator::areArcsDuplicate(*arc1, *arc2, TOLERANCE));
}

void TestDuplicateDetection::testDifferentArcs() {
    auto arc1 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, true);
    auto arc2 = Arc2D::create(Point2D(0, 0), 100.0, M_PI/2, M_PI * 1.5, true);
    QVERIFY(arc1.has_value() && arc2.has_value());

    QVERIFY(!GeometryValidator::areArcsDuplicate(*arc1, *arc2, TOLERANCE));
}

// =============================================================================
// COINCIDENT ARC TESTS
// =============================================================================

void TestDuplicateDetection::testCoincidentArcsOverlappingAngles() {
    // Same circle, overlapping angular ranges
    auto arc1 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, true);
    auto arc2 = Arc2D::create(Point2D(0, 0), 100.0, M_PI/2, M_PI * 1.5, true);
    QVERIFY(arc1.has_value() && arc2.has_value());

    // arc1: 0 to PI, arc2: PI/2 to 3PI/2
    // Overlap: PI/2 to PI
    QVERIFY(GeometryValidator::areArcsCoincident(*arc1, *arc2, TOLERANCE));
}

void TestDuplicateDetection::testCoincidentArcsContained() {
    // One arc entirely contained within another
    auto arc1 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, true);
    auto arc2 = Arc2D::create(Point2D(0, 0), 100.0, M_PI/4, M_PI/2, true);
    QVERIFY(arc1.has_value() && arc2.has_value());

    QVERIFY(GeometryValidator::areArcsCoincident(*arc1, *arc2, TOLERANCE));
}

void TestDuplicateDetection::testSameCircleDifferentArcs() {
    // Same circle but non-overlapping angular ranges
    auto arc1 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI/4, true);
    auto arc2 = Arc2D::create(Point2D(0, 0), 100.0, M_PI, M_PI * 1.5, true);
    QVERIFY(arc1.has_value() && arc2.has_value());

    // arc1: 0 to PI/4, arc2: PI to 3PI/2
    // No overlap
    QVERIFY(!GeometryValidator::areArcsCoincident(*arc1, *arc2, TOLERANCE));
}

void TestDuplicateDetection::testDifferentCenterArcs() {
    auto arc1 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, true);
    auto arc2 = Arc2D::create(Point2D(50, 0), 100.0, 0, M_PI, true);
    QVERIFY(arc1.has_value() && arc2.has_value());

    QVERIFY(!GeometryValidator::areArcsCoincident(*arc1, *arc2, TOLERANCE));
}

void TestDuplicateDetection::testDifferentRadiusArcs() {
    auto arc1 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, true);
    auto arc2 = Arc2D::create(Point2D(0, 0), 50.0, 0, M_PI, true);
    QVERIFY(arc1.has_value() && arc2.has_value());

    QVERIFY(!GeometryValidator::areArcsCoincident(*arc1, *arc2, TOLERANCE));
}

// =============================================================================
// COLLECTION VALIDATION TESTS
// =============================================================================

void TestDuplicateDetection::testValidateEntitiesWithDuplicateLines() {
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(100, 100));
    auto line2 = Line2D::create(Point2D(0, 0), Point2D(100, 100));  // Duplicate
    auto line3 = Line2D::create(Point2D(200, 0), Point2D(300, 100));
    QVERIFY(line1.has_value() && line2.has_value() && line3.has_value());

    std::vector<std::variant<Line2D, Arc2D>> entities = {*line1, *line2, *line3};
    std::vector<std::string> handles = {"H1", "H2", "H3"};

    auto result = GeometryValidator::validateEntitiesWithHandles(entities, handles, TOLERANCE);

    QVERIFY(!result.isValid);
    QVERIFY(result.hasIssueType(GeometryIssueType::DuplicateLine));
}

void TestDuplicateDetection::testValidateEntitiesWithCoincidentArcs() {
    auto arc1 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, true);
    auto arc2 = Arc2D::create(Point2D(0, 0), 100.0, M_PI/2, M_PI * 1.5, true);
    QVERIFY(arc1.has_value() && arc2.has_value());

    std::vector<std::variant<Line2D, Arc2D>> entities = {*arc1, *arc2};
    std::vector<std::string> handles = {"A1", "A2"};

    auto result = GeometryValidator::validateEntitiesWithHandles(entities, handles, TOLERANCE);

    QVERIFY(!result.isValid);
    QVERIFY(result.hasIssueType(GeometryIssueType::CoincidentArcs));
}

void TestDuplicateDetection::testValidateEntitiesWithMixedIssues() {
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(100, 0));
    auto line2 = Line2D::create(Point2D(50, 0), Point2D(150, 0));  // Overlapping
    auto arc1 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, true);
    auto arc2 = Arc2D::create(Point2D(0, 0), 100.0, 0, M_PI, true);  // Duplicate
    QVERIFY(line1.has_value() && line2.has_value());
    QVERIFY(arc1.has_value() && arc2.has_value());

    std::vector<std::variant<Line2D, Arc2D>> entities = {*line1, *line2, *arc1, *arc2};
    std::vector<std::string> handles = {"L1", "L2", "A1", "A2"};

    auto result = GeometryValidator::validateEntitiesWithHandles(entities, handles, TOLERANCE);

    QVERIFY(!result.isValid);
    QVERIFY(result.hasIssueType(GeometryIssueType::OverlappingLines));
    QVERIFY(result.hasIssueType(GeometryIssueType::DuplicateArc));
}

void TestDuplicateDetection::testValidateEntitiesEmpty() {
    std::vector<std::variant<Line2D, Arc2D>> entities;
    std::vector<std::string> handles;

    auto result = GeometryValidator::validateEntitiesWithHandles(entities, handles, TOLERANCE);

    QVERIFY(result.isValid);
    QVERIFY(result.passed());
    QVERIFY(result.issueCount() == 0);
}

void TestDuplicateDetection::testValidateEntitiesSingleEntity() {
    auto line = Line2D::create(Point2D(0, 0), Point2D(100, 100));
    QVERIFY(line.has_value());

    std::vector<std::variant<Line2D, Arc2D>> entities = {*line};
    std::vector<std::string> handles = {"SINGLE"};

    auto result = GeometryValidator::validateEntitiesWithHandles(entities, handles, TOLERANCE);

    // Single entity can't have duplicates
    QVERIFY(result.isValid);
    QVERIFY(!result.hasIssueType(GeometryIssueType::DuplicateLine));
}

// =============================================================================
// ENTITY HANDLE TESTS
// =============================================================================

void TestDuplicateDetection::testIssueContainsEntityHandles() {
    auto line1 = Line2D::create(Point2D(0, 0), Point2D(100, 100));
    auto line2 = Line2D::create(Point2D(0, 0), Point2D(100, 100));  // Duplicate
    QVERIFY(line1.has_value() && line2.has_value());

    std::vector<std::variant<Line2D, Arc2D>> entities = {*line1, *line2};
    std::vector<std::string> handles = {"HANDLE_001", "HANDLE_002"};

    auto result = GeometryValidator::validateEntitiesWithHandles(entities, handles, TOLERANCE);

    QVERIFY(!result.isValid);
    QVERIFY(result.issueCount() == 1);

    const auto& issue = result.issues[0];
    QVERIFY(issue.type == GeometryIssueType::DuplicateLine);
    QVERIFY(issue.entityHandle == "HANDLE_001");
    QVERIFY(issue.relatedEntityHandle == "HANDLE_002");
    QVERIFY(issue.entityIndex == 0);
    QVERIFY(issue.relatedEntityIndex == 1);
}

QTEST_MAIN(TestDuplicateDetection)
#include "test_DuplicateDetection.moc"
