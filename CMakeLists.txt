cmake_minimum_required(VERSION 3.16)

project(OwnCAD VERSION 0.1.0 LANGUAGES CXX)

# ============================================================================
# PROJECT CONFIGURATION
# ============================================================================

# C++ Standard - using C++17 minimum, C++20 preferred
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable strict warnings for production-grade code
if(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Build type default to Release with debug info
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# ============================================================================
# QT6 CONFIGURATION
# ============================================================================

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Test
)

# ============================================================================
# GLOBAL INCLUDES
# ============================================================================

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# ============================================================================
# GEOMETRY LIBRARY (CORE IP)
# ============================================================================

set(GEOMETRY_HEADERS
    include/geometry/Point2D.h
    include/geometry/Line2D.h
    include/geometry/Arc2D.h
    include/geometry/Ellipse2D.h
    include/geometry/BoundingBox.h
    include/geometry/GeometryMath.h
    include/geometry/GeometryValidator.h
    include/geometry/GeometryConstants.h
    include/geometry/TransformValidator.h
)

set(GEOMETRY_SOURCES
    src/geometry/Point2D.cpp
    src/geometry/Line2D.cpp
    src/geometry/Arc2D.cpp
    src/geometry/Ellipse2D.cpp
    src/geometry/BoundingBox.cpp
    src/geometry/GeometryMath.cpp
    src/geometry/GeometryValidator.cpp
    src/geometry/TransformValidator.cpp
)

add_library(geometry STATIC
    ${GEOMETRY_HEADERS}
    ${GEOMETRY_SOURCES}
)

target_link_libraries(geometry
    Qt6::Core
)

target_include_directories(geometry PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# IMPORT LIBRARY (DXF PARSING)
# ============================================================================

set(IMPORT_HEADERS
    include/import/DXFEntity.h
    include/import/DXFParser.h
    include/import/GeometryConverter.h
    include/import/DXFColors.h
)

set(IMPORT_SOURCES
    src/import/DXFParser.cpp
    src/import/GeometryConverter.cpp
    src/import/DXFColors.cpp
)

add_library(import STATIC
    ${IMPORT_HEADERS}
    ${IMPORT_SOURCES}
)

target_link_libraries(import
    Qt6::Core
    Qt6::Gui
    geometry
)

target_include_directories(import PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# EXPORT LIBRARY (DXF WRITING)
# ============================================================================

set(EXPORT_HEADERS
    include/export/DXFWriter.h
    include/export/GeometryExporter.h
)

set(EXPORT_SOURCES
    src/export/DXFWriter.cpp
    src/export/GeometryExporter.cpp
)

add_library(export STATIC
    ${EXPORT_HEADERS}
    ${EXPORT_SOURCES}
)

target_link_libraries(export
    Qt6::Core
    geometry
    import
)

target_include_directories(export PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# MODEL LIBRARY (DOCUMENT MANAGEMENT)
# ============================================================================

set(MODEL_HEADERS
    include/model/DocumentModel.h
    include/model/Command.h
    include/model/CommandHistory.h
    include/model/EntityCommands.h
    include/model/ExportValidator.h
)

set(MODEL_SOURCES
    src/model/DocumentModel.cpp
    src/model/CommandHistory.cpp
    src/model/EntityCommands.cpp
    src/model/ExportValidator.cpp
)

add_library(model STATIC
    ${MODEL_HEADERS}
    ${MODEL_SOURCES}
)

target_link_libraries(model
    Qt6::Core
    geometry
    import
    export
)

target_include_directories(model PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# UI LIBRARY (CANVAS, VIEWPORT, TOOLS)
# ============================================================================

set(UI_HEADERS
    include/ui/CADCanvas.h
    include/ui/GridSettingsDialog.h
    include/ui/SelectionManager.h
    include/ui/Tool.h
    include/ui/ToolManager.h
    include/ui/LineTool.h
    include/ui/ArcTool.h
    include/ui/RectangleTool.h
    include/ui/MoveTool.h
    include/ui/RotateTool.h
    include/ui/RotateInputDialog.h
    include/ui/MirrorTool.h
)

set(UI_SOURCES
    src/ui/CADCanvas.cpp
    src/ui/GridSettingsDialog.cpp
    src/ui/SelectionManager.cpp
    src/ui/ToolManager.cpp
    src/ui/LineTool.cpp
    src/ui/ArcTool.cpp
    src/ui/RectangleTool.cpp
    src/ui/MoveTool.cpp
    src/ui/RotateTool.cpp
    src/ui/RotateInputDialog.cpp
    src/ui/MirrorTool.cpp
)

add_library(ui STATIC
    ${UI_HEADERS}
    ${UI_SOURCES}
)

target_link_libraries(ui
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    geometry
    import
    model
)

target_include_directories(ui PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# MAIN APPLICATION
# ============================================================================

set(APP_SOURCES
    src/main.cpp
)
# Console enabled for debugging (remove WIN32 flag)
# Add WIN32 flag back for release builds if you don't want console window
add_executable(OwnCAD
    ${APP_SOURCES}
)

target_link_libraries(OwnCAD
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    geometry
    import
    model
    ui
)

# ============================================================================
# TESTING
# ============================================================================

enable_testing()

# Helper function to create individual test executables
function(add_geometry_test test_name test_file)
    add_executable(${test_name}
        ${test_file}
    )

    target_link_libraries(${test_name}
        Qt6::Test
        Qt6::Core
        geometry
    )

    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Individual test executables (avoids multiple main() definitions)
add_geometry_test(test_Point2D tests/geometry/test_Point2D.cpp)
add_geometry_test(test_Line2D tests/geometry/test_Line2D.cpp)
add_geometry_test(test_Arc2D tests/geometry/test_Arc2D.cpp)
add_geometry_test(test_GeometryMath tests/geometry/test_GeometryMath.cpp)
add_geometry_test(test_Intersections tests/geometry/test_Intersections.cpp)
add_geometry_test(test_TransformValidator tests/geometry/test_TransformValidator.cpp)
add_geometry_test(test_DuplicateDetection tests/geometry/test_DuplicateDetection.cpp)

# Helper function for model tests
function(add_model_test test_name test_file)
    add_executable(${test_name}
        ${test_file}
    )

    target_link_libraries(${test_name}
        Qt6::Test
        Qt6::Core
        geometry
        model
        import
    )

    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Model tests
add_model_test(test_CommandHistory tests/model/test_CommandHistory.cpp)
add_model_test(test_EntityCommands tests/model/test_EntityCommands.cpp)
add_model_test(test_UndoRedoStress tests/model/test_UndoRedoStress.cpp)
add_model_test(test_MetadataPreservation tests/model/test_MetadataPreservation.cpp)
add_model_test(test_DXFRoundTrip tests/model/test_DXFRoundTrip.cpp)


# ============================================================================
# INSTALLATION
# ============================================================================

install(TARGETS OwnCAD
    RUNTIME DESTINATION bin
)

# ============================================================================
# BUILD INFO
# ============================================================================

message(STATUS "========================================")
message(STATUS "OwnCAD - Industrial 2D CAD Validator")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt6 Version: ${Qt6_VERSION}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "========================================")
