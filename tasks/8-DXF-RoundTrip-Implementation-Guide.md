# Task 8: DXF Round-Trip Safety - Implementation Guide

## Status Assessment

### What's Already Done ✅

#### 8.1 Metadata Preservation - PARTIALLY COMPLETE
- ✅ **DXF handle tracking**: `GeometryEntityWithMetadata.handle` (lines 34 in GeometryConverter.h)
- ✅ **Layer preservation**: `GeometryEntityWithMetadata.layer` (line 33)
- ✅ **Color preservation**: `GeometryEntityWithMetadata.colorNumber` (line 35)
- ✅ **Source line numbers**: `GeometryEntityWithMetadata.sourceLineNumber` (line 36)
- ✅ **Entity order**: Implicitly preserved (DocumentModel stores entities in std::vector)

**Note**: Import pipeline already captures all necessary metadata during DXF parsing.

### What's Missing ❌

#### 8.2 Export Validation - NOT IMPLEMENTED
**BLOCKER**: No DXF export functionality exists
- ❌ No `DXFWriter` class
- ❌ No `DXFExporter` class
- ❌ No way to write DXF files
- ❌ No entity-to-DXF conversion

#### 8.3 Re-import Verification - NOT IMPLEMENTED
**BLOCKER**: Depends on 8.2 export functionality
- ❌ Cannot test round-trip (Import → Export → Re-import)
- ❌ No visual diff capability
- ❌ No automated round-trip tests

---

## Root Cause Analysis

**The fundamental issue**: The project has a complete DXF import pipeline, but **zero** export capability.

**Why this matters**:
- Users can load DXF files but cannot save changes back to DXF
- No way to verify data integrity through round-trip testing
- Risk of metadata loss during future edits if not validated now
- Cannot use the application as a DXF editor (read-only mode only)

---

## Implementation Plan

### Overview
This task requires building a complete **DXF export system** from scratch, then implementing validation and round-trip testing.

**Estimated Scope**: Large (3-5 subtasks)
**Complexity**: Medium-High (inverse of import pipeline, requires DXF format knowledge)
**Dependencies**: None (can start immediately)

### Architecture Decision: Symmetry with Import Pipeline

**Principle**: Export should mirror the import architecture for maintainability.

```
IMPORT PIPELINE:
DXF File → DXFParser → DXFEntity → GeometryConverter → GeometryEntityWithMetadata

EXPORT PIPELINE (to build):
GeometryEntityWithMetadata → GeometryExporter → DXFEntity → DXFWriter → DXF File
```

---

## Subtask Breakdown

### Subtask 8.1: Verify Metadata Preservation (Current State)
**Status**: ✅ COMPLETE (verification only needed)
**Effort**: 15 minutes

**Action**:
1. Review `GeometryEntityWithMetadata` structure
2. Confirm all DXF metadata is captured:
   - Handle (entity ID)
   - Layer name
   - Color number
   - Source line number (for debugging)
3. Document any missing metadata fields

**Deliverable**: Confirmation that import preserves all necessary metadata.

---

### Subtask 8.2: Implement DXF Export (Core Infrastructure)
**Status**: ❌ NOT STARTED
**Effort**: 4-6 hours
**Files to Create**:
- `include/export/DXFWriter.h`
- `src/export/DXFWriter.cpp`
- `include/export/GeometryExporter.h`
- `src/export/GeometryExporter.cpp`

#### Step 1: Create `DXFWriter` Class
**Responsibility**: Convert `DXFEntity` structures back to DXF text format.

**Interface Design**:
```cpp
namespace OwnCAD {
namespace Export {

class DXFWriter {
public:
    /**
     * @brief Write DXF entities to file
     * @param filePath Output DXF file path
     * @param entities DXF entities to write
     * @return true if successful
     */
    static bool writeFile(
        const std::string& filePath,
        const std::vector<Import::DXFEntity>& entities
    );

private:
    static void writeHeader(std::ostream& out);
    static void writeTables(std::ostream& out);
    static void writeEntities(std::ostream& out, const std::vector<Import::DXFEntity>& entities);
    static void writeFooter(std::ostream& out);

    // Entity-specific writers
    static void writeLine(std::ostream& out, const Import::DXFLine& line);
    static void writeArc(std::ostream& out, const Import::DXFArc& arc);
    static void writeCircle(std::ostream& out, const Import::DXFCircle& circle);
    static void writeLWPolyline(std::ostream& out, const Import::DXFLWPolyline& poly);
    static void writeEllipse(std::ostream& out, const Import::DXFEllipse& ellipse);
    static void writePoint(std::ostream& out, const Import::DXFPoint& point);
    // ... other entity types
};

} // namespace Export
} // namespace OwnCAD
```

