#include "ui/DataToolsController.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPointer>
#include <QProgressDialog>
#include <QStandardPaths>
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "data/InterviewRepository.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "services/ReminderService.h"
#include "services/SeedService.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui {

namespace {

/// Production implementation of IFileChooser using standard Qt dialogs (desktop portal under Wayland).
class SystemFileChooser : public IFileChooser {
public:
    QString getSaveFileName(QWidget* parent,
                            const QString& title,
                            const QString& defaultPath,
                            const QString& filter) override {
        return QFileDialog::getSaveFileName(parent, title, defaultPath, filter);
    }

    QString getOpenFileName(QWidget* parent,
                            const QString& title,
                            const QString& defaultPath,
                            const QString& filter) override {
        return QFileDialog::getOpenFileName(parent, title, defaultPath, filter);
    }
};

/// Production implementation of IMessagePresenter using QMessageBox.
class SystemMessagePresenter : public IMessagePresenter {
public:
    void showInformation(QWidget* parent, const QString& title, const QString& text) override {
        QMessageBox::information(parent, title, text);
    }

    void showWarning(QWidget* parent, const QString& title, const QString& text) override {
        QMessageBox::warning(parent, title, text);
    }

    void showError(QWidget* parent, const QString& title, const QString& text) override {
        QMessageBox::critical(parent, title, text);
    }

    bool askConfirmation(QWidget* parent, const QString& title, const QString& text) override {
        return QMessageBox::question(parent, title, text, QMessageBox::Yes | QMessageBox::No) ==
               QMessageBox::Yes;
    }
};

/// Production implementation of IBusyIndicator using an indeterminate modal QProgressDialog.
class SystemBusyIndicator : public IBusyIndicator {
public:
    void showBusy(QWidget* parent, const QString& title, const QString& message) override {
        hideBusy();
        m_dlg = new QProgressDialog(message, QString(), 0, 0, parent);
        m_dlg->setWindowTitle(title);
        m_dlg->setWindowModality(Qt::WindowModal);
        m_dlg->setCancelButton(nullptr);
        m_dlg->setMinimumDuration(0);
        m_dlg->show();
        QCoreApplication::processEvents();
    }

    void hideBusy() override {
        if (m_dlg) {
            m_dlg->close();
            m_dlg->deleteLater();
            m_dlg = nullptr;
        }
    }

private:
    QPointer<QProgressDialog> m_dlg;
};

}  // namespace

DataToolsController::DataToolsController(Data::Database& database,
                                         Services::ExportService& exportService,
                                         Services::RestoreService& restoreService,
                                         Services::SeedService& seedService,
                                         Data::TrackRepository& tracks,
                                         Data::TopicRepository& topics,
                                         Data::SessionRepository& sessions,
                                         Data::ApplicationRepository& applications,
                                         Data::InterviewRepository& interviews,
                                         Services::ReminderService* reminderService,
                                         QObject* parent)
    : QObject(parent),
      m_database(database),
      m_exportService(exportService),
      m_restoreService(restoreService),
      m_seedService(seedService),
      m_tracks(tracks),
      m_topics(topics),
      m_sessions(sessions),
      m_applications(applications),
      m_interviews(interviews),
      m_reminderService(reminderService),
      m_fileChooser(std::make_unique<SystemFileChooser>()),
      m_presenter(std::make_unique<SystemMessagePresenter>()),
      m_busyIndicator(std::make_unique<SystemBusyIndicator>()) {}

DataToolsController::~DataToolsController() = default;

void DataToolsController::setFileChooser(std::unique_ptr<IFileChooser> chooser) {
    if (chooser) m_fileChooser = std::move(chooser);
}

void DataToolsController::setMessagePresenter(std::unique_ptr<IMessagePresenter> presenter) {
    if (presenter) m_presenter = std::move(presenter);
}

void DataToolsController::setBusyIndicator(std::unique_ptr<IBusyIndicator> busy) {
    if (busy) m_busyIndicator = std::move(busy);
}

void DataToolsController::setUrlOpener(UrlOpener opener) {
    m_urlOpener = std::move(opener);
}

void DataToolsController::setClock(Clock clock) {
    m_clock = std::move(clock);
}

void DataToolsController::setFlushCallback(FlushCallback callback) {
    m_flushCallback = std::move(callback);
}

void DataToolsController::setIsEmptyCallback(IsEmptyCallback callback) {
    m_isEmptyCallback = std::move(callback);
}

void DataToolsController::setReminderService(Services::ReminderService* reminderService) {
    m_reminderService = reminderService;
}

QDateTime DataToolsController::now() const {
    return m_clock ? m_clock() : QDateTime::currentDateTime();
}

QDate DataToolsController::today() const {
    return now().date();
}

