#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QImage>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QSqlQuery>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextEdit>
#include <QTimeEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/AppContext.h"
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "data/InterviewRepository.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Structs.h"
#include "services/SeedService.h"
#include "ui/MainWindow.h"
#include "ui/Sidebar.h"
#include "ui/dialogs/ApplicationDialog.h"
#include "ui/dialogs/FollowUpDialog.h"
#include "ui/dialogs/InterviewDialog.h"
#include "ui/dialogs/SessionDialog.h"
#include "ui/dialogs/TrackDialog.h"
#include "ui/pages/CalendarPage.h"
#include "ui/pages/HomePage.h"
#include "ui/pages/JobsPage.h"
#include "ui/pages/SettingsPage.h"
#include "ui/pages/StudyPage.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/ChipBar.h"
#include "ui/widgets/EmptyState.h"
#include "ui/widgets/FunnelWidget.h"
#include "ui/widgets/MonthView.h"
#include "ui/widgets/ProgressRing.h"
#include "ui/widgets/TopicBoardColumn.h"
#include "ui/widgets/TopicDetailPanel.h"
#include "UiTestHelpers.h"

using namespace Qt::StringLiterals;
using namespace JobPrep::UiTest;

class tst_EmptyStatesAndFocus : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // D2 Empty states inventory
    void testEmptyDashboardAsAWhole();
    void testEmptyDashboardTodayAndUpcoming();
    void testEmptyDashboardFunnelAndStudyProgress();
    void testEmptyStudyPageNoTracks();
    void testEmptyStudyPageNoTopicsInColumnAndList();
    void testEmptyStudyPageFilteredEmpty();
    void testEmptyTopicChecklistAndResources();
    void testEmptyJobsPageNoApplications();
    void testEmptyJobsPageFilteredEmpty();
    void testEmptyApplicationDialogInterviewsTab();
    void testEmptyApplicationDialogTimelineTab();
    void testEmptyCalendarAgendaNoDaySelected();
    void testEmptyCalendarAgendaSelectedEmptyDay();
    void testEmptyTrackDialogNoTracks();

    // D3 Tab order walks
    void testTabOrderSidebar();
    void testStudyDetailPanelDateTabStops();
    void testTabOrderPageHome();
    void testTabOrderPageHomeEmpty();
    void testTabOrderPageStudy();
    void testTabOrderPageJobs();
    void testTabOrderPageCalendar();
    void testTabOrderPageSettings();
    void testTabOrderDialogApplicationOverview();
    void testTabOrderDialogApplicationInterviewsAndNotes();
    void testTabOrderDialogApplicationTimeline();
    void testTabOrderDialogInterview();
    void testTabOrderDialogFollowUp();
    void testTabOrderDialogSession();
    void testTabOrderDialogTrack();

    // D3 Visible focus indicator painted tests
    void testVisibleFocusIndicatorPushButton();
    void testVisibleFocusIndicatorLineEdit();
    void testVisibleFocusIndicatorChip();
    void testFocusIndicatorDoesNotChangeGeometryOrSizeHint();

private:
    QTemporaryDir m_tempDir;
    std::unique_ptr<WindowFixture> m_fix;
};

void tst_EmptyStatesAndFocus::initTestCase() {
    installWarningFilter();
    QVERIFY(m_tempDir.isValid());
    isolateSettings(m_tempDir);
}

void tst_EmptyStatesAndFocus::cleanupTestCase() {
    uninstallWarningFilter();
}

void tst_EmptyStatesAndFocus::init() {
    m_fix = std::make_unique<WindowFixture>();
    clearCapturedWarnings();
}

void tst_EmptyStatesAndFocus::cleanup() {
    m_fix.reset();
    JOBPREP_VERIFY_ZERO_WARNINGS();
}

// -----------------------------------------------------------------------------
// D2 Empty States Inventory
// -----------------------------------------------------------------------------

void tst_EmptyStatesAndFocus::testEmptyDashboardAsAWhole() {
    // Fresh fixture with no data
    auto* home = m_fix->window().findChild<JobPrep::Ui::Pages::HomePage*>();
    QVERIFY(home);
    home->onActivated();
    QApplication::processEvents();

    auto* emptyWidget = home->findChild<JobPrep::Ui::Widgets::EmptyState*>();
    QVERIFY(emptyWidget);
    QVERIFY(emptyWidget->isVisible());

    bool addAppFired = false;
    connect(home, &JobPrep::Ui::Pages::HomePage::addApplicationRequested, this,
            [&addAppFired]() { addAppFired = true; });

    answerModalWhenReady([](QWidget* modal) {
        if (auto* dlg = qobject_cast<JobPrep::Ui::Dialogs::ApplicationDialog*>(modal)) {
            dlg->reject();
        }
    });

    auto* actionBtn = emptyWidget->findChild<QPushButton*>(u"EmptyStateActionBtn"_s);
    QVERIFY(actionBtn);
    QTest::mouseClick(actionBtn, Qt::LeftButton);
    QVERIFY(addAppFired);
}

void tst_EmptyStatesAndFocus::testEmptyDashboardTodayAndUpcoming() {
    JobPrep::Domain::Track track;
    track.name = u"C++"_s;
    track.color = u"#336699"_s;
    m_fix->context().tracks().insert(track);

    JobPrep::Domain::JobApplication app;
    app.company = u"Acme Corp"_s;
    app.position = u"C++ Engineer"_s;
    m_fix->context().applications().insert(app);

    auto* home = m_fix->window().findChild<JobPrep::Ui::Pages::HomePage*>();
    QVERIFY(home);
    home->onActivated();
    QApplication::processEvents();

    auto* todayEmpty = home->findChild<JobPrep::Ui::Widgets::EmptyState*>(u"TodayEmptyState"_s);
    auto* upcomingEmpty = home->findChild<JobPrep::Ui::Widgets::EmptyState*>(u"UpcomingEmptyState"_s);
    QVERIFY(todayEmpty);
    QVERIFY(upcomingEmpty);
    QVERIFY(todayEmpty->isVisible());
    QVERIFY(upcomingEmpty->isVisible());

    bool interviewRequested = false;
    connect(home, &JobPrep::Ui::Pages::HomePage::newInterviewRequested, this,
            [&interviewRequested]() { interviewRequested = true; });

    answerModalWhenReady([](QWidget* modal) {
        if (auto* dlg = qobject_cast<JobPrep::Ui::Dialogs::InterviewDialog*>(modal)) {
            dlg->done(QDialog::Rejected);
        }
    });

    auto* scheduleBtn = upcomingEmpty->findChild<QPushButton*>(u"EmptyStateActionBtn"_s);
    QVERIFY(scheduleBtn);
    QTest::mouseClick(scheduleBtn, Qt::LeftButton);
    QVERIFY(interviewRequested);
}

void tst_EmptyStatesAndFocus::testEmptyDashboardFunnelAndStudyProgress() {
    auto* funnel = m_fix->window().findChild<JobPrep::Ui::Widgets::FunnelWidget*>();
    auto* rings = m_fix->window().findChild<JobPrep::Ui::Widgets::ProgressRing*>();
    QVERIFY(funnel);
    QVERIFY(rings);

    // Render both to pixmap to assert zero errors or crashes when data is completely empty
    QPixmap p1(300, 200);
    p1.fill(Qt::transparent);
    funnel->render(&p1);
    QVERIFY(!p1.isNull());

    QPixmap p2(300, 200);
    p2.fill(Qt::transparent);
    rings->render(&p2);
    QVERIFY(!p2.isNull());
}

