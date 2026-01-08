#include "import/DXFColors.h"

namespace OwnCAD {
namespace Import {

// AutoCAD Color Index (ACI) - Standard 256 colors
// Source: AutoCAD DXF Reference
const QColor DXFColors::ACI_COLORS[256] = {
    QColor(0, 0, 0),         // 0 - BYBLOCK (black)
    QColor(255, 0, 0),       // 1 - Red
    QColor(255, 255, 0),     // 2 - Yellow
    QColor(0, 255, 0),       // 3 - Green
    QColor(0, 255, 255),     // 4 - Cyan
    QColor(0, 0, 255),       // 5 - Blue
    QColor(255, 0, 255),     // 6 - Magenta
    QColor(255, 255, 255),   // 7 - White/Black (context-dependent)
    QColor(128, 128, 128),   // 8 - Gray
    QColor(192, 192, 192),   // 9 - Light Gray

    // Colors 10-249: Various shades (simplified mapping)
    // For full accuracy, these should be the exact ACI RGB values
    // Here we provide reasonable defaults
    QColor(255, 0, 0), QColor(255, 127, 127), QColor(165, 0, 0), QColor(165, 82, 82), QColor(127, 0, 0),  // 10-14 Red shades
    QColor(127, 63, 63), QColor(76, 0, 0), QColor(76, 38, 38), QColor(38, 0, 0), QColor(38, 19, 19),      // 15-19
    QColor(255, 63, 0), QColor(255, 159, 127), QColor(165, 41, 0), QColor(165, 103, 82), QColor(127, 31, 0), // 20-24

    // Colors 25-255: More complex gradients - using grayscale for simplicity
    QColor(100, 100, 100), QColor(110, 110, 110), QColor(120, 120, 120), QColor(130, 130, 130), QColor(140, 140, 140), // 25-29
    // ... continue pattern ...
    // (Full 256 colors would be very long - using defaults for now)
};

QColor DXFColors::toQColor(int colorNumber, const QColor& defaultColor) {
    // Handle special codes
    if (colorNumber == 0 || colorNumber == 256 || colorNumber == 257) {
        // BYBLOCK, BYLAYER, BYENTITY - use default
        return defaultColor;
    }

    // Clamp to valid range
    if (colorNumber < 1 || colorNumber > 255) {
        return defaultColor;
    }

    // Return ACI color (index is color number)
    if (colorNumber < 10) {
        return ACI_COLORS[colorNumber];
    }

    // For colors 10-255, use a simplified mapping
    // In production, you'd want the full ACI table
    // For now, generate reasonable colors
    if (colorNumber < 256) {
        // Generate color based on index
        int r = (colorNumber * 37) % 256;
        int g = (colorNumber * 79) % 256;
        int b = (colorNumber * 113) % 256;
        return QColor(r, g, b);
    }

    return defaultColor;
}

} // namespace Import
} // namespace OwnCAD
