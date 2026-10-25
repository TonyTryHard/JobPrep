#include <memory>
#include <QApplication>
#include <QCalendarWidget>
#include <QDateEdit>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QTest>
#include "app/AppContext.h"
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/ReminderLogRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Enums.h"
#include "domain/Structs.h"
#include "models/TopicBoardProxy.h"
#include "models/TopicListModel.h"
#include "services/SettingsService.h"
#include "ui/MainWindow.h"
#if __has_include("ui/InAppNotifier.h")
#include "ui/InAppNotifier.h"
#endif
#if __has_include("ui/ReminderCoordinator.h")
#include "ui/ReminderCoordinator.h"
#endif
#include "ui/Sidebar.h"
#if __has_include("ui/Toast.h")
#include "ui/Toast.h"
#endif
#include "ui/pages/StudyPage.h"

#include "ui/widgets/TopicBoardColumn.h"
#include "ui/widgets/TopicDetailPanel.h"
#include "ui/widgets/TopicListView.h"


using namespace Qt::StringLiterals;
using namespace JobPrep;
using namespace JobPrep::Domain;
using namespace JobPrep::Ui;

class tst_StudyRegression : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testRealMouseClickSelectionAcrossColumns();
    void testWidgetAtReturnsRealControlsNotOverlay();
    void testRealKeyClicksTitleAutosave();
    void testRealMouseClickChecklistAddAndSpaceToggle();
    void testRealMouseClickCalendarDateEdit();
    void testRegressionSequenceWithToastVisible();
};

void tst_StudyRegression::initTestCase() {
}

void tst_StudyRegression::testRealMouseClickSelectionAcrossColumns() {
    App::AppContext ctx(u":memory:"_s);


    Track track;
    track.name = u"C++"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(ctx.tracks().insert(track));
    const int trackId = ctx.tracks().all().last().id;

    Topic topicA;
    topicA.trackId = trackId;
    topicA.title = u"Topic Alpha"_s;
    topicA.status = TopicStatus::Backlog;
    QVERIFY(ctx.topics().insert(topicA));
    const int idA = ctx.topics().all().last().id;

    Topic topicB;
    topicB.trackId = trackId;
    topicB.title = u"Topic Beta"_s;
    topicB.status = TopicStatus::InProgress;
    QVERIFY(ctx.topics().insert(topicB));
    const int idB = ctx.topics().all().last().id;

    Topic topicC;
    topicC.trackId = trackId;
    topicC.title = u"Topic Gamma"_s;
    topicC.status = TopicStatus::Review;
    QVERIFY(ctx.topics().insert(topicC));
    const int idC = ctx.topics().all().last().id;

    MainWindow win(ctx);
    win.resize(1200, 800);
    win.show();
    QCoreApplication::processEvents();

#if __has_include("ui/ReminderCoordinator.h")
    ReminderCoordinator coordinator(
        ctx.interviews(), ctx.applications(), ctx.reminderLog(),
        ctx.settings(), win, false
    );
    coordinator.start();
    QCoreApplication::processEvents();
#endif

    win.sidebar()->setCurrentPage(1);
    emit win.sidebar()->pageSelected(1);
    QCoreApplication::processEvents();

    auto* studyPage = win.findChild<Pages::StudyPage*>();
    QVERIFY(studyPage != nullptr);

    auto* colBacklog = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnBacklog"_s);
    auto* colInProgress = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnInProgress"_s);
    auto* colReview = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnReview"_s);
    auto* colDone = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnDone"_s);
    QVERIFY(colBacklog && colInProgress && colReview && colDone);

    auto* detailPanel = win.findChild<Widgets::TopicDetailPanel*>(u"StudyDetailPanel"_s);
    QVERIFY(detailPanel != nullptr);
    auto* titleEdit = detailPanel->findChild<QLineEdit*>(u"StudyDetailTitleEdit"_s);
    QVERIFY(titleEdit != nullptr);

    const auto checkSingleSelection = [&](Widgets::TopicBoardColumn* activeCol, int expectedTopicId, const QString& expectedTitle) {
        QCOMPARE(studyPage->selectedTopicId(), std::optional<int>(expectedTopicId));

        for (auto* col : {colBacklog, colInProgress, colReview, colDone}) {
            if (col == activeCol) {
                QCOMPARE(col->list()->selectionModel()->selectedRows().size(), 1);
                QVERIFY(col->list()->currentIndex().isValid());
                QCOMPARE(col->list()->currentIndex().data(Models::TopicListModel::IdRole).toInt(), expectedTopicId);
            } else {
                QVERIFY2(!col->list()->selectionModel()->hasSelection(),
                         qPrintable(QString("Expected empty selection in column %1").arg(col->objectName())));
                QVERIFY2(!col->list()->currentIndex().isValid(),
                         qPrintable(QString("Expected invalid current index in column %1").arg(col->objectName())));
            }
        }

        QVERIFY(titleEdit->isEnabled());
        QCOMPARE(titleEdit->text(), expectedTitle);
    };

    // 1. Mouse click on Card A in Backlog
    {
        const QModelIndex idxA = colBacklog->proxy()->index(0, 0);
        QVERIFY(idxA.isValid());
        const QRect rectA = colBacklog->list()->visualRect(idxA);
        QVERIFY(!rectA.isEmpty());
        QTest::mouseClick(colBacklog->list()->viewport(), Qt::LeftButton, Qt::NoModifier, rectA.center());
        QCoreApplication::processEvents();

        checkSingleSelection(colBacklog, idA, u"Topic Alpha"_s);
    }

    // 2. Mouse click on Card B in In Progress
    {
        const QModelIndex idxB = colInProgress->proxy()->index(0, 0);
        QVERIFY(idxB.isValid());
        const QRect rectB = colInProgress->list()->visualRect(idxB);
        QVERIFY(!rectB.isEmpty());
        QTest::mouseClick(colInProgress->list()->viewport(), Qt::LeftButton, Qt::NoModifier, rectB.center());
        QCoreApplication::processEvents();

        checkSingleSelection(colInProgress, idB, u"Topic Beta"_s);
    }

    // 3. Mouse click on Card C in Review
    {
        const QModelIndex idxC = colReview->proxy()->index(0, 0);
        QVERIFY(idxC.isValid());
        const QRect rectC = colReview->list()->visualRect(idxC);
        QVERIFY(!rectC.isEmpty());
        QTest::mouseClick(colReview->list()->viewport(), Qt::LeftButton, Qt::NoModifier, rectC.center());
        QCoreApplication::processEvents();

        checkSingleSelection(colReview, idC, u"Topic Gamma"_s);
    }
}