void tst_EmptyStatesAndFocus::testEmptyStudyPageNoTracks() {
    auto* study = m_fix->window().findChild<JobPrep::Ui::Pages::StudyPage*>();
    QVERIFY(study);
    emit m_fix->window().sidebar()->pageSelected(1);
    study->onActivated();
    QApplication::processEvents();

    auto* noTracksState = study->findChild<JobPrep::Ui::Widgets::EmptyState*>(u"StudyNoTracksState"_s);
    QVERIFY(noTracksState);
    QVERIFY(noTracksState->isVisible());

    // Clicking action opens TrackDialog
    auto* actionBtn = noTracksState->findChild<QPushButton*>(u"EmptyStateActionBtn"_s);
    QVERIFY(actionBtn);

    bool dialogHandled = false;
    answerModalWhenReady([&dialogHandled](QWidget* modal) {
        if (auto* dlg = qobject_cast<JobPrep::Ui::Dialogs::TrackDialog*>(modal)) {
            dialogHandled = true;
            dlg->reject();
        }
    });
    QTest::mouseClick(actionBtn, Qt::LeftButton);
    QVERIFY(dialogHandled);
}

void tst_EmptyStatesAndFocus::testEmptyStudyPageNoTopicsInColumnAndList() {
    JobPrep::Domain::Track track;
    track.name = u"C++"_s;
    track.color = u"#336699"_s;
    m_fix->context().tracks().insert(track);

    auto* study = m_fix->window().findChild<JobPrep::Ui::Pages::StudyPage*>();
    QVERIFY(study);
    emit m_fix->window().sidebar()->pageSelected(1);
    study->onActivated();
    QApplication::processEvents();

    // Switch to List view
    auto* listBtn = study->findChild<QPushButton*>(u"StudyViewListBtn"_s);
    QVERIFY(listBtn);
    QTest::mouseClick(listBtn, Qt::LeftButton);
    QApplication::processEvents();

    auto* noDataState = study->findChild<JobPrep::Ui::Widgets::EmptyState*>(u"StudyEmptyState"_s);
    QVERIFY(noDataState);
    QVERIFY(noDataState->isVisible());
}

void tst_EmptyStatesAndFocus::testEmptyStudyPageFilteredEmpty() {
    JobPrep::Domain::Track track;
    track.name = u"C++"_s;
    track.color = u"#336699"_s;
    m_fix->context().tracks().insert(track);

    JobPrep::Domain::Topic topic;
    topic.trackId = track.id;
    topic.title = u"Smart Pointers"_s;
    m_fix->context().topics().insert(topic);

    auto* study = m_fix->window().findChild<JobPrep::Ui::Pages::StudyPage*>();
    QVERIFY(study);
    emit m_fix->window().sidebar()->pageSelected(1);
    study->onActivated();
    QApplication::processEvents();

    auto* searchEdit = study->findChild<QLineEdit*>(u"StudySearchEdit"_s);
    QVERIFY(searchEdit);
    searchEdit->setText(u"NonExistentString12345"_s);
    QApplication::processEvents();

    auto* noMatchesState = study->findChild<JobPrep::Ui::Widgets::EmptyState*>(u"StudyNoMatchesState"_s);
    QVERIFY(noMatchesState);
    QVERIFY(noMatchesState->isVisible());

    // Action clears search filter
    auto* clearBtn = noMatchesState->findChild<QPushButton*>(u"EmptyStateActionBtn"_s);
    QVERIFY(clearBtn);
    QTest::mouseClick(clearBtn, Qt::LeftButton);
    QApplication::processEvents();

    QCOMPARE(searchEdit->text(), QString());
    QVERIFY(!noMatchesState->isVisible());
}

void tst_EmptyStatesAndFocus::testEmptyTopicChecklistAndResources() {
    JobPrep::Domain::Track track;
    track.name = u"C++"_s;
    track.color = u"#336699"_s;
    m_fix->context().tracks().insert(track);

    JobPrep::Domain::Topic topic;
    topic.trackId = track.id;
    topic.title = u"Move Semantics"_s;
    m_fix->context().topics().insert(topic);

    auto* study = m_fix->window().findChild<JobPrep::Ui::Pages::StudyPage*>();
    QVERIFY(study);
    emit m_fix->window().sidebar()->pageSelected(1);
    study->selectTopic(topic.id);
    QApplication::processEvents();

    auto* panel = study->findChild<JobPrep::Ui::Widgets::TopicDetailPanel*>();
    QVERIFY(panel);
    auto* checkList = panel->findChild<QListWidget*>(u"StudyDetailCheckList"_s);
    auto* resourcesTable = panel->findChild<QTableWidget*>(u"StudyDetailResourcesTable"_s);
    QVERIFY(checkList);
    QVERIFY(resourcesTable);
    QCOMPARE(checkList->count(), 0);
    QCOMPARE(resourcesTable->rowCount(), 0);
}

void tst_EmptyStatesAndFocus::testEmptyJobsPageNoApplications() {
    auto* jobs = m_fix->window().findChild<JobPrep::Ui::Pages::JobsPage*>();
    QVERIFY(jobs);
    emit m_fix->window().sidebar()->pageSelected(2);
    jobs->onActivated();
    QApplication::processEvents();

    auto* noDataState = jobs->findChild<JobPrep::Ui::Widgets::EmptyState*>(u"JobsNoDataState"_s);
    auto* view = jobs->findChild<QTableView*>(u"JobsTableView"_s);
    QVERIFY(noDataState);
    QVERIFY(view);
    QVERIFY(noDataState->isVisible());
    QVERIFY(!view->isVisible());

    // Action opens ApplicationDialog
    auto* actionBtn = noDataState->findChild<QPushButton*>(u"EmptyStateActionBtn"_s);
    QVERIFY(actionBtn);

    bool dialogHandled = false;
    answerModalWhenReady([&dialogHandled](QWidget* modal) {
        if (auto* dlg = qobject_cast<JobPrep::Ui::Dialogs::ApplicationDialog*>(modal)) {
            dialogHandled = true;
            dlg->reject();
        }
    });
    QTest::mouseClick(actionBtn, Qt::LeftButton);
    QVERIFY(dialogHandled);

    // Adding 1 application hides empty state and shows view
    JobPrep::Domain::JobApplication app;
    app.company = u"Canonical"_s;
    app.position = u"Software Engineer"_s;
    m_fix->context().applications().insert(app);
    jobs->onActivated();
    QApplication::processEvents();

    QVERIFY(!noDataState->isVisible());
    QVERIFY(view->isVisible());
}

void tst_EmptyStatesAndFocus::testEmptyJobsPageFilteredEmpty() {
    JobPrep::Domain::JobApplication app;
    app.company = u"Red Hat"_s;
    app.position = u"Platform Engineer"_s;
    m_fix->context().applications().insert(app);

    auto* jobs = m_fix->window().findChild<JobPrep::Ui::Pages::JobsPage*>();
    QVERIFY(jobs);
    emit m_fix->window().sidebar()->pageSelected(2);
    jobs->onActivated();
    QApplication::processEvents();

    auto* searchEdit = jobs->findChild<QLineEdit*>(u"JobsSearchEdit"_s);
    QVERIFY(searchEdit);
    searchEdit->setText(u"NonExistentFilter999"_s);
    QApplication::processEvents();

    auto* noMatchesState = jobs->findChild<JobPrep::Ui::Widgets::EmptyState*>(u"JobsNoMatchesState"_s);
    QVERIFY(noMatchesState);
    QVERIFY(noMatchesState->isVisible());

    // Action clears search filter
    auto* clearBtn = noMatchesState->findChild<QPushButton*>(u"EmptyStateActionBtn"_s);
    QVERIFY(clearBtn);
    QTest::mouseClick(clearBtn, Qt::LeftButton);
    QApplication::processEvents();

    QCOMPARE(searchEdit->text(), QString());
    QVERIFY(!noMatchesState->isVisible());
}

