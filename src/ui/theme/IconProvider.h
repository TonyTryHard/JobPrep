#pragma once

#include <QColor>
#include <QIcon>
#include <QMap>
#include <QPair>
#include <QString>

namespace JobPrep::Ui::Theme {

/// Provides tinted QIcon instances from monochrome SVGs in resources/icons/.
class IconProvider {
public:
    static QIcon icon(const QString& name, const QColor& color);
    static void clearCache();

private:
    static QPixmap renderSvg(const QString& svgContent, const QSize& size, qreal dpr);
    static QMap<QPair<QString, QRgb>, QIcon> s_cache;
};

}  // namespace JobPrep::Ui::Theme
