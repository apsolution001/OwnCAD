#pragma once

#include <QColor>

namespace OwnCAD {
namespace Import {

/**
 * @brief DXF color index to RGB converter
 *
 * AutoCAD Color Index (ACI) standard colors (1-255)
 * Special codes:
 * - 0: BYBLOCK
 * - 256: BYLAYER
 * - 257: BYENTITY
 */
class DXFColors {
public:
    /**
     * @brief Convert DXF color number to QColor
     * @param colorNumber DXF color code (0-257)
     * @param defaultColor Fallback color for BYLAYER/BYBLOCK
     * @return QColor RGB value
     */
    static QColor toQColor(int colorNumber, const QColor& defaultColor = QColor(0, 0, 0));

private:
    /**
     * @brief Standard AutoCAD Color Index (ACI) colors
     * First 10 colors are the most commonly used
     */
    static const QColor ACI_COLORS[256];
};

} // namespace Import
} // namespace OwnCAD