void tst_EmptyStatesAndFocus::testEmptyApplicationDialogInterviewsTab() {
    JobPrep::Domain::JobApplication app;
    app.company = u"Mozilla"_s;
    app.position = u"Rust/C++ Dev"_s;
    m_fix->context().applications().insert(app);
    const auto createdApp = m_fix->context().applications().byId(app.id).value();

    JobPrep::Ui::Dialogs::ApplicationDialog dlg(m_fix->context().applications(),
                                              m_fix->context().interviews(),
                                              JobPrep::Ui::Dialogs::ApplicationDialog::Edit);
    dlg.setApplication(createdApp);
    dlg.show();
    QApplication::processEvents();

    auto* tabs = dlg.findChild<QTabWidget*>(u"ApplicationDialogTabs"_s);
    QVERIFY(tabs);
    tabs->setCurrentIndex(2); // Interviews tab
    QApplication::processEvents();

    auto* emptyInterviews = dlg.findChild<JobPrep::Ui::Widgets::EmptyState*>(u"InterviewsEmptyState"_s);
    auto* table = dlg.findChild<QTableWidget*>(u"InterviewsTable"_s);
    QVERIFY(emptyInterviews);
    QVERIFY(table);
    QVERIFY(emptyInterviews->isVisible());
    QVERIFY(!table->isVisible());

    // Action opens InterviewDialog
    auto* actionBtn = emptyInterviews->findChild<QPushButton*>(u"EmptyStateActionBtn"_s);
    QVERIFY(actionBtn);

    bool dialogHandled = false;
    answerModalWhenReady([&dialogHandled](QWidget* modal) {
        if (auto* idlg = qobject_cast<JobPrep::Ui::Dialogs::InterviewDialog*>(modal)) {
            dialogHandled = true;
            idlg->done(QDialog::Rejected);
        }
    });
    QTest::mouseClick(actionBtn, Qt::LeftButton);
    QVERIFY(dialogHandled);

    // Adding 1 interview hides empty state and shows table
    JobPrep::Domain::Interview interview;
    interview.applicationId = app.id;
    interview.startAt = QDateTime::currentDateTime();
    interview.durationMin = 45;
    m_fix->context().interviews().insert(interview);

    dlg.setApplication(createdApp);
    tabs->setCurrentIndex(2);
    QApplication::processEvents();

    QVERIFY(!emptyInterviews->isVisible());
    QVERIFY(table->isVisible());
    dlg.reject();
}

void tst_EmptyStatesAndFocus::testEmptyApplicationDialogTimelineTab() {
    JobPrep::Domain::JobApplication app;
    app.company = u"Valve"_s;
    app.position = u"Graphics Dev"_s;
    m_fix->context().applications().insert(app);
    // Clear initial status transition so history is empty
    QSqlQuery q(m_fix->context().database().connection());
    q.prepare(u"DELETE FROM status_history WHERE application_id = ?"_s);
    q.addBindValue(app.id);
    q.exec();

    const auto createdApp = m_fix->context().applications().byId(app.id).value();

    JobPrep::Ui::Dialogs::ApplicationDialog dlg(m_fix->context().applications(),
                                                m_fix->context().interviews(),
                                                JobPrep::Ui::Dialogs::ApplicationDialog::Edit);
    dlg.setApplication(createdApp);
    dlg.show();
    QApplication::processEvents();

    auto* tabs = dlg.findChild<QTabWidget*>(u"ApplicationDialogTabs"_s);
    auto* timelineEmpty = dlg.findChild<JobPrep::Ui::Widgets::EmptyState*>(u"TimelineEmptyState"_s);
    auto* timelineTable = dlg.findChild<QTableWidget*>(u"TimelineTable"_s);
    QVERIFY(tabs);
    QVERIFY(timelineEmpty);
    QVERIFY(timelineTable);

    for (int i = 0; i < tabs->count(); ++i) {
        if (tabs->tabText(i) == tr("Timeline")) {
            tabs->setCurrentIndex(i);
            break;
        }
    }
    QApplication::processEvents();

    // When status history is empty, the empty state is visible
    QVERIFY(timelineEmpty->isVisible());
    QVERIFY(!timelineTable->isVisible());

    // Updating status records a transition in history, which hides empty state and shows table
    const auto historyBefore = m_fix->context().applications().statusHistory(app.id);
    app.status = JobPrep::Domain::ApplicationStatus::Technical;
    m_fix->context().applications().update(app);
    const auto updatedApp = m_fix->context().applications().byId(app.id).value();
    dlg.setApplication(updatedApp);
    tabs->setCurrentIndex(3);
    QApplication::processEvents();
    QVERIFY(!timelineEmpty->isVisible());
    QVERIFY(timelineTable->isVisible());

    dlg.reject();
}

void tst_EmptyStatesAndFocus::testEmptyCalendarAgendaNoDaySelected() {
    auto* cal = m_fix->window().findChild<JobPrep::Ui::Pages::CalendarPage*>();
    QVERIFY(cal);
    emit m_fix->window().sidebar()->pageSelected(3);
    cal->onActivated();
    QApplication::processEvents();

    auto* monthView = cal->findChild<JobPrep::Ui::Widgets::MonthView*>();
    QVERIFY(monthView);
    monthView->setSelectedDay(QDate());
    QApplication::processEvents();

    auto* emptyState = cal->findChild<JobPrep::Ui::Widgets::EmptyState*>(u"CalendarAgendaEmptyState"_s);
    QVERIFY(emptyState);
    QVERIFY(emptyState->isVisible());

    auto* titleLabel = emptyState->findChild<QLabel*>(u"EmptyStateTitle"_s);
    QVERIFY(titleLabel);
    QCOMPARE(titleLabel->text(), tr("No Day Selected"));
}

void tst_EmptyStatesAndFocus::testEmptyCalendarAgendaSelectedEmptyDay() {
    auto* cal = m_fix->window().findChild<JobPrep::Ui::Pages::CalendarPage*>();
    QVERIFY(cal);
    emit m_fix->window().sidebar()->pageSelected(3);
    cal->onActivated();
    QApplication::processEvents();

    auto* monthView = cal->findChild<JobPrep::Ui::Widgets::MonthView*>();
    QVERIFY(monthView);
    monthView->setSelectedDay(QDate(2026, 11, 15));
    QApplication::processEvents();

    auto* emptyState = cal->findChild<JobPrep::Ui::Widgets::EmptyState*>(u"CalendarAgendaEmptyState"_s);
    QVERIFY(emptyState);
    QVERIFY(emptyState->isVisible());

    auto* titleLabel = emptyState->findChild<QLabel*>(u"EmptyStateTitle"_s);
    QVERIFY(titleLabel);
    QCOMPARE(titleLabel->text(), tr("Nothing Planned"));

    // Action opens InterviewDialog
    auto* actionBtn = emptyState->findChild<QPushButton*>(u"EmptyStateActionBtn"_s);
    QVERIFY(actionBtn);

    bool dialogHandled = false;
    answerModalWhenReady([&dialogHandled](QWidget* modal) {
        if (auto* idlg = qobject_cast<JobPrep::Ui::Dialogs::InterviewDialog*>(modal)) {
            dialogHandled = true;
            idlg->done(QDialog::Rejected);
        }
    });
    QTest::mouseClick(actionBtn, Qt::LeftButton);
    QVERIFY(dialogHandled);
}

void tst_EmptyStatesAndFocus::testEmptyTrackDialogNoTracks() {
    JobPrep::Ui::Dialogs::TrackDialog dlg(m_fix->context().tracks(),
                                          m_fix->context().topics());
    dlg.show();
    QApplication::processEvents();

    auto* list = dlg.findChild<QListWidget*>(u"TrackListWidget"_s);
    auto* deleteBtn = dlg.findChild<QPushButton*>(u"TrackDeleteBtn"_s);
    auto* nameEdit = dlg.findChild<QLineEdit*>(u"TrackNameEdit"_s);
    QVERIFY(list);
    QVERIFY(deleteBtn);
    QVERIFY(nameEdit);

    QCOMPARE(list->count(), 0);
    QVERIFY(!deleteBtn->isEnabled());
    QCOMPARE(nameEdit->text(), QString());
    dlg.reject();
}

// -----------------------------------------------------------------------------
// D3 Tab Order Walks
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// D3 Tab Order Walks
// -----------------------------------------------------------------------------