void tst_StudyRegression::testWidgetAtReturnsRealControlsNotOverlay() {
    App::AppContext ctx(u":memory:"_s);
    Track track;
    track.name = u"C++"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(ctx.tracks().insert(track));
    const int trackId = ctx.tracks().all().last().id;

    Topic topicA;
    topicA.trackId = trackId;
    topicA.title = u"Topic Alpha"_s;
    topicA.status = TopicStatus::Backlog;
    QVERIFY(ctx.topics().insert(topicA));
    const int idA = ctx.topics().all().last().id;

    MainWindow win(ctx);
    win.resize(1200, 800);
    win.show();
    QCoreApplication::processEvents();

#if __has_include("ui/ReminderCoordinator.h")
    ReminderCoordinator coordinator(
        ctx.interviews(), ctx.applications(), ctx.reminderLog(),
        ctx.settings(), win, false
    );
    coordinator.start();
    QCoreApplication::processEvents();
#endif

    win.sidebar()->setCurrentPage(1);
    emit win.sidebar()->pageSelected(1);
    QCoreApplication::processEvents();

    auto* colBacklog = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnBacklog"_s);
    QVERIFY(colBacklog != nullptr);
    const QModelIndex idxA = colBacklog->proxy()->index(0, 0);
    const QRect cardRect = colBacklog->list()->visualRect(idxA);
    const QPoint cardCenterGlobal = colBacklog->list()->viewport()->mapToGlobal(cardRect.center());

    QWidget* widgetAtCard = QApplication::widgetAt(cardCenterGlobal);
    if (!widgetAtCard) {
        widgetAtCard = win.childAt(win.mapFromGlobal(cardCenterGlobal));
    }
    QCOMPARE(widgetAtCard, static_cast<QWidget*>(colBacklog->list()->viewport()));

    // Select topic via mouse click so detail panel opens
    QTest::mouseClick(colBacklog->list()->viewport(), Qt::LeftButton, Qt::NoModifier, cardRect.center());
    QCoreApplication::processEvents();

    auto* studyPage = win.findChild<Pages::StudyPage*>();
    QVERIFY(studyPage != nullptr);
    QCOMPARE(studyPage->selectedTopicId(), std::optional<int>(idA));

    auto* detailPanel = win.findChild<Widgets::TopicDetailPanel*>(u"StudyDetailPanel"_s);
    QVERIFY(detailPanel != nullptr);
    QVERIFY(detailPanel->isVisible());

    auto* titleEdit = win.findChild<QLineEdit*>(u"StudyDetailTitleEdit"_s);
    QVERIFY(titleEdit != nullptr);
    const QPoint titleCenterGlobal = titleEdit->mapToGlobal(titleEdit->rect().center());
    QWidget* widgetAtTitle = QApplication::widgetAt(titleCenterGlobal);
    if (!widgetAtTitle) {
        widgetAtTitle = win.childAt(win.mapFromGlobal(titleCenterGlobal));
    }
    QVERIFY(widgetAtTitle == titleEdit || titleEdit->isAncestorOf(widgetAtTitle));

    auto* addBtn = win.findChild<QPushButton*>(u"StudyDetailCheckAddBtn"_s);
    QVERIFY(addBtn != nullptr);
    const QPoint addBtnCenterGlobal = addBtn->mapToGlobal(addBtn->rect().center());
    QWidget* widgetAtAdd = QApplication::widgetAt(addBtnCenterGlobal);
    if (!widgetAtAdd) {
        widgetAtAdd = win.childAt(win.mapFromGlobal(addBtnCenterGlobal));
    }
    QCOMPARE(widgetAtAdd, static_cast<QWidget*>(addBtn));
}