**Key Requirements**:
- Write minimal valid DXF structure (HEADER, TABLES, ENTITIES, EOF sections)
- Preserve handles exactly as imported
- Preserve layer names and color numbers
- Use DXF R2018 AC1032 format (modern, widely supported)
- Write clean, readable DXF (proper indentation for debugging)

**DXF Format Reference**:
```
0
SECTION
2
HEADER
...
0
ENDSEC
0
SECTION
2
ENTITIES
0
LINE
5
[HANDLE]
8
[LAYER]
62
[COLOR]
10
[X1]
20
[Y1]
11
[X2]
21
[Y2]
...
0
ENDSEC
0
EOF
```

#### Step 2: Create `GeometryExporter` Class
**Responsibility**: Convert `GeometryEntityWithMetadata` back to `DXFEntity`.

**Interface Design**:
```cpp
namespace OwnCAD {
namespace Export {

struct ExportResult {
    bool success;
    std::vector<Import::DXFEntity> entities;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    size_t totalExported;
    size_t totalFailed;
};

class GeometryExporter {
public:
    /**
     * @brief Convert internal geometry back to DXF entities
     * @param entities Internal geometry with metadata
     * @return Export result with DXF entities or errors
     */
    static ExportResult exportToDXF(
        const std::vector<Import::GeometryEntityWithMetadata>& entities
    );

private:
    static std::optional<Import::DXFLine> exportLine(
        const Geometry::Line2D& line,
        const std::string& layer,
        const std::string& handle,
        int colorNumber
    );

    static std::optional<Import::DXFArc> exportArc(
        const Geometry::Arc2D& arc,
        const std::string& layer,
        const std::string& handle,
        int colorNumber
    );

    static std::optional<Import::DXFEllipse> exportEllipse(
        const Geometry::Ellipse2D& ellipse,
        const std::string& layer,
        const std::string& handle,
        int colorNumber
    );

    static std::optional<Import::DXFPoint> exportPoint(
        const Geometry::Point2D& point,
        const std::string& layer,
        const std::string& handle,
        int colorNumber
    );

    // Conversion helpers
    static double radiansToDegrees(double radians) noexcept;
};

} // namespace Export
} // namespace OwnCAD
```

**Key Requirements**:
- **Exact inverse of GeometryConverter**: Line2D → DXFLine, Arc2D → DXFArc, etc.
- Preserve metadata: handle, layer, color
- Convert radians back to degrees (DXF uses degrees)
- Handle edge cases: full circles (Arc2D with 360° sweep → DXFCircle)
- Error handling: log any geometry that cannot be exported

#### Step 3: Integrate Export into DocumentModel
**File**: `include/model/DocumentModel.h`, `src/model/DocumentModel.cpp`

**Add Methods**:
```cpp
/**
 * @brief Export document to DXF file
 * @param filePath Output DXF file path
 * @return true if successful
 */
bool exportDXFFile(const std::string& filePath) const;

/**
 * @brief Get export errors
 */
const std::vector<std::string>& exportErrors() const noexcept;
```

**Implementation**:
```cpp
bool DocumentModel::exportDXFFile(const std::string& filePath) const {
    exportErrors_.clear();

    // Step 1: Convert geometry to DXF entities
    Export::ExportResult exportResult = Export::GeometryExporter::exportToDXF(entities_);

    if (!exportResult.success || !exportResult.errors.empty()) {
        exportErrors_ = exportResult.errors;
        return false;
    }

    // Step 2: Write DXF entities to file
    bool writeSuccess = Export::DXFWriter::writeFile(filePath, exportResult.entities);

    if (!writeSuccess) {
        exportErrors_.push_back("Failed to write DXF file: " + filePath);
        return false;
    }

    return true;
}
```

#### Step 4: Update CMakeLists.txt
**File**: `CMakeLists.txt`

**Add**:
```cmake
# Export module
include/export/DXFWriter.h
src/export/DXFWriter.cpp
include/export/GeometryExporter.h
src/export/GeometryExporter.cpp
```

**Create directory**:
```bash
mkdir include/export
mkdir src/export
```

**Deliverable**: Functional DXF export pipeline that preserves all metadata.

---