static QString printChain(const QString& title, const QStringList& names) {
    const QString joined = names.join(u" -> "_s);
    fprintf(stdout, "TAB_CHAIN [%s]: %s\n", qPrintable(title), qPrintable(joined));
    fflush(stdout);
    return joined;
}

static void verifyTabAndBacktabChain(QWidget* window, const QList<QWidget*>& chain) {
    if (chain.isEmpty()) return;
    activateWindow(window);
    QApplication::processEvents();

    chain.first()->setFocus(Qt::TabFocusReason);
    QApplication::processEvents();

    // Forward walk with real Tab
    for (qsizetype i = 0; i < chain.size() - 1; ++i) {
        sendKeyClick(window, Qt::Key_Tab);
        QWidget* focused = window->focusWidget();
        if (!focused) focused = QApplication::focusWidget();
        QWidget* expected = chain[i + 1];
        QVERIFY2(focused == expected || (focused && focused->parentWidget() == expected),
                 qPrintable(QString(u"Forward Tab step %1 failed: focused=%2 expected=%3"_s)
                                .arg(i)
                                .arg(focused ? focused->objectName() : u"null"_s)
                                .arg(expected ? expected->objectName() : u"null"_s)));
    }

    // Backward walk with real Backtab
    for (qsizetype i = chain.size() - 1; i > 0; --i) {
        sendKeyClick(window, Qt::Key_Backtab);
        QWidget* focused = window->focusWidget();
        if (!focused) focused = QApplication::focusWidget();
        QWidget* expected = chain[i - 1];
        QVERIFY2(focused == expected || (focused && focused->parentWidget() == expected),
                 qPrintable(QString(u"Backward Backtab step %1 failed: focused=%2 expected=%3"_s)
                                .arg(i)
                                .arg(focused ? focused->objectName() : u"null"_s)
                                .arg(expected ? expected->objectName() : u"null"_s)));
    }
}

void tst_EmptyStatesAndFocus::testTabOrderSidebar() {
    auto* sidebar = m_fix->window().sidebar();
    QVERIFY(sidebar != nullptr);

    const QStringList expected = {
        u"SidebarNavBtn_home"_s,
        u"SidebarNavBtn_study"_s,
        u"SidebarNavBtn_jobs"_s,
        u"SidebarNavBtn_calendar"_s,
        u"SidebarNavBtn_settings"_s,
        u"SidebarToggleBtn"_s
    };

    QList<QWidget*> chain;
    for (const auto& name : expected) {
        auto* btn = sidebar->findChild<QWidget*>(name);
        QVERIFY2(btn != nullptr, qPrintable(u"Missing widget: "_s + name));
        QVERIFY2(btn->focusPolicy() != Qt::NoFocus, qPrintable(u"NoFocus: "_s + name));
        chain.append(btn);
    }
    verifyTabAndBacktabChain(&m_fix->window(), chain);
    printChain(u"Sidebar"_s, expected);
}

void tst_EmptyStatesAndFocus::testStudyDetailPanelDateTabStops() {
    auto* study = m_fix->window().findChild<JobPrep::Ui::Pages::StudyPage*>();
    QVERIFY(study);
    m_fix->window().sidebar()->setCurrentPage(1);
    emit m_fix->window().sidebar()->pageSelected(1);
    study->onActivated();
    activateWindow(&m_fix->window());
    QApplication::processEvents();

    JobPrep::Domain::Track trk;
    trk.name = u"C++"_s;
    m_fix->context().tracks().insert(trk);
    JobPrep::Domain::Topic top;
    top.trackId = trk.id;
    top.title = u"Smart Pointers"_s;
    top.targetDate = QDate::currentDate().addDays(7);
    m_fix->context().topics().insert(top);

    auto* detail = study->detailPanel();
    QVERIFY(detail);
    detail->loadFrom(top.id);
    QApplication::processEvents();

    auto* titleEdit = detail->findChild<QLineEdit*>(u"StudyDetailTitleEdit"_s);
    auto* dateWidget = detail->findChild<QWidget*>(u"StudyDetailDateEdit"_s);
    QVERIFY(titleEdit);
    QVERIFY(dateWidget);

    fprintf(stdout, "F8_TARGET_DATE_INFO: class=%s focusPolicy=%d isQDateEdit=%d\n",
            dateWidget->metaObject()->className(),
            static_cast<int>(dateWidget->focusPolicy()),
            dateWidget->inherits("QDateEdit") ? 1 : 0);
    fflush(stdout);

    titleEdit->setFocus(Qt::TabFocusReason);
    QApplication::processEvents();
    fprintf(stdout, "F8_START_FOCUS: %s (%s)\n",
            QApplication::focusWidget() ? QApplication::focusWidget()->metaObject()->className() : "null",
            QApplication::focusWidget() ? qPrintable(QApplication::focusWidget()->objectName()) : "null");
    fflush(stdout);

    // Press Tab up to 15 times
    int dateEditHitCount = 0;
    for (int i = 1; i <= 15; ++i) {
        sendKeyClick(&m_fix->window(), Qt::Key_Tab);
        QWidget* focused = QApplication::focusWidget();
        const QString className = focused ? QString::fromUtf8(focused->metaObject()->className()) : u"null"_s;
        const QString objName = focused ? focused->objectName() : u"null"_s;
        QWidget* parent = focused ? focused->parentWidget() : nullptr;
        const QString parentName = parent ? parent->objectName() : u""_s;

        fprintf(stdout, "TAB_PRESS_%02d: %s (name: %s, parent: %s)\n",
                i, qPrintable(className), qPrintable(objName), qPrintable(parentName));
        fflush(stdout);

        if (focused == dateWidget || parent == dateWidget) {
            dateEditHitCount++;
        }
    }
    fprintf(stdout, "F8_DATE_EDIT_TOTAL_TAB_STOPS: %d\n", dateEditHitCount);
    fflush(stdout);
    QCOMPARE(dateEditHitCount, 1);

    // Direct Forward Tab step: priorityCombo -> dateEdit -> dateClearBtn
    auto* priorityCombo = detail->findChild<QComboBox*>(u"StudyDetailPriorityCombo"_s);
    auto* clearBtn = detail->findChild<QPushButton*>(u"StudyDetailDateClearBtn"_s);
    QVERIFY(priorityCombo);
    QVERIFY(clearBtn);

    priorityCombo->setFocus(Qt::TabFocusReason);
    QApplication::processEvents();
    QCOMPARE(QApplication::focusWidget(), priorityCombo);

    sendKeyClick(&m_fix->window(), Qt::Key_Tab);
    QWidget* afterPriority = QApplication::focusWidget();
    QVERIFY(afterPriority == dateWidget || (afterPriority && afterPriority->parentWidget() == dateWidget));

    sendKeyClick(&m_fix->window(), Qt::Key_Tab);
    QCOMPARE(QApplication::focusWidget(), clearBtn);

    // Direct Backward Tab step: clearBtn -> dateEdit -> priorityCombo
    sendKeyClick(&m_fix->window(), Qt::Key_Backtab);
    QWidget* afterBackFromClear = QApplication::focusWidget();
    QVERIFY(afterBackFromClear == dateWidget || (afterBackFromClear && afterBackFromClear->parentWidget() == dateWidget));

    sendKeyClick(&m_fix->window(), Qt::Key_Backtab);
    QCOMPARE(QApplication::focusWidget(), priorityCombo);

    // Verify arrow keys, calendar popup, and clear button functionality
    auto* dateEdit = qobject_cast<QDateEdit*>(dateWidget);
    QVERIFY(dateEdit);
    QVERIFY(dateEdit->calendarPopup());
    dateWidget->setFocus(Qt::TabFocusReason);
    QApplication::processEvents();
    const QDate origDate = dateEdit->date();

    // Arrow keys still increment/decrement sections
    QTest::keyClick(dateWidget, Qt::Key_Up);
    QApplication::processEvents();
    QVERIFY(dateEdit->date() != origDate);

    // Clear button functionality
    QVERIFY(clearBtn->isEnabled());
    clearBtn->click();
    QApplication::processEvents();
    QCOMPARE(dateEdit->date(), dateEdit->minimumDate());
}

