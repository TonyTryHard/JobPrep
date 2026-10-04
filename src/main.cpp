#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QMessageBox>
#include "app/AppContext.h"
#include "data/Database.h"
#include "services/SeedService.h"
#include "services/SettingsService.h"
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

    if (!ctx.database().isOpen()) {
        QMessageBox::critical(
            nullptr, QObject::tr("Cannot open the database"),
            QObject::tr("JobPrep could not open its data file.\n\n%1").arg(ctx.database().lastError()));
        return 1;
    }

    // First launch: offer the sample topics (SPEC §3.7).
    if (!ctx.settings().sampleDataPrompted() && ctx.isDataEmpty()) {
        const auto answer = QMessageBox::question(
            nullptr, QObject::tr("Load sample data?"),
            QObject::tr("Would you like to start with a sample study plan?\n\n"
                        "It adds the C++ and English tracks with their topics and checklists."),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (answer == QMessageBox::Yes) {
            QString error;
            if (!ctx.seedService().loadSampleTopics(&error)) {
                QMessageBox::warning(nullptr, QObject::tr("Sample data"),
                                     QObject::tr("The sample data could not be loaded.\n\n%1")
                                         .arg(error));
            }
        }
        ctx.settings().setSampleDataPrompted(true);
    }

    // Create and show main window
    JobPrep::Ui::MainWindow mainWindow(ctx);
    mainWindow.show();

    return app.exec();
}
