#pragma once

#include <functional>
#include <QMainWindow>
#include <QList>

class QLabel;
class QLineEdit;
class QMenu;
class QStackedWidget;
class QToolButton;

namespace JobPrep::App {
class AppContext;
}

namespace JobPrep::Services {
struct Notification;
class ReminderService;
}

namespace JobPrep::Ui {
class Sidebar;
class PageBase;

namespace Theme {
class ThemeManager;
}

namespace Pages {
class CalendarPage;
class HomePage;
class JobsPage;
class SettingsPage;
class StudyPage;
}  // namespace Pages

class DashboardController;
class DataToolsController;

/// Main application window shell for JobPrep.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(App::AppContext& ctx, bool trayAvailable = false, QWidget* parent = nullptr);
    ~MainWindow() override;

    Sidebar* sidebar() const { return m_sidebar; }
    QStackedWidget* pageStack() const { return m_pageStack; }
    DataToolsController* dataToolsController() const { return m_dataToolsController.get(); }

    /// Sets system tray availability learned once at startup (SPEC §8, B3).
    void setTrayAvailable(bool available);
    /// Returns whether the system tray is available.
    bool isTrayAvailable() const { return m_trayAvailable; }

    /// Sets a live callback to check whether system tray is active and ready (C0.1).
    void setLiveTrayCheck(std::function<bool()> check);
    /// Live check for tray availability and visibility.
    bool isTrayAvailableLive() const;

    /// Shuts down the application running the same path as window close (B3).
    void quitApplication();
    /// Returns whether application shutdown is currently in progress.
    bool isQuitting() const { return m_isQuitting; }

    /// Unified shutdown path: flushes page edits and saves geometry/state (C0.1).
    void prepareForShutdown();

    /// Injects the active ReminderService for coordination during database restore.
    void setReminderService(Services::ReminderService* service);

    /// Registers a callback to display messages in the system tray.
    void setTrayMessageCallback(std::function<void(const QString& title, const QString& message)> callback);

    /// Switches to the Study page and selects `topicId` (B9).
    void openTopic(int topicId);

    /// Switches to the Calendar page and opens the interview editor (B2).
    void openInterview(int interviewId);

    /// Switches to the Jobs page and opens the application editor (B2).
    void openApplication(int applicationId);

    /// Returns the application theme manager.
    Theme::ThemeManager& themeManager() const;

    /// Brings the application window to front (unminimizes, raises, activates).
    void bringToFront();

    /// Routes notification activation to the appropriate page/item.
    void routeNotification(const Services::Notification& notification);

    /// Activates window and opens the notification's target item.
    void openItemFromNotification(const Services::Notification& notification);

    Pages::HomePage* homePage() const { return m_homePage; }
    Pages::SettingsPage* settingsPage() const { return m_settingsPage; }
    DashboardController* dashboardController() const { return m_dashboardController.get(); }

    /// Focuses the search box of the page that is actually showing (Ctrl+F).
    void focusCurrentPageSearch();
    /// Runs the primary action of the page that is actually showing (Ctrl+N).
    void triggerCurrentPageNew();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    QMenu* setupNewMenu(QWidget* parent);
    void setupShortcuts();
    void setupConnections();
    void updateHeader(int pageIndex);
    void updateIcons();
    /// Header "+ New -> Application": switch to Jobs and open the add-application dialog.
    void newApplicationFromHeader();
    /// Header "+ New -> Interview": switch to Calendar and open a dated interview dialog.
    void newInterviewFromHeader();
    /// Header "+ New -> Topic": switch to Study and focus Backlog quick-add (or TrackDialog).
    void newTopicFromHeader();
    /// Header "+ New -> Study session": open SessionDialog from any page.
    void newSessionFromHeader();
    void goToPage(int pageIndex);

    App::AppContext& m_ctx;
    Sidebar* m_sidebar{nullptr};
    QLabel* m_pageTitleLabel{nullptr};
    QLineEdit* m_searchEdit{nullptr};
    QAction* m_searchAction{nullptr};
    QToolButton* m_newBtn{nullptr};
    QStackedWidget* m_pageStack{nullptr};
    QList<PageBase*> m_pages;
    std::unique_ptr<DashboardController> m_dashboardController;
    std::unique_ptr<DataToolsController> m_dataToolsController;
    Pages::HomePage* m_homePage{nullptr};
    int m_homeIndex{-1};
    /// The header search and the "+ New" menu are bound to this page, hence the index.
    Pages::JobsPage* m_jobsPage{nullptr};
    int m_jobsIndex{-1};
    Pages::StudyPage* m_studyPage{nullptr};
    int m_studyIndex{-1};
    Pages::CalendarPage* m_calendarPage{nullptr};
    int m_calendarIndex{-1};
    Pages::SettingsPage* m_settingsPage{nullptr};
    int m_settingsIndex{-1};

    bool m_trayAvailable{false};
    bool m_isQuitting{false};
    std::function<bool()> m_liveTrayCheck;
    std::function<void(const QString& title, const QString& message)> m_trayMessageCallback;

    void flushStateAndEdits();
    void updateQuitOnLastWindowClosed();

    QLabel* m_statusStreakLabel{nullptr};
    QLabel* m_statusProcessesLabel{nullptr};
    QLabel* m_statusNextInterviewLabel{nullptr};
};

}  // namespace JobPrep::Ui
