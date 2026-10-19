#pragma once

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

namespace JobPrep::Ui {
class Sidebar;
class PageBase;

namespace Pages {
class CalendarPage;
class JobsPage;
class StudyPage;
}  // namespace Pages

/// Main application window shell for JobPrep.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(App::AppContext& ctx, QWidget* parent = nullptr);
    ~MainWindow() override;

    Sidebar* sidebar() const { return m_sidebar; }
    QStackedWidget* pageStack() const { return m_pageStack; }

    /// Switches to the Study page and selects `topicId` (B9).
    void openTopic(int topicId);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    QMenu* setupNewMenu(QWidget* parent);
    void setupShortcuts();
    void setupConnections();
    void updateHeader(int pageIndex);
    void updateIcons();
    /// Focuses the search box of the page that is actually showing.
    void focusCurrentPageSearch();
    /// Runs the primary action of the page that is actually showing.
    void triggerCurrentPageNew();
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
    /// The header search and the "+ New" menu are bound to this page, hence the index.
    Pages::JobsPage* m_jobsPage{nullptr};
    int m_jobsIndex{-1};
    Pages::StudyPage* m_studyPage{nullptr};
    int m_studyIndex{-1};
    Pages::CalendarPage* m_calendarPage{nullptr};
    int m_calendarIndex{-1};

    QLabel* m_statusStreakLabel{nullptr};
    QLabel* m_statusProcessesLabel{nullptr};
    QLabel* m_statusNextInterviewLabel{nullptr};
};

}  // namespace JobPrep::Ui
