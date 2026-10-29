#include <QApplication>
#include <QKeySequence>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <QTextEdit>
#include <QTimer>

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
#include "ui/pages/CalendarPage.h"
#include "ui/pages/HomePage.h"
#include "ui/pages/JobsPage.h"
#include "ui/pages/SettingsPage.h"
#include "ui/pages/StudyPage.h"
#include "ui/widgets/TopicBoardColumn.h"
#include "ui/widgets/TopicDetailPanel.h"
#include "UiTestHelpers.h"

using namespace Qt::StringLiterals;
using namespace JobPrep::UiTest;

class tst_Shortcuts : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // D1 keyboard shortcuts suites
    void testPageShortcutsCtrl1To4();
    void testPreferencesShortcutFromAllPages();
    void testPageShortcutsWithFocusInLineEditAndTextEdit();
    void testCtrlNPerCurrentPage();
    void testCtrlFPerCurrentPage();
    void testCtrlSSavesInStudyDetailPanel();
    void testCtrlSSavesInDialogs();
    void testEscHandlingAcrossWidgets();

    // D1 collisions
    void testCollisionModalBlocksWindowShortcuts();
    void testCollisionCtrlSInsideTextEdit();
    void testCollisionCtrlNInsideLineEdit();
    void testCollisionAltArrowsOnBoard();

private:
    QTemporaryDir m_tempDir;
    std::unique_ptr<WindowFixture> m_fix;
};

void tst_Shortcuts::initTestCase() {
    installWarningFilter();
    isolateSettings(m_tempDir);
}

void tst_Shortcuts::cleanupTestCase() {
    uninstallWarningFilter();
}

void tst_Shortcuts::init() {
    clearCapturedWarnings();
    m_fix = std::make_unique<WindowFixture>();
    QVERIFY(activateWindow(&m_fix->window()));
}

void tst_Shortcuts::cleanup() {
    m_fix.reset();
    JOBPREP_VERIFY_ZERO_WARNINGS();
}

void tst_Shortcuts::testPageShortcutsCtrl1To4() {
    auto& win = m_fix->window();
    auto* stack = win.findChild<QStackedWidget*>(u"PageStack"_s);
    QVERIFY(stack != nullptr);

    // Initial page is Home (0)
    QCOMPARE(stack->currentIndex(), 0);

    // Ctrl+2 -> Study (1)
    QVERIFY(sendKeySequence(&win, QKeySequence(Qt::CTRL | Qt::Key_2)));
    QCOMPARE(stack->currentIndex(), 1);

    // Ctrl+3 -> Jobs (2)
    QVERIFY(sendKeySequence(&win, QKeySequence(Qt::CTRL | Qt::Key_3)));
    QCOMPARE(stack->currentIndex(), 2);

    // Ctrl+4 -> Calendar (3)
    QVERIFY(sendKeySequence(&win, QKeySequence(Qt::CTRL | Qt::Key_4)));
    QCOMPARE(stack->currentIndex(), 3);

    // Ctrl+1 -> Home (0)
    QVERIFY(sendKeySequence(&win, QKeySequence(Qt::CTRL | Qt::Key_1)));
    QCOMPARE(stack->currentIndex(), 0);

    // Cross-page navigation: Calendar directly to Study
    emit win.sidebar()->pageSelected(3);
    QCOMPARE(stack->currentIndex(), 3);
    QVERIFY(sendKeySequence(&win, QKeySequence(Qt::CTRL | Qt::Key_2)));
    QCOMPARE(stack->currentIndex(), 1);

    // Study directly to Jobs
    QVERIFY(sendKeySequence(&win, QKeySequence(Qt::CTRL | Qt::Key_3)));
    QCOMPARE(stack->currentIndex(), 2);
}