void tst_StudyRegression::testRealKeyClicksTitleAutosave() {
    App::AppContext ctx(u":memory:"_s);
    Track track;
    track.name = u"C++"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(ctx.tracks().insert(track));
    const int trackId = ctx.tracks().all().last().id;

    Topic topicA;
    topicA.trackId = trackId;
    topicA.title = u"Alpha"_s;
    topicA.status = TopicStatus::Backlog;
    QVERIFY(ctx.topics().insert(topicA));
    const int idA = ctx.topics().all().last().id;

    Topic topicB;
    topicB.trackId = trackId;
    topicB.title = u"Beta"_s;
    topicB.status = TopicStatus::InProgress;
    QVERIFY(ctx.topics().insert(topicB));
    const int idB = ctx.topics().all().last().id;

    MainWindow win(ctx);
    win.resize(1200, 800);
    win.show();
    QCoreApplication::processEvents();

#if __has_include("ui/ReminderCoordinator.h")
    ReminderCoordinator coordinator(
        ctx.interviews(), ctx.applications(), ctx.reminderLog(),
        ctx.settings(), win, false
    );
    coordinator.start();
    QCoreApplication::processEvents();
#endif

    win.sidebar()->setCurrentPage(1);
    emit win.sidebar()->pageSelected(1);
    QCoreApplication::processEvents();

    auto* colBacklog = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnBacklog"_s);
    auto* colInProgress = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnInProgress"_s);
    QVERIFY(colBacklog && colInProgress);

    // Mouse click on topic A
    const QRect rectA = colBacklog->list()->visualRect(colBacklog->proxy()->index(0, 0));
    QTest::mouseClick(colBacklog->list()->viewport(), Qt::LeftButton, Qt::NoModifier, rectA.center());
    QCoreApplication::processEvents();

    auto* titleEdit = win.findChild<QLineEdit*>(u"StudyDetailTitleEdit"_s);
    QVERIFY(titleEdit != nullptr);
    titleEdit->setFocus();
    titleEdit->clear();
    QTest::keyClicks(titleEdit, u"Alpha Edited"_s);
    QCoreApplication::processEvents();

    // Change selection to topic B with mouse click
    const QRect rectB = colInProgress->list()->visualRect(colInProgress->proxy()->index(0, 0));
    QTest::mouseClick(colInProgress->list()->viewport(), Qt::LeftButton, Qt::NoModifier, rectB.center());
    QCoreApplication::processEvents();

    auto* studyPage = win.findChild<Pages::StudyPage*>();
    QVERIFY(studyPage != nullptr);
    QCOMPARE(studyPage->selectedTopicId(), std::optional<int>(idB));

    // Verify topic A title was autosaved to database
    const auto savedA = ctx.topics().byId(idA);
    QVERIFY(savedA.has_value());
    QCOMPARE(savedA->title, u"Alpha Edited"_s);
}

