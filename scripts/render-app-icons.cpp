#include <QGuiApplication>
#include <QPainter>
#include <QImage>
#include <QSvgRenderer>
#include <QFileInfo>
#include <QDir>
#include <cstdio>

int main(int argc, char* argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    const QString svgPath = QStringLiteral("resources/icons/app-color.svg");
    if (!QFileInfo::exists(svgPath)) {
        fprintf(stderr, "Error: SVG file not found at %s\n", qPrintable(svgPath));
        return 1;
    }

    QSvgRenderer renderer(svgPath);
    if (!renderer.isValid()) {
        fprintf(stderr, "Error: invalid SVG at %s\n", qPrintable(svgPath));
        return 1;
    }

    const int sizes[] = {16, 24, 32, 48, 64, 128, 256};
    for (int size : sizes) {
        QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&painter);
        painter.end();

        const QString outPath = QStringLiteral("resources/icons/app-%1.png").arg(size);
        if (!image.save(outPath, "PNG")) {
            fprintf(stderr, "Error: failed to save %s\n", qPrintable(outPath));
            return 1;
        }
        printf("Rendered %s (%dx%d)\n", qPrintable(outPath), size, size);
    }

    return 0;
}
