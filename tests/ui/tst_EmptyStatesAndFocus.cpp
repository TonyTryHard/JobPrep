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
    void testTabOrderDialogApplicationOverview();
    void testTabOrderDialogApplicationInterviewsAndNotes();
    void testTabOrderDialogInterview();
    void testTabOrderDialogFollowUp();
    void testTabOrderDialogSession();
    void testTabOrderDialogTrack();
    void testTabOrderPageHome();
    void testTabOrderPageJobs();
    void testTabOrderPageCalendar();
    void testTabOrderPageSettings();

    // D3 Visible focus indicator painted tests
    void testVisibleFocusIndicatorPushButton();
    void testVisibleFocusIndicatorLineEdit();
    void testVisibleFocusIndicatorChip();

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
    const bool updateOk = m_fix->context().applications().update(app);
    const auto historyAfter = m_fix->context().applications().statusHistory(app.id);
    qDebug() << "updateOk:" << updateOk << "historyBefore:" << historyBefore.size() << "historyAfter:" << historyAfter.size();
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

void tst_EmptyStatesAndFocus::testTabOrderDialogApplicationOverview() {
    JobPrep::Ui::Dialogs::ApplicationDialog dlg(m_fix->context().applications(),
                                              m_fix->context().interviews(),
                                              JobPrep::Ui::Dialogs::ApplicationDialog::Create);
    dlg.show();
    QApplication::processEvents();

    auto* companyEdit = dlg.findChild<QLineEdit*>(u"FieldCompany"_s);
    QVERIFY(companyEdit);
    companyEdit->setFocus();

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

    QWidget* cur = companyEdit;
    for (const auto& expectedName : expected) {
        QVERIFY(cur);
        QCOMPARE(cur->objectName(), expectedName);
        QVERIFY(cur->focusPolicy() != Qt::NoFocus);
        cur = cur->nextInFocusChain();
        while (cur && cur != companyEdit &&
               (!cur->isVisibleTo(cur->window()) || cur->focusPolicy() == Qt::NoFocus ||
                (cur->parentWidget() && qobject_cast<QAbstractSpinBox*>(cur->parentWidget())) ||
                (cur->parentWidget() && qobject_cast<QComboBox*>(cur->parentWidget())))) {
            cur = cur->nextInFocusChain();
        }
    }
    dlg.reject();
}

void tst_EmptyStatesAndFocus::testTabOrderDialogApplicationInterviewsAndNotes() {
    JobPrep::Domain::JobApplication app;
    app.company = u"Example"_s;
    app.position = u"Dev"_s;
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

    // Switch to Interviews tab (tab 2)
    tabs->setCurrentIndex(2);
    QApplication::processEvents();

    auto* interviewsTable = dlg.findChild<QTableWidget*>(u"InterviewsTable"_s);
    auto* addInterviewBtn = dlg.findChild<QPushButton*>(u"InterviewsAddBtn"_s);
    auto* saveBtn = dlg.findChild<QPushButton*>(u"DialogSaveBtn"_s);
    QVERIFY(interviewsTable);
    QVERIFY(addInterviewBtn);
    QVERIFY(saveBtn);
    QVERIFY(interviewsTable->focusPolicy() != Qt::NoFocus);
    QVERIFY(addInterviewBtn->focusPolicy() != Qt::NoFocus);
    QVERIFY(saveBtn->focusPolicy() != Qt::NoFocus);

    // Switch to Notes tab (tab 1)
    tabs->setCurrentIndex(1);
    QApplication::processEvents();

    auto* notesEdit = dlg.findChild<QTextEdit*>(u"FieldNotes"_s);
    QVERIFY(notesEdit);
    QVERIFY(notesEdit->focusPolicy() != Qt::NoFocus);

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
    QApplication::processEvents();

    auto* appCombo = dlg.findChild<QComboBox*>(u"InterviewApplicationCombo"_s);
    QVERIFY(appCombo);
    appCombo->setFocus();

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

    QWidget* cur = appCombo;
    for (const auto& expectedName : expected) {
        QVERIFY(cur);
        QCOMPARE(cur->objectName(), expectedName);
        QVERIFY(cur->focusPolicy() != Qt::NoFocus);
        cur = cur->nextInFocusChain();
        while (cur && cur != appCombo &&
               (!cur->isVisibleTo(cur->window()) || cur->focusPolicy() == Qt::NoFocus ||
                (cur->parentWidget() && qobject_cast<QAbstractSpinBox*>(cur->parentWidget())))) {
            cur = cur->nextInFocusChain();
        }
    }
    dlg.reject();
}

