#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDateEdit>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStringList>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextEdit>
#include <QDropEvent>
#include <QMimeData>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include "app/AppContext.h"
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "data/InterviewRepository.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Structs.h"
#include "models/AgendaListModel.h"
#include "models/ApplicationTableModel.h"
#include "models/TopicBoardProxy.h"
#include "models/TopicListModel.h"
#include "services/DayWatcher.h"
#include "services/ExportService.h"
#include "services/INotifier.h"
#include "services/SeedService.h"
#include "services/SettingsService.h"
#include "services/StatsService.h"
#include "ui/DashboardController.h"
#include "ui/FormatHelper.h"
#include "ui/InAppNotifier.h"
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
#include "ui/pages/StudyPage.h"
#include "ui/theme/StatusStyle.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/ChipBar.h"
#include "ui/widgets/EmptyState.h"
#include "ui/widgets/FunnelWidget.h"
#include "ui/widgets/GoalProgressBar.h"
#include "ui/widgets/MonthView.h"
#include "ui/widgets/ProgressRing.h"
#include "ui/widgets/SelectionRestorer.h"
#include "ui/widgets/StatCard.h"
#include "ui/widgets/TopicBoardColumn.h"
#include "ui/widgets/TopicDetailPanel.h"
#include "ui/widgets/TopicListView.h"
#include "UiTestHelpers.h"

using namespace Qt::StringLiterals;
using JobPrep::Services::Notification;
using JobPrep::Services::NotificationKind;
using JobPrep::Ui::InAppNotifier;
using namespace JobPrep::UiTest;

namespace {

using JobPrep::App::AppContext;
using JobPrep::Domain::AgendaKind;
using JobPrep::Domain::ApplicationStatus;
using JobPrep::Domain::Interview;
using JobPrep::Domain::InterviewOutcome;
using JobPrep::Domain::InterviewType;
using JobPrep::Domain::JobApplication;
using JobPrep::Domain::StudySession;
using JobPrep::Domain::Subtask;
using JobPrep::Domain::Topic;
using JobPrep::Domain::TopicStatus;
using JobPrep::Domain::Track;
using JobPrep::Models::ApplicationTableModel;
using JobPrep::Ui::MainWindow;
using JobPrep::Ui::Pages::HomePage;
using JobPrep::Ui::Dialogs::ApplicationDialog;
using JobPrep::Ui::Dialogs::FollowUpDialog;
using JobPrep::Ui::Dialogs::InterviewDialog;
using JobPrep::Ui::Dialogs::SessionDialog;
using JobPrep::Ui::Dialogs::TrackDialog;

} // namespace

class tst_PageCalendar : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCalendarPageMarkersAndAgenda();
    void testCalendarLayerChipsFilterAgenda();
    void testCalendarNewInterviewFromDay();
    void testCalendarFollowUpEndToEnd();
    void testCalendarFollowUpOverwritePrompt();
    void testCalendarSplitterStatePersisted();
    void testCalendarThemes();
    void testCalendarScreenshots();
    void testCalendarJumpToStudyTopic();
    void testCalendarTopicDateAndTrackRename();
    void testCalendarTrackDeleteClearsMarkers();
    void testMonthViewDoubleClicksDayWithoutOpeningStudyTarget();

private:
    void resetCalendarData(const QDate& today);
    JobPrep::Ui::Pages::CalendarPage* calendarPage() { return m_calendar.get(); }
    JobPrep::Ui::Widgets::MonthView* monthView() const;
    QListView* agendaList() const;
    int addApplicationWithInterview(const QString& company, const QDate& day, int hour);
    int addStudyTarget(const QDate& day);

    QTemporaryDir m_tempDir;
    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::MainWindow> m_window;
    std::unique_ptr<JobPrep::App::AppContext> m_calCtx;
    std::unique_ptr<JobPrep::Ui::Pages::CalendarPage> m_calendar;
};