void tst_Shortcuts::testPreferencesShortcutFromAllPages() {
    auto& win = m_fix->window();
    auto* stack = win.findChild<QStackedWidget*>(u"PageStack"_s);
    QVERIFY(stack != nullptr);

    // From Home (0) -> Settings (4) via Ctrl+,
    emit win.sidebar()->pageSelected(0);
    QCOMPARE(stack->currentIndex(), 0);
    QVERIFY(sendKeySequence(&win, QKeySequence(Qt::CTRL | Qt::Key_Comma)));
    QCOMPARE(stack->currentIndex(), 4);

    // From Study (1) -> Settings (4)
    emit win.sidebar()->pageSelected(1);
    QCOMPARE(stack->currentIndex(), 1);
    QVERIFY(sendKeySequence(&win, QKeySequence(Qt::CTRL | Qt::Key_Comma)));
    QCOMPARE(stack->currentIndex(), 4);

    // From Jobs (2) -> Settings (4)
    emit win.sidebar()->pageSelected(2);
    QCOMPARE(stack->currentIndex(), 2);
    QVERIFY(sendKeySequence(&win, QKeySequence(Qt::CTRL | Qt::Key_Comma)));
    QCOMPARE(stack->currentIndex(), 4);

    // From Calendar (3) -> Settings (4)
    emit win.sidebar()->pageSelected(3);
    QCOMPARE(stack->currentIndex(), 3);
    QVERIFY(sendKeySequence(&win, QKeySequence(Qt::CTRL | Qt::Key_Comma)));
    QCOMPARE(stack->currentIndex(), 4);
}

void tst_Shortcuts::testPageShortcutsWithFocusInLineEditAndTextEdit() {
    auto& win = m_fix->window();
    auto* stack = win.findChild<QStackedWidget*>(u"PageStack"_s);
    QVERIFY(stack != nullptr);

    // Go to Jobs page and focus its search line edit
    emit win.sidebar()->pageSelected(2);
    QCOMPARE(stack->currentIndex(), 2);
    auto* jobsSearch = win.findChild<QLineEdit*>(u"JobsSearchEdit"_s);
    QVERIFY(jobsSearch != nullptr);
    jobsSearch->setFocus();
    QVERIFY(jobsSearch->hasFocus());

    // Ctrl+1 from within focused line edit switches to Home (0)
    QVERIFY(sendKeySequence(&win, QKeySequence(Qt::CTRL | Qt::Key_1)));
    QCOMPARE(stack->currentIndex(), 0);

    // Focus header search edit on Home page
    auto* headerSearch = win.findChild<QLineEdit*>(u"HeaderSearchEdit"_s);
    QVERIFY(headerSearch != nullptr);
    headerSearch->setFocus();
    QVERIFY(headerSearch->hasFocus());

    // Ctrl+, switches to Settings (4)
    QVERIFY(sendKeySequence(&win, QKeySequence(Qt::CTRL | Qt::Key_Comma)));
    QCOMPARE(stack->currentIndex(), 4);
}

void tst_Shortcuts::testCtrlNPerCurrentPage() {
    auto& win = m_fix->window();
    auto* stack = win.findChild<QStackedWidget*>(u"PageStack"_s);
    QVERIFY(stack != nullptr);

    // 1. Home page (0): Ctrl+N opens ApplicationDialog
    emit win.sidebar()->pageSelected(0);
    QCOMPARE(stack->currentIndex(), 0);
    bool dialogHandled = false;
    answerModalWhenReady([&dialogHandled](QWidget* modal) {
        if (qobject_cast<JobPrep::Ui::Dialogs::ApplicationDialog*>(modal)) {
            dialogHandled = true;
            modal->close();
        }
    });
    QVERIFY(sendKeySequence(&win, QKeySequence(QKeySequence::New)));
    QTest::qWait(50);
    QVERIFY(dialogHandled);

    // 2. Jobs page (2): Ctrl+N opens ApplicationDialog
    emit win.sidebar()->pageSelected(2);
    QCOMPARE(stack->currentIndex(), 2);
    dialogHandled = false;
    answerModalWhenReady([&dialogHandled](QWidget* modal) {
        if (qobject_cast<JobPrep::Ui::Dialogs::ApplicationDialog*>(modal)) {
            dialogHandled = true;
            modal->close();
        }
    });
    QVERIFY(sendKeySequence(&win, QKeySequence(QKeySequence::New)));
    QTest::qWait(50);
    QVERIFY(dialogHandled);

    // 3. Calendar page (3): Ctrl+N opens InterviewDialog
    emit win.sidebar()->pageSelected(3);
    QCOMPARE(stack->currentIndex(), 3);
    dialogHandled = false;
    answerModalWhenReady([&dialogHandled](QWidget* modal) {
        if (qobject_cast<JobPrep::Ui::Dialogs::InterviewDialog*>(modal)) {
            dialogHandled = true;
            modal->close();
        }
    });
    QVERIFY(sendKeySequence(&win, QKeySequence(QKeySequence::New)));
    QTest::qWait(50);
    QVERIFY(dialogHandled);

    // 4. Study page (1): with tracks seeded, Ctrl+N triggers newTopic
    // Seed one track so topic creation can proceed without opening TrackDialog
    JobPrep::Domain::Track track;
    track.name = u"C++"_s;
    track.color = u"#5B6CFF"_s;
    m_fix->context().tracks().insert(track);

    emit win.sidebar()->pageSelected(1);
    QCOMPARE(stack->currentIndex(), 1);
    // On Study page, newTopic focuses the Backlog quick-add line
    auto* backlogCol = win.findChild<JobPrep::Ui::Widgets::TopicBoardColumn*>(u"StudyColumnBacklog"_s);
    QVERIFY(backlogCol != nullptr);
    auto* quickAdd = backlogCol->findChild<QLineEdit*>(u"StudyColumnQuickAdd"_s);
    QVERIFY(quickAdd != nullptr);

    QVERIFY(sendKeySequence(&win, QKeySequence(QKeySequence::New)));
    QTest::qWait(50);
    QVERIFY(quickAdd->hasFocus());

    // 5. Settings page (4): Ctrl+N is disabled / does not trigger any dialog
    emit win.sidebar()->pageSelected(4);
    QCOMPARE(stack->currentIndex(), 4);
    const int appDialogsBefore = countApplicationDialogs();
    const int interviewDialogsBefore = countInterviewDialogs();
    sendKeySequence(&win, QKeySequence(QKeySequence::New));
    QTest::qWait(50);
    QCOMPARE(countApplicationDialogs(), appDialogsBefore);
    QCOMPARE(countInterviewDialogs(), interviewDialogsBefore);
}