void tst_EmptyStatesAndFocus::testTabOrderDialogFollowUp() {
    JobPrep::Domain::JobApplication app;
    app.company = u"Apple"_s;
    app.position = u"Firmware Dev"_s;
    m_fix->context().applications().insert(app);

    JobPrep::Ui::Dialogs::FollowUpDialog dlg(m_fix->context().applications(), QDate::currentDate());
    dlg.show();
    QApplication::processEvents();

    auto* appCombo = dlg.findChild<QComboBox*>(u"FollowUpApplicationCombo"_s);
    auto* actionEdit = dlg.findChild<QLineEdit*>(u"FollowUpActionEdit"_s);
    auto* saveBtn = dlg.findChild<QPushButton*>(u"DialogSaveBtn"_s);
    QVERIFY(appCombo);
    QVERIFY(actionEdit);
    QVERIFY(saveBtn);

    QVERIFY(appCombo->focusPolicy() != Qt::NoFocus);
    QVERIFY(actionEdit->focusPolicy() != Qt::NoFocus);
    QVERIFY(saveBtn->focusPolicy() != Qt::NoFocus);
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
    QApplication::processEvents();

    auto* dateEdit = dlg.findChild<QDateEdit*>(u"SessionDateEdit"_s);
    auto* minSpin = dlg.findChild<QSpinBox*>(u"SessionMinutesSpin"_s);
    auto* topicCombo = dlg.findChild<QComboBox*>(u"SessionTopicCombo"_s);
    auto* noteEdit = dlg.findChild<QTextEdit*>(u"SessionNoteEdit"_s);
    auto* saveBtn = dlg.findChild<QPushButton*>(u"SessionSaveBtn"_s);
    auto* cancelBtn = dlg.findChild<QPushButton*>(u"SessionCancelBtn"_s);
    QVERIFY(dateEdit);
    QVERIFY(minSpin);
    QVERIFY(topicCombo);
    QVERIFY(noteEdit);
    QVERIFY(saveBtn);
    QVERIFY(cancelBtn);

    QVERIFY(dateEdit->focusPolicy() != Qt::NoFocus);
    QVERIFY(minSpin->focusPolicy() != Qt::NoFocus);
    QVERIFY(topicCombo->focusPolicy() != Qt::NoFocus);
    QVERIFY(noteEdit->focusPolicy() != Qt::NoFocus);
    QVERIFY(saveBtn->focusPolicy() != Qt::NoFocus);
    QVERIFY(cancelBtn->focusPolicy() != Qt::NoFocus);
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
    QApplication::processEvents();

    auto* list = dlg.findChild<QListWidget*>(u"TrackListWidget"_s);
    auto* newBtn = dlg.findChild<QPushButton*>(u"TrackNewBtn"_s);
    auto* deleteBtn = dlg.findChild<QPushButton*>(u"TrackDeleteBtn"_s);
    auto* nameEdit = dlg.findChild<QLineEdit*>(u"TrackNameEdit"_s);
    auto* saveBtn = dlg.findChild<QPushButton*>(u"TrackSaveBtn"_s);
    auto* closeBtn = dlg.findChild<QPushButton*>(u"TrackCloseBtn"_s);
    QVERIFY(list);
    QVERIFY(newBtn);
    QVERIFY(deleteBtn);
    QVERIFY(nameEdit);
    QVERIFY(saveBtn);
    QVERIFY(closeBtn);

    QVERIFY(list->focusPolicy() != Qt::NoFocus);
    QVERIFY(newBtn->focusPolicy() != Qt::NoFocus);
    QVERIFY(nameEdit->focusPolicy() != Qt::NoFocus);
    QVERIFY(saveBtn->focusPolicy() != Qt::NoFocus);
    QVERIFY(closeBtn->focusPolicy() != Qt::NoFocus);
    dlg.reject();
}

void tst_EmptyStatesAndFocus::testTabOrderPageHome() {
    auto* home = m_fix->window().findChild<JobPrep::Ui::Pages::HomePage*>();
    QVERIFY(home);
    emit m_fix->window().sidebar()->pageSelected(0);
    home->onActivated();
    QApplication::processEvents();

    auto* todayView = home->findChild<QListView*>();
    QVERIFY(todayView);
    QVERIFY(todayView->focusPolicy() != Qt::NoFocus);
}