void tst_EmptyStatesAndFocus::testTabOrderDialogApplicationOverview() {
    JobPrep::Ui::Dialogs::ApplicationDialog dlg(m_fix->context().applications(),
                                              m_fix->context().interviews(),
                                              JobPrep::Ui::Dialogs::ApplicationDialog::Create);
    dlg.show();
    activateWindow(&dlg);
    QApplication::processEvents();

    auto* appliedCheck = dlg.findChild<QCheckBox*>(u"FieldAppliedDateCheck"_s);
    auto* nextCheck = dlg.findChild<QCheckBox*>(u"FieldNextActionDateCheck"_s);
    if (appliedCheck) appliedCheck->setChecked(true);
    if (nextCheck) nextCheck->setChecked(true);
    QApplication::processEvents();

    const QStringList expected = {
        u"FieldCompany"_s,
        u"FieldPosition"_s,
        u"FieldStatus"_s,
        u"FieldSourceCombo"_s,
        u"FieldWorkMode"_s,
        u"FieldSalaryMin"_s,
        u"FieldSalaryMax"_s,
        u"FieldCurrency"_s,
        u"FieldAppliedDateCheck"_s,
        u"FieldAppliedDate"_s,
        u"FieldNextAction"_s,
        u"FieldNextActionDateCheck"_s,
        u"FieldNextActionDate"_s,
        u"FieldUrl"_s,
        u"FieldResumeVersion"_s,
        u"FieldLocation"_s,
        u"FieldContactName"_s,
        u"FieldContactEmail"_s,
        u"DialogSaveBtn"_s
    };

    QList<QWidget*> chain;
    for (const auto& name : expected) {
        auto* w = dlg.findChild<QWidget*>(name);
        QVERIFY2(w != nullptr, qPrintable(u"Missing widget: "_s + name));
        chain.append(w);
    }
    verifyTabAndBacktabChain(&dlg, chain);
    printChain(u"ApplicationDialog_Overview"_s, expected);
    dlg.reject();
}

void tst_EmptyStatesAndFocus::testTabOrderDialogApplicationInterviewsAndNotes() {
    JobPrep::Domain::JobApplication app;
    app.company = u"Example"_s;
    app.position = u"Dev"_s;
    m_fix->context().applications().insert(app);

    JobPrep::Domain::Interview iv;
    iv.applicationId = app.id;
    iv.startAt = QDateTime::currentDateTime();
    m_fix->context().interviews().insert(iv);

    const auto createdApp = m_fix->context().applications().byId(app.id).value();

    JobPrep::Ui::Dialogs::ApplicationDialog dlg(m_fix->context().applications(),
                                              m_fix->context().interviews(),
                                              JobPrep::Ui::Dialogs::ApplicationDialog::Edit);
    dlg.setApplication(createdApp);
    dlg.show();
    activateWindow(&dlg);
    QApplication::processEvents();

    auto* tabs = dlg.findChild<QTabWidget*>(u"ApplicationDialogTabs"_s);
    QVERIFY(tabs);

    // 1. Notes tab (tab 1)
    tabs->setCurrentIndex(1);
    QApplication::processEvents();
    auto* notesEdit = dlg.findChild<QTextEdit*>(u"FieldNotes"_s);
    auto* saveBtn = dlg.findChild<QPushButton*>(u"DialogSaveBtn"_s);
    QVERIFY(notesEdit && saveBtn);
    verifyTabAndBacktabChain(&dlg, {notesEdit, saveBtn});
    printChain(u"ApplicationDialog_Notes"_s, {u"FieldNotes"_s, u"DialogSaveBtn"_s});

    // 2. Switch to Interviews tab (tab 2)
    tabs->setCurrentIndex(2);
    QApplication::processEvents();

    auto* interviewsTable = dlg.findChild<QTableWidget*>(u"InterviewsTable"_s);
    auto* addInterviewBtn = dlg.findChild<QPushButton*>(u"InterviewsAddBtn"_s);
    auto* editInterviewBtn = dlg.findChild<QPushButton*>(u"InterviewsEditBtn"_s);
    auto* deleteInterviewBtn = dlg.findChild<QPushButton*>(u"InterviewsDeleteBtn"_s);
    QVERIFY(interviewsTable && addInterviewBtn && editInterviewBtn && deleteInterviewBtn && saveBtn);

    if (interviewsTable->rowCount() > 0) {
        interviewsTable->selectRow(0);
        QApplication::processEvents();
    }

    verifyTabAndBacktabChain(&dlg, {interviewsTable, addInterviewBtn, editInterviewBtn, deleteInterviewBtn, saveBtn});
    printChain(u"ApplicationDialog_Interviews"_s,
               {u"InterviewsTable"_s, u"InterviewsAddBtn"_s, u"InterviewsEditBtn"_s,
                u"InterviewsDeleteBtn"_s, u"DialogSaveBtn"_s});

    dlg.reject();
}

void tst_EmptyStatesAndFocus::testTabOrderDialogApplicationTimeline() {
    JobPrep::Domain::JobApplication app;
    app.company = u"TimelineCorp"_s;
    app.position = u"Lead"_s;
    m_fix->context().applications().insert(app);
    const auto createdApp = m_fix->context().applications().byId(app.id).value();

    JobPrep::Ui::Dialogs::ApplicationDialog dlg(m_fix->context().applications(),
                                              m_fix->context().interviews(),
                                              JobPrep::Ui::Dialogs::ApplicationDialog::Edit);
    dlg.setApplication(createdApp);
    dlg.show();
    activateWindow(&dlg);
    QApplication::processEvents();

    auto* tabs = dlg.findChild<QTabWidget*>(u"ApplicationDialogTabs"_s);
    QVERIFY(tabs);
    tabs->setCurrentIndex(3);
    QApplication::processEvents();

    auto* timelineTable = dlg.findChild<QTableWidget*>(u"TimelineTable"_s);
    auto* saveBtn = dlg.findChild<QPushButton*>(u"DialogSaveBtn"_s);
    QVERIFY(timelineTable && saveBtn);
    verifyTabAndBacktabChain(&dlg, {timelineTable, saveBtn});
    printChain(u"ApplicationDialog_Timeline"_s, {u"TimelineTable"_s, u"DialogSaveBtn"_s});

    dlg.reject();
}

void tst_EmptyStatesAndFocus::testTabOrderDialogInterview() {
    JobPrep::Domain::JobApplication app;
    app.company = u"Tesla"_s;
    app.position = u"Autopilot Dev"_s;
    m_fix->context().applications().insert(app);

    JobPrep::Ui::Dialogs::InterviewDialog dlg(m_fix->context().applications(),
                                            m_fix->context().interviews(),
                                            JobPrep::Ui::Dialogs::InterviewDialog::Create);
    dlg.show();
    activateWindow(&dlg);
    QApplication::processEvents();

    const QStringList expected = {
        u"InterviewApplicationCombo"_s,
        u"InterviewDateEdit"_s,
        u"InterviewTimeEdit"_s,
        u"InterviewDurationSpin"_s,
        u"InterviewTypeCombo"_s,
        u"InterviewOutcomeCombo"_s,
        u"InterviewPlaceEdit"_s,
        u"InterviewInterviewerEdit"_s,
        u"InterviewNotesEdit"_s,
        u"DialogSaveBtn"_s
    };

    QList<QWidget*> chain;
    for (const auto& name : expected) {
        auto* w = dlg.findChild<QWidget*>(name);
        QVERIFY2(w != nullptr, qPrintable(u"Missing widget: "_s + name));
        chain.append(w);
    }
    verifyTabAndBacktabChain(&dlg, chain);
    printChain(u"InterviewDialog"_s, expected);
    dlg.reject();
}