void tst_PageCalendar::initTestCase() {
    JobPrep::UiTest::installWarningFilter();
    JobPrep::UiTest::isolateSettings(m_tempDir);

    m_ctx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_ctx->database().isOpen());
    m_window = std::make_unique<JobPrep::Ui::MainWindow>(*m_ctx);
    m_window->show();
    QCoreApplication::processEvents();
}

void tst_PageCalendar::cleanupTestCase() {
    m_calendar.reset();
    m_calCtx.reset();
    m_window.reset();
    m_ctx.reset();
    JOBPREP_VERIFY_ZERO_WARNINGS();
    JobPrep::UiTest::uninstallWarningFilter();
}

void tst_PageCalendar::resetCalendarData(const QDate& today) {
    m_calendar.reset();
    m_calCtx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_calCtx->database().isOpen());

    // Layer visibility is persisted in QSettings, so pin it for every calendar slot.
    m_calCtx->settings().setCalendarLayerInterviews(true);
    m_calCtx->settings().setCalendarLayerFollowUps(true);
    m_calCtx->settings().setCalendarLayerTargets(true);

    m_calendar = std::make_unique<JobPrep::Ui::Pages::CalendarPage>(
        m_calCtx->interviews(), m_calCtx->applications(), m_calCtx->topics(), m_calCtx->tracks(),
        m_calCtx->agendaService(), m_calCtx->settings(), m_calCtx->themeManager(), nullptr,
        [today]() { return today; });
    m_calendar->resize(1200, 720);
    m_calendar->show();
    QCoreApplication::processEvents();
}


JobPrep::Ui::Widgets::MonthView* tst_PageCalendar::monthView() const {
    return childNamed<JobPrep::Ui::Widgets::MonthView>(m_calendar.get(), u"CalendarMonthView"_s);
}


QListView* tst_PageCalendar::agendaList() const {
    return childNamed<QListView>(m_calendar.get(), u"CalendarAgendaList"_s);
}


int tst_PageCalendar::addApplicationWithInterview(const QString& company, const QDate& day, int hour) {
    JobApplication app;
    app.company = company;
    app.position = u"Developer"_s;
    if (!m_calCtx->applications().insert(app)) {
        QTest::qFail("failed to insert application", __FILE__, __LINE__);
        return 0;
    }

    Interview interview;
    interview.applicationId = app.id;
    interview.startAt = QDateTime(day, QTime(hour, 0));
    if (!m_calCtx->interviews().insert(interview)) {
        QTest::qFail("failed to insert interview", __FILE__, __LINE__);
    }
    return app.id;
}


int tst_PageCalendar::addStudyTarget(const QDate& day) {
    Track track;
    track.name = u"English"_s;
    if (!m_calCtx->tracks().insert(track)) {
        QTest::qFail("failed to insert track", __FILE__, __LINE__);
        return 0;
    }

    Topic topic;
    topic.trackId = track.id;
    topic.title = u"Review STL containers"_s;
    topic.targetDate = day;
    if (!m_calCtx->topics().insert(topic)) {
        QTest::qFail("failed to insert topic", __FILE__, __LINE__);
    }
    return track.id;
}


void tst_PageCalendar::testCalendarPageMarkersAndAgenda() {
    const QDate today(2026, 10, 11);
    resetCalendarData(today);

    auto* view = monthView();
    auto* list = agendaList();
    QVERIFY(view != nullptr);
    QVERIFY(list != nullptr);
    QCOMPARE(view->visibleMonth(), QDate(2026, 10, 1));
    QCOMPARE(list->model()->rowCount(), 0);
    QVERIFY(!calendarPage()->hasItemsOn(today));

    // An interview lands on the selected day: one marker, one agenda row.
    const int appId = addApplicationWithInterview(u"Acme"_s, today, 10);
    QVERIFY(calendarPage()->hasItemsOn(today));
    QVERIFY(!calendarPage()->hasItemsOn(today.addDays(1)));
    QCOMPARE(view->markersFor(today).total, 1);
    QCOMPARE(view->markersFor(today).kinds.first(), AgendaKind::Interview);
    QCOMPARE(list->model()->rowCount(), 1);
    QCOMPARE(list->model()->index(0, 0).data(JobPrep::Models::AgendaListModel::KindRole).toInt(),
             static_cast<int>(AgendaKind::Interview));

    // A follow-up on the same day adds a second marker and agenda row.
    QVERIFY(m_calCtx->applications().setNextAction(appId, u"Email the recruiter"_s, today));
    QCOMPARE(view->markersFor(today).total, 2);
    QCOMPARE(view->markersFor(today).kinds.size(), 2);
    QCOMPARE(list->model()->rowCount(), 2);
}