void tst_Shortcuts::testCtrlFPerCurrentPage() {
    auto& win = m_fix->window();
    auto* stack = win.findChild<QStackedWidget*>(u"PageStack"_s);
    QVERIFY(stack != nullptr);

    // 1. Study page (1): Ctrl+F focuses and selects Study search box
    emit win.sidebar()->pageSelected(1);
    QCOMPARE(stack->currentIndex(), 1);
    auto* studySearch = win.findChild<QLineEdit*>(u"StudySearchEdit"_s);
    QVERIFY(studySearch != nullptr);
    studySearch->setText(u"algorithm"_s);
    studySearch->clearFocus();
    QVERIFY(!studySearch->hasFocus());

    QVERIFY(sendKeySequence(&win, QKeySequence(QKeySequence::Find)));
    QVERIFY(studySearch->hasFocus());
    QCOMPARE(studySearch->selectedText(), u"algorithm"_s);

    // 2. Jobs page (2): Ctrl+F focuses and selects Jobs search box
    emit win.sidebar()->pageSelected(2);
    QCOMPARE(stack->currentIndex(), 2);
    auto* jobsSearch = win.findChild<QLineEdit*>(u"JobsSearchEdit"_s);
    QVERIFY(jobsSearch != nullptr);
    jobsSearch->setText(u"engineer"_s);
    jobsSearch->clearFocus();
    QVERIFY(!jobsSearch->hasFocus());

    QVERIFY(sendKeySequence(&win, QKeySequence(QKeySequence::Find)));
    QVERIFY(jobsSearch->hasFocus());
    QCOMPARE(jobsSearch->selectedText(), u"engineer"_s);

    // 3. Settings page (4): Ctrl+F does nothing
    emit win.sidebar()->pageSelected(4);
    QCOMPARE(stack->currentIndex(), 4);
    studySearch->clearFocus();
    jobsSearch->clearFocus();
    sendKeySequence(&win, QKeySequence(QKeySequence::Find));
    QVERIFY(!studySearch->hasFocus());
    QVERIFY(!jobsSearch->hasFocus());
}

