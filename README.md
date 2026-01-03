# OwnCAD - Industrial 2D CAD Validator

**Version:** 0.1.0 (Phase 1 - Core Geometry)
**Target:** Windows Desktop
**Tech Stack:** C++17 + Qt6 Widgets

---

## Project Overview

OwnCAD is a **production-safety tool** for industrial sheet-metal fabrication, specifically designed for:
- Laser cutting
- Plasma cutting
- Waterjet cutting
- CNC sheet-metal workflows

**Core Philosophy:**
- Validation-first, drawing-second
- Geometry correctness > UI beauty
- Offline-first (no cloud dependency)
- Deterministic behavior (no hidden automation)

---

## Current Status (Phase 1 - Week 1)

### ✅ Completed: Core Geometry Stability

**Implemented:**
- [x] Point2D class (immutable, validated)
- [x] Line2D class (factory pattern, zero-length rejection)
- [x] Arc2D class (angle normalization, direction preservation)
- [x] BoundingBox class (arc quadrant handling)
- [x] GeometryMath utilities (distance, angles, intersections)
- [x] GeometryValidator (zero-length, zero-radius, stability checks)
- [x] Qt6 application shell with basic UI
- [x] Unit test framework (Qt Test)
- [x] CMake build system

**Next Steps (Phase 1):**
- Week 1-2: Selection system
- Week 2: Drawing tools (Line, Arc, Rectangle)
- Week 3: Transformation tools (Move, Rotate, Mirror)
- Week 3-4: Undo/Redo system
- Week 4: Duplicate & zero-length detection UI

---

## Prerequisites

### Required Software

1. **Qt6** (version 6.5 or later)
   - Download from: https://www.qt.io/download-qt-installer
   - Components needed:
     - Qt 6.x for Desktop (MSVC 2019 or MinGW)
     - Qt Creator (recommended IDE)
     - CMake integration

2. **CMake** (version 3.16 or later)
   - Download from: https://cmake.org/download/
   - Add to system PATH during installation

3. **C++ Compiler**
   - **Option A (Recommended):** Visual Studio 2019 or 2022
     - Download Community Edition: https://visualstudio.microsoft.com/
     - Install "Desktop development with C++" workload

   - **Option B:** MinGW (included with Qt installer)
     - Select MinGW during Qt installation

4. **Git** (for version control)
   - Download from: https://git-scm.com/

---

## Build Instructions

### Option 1: Qt Creator (Recommended for Development)

1. **Open the project:**
   ```
   File → Open File or Project → Select "CMakeLists.txt"
   ```

2. **Configure the build:**
   - Qt Creator will auto-detect Qt6 installation
   - Select build kit (MSVC or MinGW)
   - Click "Configure Project"

3. **Build the project:**
   ```
   Build → Build All (Ctrl+B)
   ```

4. **Run the application:**
   ```
   Build → Run (Ctrl+R)
   ```

5. **Run tests:**
   ```
   Tools → Tests → Run All Tests
   ```

---

### Option 2: Command Line (MSVC)

1. **Open "x64 Native Tools Command Prompt for VS 2022"**

2. **Navigate to project directory:**
   ```cmd
   cd D:\OwnCAD
   ```

3. **Create build directory:**
   ```cmd
   mkdir build
   cd build
   ```

4. **Configure with CMake:**
   ```cmd
   cmake .. -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2019_64"
   ```
   *(Replace `6.x.x` with your Qt version)*

5. **Build the project:**
   ```cmd
   cmake --build . --config Release
   ```

6. **Run the application:**
   ```cmd
   Release\OwnCAD.exe
   ```

7. **Run tests:**
   ```cmd
   ctest -C Release --output-on-failure
   ```

---

### Option 3: Command Line (MinGW)

1. **Open Command Prompt or PowerShell**

2. **Navigate to project directory:**
   ```cmd
   cd D:\OwnCAD
   ```

3. **Create build directory:**
   ```cmd
   mkdir build
   cd build
   ```

4. **Configure with CMake:**
   ```cmd
   cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\mingw_64"
   ```

5. **Build the project:**
   ```cmd
   mingw32-make
   ```