QString DataToolsController::formatRestoreError(Services::RestoreErrorKind kind) {
    switch (kind) {
        case Services::RestoreErrorKind::None:
            return tr("No error.");
        case Services::RestoreErrorKind::CannotOpenFile:
            return tr("Could not open the selected backup file.");
        case Services::RestoreErrorKind::FileTooLarge:
            return tr("The backup file exceeds the maximum allowed size (64 MB).");
        case Services::RestoreErrorKind::EmptyFile:
            return tr("The backup file is empty.");
        case Services::RestoreErrorKind::InvalidJson:
            return tr("The file is not a valid JSON document.");
        case Services::RestoreErrorKind::InvalidFormat:
            return tr("The file is not a valid JobPrep backup (format mismatch).");
        case Services::RestoreErrorKind::UnsupportedVersion:
            return tr("The backup version is not supported by this version of JobPrep.");
        case Services::RestoreErrorKind::DuplicateId:
            return tr("The backup file contains duplicate item identifiers.");
        case Services::RestoreErrorKind::InvalidId:
            return tr("The backup file contains invalid non-positive identifiers.");
        case Services::RestoreErrorKind::InvalidEnumValue:
            return tr("The backup file contains unrecognized status or priority values.");
        case Services::RestoreErrorKind::InvalidDateFormat:
            return tr("The backup file contains invalid date or time format strings.");
        case Services::RestoreErrorKind::BrokenReference:
            return tr("The backup file contains broken cross-table references.");
        case Services::RestoreErrorKind::MissingRequiredField:
            return tr("The backup file is missing required fields or table entries.");
        case Services::RestoreErrorKind::SafetyBackupFailed:
            return tr("Could not create the safety pre-restore backup. The restore was aborted.");
        case Services::RestoreErrorKind::DatabaseError:
            return tr("A database error occurred while replacing tables.");
    }
    return tr("An unknown restore error occurred.");
}

void DataToolsController::exportCsv(QWidget* parent) {
    const QString dateStr = today().toString(u"yyyy-MM-dd"_s);
    const QString defaultName = u"jobprep-applications-%1.csv"_s.arg(dateStr);
    const QString filter = tr("CSV Files (*.csv);;All Files (*)");

    const QString path =
        m_fileChooser->getSaveFileName(parent, tr("Export Applications (CSV)"), defaultName, filter);
    if (path.isEmpty()) return;

    const Services::ExportResult res = m_exportService.exportApplicationsCsv(m_applications.all(), path);
    if (res.success) {
        m_presenter->showInformation(
            parent, tr("Export Successful"),
            tr("Applications successfully exported to:\n%1").arg(QDir::toNativeSeparators(path)));
    } else {
        m_presenter->showError(parent, tr("Export Failed"),
                               tr("Could not export applications:\n%1").arg(res.errorDetail));
    }
}

void DataToolsController::exportIcs(QWidget* parent) {
    const QString dateStr = today().toString(u"yyyy-MM-dd"_s);
    const QString defaultName = u"jobprep-interviews-%1.ics"_s.arg(dateStr);
    const QString filter = tr("iCalendar Files (*.ics);;All Files (*)");

    const QString path =
        m_fileChooser->getSaveFileName(parent, tr("Export Interviews (ICS)"), defaultName, filter);
    if (path.isEmpty()) return;

    const Services::ExportResult res =
        m_exportService.exportInterviewsToIcs(m_database, path, m_clock);
    if (res.success) {
        m_presenter->showInformation(
            parent, tr("Export Successful"),
            tr("Interviews successfully exported to:\n%1").arg(QDir::toNativeSeparators(path)));
    } else {
        m_presenter->showError(parent, tr("Export Failed"),
                               tr("Could not export interviews:\n%1").arg(res.errorDetail));
    }
}

void DataToolsController::backupJson(QWidget* parent) {
    const QString dateStr = today().toString(u"yyyy-MM-dd"_s);
    const QString defaultName = u"jobprep-backup-%1.json"_s.arg(dateStr);
    const QString filter = tr("JSON Backup Files (*.json);;All Files (*)");

    const QString path =
        m_fileChooser->getSaveFileName(parent, tr("Backup Database (JSON)"), defaultName, filter);
    if (path.isEmpty()) return;

    const Services::ExportResult res =
        m_exportService.exportBackupJson(m_database, path, m_clock);
    if (res.success) {
        m_presenter->showInformation(
            parent, tr("Backup Successful"),
            tr("Database backup successfully created at:\n%1").arg(QDir::toNativeSeparators(path)));
    } else {
        m_presenter->showError(parent, tr("Backup Failed"),
                               tr("Could not create database backup:\n%1").arg(res.errorDetail));
    }
}

bool DataToolsController::isRestoring() const {
    return m_restoreService.isRestoring();
}