void tst_PageCalendar::testCalendarLayerChipsFilterAgenda() {
    const QDate today(2026, 10, 11);
    resetCalendarData(today);
    const int appId = addApplicationWithInterview(u"Acme"_s, today, 10);
    QVERIFY(m_calCtx->applications().setNextAction(appId, u"Email the recruiter"_s, today));
    addStudyTarget(today);

    auto* view = monthView();
    auto* list = agendaList();
    QCOMPARE(list->model()->rowCount(), 3);
    QCOMPARE(view->markersFor(today).total, 3);

    // Hiding a layer drops its marker and its agenda row but keeps the others.
    auto* followUpChip = buttonNamed(m_calendar.get(), u"CalendarLayerFollowUpBtn"_s);
    QVERIFY(followUpChip != nullptr);
    followUpChip->click();
    QCOMPARE(m_calCtx->settings().calendarLayerFollowUps(), false);
    QCOMPARE(list->model()->rowCount(), 2);
    QCOMPARE(view->markersFor(today).total, 2);
    QVERIFY(!view->markersFor(today).kinds.contains(AgendaKind::FollowUp));
    QVERIFY(calendarPage()->hasItemsOn(today));

    auto* targetChip = buttonNamed(m_calendar.get(), u"CalendarLayerTargetBtn"_s);
    targetChip->click();
    QCOMPARE(m_calCtx->settings().calendarLayerTargets(), false);
    QCOMPARE(list->model()->rowCount(), 1);

    // Re-enabling restores everything.
    followUpChip->click();
    targetChip->click();
    QCOMPARE(list->model()->rowCount(), 3);
    QCOMPARE(view->markersFor(today).total, 3);
}


void tst_PageCalendar::testCalendarNewInterviewFromDay() {
    resetCalendarData(QDate(2026, 10, 11));
    JobApplication app;
    app.company = u"Acme"_s;
    app.position = u"Developer"_s;
    QVERIFY(m_calCtx->applications().insert(app));

    // Double-clicking a day opens one InterviewDialog preselected to that day.
    const QDate day(2026, 10, 20);
    auto* view = monthView();
    view->setVisibleMonth(day);
    QCoreApplication::processEvents();
    int dialogs = 0;
    answerModalWhenReady([&dialogs, day](QWidget* modal) {
        auto* dialog = qobject_cast<InterviewDialog*>(modal);
        QVERIFY(dialog != nullptr);
        ++dialogs;
        QCOMPARE(childNamed<QDateEdit>(dialog, u"InterviewDateEdit"_s)->date(), day);
        dialog->done(QDialog::Rejected);  // skip the "discard changes?" prompt
    });
    QTest::mouseDClick(view, Qt::LeftButton, Qt::KeyboardModifiers(), view->dayRect(day).center());
    QCOMPARE(dialogs, 1);

    // triggerNew uses the selected day.
    view->setSelectedDay(QDate(2026, 10, 25));
    withNextModal([](QWidget* modal) {
        auto* dialog = qobject_cast<InterviewDialog*>(modal);
        QVERIFY(dialog != nullptr);
        QCOMPARE(childNamed<QDateEdit>(dialog, u"InterviewDateEdit"_s)->date(), QDate(2026, 10, 25));
        dialog->done(QDialog::Rejected);
    });
    calendarPage()->triggerNew();
}


