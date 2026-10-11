#include <QtTest>
#include <QLineEdit>
#include <QSignalSpy>
#include <QVBoxLayout>
#include "services/SettingsService.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/MonthView.h"

using namespace Qt::StringLiterals;

namespace {

using JobPrep::Ui::Widgets::MonthView;

const QDate kToday(2026, 10, 11);

/// ThemeManager needs a live Application, which QTEST_MAIN provides; build one fresh so
/// each slot starts from a clean palette.
struct CalendarFixture {
    JobPrep::Services::SettingsService settings;
    JobPrep::Ui::Theme::ThemeManager theme{settings};
};

MonthView* makeView(JobPrep::Ui::Theme::ThemeManager& theme,
                    std::function<QDate()> provider) {
    auto* view = new MonthView(theme, std::move(provider));
    view->resize(640, 460);
    view->show();
    QCoreApplication::processEvents();
    return view;
}

}  // namespace

class tst_MonthView : public QObject {
    Q_OBJECT

private slots:
    void gridStartsOnMondayFirstAndSundayFirst();
    void february2028Grid();
    void monthBeginningOnFirstWeekday();
    void todayFromProviderAndInitialSelection();
    void clickingOutOfMonthDayFollowsMonth();
    void keyboardNavigation();
    void homeEndSelectWeekBounds();
    void tabFocusEntersAndLeaves();
    void pageUpDownMoveMonth();
    void activateKeysEmitDayActivated();
    void doubleClickEmitsDayActivated();
    void markersOnTrailingDimmedDay();
    void dayRectNullOutsideGrid();
};

void tst_MonthView::gridStartsOnMondayFirstAndSundayFirst() {
    CalendarFixture fixture;
    MonthView* view = makeView(fixture.theme, [] { return kToday; });
    QVERIFY(view != nullptr);
    view->setVisibleMonth(QDate(2023, 10, 15));

    // Monday-first: the week containing 1 Oct 2023 starts on Mon 25 Sep 2023.
    view->setFirstDayOfWeek(Qt::Monday);
    auto range = view->visibleRange();
    QCOMPARE(range.first, QDate(2023, 9, 25));
    QCOMPARE(range.second, QDate(2023, 11, 5));

    // Sunday-first: the same month starts exactly on the first weekday.
    view->setFirstDayOfWeek(Qt::Sunday);
    range = view->visibleRange();
    QCOMPARE(range.first, QDate(2023, 10, 1));
    QCOMPARE(range.second, QDate(2023, 11, 11));

    delete view;
}

void tst_MonthView::february2028Grid() {
    CalendarFixture fixture;
    MonthView* view = makeView(fixture.theme, [] { return kToday; });
    view->setFirstDayOfWeek(Qt::Monday);
    view->setVisibleMonth(QDate(2028, 2, 29));

    // 1 Feb 2028 is a Tuesday; the week starts Mon 31 Jan 2028.
    const auto range = view->visibleRange();
    QCOMPARE(range.first, QDate(2028, 1, 31));
    QCOMPARE(QDate(2028, 2, 29).dayOfWeek(), 2);

    delete view;
}

void tst_MonthView::monthBeginningOnFirstWeekday() {
    CalendarFixture fixture;
    MonthView* view = makeView(fixture.theme, [] { return kToday; });
    view->setFirstDayOfWeek(Qt::Monday);
    view->setVisibleMonth(QDate(2024, 1, 10));  // 1 Jan 2024 is a Monday
    QCOMPARE(QDate(2024, 1, 1).dayOfWeek(), 1);
    QCOMPARE(view->visibleRange().first, QDate(2024, 1, 1));

    delete view;
}

void tst_MonthView::todayFromProviderAndInitialSelection() {
    CalendarFixture fixture;
    MonthView* view = makeView(fixture.theme, [] { return QDate(2026, 10, 11); });
    QCOMPARE(view->today(), QDate(2026, 10, 11));
    QCOMPARE(view->selectedDay(), QDate(2026, 10, 11));
    QCOMPARE(view->visibleMonth(), QDate(2026, 10, 1));

    delete view;
}

