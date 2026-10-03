#include "ui/theme/IconProvider.h"
#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QSvgRenderer>

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Theme {

QMap<QPair<QString, QRgb>, QIcon> IconProvider::s_cache;

QIcon IconProvider::icon(const QString& name, const QColor& color) {
    const auto key = qMakePair(name, color.rgba());
    auto it = s_cache.constFind(key);
    if (it != s_cache.constEnd()) {
        return it.value();
    }

    const QString path = u":/icons/"_s + name + u".svg"_s;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QIcon();
    }

    QString svgContent = QString::fromUtf8(file.readAll());
    file.close();

    const QString colorHex = color.name(QColor::HexRgb);
    svgContent.replace(u"currentColor"_s, colorHex);

    QIcon result;
    const QList<int> sizes = {16, 20, 24, 32, 48};
    const qreal dpr = QGuiApplication::primaryScreen()
                          ? QGuiApplication::primaryScreen()->devicePixelRatio()
                          : 1.0;

    for (int s : sizes) {
        QPixmap pixmap = renderSvg(svgContent, QSize(s, s), dpr);
        result.addPixmap(pixmap);
    }

    s_cache.insert(key, result);
    return result;
}

void IconProvider::clearCache() {
    s_cache.clear();
}

QPixmap IconProvider::renderSvg(const QString& svgContent, const QSize& size, qreal dpr) {
    const QByteArray utf8 = svgContent.toUtf8();
    QSvgRenderer renderer(utf8);

    const int physicalW = qRound(size.width() * dpr);
    const int physicalH = qRound(size.height() * dpr);

    QPixmap pixmap(physicalW, physicalH);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();

    pixmap.setDevicePixelRatio(dpr);
    return pixmap;
}

}  // namespace JobPrep::Ui::Theme