void tst_Shortcuts::testCtrlSSavesInStudyDetailPanel() {
    auto& win = m_fix->window();
    auto& ctx = m_fix->context();

    // Create track and topic
    JobPrep::Domain::Track trk;
    trk.name = u"C++"_s;
    trk.color = u"#5B6CFF"_s;
    ctx.tracks().insert(trk);
    const int trkId = trk.id;

    JobPrep::Domain::Topic top;
    top.trackId = trkId;
    top.title = u"Original Title"_s;
    top.notes = u"Original Notes"_s;
    top.status = JobPrep::Domain::TopicStatus::Backlog;
    ctx.topics().insert(top);
    const int topId = top.id;

    // Switch to Study page and select topic
    emit win.sidebar()->pageSelected(1);
    auto* studyPage = win.findChild<JobPrep::Ui::Pages::StudyPage*>();
    QVERIFY(studyPage != nullptr);
    studyPage->selectTopic(topId);

    auto* detail = win.findChild<JobPrep::Ui::Widgets::TopicDetailPanel*>();
    QVERIFY(detail != nullptr);
    auto* titleEdit = detail->findChild<QLineEdit*>(u"StudyDetailTitleEdit"_s);
    QVERIFY(titleEdit != nullptr);

    titleEdit->setFocus();
    titleEdit->setText(u"Updated Title via Ctrl+S"_s);

    // Send Ctrl+S
    QVERIFY(sendKeySequence(detail, QKeySequence(QKeySequence::Save)));
    QTest::qWait(50);

    // Verify saved to repository
    const auto reloaded = ctx.topics().byId(topId);
    QVERIFY(reloaded.has_value());
    QCOMPARE(reloaded->title, u"Updated Title via Ctrl+S"_s);
}

void tst_Shortcuts::testCtrlSSavesInDialogs() {
    auto& ctx = m_fix->context();

    // 1. ApplicationDialog: Ctrl+S saves
    {
        JobPrep::Ui::Dialogs::ApplicationDialog dlg(ctx.applications(), ctx.interviews(),
                                                    JobPrep::Ui::Dialogs::ApplicationDialog::Create);
        dlg.show();
        QVERIFY(activateWindow(&dlg));

        auto* compEdit = dlg.findChild<QLineEdit*>(u"FieldCompany"_s);
        auto* posEdit = dlg.findChild<QLineEdit*>(u"FieldPosition"_s);
        QVERIFY(compEdit != nullptr && posEdit != nullptr);
        compEdit->setText(u"Acme Corp"_s);
        posEdit->setText(u"Senior Qt Engineer"_s);

        QVERIFY(sendKeySequence(&dlg, QKeySequence(QKeySequence::Save)));
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));

        const auto apps = ctx.applications().all();
        QCOMPARE(apps.size(), 1);
        QCOMPARE(apps.first().company, u"Acme Corp"_s);
    }

    // 2. InterviewDialog: Ctrl+S saves
    {
        const auto apps = ctx.applications().all();
        QVERIFY(!apps.isEmpty());

        JobPrep::Ui::Dialogs::InterviewDialog dlg(ctx.applications(), ctx.interviews(),
                                                  JobPrep::Ui::Dialogs::InterviewDialog::Create);
        dlg.show();
        QVERIFY(activateWindow(&dlg));

        auto* placeEdit = dlg.findChild<QLineEdit*>(u"InterviewPlaceEdit"_s);
        QVERIFY(placeEdit != nullptr);
        placeEdit->setText(u"Google Meet"_s);

        QVERIFY(sendKeySequence(&dlg, QKeySequence(QKeySequence::Save)));
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));

        const auto interviews = ctx.interviews().all();
        QCOMPARE(interviews.size(), 1);
        QCOMPARE(interviews.first().place, u"Google Meet"_s);
    }

    // 3. FollowUpDialog: Ctrl+S saves
    {
        JobPrep::Ui::Dialogs::FollowUpDialog dlg(ctx.applications(), QDate::currentDate());
        dlg.show();
        QVERIFY(activateWindow(&dlg));

        auto* actionEdit = dlg.findChild<QLineEdit*>(u"FollowUpActionEdit"_s);
        QVERIFY(actionEdit != nullptr);
        actionEdit->setText(u"Send thank-you email"_s);

        QVERIFY(sendKeySequence(&dlg, QKeySequence(QKeySequence::Save)));
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));

        const auto apps = ctx.applications().all();
        QCOMPARE(apps.first().nextAction, u"Send thank-you email"_s);
    }

    // 4. SessionDialog: Ctrl+S saves
    {
        JobPrep::Ui::Dialogs::SessionDialog dlg(ctx.sessions(), ctx.topics());
        dlg.show();
        QVERIFY(activateWindow(&dlg));

        auto* notesEdit = dlg.findChild<QTextEdit*>(u"SessionNoteEdit"_s);
        QVERIFY(notesEdit != nullptr);
        notesEdit->setText(u"Studied smart pointers"_s);

        QVERIFY(sendKeySequence(&dlg, QKeySequence(QKeySequence::Save)));
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));

        const auto sessions = ctx.sessions().all();
        QCOMPARE(sessions.size(), 1);
        QCOMPARE(sessions.first().note, u"Studied smart pointers"_s);
    }
}