void tst_MonthView::clickingOutOfMonthDayFollowsMonth() {
    CalendarFixture fixture;
    MonthView* view = makeView(fixture.theme, [] { return kToday; });
    view->setSelectedDay(QDate(2026, 10, 11));
    QSignalSpy monthSpy(view, &MonthView::monthChanged);
    QSignalSpy selectionSpy(view, &MonthView::selectionChanged);

    // 2026-11-03 is a trailing dimmed day of the October grid; clicking it follows month.
    const QRect trailingRect = view->dayRect(QDate(2026, 11, 3));
    QVERIFY(trailingRect.isValid());
    QTest::mouseClick(view, Qt::LeftButton, Qt::KeyboardModifiers(), trailingRect.center());

    QCOMPARE(view->selectedDay(), QDate(2026, 11, 3));
    QCOMPARE(view->visibleMonth(), QDate(2026, 11, 1));
    QCOMPARE(monthSpy.count(), 1);
    QCOMPARE(monthSpy.takeFirst().at(0).toDate(), QDate(2026, 11, 1));
    QCOMPARE(selectionSpy.count(), 1);

    delete view;
}

void tst_MonthView::keyboardNavigation() {
    CalendarFixture fixture;
    MonthView* view = makeView(fixture.theme, [] { return kToday; });
    view->setSelectedDay(QDate(2026, 10, 11));
    view->setFocus();
    QTEST_ASSERT(view->hasFocus());

    QTest::keyClick(view, Qt::Key_Right);
    QCOMPARE(view->selectedDay(), QDate(2026, 10, 12));
    QTest::keyClick(view, Qt::Key_Left);
    QCOMPARE(view->selectedDay(), QDate(2026, 10, 11));
    QTest::keyClick(view, Qt::Key_Down);
    QCOMPARE(view->selectedDay(), QDate(2026, 10, 18));
    QTest::keyClick(view, Qt::Key_Up);
    QCOMPARE(view->selectedDay(), QDate(2026, 10, 11));

    delete view;
}

void tst_MonthView::homeEndSelectWeekBounds() {
    CalendarFixture fixture;
    MonthView* view = makeView(fixture.theme, [] { return kToday; });

    // Monday-first: week of Sun 11 Oct 2026 runs Mon 5 Oct .. Sun 11 Oct.
    view->setFirstDayOfWeek(Qt::Monday);
    view->setSelectedDay(QDate(2026, 10, 11));
    view->setFocus();
    QTest::keyClick(view, Qt::Key_Home);
    QCOMPARE(view->selectedDay(), QDate(2026, 10, 5));
    QTest::keyClick(view, Qt::Key_End);
    QCOMPARE(view->selectedDay(), QDate(2026, 10, 11));

    // Sunday-first: the same day is Sunday, so it starts its own week.
    view->setFirstDayOfWeek(Qt::Sunday);
    view->setSelectedDay(QDate(2026, 10, 11));
    QTest::keyClick(view, Qt::Key_Home);
    QCOMPARE(view->selectedDay(), QDate(2026, 10, 11));
    QTest::keyClick(view, Qt::Key_End);
    QCOMPARE(view->selectedDay(), QDate(2026, 10, 17));

    delete view;
}

