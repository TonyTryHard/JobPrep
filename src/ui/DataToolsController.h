#pragma once

#include <functional>
#include <memory>
#include <QDate>
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QUrl>
#include "services/ExportService.h"
#include "services/RestoreService.h"

class QWidget;

namespace JobPrep::Data {
class ApplicationRepository;
class Database;
class InterviewRepository;
class SessionRepository;
class TopicRepository;
class TrackRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Services {
class ReminderService;
class SeedService;
}  // namespace JobPrep::Services

namespace JobPrep::Ui {

/// Test seam interface for file open / save dialogs (SPEC §10, Amendment C8).
class IFileChooser {
public:
    virtual ~IFileChooser() = default;
    virtual QString getSaveFileName(QWidget* parent,
                                    const QString& title,
                                    const QString& defaultPath,
                                    const QString& filter) = 0;
    virtual QString getOpenFileName(QWidget* parent,
                                    const QString& title,
                                    const QString& defaultPath,
                                    const QString& filter) = 0;
};

/// Test seam interface for message boxes and user confirmation dialogs (SPEC §10, Amendment C8).
class IMessagePresenter {
public:
    virtual ~IMessagePresenter() = default;
    virtual void showInformation(QWidget* parent, const QString& title, const QString& text) = 0;
    virtual void showWarning(QWidget* parent, const QString& title, const QString& text) = 0;
    virtual void showError(QWidget* parent, const QString& title, const QString& text) = 0;
    virtual bool askConfirmation(QWidget* parent, const QString& title, const QString& text) = 0;
};

/// Test seam interface for modal busy indication during lengthy transactions (Amendment C8).
class IBusyIndicator {
public:
    virtual ~IBusyIndicator() = default;
    virtual void showBusy(QWidget* parent, const QString& title, const QString& message) = 0;
    virtual void hideBusy() = 0;
};

/// Orchestrates export, backup, restore, sample data loading, and folder opening
/// behind test seams without bloating SettingsPage (SPEC §10, Amendments C1–C8).
class DataToolsController : public QObject {
    Q_OBJECT

public:
    using Clock = std::function<QDateTime()>;
    using UrlOpener = std::function<bool(const QUrl&)>;
    using FlushCallback = std::function<void()>;
    using IsEmptyCallback = std::function<bool()>;

    DataToolsController(Data::Database& database,
                        Services::ExportService& exportService,
                        Services::RestoreService& restoreService,
                        Services::SeedService& seedService,
                        Data::TrackRepository& tracks,
                        Data::TopicRepository& topics,
                        Data::SessionRepository& sessions,
                        Data::ApplicationRepository& applications,
                        Data::InterviewRepository& interviews,
                        Services::ReminderService* reminderService = nullptr,
                        QObject* parent = nullptr);
    ~DataToolsController() override;

    // Seam injection
    void setFileChooser(std::unique_ptr<IFileChooser> chooser);
    void setMessagePresenter(std::unique_ptr<IMessagePresenter> presenter);
    void setBusyIndicator(std::unique_ptr<IBusyIndicator> busy);
    void setUrlOpener(UrlOpener opener);
    void setClock(Clock clock);
    void setFlushCallback(FlushCallback callback);
    void setIsEmptyCallback(IsEmptyCallback callback);
    void setReminderService(Services::ReminderService* reminderService);

    // Operations (C8)
    bool isRestoring() const;
    void exportCsv(QWidget* parent = nullptr);
    void exportIcs(QWidget* parent = nullptr);
    void backupJson(QWidget* parent = nullptr);
    void restoreJson(QWidget* parent = nullptr);
    void loadSampleData(QWidget* parent = nullptr);
    void openDataFolder(QWidget* parent = nullptr);

    /// Translates structured RestoreErrorKind into a user-facing explanation.
    static QString formatRestoreError(Services::RestoreErrorKind kind);

signals:
    void dataRestored();
    void restoreFinished(bool success);

private:
    QDateTime now() const;
    QDate today() const;

    Data::Database& m_database;
    Services::ExportService& m_exportService;
    Services::RestoreService& m_restoreService;
    Services::SeedService& m_seedService;
    Data::TrackRepository& m_tracks;
    Data::TopicRepository& m_topics;
    Data::SessionRepository& m_sessions;
    Data::ApplicationRepository& m_applications;
    Data::InterviewRepository& m_interviews;
    Services::ReminderService* m_reminderService{nullptr};

    std::unique_ptr<IFileChooser> m_fileChooser;
    std::unique_ptr<IMessagePresenter> m_presenter;
    std::unique_ptr<IBusyIndicator> m_busyIndicator;
    UrlOpener m_urlOpener;
    Clock m_clock;
    FlushCallback m_flushCallback;
    IsEmptyCallback m_isEmptyCallback;
};

}  // namespace JobPrep::Ui