void tst_Shortcuts::testEscHandlingAcrossWidgets() {
    auto& win = m_fix->window();
    auto& ctx = m_fix->context();

    // 1. Search edit clears text on Esc
    emit win.sidebar()->pageSelected(2);
    auto* jobsSearch = win.findChild<QLineEdit*>(u"JobsSearchEdit"_s);
    QVERIFY(jobsSearch != nullptr);
    jobsSearch->setText(u"sample query"_s);
    jobsSearch->setFocus();
    QVERIFY(jobsSearch->hasFocus());

    QVERIFY(sendKeyClick(jobsSearch, Qt::Key_Escape));
    QVERIFY(jobsSearch->text().isEmpty());
    QVERIFY(jobsSearch->hasFocus());  // Focus preserved

    // 2. Study detail panel closes on Esc
    JobPrep::Domain::Track trk;
    trk.name = u"C++"_s;
    trk.color = u"#5B6CFF"_s;
    ctx.tracks().insert(trk);
    const int trkId = trk.id;

    JobPrep::Domain::Topic top;
    top.trackId = trkId;
    top.title = u"Move Semantics"_s;
    top.status = JobPrep::Domain::TopicStatus::Backlog;
    ctx.topics().insert(top);
    const int topId = top.id;

    emit win.sidebar()->pageSelected(1);
    auto* studyPage = win.findChild<JobPrep::Ui::Pages::StudyPage*>();
    QVERIFY(studyPage != nullptr);
    studyPage->selectTopic(topId);

    auto* detail = win.findChild<JobPrep::Ui::Widgets::TopicDetailPanel*>();
    QVERIFY(detail != nullptr);
    QVERIFY(detail->isVisible());

    auto* titleEdit = detail->findChild<QLineEdit*>(u"StudyDetailTitleEdit"_s);
    QVERIFY(titleEdit != nullptr);
    titleEdit->setFocus();

    QVERIFY(sendKeyClick(titleEdit, Qt::Key_Escape));
    QTest::qWait(50);
    QVERIFY(!detail->isVisible());

    // 3. Dialog rejects on Esc
    JobPrep::Ui::Dialogs::ApplicationDialog dlg(ctx.applications(), ctx.interviews(),
                                                JobPrep::Ui::Dialogs::ApplicationDialog::Create);
    dlg.show();
    QVERIFY(activateWindow(&dlg));
    QVERIFY(sendKeyClick(&dlg, Qt::Key_Escape));
    QCOMPARE(dlg.result(), static_cast<int>(QDialog::Rejected));
}

void tst_Shortcuts::testCollisionModalBlocksWindowShortcuts() {
    auto& win = m_fix->window();
    auto& ctx = m_fix->context();
    auto* stack = win.findChild<QStackedWidget*>(u"PageStack"_s);
    QVERIFY(stack != nullptr);

    emit win.sidebar()->pageSelected(0);
    QCOMPARE(stack->currentIndex(), 0);

    // Open modal dialog
    JobPrep::Ui::Dialogs::ApplicationDialog dlg(ctx.applications(), ctx.interviews(),
                                                JobPrep::Ui::Dialogs::ApplicationDialog::Create, &win);
    dlg.setModal(true);
    dlg.show();
    QVERIFY(activateWindow(&dlg));
    QCOMPARE(QApplication::activeModalWidget(), &dlg);

    // Press Ctrl+, and Ctrl+2 while modal is active
    sendKeySequence(&dlg, QKeySequence(Qt::CTRL | Qt::Key_Comma));
    sendKeySequence(&dlg, QKeySequence(Qt::CTRL | Qt::Key_2));
    QTest::qWait(50);

    // Page must remain 0 (Home)
    QCOMPARE(stack->currentIndex(), 0);

    dlg.reject();
    QCOMPARE(stack->currentIndex(), 0);
}