void tst_StudyRegression::testRealMouseClickChecklistAddAndSpaceToggle() {
    App::AppContext ctx(u":memory:"_s);
    Track track;
    track.name = u"C++"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(ctx.tracks().insert(track));
    const int trackId = ctx.tracks().all().last().id;

    Topic topicA;
    topicA.trackId = trackId;
    topicA.title = u"Alpha"_s;
    topicA.status = TopicStatus::Backlog;
    QVERIFY(ctx.topics().insert(topicA));
    const int idA = ctx.topics().all().last().id;

    MainWindow win(ctx);
    win.resize(1200, 800);
    win.show();
    QCoreApplication::processEvents();

#if __has_include("ui/ReminderCoordinator.h")
    ReminderCoordinator coordinator(
        ctx.interviews(), ctx.applications(), ctx.reminderLog(),
        ctx.settings(), win, false
    );
    coordinator.start();
    QCoreApplication::processEvents();
#endif

    win.sidebar()->setCurrentPage(1);
    emit win.sidebar()->pageSelected(1);
    QCoreApplication::processEvents();

    auto* colBacklog = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnBacklog"_s);
    const QRect rectA = colBacklog->list()->visualRect(colBacklog->proxy()->index(0, 0));
    QTest::mouseClick(colBacklog->list()->viewport(), Qt::LeftButton, Qt::NoModifier, rectA.center());
    QCoreApplication::processEvents();

    auto* checkAddEdit = win.findChild<QLineEdit*>(u"StudyDetailCheckAddEdit"_s);
    auto* checkAddBtn = win.findChild<QPushButton*>(u"StudyDetailCheckAddBtn"_s);
    auto* checkList = win.findChild<QListWidget*>(u"StudyDetailCheckList"_s);
    QVERIFY(checkAddEdit && checkAddBtn && checkList);

    // Type text into checklist edit
    checkAddEdit->setFocus();
    QTest::keyClicks(checkAddEdit, u"Item 1"_s);
    QCoreApplication::processEvents();

    // Click Add button
    QTest::mouseClick(checkAddBtn, Qt::LeftButton);
    QCoreApplication::processEvents();

    // Verify item appears and is persisted
    QCOMPARE(checkList->count(), 1);
    QCOMPARE(checkList->item(0)->text(), u"Item 1"_s);
    QCOMPARE(checkList->item(0)->checkState(), Qt::Unchecked);

    auto subtasks = ctx.topics().subtasks(idA);
    QCOMPARE(subtasks.size(), 1);
    QCOMPARE(subtasks.first().text, u"Item 1"_s);
    QCOMPARE(subtasks.first().done, false);

    // Press Space on checklist item to toggle
    checkList->setCurrentRow(0);
    checkList->setFocus();
    QTest::keyClick(checkList, Qt::Key_Space);
    QCoreApplication::processEvents();

    QCOMPARE(checkList->item(0)->checkState(), Qt::Checked);
    subtasks = ctx.topics().subtasks(idA);
    QCOMPARE(subtasks.size(), 1);
    QCOMPARE(subtasks.first().done, true);
}

void tst_StudyRegression::testRealMouseClickCalendarDateEdit() {
    App::AppContext ctx(u":memory:"_s);
    Track track;
    track.name = u"C++"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(ctx.tracks().insert(track));
    const int trackId = ctx.tracks().all().last().id;

    Topic topicA;
    topicA.trackId = trackId;
    topicA.title = u"Alpha"_s;
    topicA.status = TopicStatus::Backlog;
    QVERIFY(ctx.topics().insert(topicA));
    const int idA = ctx.topics().all().last().id;

    MainWindow win(ctx);
    win.resize(1200, 800);
    win.show();
    QCoreApplication::processEvents();

    win.sidebar()->setCurrentPage(1);
    emit win.sidebar()->pageSelected(1);
    QCoreApplication::processEvents();

    auto* colBacklog = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnBacklog"_s);
    const QRect rectA = colBacklog->list()->visualRect(colBacklog->proxy()->index(0, 0));
    QTest::mouseClick(colBacklog->list()->viewport(), Qt::LeftButton, Qt::NoModifier, rectA.center());
    QCoreApplication::processEvents();

    auto* dateEdit = win.findChild<QDateEdit*>(u"StudyDetailDateEdit"_s);
    auto* dateBtn = win.findChild<QPushButton*>(u"StudyDetailDateClearBtn"_s);
    QVERIFY(dateEdit && dateBtn);

    // Initially no target date -> button says "Set date"
    QCOMPARE(dateBtn->text(), tr("Set date"));
    // Target date field should not show 2000-01-01 when empty
    QVERIFY(dateEdit->text() == tr("No target date") || !dateEdit->isEnabled());

    // Click "Set date"
    QTest::mouseClick(dateBtn, Qt::LeftButton);
    QCoreApplication::processEvents();
    QVERIFY(dateEdit->isEnabled());
    QCOMPARE(dateBtn->text(), tr("Clear"));

    // Set a new date
    const QDate testDate(2026, 11, 15);
    dateEdit->setDate(testDate);
    auto* detailPanel = win.findChild<Widgets::TopicDetailPanel*>(u"StudyDetailPanel"_s);
    QVERIFY(detailPanel != nullptr);
    detailPanel->flush();
    QCoreApplication::processEvents();

    auto saved = ctx.topics().byId(idA);
    QVERIFY(saved.has_value());
    QCOMPARE(saved->targetDate, std::optional<QDate>(testDate));

    // Click "Clear" button
    QTest::mouseClick(dateBtn, Qt::LeftButton);
    QCoreApplication::processEvents();

    saved = ctx.topics().byId(idA);
    QVERIFY(saved.has_value());
    QCOMPARE(saved->targetDate, std::nullopt);
    QCOMPARE(dateBtn->text(), tr("Set date"));
}