void tst_EmptyStatesAndFocus::testTabOrderDialogFollowUp() {
    JobPrep::Domain::JobApplication app;
    app.company = u"Apple"_s;
    app.position = u"Firmware Dev"_s;
    m_fix->context().applications().insert(app);

    JobPrep::Ui::Dialogs::FollowUpDialog dlg(m_fix->context().applications(), QDate::currentDate());
    dlg.show();
    activateWindow(&dlg);
    QApplication::processEvents();

    auto* appCombo = dlg.findChild<QComboBox*>(u"FollowUpApplicationCombo"_s);
    auto* actionEdit = dlg.findChild<QLineEdit*>(u"FollowUpActionEdit"_s);
    auto* saveBtn = dlg.findChild<QPushButton*>(u"DialogSaveBtn"_s);
    QVERIFY(appCombo && actionEdit && saveBtn);

    verifyTabAndBacktabChain(&dlg, {appCombo, actionEdit, saveBtn});
    printChain(u"FollowUpDialog"_s, {u"FollowUpApplicationCombo"_s, u"FollowUpActionEdit"_s, u"DialogSaveBtn"_s});
    dlg.reject();
}

void tst_EmptyStatesAndFocus::testTabOrderDialogSession() {
    JobPrep::Domain::Track track;
    track.name = u"Algorithms"_s;
    track.color = u"#993366"_s;
    m_fix->context().tracks().insert(track);

    JobPrep::Domain::Topic topic;
    topic.trackId = track.id;
    topic.title = u"Dynamic Programming"_s;
    m_fix->context().topics().insert(topic);

    JobPrep::Ui::Dialogs::SessionDialog dlg(m_fix->context().sessions(),
                                          m_fix->context().topics());
    dlg.show();
    activateWindow(&dlg);
    QApplication::processEvents();

    const QStringList expected = {
        u"SessionDateEdit"_s, u"SessionMinutesSpin"_s, u"SessionTopicCombo"_s,
        u"SessionNoteEdit"_s, u"SessionSaveBtn"_s, u"SessionCancelBtn"_s
    };

    QList<QWidget*> chain;
    for (const auto& name : expected) {
        auto* w = dlg.findChild<QWidget*>(name);
        QVERIFY2(w != nullptr, qPrintable(u"Missing widget: "_s + name));
        chain.append(w);
    }
    verifyTabAndBacktabChain(&dlg, chain);
    printChain(u"SessionDialog"_s, expected);
    dlg.reject();
}

void tst_EmptyStatesAndFocus::testTabOrderDialogTrack() {
    JobPrep::Domain::Track track;
    track.name = u"C++"_s;
    track.color = u"#336699"_s;
    m_fix->context().tracks().insert(track);

    JobPrep::Ui::Dialogs::TrackDialog dlg(m_fix->context().tracks(),
                                          m_fix->context().topics());
    dlg.show();
    activateWindow(&dlg);
    QApplication::processEvents();

    const QStringList expected = {
        u"TrackListWidget"_s, u"TrackNewBtn"_s, u"TrackDeleteBtn"_s,
        u"TrackNameEdit"_s, u"TrackSwatch_0"_s, u"TrackSaveBtn"_s, u"TrackCloseBtn"_s
    };

    QList<QWidget*> chain;
    for (const auto& name : expected) {
        auto* w = dlg.findChild<QWidget*>(name);
        QVERIFY2(w != nullptr, qPrintable(u"Missing widget: "_s + name));
        chain.append(w);
    }
    verifyTabAndBacktabChain(&dlg, chain);
    printChain(u"TrackDialog"_s, expected);
    dlg.reject();
}

void tst_EmptyStatesAndFocus::testTabOrderPageHome() {
    // Populate data so views are visible
    JobPrep::Domain::JobApplication app;
    app.company = u"Google"_s;
    app.position = u"SWE"_s;
    m_fix->context().applications().insert(app);

    JobPrep::Domain::Interview ivToday;
    ivToday.applicationId = app.id;
    ivToday.startAt = QDateTime::currentDateTime();
    m_fix->context().interviews().insert(ivToday);

    JobPrep::Domain::Interview ivUpcoming;
    ivUpcoming.applicationId = app.id;
    ivUpcoming.startAt = QDateTime::currentDateTime().addDays(1);
    m_fix->context().interviews().insert(ivUpcoming);

    auto* home = m_fix->window().findChild<JobPrep::Ui::Pages::HomePage*>();
    QVERIFY(home);
    emit m_fix->window().sidebar()->pageSelected(0);
    home->onActivated();
    activateWindow(&m_fix->window());
    QApplication::processEvents();

    const auto views = home->findChildren<QListView*>();
    QVERIFY(views.size() >= 2);
    auto* todayView = views.at(0);
    auto* upcomingView = views.at(1);
    todayView->setObjectName(u"HomeTodayView"_s);
    upcomingView->setObjectName(u"HomeUpcomingView"_s);

    verifyTabAndBacktabChain(&m_fix->window(), {todayView, upcomingView});
    printChain(u"HomePage_WithData"_s, {u"HomeTodayView"_s, u"HomeUpcomingView"_s});
}

void tst_EmptyStatesAndFocus::testTabOrderPageHomeEmpty() {
    auto* home = m_fix->window().findChild<JobPrep::Ui::Pages::HomePage*>();
    QVERIFY(home);
    emit m_fix->window().sidebar()->pageSelected(0);
    home->onActivated();
    activateWindow(&m_fix->window());
    QApplication::processEvents();

    auto* emptyWidget = home->findChild<JobPrep::Ui::Widgets::EmptyState*>();
    QVERIFY(emptyWidget);
    auto* emptyAction = emptyWidget->findChild<QPushButton*>(u"EmptyStateActionBtn"_s);
    auto* emptySecAction = emptyWidget->findChild<QPushButton*>(u"EmptyStateSecondaryActionBtn"_s);
    QVERIFY(emptyAction && emptySecAction);

    verifyTabAndBacktabChain(&m_fix->window(), {emptyAction, emptySecAction});
    printChain(u"HomePage_Empty"_s, {u"DashboardEmptyActionBtn (EmptyStateActionBtn)"_s,
                                  u"DashboardEmptySecondaryActionBtn (EmptyStateSecondaryActionBtn)"_s});
}

void tst_EmptyStatesAndFocus::testTabOrderPageStudy() {
    JobPrep::Domain::Track trk;
    trk.name = u"C++"_s;
    m_fix->context().tracks().insert(trk);

    auto* study = m_fix->window().findChild<JobPrep::Ui::Pages::StudyPage*>();
    QVERIFY(study);
    m_fix->window().sidebar()->setCurrentPage(1);
    emit m_fix->window().sidebar()->pageSelected(1);
    study->onActivated();
    activateWindow(&m_fix->window());
    QApplication::processEvents();

    auto* chips = study->findChild<JobPrep::Ui::Widgets::ChipBar*>(u"StudyTrackChips"_s);
    auto* addTrackBtn = study->findChild<QPushButton*>(u"StudyAddTrackBtn"_s);
    auto* manageTracksBtn = study->findChild<QPushButton*>(u"StudyManageTracksBtn"_s);
    auto* boardBtn = study->findChild<QPushButton*>(u"StudyViewBoardBtn"_s);
    auto* listBtn = study->findChild<QPushButton*>(u"StudyViewListBtn"_s);
    auto* searchEdit = study->findChild<QLineEdit*>(u"StudySearchEdit"_s);
    auto* addTopicBtn = study->findChild<QPushButton*>(u"StudyAddTopicBtn"_s);
    auto* detail = study->detailPanel();

    QVERIFY(chips && addTrackBtn && manageTracksBtn && boardBtn && listBtn && searchEdit && addTopicBtn && detail);

    verifyTabAndBacktabChain(&m_fix->window(), {chips, addTrackBtn, manageTracksBtn, boardBtn, listBtn, searchEdit, addTopicBtn});

    const QStringList expected = {
        u"StudyTrackChips"_s,
        u"StudyAddTrackBtn"_s,
        u"StudyManageTracksBtn"_s,
        u"StudyViewBoardBtn"_s,
        u"StudyViewListBtn"_s,
        u"StudySearchEdit"_s,
        u"StudyAddTopicBtn"_s,
    };
    printChain(u"StudyPage"_s, expected);
}