void tst_Shortcuts::testCollisionCtrlSInsideTextEdit() {
    auto& win = m_fix->window();
    auto& ctx = m_fix->context();

    JobPrep::Domain::Track trk;
    trk.name = u"C++"_s;
    trk.color = u"#5B6CFF"_s;
    ctx.tracks().insert(trk);
    const int trkId = trk.id;

    JobPrep::Domain::Topic top;
    top.trackId = trkId;
    top.title = u"Concurrency"_s;
    top.notes = u"Initial text"_s;
    top.status = JobPrep::Domain::TopicStatus::Backlog;
    ctx.topics().insert(top);
    const int topId = top.id;

    emit win.sidebar()->pageSelected(1);
    auto* studyPage = win.findChild<JobPrep::Ui::Pages::StudyPage*>();
    QVERIFY(studyPage != nullptr);
    studyPage->selectTopic(topId);

    auto* detail = win.findChild<JobPrep::Ui::Widgets::TopicDetailPanel*>();
    QVERIFY(detail != nullptr);
    auto* notesEdit = detail->findChild<QTextEdit*>(u"StudyDetailNotesEdit"_s);
    QVERIFY(notesEdit != nullptr);

    notesEdit->setFocus();
    notesEdit->setPlainText(u"New notes without corruption"_s);
    QVERIFY(notesEdit->hasFocus());

    // Send Ctrl+S while focused in QTextEdit
    QVERIFY(sendKeySequence(notesEdit, QKeySequence(QKeySequence::Save)));
    QTest::qWait(50);

    // Verify notes editor text is not corrupted
    QCOMPARE(notesEdit->toPlainText(), u"New notes without corruption"_s);

    // Verify persisted
    const auto reloaded = ctx.topics().byId(topId);
    QVERIFY(reloaded.has_value());
    QCOMPARE(reloaded->notes, u"New notes without corruption"_s);
}

void tst_Shortcuts::testCollisionCtrlNInsideLineEdit() {
    auto& win = m_fix->window();
    auto* stack = win.findChild<QStackedWidget*>(u"PageStack"_s);
    QVERIFY(stack != nullptr);

    emit win.sidebar()->pageSelected(2);
    QCOMPARE(stack->currentIndex(), 2);

    auto* jobsSearch = win.findChild<QLineEdit*>(u"JobsSearchEdit"_s);
    QVERIFY(jobsSearch != nullptr);
    jobsSearch->setText(u"developer"_s);
    jobsSearch->setFocus();
    QVERIFY(jobsSearch->hasFocus());

    // Send Ctrl+N while typing in search edit
    bool dialogOpened = false;
    answerModalWhenReady([&dialogOpened](QWidget* modal) {
        if (qobject_cast<JobPrep::Ui::Dialogs::ApplicationDialog*>(modal)) {
            dialogOpened = true;
            modal->close();
        }
    });

    QVERIFY(sendKeySequence(jobsSearch, QKeySequence(QKeySequence::New)));
    QTest::qWait(50);

    QVERIFY(dialogOpened);
    // Search edit text must still be "developer", no 'n' character appended
    QCOMPARE(jobsSearch->text(), u"developer"_s);
}

void tst_Shortcuts::testCollisionAltArrowsOnBoard() {
    auto& win = m_fix->window();
    auto& ctx = m_fix->context();

    JobPrep::Domain::Track trk;
    trk.name = u"C++"_s;
    trk.color = u"#5B6CFF"_s;
    ctx.tracks().insert(trk);
    const int trkId = trk.id;

    JobPrep::Domain::Topic top;
    top.trackId = trkId;
    top.title = u"Lambdas"_s;
    top.status = JobPrep::Domain::TopicStatus::Backlog;
    ctx.topics().insert(top);
    const int topId = top.id;

    emit win.sidebar()->pageSelected(1);
    auto* studyPage = win.findChild<JobPrep::Ui::Pages::StudyPage*>();
    QVERIFY(studyPage != nullptr);
    studyPage->selectTopic(topId);

    auto* backlogCol = win.findChild<JobPrep::Ui::Widgets::TopicBoardColumn*>(u"StudyColumnBacklog"_s);
    QVERIFY(backlogCol != nullptr);
    auto* list = backlogCol->findChild<QListView*>(u"StudyColumnList"_s);
    QVERIFY(list != nullptr);
    list->setFocus();

    // Alt+Right moves topic from Backlog (0) to In Progress (1)
    QVERIFY(sendKeyClick(list, Qt::Key_Right, Qt::AltModifier));
    QTest::qWait(50);

    const auto moved = ctx.topics().byId(topId);
    QVERIFY(moved.has_value());
    QCOMPARE(moved->status, JobPrep::Domain::TopicStatus::InProgress);

    // Window must still be on Study page
    auto* stack = win.findChild<QStackedWidget*>(u"PageStack"_s);
    QVERIFY(stack != nullptr);
    QCOMPARE(stack->currentIndex(), 1);
}

QTEST_MAIN(tst_Shortcuts)
#include "tst_Shortcuts.moc"
