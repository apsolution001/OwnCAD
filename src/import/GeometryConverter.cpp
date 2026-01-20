#include "import/GeometryConverter.h"
#include "geometry/GeometryConstants.h"
#include <cmath>
#include <sstream>
#include <iostream>

namespace OwnCAD {
namespace Import {

using namespace OwnCAD::Geometry;

// ============================================================================
// PUBLIC API
// ============================================================================

ConversionResult GeometryConverter::convert(const std::vector<DXFEntity>& dxfEntities) {
    ConversionResult result;

    std::cout << "\n=== GeometryConverter: Converting " << dxfEntities.size() << " DXF entities ===" << std::endl;

    for (size_t i = 0; i < dxfEntities.size(); ++i) {
        const auto& dxfEntity = dxfEntities[i];
        std::cout << "\nEntity " << (i+1) << "/" << dxfEntities.size() << " (line " << dxfEntity.lineNumber << "):" << std::endl;
        std::optional<GeometryEntity> converted;
        std::string layer;
        std::string handle;
        int colorNumber = 256;  // Default: BYLAYER

        // Convert based on type
        std::visit([&](auto&& entity) {
            using T = std::decay_t<decltype(entity)>;

            if constexpr (std::is_same_v<T, DXFLine>) {
                std::cout << "  Type: LINE" << std::endl;
                std::cout << "  Start: (" << entity.startX << ", " << entity.startY << ")" << std::endl;
                std::cout << "  End: (" << entity.endX << ", " << entity.endY << ")" << std::endl;

                auto line = convertLine(entity);
                if (line.has_value()) {
                    double len = line->length();
                    std::cout << "  ✅ VALID - Length: " << len << std::endl;
                    converted = GeometryEntity(line.value());
                    layer = entity.layer;
                    handle = entity.handle;
                    colorNumber = entity.colorNumber;
                } else{
                    std::cout << "  ❌ REJECTED - Zero-length or invalid coordinates" << std::endl;
                    result.errors.push_back(
                        createErrorMessage("LINE", "Invalid geometry (zero-length or bad coordinates)",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }
            }
            else if constexpr (std::is_same_v<T, DXFArc>) {
                std::cout << "  Type: ARC" << std::endl;
                std::cout << "  Center: (" << entity.centerX << ", " << entity.centerY << ")" << std::endl;
                std::cout << "  Radius: " << entity.radius << std::endl;
                std::cout << "  Angles: " << entity.startAngle << "° to " << entity.endAngle << "°" << std::endl;

                auto arc = convertArc(entity);
                if (arc.has_value()) {
                    std::cout << "  ✅ VALID" << std::endl;
                    converted = GeometryEntity(arc.value());
                    layer = entity.layer;
                    handle = entity.handle;
                    colorNumber = entity.colorNumber;
                } else {
                    std::cout << "  ❌ REJECTED - Zero-radius or degenerate" << std::endl;
                    result.errors.push_back(
                        createErrorMessage("ARC", "Invalid geometry (zero-radius or degenerate)",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }
            }
            else if constexpr (std::is_same_v<T, DXFCircle>) {
                std::cout << "  Type: CIRCLE" << std::endl;
                std::cout << "  Center: (" << entity.centerX << ", " << entity.centerY << ")" << std::endl;
                std::cout << "  Radius: " << entity.radius << std::endl;

                auto arc = convertCircle(entity);
                if (arc.has_value()) {
                    std::cout << "  ✅ VALID (converted to full arc)" << std::endl;
                    converted = GeometryEntity(arc.value());
                    layer = entity.layer;
                    handle = entity.handle;
                    colorNumber = entity.colorNumber;
                } else {
                    std::cout << "  ❌ REJECTED - Zero-radius" << std::endl;
                    result.errors.push_back(
                        createErrorMessage("CIRCLE", "Invalid geometry (zero-radius)",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }
            }
            else if constexpr (std::is_same_v<T, DXFLWPolyline>) {
                std::cout << "  Type: LWPOLYLINE" << std::endl;
                std::cout << "  Vertices: " << entity.vertices.size() << std::endl;
                std::cout << "  Closed: " << (entity.closed ? "yes" : "no") << std::endl;

                // Check for bulge values
                int bulgeCount = 0;
                for (const auto& v : entity.vertices) {
                    if (std::abs(v.bulge) > 1e-9) bulgeCount++;
                }
                if (bulgeCount > 0) {
                    std::cout << "  Bulge segments: " << bulgeCount << " (will be converted to arcs)" << std::endl;
                }

                // Convert polyline to geometry segments (Line2D or Arc2D)
                auto segments = convertPolyline(entity);

                // Count segment types for logging
                int lineCount = 0, arcCount = 0;
                for (const auto& seg : segments) {
                    if (std::holds_alternative<Geometry::Line2D>(seg)) lineCount++;
                    else if (std::holds_alternative<Geometry::Arc2D>(seg)) arcCount++;
                }
                std::cout << "  Converted to " << segments.size() << " segments ("
                          << lineCount << " lines, " << arcCount << " arcs)" << std::endl;

                if (!segments.empty()) {
                    // Add all segments individually
                    for (const auto& segment : segments) {
                        GeometryEntityWithMetadata entityWithMeta{
                            segment,               // entity (already GeometryEntity)
                            entity.layer,          // layer
                            entity.handle,         // handle (same for all segments)
                            entity.colorNumber,    // color
                            dxfEntity.lineNumber   // sourceLineNumber
                        };
                        result.entities.push_back(entityWithMeta);
                    }

                    // Count this as ONE converted entity (the polyline)
                    // even though it created multiple segments
                    result.totalConverted++;

                    std::cout << "  ✅ VALID - Polyline decomposed into " << segments.size() << " segments" << std::endl;
                } else {
                    std::cout << "  ❌ REJECTED - No valid segments" << std::endl;
                    result.errors.push_back(
                        createErrorMessage("LWPOLYLINE", "No valid segments could be created",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }

                // Mark as processed (don't add to result below)
                layer.clear();
                handle.clear();
            }
            else if constexpr (std::is_same_v<T, DXFEllipse>) {
                std::cout << "  Type: ELLIPSE" << std::endl;
                std::cout << "  Center: (" << entity.centerX << ", " << entity.centerY << ")" << std::endl;
                std::cout << "  Minor/Major ratio: " << entity.minorAxisRatio << std::endl;

                auto ellipse = convertEllipse(entity);
                if (ellipse.has_value()) {
                    std::cout << "  ✅ VALID" << std::endl;
                    converted = GeometryEntity(ellipse.value());
                    layer = entity.layer;
                    handle = entity.handle;
                    colorNumber = entity.colorNumber;
                } else {
                    std::cout << "  ❌ REJECTED - Invalid ellipse parameters" << std::endl;
                    result.errors.push_back(
                        createErrorMessage("ELLIPSE", "Invalid geometry",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }
            }
            else if constexpr (std::is_same_v<T, DXFSpline>) {
                std::cout << "  Type: SPLINE" << std::endl;
                std::cout << "  Control points: " << entity.controlPoints.size() << std::endl;
                std::cout << "  Degree: " << entity.degree << std::endl;

                // Convert spline to line segments
                auto lines = convertSpline(entity);
                std::cout << "  Approximated with " << lines.size() << " line segments" << std::endl;

                if (!lines.empty()) {
                    // Add all line segments individually
                    for (const auto& line : lines) {
                        GeometryEntityWithMetadata entityWithMeta{
                            GeometryEntity(line),  // entity
                            entity.layer,          // layer
                            entity.handle,         // handle (same for all segments)
                            entity.colorNumber,    // color
                            dxfEntity.lineNumber   // sourceLineNumber
                        };
                        result.entities.push_back(entityWithMeta);
                    }

                    result.totalConverted++;
                    std::cout << "  ✅ VALID - Spline approximated with " << lines.size() << " segments" << std::endl;
                } else {
                    std::cout << "  ❌ REJECTED - No valid segments" << std::endl;
                    result.errors.push_back(
                        createErrorMessage("SPLINE", "No valid line segments could be created",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }

                // Mark as processed (don't add to result below)
                layer.clear();
                handle.clear();
            }
            else if constexpr (std::is_same_v<T, DXFPoint>) {
                std::cout << "  Type: POINT" << std::endl;
                std::cout << "  Location: (" << entity.x << ", " << entity.y << ")" << std::endl;

                auto point = convertPoint(entity);
                if (point.has_value()) {
                    std::cout << "  ✅ VALID" << std::endl;
                    converted = GeometryEntity(point.value());
                    layer = entity.layer;
                    handle = entity.handle;
                    colorNumber = entity.colorNumber;
                } else {
                    std::cout << "  ❌ REJECTED - Invalid coordinates" << std::endl;
                    result.errors.push_back(
                        createErrorMessage("POINT", "Invalid coordinates",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }
            }
            else if constexpr (std::is_same_v<T, DXFSolid>) {
                std::cout << "  Type: SOLID" << std::endl;
                std::cout << "  Shape: " << (entity.isTriangle ? "Triangle" : "Quadrilateral") << std::endl;

                // Convert solid to line segments
                auto lines = convertSolid(entity);
                std::cout << "  Converted to " << lines.size() << " line segments" << std::endl;

                if (!lines.empty()) {
                    // Add all line segments individually
                    for (const auto& line : lines) {
                        GeometryEntityWithMetadata entityWithMeta{
                            GeometryEntity(line),  // entity
                            entity.layer,          // layer
                            entity.handle,         // handle (same for all segments)
                            entity.colorNumber,    // color
                            dxfEntity.lineNumber   // sourceLineNumber
                        };
                        result.entities.push_back(entityWithMeta);
                    }

                    result.totalConverted++;
                    std::cout << "  ✅ VALID - Solid converted to " << lines.size() << " segments" << std::endl;
                } else {
                    std::cout << "  ❌ REJECTED - No valid segments" << std::endl;
                    result.errors.push_back(
                        createErrorMessage("SOLID", "No valid line segments could be created",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }

                // Mark as processed (don't add to result below)
                layer.clear();
                handle.clear();
            }
            else if constexpr (std::is_same_v<T, DXFPolyline>) {
                // Handle legacy POLYLINE (similar to LWPOLYLINE)
                std::cout << "  Type: POLYLINE (legacy)" << std::endl;
                std::cout << "  Vertices: " << entity.vertices.size() << std::endl;
                std::cout << "  Closed: " << (entity.closed ? "yes" : "no") << std::endl;

                // Convert as LWPOLYLINE
                DXFLWPolyline lwPoly;
                lwPoly.vertices = entity.vertices;
                lwPoly.closed = entity.closed;
                lwPoly.layer = entity.layer;
                lwPoly.handle = entity.handle;
                lwPoly.colorNumber = entity.colorNumber;

                auto segments = convertPolyline(lwPoly);

                // Count segment types for logging
                int lineCount = 0, arcCount = 0;
                for (const auto& seg : segments) {
                    if (std::holds_alternative<Geometry::Line2D>(seg)) lineCount++;
                    else if (std::holds_alternative<Geometry::Arc2D>(seg)) arcCount++;
                }
                std::cout << "  Converted to " << segments.size() << " segments ("
                          << lineCount << " lines, " << arcCount << " arcs)" << std::endl;

                if (!segments.empty()) {
                    for (const auto& segment : segments) {
                        GeometryEntityWithMetadata entityWithMeta{
                            segment,
                            entity.layer,
                            entity.handle,
                            entity.colorNumber,
                            dxfEntity.lineNumber
                        };
                        result.entities.push_back(entityWithMeta);
                    }

                    result.totalConverted++;
                    std::cout << "  ✅ VALID - Polyline decomposed into " << segments.size() << " segments" << std::endl;
                } else {
                    std::cout << "  ❌ REJECTED - No valid segments" << std::endl;
                    result.errors.push_back(
                        createErrorMessage("POLYLINE", "No valid segments could be created",
                                         dxfEntity.lineNumber)
                    );
                    result.totalFailed++;
                }

                // Mark as processed
                layer.clear();
                handle.clear();
            }
        }, dxfEntity.data);

        // Add to result if conversion succeeded
        if (converted.has_value() && !layer.empty()) {
            GeometryEntityWithMetadata entityWithMeta{
                converted.value(),  // entity
                layer,              // layer
                handle,             // handle
                colorNumber,        // color
                dxfEntity.lineNumber // sourceLineNumber
            };

            result.entities.push_back(entityWithMeta);
            result.totalConverted++;
        }
    }

    std::cout << "\n=== Conversion Summary ===" << std::endl;
    std::cout << "  Total converted: " << result.totalConverted << std::endl;
    std::cout << "  Total failed: " << result.totalFailed << std::endl;
    std::cout << "  Errors: " << result.errors.size() << std::endl;

    if (!result.errors.empty()) {
        std::cout << "\n  Error details:" << std::endl;
        for (const auto& error : result.errors) {
            std::cout << "    - " << error << std::endl;
        }
    }
    std::cout << "========================\n" << std::endl;

    result.success = result.errors.empty();
    return result;
}

// ============================================================================
// INDIVIDUAL CONVERTERS
// ============================================================================

std::optional<Line2D> GeometryConverter::convertLine(const DXFLine& dxfLine) {
    // Validate coordinates
    if (!validateCoordinates(dxfLine.startX, dxfLine.startY) ||
        !validateCoordinates(dxfLine.endX, dxfLine.endY)) {
        return std::nullopt;
    }

    // Create points (Z coordinate ignored - 2D projection)
    Point2D start(dxfLine.startX, dxfLine.startY);
    Point2D end(dxfLine.endX, dxfLine.endY);

    // Use factory pattern - will reject zero-length lines
    return Line2D::create(start, end);
}

std::optional<Arc2D> GeometryConverter::convertArc(const DXFArc& dxfArc) {
    // Validate coordinates
    if (!validateCoordinates(dxfArc.centerX, dxfArc.centerY)) {
        return std::nullopt;
    }

    // Validate radius
    if (dxfArc.radius <= MIN_ARC_RADIUS || !std::isfinite(dxfArc.radius)) {
        return std::nullopt;
    }

    // Create center point (Z coordinate ignored)
    Point2D center(dxfArc.centerX, dxfArc.centerY);

    // Convert angles from degrees to radians
    double startAngleRad = degreesToRadians(dxfArc.startAngle);
    double endAngleRad = degreesToRadians(dxfArc.endAngle);

    // DXF arcs are always counter-clockwise
    // Use factory pattern - will reject degenerate arcs
    return Arc2D::create(center, dxfArc.radius, startAngleRad, endAngleRad, true);
}

std::optional<Arc2D> GeometryConverter::convertCircle(const DXFCircle& dxfCircle) {
    // Validate coordinates
    if (!validateCoordinates(dxfCircle.centerX, dxfCircle.centerY)) {
        return std::nullopt;
    }

    // Validate radius
    if (dxfCircle.radius <= MIN_ARC_RADIUS || !std::isfinite(dxfCircle.radius)) {
        return std::nullopt;
    }

    // Create center point (Z coordinate ignored)
    Point2D center(dxfCircle.centerX, dxfCircle.centerY);

    // Convert circle to full arc (0° to 360°)
    // Use factory pattern - will validate
    return Arc2D::create(center, dxfCircle.radius, 0.0, TWO_PI, true);
}

std::vector<GeometryEntity> GeometryConverter::convertPolyline(const DXFLWPolyline& polyline) {
    std::vector<GeometryEntity> segments;

    if (polyline.vertices.size() < 2) {
        return segments;  // Empty result
    }

    // Create segments between consecutive vertices
    // Each vertex's bulge applies to the segment FROM that vertex TO the next
    for (size_t i = 0; i < polyline.vertices.size() - 1; ++i) {
        const auto& v1 = polyline.vertices[i];
        const auto& v2 = polyline.vertices[i + 1];

        // Validate coordinates
        if (!validateCoordinates(v1.x, v1.y) || !validateCoordinates(v2.x, v2.y)) {
            continue;  // Skip invalid vertices
        }

        Point2D p1(v1.x, v1.y);
        Point2D p2(v2.x, v2.y);

        // Check bulge value - if non-zero, create arc instead of line
        if (std::abs(v1.bulge) > GEOMETRY_EPSILON) {
            // Arc segment
            auto arc = convertPolylineArcSegment(p1, p2, v1.bulge);
            if (arc.has_value()) {
                segments.push_back(GeometryEntity(arc.value()));
            } else {
                // Fallback to line if arc conversion fails
                auto line = Line2D::create(p1, p2);
                if (line.has_value()) {
                    segments.push_back(GeometryEntity(line.value()));
                }
            }
        } else {
            // Straight line segment
            auto line = Line2D::create(p1, p2);
            if (line.has_value()) {
                segments.push_back(GeometryEntity(line.value()));
            }
        }
    }

    // If closed, add closing segment from last to first vertex
    if (polyline.closed && polyline.vertices.size() >= 2) {
        const auto& vFirst = polyline.vertices.front();
        const auto& vLast = polyline.vertices.back();

        if (validateCoordinates(vFirst.x, vFirst.y) &&
            validateCoordinates(vLast.x, vLast.y)) {

            Point2D p1(vLast.x, vLast.y);
            Point2D p2(vFirst.x, vFirst.y);

            // Check bulge of last vertex for closing segment
            if (std::abs(vLast.bulge) > GEOMETRY_EPSILON) {
                auto arc = convertPolylineArcSegment(p1, p2, vLast.bulge);
                if (arc.has_value()) {
                    segments.push_back(GeometryEntity(arc.value()));
                } else {
                    auto line = Line2D::create(p1, p2);
                    if (line.has_value()) {
                        segments.push_back(GeometryEntity(line.value()));
                    }
                }
            } else {
                auto line = Line2D::create(p1, p2);
                if (line.has_value()) {
                    segments.push_back(GeometryEntity(line.value()));
                }
            }
        }
    }

    return segments;
}

std::optional<Ellipse2D> GeometryConverter::convertEllipse(const DXFEllipse& dxfEllipse) {
    // Validate center coordinates
    if (!validateCoordinates(dxfEllipse.centerX, dxfEllipse.centerY)) {
        return std::nullopt;
    }

    // Validate major axis endpoint (relative to center)
    if (!validateCoordinates(dxfEllipse.majorAxisX, dxfEllipse.majorAxisY)) {
        return std::nullopt;
    }

    // Create center point (Z coordinate ignored)
    Point2D center(dxfEllipse.centerX, dxfEllipse.centerY);

    // Calculate major axis endpoint (absolute position)
    Point2D majorAxisEnd(
        dxfEllipse.centerX + dxfEllipse.majorAxisX,
        dxfEllipse.centerY + dxfEllipse.majorAxisY
    );

    // Validate minor axis ratio
    if (dxfEllipse.minorAxisRatio <= 0.0 || dxfEllipse.minorAxisRatio > 1.0) {
        return std::nullopt;
    }

    // Use factory pattern - will validate all parameters
    return Ellipse2D::create(
        center,
        majorAxisEnd,
        dxfEllipse.minorAxisRatio,
        dxfEllipse.startParameter,  // Already in radians
        dxfEllipse.endParameter     // Already in radians
    );
}

std::vector<Line2D> GeometryConverter::convertSpline(const DXFSpline& dxfSpline) {
    std::vector<Line2D> lines;

    // For now, approximate spline by connecting control points
    // TODO: Implement proper B-spline evaluation
    if (dxfSpline.controlPoints.size() < 2) {
        return lines;  // Empty result
    }

    // Create line segments between consecutive control points
    for (size_t i = 0; i < dxfSpline.controlPoints.size() - 1; ++i) {
        const auto& v1 = dxfSpline.controlPoints[i];
        const auto& v2 = dxfSpline.controlPoints[i + 1];

        // Validate coordinates
        if (!validateCoordinates(v1.x, v1.y) || !validateCoordinates(v2.x, v2.y)) {
            continue;  // Skip invalid vertices
        }

        Point2D p1(v1.x, v1.y);
        Point2D p2(v2.x, v2.y);

        // Create line (factory will reject zero-length)
        auto line = Line2D::create(p1, p2);
        if (line.has_value()) {
            lines.push_back(line.value());
        }
    }

    // If closed, add closing segment
    if (dxfSpline.closed && dxfSpline.controlPoints.size() >= 2) {
        const auto& vFirst = dxfSpline.controlPoints.front();
        const auto& vLast = dxfSpline.controlPoints.back();

        if (validateCoordinates(vFirst.x, vFirst.y) &&
            validateCoordinates(vLast.x, vLast.y)) {

            Point2D p1(vLast.x, vLast.y);
            Point2D p2(vFirst.x, vFirst.y);

            auto line = Line2D::create(p1, p2);
            if (line.has_value()) {
                lines.push_back(line.value());
            }
        }
    }

    return lines;
}

std::optional<Point2D> GeometryConverter::convertPoint(const DXFPoint& dxfPoint) {
    // Validate coordinates
    if (!validateCoordinates(dxfPoint.x, dxfPoint.y)) {
        return std::nullopt;
    }

    // Create point (Z coordinate ignored)
    return Point2D(dxfPoint.x, dxfPoint.y);
}

std::vector<Line2D> GeometryConverter::convertSolid(const DXFSolid& dxfSolid) {
    std::vector<Line2D> lines;

    // Validate all corners
    if (!validateCoordinates(dxfSolid.x1, dxfSolid.y1) ||
        !validateCoordinates(dxfSolid.x2, dxfSolid.y2) ||
        !validateCoordinates(dxfSolid.x3, dxfSolid.y3)) {
        return lines;  // Invalid solid
    }

    Point2D p1(dxfSolid.x1, dxfSolid.y1);
    Point2D p2(dxfSolid.x2, dxfSolid.y2);
    Point2D p3(dxfSolid.x3, dxfSolid.y3);

    // Create edges of triangle/quad
    auto line1 = Line2D::create(p1, p2);
    auto line2 = Line2D::create(p2, p3);

    if (line1.has_value()) lines.push_back(line1.value());
    if (line2.has_value()) lines.push_back(line2.value());

    if (!dxfSolid.isTriangle) {
        // Quadrilateral - add fourth corner
        if (validateCoordinates(dxfSolid.x4, dxfSolid.y4)) {
            Point2D p4(dxfSolid.x4, dxfSolid.y4);

            auto line3 = Line2D::create(p3, p4);
            auto line4 = Line2D::create(p4, p1);

            if (line3.has_value()) lines.push_back(line3.value());
            if (line4.has_value()) lines.push_back(line4.value());
        }
    } else {
        // Triangle - close with line from p3 to p1
        auto line3 = Line2D::create(p3, p1);
        if (line3.has_value()) lines.push_back(line3.value());
    }

    return lines;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

double GeometryConverter::degreesToRadians(double degrees) noexcept {
    return degrees * (PI / 180.0);
}

double GeometryConverter::radiansToDegrees(double radians) noexcept {
    return radians * (180.0 / PI);
}

bool GeometryConverter::validateCoordinates(double x, double y) {
    return std::isfinite(x) && std::isfinite(y);
}

std::string GeometryConverter::createErrorMessage(
    const std::string& entityType,
    const std::string& reason,
    size_t lineNumber
) {
    std::ostringstream oss;
    oss << entityType << " at line " << lineNumber << ": " << reason;
    return oss.str();
}

std::optional<Geometry::Arc2D> GeometryConverter::convertPolylineArcSegment(
    const Geometry::Point2D& p1,
    const Geometry::Point2D& p2,
    double bulge
) {
    // Bulge = tan(included_angle / 4)
    // Positive bulge: arc curves to the left (CCW from p1 to p2)
    // Negative bulge: arc curves to the right (CW from p1 to p2)

    if (std::abs(bulge) < GEOMETRY_EPSILON) {
        // Zero bulge means straight line, not an arc
        return std::nullopt;
    }

    // Calculate chord length
    double chordLength = p1.distanceTo(p2);
    if (chordLength < GEOMETRY_EPSILON) {
        // Degenerate: start and end points are the same
        return std::nullopt;
    }

    // Calculate included angle from bulge
    // bulge = tan(θ/4), so θ = 4 * atan(|bulge|)
    double includedAngle = 4.0 * std::atan(std::abs(bulge));

    // Calculate radius using: chord = 2 * r * sin(θ/2)
    // Therefore: r = chord / (2 * sin(θ/2))
    double halfAngle = includedAngle / 2.0;
    double sinHalfAngle = std::sin(halfAngle);

    if (std::abs(sinHalfAngle) < GEOMETRY_EPSILON) {
        // Degenerate arc
        return std::nullopt;
    }

    double radius = chordLength / (2.0 * sinHalfAngle);

    if (radius < MIN_ARC_RADIUS) {
        return std::nullopt;
    }

    // Calculate midpoint of chord
    double midX = (p1.x() + p2.x()) / 2.0;
    double midY = (p1.y() + p2.y()) / 2.0;

    // Calculate unit vector along chord (from p1 to p2)
    double chordDirX = (p2.x() - p1.x()) / chordLength;
    double chordDirY = (p2.y() - p1.y()) / chordLength;

    // Perpendicular to chord (90° CCW rotation): (-dy, dx)
    double perpX = -chordDirY;
    double perpY = chordDirX;

    // Distance from chord midpoint to arc center
    // Using: apothem = r * cos(θ/2)
    // Distance from midpoint to center = apothem (for arcs <= 180°)
    // For arcs > 180°, center is on opposite side
    double apothem = radius * std::cos(halfAngle);

    // Determine center position based on bulge sign
    // Positive bulge: arc bulges in positive perpendicular direction (left of chord)
    //   For CCW arc from p1 to p2, center is to the right of chord
    // Negative bulge: arc bulges in negative perpendicular direction (right of chord)
    //   For CW arc from p1 to p2, center is to the left of chord

    double centerX, centerY;
    bool isCCW;

    if (bulge > 0) {
        // Positive bulge: CCW arc, center is to the right of chord direction
        // (perpendicular points left, so we go negative perpendicular)
        centerX = midX - perpX * apothem;
        centerY = midY - perpY * apothem;
        isCCW = true;
    } else {
        // Negative bulge: CW arc, center is to the left of chord direction
        // (perpendicular points left, so we go positive perpendicular)
        centerX = midX + perpX * apothem;
        centerY = midY + perpY * apothem;
        isCCW = false;
    }

    Geometry::Point2D center(centerX, centerY);

    // Calculate start and end angles (from center to p1 and p2)
    double startAngle = std::atan2(p1.y() - centerY, p1.x() - centerX);
    double endAngle = std::atan2(p2.y() - centerY, p2.x() - centerX);

    // Normalize angles to [0, 2π)
    if (startAngle < 0) startAngle += TWO_PI;
    if (endAngle < 0) endAngle += TWO_PI;

    // Create arc using factory
    return Geometry::Arc2D::create(center, radius, startAngle, endAngle, isCCW);
}

} // namespace Import
} // namespace OwnCAD
