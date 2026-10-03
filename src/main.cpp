#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include "app/AppContext.h"
#include "ui/MainWindow.h"

using namespace Qt::StringLiterals;

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Organization and application name must be set early for QSettings and QStandardPaths
    QCoreApplication::setOrganizationName(u"JobPrep"_s);
    QCoreApplication::setOrganizationDomain(u"jobprep.local"_s);
    QCoreApplication::setApplicationName(u"JobPrep"_s);

    // Set base font (10pt, fallback to system UI font)
    QFont baseFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    baseFont.setPointSize(10);
    app.setFont(baseFont);

    // Build composition root
    JobPrep::App::AppContext ctx;

    // Create and show main window
    JobPrep::Ui::MainWindow mainWindow(ctx);
    mainWindow.show();

    return app.exec();
}