void DataToolsController::restoreJson(QWidget* parent) {
    if (isRestoring()) {
        m_presenter->showError(parent, tr("Restore in Progress"),
                               tr("A database restore is already in progress."));
        return;
    }
    const QString filter = tr("JSON Backup Files (*.json);;All Files (*)");
    const QString path =
        m_fileChooser->getOpenFileName(parent, tr("Restore Database from Backup"), QString(), filter);
    if (path.isEmpty()) return;

    const QString safetyDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/backups"_s;

    const QString confirmMsg =
        tr("Restoring from '%1' will REPLACE all current data.\n\n"
           "A safety backup of your current database will be saved to:\n%2\n\n"
           "Do you want to continue?")
            .arg(QFileInfo(path).fileName(), QDir::toNativeSeparators(safetyDir));

    if (!m_presenter->askConfirmation(parent, tr("Confirm Database Restore"), confirmMsg)) {
        return;
    }

    if (m_flushCallback) {
        m_flushCallback();
    }

    m_busyIndicator->showBusy(parent, tr("Restoring Database"),
                              tr("Restoring database from backup file. Please wait..."));

    // Connect one-shot handler to restoreService finished signal
    auto* conn = new QMetaObject::Connection();
    *conn = connect(&m_restoreService, &Services::RestoreService::finished, this,
                    [this, parent, conn](const Services::RestoreResult& res) {
                        disconnect(*conn);
                        delete conn;

                        m_busyIndicator->hideBusy();

                        if (res.success) {
                            // Notify all repositories so item models and views refresh cleanly
                            m_tracks.notifyChanged();
                            m_topics.notifyChanged();
                            m_sessions.notifyChanged();
                            m_applications.notifyChanged();
                            m_interviews.notifyChanged();

                            emit dataRestored();

                            const QString msg =
                                tr("Database restore completed successfully.\n\n"
                                   "Restored records:\n"
                                   "- %1 tracks\n"
                                   "- %2 topics\n"
                                   "- %3 subtasks\n"
                                   "- %4 study sessions\n"
                                   "- %5 applications\n"
                                   "- %6 status history entries\n"
                                   "- %7 interviews\n\n"
                                   "Safety backup saved to:\n%8")
                                    .arg(res.restoredTracks)
                                    .arg(res.restoredTopics)
                                    .arg(res.restoredSubtasks)
                                    .arg(res.restoredSessions)
                                    .arg(res.restoredApplications)
                                    .arg(res.restoredStatusHistory)
                                    .arg(res.restoredInterviews)
                                    .arg(QDir::toNativeSeparators(res.safetyBackupPath));

                            m_presenter->showInformation(parent, tr("Restore Successful"), msg);
                        } else {
                            const QString errorSummary = formatRestoreError(res.errorKind);
                            const QString detail = res.errorDetail.isEmpty()
                                                       ? QString()
                                                       : u"\n\nDetails: %1"_s.arg(res.errorDetail);

                            m_presenter->showError(
                                parent, tr("Restore Failed"),
                                tr("Could not restore database:\n%1%2\n\n"
                                   "Your current database was NOT modified.")
                                    .arg(errorSummary, detail));
                        }

                        emit restoreFinished(res.success);
                    });

    m_restoreService.restoreAsync(m_database.path(), path, safetyDir, m_reminderService, m_clock);
}

void DataToolsController::loadSampleData(QWidget* parent) {
    const bool hasData = m_isEmptyCallback ? !m_isEmptyCallback()
                                           : (!m_tracks.isEmpty() || !m_topics.isEmpty() ||
                                              !m_applications.isEmpty());
    if (hasData) {
        const QString confirmMsg =
            tr("Your database already contains data. Loading sample data will add standard study "
               "tracks and topics without deleting your existing items.\n\n"
               "Do you want to continue?");
        if (!m_presenter->askConfirmation(parent, tr("Confirm Load Sample Data"), confirmMsg)) {
            return;
        }
    }

    QString error;
    if (m_seedService.loadSampleTopics(&error)) {
        m_tracks.notifyChanged();
        m_topics.notifyChanged();
        m_presenter->showInformation(
            parent, tr("Sample Data Loaded"),
            tr("Sample study tracks and topics were successfully loaded."));
    } else {
        m_presenter->showError(parent, tr("Load Sample Data Failed"),
                               tr("Could not load sample data: %1").arg(error));
    }
}

void DataToolsController::openDataFolder(QWidget* parent) {
    const QString folder = (m_database.path() == u":memory:"_s || m_database.path().isEmpty())
                               ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                               : QFileInfo(m_database.path()).absolutePath();

    QDir().mkpath(folder);
    const QUrl url = QUrl::fromLocalFile(folder);
    const bool ok = m_urlOpener ? m_urlOpener(url) : QDesktopServices::openUrl(url);

    if (!ok) {
        m_presenter->showInformation(
            parent, tr("Data Folder"),
            tr("Could not open the data folder in file manager.\n\nPath: %1")
                .arg(QDir::toNativeSeparators(folder)));
    }
}

}  // namespace JobPrep::Ui