void tst_EmptyStatesAndFocus::testTabOrderPageJobs() {
    JobPrep::Domain::JobApplication app;
    app.company = u"Acme"_s;
    app.position = u"Dev"_s;
    m_fix->context().applications().insert(app);

    auto* jobs = m_fix->window().findChild<JobPrep::Ui::Pages::JobsPage*>();
    QVERIFY(jobs);
    emit m_fix->window().sidebar()->pageSelected(2);
    jobs->onActivated();
    activateWindow(&m_fix->window());
    QApplication::processEvents();

    auto* searchEdit = jobs->findChild<QLineEdit*>(u"JobsSearchEdit"_s);
    auto* chips = jobs->findChild<JobPrep::Ui::Widgets::ChipBar*>(u"JobsStatusChips"_s);
    auto* exportBtn = jobs->findChild<QPushButton*>(u"JobsExportBtn"_s);
    auto* addBtn = jobs->findChild<QPushButton*>(u"JobsAddBtn"_s);
    auto* view = jobs->findChild<QTableView*>(u"JobsTableView"_s);
    QVERIFY(searchEdit && chips && exportBtn && addBtn && view);

    verifyTabAndBacktabChain(&m_fix->window(), {searchEdit, chips, exportBtn, addBtn, view});

    const QStringList expected = {
        u"JobsSearchEdit"_s,
        u"JobsStatusChips"_s,
        u"JobsExportBtn"_s,
        u"JobsAddBtn"_s,
        u"JobsTableView"_s
    };
    printChain(u"JobsPage"_s, expected);
}

void tst_EmptyStatesAndFocus::testTabOrderPageCalendar() {
    JobPrep::Domain::JobApplication app;
    app.company = u"Acme"_s;
    app.position = u"Dev"_s;
    m_fix->context().applications().insert(app);
    JobPrep::Domain::Interview iv;
    iv.applicationId = app.id;
    iv.startAt = QDateTime(QDate::currentDate(), QTime(10, 0));
    m_fix->context().interviews().insert(iv);

    auto* cal = m_fix->window().findChild<JobPrep::Ui::Pages::CalendarPage*>();
    QVERIFY(cal);
    emit m_fix->window().sidebar()->pageSelected(3);
    cal->onActivated();
    activateWindow(&m_fix->window());
    QApplication::processEvents();

    auto* prevBtn = cal->findChild<QToolButton*>(u"CalendarPrevBtn"_s);
    auto* nextBtn = cal->findChild<QToolButton*>(u"CalendarNextBtn"_s);
    auto* todayBtn = cal->findChild<QPushButton*>(u"CalendarTodayBtn"_s);
    auto* interviewChip = cal->findChild<QPushButton*>(u"CalendarLayerInterviewBtn"_s);
    auto* followUpChip = cal->findChild<QPushButton*>(u"CalendarLayerFollowUpBtn"_s);
    auto* targetChip = cal->findChild<QPushButton*>(u"CalendarLayerTargetBtn"_s);
    auto* monthView = cal->findChild<JobPrep::Ui::Widgets::MonthView*>(u"CalendarMonthView"_s);
    auto* addInterviewBtn = cal->findChild<QPushButton*>(u"CalendarAddInterviewBtn"_s);
    auto* addFollowUpBtn = cal->findChild<QPushButton*>(u"CalendarAddFollowUpBtn"_s);
    auto* agendaView = cal->findChild<QListView*>(u"CalendarAgendaList"_s);

    QVERIFY(prevBtn && nextBtn && todayBtn && interviewChip && followUpChip && targetChip &&
            monthView && addInterviewBtn && addFollowUpBtn && agendaView);

    const QList<QWidget*> chain = {
        prevBtn, nextBtn, todayBtn, interviewChip, followUpChip, targetChip,
        monthView, addInterviewBtn, addFollowUpBtn, agendaView
    };
    verifyTabAndBacktabChain(&m_fix->window(), chain);

    const QStringList expected = {
        u"CalendarPrevBtn"_s,
        u"CalendarNextBtn"_s,
        u"CalendarTodayBtn"_s,
        u"CalendarLayerInterviewBtn"_s,
        u"CalendarLayerFollowUpBtn"_s,
        u"CalendarLayerTargetBtn"_s,
        u"CalendarMonthView"_s,
        u"CalendarAddInterviewBtn"_s,
        u"CalendarAddFollowUpBtn"_s,
        u"CalendarAgendaList"_s
    };
    printChain(u"CalendarPage"_s, expected);
}

void tst_EmptyStatesAndFocus::testTabOrderPageSettings() {
    auto* settings = m_fix->window().findChild<JobPrep::Ui::Pages::SettingsPage*>();
    QVERIFY(settings);
    emit m_fix->window().sidebar()->pageSelected(4);
    settings->onActivated();
    activateWindow(&m_fix->window());
    QApplication::processEvents();

    auto* themeCombo = settings->findChild<QComboBox*>(u"SettingsThemeCombo"_s);
    auto* accentCombo = settings->findChild<QComboBox*>(u"SettingsAccentCombo"_s);
    auto* goalSpin = settings->findChild<QSpinBox*>(u"SettingsStudyGoalSpinBox"_s);
    auto* weekStartCombo = settings->findChild<QComboBox*>(u"SettingsWeekStartCombo"_s);
    auto* closeToTrayCheck = settings->findChild<QCheckBox*>(u"SettingsCloseToTrayCheck"_s);
    auto* leadList = settings->findChild<QListWidget*>(u"SettingsLeadList"_s);
    auto* leadPreset = settings->findChild<QComboBox*>(u"SettingsLeadPresetCombo"_s);
    auto* customSpin = settings->findChild<QSpinBox*>(u"SettingsCustomLeadSpinBox"_s);
    auto* addLeadBtn = settings->findChild<QPushButton*>(u"SettingsAddLeadBtn"_s);
    auto* removeLeadBtn = settings->findChild<QPushButton*>(u"SettingsRemoveLeadBtn"_s);
    auto* resetLeadBtn = settings->findChild<QPushButton*>(u"SettingsResetLeadBtn"_s);
    auto* exportCsvBtn = settings->findChild<QPushButton*>(u"SettingsExportCsvBtn"_s);
    auto* exportIcsBtn = settings->findChild<QPushButton*>(u"SettingsExportIcsBtn"_s);
    auto* backupJsonBtn = settings->findChild<QPushButton*>(u"SettingsBackupJsonBtn"_s);
    auto* restoreJsonBtn = settings->findChild<QPushButton*>(u"SettingsRestoreJsonBtn"_s);
    auto* sampleBtn = settings->findChild<QPushButton*>(u"SettingsLoadSampleBtn"_s);
    auto* openFolderBtn = settings->findChild<QPushButton*>(u"SettingsOpenFolderBtn"_s);

    QVERIFY(themeCombo && accentCombo && goalSpin && weekStartCombo &&
            leadList && leadPreset && customSpin && addLeadBtn && removeLeadBtn && resetLeadBtn &&
            exportCsvBtn && exportIcsBtn && backupJsonBtn && restoreJsonBtn && sampleBtn && openFolderBtn);

    // Select Custom in leadPreset so customSpin is enabled in tab order
    const int customIdx = leadPreset->findData(-1);
    if (customIdx >= 0) {
        leadPreset->setCurrentIndex(customIdx);
        QApplication::processEvents();
    }

    QList<QWidget*> chain = {themeCombo, accentCombo, goalSpin, weekStartCombo};
    if (closeToTrayCheck && closeToTrayCheck->isEnabled()) {
        chain.append(closeToTrayCheck);
    }
    chain.append({leadList, leadPreset, customSpin, addLeadBtn, removeLeadBtn, resetLeadBtn,
                  exportCsvBtn, exportIcsBtn, backupJsonBtn, restoreJsonBtn, sampleBtn, openFolderBtn});

    verifyTabAndBacktabChain(&m_fix->window(), chain);

    QStringList expected;
    for (auto* w : chain) {
        expected.append(w->objectName());
    }
    printChain(u"SettingsPage"_s, expected);
}