void tst_PageCalendar::testCalendarFollowUpEndToEnd() {
    const QDate today(2026, 10, 11);
    resetCalendarData(today);
    JobApplication app;
    app.company = u"Acme"_s;
    app.position = u"Developer"_s;
    QVERIFY(m_calCtx->applications().insert(app));

    auto* view = monthView();
    const QDate day(2026, 10, 15);
    view->setVisibleMonth(day);
    view->setSelectedDay(day);
    QCoreApplication::processEvents();

    withNextModal([appId = app.id](QWidget* modal) {
        auto* dialog = qobject_cast<FollowUpDialog*>(modal);
        QVERIFY(dialog != nullptr);
        auto* combo = childNamed<QComboBox>(dialog, u"FollowUpApplicationCombo"_s);
        combo->setCurrentIndex(combo->findData(appId));
        childNamed<QLineEdit>(dialog, u"FollowUpActionEdit"_s)->setText(u"Email recruiter"_s);
        buttonNamed(dialog, u"DialogSaveBtn"_s)->click();
    });
    buttonNamed(m_calendar.get(), u"CalendarAddFollowUpBtn"_s)->click();

    QVERIFY(m_calCtx->applications().byId(app.id)->nextActionDate.has_value());
    QCOMPARE(*m_calCtx->applications().byId(app.id)->nextActionDate, day);
    QCOMPARE(agendaList()->model()->rowCount(), 1);
    QCOMPARE(agendaList()->model()->index(0, 0).data(JobPrep::Models::AgendaListModel::KindRole).toInt(),
             static_cast<int>(AgendaKind::FollowUp));
    QCOMPARE(view->markersFor(day).total, 1);
}


void tst_PageCalendar::testCalendarFollowUpOverwritePrompt() {
    resetCalendarData(QDate(2026, 10, 11));
    JobApplication app;
    app.company = u"Acme"_s;
    app.position = u"Developer"_s;
    QVERIFY(m_calCtx->applications().insert(app));
    const QDate first(2026, 10, 15);
    const QDate second(2026, 10, 20);
    QVERIFY(m_calCtx->applications().setNextAction(app.id, u"First action"_s, first));

    FollowUpDialog dialog(m_calCtx->applications(), second);
    dialog.show();
    auto* combo = childNamed<QComboBox>(&dialog, u"FollowUpApplicationCombo"_s);
    combo->setCurrentIndex(combo->findData(app.id));
    childNamed<QLineEdit>(&dialog, u"FollowUpActionEdit"_s)->setText(u"Second action"_s);
    auto* saveBtn = buttonNamed(&dialog, u"DialogSaveBtn"_s);
    QVERIFY(saveBtn != nullptr);

    // Declining the overwrite prompt keeps the original date and the dialog open.
    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::No)->click(); });
    saveBtn->click();
    QCOMPARE(*m_calCtx->applications().byId(app.id)->nextActionDate, first);
    QVERIFY(dialog.isVisible());

    // Accepting replaces it.
    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::Yes)->click(); });
    saveBtn->click();
    QCOMPARE(*m_calCtx->applications().byId(app.id)->nextActionDate, second);
    QVERIFY(!dialog.isVisible());
}


void tst_PageCalendar::testCalendarSplitterStatePersisted() {
    resetCalendarData(QDate(2026, 10, 11));
    auto* splitter = childNamed<QSplitter>(m_calendar.get(), u"CalendarSplitter"_s);
    QVERIFY(splitter != nullptr);

    if (splitter->sizes().value(1) < splitter->width() / 4) {
        splitter->setSizes({splitter->width() / 2, splitter->width() / 2});
        QCoreApplication::processEvents();
    }

    // Drag handle #1 like a user would; only a real move emits splitterMoved, which is
    // what saves the state (QSplitter::moveSplitter is protected).
    auto* handle = splitter->handle(1);
    QVERIFY(handle != nullptr);
    const QPoint start(handle->width() / 2, handle->height() / 2);
    QTest::mousePress(handle, Qt::LeftButton, Qt::KeyboardModifiers(), start);
    QTest::mouseMove(handle, start + QPoint(80, 0));
    QTest::mouseRelease(handle, Qt::LeftButton, Qt::KeyboardModifiers(), start + QPoint(80, 0));
    QCoreApplication::processEvents();
    const QList<int> movedSizes = splitter->sizes();
    const QByteArray saved = m_calCtx->settings().calendarSplitterState();
    QVERIFY(!saved.isEmpty());

    // A fresh page in the same context restores exactly those sizes.
    JobPrep::Ui::Pages::CalendarPage second(
        m_calCtx->interviews(), m_calCtx->applications(), m_calCtx->topics(), m_calCtx->tracks(),
        m_calCtx->agendaService(), m_calCtx->settings(), m_calCtx->themeManager(), nullptr,
        [] { return QDate(2026, 10, 11); });
    second.resize(1200, 720);
    second.show();
    QCoreApplication::processEvents();
    auto* restored = childNamed<QSplitter>(&second, u"CalendarSplitter"_s);
    QVERIFY(restored != nullptr);
    QCOMPARE(restored->sizes(), movedSizes);
}


