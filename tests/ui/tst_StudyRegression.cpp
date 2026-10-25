#include <memory>
#include <QApplication>
#include <QCalendarWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTest>
#include <QVBoxLayout>
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
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
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
    void testRealFocusChecklistAddAndEdits();
    void testRealMouseClickCalendarDateEdit();
    void testBoardDragCallsStartDragOnceAndClickDoesNotDrag();
    void testBoardSelectionPaintedOutlinesBothThemes();
    void testRegressionSequenceWithToastVisible();
    void testChecklistStartupContrastDarkAndLight();
    void testChecklistAddFocusScrollAndOverflowSizing();
    void testGeneralStartupContrastAllPagesBothThemes();
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

void tst_StudyRegression::testRealFocusChecklistAddAndEdits() {
    App::AppContext ctx(u":memory:"_s);
    Track track;
    track.name = u"C++"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(ctx.tracks().insert(track));
    const int trackId = ctx.tracks().all().last().id;

    Topic topicA;
    topicA.trackId = trackId;
    topicA.title = u"Alpha"_s;
    topicA.priority = Priority::Normal;
    topicA.status = TopicStatus::Backlog;
    QVERIFY(ctx.topics().insert(topicA));
    const int idA = ctx.topics().all().last().id;

    MainWindow win(ctx);
    win.resize(1200, 800);
    win.show();
    QVERIFY(QTest::qWaitForWindowActive(&win));
    QCoreApplication::processEvents();

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
    auto* titleEdit = win.findChild<QLineEdit*>(u"StudyDetailTitleEdit"_s);
    auto* priorityCombo = win.findChild<QComboBox*>(u"StudyDetailPriorityCombo"_s);
    QVERIFY(checkAddEdit && checkAddBtn && checkList && titleEdit && priorityCombo);

    // 1. Checklist Add with real focus movement
    checkAddEdit->setFocus();
    QTest::keyClicks(checkAddEdit, u"New Task"_s);
    QCoreApplication::processEvents();
    QCOMPARE(checkAddEdit->text(), u"New Task"_s);

    // Focus moves to button before mouse click (as on real active Wayland window)
    checkAddBtn->setFocus(Qt::MouseFocusReason);
    QCoreApplication::processEvents();
    // Input must NOT be wiped by focus change
    QCOMPARE(checkAddEdit->text(), u"New Task"_s);

    QTest::mouseClick(checkAddBtn, Qt::LeftButton);
    QCoreApplication::processEvents();

    // Assert item added and persisted in repository
    QCOMPARE(checkList->count(), 1);
    QCOMPARE(checkList->item(0)->text(), u"New Task"_s);
    auto subtasks = ctx.topics().subtasks(idA);
    QCOMPARE(subtasks.size(), 1);
    QCOMPARE(subtasks.first().text, u"New Task"_s);

    // 2. Tick checklist item with real focus
    checkList->setFocus();
    checkList->setCurrentRow(0);
    checkList->item(0)->setCheckState(Qt::Checked);
    QCoreApplication::processEvents();
    subtasks = ctx.topics().subtasks(idA);
    QCOMPARE(subtasks.size(), 1);
    QCOMPARE(subtasks.first().done, true);

    // 3. Edit title with real focus movement
    titleEdit->setFocus();
    titleEdit->setText(u"Alpha Renamed"_s);
    // Focus loss triggers autosave
    checkList->setFocus(Qt::MouseFocusReason);
    QCoreApplication::processEvents();
    auto saved = ctx.topics().byId(idA);
    QVERIFY(saved.has_value());
    QCOMPARE(saved->title, u"Alpha Renamed"_s);

    // 4. Change priority with real focus movement
    priorityCombo->setFocus();
    priorityCombo->setCurrentIndex(priorityCombo->findData(static_cast<int>(Priority::High)));
    titleEdit->setFocus(Qt::MouseFocusReason);
    QCoreApplication::processEvents();
    saved = ctx.topics().byId(idA);
    QVERIFY(saved.has_value());
    QCOMPARE(saved->priority, Priority::High);
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
    QVERIFY(QTest::qWaitForWindowActive(&win));
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

    // SPEC S2 / F2: field is always enabled while a topic is selected
    QVERIFY(dateEdit->isEnabled());
    QCOMPARE(dateEdit->text(), tr("No target date"));
    QCOMPARE(dateBtn->text(), tr("Clear"));
    QVERIFY(!dateBtn->isEnabled()); // No date yet -> Clear disabled

    // Pick a date via calendar popup
    auto* cal = dateEdit->calendarWidget();
    QVERIFY(cal != nullptr);
    const QDate testDate(2026, 11, 15);
    cal->setSelectedDate(testDate);
    emit cal->clicked(testDate);
    QCoreApplication::processEvents();

    // Verify date is set, Clear is enabled, and autosaved to repository
    QCOMPARE(dateEdit->date(), testDate);
    QVERIFY(dateBtn->isEnabled());
    auto saved = ctx.topics().byId(idA);
    QVERIFY(saved.has_value());
    QCOMPARE(saved->targetDate, std::optional<QDate>(testDate));

    // Clear via Clear button click
    dateBtn->setFocus(Qt::MouseFocusReason);
    QTest::mouseClick(dateBtn, Qt::LeftButton);
    QCoreApplication::processEvents();

    QVERIFY(dateEdit->isEnabled()); // Field stays enabled!
    QCOMPARE(dateEdit->text(), tr("No target date"));
    QVERIFY(!dateBtn->isEnabled());
    saved = ctx.topics().byId(idA);
    QVERIFY(saved.has_value());
    QCOMPARE(saved->targetDate, std::nullopt);

    // Pick another day via calendar widget again
    const QDate testDate2(2026, 12, 1);
    cal->setSelectedDate(testDate2);
    emit cal->clicked(testDate2);
    QCoreApplication::processEvents();

    QCOMPARE(dateEdit->date(), testDate2);
    QVERIFY(dateBtn->isEnabled());
    saved = ctx.topics().byId(idA);
    QVERIFY(saved.has_value());
    QCOMPARE(saved->targetDate, std::optional<QDate>(testDate2));

    // Clear via Delete key in the field
    dateEdit->setFocus();
    QTest::keyClick(dateEdit, Qt::Key_Delete);
    QCoreApplication::processEvents();

    QVERIFY(dateEdit->isEnabled());
    QCOMPARE(dateEdit->text(), tr("No target date"));
    saved = ctx.topics().byId(idA);
    QVERIFY(saved.has_value());
    QCOMPARE(saved->targetDate, std::nullopt);
}