void tst_EmptyStatesAndFocus::testFocusIndicatorDoesNotChangeGeometryOrSizeHint() {
    for (const auto theme : {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
        m_fix->context().themeManager().setThemeMode(theme);
        QApplication::processEvents();

        QDialog dlg;
        dlg.resize(400, 300);
        auto* layout = new QVBoxLayout(&dlg);
        auto* other = new QLineEdit(&dlg);
        auto* btn = new QPushButton(tr("Test Button"), &dlg);
        auto* edit = new QLineEdit(&dlg);
        edit->setText(u"Line Edit Sample"_s);
        auto* combo = new QComboBox(&dlg);
        combo->addItem(u"Option 1"_s);
        combo->addItem(u"Option 2"_s);
        auto* chip = new QPushButton(tr("Chip"), &dlg);
        chip->setProperty("chip", true);

        layout->addWidget(other);
        layout->addWidget(btn);
        layout->addWidget(edit);
        layout->addWidget(combo);
        layout->addWidget(chip);
        dlg.show();
        activateWindow(&dlg);
        QApplication::processEvents();

        const QList<QWidget*> widgets = {btn, edit, combo, chip};
        for (auto* w : widgets) {
            other->setFocus(Qt::TabFocusReason);
            QApplication::processEvents();
            QVERIFY(!w->hasFocus());

            const QSize sizeHintBefore = w->sizeHint();
            const QRect geomBefore = w->geometry();

            w->setFocus(Qt::TabFocusReason);
            QApplication::processEvents();
            QVERIFY(w->hasFocus());

            const QSize sizeHintAfter = w->sizeHint();
            const QRect geomAfter = w->geometry();

            QCOMPARE(sizeHintAfter, sizeHintBefore);
            QCOMPARE(geomAfter, geomBefore);
        }
        dlg.close();
        QApplication::processEvents();
    }
}

// -----------------------------------------------------------------------------
// D3 Visible Focus Indicator Painted Tests
// -----------------------------------------------------------------------------

static int countDifferentPixels(const QImage& img1, const QImage& img2) {
    if (img1.size() != img2.size()) return -1;
    int diffs = 0;
    for (int y = 0; y < img1.height(); ++y) {
        const auto* line1 = reinterpret_cast<const QRgb*>(img1.constScanLine(y));
        const auto* line2 = reinterpret_cast<const QRgb*>(img2.constScanLine(y));
        for (int x = 0; x < img1.width(); ++x) {
            if (line1[x] != line2[x]) {
                ++diffs;
            }
        }
    }
    return diffs;
}

void tst_EmptyStatesAndFocus::testVisibleFocusIndicatorPushButton() {
    for (const auto theme : {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
        m_fix->context().themeManager().setThemeMode(theme);
        QApplication::processEvents();

        QDialog dlg;
        dlg.resize(300, 150);
        auto* layout = new QVBoxLayout(&dlg);
        auto* other = new QLineEdit(&dlg);
        auto* btn = new QPushButton(tr("Test Button"), &dlg);
        layout->addWidget(other);
        layout->addWidget(btn);
        dlg.show();
        activateWindow(&dlg);
        QApplication::processEvents();

        other->setFocus(Qt::TabFocusReason);
        QApplication::processEvents();
        QVERIFY(!btn->hasFocus());
        QImage unfocused(btn->size(), QImage::Format_ARGB32_Premultiplied);
        unfocused.fill(Qt::transparent);
        btn->render(&unfocused);

        btn->setFocus(Qt::TabFocusReason);
        QApplication::processEvents();
        QVERIFY(btn->hasFocus());
        QImage focused(btn->size(), QImage::Format_ARGB32_Premultiplied);
        focused.fill(Qt::transparent);
        btn->render(&focused);

        const int diff = countDifferentPixels(unfocused, focused);
        QVERIFY2(diff > 0, qPrintable(QString(u"PushButton focus indicator invisible in %1 theme"_s)
                                          .arg(theme == JobPrep::Ui::Theme::ThemeMode::Light ? u"Light"_s : u"Dark"_s)));
        dlg.close();
        QApplication::processEvents();
    }
}

void tst_EmptyStatesAndFocus::testVisibleFocusIndicatorLineEdit() {
    for (const auto theme : {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
        m_fix->context().themeManager().setThemeMode(theme);
        QApplication::processEvents();

        QDialog dlg;
        dlg.resize(300, 150);
        auto* layout = new QVBoxLayout(&dlg);
        auto* other = new QPushButton(tr("Other"), &dlg);
        auto* edit = new QLineEdit(&dlg);
        edit->setText(u"Sample text"_s);
        layout->addWidget(other);
        layout->addWidget(edit);
        dlg.show();
        activateWindow(&dlg);
        QApplication::processEvents();

        other->setFocus(Qt::TabFocusReason);
        QApplication::processEvents();
        QVERIFY(!edit->hasFocus());
        QImage unfocused(edit->size(), QImage::Format_ARGB32_Premultiplied);
        unfocused.fill(Qt::transparent);
        edit->render(&unfocused);

        edit->setFocus(Qt::TabFocusReason);
        QApplication::processEvents();
        QVERIFY(edit->hasFocus());
        QImage focused(edit->size(), QImage::Format_ARGB32_Premultiplied);
        focused.fill(Qt::transparent);
        edit->render(&focused);

        const int diff = countDifferentPixels(unfocused, focused);
        QVERIFY2(diff > 0, qPrintable(QString(u"LineEdit focus indicator invisible in %1 theme"_s)
                                          .arg(theme == JobPrep::Ui::Theme::ThemeMode::Light ? u"Light"_s : u"Dark"_s)));
        dlg.close();
        QApplication::processEvents();
    }
}

void tst_EmptyStatesAndFocus::testVisibleFocusIndicatorChip() {
    for (const auto theme : {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
        m_fix->context().themeManager().setThemeMode(theme);
        QApplication::processEvents();

        QDialog dlg;
        dlg.resize(300, 150);
        auto* layout = new QVBoxLayout(&dlg);
        auto* other = new QLineEdit(&dlg);
        auto* chip = new QPushButton(tr("All"), &dlg);
        chip->setProperty("chip", true);
        chip->setCheckable(true);
        layout->addWidget(other);
        layout->addWidget(chip);
        dlg.show();
        activateWindow(&dlg);
        QApplication::processEvents();

        other->setFocus(Qt::TabFocusReason);
        QApplication::processEvents();
        QVERIFY(!chip->hasFocus());
        QImage unfocused(chip->size(), QImage::Format_ARGB32_Premultiplied);
        unfocused.fill(Qt::transparent);
        chip->render(&unfocused);

        chip->setFocus(Qt::TabFocusReason);
        QApplication::processEvents();
        QVERIFY(chip->hasFocus());
        QImage focused(chip->size(), QImage::Format_ARGB32_Premultiplied);
        focused.fill(Qt::transparent);
        chip->render(&focused);

        const int diff = countDifferentPixels(unfocused, focused);
        QVERIFY2(diff > 0, qPrintable(QString(u"Chip focus indicator invisible in %1 theme"_s)
                                          .arg(theme == JobPrep::Ui::Theme::ThemeMode::Light ? u"Light"_s : u"Dark"_s)));
        dlg.close();
        QApplication::processEvents();
    }
}

QTEST_MAIN(tst_EmptyStatesAndFocus)
#include "tst_EmptyStatesAndFocus.moc"