### Subtask 8.3: Implement Export Validation
**Status**: ❌ NOT STARTED
**Effort**: 2-3 hours
**Dependencies**: 8.2 complete
**Files to Create**:
- `include/model/ExportValidator.h`
- `src/model/ExportValidator.cpp`

#### Validation Checks

**1. Entity Count Match**
```cpp
struct EntityCountReport {
    size_t importedCount;
    size_t exportedCount;
    bool matches;
    std::vector<std::string> missingHandles;
};

EntityCountReport validateEntityCount(
    const DocumentModel& document,
    const ExportResult& exportResult
);
```

**Expected**: `imported.size() == exported.size()`
**Failure Reason**: Entity lost during internal processing or export conversion.

**2. Geometry Precision Check**
```cpp
struct PrecisionReport {
    std::vector<std::string> entitiesWithPrecisionLoss;
    double maxDeviation;
    bool withinTolerance;
};

PrecisionReport validatePrecision(
    const DocumentModel& originalDocument,
    const DocumentModel& reimportedDocument,
    double tolerance = GEOMETRY_EPSILON
);
```

**Test**: For each entity pair (original vs re-imported):
- Line2D: Compare start/end points within tolerance
- Arc2D: Compare center, radius, angles within tolerance
- Ellipse2D: Compare center, axes, parameters within tolerance

**Expected**: All geometry differences < GEOMETRY_EPSILON (1e-9)
**Failure Reason**: Precision loss in export/import conversions (e.g., double→string→double).

**3. Layer Integrity Check**
```cpp
struct LayerReport {
    std::vector<std::string> originalLayers;
    std::vector<std::string> exportedLayers;
    std::vector<std::string> missingLayers;
    std::vector<std::string> extraLayers;
    bool matches;
};

LayerReport validateLayers(
    const DocumentModel& original,
    const DocumentModel& reimported
);
```

**Expected**: Exact layer name match (set equality).
**Failure Reason**: Layer metadata not preserved during export.

**4. Handle Preservation Check**
```cpp
struct HandleReport {
    std::vector<std::string> originalHandles;
    std::vector<std::string> exportedHandles;
    std::vector<std::string> missingHandles;
    std::vector<std::string> duplicateHandles;
    bool matches;
};

HandleReport validateHandles(
    const DocumentModel& original,
    const std::vector<Import::DXFEntity>& exported
);
```

**Expected**: Every exported entity has the same handle as original.
**Failure Reason**: Handle lost or corrupted during export.

**Deliverable**: Automated validation reports showing export correctness.

---

### Subtask 8.4: Implement Round-Trip Testing
**Status**: ❌ NOT STARTED
**Effort**: 3-4 hours
**Dependencies**: 8.2, 8.3 complete
**Files to Create**:
- `tests/model/test_DXFRoundTrip.cpp`
- `include/model/RoundTripValidator.h`
- `src/model/RoundTripValidator.cpp`

#### Test Strategy

**Round-Trip Test Flow**:
```
1. Import original DXF → DocumentModel A
2. Export DocumentModel A → DXF File (temp)
3. Re-import temp DXF → DocumentModel B
4. Compare A vs B (geometry, metadata, statistics)
```

**Test Cases**:

**Test 1: Simple Geometry Round-Trip**
```cpp
TEST(DXFRoundTrip, SimpleGeometry) {
    // Load test DXF with basic shapes (lines, arcs, circles)
    DocumentModel original;
    ASSERT_TRUE(original.loadDXFFile("tests/data/simple.dxf"));

    // Export to temp file
    std::string tempPath = createTempFile();
    ASSERT_TRUE(original.exportDXFFile(tempPath));

    // Re-import
    DocumentModel reimported;
    ASSERT_TRUE(reimported.loadDXFFile(tempPath));

    // Validate entity count
    EXPECT_EQ(original.entities().size(), reimported.entities().size());

    // Validate geometry precision
    validateGeometryMatch(original, reimported, GEOMETRY_EPSILON);

    // Validate metadata
    validateMetadataMatch(original, reimported);

    // Cleanup
    std::remove(tempPath.c_str());
}
```

**Test 2: Complex Polyline Round-Trip**
```cpp
TEST(DXFRoundTrip, PolylineWithBulges) {
    // Test polylines with arc segments (bulge values)
    // CRITICAL: Bulge → Arc2D → DXFArc conversion must preserve geometry
}
```

**Test 3: Metadata Preservation**
```cpp
TEST(DXFRoundTrip, MetadataPreservation) {
    // Test handle, layer, color preservation
}
```