void tst_EmptyStatesAndFocus::testTabOrderPageJobs() {
    auto* jobs = m_fix->window().findChild<JobPrep::Ui::Pages::JobsPage*>();
    QVERIFY(jobs);
    emit m_fix->window().sidebar()->pageSelected(2);
    jobs->onActivated();
    QApplication::processEvents();

    auto* searchEdit = jobs->findChild<QLineEdit*>(u"JobsSearchEdit"_s);
    auto* chips = jobs->findChild<JobPrep::Ui::Widgets::ChipBar*>(u"JobsStatusChips"_s);
    auto* exportBtn = jobs->findChild<QPushButton*>(u"JobsExportBtn"_s);
    auto* view = jobs->findChild<QTableView*>(u"JobsTableView"_s);
    QVERIFY(searchEdit);
    QVERIFY(chips);
    QVERIFY(exportBtn);
    QVERIFY(view);

    QVERIFY(searchEdit->focusPolicy() != Qt::NoFocus);
    QVERIFY(exportBtn->focusPolicy() != Qt::NoFocus);
}

void tst_EmptyStatesAndFocus::testTabOrderPageCalendar() {
    auto* cal = m_fix->window().findChild<JobPrep::Ui::Pages::CalendarPage*>();
    QVERIFY(cal);
    emit m_fix->window().sidebar()->pageSelected(3);
    cal->onActivated();
    QApplication::processEvents();

    auto* prevBtn = cal->findChild<QToolButton*>(u"CalendarPrevBtn"_s);
    auto* nextBtn = cal->findChild<QToolButton*>(u"CalendarNextBtn"_s);
    auto* todayBtn = cal->findChild<QPushButton*>(u"CalendarTodayBtn"_s);
    auto* addInterviewBtn = cal->findChild<QPushButton*>(u"CalendarAddInterviewBtn"_s);
    auto* addFollowUpBtn = cal->findChild<QPushButton*>(u"CalendarAddFollowUpBtn"_s);
    auto* agendaView = cal->findChild<QListView*>(u"CalendarAgendaList"_s);
    QVERIFY(prevBtn);
    QVERIFY(nextBtn);
    QVERIFY(todayBtn);
    QVERIFY(addInterviewBtn);
    QVERIFY(addFollowUpBtn);
    QVERIFY(agendaView);

    QVERIFY(prevBtn->focusPolicy() != Qt::NoFocus);
    QVERIFY(nextBtn->focusPolicy() != Qt::NoFocus);
    QVERIFY(todayBtn->focusPolicy() != Qt::NoFocus);
    QVERIFY(addInterviewBtn->focusPolicy() != Qt::NoFocus);
    QVERIFY(addFollowUpBtn->focusPolicy() != Qt::NoFocus);
    QVERIFY(agendaView->focusPolicy() != Qt::NoFocus);
}

void tst_EmptyStatesAndFocus::testTabOrderPageSettings() {
    auto* settings = m_fix->window().findChild<JobPrep::Ui::Pages::SettingsPage*>();
    QVERIFY(settings);
    emit m_fix->window().sidebar()->pageSelected(4);
    settings->onActivated();
    QApplication::processEvents();

    auto* themeCombo = settings->findChild<QComboBox*>(u"SettingsThemeCombo"_s);
    auto* accentCombo = settings->findChild<QComboBox*>(u"SettingsAccentCombo"_s);
    auto* goalSpin = settings->findChild<QSpinBox*>(u"SettingsStudyGoalSpinBox"_s);
    auto* exportCsvBtn = settings->findChild<QPushButton*>(u"SettingsExportCsvBtn"_s);
    auto* exportIcsBtn = settings->findChild<QPushButton*>(u"SettingsExportIcsBtn"_s);
    auto* backupJsonBtn = settings->findChild<QPushButton*>(u"SettingsBackupJsonBtn"_s);
    auto* restoreJsonBtn = settings->findChild<QPushButton*>(u"SettingsRestoreJsonBtn"_s);
    auto* sampleBtn = settings->findChild<QPushButton*>(u"SettingsLoadSampleBtn"_s);
    auto* openFolderBtn = settings->findChild<QPushButton*>(u"SettingsOpenFolderBtn"_s);
    QVERIFY(themeCombo);
    QVERIFY(accentCombo);
    QVERIFY(goalSpin);
    QVERIFY(exportCsvBtn);
    QVERIFY(exportIcsBtn);
    QVERIFY(backupJsonBtn);
    QVERIFY(restoreJsonBtn);
    QVERIFY(sampleBtn);
    QVERIFY(openFolderBtn);

    QVERIFY(themeCombo->focusPolicy() != Qt::NoFocus);
    QVERIFY(accentCombo->focusPolicy() != Qt::NoFocus);
    QVERIFY(goalSpin->focusPolicy() != Qt::NoFocus);
    QVERIFY(backupJsonBtn->focusPolicy() != Qt::NoFocus);
    QVERIFY(restoreJsonBtn->focusPolicy() != Qt::NoFocus);
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