6. **Run the application:**
   ```cmd
   OwnCAD.exe
   ```

7. **Run tests:**
   ```cmd
   ctest --output-on-failure
   ```

---

## Project Structure

```
OwnCAD/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── CLAUDE.md              # Development guidelines
│
├── include/
│   └── geometry/           # Public geometry headers
│       ├── Point2D.h
│       ├── Line2D.h
│       ├── Arc2D.h
│       ├── BoundingBox.h
│       ├── GeometryMath.h
│       ├── GeometryValidator.h
│       └── GeometryConstants.h
│
├── src/
│   ├── main.cpp           # Application entry point
│   ├── geometry/          # Geometry implementations
│   │   ├── Point2D.cpp
│   │   ├── Line2D.cpp
│   │   ├── Arc2D.cpp
│   │   ├── BoundingBox.cpp
│   │   ├── GeometryMath.cpp
│   │   └── GeometryValidator.cpp
│   ├── import/            # DXF import (Phase 0)
│   ├── export/            # DXF export (Phase 0)
│   └── ui/                # Qt UI components
│
├── tests/
│   └── geometry/          # Unit tests
│       ├── test_Point2D.cpp
│       ├── test_Line2D.cpp
│       ├── test_Arc2D.cpp
│       └── test_GeometryMath.cpp
│
└── tasks/
    └── phase1.md          # Phase 1 task breakdown
```

---

## Architecture Highlights

### Geometry Layer Design

**Immutability:** All geometry classes are immutable by design
- Prevents accidental state corruption
- Thread-safe for future multi-threading
- Value semantics (copyable, not mutable)

**Factory Pattern:** Validation enforced at construction
```cpp
auto line = Line2D::create(startPoint, endPoint);
if (line.has_value()) {
    // Use valid line
} else {
    // Handle invalid geometry
}
```

**Tolerance-Based Equality:**
- Manufacturing precision: `GEOMETRY_EPSILON = 1e-9`
- Explicit tolerance parameters in comparisons
- No hidden global state

**Lazy Caching:**
- Bounding boxes computed on first access
- Length calculations cached
- Performance-critical queries optimized

---

## Running Tests

### All tests:
```bash
ctest --output-on-failure
```

### Specific test:
```bash
# From build directory
./test_Point2D
./test_Line2D
./test_Arc2D
./test_GeometryMath
```

### Individual test executables:
Each test class has its own executable for better isolation and parallel execution:
- `test_Point2D` - Point2D class tests
- `test_Line2D` - Line2D class tests
- `test_Arc2D` - Arc2D class tests
- `test_GeometryMath` - GeometryMath utilities tests

### Test coverage (current):
- ✅ Point2D: Construction, equality, distance, validation
- ✅ Line2D: Creation, length, bounding box, point queries
- ✅ Arc2D: Angles, sweep, full circles, endpoints
- ✅ GeometryMath: Distance, angles, normalization, arc calculations

---

## Development Guidelines

See `CLAUDE.md` for full development philosophy and standards.

**Key Principles:**
1. **No invalid geometry can be constructed** (enforced by factory pattern)
2. **Every geometry function must have tests**
3. **Root cause analysis** for all bugs (no quick patches)
4. **Production-grade code only** (no prototypes or TODOs)
5. **Geometry safety > convenience**

---

## Troubleshooting

### CMake can't find Qt6
```bash
# Explicitly set Qt path
cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2019_64"
```

### Build fails with "Qt6::Widgets not found"
- Ensure Qt6 Widgets component is installed
- Re-run Qt Maintenance Tool if needed

### Tests fail to run
- Ensure Qt6::Test is installed
- Check that test executables are built in `build/` directory

### Application won't start (missing DLLs)
- Copy Qt DLLs to executable directory, or
- Add Qt `bin` directory to system PATH, or
- Use `windeployqt.exe` to deploy dependencies:
  ```bash
  windeployqt Release\OwnCAD.exe
  ```

---

## License

Internal project - Not for public distribution.

---

## Contact

Project lead: [Your Name]
Phase: 1 of 4 (Geometry & Editing)
Target completion: Month 3