**Test 4: Precision Stress Test**
```cpp
TEST(DXFRoundTrip, PrecisionStressTest) {
    // Test with extreme coordinates (1e-9, 1e9)
    // Test with tight tolerances (angles near 0°, 90°, 180°, 270°)
}
```

**Test 5: Multiple Round-Trips**
```cpp
TEST(DXFRoundTrip, MultipleRoundTrips) {
    // Test: Import → Export → Import → Export → Import
    // Verify: No cumulative precision loss
    // Expected: Geometry identical after 3+ round-trips
}
```

**Visual Diff Helper** (for manual verification):
```cpp
class RoundTripVisualizer {
public:
    /**
     * @brief Generate overlay comparison image
     * @param original Original document
     * @param reimported Re-imported document
     * @param outputPath PNG file path
     *
     * Renders:
     * - Original geometry in blue
     * - Re-imported geometry in red
     * - Perfect overlap shows as purple (success)
     * - Mismatches show as separate blue/red lines (failure)
     */
    static void generateOverlay(
        const DocumentModel& original,
        const DocumentModel& reimported,
        const std::string& outputPath
    );
};
```

**Deliverable**: Automated test suite proving import → export → re-import is lossless.

---

### Subtask 8.5: Add Export to UI
**Status**: ❌ NOT STARTED (OPTIONAL - can defer to later phase)
**Effort**: 1 hour
**Dependencies**: 8.2 complete
**File**: `src/main.cpp`

**Add Menu Item**:
```cpp
// File menu
QAction* exportAction = fileMenu->addAction("Export DXF...");
QObject::connect(exportAction, &QAction::triggered, [&]() {
    QString filePath = QFileDialog::getSaveFileName(
        &mainWindow,
        "Export DXF File",
        "",
        "DXF Files (*.dxf);;All Files (*)"
    );

    if (filePath.isEmpty()) return;

    if (document.exportDXFFile(filePath.toStdString())) {
        statusBar->showMessage("DXF exported successfully: " + filePath, 3000);
    } else {
        QMessageBox::critical(&mainWindow, "Export Error",
            "Failed to export DXF file.\nCheck console for details.");
    }
});
```

**Keyboard Shortcut**: Ctrl+Shift+S (Save As)

**Deliverable**: User can export modified documents back to DXF.

---

## Risk Analysis

### Technical Risks

**Risk 1: DXF Format Complexity**
- **Problem**: DXF has 30+ years of format variations, optional fields, vendor extensions.
- **Mitigation**: Target DXF R2018 (AC1032) only. Use minimal valid structure. Test with AutoCAD/LibreCAD for compatibility.

**Risk 2: Precision Loss in String Conversion**
- **Problem**: double → string → double conversions can lose precision.
- **Mitigation**: Use high-precision string formatting (15+ decimal places). Add precision validation tests.

**Risk 3: Polyline Bulge Conversion**
- **Problem**: Bulge → Arc2D → DXFArc may not round-trip perfectly due to geometric reconstruction.
- **Mitigation**: Add dedicated round-trip tests for bulge polylines. Document tolerance requirements.

**Risk 4: Full Circle Representation**
- **Problem**: Arc2D can represent full circles (360° sweep), but should export as DXFCircle for compatibility.
- **Mitigation**: Add export logic to detect full circles and convert Arc2D → DXFCircle.

### Process Risks

**Risk 5: Scope Creep**
- **Problem**: Export functionality could expand to include formatting options, layer filtering, etc.
- **Mitigation**: Strict scope: Export ALL entities exactly as imported. No filtering, no options. Defer enhancements to Phase 2.

---

## Testing Strategy

### Unit Tests
- **GeometryExporter**: Test each entity type conversion (Line2D → DXFLine, etc.)
- **DXFWriter**: Test DXF text generation (verify group codes, field ordering)

### Integration Tests
- **DocumentModel.exportDXFFile()**: End-to-end export test
- **Round-trip tests**: Import → Export → Re-import comparison

### Manual Tests
1. Export simple test DXF
2. Open exported DXF in AutoCAD / LibreCAD / FreeCAD
3. Verify visual match with original
4. Check for import warnings/errors

---

## Acceptance Criteria

**Subtask 8.2 (Export) Complete When**:
- ✅ DXFWriter can write all supported entity types (Line, Arc, Circle, Ellipse, Point)
- ✅ GeometryExporter converts all internal geometry types to DXF entities
- ✅ DocumentModel.exportDXFFile() method works end-to-end
- ✅ Exported DXF opens successfully in AutoCAD / LibreCAD
- ✅ All metadata preserved: handles, layers, colors