void tst_PageCalendar::testCalendarThemes() {
    const QDate today(2026, 10, 11);
    resetCalendarData(today);
    const int appId = addApplicationWithInterview(u"Acme"_s, today, 10);
    QVERIFY(m_calCtx->applications().setNextAction(appId, u"Email the recruiter"_s, today));
    addStudyTarget(today);

    for (const auto mode :
         {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
        m_calCtx->themeManager().setThemeMode(mode);
        QCoreApplication::processEvents();
        QCOMPARE(monthView()->markersFor(today).total, 3);
        QCOMPARE(agendaList()->model()->rowCount(), 3);
    }
}


void tst_PageCalendar::testCalendarScreenshots() {
    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
    QVERIFY(QDir().mkpath(dir));

    const QDate today(2026, 10, 11);
    resetCalendarData(today);

    const auto save = [this, dir](JobPrep::Ui::Theme::ThemeMode mode, const QString& name) {
        m_calCtx->themeManager().setThemeMode(mode);
        QCoreApplication::processEvents();
        const QImage image = m_calendar->grab().toImage();
        QVERIFY(!image.isNull());
        QVERIFY(image.save(QDir(dir).filePath(name + u".png"_s)));
    };
    const auto forBothThemes = [&save](const QString& name) {
        save(JobPrep::Ui::Theme::ThemeMode::Light, name + u"-light"_s);
        save(JobPrep::Ui::Theme::ThemeMode::Dark, name + u"-dark"_s);
    };

    // Empty: no agenda data, the "Nothing Planned" state is shown.
    forBothThemes(u"calendar-empty"_s);

    // With markers: interviews, a follow-up and a study target across the month.
    addApplicationWithInterview(u"Google"_s, today, 10);
    const int second = addApplicationWithInterview(u"DeepMind"_s, today.addDays(3), 14);
    QVERIFY(m_calCtx->applications().setNextAction(second, u"Send thank-you email"_s,
                                                   today.addDays(1)));
    addStudyTarget(today.addDays(2));
    forBothThemes(u"calendar-markers"_s);

    // With a selected day: the agenda panel is filled for a specific interview day.
    monthView()->setSelectedDay(today.addDays(3));
    QCoreApplication::processEvents();
    forBothThemes(u"calendar-selected"_s);
}


void tst_PageCalendar::testCalendarJumpToStudyTopic() {
    const QDate day(2026, 10, 20);
    Track track;
    track.name = u"System Design"_s;
    track.color = u"#0d9488"_s;
    QVERIFY(m_ctx->tracks().insert(track));
    const int trackId = m_ctx->tracks().all().last().id;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"Rate Limiter"_s;
    topic.targetDate = day;
    QVERIFY(m_ctx->topics().insert(topic));
    const int topicId = m_ctx->topics().all().last().id;

    // Switch to Calendar
    m_window->sidebar()->setCurrentPage(3);
    emit m_window->sidebar()->pageSelected(3);
    QCOMPARE(m_window->pageStack()->currentIndex(), 3);

    // Verify SPEC C5: Double-clicking a day with study target still opens InterviewDialog
    int interviewDialogs = 0;
    answerModalWhenReady([&interviewDialogs](QWidget* modal) {
        if (auto* dlg = qobject_cast<JobPrep::Ui::Dialogs::InterviewDialog*>(modal)) {
            interviewDialogs++;
            dlg->done(QDialog::Rejected);
        }
    });
    auto* calPage = qobject_cast<JobPrep::Ui::Pages::CalendarPage*>(m_window->pageStack()->currentWidget());
    QVERIFY(calPage != nullptr);
    auto* mv = calPage->findChild<JobPrep::Ui::Widgets::MonthView*>();
    QVERIFY(mv != nullptr);
    mv->setVisibleMonth(day);
    QTest::mouseDClick(mv, Qt::LeftButton, Qt::KeyboardModifiers(), mv->dayRect(day).center());
    QCOMPARE(interviewDialogs, 1);

    // Call openTopic(topicId) in Board view
    m_window->openTopic(topicId);
    QCoreApplication::processEvents();

    // Verify switched to Study page (index 1) and topic is selected
    QCOMPARE(m_window->pageStack()->currentIndex(), 1);
    auto* study = qobject_cast<JobPrep::Ui::Pages::StudyPage*>(m_window->pageStack()->currentWidget());
    QVERIFY(study != nullptr);
    QCOMPARE(study->selectedTopicId(), std::optional<int>(topicId));
    QVERIFY(study->isDetailVisible());

    // Test jumping while Study is in List view (works in both views)
    study->setViewMode(u"list"_s);
    m_window->sidebar()->setCurrentPage(3);
    emit m_window->sidebar()->pageSelected(3);
    m_window->openTopic(topicId);
    QCoreApplication::processEvents();
    QCOMPARE(m_window->pageStack()->currentIndex(), 1);
    QCOMPARE(study->viewMode(), u"list"_s);
    QCOMPARE(study->selectedTopicId(), std::optional<int>(topicId));
    QVERIFY(study->isDetailVisible());
    study->setViewMode(u"board"_s);

    // Now test jump when topic is hidden by track filter and search text
    Track track2;
    track2.name = u"Databases"_s;
    track2.color = u"#e11d48"_s;
    QVERIFY(m_ctx->tracks().insert(track2));
    const int track2Id = m_ctx->tracks().all().last().id;

    auto* studyChips = childNamed<JobPrep::Ui::Widgets::ChipBar>(study, u"StudyTrackChips"_s);
    if (studyChips && studyChips->button(track2Id)) studyChips->button(track2Id)->click();
    study->setSearchText(u"nonexistent_xyz"_s);
    QCoreApplication::processEvents();

    m_window->openTopic(topicId);
    QCoreApplication::processEvents();

    QCOMPARE(study->searchText(), QString());
    auto* chips = childNamed<JobPrep::Ui::Widgets::ChipBar>(study, u"StudyTrackChips"_s);
    QCOMPARE(chips->currentId(), -1);
    QCOMPARE(study->selectedTopicId(), std::optional<int>(topicId));

    // Test non-fatal message when topic was deleted
    QVERIFY(m_ctx->topics().remove(topicId));
    m_window->openTopic(topicId);
    QCoreApplication::processEvents();
    // Does not crash, stays responsive
    QCOMPARE(m_window->pageStack()->currentIndex(), 1);
}


void tst_PageCalendar::testCalendarTopicDateAndTrackRename() {
    const QDate today(2026, 10, 11);
    resetCalendarData(today);

    Track track;
    track.name = u"Algorithms Track"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(m_calCtx->tracks().insert(track));
    const int trackId = m_calCtx->tracks().all().last().id;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"Dynamic Programming"_s;
    topic.targetDate = today;
    QVERIFY(m_calCtx->topics().insert(topic));

    m_calCtx->topics().changed();
    QCoreApplication::processEvents();

    // Verify calendar month view has target marker
    QVERIFY(monthView()->markersFor(today).kinds.contains(JobPrep::Domain::AgendaKind::StudyTarget));

    // Rename track -> agenda subtitle updates
    track.name = u"Advanced DP"_s;
    QVERIFY(m_calCtx->tracks().update(track));
    m_calCtx->tracks().changed();
    QCoreApplication::processEvents();

    auto* agenda = agendaList();
    QVERIFY(agenda != nullptr);
    auto* agendaModel = agenda->model();
    QVERIFY(agendaModel != nullptr);
    QVERIFY(agendaModel->rowCount() > 0);
    bool foundUpdatedSubtitle = false;
    for (int r = 0; r < agendaModel->rowCount(); ++r) {
        const QString sub = agendaModel->index(r, 0).data(JobPrep::Models::AgendaListModel::SubtitleRole).toString();
        if (sub.contains(u"Advanced DP"_s)) {
            foundUpdatedSubtitle = true;
            break;
        }
    }
    QVERIFY(foundUpdatedSubtitle);

    // Delete track -> cascade deletes topic, markers clear
    QVERIFY(m_calCtx->tracks().remove(trackId));
    m_calCtx->tracks().changed();
    m_calCtx->topics().changed();
    QCoreApplication::processEvents();

    QVERIFY(!monthView()->markersFor(today).kinds.contains(JobPrep::Domain::AgendaKind::StudyTarget));
}


void tst_PageCalendar::testCalendarTrackDeleteClearsMarkers() {
    const QDate today(2026, 10, 11);
    resetCalendarData(today);

    Track track;
    track.name = u"Track Cascade Markers"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(m_calCtx->tracks().insert(track));
    const int trackId = m_calCtx->tracks().all().last().id;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"Target Marker Topic"_s;
    topic.targetDate = today;
    QVERIFY(m_calCtx->topics().insert(topic));

    m_calCtx->topics().changed();
    QCoreApplication::processEvents();

    QVERIFY(monthView()->markersFor(today).kinds.contains(JobPrep::Domain::AgendaKind::StudyTarget));

    // Deleting track ONLY (triggers tracks().changed() which cascades in DB and signals CalendarPage)
    QVERIFY(m_calCtx->tracks().remove(trackId));
    m_calCtx->tracks().changed();
    QCoreApplication::processEvents();

    QVERIFY(!monthView()->markersFor(today).kinds.contains(JobPrep::Domain::AgendaKind::StudyTarget));
}


void tst_PageCalendar::testMonthViewDoubleClicksDayWithoutOpeningStudyTarget() {
    const QDate day(2026, 10, 22);
    Track track;
    track.name = u"Interview Target Track"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(m_ctx->tracks().insert(track));
    const int trackId = m_ctx->tracks().all().last().id;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"System Architecture"_s;
    topic.targetDate = day;
    QVERIFY(m_ctx->topics().insert(topic));

    // Switch to Calendar
    m_window->sidebar()->setCurrentPage(3);
    emit m_window->sidebar()->pageSelected(3);
    QCOMPARE(m_window->pageStack()->currentIndex(), 3);

    auto* calPage = qobject_cast<JobPrep::Ui::Pages::CalendarPage*>(m_window->pageStack()->currentWidget());
    QVERIFY(calPage != nullptr);
    auto* mv = calPage->findChild<JobPrep::Ui::Widgets::MonthView*>();
    QVERIFY(mv != nullptr);
    mv->setVisibleMonth(day);

    // Double-click on day having a study target opens InterviewDialog per SPEC C5, never jumps to Study
    int interviewDialogs = 0;
    answerModalWhenReady([&interviewDialogs](QWidget* modal) {
        if (auto* dlg = qobject_cast<JobPrep::Ui::Dialogs::InterviewDialog*>(modal)) {
            interviewDialogs++;
            dlg->done(QDialog::Rejected);
        }
    });

    QTest::mouseDClick(mv, Qt::LeftButton, Qt::KeyboardModifiers(), mv->dayRect(day).center());
    QCOMPARE(interviewDialogs, 1);
    // MainWindow remains on Calendar page (did not jump to Study)
    QCOMPARE(m_window->pageStack()->currentIndex(), 3);
}


QTEST_MAIN(tst_PageCalendar)
#include "tst_PageCalendar.moc"