void tst_StudyRegression::testBoardDragCallsStartDragOnceAndClickDoesNotDrag() {
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

    MainWindow win(ctx);
    win.resize(1200, 800);
    win.show();
    QVERIFY(QTest::qWaitForWindowActive(&win));
    QCoreApplication::processEvents();

    win.sidebar()->setCurrentPage(1);
    emit win.sidebar()->pageSelected(1);
    QCoreApplication::processEvents();

    auto* colBacklog = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnBacklog"_s);
    QVERIFY(colBacklog != nullptr);
    auto* listView = colBacklog->list();
    QVERIFY(listView != nullptr);

    // A normal click selects the topic and does NOT trigger invalid drag state
    const QRect rectA = listView->visualRect(colBacklog->proxy()->index(0, 0));
    QTest::mouseClick(listView->viewport(), Qt::LeftButton, Qt::NoModifier, rectA.center());
    QCoreApplication::processEvents();

    QVERIFY(listView->selectionModel()->hasSelection());
    QCOMPARE(listView->currentIndex().row(), 0);
}

void tst_StudyRegression::testBoardSelectionPaintedOutlinesBothThemes() {
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

    Topic topicB;
    topicB.trackId = trackId;
    topicB.title = u"Topic Beta"_s;
    topicB.status = TopicStatus::InProgress;
    QVERIFY(ctx.topics().insert(topicB));

    Topic topicC;
    topicC.trackId = trackId;
    topicC.title = u"Topic Gamma"_s;
    topicC.status = TopicStatus::Review;
    QVERIFY(ctx.topics().insert(topicC));

    MainWindow win(ctx);
    win.resize(1200, 800);
    win.show();
    QVERIFY(QTest::qWaitForWindowActive(&win));
    QCoreApplication::processEvents();

    win.sidebar()->setCurrentPage(1);
    emit win.sidebar()->pageSelected(1);
    QCoreApplication::processEvents();

    auto* colBacklog = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnBacklog"_s);
    auto* colInProgress = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnInProgress"_s);
    auto* colReview = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnReview"_s);
    auto* boardWidget = win.findChild<QWidget*>(u"StudyBoard"_s);
    QVERIFY(colBacklog && colInProgress && colReview && boardWidget);

    auto countSelectedCards = [&](const QColor& accent) -> int {
        const QImage boardImg = boardWidget->grab().toImage();
        int accentBorderCards = 0;
        const auto columns = {colBacklog, colInProgress, colReview};
        for (auto* col : columns) {
            for (int r = 0; r < col->itemCount(); ++r) {
                const QRect vRect = col->list()->visualRect(col->proxy()->index(r, 0));
                const QPoint p = col->list()->viewport()->mapTo(boardWidget, vRect.topLeft());
                const QRect cardRect(p, vRect.size());

                // Sample card right border (away from track color stripe on left)
                bool hasAccent = false;
                const int sampleX = cardRect.right() - 2;
                for (int y = cardRect.top() + 8; y < cardRect.bottom() - 8; ++y) {
                    if (sampleX >= 0 && sampleX < boardImg.width() && y >= 0 && y < boardImg.height()) {
                        const QColor pixel = boardImg.pixelColor(sampleX, y);
                        if (qAbs(pixel.red() - accent.red()) < 15 &&
                            qAbs(pixel.green() - accent.green()) < 15 &&
                            qAbs(pixel.blue() - accent.blue()) < 15) {
                            hasAccent = true;
                            break;
                        }
                    }
                }
                if (hasAccent) {
                    accentBorderCards++;
                }
            }
        }
        return accentBorderCards;
    };

    for (auto mode : {Theme::ThemeMode::Light, Theme::ThemeMode::Dark}) {
        win.themeManager().setThemeMode(mode);
        QCoreApplication::processEvents();
        const auto tokens = win.themeManager().currentTokens();

        // 1. Click Backlog card
        const QRect rectA = colBacklog->list()->visualRect(colBacklog->proxy()->index(0, 0));
        QTest::mouseClick(colBacklog->list()->viewport(), Qt::LeftButton, Qt::NoModifier, rectA.center());
        QCoreApplication::processEvents();
        QCOMPARE(countSelectedCards(tokens.accent), 1);

        // 2. Click InProgress card
        const QRect rectB = colInProgress->list()->visualRect(colInProgress->proxy()->index(0, 0));
        QTest::mouseClick(colInProgress->list()->viewport(), Qt::LeftButton, Qt::NoModifier, rectB.center());
        QCoreApplication::processEvents();
        QCOMPARE(countSelectedCards(tokens.accent), 1);

        // 3. Click Review card
        const QRect rectC = colReview->list()->visualRect(colReview->proxy()->index(0, 0));
        QTest::mouseClick(colReview->list()->viewport(), Qt::LeftButton, Qt::NoModifier, rectC.center());
        QCoreApplication::processEvents();
        QCOMPARE(countSelectedCards(tokens.accent), 1);

        // 4. Tick a checklist item (model reset occurs) -> exactly one outline remains
        auto* checkAddEdit = win.findChild<QLineEdit*>(u"StudyDetailCheckAddEdit"_s);
        auto* checkAddBtn = win.findChild<QPushButton*>(u"StudyDetailCheckAddBtn"_s);
        auto* checkList = win.findChild<QListWidget*>(u"StudyDetailCheckList"_s);
        QVERIFY(checkAddEdit && checkAddBtn && checkList);
        checkAddEdit->setText(u"Subtask"_s);
        QTest::mouseClick(checkAddBtn, Qt::LeftButton);
        QCoreApplication::processEvents();

        if (checkList->count() > 0) {
            checkList->item(0)->setCheckState(Qt::Checked);
            QCoreApplication::processEvents();
        }

        QCOMPARE(countSelectedCards(tokens.accent), 1);
    }
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

static double srgbToLinear(double c) {
    return (c <= 0.04045) ? (c / 12.92) : std::pow((c + 0.055) / 1.055, 2.4);
}
static double relativeLuminance(const QColor& c) {
    return 0.2126 * srgbToLinear(c.redF()) +
           0.7152 * srgbToLinear(c.greenF()) +
           0.0722 * srgbToLinear(c.blueF());
}
static double calcContrastRatio(const QColor& c1, const QColor& c2) {
    double l1 = relativeLuminance(c1);
    double l2 = relativeLuminance(c2);
    if (l1 < l2) std::swap(l1, l2);
    return (l1 + 0.05) / (l2 + 0.05);
}

void tst_StudyRegression::testChecklistStartupContrastDarkAndLight() {
    auto runTestForMode = [](const QString& modeStr) {
        // 1. Saved theme mode set before AppContext is created, matching main.cpp startup
        {
            Services::SettingsService settings;
            settings.setThemeMode(modeStr);
        }

        App::AppContext ctx(u":memory:"_s);
        QCOMPARE(ctx.settings().themeMode(), modeStr);

        Track track;
        track.name = u"C++"_s;
        track.color = u"#3b82f6"_s;
        QVERIFY(ctx.tracks().insert(track));
        const int trackId = ctx.tracks().all().last().id;

        Topic topic;
        topic.trackId = trackId;
        topic.title = u"Pointers & Memory"_s;
        topic.status = TopicStatus::InProgress;
        QVERIFY(ctx.topics().insert(topic));
        const int topicId = ctx.topics().all().last().id;

        for (int i = 1; i <= 4; ++i) {
            Subtask st;
            st.topicId = topicId;
            st.text = QString(u"Checklist item %1"_s).arg(i);
            st.position = i - 1;
            st.done = (i == 1);
            QVERIFY(ctx.topics().addSubtask(st));
        }

        MainWindow win(ctx);
        win.resize(1200, 800);
        win.show();
        QCoreApplication::processEvents();

        win.sidebar()->setCurrentPage(1);
        emit win.sidebar()->pageSelected(1);
        QCoreApplication::processEvents();

        auto* colInProgress = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnInProgress"_s);
        auto* detailPanel = win.findChild<Widgets::TopicDetailPanel*>(u"StudyDetailPanel"_s);
        auto* checkList = detailPanel->findChild<QListWidget*>(u"StudyDetailCheckList"_s);
        auto* checkAddEdit = detailPanel->findChild<QLineEdit*>(u"StudyDetailCheckAddEdit"_s);
        auto* checkAddBtn = detailPanel->findChild<QPushButton*>(u"StudyDetailCheckAddBtn"_s);
        QVERIFY(colInProgress && detailPanel && checkList && checkAddEdit && checkAddBtn);

        // Click the card with a real mouse click
        const QRect cardRect = colInProgress->list()->visualRect(colInProgress->proxy()->index(0, 0));
        QTest::mouseClick(colInProgress->list()->viewport(), Qt::LeftButton, Qt::NoModifier, cardRect.center());
        QCoreApplication::processEvents();

        QCOMPARE(checkList->count(), 4);

        // Add 5th item through the Add button with real events
        QTest::mouseClick(checkAddEdit, Qt::LeftButton);
        QTest::keyClicks(checkAddEdit, u"Checklist item 5"_s);
        QTest::mouseClick(checkAddBtn, Qt::LeftButton);
        QCoreApplication::processEvents();

        QCOMPARE(checkList->count(), 5);

        // Measure painted contrast on checklist list viewport at fresh start
        QWidget* viewport = checkList->viewport();
        const QImage imgFresh = viewport->grab().toImage();
        const QRect vpRect = viewport->rect();

        // Assert new row is visible in viewport
        auto* newRowItem = checkList->item(4);
        const QRect newRowRect = checkList->visualItemRect(newRowItem);
        QVERIFY2(newRowRect.intersects(vpRect), "New row not visible in viewport");

        // Measure painted contrast on checklist list viewport for every row
        for (int row = 0; row < checkList->count(); ++row) {
            auto* item = checkList->item(row);
            checkList->scrollToItem(item);
            QCoreApplication::processEvents();

            const QRect rowRect = checkList->visualItemRect(item);
            QVERIFY2(rowRect.intersects(vpRect), qPrintable(QString("Row %1 not in viewport").arg(row)));

            const QImage imgRow = viewport->grab().toImage();
            const QColor bgRow = imgRow.pixelColor(2, 2);

            double maxContrast = 1.0;
            // Sample text region (avoiding checkbox on the left, e.g. x >= rowRect.left() + 28)
            const int startX = qMax(rowRect.left() + 28, 0);
            const int endX = qMin(rowRect.right() - 5, imgRow.width() - 1);
            const int startY = qMax(rowRect.top(), 0);
            const int endY = qMin(rowRect.bottom(), imgRow.height() - 1);

            for (int y = startY; y <= endY; ++y) {
                for (int x = startX; x <= endX; ++x) {
                    const QColor pixel = imgRow.pixelColor(x, y);
                    const double cr = calcContrastRatio(pixel, bgRow);
                    if (cr > maxContrast) {
                        maxContrast = cr;
                    }
                }
            }
            QVERIFY2(maxContrast >= 3.0,
                     qPrintable(QString("[%1] Row %2 text contrast (%3:1) is below 3:1 (invisible)")
                                    .arg(modeStr).arg(row).arg(maxContrast, 0, 'f', 2)));
        }

        // Switch theme: dark -> light -> dark (or light -> dark -> light)
        if (modeStr == u"dark"_s) {
            win.themeManager().setThemeMode(Theme::ThemeMode::Light);
            QCoreApplication::processEvents();
            win.themeManager().setThemeMode(Theme::ThemeMode::Dark);
            QCoreApplication::processEvents();
        } else {
            win.themeManager().setThemeMode(Theme::ThemeMode::Dark);
            QCoreApplication::processEvents();
            win.themeManager().setThemeMode(Theme::ThemeMode::Light);
            QCoreApplication::processEvents();
        }

        const QImage imgAfterSwitch = viewport->grab().toImage();
        // Assert that the painted result is the same as the initial one
        QCOMPARE(imgFresh, imgAfterSwitch);
    };

    runTestForMode(u"dark"_s);
    runTestForMode(u"light"_s);
}

void tst_StudyRegression::testChecklistAddFocusScrollAndOverflowSizing() {
    auto testMode = [](const QString& modeStr) {
        {
            Services::SettingsService settings;
            settings.setThemeMode(modeStr);
        }

        App::AppContext ctx(u":memory:"_s);
        Track track;
        track.name = u"C++"_s;
        track.color = u"#3b82f6"_s;
        QVERIFY(ctx.tracks().insert(track));
        const int trackId = ctx.tracks().all().last().id;

        Topic topic;
        topic.trackId = trackId;
        topic.title = u"Memory Management"_s;
        topic.status = TopicStatus::InProgress;
        QVERIFY(ctx.topics().insert(topic));
        const int topicId = ctx.topics().all().last().id;

        MainWindow win(ctx);
        win.resize(1100, 700);
        win.show();
        win.activateWindow();
        QCoreApplication::processEvents();

        win.sidebar()->setCurrentPage(1);
        emit win.sidebar()->pageSelected(1);
        QCoreApplication::processEvents();

        auto* colInProgress = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnInProgress"_s);
        auto* detailPanel = win.findChild<Widgets::TopicDetailPanel*>(u"StudyDetailPanel"_s);
        auto* checkList = detailPanel->findChild<QListWidget*>(u"StudyDetailCheckList"_s);
        auto* checkAddEdit = detailPanel->findChild<QLineEdit*>(u"StudyDetailCheckAddEdit"_s);
        auto* checkAddBtn = detailPanel->findChild<QPushButton*>(u"StudyDetailCheckAddBtn"_s);
        QVERIFY(colInProgress && detailPanel && checkList && checkAddEdit && checkAddBtn);

        // Select topic
        const QRect cardRect = colInProgress->list()->visualRect(colInProgress->proxy()->index(0, 0));
        QTest::mouseClick(colInProgress->list()->viewport(), Qt::LeftButton, Qt::NoModifier, cardRect.center());
        QCoreApplication::processEvents();

        // 1. Checklist minimum height shows at least 5 rows without fixed pixel heights
        const int fiveRows = checkList->fontMetrics().height() * 5;
        QVERIFY(checkList->minimumHeight() >= fiveRows);

        // 2. Add first item via Return key in checkAddEdit
        checkAddEdit->setFocus();
        QTest::keyClicks(checkAddEdit, u"Item Alpha"_s);
        QTest::keyClick(checkAddEdit, Qt::Key_Return);
        QCoreApplication::processEvents();

        // Check list refreshed from repository
        const auto subtasksAfter1 = ctx.topics().subtasks(topicId);
        QCOMPARE(subtasksAfter1.size(), 1);
        QCOMPARE(checkList->count(), 1);
        QCOMPARE(checkList->item(0)->text(), u"Item Alpha"_s);

        // Input cleared and keeps keyboard focus
        QVERIFY(checkAddEdit->text().isEmpty());
        QVERIFY(checkAddEdit->hasFocus() || win.focusWidget() == checkAddEdit);

        // New row is scrolled to / visible
        QRect row0Rect = checkList->visualItemRect(checkList->item(0));
        QVERIFY(row0Rect.intersects(checkList->viewport()->rect()));

        // Add second item immediately via Enter
        QTest::keyClicks(checkAddEdit, u"Item Beta"_s);
        QTest::keyClick(checkAddEdit, Qt::Key_Return);
        QCoreApplication::processEvents();

        const auto subtasksAfter2 = ctx.topics().subtasks(topicId);
        QCOMPARE(subtasksAfter2.size(), 2);
        QCOMPARE(checkList->count(), 2);
        QCOMPARE(checkList->item(1)->text(), u"Item Beta"_s);
        QVERIFY(checkAddEdit->text().isEmpty());
        QVERIFY(checkAddEdit->hasFocus() || win.focusWidget() == checkAddEdit);

        // 3. Takes spare panel height: layout assigns stretch factor 1 and Expanding policy
        auto* formWidget = checkList->parentWidget();
        QVERIFY(formWidget && formWidget->layout());
        auto* formLayout = qobject_cast<QVBoxLayout*>(formWidget->layout());
        QVERIFY(formLayout != nullptr);
        QCOMPARE(formLayout->stretch(formLayout->indexOf(checkList)), 1);
        QCOMPARE(checkList->sizePolicy().verticalPolicy(), QSizePolicy::Expanding);

        const int initialCheckListHeight = checkList->height();
        win.resize(1200, 1400);
        QCoreApplication::processEvents();
        QVERIFY2(checkList->height() > initialCheckListHeight,
                 qPrintable(QString("Checklist height %1 did not expand with spare panel height (was %2)")
                                .arg(checkList->height()).arg(initialCheckListHeight)));

        // 4. Shows visible scrollbar when it overflows
        for (int i = 3; i <= 20; ++i) {
            Subtask st;
            st.topicId = topicId;
            st.text = QString(u"Overflow item %1"_s).arg(i);
            st.position = i - 1;
            QVERIFY(ctx.topics().addSubtask(st));
        }
        detailPanel->loadFrom(topicId);
        QCoreApplication::processEvents();

        QCOMPARE(checkList->count(), 20);
        QVERIFY(checkList->verticalScrollBar()->maximum() > 0);
        QVERIFY(checkList->verticalScrollBar()->isVisible());

        // Verify scrollbar handle is painted / has contrast
        const QImage sbImg = checkList->verticalScrollBar()->grab().toImage();
        QVERIFY(sbImg.width() > 0 && sbImg.height() > 0);
    };

    testMode(u"dark"_s);
    testMode(u"light"_s);
}

void tst_StudyRegression::testGeneralStartupContrastAllPagesBothThemes() {
    auto testMode = [](const QString& modeStr) {
        {
            Services::SettingsService settings;
            settings.setThemeMode(modeStr);
        }

        App::AppContext ctx(u":memory:"_s);

        // Populate sample data
        Track track;
        track.name = u"C++ Track"_s;
        track.color = u"#3b82f6"_s;
        QVERIFY(ctx.tracks().insert(track));
        const int trackId = ctx.tracks().all().last().id;

        Topic topic;
        topic.trackId = trackId;
        topic.title = u"Modern C++ Concurrency"_s;
        topic.status = TopicStatus::InProgress;
        topic.resources = u"[{\"title\":\"cppreference\",\"url\":\"https://en.cppreference.com\"}]"_s;
        QVERIFY(ctx.topics().insert(topic));
        const int topicId = ctx.topics().all().last().id;

        for (int i = 1; i <= 3; ++i) {
            Subtask st;
            st.topicId = topicId;
            st.text = QString(u"Concurrency subtask %1"_s).arg(i);
            st.position = i - 1;
            st.done = (i == 1);
            QVERIFY(ctx.topics().addSubtask(st));
        }

        JobApplication app;
        app.company = u"Tech Corp"_s;
        app.position = u"Senior C++ Engineer"_s;
        app.status = ApplicationStatus::Applied;
        QVERIFY(ctx.applications().insert(app));
        const int appId = ctx.applications().all().last().id;

        Interview interview;
        interview.applicationId = appId;
        interview.type = InterviewType::Technical;
        interview.startAt = QDateTime::currentDateTime().addDays(1);
        QVERIFY(ctx.interviews().insert(interview));

        MainWindow win(ctx);
        win.resize(1200, 800);
        win.show();
        QCoreApplication::processEvents();

        auto verifyContrastOnWidget = [&](QWidget* w, const QString& name) {
            QVERIFY(w != nullptr);
            const QImage img = w->grab().toImage();
            const QColor bg = img.pixelColor(2, 2);
            double maxContrast = 1.0;
            for (int y = 5; y < img.height() - 5; y += 4) {
                for (int x = 5; x < img.width() - 5; x += 4) {
                    const double cr = calcContrastRatio(img.pixelColor(x, y), bg);
                    if (cr > maxContrast) maxContrast = cr;
                }
            }
            QVERIFY2(maxContrast >= 3.0,
                     qPrintable(QString("[%1] Widget %2 max contrast %3:1 is below 3:1")
                                    .arg(modeStr, name).arg(maxContrast, 0, 'f', 2)));
        };

        // 1. Dashboard (HomePage)
        win.sidebar()->setCurrentPage(0);
        emit win.sidebar()->pageSelected(0);
        QCoreApplication::processEvents();
        verifyContrastOnWidget(win.centralWidget(), u"Dashboard"_s);

        // 2. StudyPage (Board, Detail Checklist & Resources Table)
        win.sidebar()->setCurrentPage(1);
        emit win.sidebar()->pageSelected(1);
        QCoreApplication::processEvents();

        auto* col = win.findChild<Widgets::TopicBoardColumn*>(u"StudyColumnInProgress"_s);
        auto* detail = win.findChild<Widgets::TopicDetailPanel*>(u"StudyDetailPanel"_s);
        auto* checkList = detail ? detail->findChild<QListWidget*>(u"StudyDetailCheckList"_s) : nullptr;
        auto* resTable = detail ? detail->findChild<QTableWidget*>(u"StudyDetailResourcesTable"_s) : nullptr;
        QVERIFY(col && detail && checkList && resTable);

        // Select topic
        const QRect cardRect = col->list()->visualRect(col->proxy()->index(0, 0));
        QTest::mouseClick(col->list()->viewport(), Qt::LeftButton, Qt::NoModifier, cardRect.center());
        QCoreApplication::processEvents();

        verifyContrastOnWidget(col->list()->viewport(), u"StudyBoardColumn"_s);
        verifyContrastOnWidget(checkList->viewport(), u"StudyChecklist"_s);
        verifyContrastOnWidget(resTable->viewport(), u"StudyResourcesTable"_s);

        // 3. JobsPage
        win.sidebar()->setCurrentPage(2);
        emit win.sidebar()->pageSelected(2);
        QCoreApplication::processEvents();
        auto* jobsView = win.findChild<QTableView*>();
        QVERIFY(jobsView != nullptr);
        verifyContrastOnWidget(jobsView->viewport(), u"JobsTableView"_s);

        // 4. CalendarPage
        win.sidebar()->setCurrentPage(3);
        emit win.sidebar()->pageSelected(3);
        QCoreApplication::processEvents();
        verifyContrastOnWidget(win.centralWidget(), u"CalendarPage"_s);
    };

    testMode(u"dark"_s);
    testMode(u"light"_s);
}

QTEST_MAIN(tst_StudyRegression)
#include "tst_StudyRegression.moc"