void tst_StudyRegression::testRegressionSequenceWithToastVisible() {
    App::AppContext ctx(u":memory:"_s);
    Track track;
    track.name = u"C++"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(ctx.tracks().insert(track));
    const int trackId = ctx.tracks().all().last().id;

    Topic topicA;
    topicA.trackId = trackId;
    topicA.title = u"Alpha"_s;
    topicA.status = TopicStatus::Backlog;
    QVERIFY(ctx.topics().insert(topicA));
    const int idA = ctx.topics().all().last().id;

    Topic topicB;
    topicB.trackId = trackId;
    topicB.title = u"Beta"_s;
    topicB.status = TopicStatus::InProgress;
    QVERIFY(ctx.topics().insert(topicB));
    const int idB = ctx.topics().all().last().id;

    MainWindow win(ctx);
    win.resize(1200, 800);
    win.show();
    QCoreApplication::processEvents();

#if __has_include("ui/ReminderCoordinator.h")
    ReminderCoordinator coordinator(
        ctx.interviews(), ctx.applications(), ctx.reminderLog(),
        ctx.settings(), win, false // in-app toast notifier
    );
    coordinator.start();
    QCoreApplication::processEvents();
#endif

#if __has_include("ui/InAppNotifier.h")
    // Trigger a toast
    Services::Notification notif;
    notif.kind = Services::NotificationKind::FollowUp;
    notif.company = u"Acme Corp"_s;
    notif.position = u"C++ Engineer"_s;
    InAppNotifier notifier(win, win.themeManager(), nullptr);
    QVERIFY(notifier.notify(notif));
    QCoreApplication::processEvents();
    QCOMPARE(notifier.visibleToastCount(), 1);
#endif

    win.sidebar()->setCurrentPage(1);
    emit win.sidebar()->pageSelected(1);
    QCoreApplication::processEvents();

    auto* colBacklog = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnBacklog"_s);
    auto* colInProgress = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnInProgress"_s);
    auto* detailPanel = win.findChild<Widgets::TopicDetailPanel*>(u"StudyDetailPanel"_s);
    auto* titleEdit = detailPanel->findChild<QLineEdit*>(u"StudyDetailTitleEdit"_s);
    QVERIFY(colBacklog && colInProgress && detailPanel && titleEdit);

    auto* studyPage = win.findChild<Pages::StudyPage*>();
    QVERIFY(studyPage != nullptr);

    // Mouse click on topic A
    const QRect rectA = colBacklog->list()->visualRect(colBacklog->proxy()->index(0, 0));
    QTest::mouseClick(colBacklog->list()->viewport(), Qt::LeftButton, Qt::NoModifier, rectA.center());
    QCoreApplication::processEvents();

    QCOMPARE(studyPage->selectedTopicId(), std::optional<int>(idA));
    QCOMPARE(colBacklog->list()->selectionModel()->selectedRows().size(), 1);
    QVERIFY(!colInProgress->list()->currentIndex().isValid());
    QCOMPARE(titleEdit->text(), u"Alpha"_s);

    // Mouse click on topic B
    const QRect rectB = colInProgress->list()->visualRect(colInProgress->proxy()->index(0, 0));
    QTest::mouseClick(colInProgress->list()->viewport(), Qt::LeftButton, Qt::NoModifier, rectB.center());
    QCoreApplication::processEvents();

    QCOMPARE(studyPage->selectedTopicId(), std::optional<int>(idB));
    QCOMPARE(colInProgress->list()->selectionModel()->selectedRows().size(), 1);
    QVERIFY(!colBacklog->list()->currentIndex().isValid());
    QCOMPARE(titleEdit->text(), u"Beta"_s);

}

QTEST_MAIN(tst_StudyRegression)
#include "tst_StudyRegression.moc"