**Subtask 8.3 (Validation) Complete When**:
- ✅ Entity count validator detects missing entities
- ✅ Precision validator detects geometry drift > tolerance
- ✅ Layer validator detects missing/extra layers
- ✅ Handle validator detects handle corruption

**Subtask 8.4 (Round-Trip) Complete When**:
- ✅ 5+ round-trip test cases pass
- ✅ Simple geometry round-trip is lossless (< GEOMETRY_EPSILON deviation)
- ✅ Complex polylines with bulges round-trip correctly
- ✅ Multiple round-trips (3+) show no cumulative error
- ✅ Test suite runs in CI pipeline

**Task 8 Complete When**:
- ✅ All subtasks 8.2, 8.3, 8.4 complete
- ✅ Real-world DXF files round-trip successfully (test with fabrication shop DXFs)
- ✅ No silent data loss or corruption
- ✅ Export functionality integrated into UI (optional)

---

## Implementation Order

**Recommended sequence**:

1. **Day 1**: Subtask 8.2 Step 1 - DXFWriter class (write minimal valid DXF)
2. **Day 2**: Subtask 8.2 Step 2 - GeometryExporter class (geometry → DXF entities)
3. **Day 3**: Subtask 8.2 Steps 3-4 - DocumentModel integration, CMake updates, manual testing
4. **Day 4**: Subtask 8.3 - Export validation (entity count, precision, layers, handles)
5. **Day 5**: Subtask 8.4 - Round-trip testing (automated test suite)
6. **Day 6**: Subtask 8.5 (optional) - UI integration (File → Export DXF menu)

**Total Estimated Time**: 5-6 days (assuming 4-6 hours/day focused work)

---

## Dependencies

**Before starting this task**:
- ✅ DocumentModel with entity storage (DONE)
- ✅ DXF import pipeline (DONE)
- ✅ Metadata preservation in GeometryEntityWithMetadata (DONE)

**Blocks**:
- None (can start immediately)

**Blocked by this task**:
- Phase 1 Exit Criteria (round-trip safety requirement)
- Phase 2 features that require saving edited files

---

## Questions to Resolve Before Implementation

1. **DXF Version Target**: Use DXF R2018 AC1032? (Recommendation: Yes - modern, widely supported)
2. **Export Scope**: Export ALL entities or allow layer/selection filtering? (Recommendation: All entities - keep it simple)
3. **String Precision**: How many decimal places for coordinate output? (Recommendation: 15 decimal places - matches double precision)
4. **Error Handling**: Fail entire export if one entity fails, or skip invalid entities? (Recommendation: Fail entire export - no silent data loss)
5. **UI Priority**: Add export UI now or defer to Phase 2? (Recommendation: Add basic export UI now - needed for manual testing)

---

## Next Steps

**For Claude**:
1. Review this guide with user
2. Get approval for architecture decisions
3. Start with Subtask 8.2 Step 1 (DXFWriter class)
4. Implement incrementally, test at each step

**For User**:
1. Approve/modify this plan
2. Answer questions above
3. Provide sample DXF files for testing (especially complex ones from fabrication shop)
4. Decide if export UI is required now or can be deferred

---

## File Structure After Implementation

```
include/
  export/                    ← NEW DIRECTORY
    DXFWriter.h
    GeometryExporter.h
  model/
    DocumentModel.h          ← MODIFIED (add exportDXFFile method)
    ExportValidator.h        ← NEW
    RoundTripValidator.h     ← NEW

src/
  export/                    ← NEW DIRECTORY
    DXFWriter.cpp
    GeometryExporter.cpp
  model/
    DocumentModel.cpp        ← MODIFIED
    ExportValidator.cpp      ← NEW
    RoundTripValidator.cpp   ← NEW

tests/
  model/
    test_DXFRoundTrip.cpp    ← NEW
  export/                    ← NEW DIRECTORY
    test_DXFWriter.cpp
    test_GeometryExporter.cpp
```

---

## References

- **DXF Format Specification**: [Autodesk DXF Reference (AC1032 / 2018)](https://help.autodesk.com/view/OARX/2018/ENU/?guid=GUID-235B22E0-A567-4CF6-92D3-38A2306D73F3)
- **Existing Code**: `src/import/DXFParser.cpp` (reverse engineer output format)
- **Existing Tests**: `tests/import/` (mirror structure for export tests)

---

**End of Implementation Guide**