void tst_MonthView::tabFocusEntersAndLeaves() {
    CalendarFixture fixture;
    QWidget host;
    auto* layout = new QVBoxLayout(&host);
    auto* before = new QLineEdit(&host);
    auto* view = new MonthView(fixture.theme, [] { return kToday; }, &host);
    auto* after = new QLineEdit(&host);
    layout->addWidget(before);
    layout->addWidget(view);
    layout->addWidget(after);
    host.resize(640, 520);
    host.show();
    QCoreApplication::processEvents();

    // MonthView opts into the tab chain (StrongFocus includes TabFocus).
    QVERIFY(view->focusPolicy() & Qt::TabFocus);

    before->setFocus();
    QVERIFY(before->hasFocus());
    QTest::keyClick(before, Qt::Key_Tab);
    QVERIFY(view->hasFocus());

    QTest::keyClick(view, Qt::Key_Tab);
    QVERIFY(after->hasFocus());
    QTest::keyClick(after, Qt::Key_Backtab);
    QVERIFY(view->hasFocus());
}

void tst_MonthView::pageUpDownMoveMonth() {
    CalendarFixture fixture;
    MonthView* view = makeView(fixture.theme, [] { return kToday; });
    view->setSelectedDay(QDate(2026, 10, 11));
    QSignalSpy monthSpy(view, &MonthView::monthChanged);

    QTest::keyClick(view, Qt::Key_PageDown);
    QCOMPARE(view->selectedDay(), QDate(2026, 11, 11));
    QCOMPARE(view->visibleMonth(), QDate(2026, 11, 1));
    QCOMPARE(monthSpy.count(), 1);

    QTest::keyClick(view, Qt::Key_PageUp);
    QCOMPARE(view->selectedDay(), QDate(2026, 10, 11));
    QCOMPARE(view->visibleMonth(), QDate(2026, 10, 1));

    delete view;
}

void tst_MonthView::activateKeysEmitDayActivated() {
    CalendarFixture fixture;
    MonthView* view = makeView(fixture.theme, [] { return kToday; });
    view->setSelectedDay(QDate(2026, 10, 11));
    QSignalSpy spy(view, &MonthView::dayActivated);

    view->setFocus();
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toDate(), QDate(2026, 10, 11));

    QTest::keyClick(view, Qt::Key_Space);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toDate(), QDate(2026, 10, 11));

    delete view;
}

void tst_MonthView::doubleClickEmitsDayActivated() {
    CalendarFixture fixture;
    MonthView* view = makeView(fixture.theme, [] { return kToday; });
    view->setVisibleMonth(QDate(2026, 10, 1));
    QSignalSpy spy(view, &MonthView::dayActivated);

    const QDate day(2026, 10, 12);
    QTest::mouseDClick(view, Qt::LeftButton, Qt::KeyboardModifiers(), view->dayRect(day).center());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toDate(), day);

    delete view;
}

void tst_MonthView::markersOnTrailingDimmedDay() {
    CalendarFixture fixture;
    MonthView* view = makeView(fixture.theme, [] { return kToday; });
    view->setVisibleMonth(QDate(2026, 10, 1));

    QHash<QDate, MonthView::DayMarkers> markers;
    MonthView::DayMarkers trailing;
    trailing.kinds = {JobPrep::Domain::AgendaKind::Interview,
                      JobPrep::Domain::AgendaKind::FollowUp};
    trailing.total = 5;
    markers.insert(QDate(2026, 11, 3), trailing);
    view->setMarkers(markers);

    const auto read = view->markersFor(QDate(2026, 11, 3));
    QCOMPARE(read.total, 5);
    QCOMPARE(read.kinds.size(), 2);
    QCOMPARE(read.kinds.first(), JobPrep::Domain::AgendaKind::Interview);
    QVERIFY(view->dayRect(QDate(2026, 11, 3)).isValid());

    delete view;
}

void tst_MonthView::dayRectNullOutsideGrid() {
    CalendarFixture fixture;
    MonthView* view = makeView(fixture.theme, [] { return kToday; });
    view->setVisibleMonth(QDate(2026, 10, 1));
    QVERIFY(view->dayRect(QDate(2026, 12, 1)).isNull());
    QVERIFY(view->dayRect(QDate(2026, 9, 20)).isNull());

    delete view;
}

QTEST_MAIN(tst_MonthView)
#include "tst_MonthView.moc"